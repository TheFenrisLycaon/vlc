/*****************************************************************************
 * fetcher.c
 *****************************************************************************
 * Copyright © 2017-2017 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_atomic.h>
#include <heimdall_stream.h>
#include <heimdall_modules.h>
#include <heimdall_interrupt.h>
#include <heimdall_arrays.h>
#include <heimdall_threads.h>
#include <heimdall_memstream.h>
#include <heimdall_meta_fetcher.h>
#include <heimdall_executor.h>

#include "art.h"
#include "../libheimdall.h"
#include "fetcher.h"
#include "input/input_interface.h"
#include "misc/interrupt.h"

struct input_fetcher_t {
    heimdall_executor_t *executor_local;
    heimdall_executor_t *executor_network;
    heimdall_executor_t *executor_downloader;

    heimdall_dictionary_t album_cache;
    heimdall_object_t* owner;

    heimdall_mutex_t lock;
    struct heimdall_list submitted_tasks; /**< list of struct task */
};

struct task {
    input_fetcher_t *fetcher;
    heimdall_executor_t *executor;
    input_item_t* item;
    int options;
    const input_fetcher_callbacks_t *cbs;
    void *userdata;

    heimdall_interrupt_t interrupt;

    struct heimdall_runnable runnable; /**< to be passed to the executor */
    struct heimdall_list node; /**< node of input_fetcher_t.submitted_tasks */
};

static void RunDownloader(void *);
static void RunSearchLocal(void *);
static void RunSearchNetwork(void *);

static struct task *
TaskNew(input_fetcher_t *fetcher, heimdall_executor_t *executor, input_item_t *item,
        int options,
        const input_fetcher_callbacks_t *cbs, void *userdata)
{
    struct task *task = malloc(sizeof(*task));
    if (!task)
        return NULL;

    task->fetcher = fetcher;
    task->executor = executor;
    task->item = item;
    task->options = options;
    task->cbs = cbs;
    task->userdata = userdata;

    heimdall_interrupt_init(&task->interrupt);

    input_item_Hold(item);

    if (executor == fetcher->executor_local)
        task->runnable.run = RunSearchLocal;
    else if (executor == fetcher->executor_network)
        task->runnable.run = RunSearchNetwork;
    else
        task->runnable.run = RunDownloader;

    task->runnable.userdata = task;

    return task;
}

static void
TaskDelete(struct task *task)
{
    input_item_Release(task->item);
    heimdall_interrupt_deinit(&task->interrupt);
    free(task);
}

static void
FetcherAddTask(input_fetcher_t *fetcher, struct task *task)
{
    heimdall_mutex_lock(&fetcher->lock);
    heimdall_list_append(&task->node, &fetcher->submitted_tasks);
    heimdall_mutex_unlock(&fetcher->lock);
}

static void
FetcherRemoveTask(input_fetcher_t *fetcher, struct task *task)
{
    heimdall_mutex_lock(&fetcher->lock);
    heimdall_list_remove(&task->node);
    heimdall_mutex_unlock(&fetcher->lock);
}

static int
Submit(input_fetcher_t *fetcher, heimdall_executor_t *executor, input_item_t *item,
       int options,
       const input_fetcher_callbacks_t *cbs, void *userdata)
{
    struct task *task =
        TaskNew(fetcher, executor, item, options, cbs, userdata);
    if (!task)
        return HEIMDALL_ENOMEM;

    FetcherAddTask(fetcher, task);
    heimdall_executor_Submit(task->executor, &task->runnable);

    return HEIMDALL_SUCCESS;
}

static char* CreateCacheKey( input_item_t* item )
{
    heimdall_mutex_lock( &item->lock );

    if( !item->p_meta )
    {
        heimdall_mutex_unlock( &item->lock );
        return NULL;
    }

    char const* artist = heimdall_meta_Get( item->p_meta, heimdall_meta_Artist );
    char const* album = heimdall_meta_Get( item->p_meta, heimdall_meta_Album );
    char const *date = heimdall_meta_Get( item->p_meta, heimdall_meta_Date );
    char* key;

    /**
     * Simple concatenation of artist and album can lead to the same key
     * for entities that should not have such. Imagine { dogs, tick } and
     * { dog, stick } */
    if( !artist || !album || asprintf( &key, "%s:%zu:%s:%zu:%s",
          artist, strlen( artist ), album, strlen( album ),
          date ? date : "0000" ) < 0 )
    {
        key = NULL;
    }
    heimdall_mutex_unlock( &item->lock );

    return key;
}

static void FreeCacheEntry( void* data, void* obj )
{
    free( data );
    HEIMDALL_UNUSED( obj );
}

static int ReadAlbumCache( input_fetcher_t* fetcher, input_item_t* item )
{
    char* key = CreateCacheKey( item );

    if( key == NULL )
        return HEIMDALL_EGENERIC;

    heimdall_mutex_lock( &fetcher->lock );
    char const* art = heimdall_dictionary_value_for_key( &fetcher->album_cache,
                                                    key );
    if( art )
        input_item_SetArtURL( item, art );
    heimdall_mutex_unlock( &fetcher->lock );

    free( key );
    return art ? HEIMDALL_SUCCESS : HEIMDALL_EGENERIC;
}

static void AddAlbumCache( input_fetcher_t* fetcher, input_item_t* item,
                           bool overwrite )
{
    char* art = input_item_GetArtURL( item );
    char* key = CreateCacheKey( item );

    if( key && art && strncasecmp( art, "attachment://", 13 ) )
    {
        heimdall_mutex_lock( &fetcher->lock );
        if( overwrite || !heimdall_dictionary_has_key( &fetcher->album_cache, key ) )
        {
            heimdall_dictionary_insert( &fetcher->album_cache, key, art );
            art = NULL;
        }
        heimdall_mutex_unlock( &fetcher->lock );
    }

    free( art );
    free( key );
}

static int InvokeModule( input_fetcher_t* fetcher, input_item_t* item,
                         int scope, char const* type )
{
    meta_fetcher_t* mf = heimdall_custom_create( fetcher->owner,
                                            sizeof( *mf ), type );
    if( unlikely( !mf ) )
        return HEIMDALL_ENOMEM;

    mf->e_scope = scope;
    mf->p_item = item;

    module_t* mf_module = module_need( mf, type, NULL, false );

    if( mf_module )
        module_unneed( mf, mf_module );

    heimdall_object_delete(mf);

    return HEIMDALL_SUCCESS;
}

static int CheckMeta( input_item_t* item )
{
    heimdall_mutex_lock( &item->lock );
    bool error = !item->p_meta ||
                 !heimdall_meta_Get( item->p_meta, heimdall_meta_Title ) ||
                 !heimdall_meta_Get( item->p_meta, heimdall_meta_Artist ) ||
                 !heimdall_meta_Get( item->p_meta, heimdall_meta_Album );
    heimdall_mutex_unlock( &item->lock );
    return error;
}

static int CheckArt( input_item_t* item )
{
    heimdall_mutex_lock( &item->lock );
    bool error = !item->p_meta ||
                 !heimdall_meta_Get( item->p_meta, heimdall_meta_ArtworkURL );
    heimdall_mutex_unlock( &item->lock );
    return error;
}

static int SearchArt( input_fetcher_t* fetcher, input_item_t* item, int scope)
{
    InvokeModule( fetcher, item, scope, "art finder" );
    return CheckArt( item );
}

static int SearchByScope(struct task *task, int scope)
{
    input_fetcher_t *fetcher = task->fetcher;
    input_item_t* item = task->item;

    if( CheckMeta( item ) &&
        InvokeModule( fetcher, item, scope, "meta fetcher" ) )
    {
        return HEIMDALL_EGENERIC;
    }

    if( ! CheckArt( item )                         ||
        ! ReadAlbumCache( fetcher, item )          ||
        ! input_FindArtInCacheUsingItemUID( item ) ||
        ! input_FindArtInCache( item )             ||
        ! SearchArt( fetcher, item, scope ) )
    {
        AddAlbumCache( fetcher, task->item, false );
        int ret = Submit(fetcher, fetcher->executor_downloader, item,
                         task->options, task->cbs, task->userdata);
        if (ret == HEIMDALL_SUCCESS)
            return HEIMDALL_SUCCESS;
    }

    return HEIMDALL_EGENERIC;
}

static void NotifyArtFetchEnded(struct task *task, bool fetched)
{
    if (task->cbs && task->cbs->on_art_fetch_ended)
        task->cbs->on_art_fetch_ended(task->item, fetched, task->userdata);
}

static void RunDownloader(void *userdata)
{
    heimdall_thread_set_name("heimdall-run-fetcher");

    bool fetched = false;
    struct task *task = userdata;
    input_fetcher_t *fetcher = task->fetcher;

    heimdall_interrupt_set(&task->interrupt);

    ReadAlbumCache( fetcher, task->item );

    char *psz_arturl = input_item_GetArtURL( task->item );
    if( !psz_arturl )
        goto out;

    if( !strncasecmp( psz_arturl, "file://", 7 ) ||
        !strncasecmp( psz_arturl, "attachment://", 13 ) )
    {
        fetched = true;
        goto out; /* no fetch required */
    }

    stream_t* source = heimdall_stream_NewURL( fetcher->owner, psz_arturl );

    if( !source )
        goto out;

    struct heimdall_memstream output_stream;
    heimdall_memstream_open( &output_stream );

    for( ;; )
    {
        char buffer[2048];

        int read = heimdall_stream_Read( source, buffer, sizeof( buffer ) );
        if( read <= 0 )
            break;

        if( (int)heimdall_memstream_write( &output_stream, buffer, read ) < read )
            break;
    }

    heimdall_stream_Delete( source );

    if( heimdall_memstream_close( &output_stream ) )
        goto out;

    if( heimdall_killed() )
    {
        free( output_stream.ptr );
        goto out;
    }

    input_SaveArt( fetcher->owner, task->item, output_stream.ptr,
                   output_stream.length, NULL );

    free( output_stream.ptr );
    AddAlbumCache( fetcher, task->item, true );
    fetched = true;

out:
    heimdall_interrupt_set(NULL);

    if( fetched )
        input_item_SetArtFetched( task->item, true );
    free( psz_arturl );
    NotifyArtFetchEnded(task, fetched);
    FetcherRemoveTask(fetcher, task);
    TaskDelete(task);
}

static void RunSearchLocal(void *userdata)
{
    heimdall_thread_set_name("heimdall-run-searchl");

    struct task *task = userdata;
    input_fetcher_t *fetcher = task->fetcher;

    heimdall_interrupt_set(&task->interrupt);

    if( SearchByScope( task, FETCHER_SCOPE_LOCAL ) == HEIMDALL_SUCCESS )
        goto end; /* done */

    if( task->options & HEIMDALL_PREPARSER_TYPE_FETCHMETA_NET )
    {
        int ret = Submit(fetcher, fetcher->executor_network, task->item,
                         task->options, task->cbs, task->userdata);
        if (ret != HEIMDALL_SUCCESS)
            NotifyArtFetchEnded(task, false);
    }
    else
    {
        input_item_SetArtNotFound( task->item, true );
        NotifyArtFetchEnded(task, false);
    }

end:
    heimdall_interrupt_set(NULL);

    FetcherRemoveTask(fetcher, task);
    TaskDelete(task);
}

static void RunSearchNetwork(void *userdata)
{
    heimdall_thread_set_name("heimdall-run-searchn");

    struct task *task = userdata;

    heimdall_interrupt_set(&task->interrupt);

    if( SearchByScope( task, FETCHER_SCOPE_NETWORK ) != HEIMDALL_SUCCESS )
    {
        input_item_SetArtNotFound( task->item, true );
        NotifyArtFetchEnded(task, false);
    }

    heimdall_interrupt_set(NULL);

    input_fetcher_t *fetcher = task->fetcher;
    FetcherRemoveTask(fetcher, task);
    TaskDelete(task);
}

input_fetcher_t* input_fetcher_New( heimdall_object_t* owner,
                                    int request_type )
{
    assert(request_type & HEIMDALL_PREPARSER_TYPE_FETCHMETA_ALL);
    input_fetcher_t* fetcher = malloc( sizeof( *fetcher ) );

    if( unlikely( !fetcher ) )
        return NULL;

    int max_threads = var_InheritInteger(owner, "fetch-art-threads");
    if (max_threads < 1)
        max_threads = 1;

    if (request_type & HEIMDALL_PREPARSER_TYPE_FETCHMETA_LOCAL)
    {
        fetcher->executor_local = heimdall_executor_New(max_threads);
        if (!fetcher->executor_local)
        {
            free(fetcher);
            return NULL;
        }
    }
    else
        fetcher->executor_local = NULL;

    if (request_type & HEIMDALL_PREPARSER_TYPE_FETCHMETA_NET)
    {
        fetcher->executor_network = heimdall_executor_New(max_threads);
        if (!fetcher->executor_network)
        {
            if (fetcher->executor_local)
                heimdall_executor_Delete(fetcher->executor_local);
            free(fetcher);
            return NULL;
        }

    }
    else
        fetcher->executor_network = NULL;

    fetcher->executor_downloader = heimdall_executor_New(max_threads);
    if (!fetcher->executor_downloader)
    {
        if (fetcher->executor_network)
            heimdall_executor_Delete(fetcher->executor_network);
        if (fetcher->executor_local)
            heimdall_executor_Delete(fetcher->executor_local);
        free(fetcher);
        return NULL;
    }

    fetcher->owner = owner;

    heimdall_mutex_init(&fetcher->lock);
    heimdall_list_init(&fetcher->submitted_tasks);

    heimdall_dictionary_init( &fetcher->album_cache, 0 );

    return fetcher;
}

int input_fetcher_Push(input_fetcher_t* fetcher, input_item_t* item,
                       int options,
                       const input_fetcher_callbacks_t *cbs, void *cbs_userdata)
{
    assert(options & HEIMDALL_PREPARSER_TYPE_FETCHMETA_ALL);
    if (options & HEIMDALL_PREPARSER_TYPE_FETCHMETA_LOCAL)
        assert(fetcher->executor_local != NULL);
    if (options & HEIMDALL_PREPARSER_TYPE_FETCHMETA_NET)
        assert(fetcher->executor_network != NULL);

    heimdall_executor_t *executor = options & HEIMDALL_PREPARSER_TYPE_FETCHMETA_LOCAL
                             ? fetcher->executor_local
                             : fetcher->executor_network;

    return Submit(fetcher, executor, item, options, cbs, cbs_userdata);
}

static void
CancelAllTasks(input_fetcher_t *fetcher)
{
    heimdall_mutex_lock(&fetcher->lock);

    struct task *task;
    heimdall_list_foreach(task, &fetcher->submitted_tasks, node)
    {
        bool canceled = heimdall_executor_Cancel(task->executor, &task->runnable);
        if (canceled)
        {
            NotifyArtFetchEnded(task, false);
            heimdall_list_remove(&task->node);
            TaskDelete(task);
        }
        /* Otherwise, the task will be finished and destroyed after run() */
    }

    heimdall_mutex_unlock(&fetcher->lock);
}

void input_fetcher_Delete( input_fetcher_t* fetcher )
{
    CancelAllTasks(fetcher);

    if (fetcher->executor_local)
        heimdall_executor_Delete(fetcher->executor_local);
    if (fetcher->executor_network)
        heimdall_executor_Delete(fetcher->executor_network);
    assert(fetcher->executor_downloader);
    heimdall_executor_Delete(fetcher->executor_downloader);

    heimdall_dictionary_clear( &fetcher->album_cache, FreeCacheEntry, NULL );
    free( fetcher );
}
