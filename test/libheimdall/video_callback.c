// SPDX-License-Identifier: LGPL-2.1-or-later
/*****************************************************************************
 * video_callback.c: test for the video callback libheimdall code
 *****************************************************************************
 * Copyright (C) 2026 Alexandre Janniaux <ajanni@videolabs.io>
 * Copyright (C) 2026 TheFenrisLycaon
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
#include <heimdall_common.h>
#include <heimdall_variables.h>
#include "../lib/libheimdall_internal.h"
#include "../lib/media_player_internal.h"

static void *dummy_lock(void *opaque, void **planes)
{
    (void)opaque;
    (void)planes;
    return NULL;
}

static void dummy_unlock(void *opaque, void *picture, void *const *planes)
{
    (void)opaque;
    (void)picture;
    (void)planes;
}

static void dummy_display(void *opaque, void *picture)
{
    (void)opaque;
    (void)picture;
}

static bool dummy_output_setup(void **opaque,
                               const libheimdall_video_setup_device_cfg_t *cfg,
                               libheimdall_video_setup_device_info_t *out)
{
    (void)cfg;
    (void)out;
    *opaque = (void *)0x1;
    return true;
}

static void dummy_output_cleanup(void *opaque)
{
    (void)opaque;
}

static void dummy_output_set_window(void *opaque,
                                    libheimdall_video_output_resize_cb resize_cb,
                                    libheimdall_video_output_mouse_move_cb move_cb,
                                    libheimdall_video_output_mouse_press_cb press_cb,
                                    libheimdall_video_output_mouse_release_cb release_cb,
                                    void *report_opaque)
{
    (void)opaque;
    (void)resize_cb;
    (void)move_cb;
    (void)press_cb;
    (void)release_cb;
    (void)report_opaque;
}

static bool dummy_update_output(void *opaque,
                                const libheimdall_video_render_cfg_t *cfg,
                                libheimdall_video_output_cfg_t *output)
{
    (void)opaque;
    (void)cfg;
    (void)output;
    return true;
}

static void dummy_swap(void *opaque)
{
    (void)opaque;
}

static bool dummy_make_current(void *opaque, bool enter)
{
    (void)opaque;
    (void)enter;
    return true;
}

static void *dummy_get_proc_address(void *opaque, const char *fct_name)
{
    (void)opaque;
    (void)fct_name;
    return NULL;
}

static void dummy_metadata(void *opaque, libheimdall_video_metadata_type_t type,
                           const void *metadata)
{
    (void)opaque;
    (void)type;
    (void)metadata;
}

static bool dummy_select_plane(void *opaque, size_t plane, void *output)
{
    (void)opaque;
    (void)plane;
    (void)output;
    return true;
}

static void assert_var_string_eq(heimdall_object_t *obj, const char *name,
                                 const char *expected)
{
    char *value = var_GetString(obj, name);
    if (expected == NULL)
        assert(value == NULL);
    else
    {
        assert(value != NULL);
        assert(strcmp(value, expected) == 0);
    }
    free(value);
}

static void test_media_player_detach_video_callbacks(const char** argv, int argc)
{
    test_log("Testing detach of video callbacks\n");
    (void)argv;
    (void)argc;

    const char *test_args[] = {
        "-v", "--vout=nonexistent-vout", "--aout=adummy", "--text-renderer=tdummy",
    };
    const int test_nargs = (int)(sizeof(test_args) / sizeof(test_args[0]));

    libheimdall_instance_t *instance = libheimdall_new(test_nargs, test_args);
    assert(instance != NULL);

    libheimdall_media_player_t *player = libheimdall_media_player_new(instance, NULL, NULL);
    assert(player != NULL);

    char *expected_vout = var_GetString(instance->p_libheimdall_int, "vout");
    assert(expected_vout != NULL);
    assert(strcmp(expected_vout, "nonexistent-vout") == 0);
    assert_var_string_eq(HEIMDALL_OBJECT(player), "vout", expected_vout);

    libheimdall_video_set_callbacks(player, dummy_lock, dummy_unlock, dummy_display, NULL);

    assert(var_GetAddress(player, "vmem-lock") != NULL);
    assert_var_string_eq(HEIMDALL_OBJECT(player), "vout", "vmem");
    assert_var_string_eq(HEIMDALL_OBJECT(player), "window", "dummy");

#ifdef __APPLE__
    libheimdall_media_player_set_nsobject(player, (void *)1);
#elif defined(_WIN32)
    libheimdall_media_player_set_hwnd(player, (void *)1);
#else
    libheimdall_media_player_set_xwindow(player, 1);
#endif

    assert(var_GetAddress(player, "vmem-lock") == NULL);
    assert_var_string_eq(HEIMDALL_OBJECT(player), "vout", expected_vout);

    free(expected_vout);
    libheimdall_media_player_release(player);
    libheimdall_release(instance);
}

static void test_media_player_detach_output_callbacks(const char** argv, int argc)
{
    test_log("Testing detach of output callbacks\n");
    (void)argv;
    (void)argc;

    const char *test_args[] = {
        "-v", "--vout=nonexistent-vout", "--aout=adummy", "--text-renderer=tdummy",
    };
    const int test_nargs = (int)(sizeof(test_args) / sizeof(test_args[0]));

    libheimdall_instance_t *instance = libheimdall_new(test_nargs, test_args);
    assert(instance != NULL);

    libheimdall_media_player_t *player = libheimdall_media_player_new(instance, NULL, NULL);
    assert(player != NULL);

    char *expected_vout = var_GetString(instance->p_libheimdall_int, "vout");
    assert(expected_vout != NULL);
    assert(strcmp(expected_vout, "nonexistent-vout") == 0);
    assert_var_string_eq(HEIMDALL_OBJECT(player), "vout", expected_vout);

    bool ok = libheimdall_video_set_output_callbacks(
        player, libheimdall_video_engine_opengl, dummy_output_setup,
        dummy_output_cleanup, dummy_output_set_window, dummy_update_output,
        dummy_swap, dummy_make_current, dummy_get_proc_address,
        dummy_metadata, dummy_select_plane, NULL);
    assert(ok);

    assert(var_GetInteger(player, "vout-cb-type") == libheimdall_video_engine_opengl);
    assert(var_GetAddress(player, "vout-cb-window-cb") != NULL);

    ok = libheimdall_video_set_output_callbacks(
        player, libheimdall_video_engine_disable, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL);
    assert(ok);

    assert(var_GetInteger(player, "vout-cb-type") == libheimdall_video_engine_disable);
    assert(var_GetAddress(player, "vout-cb-window-cb") == NULL);
    assert(var_GetAddress(player, "vout-cb-setup") == NULL);
    assert_var_string_eq(HEIMDALL_OBJECT(player), "vout", expected_vout);

    free(expected_vout);
    libheimdall_media_player_release(player);
    libheimdall_release(instance);
}

int main (void)
{
    test_init();

    test_media_player_detach_video_callbacks(test_defaults_args, test_defaults_nargs);
    test_media_player_detach_output_callbacks(test_defaults_args, test_defaults_nargs);

    return 0;
}
