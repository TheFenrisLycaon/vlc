/*****************************************************************************
 * video_output.c: test for the video output pipeline
 *****************************************************************************
 * Copyright (C) 2021 VideoLabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Author: Alexandre Janniaux <ajanni@videolabs.io>
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
#define MODULE_NAME test_vout_mock
#undef HEIMDALL_DYNAMIC_PLUGIN

#include "../../libheimdall/test.h"
#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_access.h>
#include <heimdall_demux.h>
#include <heimdall_codec.h>
#include <heimdall_window.h>
#include <heimdall_interface.h>
#include <heimdall_player.h>
#include <heimdall_filter.h>
#include <heimdall_vout_display.h>

#include <limits.h>

#include "video_output.h"
#include "../lib/libheimdall_internal.h"

static const char dec_dev_arg[] = "--dec-dev=" MODULE_STRING;

const char heimdall_module_name[] = MODULE_STRING;

static size_t current_scenario;

static void DecoderDeviceClose(struct heimdall_decoder_device *device)
    { HEIMDALL_UNUSED(device); }

static const struct heimdall_decoder_device_operations decoder_device_ops =
{
    .close = DecoderDeviceClose,
};

static int OpenDecoderDevice(
        struct heimdall_decoder_device *device,
        heimdall_window_t *window
) {
    HEIMDALL_UNUSED(window);
    device->ops = &decoder_device_ops;
    /* Pick any valid one, we'll not use the module which can make use of
     * the private parts. */
    device->type = HEIMDALL_DECODER_DEVICE_VAAPI;
    return HEIMDALL_SUCCESS;
}

static int DecoderDecode(decoder_t *dec, block_t *block)
{
    if (block == NULL)
        return HEIMDALL_SUCCESS;

    struct vout_scenario *scenario = &vout_scenarios[current_scenario];
    assert(scenario->decoder_decode != NULL);
    scenario->decoder_decode(dec, block);

    return HEIMDALL_SUCCESS;
}

static void CloseDecoder(heimdall_object_t *obj)
{
    decoder_t *dec = (decoder_t*)obj;
    struct heimdall_video_context *vctx = dec->p_sys;
    if (vctx)
        heimdall_video_context_Release(vctx);
}

static int OpenDecoder(heimdall_object_t *obj)
{
    decoder_t *dec = (decoder_t*)obj;

    struct heimdall_decoder_device *device = decoder_GetDecoderDevice(dec);
    assert(device);
    heimdall_decoder_device_Release(device);

    dec->pf_decode = DecoderDecode;
    // Necessary ?
    es_format_Clean(&dec->fmt_out);
    es_format_Copy(&dec->fmt_out, dec->fmt_in);

    struct vout_scenario *scenario = &vout_scenarios[current_scenario];
    assert(scenario->decoder_setup != NULL);
    scenario->decoder_setup(dec);

    msg_Dbg(obj, "Decoder chroma %4.4s -> %4.4s size %ux%u",
            (const char *)&dec->fmt_in->i_codec,
            (const char *)&dec->fmt_out.i_codec,
            dec->fmt_out.video.i_width, dec->fmt_out.video.i_height);

    return HEIMDALL_SUCCESS;
}

static int OpenFilter(filter_t *filter)
{
    static const struct heimdall_filter_operations ops = {
        .filter_video = NULL,
        .close = NULL,
    };
    filter->ops = &ops;

    return HEIMDALL_SUCCESS;
}

static picture_t *ConverterFilter(filter_t *filter, picture_t *input)
{
    video_format_Clean(&input->format);
    video_format_Copy(&input->format, &filter->fmt_out.video);
    return input;
}

static int OpenConverter(filter_t *filter)
{
    msg_Dbg(filter, "converter chroma %4.4s -> %4.4s size %ux%u -> %ux%u",
            (const char *)&filter->fmt_in.i_codec,
            (const char *)&filter->fmt_out.i_codec,
            filter->fmt_in.video.i_width, filter->fmt_in.video.i_height,
            filter->fmt_out.video.i_width, filter->fmt_out.video.i_height);

    struct vout_scenario *scenario = &vout_scenarios[current_scenario];
    assert(scenario->converter_setup != NULL);
    scenario->converter_setup(filter);

    static const struct heimdall_filter_operations ops = {
        .filter_video = ConverterFilter,
        .close = NULL,
    };
    filter->ops = &ops;

    return HEIMDALL_SUCCESS;
}

static int OpenWindow(heimdall_window_t *wnd)
{
    static const struct heimdall_window_operations ops = {

    };
    wnd->ops = &ops;
    return HEIMDALL_SUCCESS;
}

static void Display(vout_display_t *vd, picture_t *picture)
{
    (void) vd;
    (void) picture;
}

static int OpenDisplay(vout_display_t *vd, video_format_t *fmtp,
                       struct heimdall_video_context *vctx)
{
    static const struct heimdall_display_operations ops =
    {
        .display = Display,
    };
    vd->ops = &ops;

    struct vout_scenario *scenario = &vout_scenarios[current_scenario];
    assert(scenario->display_setup != NULL);
    int ret = scenario->display_setup(vd, fmtp, vctx);

    msg_Dbg(vd, "vout display chroma %4.4s size %ux%u -> %ux%u",
            (const char *)&fmtp->i_chroma,
            fmtp->i_width, fmtp->i_height,
            fmtp->i_width, fmtp->i_height);

    return ret;
}

static void play_scenario(intf_thread_t *intf, struct vout_scenario *scenario)
{
    vout_scenario_init();
    input_item_t *media = input_item_New(scenario->source, "dummy");
    assert(media);

    /* TODO: Codec doesn't seem to have effect in transcode:
     * - add a test that --codec works?
     * - do not use --codec at all here? */

    var_Create(intf, "codec", HEIMDALL_VAR_STRING);
    var_SetString(intf, "codec", MODULE_STRING);

    var_Create(intf, "vout", HEIMDALL_VAR_STRING);
    var_SetString(intf, "vout", MODULE_STRING);

    var_Create(intf, "window", HEIMDALL_VAR_STRING);
    var_SetString(intf, "window", MODULE_STRING);

    heimdall_player_t *player = heimdall_player_New(&intf->obj,
        HEIMDALL_PLAYER_LOCK_NORMAL);
    assert(player);

    heimdall_player_Lock(player);
    heimdall_player_SetCurrentMedia(player, media);
    heimdall_player_Start(player);
    heimdall_player_Unlock(player);

    vout_scenario_wait(scenario);

    heimdall_player_Delete(player);
    input_item_Release(media);

    var_Destroy(intf, "vout");
    var_Destroy(intf, "codec");
}

static int OpenIntf(heimdall_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t*)obj;

    msg_Info(intf, "Starting tests");
    while (current_scenario < vout_scenarios_count)
    {
        msg_Info(intf, " - Running vout scenario %zu", current_scenario);
        play_scenario(intf, &vout_scenarios[current_scenario]);
        current_scenario++;
    }

    return HEIMDALL_SUCCESS;
}

/**
 * Inject the mocked modules as a static plugin:
 *  - access for triggering the correct decoder
 *  - decoder for generating video format and context
 *  - filter for generating video format and context
 *  - encoder to check the previous video format and context
 **/
heimdall_module_begin()
    set_callbacks(OpenDecoder, CloseDecoder)
    set_capability("video decoder", INT_MAX)

    add_submodule()
        set_callback_dec_device(OpenDecoderDevice, 0)

    add_submodule()
        set_callback_video_filter(OpenFilter)

    add_submodule()
        set_callback_video_converter(OpenConverter, INT_MAX)

    add_submodule()
        set_callback(OpenWindow)
        set_capability("vout window", INT_MAX)

    add_submodule()
        set_callback_display(OpenDisplay, 0)

    /* Interface module to avoid casting libheimdall_instance_t to object */
    add_submodule()
        set_callback(OpenIntf)
        set_capability("interface", 0)

heimdall_module_end()

HEIMDALL_EXPORT const heimdall_plugin_cb heimdall_static_modules[] = {
    HEIMDALL_SYMBOL(heimdall_entry),
    NULL
};

int main( int argc, char **argv )
{
    (void)argc; (void)argv;
    test_init();

    const char * const args[] = {
        "-vvv", "--vout=dummy", "--aout=dummy", "--text-renderer=dummy",
        "--no-auto-preparse", dec_dev_arg,
        "--no-spu", "--no-osd",
    };

    libheimdall_instance_t *heimdall = libheimdall_new(ARRAY_SIZE(args), args);


    libheimdall_InternalAddIntf(heimdall->p_libheimdall_int, MODULE_STRING);
    libheimdall_InternalPlay(heimdall->p_libheimdall_int);

    libheimdall_release(heimdall);
    assert(vout_scenarios_count == current_scenario);
    return 0;
}
