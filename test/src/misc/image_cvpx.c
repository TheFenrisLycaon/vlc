/*****************************************************************************
 * image_cvpx.c: export test for image_handler with cvpx sources
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
#define MODULE_NAME test_misc_image_cvpx
#undef HEIMDALL_DYNAMIC_PLUGIN

#include "../../libheimdall/test.h"
#include "../../../modules/codec/vt_utils.h"

#include <heimdall/heimdall.h>

#include <heimdall_common.h>
#include <heimdall_image.h>
#include <heimdall_picture.h>
#include <heimdall_plugin.h>
#include <heimdall_block.h>

#include <limits.h>

#include "../lib/libheimdall_internal.h"

const char heimdall_module_name[] = MODULE_STRING;

static int OpenIntf(heimdall_object_t *root)
{
    image_handler_t *ih = image_HandlerCreate(root);
    assert(ih != NULL);

    video_format_t fmt_in;
    video_format_Init(&fmt_in, HEIMDALL_CODEC_CVPX_NV12);
    fmt_in.i_width = fmt_in.i_visible_width = 800;
    fmt_in.i_height = fmt_in.i_visible_height = 600;

    CVPixelBufferPoolRef pool = cvpxpool_create(&fmt_in, 1);
    assert(pool != NULL);

    CVPixelBufferRef buffer = cvpxpool_new_cvpx(pool);
    assert(buffer != NULL);

    picture_t *picture = picture_NewFromFormat(&fmt_in);
    assert(picture != NULL);

    struct heimdall_decoder_device *device = NULL;
    static const struct heimdall_video_context_operations ops = {
        NULL
    };

    struct heimdall_video_context *vctx =heimdall_video_context_CreateCVPX(
      device, CVPX_VIDEO_CONTEXT_DEFAULT, 0, &ops);
    int ret = cvpxpic_attach(picture, buffer, vctx,
                             NULL /* TODO: check everything is released */);
    assert(ret == HEIMDALL_SUCCESS);

    video_format_t fmt_out;
    video_format_Init(&fmt_out, 0);
    fmt_out.i_width = fmt_out.i_visible_width = 800;
    fmt_out.i_height = fmt_out.i_visible_height = 600;

    block_t *block;

    block = image_Write(ih, picture, &fmt_in, HEIMDALL_CODEC_PNG, &fmt_out);
    assert(block != NULL);
    block_Release(block);
    image_HandlerDelete(ih);

    picture_Release(picture);

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


int main()
{
    test_init();

    const char * const args[] = {
        "-vvv", "--vout=dummy", "--aout=dummy", "--text-renderer=dummy",
        "--no-auto-preparse",
    };

    libheimdall_instance_t *heimdall = libheimdall_new(ARRAY_SIZE(args), args);

    libheimdall_InternalAddIntf(heimdall->p_libheimdall_int, MODULE_STRING);
    libheimdall_InternalPlay(heimdall->p_libheimdall_int);

    libheimdall_release(heimdall);

}

