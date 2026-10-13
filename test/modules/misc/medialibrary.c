/*****************************************************************************
 * medialibrary.c: test for the medialibrary code
 *****************************************************************************
 * Copyright (C) 2023 Videolabs
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
# include <config.h>
#endif

/* Define a builtin module for mocked parts */
#define MODULE_NAME test_misc_medialibrary
#undef HEIMDALL_DYNAMIC_PLUGIN

#include "../../libheimdall/test.h"

#include <heimdall/heimdall.h>

#include <heimdall_common.h>
#include <heimdall_image.h>
#include <heimdall_picture.h>
#include <heimdall_plugin.h>
#include <heimdall_block.h>
#include <heimdall_codec.h>
#include <heimdall_filter.h>

#include <heimdall_media_library.h>
#include <ftw.h>

#include "../lib/libheimdall_internal.h"

const char heimdall_module_name[] = MODULE_STRING;

static int exitcode = 0;

static void ValidateThumbnail(void *data, const heimdall_ml_event_t *event)
{
    if (event->i_type != HEIMDALL_ML_EVENT_MEDIA_THUMBNAIL_GENERATED)
        return;

    assert(event->media_thumbnail_generated.b_success);
    heimdall_sem_t *sem = data;
    heimdall_sem_post(sem);
}

static int OpenIntf(heimdall_object_t *root)
{
    heimdall_medialibrary_t *ml = heimdall_ml_instance_get(root);
    if (ml == NULL)
    {
        exitcode = 77;
        return HEIMDALL_SUCCESS;
    }

    #define MOCK_URL "mock://video_track_count=1;length=100000000;" \
                     "video_width=800;video_height=600"
    heimdall_ml_media_t *media = heimdall_ml_new_external_media(ml, MOCK_URL);

    heimdall_sem_t sem;
    heimdall_sem_init(&sem, 0);

    heimdall_ml_event_callback_t *listener =
        heimdall_ml_event_register_callback(ml, ValidateThumbnail, &sem);
    heimdall_ml_media_generate_thumbnail(ml, media->i_id,
            HEIMDALL_ML_THUMBNAIL_SMALL, 800, 600, 0.f);

    heimdall_sem_wait(&sem);
    heimdall_ml_event_unregister_callback(ml, listener);

    heimdall_ml_media_release(media);

    return HEIMDALL_SUCCESS;
}

/** Inject the mocked modules as a static plugin: **/
heimdall_module_begin()
    set_callback(OpenIntf)
    set_capability("interface", 0)
heimdall_module_end()

HEIMDALL_EXPORT const heimdall_plugin_cb heimdall_static_modules[] = {
    HEIMDALL_SYMBOL(heimdall_entry),
    NULL
};

static int cleanup_tmpdir(const char *dirpath, const struct stat *sb,
                          int typeflag, struct FTW *ftwbuf)
{
    (void)sb; (void)typeflag; (void)ftwbuf;
    return remove(dirpath);
}

int main(void)
{
    char template[] = "/tmp/heimdall.test." MODULE_STRING ".XXXXXX";
    const char *tempdir = mkdtemp(template);
    if (tempdir == NULL)
    {
        assert(tempdir != NULL);
        return -1;
    }
    fprintf(stderr, "Using HEIMDALL_USERDATA_PATH directory %s\n", tempdir);
    setenv("HEIMDALL_USERDATA_PATH", tempdir, 1);

    test_init();

    const char * const args[] = {
        "-vvv", "--vout=dummy", "--aout=dummy", "--text-renderer=dummy",
        "--no-auto-preparse", "--media-library"
    };

    libheimdall_instance_t *heimdall = libheimdall_new(ARRAY_SIZE(args), args);

    libheimdall_InternalAddIntf(heimdall->p_libheimdall_int, MODULE_STRING);
    libheimdall_InternalPlay(heimdall->p_libheimdall_int);

    libheimdall_release(heimdall);

    /* Remove temporary directory */
    nftw(tempdir, cleanup_tmpdir, FOPEN_MAX, FTW_DEPTH | FTW_MOUNT | FTW_PHYS);
    return exitcode;
}
