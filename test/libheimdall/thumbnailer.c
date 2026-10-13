/*****************************************************************************
 * thumbnailer.c - libheimdall thumbnailing smoke test
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

#include "test.h"
#include "../lib/libheimdall_internal.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_modules.h>

#include <heimdall/heimdall.h>

#ifndef TEST_THUMB_TYPE
# error Define TEST_THUMB_TYPE to a libheimdall_picture_type_t value
#endif

static void thumbnail_generated(void *opaque, libheimdall_parser_task *task, libheimdall_picture_t *picture)
{
    assert(picture != NULL);
    heimdall_sem_t *sem = opaque;
    heimdall_sem_post(sem);
    libheimdall_parser_task_release(task);
}

static void test_media_thumbnail(libheimdall_instance_t *heimdall, const char *location,
                                 unsigned width, unsigned height,
                                 enum libheimdall_picture_type_t picture_type)
{
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
    test_log ("Testing media_thumbnail: width=%u height=%u type=" STRINGIFY(TEST_THUMB_TYPE) "'\n", width, height);
    libheimdall_media_t *media = libheimdall_media_new_location(location);
    assert (media != NULL);

    heimdall_sem_t sem;
    heimdall_sem_init (&sem, 0);

    static const struct libheimdall_thumbnailer_cbs cbs = {
        .version = 0,
        .on_ended = thumbnail_generated,
    };

    const struct libheimdall_parser_cfg cfg = {
        .version = 0,
        .max_thumbnailer_threads = 1,
        .timeout = 0,
    };

    libheimdall_parser_t *parser = libheimdall_parser_new(heimdall, &cfg);
    assert(parser != NULL);

    libheimdall_thumbnailer_request_t request = {
        .version = 0,
        .media = media,
        .seek = {
            .type = libheimdall_thumbnailer_seek_pos,
            .value = { .pos = 0.f },
            .speed = libheimdall_media_thumbnail_seek_precise,
        },
        .width = width,
        .height = height,
        .crop = false,
        .type = picture_type,
        .hw_dec = false,
    };
    libheimdall_parser_task *task = libheimdall_parser_task_new_thumbnail(parser, &request, &cbs, &sem);
    assert(task != NULL);
    assert(libheimdall_parser_submit(parser, task) == 0);

    heimdall_sem_wait(&sem);
    libheimdall_parser_destroy(parser);
    libheimdall_media_release(media);
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    test_init();

#ifndef ENABLE_SOUT
    (void)thumbnail_generated;
    (void)test_media_thumbnail;
    test_log("Stream output has been disabled (see ENABLE_SOUT), skipping...")
    return 77;
#else

    libheimdall_instance_t *heimdall = libheimdall_new (test_defaults_nargs,
                                         test_defaults_args);
    assert (heimdall != NULL);

    const char *module_deps[] = {
        "demux_mock", "swscale",
        TEST_REQUIRED_MODULES
    };

    for (size_t i=0; i<ARRAY_SIZE(module_deps); ++i)
    {
        test_log(" - Checking that module %s exists...\n", module_deps[i]);
        /* Skip test if one of the dependencies doesn't exist */
        if (!module_exists(module_deps[i]))
        {
            test_log("   Module %s is required and wasn't found, skipping...\n", module_deps[i]);
            libheimdall_release(heimdall);
            exit(77);
        }
    }

    /* Check that thumbnails are correctly generated. */
    const char *mock_addr =
        "mock://video_track_count=1;length=100000000;"
        "video_width=800;video_height=600";

    test_media_thumbnail(heimdall, mock_addr, 800, 600, TEST_THUMB_TYPE);
    test_media_thumbnail(heimdall, mock_addr, 400, 300, TEST_THUMB_TYPE);

    libheimdall_release (heimdall);

    return 0;
#endif
}
