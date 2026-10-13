/*****************************************************************************
 * image.c: test for the image_handler code from heimdall_image.h
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
#define MODULE_NAME test_misc_image
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

#include <limits.h>

#include "../lib/libheimdall_internal.h"

const char heimdall_module_name[] = MODULE_STRING;

static atomic_bool encoder_opened = false;

static int OpenIntf(heimdall_object_t *root)
{
    image_handler_t *ih = image_HandlerCreate(root);
    assert(ih != NULL);

    video_format_t fmt_in;
    video_format_Init(&fmt_in, HEIMDALL_CODEC_RGBA);
    fmt_in.i_width = fmt_in.i_visible_width = 800;
    fmt_in.i_height = fmt_in.i_visible_height = 600;

    video_format_t fmt_out;
    video_format_Init(&fmt_out, 0);
    fmt_out.i_width = fmt_out.i_visible_width = 800;
    fmt_out.i_height = fmt_out.i_visible_height = 600;

    picture_t *picture = picture_NewFromFormat(&fmt_in);
    assert(picture != NULL);

    block_t *block;

    block = image_Write(ih, picture, &fmt_in, HEIMDALL_CODEC_PNG, &fmt_out);
    assert(block != NULL);
    block_Release(block);
    picture_Release(picture);
    assert(atomic_load(&encoder_opened));
    atomic_store(&encoder_opened, false);

    picture = picture_NewFromFormat(&fmt_in);
    fmt_out.i_width = fmt_out.i_visible_width = 400;
    fmt_out.i_height = fmt_out.i_visible_height = 300;
    block = image_Write(ih, picture, &fmt_in, HEIMDALL_CODEC_PNG, &fmt_out);
    assert(block != NULL);
    block_Release(block);
    picture_Release(picture);
    assert(atomic_load(&encoder_opened));

    image_HandlerDelete(ih);

    return HEIMDALL_SUCCESS;
}

static block_t * EncodeVideo(encoder_t *encoder, picture_t *pic)
{
    (void)encoder; (void)pic;

    /* Dummy encoder */
    return block_Alloc(1);
}

static int OpenEncoder(heimdall_object_t *obj)
{
    encoder_t *encoder = (encoder_t*)obj;
    static const struct heimdall_encoder_operations ops =
    {
        .encode_video = EncodeVideo
    };
    encoder->ops = &ops;
    atomic_store(&encoder_opened, true);
    return HEIMDALL_SUCCESS;
}

static picture_t *ConvertVideo(filter_t *filter, picture_t *pic)
{
    picture_Release(pic);
    return picture_NewFromFormat(&filter->fmt_out.video);
}

static int OpenConverter(filter_t *filter)
{
    static const struct heimdall_filter_operations ops =
    {
        .filter_video = ConvertVideo,
    };
    filter->ops = &ops;
    return HEIMDALL_SUCCESS;
}

/** Inject the mocked modules as a static plugin: **/
heimdall_module_begin()
    set_callback(OpenEncoder)
    set_capability("image encoder", INT_MAX)

    add_submodule()
        set_callback_video_converter(OpenConverter, INT_MAX)

    add_submodule()
        set_callback(OpenIntf)
        set_capability("interface", 0)

heimdall_module_end()

HEIMDALL_EXPORT const heimdall_plugin_cb heimdall_static_modules[] = {
    HEIMDALL_SYMBOL(heimdall_entry),
    NULL
};


int main(void)
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
