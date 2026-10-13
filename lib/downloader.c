/*****************************************************************************
 * downloader.c: LibHeimdall Downloader API
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
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
#include "config.h"
#endif

#include <heimdall/libheimdall_downloader.h>

#include <heimdall_access.h>
#include <heimdall_interrupt.h>
#include <heimdall_stream.h>
#include <heimdall_threads.h>

#include "libheimdall_internal.h"

#define BUFFER_SIZE 65536

struct libheimdall_downloader_thread
{
    /* Node of libheimdall_downloader_t.threads list */
    struct heimdall_list node;

    /* The task thread */
    heimdall_thread_t thread;

    /* Thread termination state */
    bool terminated;
};

struct libheimdall_downloader_t
{
    libheimdall_instance_t *libheimdall;
    heimdall_mutex_t lock;
    libheimdall_parser_t *parser;

    /* list of ongoing tasks (terminated tasks are removed) */
    struct heimdall_list submitted_tasks;

    /* list of downloader threads (dead threads joined at
       libheimdall_downloader_submit, all threads are joined at
       libheimdall_downloader_destroy) */
    struct heimdall_list threads;
};

struct libheimdall_downloader_task
{
    libheimdall_downloader_t *downloader;
    struct heimdall_list node;
    libheimdall_downloader_status_t status;

    libheimdall_parser_task *parser_task;

    libheimdall_media_t *media;
    stream_t *s;

    const struct libheimdall_downloader_cbs *cbs;
    void *cbs_opaque;

    /* interrupt context (to interrupt blocking read) */
    heimdall_interrupt_t *interrupt;

    /* required for pause/resume */
    bool pause_requested;
    heimdall_cond_t interrupt_cond;

    /* required for ongoing task abortion */
    bool interrupted;

    /* guard against a re-submission of the same handle */
    bool submitted;

    heimdall_atomic_rc_t rc;

    struct libheimdall_downloader_thread *thread;
};

/* Create a new downloader task */
static struct libheimdall_downloader_task *
DownloaderTaskNew(libheimdall_downloader_t *downloader, libheimdall_media_t *media,
                  const struct libheimdall_downloader_cbs *cbs, void *cbs_opaque)
{
    struct libheimdall_downloader_task *task = malloc(sizeof(*task));
    if (task == NULL)
        return NULL;

    task->interrupt = heimdall_interrupt_create();
    if (task->interrupt == NULL)
    {
        free(task);
        return NULL;
    }

    task->downloader = downloader;
    task->media = libheimdall_media_retain(media);
    task->s = NULL;
    task->cbs = cbs;
    task->cbs_opaque = cbs_opaque;
    task->pause_requested = false;
    heimdall_cond_init(&task->interrupt_cond);
    task->interrupted = false;
    task->submitted = false;
    task->status = libheimdall_downloader_status_pending;
    task->parser_task = NULL;
    task->thread = NULL;

    heimdall_atomic_rc_init(&task->rc);

    return task;
}

/* Delete a downloader task */
static void
DownloaderTaskDestroy(struct libheimdall_downloader_task *task)
{
    libheimdall_media_release(task->media);
    heimdall_interrupt_destroy(task->interrupt);

    if (task->parser_task)
        libheimdall_parser_task_release(task->parser_task);

    free(task);
}

/* Interrupt a running downloader task */
static void
DownloaderTaskInterruptLocked(struct libheimdall_downloader_task *task)
{
    task->interrupted = true;
    heimdall_interrupt_kill(task->interrupt);
    heimdall_cond_signal(&task->interrupt_cond);
}

/* report downloader state changes */
static void libheimdall_downloader_task_ReportStateLocked(struct libheimdall_downloader_task *task,
                                                     libheimdall_downloader_status_t new_status)
{
    bool status_changed = false;

    if (task->interrupted)
        new_status = libheimdall_downloader_status_cancelled;

    libheimdall_downloader_status_t old_status = task->status;
    if (old_status != new_status)
    {
        task->status = new_status;
        status_changed = true;
    }

    /* report on_state_update only when the state changed */
    if (status_changed)
        task->cbs->on_state_update(task->cbs_opaque, task, new_status);
}

static void *
downloaderThread(void *data)
{
    heimdall_thread_set_name("downloader");
    struct libheimdall_downloader_task *task = data;
    libheimdall_downloader_t *downloader = task->downloader;
    libheimdall_downloader_status_t status = libheimdall_downloader_status_error;
    uint8_t *buf = NULL;

    heimdall_interrupt_set(task->interrupt);

    char *mrl = libheimdall_media_get_mrl(task->media);
    if (mrl == NULL)
        goto cleanup;

    task->s = heimdall_access_NewMRL(HEIMDALL_OBJECT(downloader->libheimdall->p_libheimdall_int), mrl);
    free(mrl);

    if (task->s == NULL || !heimdall_stream_CanDownload(task->s))
        goto cleanup;

    uint64_t stream_size = 0;
    /* only files with valid size allowed */
    if (heimdall_stream_GetSize(task->s, &stream_size) != HEIMDALL_SUCCESS)
        goto cleanup;

    buf = malloc(BUFFER_SIZE);
    if (buf == NULL)
        goto cleanup;

    uint64_t total_read = 0;
    size_t buf_len = 0;
    size_t consumed_offset = 0;
    status = libheimdall_downloader_status_cancelled;

    while (true)
    {
        heimdall_mutex_lock(&downloader->lock);
        if (task->interrupted)
            goto cleanup_locked;

        /* Pause handling */
        if (task->pause_requested)
        {
            libheimdall_downloader_task_ReportStateLocked(task, libheimdall_downloader_status_paused);
            heimdall_cond_wait(&task->interrupt_cond, &downloader->lock);
            heimdall_mutex_unlock(&downloader->lock);
            continue;
        }

        libheimdall_downloader_task_ReportStateLocked(task, libheimdall_downloader_status_running);

        /* fetch next chunk only when the previous one is fully drained */
        if (consumed_offset == buf_len)
        {
            heimdall_mutex_unlock(&downloader->lock);

            ssize_t read = heimdall_stream_ReadPartial(task->s, buf, BUFFER_SIZE);

            if (read < 0)
                continue;

            heimdall_mutex_lock(&downloader->lock);
            /* check whether task was interrupted during blocking read operation */
            if (task->interrupted)
                goto cleanup_locked;

            if (read == 0) /* EOF */
            {
                status = libheimdall_downloader_status_finished;
                goto cleanup_locked;
            }

            buf_len = (size_t)read;
            consumed_offset = 0;
            total_read += (uint64_t)read;

            /* update size in case it changed; a valid size is required throughout */
            if (heimdall_stream_GetSize(task->s, &stream_size) != HEIMDALL_SUCCESS)
            {
                status = libheimdall_downloader_status_error;
                goto cleanup_locked;
            }

            /* check whether the user requested pause during the blocking read. In that case,
               the data is buffered and download is paused. */
            if (task->pause_requested)
            {
                heimdall_mutex_unlock(&downloader->lock);
                continue;
            }
        }

        /* deliver the residual (either just fetched or left over after a partial read) */
        const uint8_t *chunk = buf + consumed_offset;
        size_t remaining = buf_len - consumed_offset;

        ptrdiff_t consumed = task->cbs->on_buffer(task->cbs_opaque, task, chunk,
                                                  remaining, total_read, stream_size);

        if (consumed == LIBHEIMDALL_DOWNLOADER_CB_CANCEL)
            goto cleanup_locked;

        if (consumed < 0 || (size_t)consumed > remaining)
        {
            status = libheimdall_downloader_status_error;
            goto cleanup_locked;
        }

        consumed_offset += (size_t)consumed;

        /* partial read: auto-pause as backpressure */
        if ((size_t)consumed < remaining)
            task->pause_requested = true;

        heimdall_mutex_unlock(&downloader->lock);
    }

cleanup:
    heimdall_mutex_lock(&downloader->lock);
cleanup_locked:
    libheimdall_downloader_task_ReportStateLocked(task, status);
    heimdall_list_remove(&task->node);
    task->thread->terminated = true;
    heimdall_mutex_unlock(&downloader->lock);
    free(buf);
    if (task->s != NULL)
        heimdall_stream_Delete(task->s);

    libheimdall_downloader_task_release(task);
    return NULL;
}

libheimdall_downloader_t *
libheimdall_downloader_new(libheimdall_instance_t *inst, const struct libheimdall_downloader_cfg *cfg)
{
    assert(inst != NULL);
    assert(cfg != NULL);

    /* No different versions to handle for now */
    assert(cfg->version <= 0);

    libheimdall_downloader_t *downloader = malloc(sizeof(*downloader));
    if (downloader == NULL)
        return NULL;

    const struct libheimdall_parser_cfg parser_cfg = {
        .version = 0,
        .max_parser_threads = cfg->max_parser_threads,
        .timeout = 0,
    };

    downloader->parser = libheimdall_parser_new(inst, &parser_cfg);
    if (downloader->parser == NULL)
    {
        free(downloader);
        return NULL;
    }

    downloader->libheimdall = libheimdall_retain(inst);
    heimdall_mutex_init(&downloader->lock);
    heimdall_list_init(&downloader->submitted_tasks);
    heimdall_list_init(&downloader->threads);
    return downloader;
}

static void notify_subitems(const struct libheimdall_downloader_cbs *cbs, void *cbs_opaque,
                            libheimdall_downloader_task *task)
{
    libheimdall_media_list_t *mlist = libheimdall_media_subitems(task->media);
    if (mlist == NULL)
        return;

    libheimdall_media_list_lock(mlist);
    int count = libheimdall_media_list_count(mlist);
    libheimdall_media_list_unlock(mlist);

    if (count > 0)
        cbs->on_subitems(cbs_opaque, task, mlist);

    libheimdall_media_list_release(mlist);
}

static void notify_slaves(const struct libheimdall_downloader_cbs *cbs, void *cbs_opaque,
                          libheimdall_downloader_task *task)
{
    libheimdall_media_slave_t **slaves = NULL;
    unsigned count = libheimdall_media_slaves_get(task->media, &slaves);
    if (count > 0 && slaves)
    {
        cbs->on_slaves(cbs_opaque, task, slaves, count);
        libheimdall_media_slaves_release(slaves, count);
    }
}

static void parse_ended(void *opaque, libheimdall_parser_task *parser_task, libheimdall_parser_status_t status)
{
    (void) parser_task;
    struct libheimdall_downloader_task *task = opaque;
    const struct libheimdall_downloader_cbs *cbs = task->cbs;
    void *cbs_opaque = task->cbs_opaque;
    libheimdall_downloader_t *downloader = task->downloader;
    libheimdall_downloader_status_t downloader_status = libheimdall_downloader_status_error;
    struct libheimdall_downloader_thread *thread = NULL;

    if (status == libheimdall_parser_status_cancelled)
    {
        downloader_status = libheimdall_downloader_status_cancelled;
        goto cleanup;
    }

    if (status != libheimdall_parser_status_done)
        goto cleanup;

    if (cbs->on_subitems)
        notify_subitems(cbs, cbs_opaque, task);

    if (cbs->on_slaves)
        notify_slaves(cbs, cbs_opaque, task);

    libheimdall_media_type_t type = libheimdall_media_get_type(task->media);
    if (type != libheimdall_media_type_file)
        goto cleanup;

    thread = malloc(sizeof(*thread));
    if (thread == NULL)
        goto cleanup;

    heimdall_mutex_lock(&downloader->lock);
    /* interruption check before spawning downloader thread */
    if (task->interrupted)
        goto cleanup_locked;

    thread->terminated = false;
    heimdall_list_append(&thread->node, &downloader->threads);
    if (heimdall_clone(&thread->thread, downloaderThread, task))
    {
        heimdall_list_remove(&thread->node);
        goto cleanup_locked;
    }
    task->thread = thread;
    heimdall_mutex_unlock(&downloader->lock);

    return;

cleanup:
    heimdall_mutex_lock(&downloader->lock);
cleanup_locked:
    libheimdall_downloader_task_ReportStateLocked(task, downloader_status);
    heimdall_list_remove(&task->node);
    heimdall_mutex_unlock(&downloader->lock);
    free(thread);
    libheimdall_downloader_task_release(task);
}

static const struct libheimdall_parser_cbs parser_cbs = {
    .version = 0,
    .on_parsed = parse_ended,
    .on_attachments_added = NULL,
};

libheimdall_downloader_task *
libheimdall_downloader_task_new(libheimdall_downloader_t *downloader, const libheimdall_downloader_request_t *req,
                           const struct libheimdall_downloader_cbs *cbs, void *cbs_opaque)
{
    assert(downloader != NULL);
    assert(req != NULL && req->media != NULL);
    assert(cbs != NULL && cbs->on_buffer != NULL && cbs->on_state_update != NULL);

    /* No different versions to handle for now */
    assert(req->version <= 0);
    assert(cbs->version <= 0);

    struct libheimdall_downloader_task *task = DownloaderTaskNew(downloader, req->media,
                                                            cbs, cbs_opaque);

    if (task == NULL)
        return NULL;

    libheimdall_parser_request_t request = {
        .version = 0,
        .media = req->media,
        .parse_flags = 0, /* default HEIMDALL_PREPARSER_TYPE_PARSE | HEIMDALL_PREPARSER_OPTION_SUBITEMS */
    };

    task->parser_task = libheimdall_parser_task_new_parse(downloader->parser, &request,
                                                     &parser_cbs, task);
    if (task->parser_task == NULL)
    {
        DownloaderTaskDestroy(task);
        return NULL;
    }

    return task;
}

int
libheimdall_downloader_submit(libheimdall_downloader_t *downloader, libheimdall_downloader_task *task)
{
    assert(downloader != NULL);
    assert(task != NULL);
    assert(!task->submitted);

    task->submitted = true;

    heimdall_mutex_lock(&downloader->lock);
    heimdall_list_append(&task->node, &downloader->submitted_tasks);

    /* clean up terminated threads */
    struct libheimdall_downloader_thread *thread;
    heimdall_list_foreach(thread, &downloader->threads, node)
    {
        if (thread->terminated)
        {
            heimdall_join(thread->thread, NULL);
            heimdall_list_remove(&thread->node);
            free(thread);
        }
    }
    heimdall_mutex_unlock(&downloader->lock);

    heimdall_atomic_rc_inc(&task->rc);

    if (libheimdall_parser_submit(downloader->parser, task->parser_task) != 0)
    {
        heimdall_mutex_lock(&downloader->lock);
        heimdall_list_remove(&task->node);
        heimdall_mutex_unlock(&downloader->lock);
        task->submitted = false;
        /* no callback will be invoked, drop the callback reference. The caller
           keeps its own and may submit the task again. */
        libheimdall_downloader_task_release(task);
        return -1;
    }

    return 0;
}

size_t libheimdall_downloader_cancel(libheimdall_downloader_t *downloader, libheimdall_downloader_task *task)
{
    assert(downloader != NULL);
    size_t cancelled = 0;

    heimdall_mutex_lock(&downloader->lock);

    struct libheimdall_downloader_task *task_itr;
    heimdall_list_foreach(task_itr, &downloader->submitted_tasks, node)
    {
        if (task != NULL && task_itr != task)
            continue;

        cancelled++;
        DownloaderTaskInterruptLocked(task_itr);

        /* small optimisation in the likely case where the user cancel
           only one task */
        if (task != NULL)
            break;
    }

    heimdall_mutex_unlock(&downloader->lock);

    if (task != NULL)
        libheimdall_parser_cancel_request(downloader->parser, task->parser_task);
    else
        libheimdall_parser_cancel_request(downloader->parser, NULL);

    return cancelled;
}

void libheimdall_downloader_set_pause(libheimdall_downloader_t *downloader, libheimdall_downloader_task *task,
                                 bool paused)
{
    assert(downloader != NULL);
    assert(task != NULL);

    heimdall_mutex_lock(&downloader->lock);
    task->pause_requested = paused;
    if (!paused)
        heimdall_cond_signal(&task->interrupt_cond);
    heimdall_mutex_unlock(&downloader->lock);
}

void libheimdall_downloader_destroy(libheimdall_downloader_t *downloader)
{
    assert(downloader != NULL);
    libheimdall_downloader_cancel(downloader, NULL);

    /* drain the parser. libheimdall_parser_destroy() waits for all in-flight
       parse_ended callbacks to complete. Every running task was interrupted
       above, so none of the parse_ended callback can spawn a new download
       thread and the threads list is stable once this returns.
       A thread spawned just before the interruption is already in that list and
       observes the interruption at its next iteration, so joining it below returns
       promptly. */
    libheimdall_parser_destroy(downloader->parser);

    struct libheimdall_downloader_thread *thread;
    heimdall_list_foreach(thread, &downloader->threads, node)
    {
        heimdall_join(thread->thread, NULL);
        heimdall_list_remove(&thread->node);
        free(thread);
    }

    /* both lists must be empty by now */
    assert(heimdall_list_is_empty(&downloader->submitted_tasks));
    assert(heimdall_list_is_empty(&downloader->threads));

    libheimdall_release(downloader->libheimdall);
    free(downloader);
}

libheimdall_media_t *
libheimdall_downloader_task_get_media(libheimdall_downloader_task *task)
{
    assert(task != NULL);
    return task->media;
}

void libheimdall_downloader_task_release(libheimdall_downloader_task *task)
{
    assert(task != NULL);
    if (!heimdall_atomic_rc_dec(&task->rc))
        return;

    DownloaderTaskDestroy(task);
}
