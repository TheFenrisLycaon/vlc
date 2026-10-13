/*****************************************************************************
 * imem.c: test for the imem libheimdall code
 *****************************************************************************
 * Copyright (C) 2023 VideoLabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Alexandre Janniaux <ajanni@videolabs.io>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
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

/* Define a builtin module for mocked parts */
#define MODULE_NAME test_imem
#undef HEIMDALL_DYNAMIC_PLUGIN

#include "./test.h"
#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_stream.h>
#include <heimdall_threads.h>
#include <heimdall_interrupt.h>

#include <heimdall/libheimdall_media.h>

#include <limits.h>

#include "media_player.h"

const char heimdall_module_name[] = MODULE_STRING;

#define ACCESS_COUNT 2
struct imem_root_access
{
    heimdall_sem_t opened;
    heimdall_sem_t read_blocking;
};

struct imem_root
{
    /* root controlled semaphores */
    heimdall_sem_t available;
    heimdall_sem_t done;
    size_t access_idx;
    struct imem_root_access accesses[ACCESS_COUNT];

    heimdall_sem_t wait;
};

struct imem_access
{
    struct imem_root *root;
    heimdall_sem_t wait;
};

static void AccessClose(void *opaque)
{
    struct imem_access *sys = opaque;
    fprintf(stderr, "test: Access: Close\n");
    heimdall_sem_post(&sys->root->done);
    heimdall_sem_post(&sys->root->available);
    sys->root->access_idx++;
    free(sys);
}

static int AccessOpen(void *opaque, void **datap, uint64_t *sizep)
{
    (void)sizep;
    struct imem_root *root = opaque;
    assert(root->access_idx < ACCESS_COUNT);
    heimdall_sem_wait(&root->available);
    fprintf(stderr, "test: Access: Opening new instance\n");

    struct imem_access *sys = *datap = malloc(sizeof *sys);
    if (sys == NULL)
        return -ENOMEM;

    sys->root = root;
    heimdall_sem_init(&sys->wait, 0);
    heimdall_sem_post(&root->accesses[root->access_idx].opened);
    return HEIMDALL_SUCCESS;
}

static ssize_t AccessRead(void *opaque, unsigned char *buf, size_t len)
{
    (void)opaque;
    assert(len < SSIZE_MAX);
    memset(buf, 0xff, len);
    return len;
}

static void SetFlag(void *opaque)
{
    bool *flag = opaque;
    *flag = true;
}

static ssize_t AccessReadBlocking(void *opaque, unsigned char *buf, size_t len)
{
    (void)opaque; (void)buf;
    struct imem_access *sys = opaque;
    struct imem_root *root = sys->root;
    assert(len < SSIZE_MAX);

    heimdall_sem_post(&root->accesses[root->access_idx].read_blocking);
    /* The interruption is used to detect when the input has been closed. */
    bool was_interrupted = false;
    heimdall_interrupt_register(SetFlag, &was_interrupted);
    fprintf(stderr, "test: Access: read() -> blocking\n");
    heimdall_sem_wait(&sys->root->wait);
    fprintf(stderr, "test: Access: read() -> unblocked\n");
    heimdall_interrupt_unregister();
    //assert(was_interrupted);

    /* We notify that we don't have data to read right now. */
    return 0;
}

static void UnblockRead(void *opaque, libheimdall_media_t *media, libheimdall_stopping_reason_t reason)
{
    (void)media;
    (void)reason;
    fprintf(stderr, "test: Player: Unblock read\n");
    struct imem_root *imem = opaque;
    heimdall_sem_post(&imem->wait);
}

static struct imem_root *imem_root_New(void)
{

    struct imem_root *imem = malloc(sizeof *imem);
    assert(imem != NULL);

    heimdall_sem_init(&imem->available, 1);
    heimdall_sem_init(&imem->done, 0);
    heimdall_sem_init(&imem->wait, 0);
    imem->access_idx = 0;
    for (size_t i = 0; i < ACCESS_COUNT; i++)
    {
        heimdall_sem_init(&imem->accesses[i].opened, 0);
        heimdall_sem_init(&imem->accesses[i].read_blocking, 0);
    }

    return imem;
}

static void test_media_callback(libheimdall_instance_t *heimdall)
{
    struct imem_root *imem = imem_root_New();

    fprintf(stderr, "test: 1/ checking that media can terminate\n");
    static const struct libheimdall_media_open_cbs cbs = {
        .version = 0,
        .open = AccessOpen,
        .read = AccessRead,
        .close = AccessClose,
    };
    static const struct libheimdall_media_player_cbs player_cbs = {
        .version = 0,
        .on_state_changed = mp_on_state_changed,
    };
    libheimdall_media_t *media;

    media = libheimdall_media_new_callbacks(&cbs, imem);
    assert(media != NULL);

    struct mp_event_ctx ctx;
    mp_event_ctx_init(&ctx);
    libheimdall_media_player_t *player =
        libheimdall_media_player_new(heimdall, &player_cbs, &ctx);
    assert(player != NULL);
    mp_event_ctx_set_mp(&ctx, player);

    libheimdall_media_player_set_media(player, media);
    libheimdall_media_player_play(player);

    mp_event_ctx_wait_state(&ctx, libheimdall_Opening);
    mp_event_ctx_wait_state(&ctx, libheimdall_Playing);

    libheimdall_media_player_stop_async(player);

    mp_event_ctx_wait_state(&ctx, libheimdall_Stopping);
    mp_event_ctx_wait_state(&ctx, libheimdall_Stopped);

    heimdall_sem_wait(&imem->done);
    libheimdall_media_release(media);

    mp_event_ctx_destroy(&ctx);
    libheimdall_media_player_release(player);

    free(imem);
}

static void test_media_callback_interrupt(libheimdall_instance_t *heimdall)
{
    struct imem_root *imem = imem_root_New();

    static const struct libheimdall_media_player_cbs player_cbs = {
        .version = 0,
        .on_media_stopping = UnblockRead,
    };

    fprintf(stderr, "test: 2/ checking that we can terminate after the input\n");
    libheimdall_media_player_t *player =
        libheimdall_media_player_new(heimdall, &player_cbs, imem);
    assert(player != NULL);

    static const struct libheimdall_media_open_cbs cbs = {
        .version = 0,
        .open = AccessOpen,
        .read = AccessReadBlocking,
        .close = AccessClose,
    };
    libheimdall_media_t *media;

    media = libheimdall_media_new_callbacks(&cbs, imem);
    assert(media != NULL);

    fprintf(stderr, "test: set initial media\n");
    libheimdall_media_player_set_media(player, media);
    libheimdall_media_player_play(player);

    /* We want to be sure that the media has been opened. */
    heimdall_sem_wait(&imem->accesses[0].opened);
    heimdall_sem_wait(&imem->accesses[0].read_blocking);

    libheimdall_media_release(media);

    media = libheimdall_media_new_callbacks(&cbs, imem);
    assert(media != NULL);
    fprintf(stderr, "test: changing to new media\n");
    libheimdall_media_player_set_media(player, media);
    fprintf(stderr, "test: waiting for the new media\n");

    /* Semaphore notifying that the first access has been closed. */
    heimdall_sem_wait(&imem->done);
    assert(heimdall_sem_trywait(&imem->accesses[0].read_blocking) == EAGAIN);

    heimdall_sem_wait(&imem->accesses[1].opened);
    heimdall_sem_wait(&imem->accesses[1].read_blocking);

    fprintf(stderr, "test: checking that we get the media stopping event\n");
    libheimdall_media_player_stop_async(player);

    heimdall_sem_wait(&imem->done);

    libheimdall_media_release(media);
    libheimdall_media_player_release(player);

    free(imem);
}

int main( int argc, char **argv )
{
    (void)argc; (void)argv;
    test_init();

    const char * const heimdall_argv[] = {
        "-vvv", "--vout=dummy", "--aout=dummy", "--text-renderer=dummy",
        "--demux="MODULE_STRING,
    };

    libheimdall_instance_t *heimdall = libheimdall_new(ARRAY_SIZE(heimdall_argv), heimdall_argv);
    assert(heimdall != NULL);

    test_media_callback(heimdall);
    test_media_callback_interrupt(heimdall);

    libheimdall_release(heimdall);
    return 0;
}

static int DemuxDemux(demux_t *demux)
{
    (void)demux;
    return HEIMDALL_SUCCESS;
}

static void DemuxClose(heimdall_object_t *obj)
{
    (void)obj;
}

static int DemuxOpen(heimdall_object_t *obj)
{
    demux_t *demux = (demux_t *)obj;

    static const struct heimdall_stream_operations ops =
    {
        .demux.demux = DemuxDemux,
    };
    demux->ops = &ops;

    return HEIMDALL_SUCCESS;
}

heimdall_module_begin()
    set_capability("demux", 0)
    set_callbacks(DemuxOpen, DemuxClose)
heimdall_module_end()

HEIMDALL_EXPORT const heimdall_plugin_cb heimdall_static_modules[] = {
    HEIMDALL_SYMBOL(heimdall_entry),
    NULL
};
