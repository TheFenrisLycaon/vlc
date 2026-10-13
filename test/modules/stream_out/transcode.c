/*****************************************************************************
 * transcode.c: test for transcoding pipeline
 *****************************************************************************
 * Copyright (C) 2021-2026 VideoLabs
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
#define MODULE_NAME test_transcode_mock
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
#include <heimdall_threads.h>
#include <heimdall_sout.h>
#include <heimdall_frame.h>

#include <limits.h>

#include "transcode.h"
#include "../lib/libheimdall_internal.h"

static const char dec_dev_arg[] = "--dec-dev=" MODULE_STRING;

const char heimdall_module_name[] = MODULE_STRING;

static size_t current_scenario = 0;

static heimdall_cond_t player_cond = HEIMDALL_STATIC_COND;

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

static int DecoderDecode(decoder_t *dec, heimdall_frame_t *frame)
{
    if (frame == NULL)
    {
        struct transcode_scenario *drained =
            &transcode_scenarios[current_scenario];

        if (drained->decoder_drain != NULL)
            return drained->decoder_drain(dec);
        return HEIMDALL_SUCCESS;
    }

    const picture_resource_t resource = {
        .p_sys = NULL,
    };
    picture_t *pic = picture_NewFromResource(&dec->fmt_out.video, &resource);
    assert(pic);
    pic->date = frame->i_pts;
    heimdall_frame_Release(frame);

    struct transcode_scenario *scenario = &transcode_scenarios[current_scenario];
    assert(scenario->decoder_decode != NULL);
    return scenario->decoder_decode(dec, pic);
}

static void CloseDecoder(heimdall_object_t *obj)
{
    decoder_t *dec = (decoder_t*)obj;

    /* An owner that cleans fmt_in before unloading would set the category
     * to UNKNOWN_ES, but it must only be cleaned after this callback. */
    assert(dec->fmt_in->i_cat == VIDEO_ES);
    assert(dec->fmt_in->i_codec != 0);

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

    struct transcode_scenario *scenario = &transcode_scenarios[current_scenario];
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

    struct transcode_scenario *scenario = &transcode_scenarios[current_scenario];
    assert(scenario->converter_setup != NULL);
    scenario->converter_setup(filter);

    static const struct heimdall_filter_operations ops = {
        .filter_video = ConverterFilter,
        .close = NULL,
    };
    filter->ops = &ops;

    return HEIMDALL_SUCCESS;
}

static heimdall_frame_t *EncodeVideo(encoder_t *enc, picture_t *pic)
{
    struct transcode_scenario *scenario = &transcode_scenarios[current_scenario];

    if (pic == NULL)
    {
        if (scenario->encoder_drain != NULL)
            return scenario->encoder_drain(enc);
        return NULL;
    }

    assert(pic->format.i_chroma == enc->fmt_in.video.i_chroma);
    heimdall_frame_t *frame = heimdall_frame_Alloc(sizeof(struct test_output));
    assert(frame != NULL);

    if (scenario->encoder_encode != NULL)
        scenario->encoder_encode(enc, pic, frame);
    return frame;
}

static void CloseEncoder(encoder_t *enc)
{
    struct transcode_scenario *scenario = &transcode_scenarios[current_scenario];
    if (scenario->encoder_close != NULL)
        scenario->encoder_close(enc);
}

static int OpenEncoder(heimdall_object_t *obj)
{
    encoder_t *enc = (encoder_t *)obj;
    enc->p_sys = NULL;

    struct transcode_scenario *scenario = &transcode_scenarios[current_scenario];
    assert(scenario->encoder_setup != NULL);
    scenario->encoder_setup(enc);

    msg_Dbg(obj, "Encoder chroma %4.4s -> %4.4s size %ux%u -> %ux%u",
            (const char *)&enc->fmt_in.i_codec,
            (const char *)&enc->fmt_out.i_codec,
            enc->fmt_in.video.i_width, enc->fmt_in.video.i_height,
            enc->fmt_out.video.i_width, enc->fmt_out.video.i_height);

    static const struct heimdall_encoder_operations ops =
    {
        .encode_video = EncodeVideo,
        .close = CloseEncoder,
    };
    enc->ops = &ops;

    return HEIMDALL_SUCCESS;
}

static int ErrorCheckerSend(sout_stream_t *stream, void *id, heimdall_frame_t *f)
{
    struct transcode_scenario *scenario = &transcode_scenarios[current_scenario];

    int ret = sout_StreamIdSend(stream->p_next, id, f);
    if (ret != HEIMDALL_SUCCESS)
        scenario->report_error(stream);
    return HEIMDALL_SUCCESS;
}

static void *ErrorCheckerAdd(sout_stream_t *stream,
                             const es_format_t *fmt,
                             const char *es_id)
    { return sout_StreamIdAdd(stream->p_next, fmt, es_id); }

static void ErrorCheckerDel(sout_stream_t *stream, void *id)
    { sout_StreamIdDel(stream->p_next, id); };

static int OpenErrorChecker(heimdall_object_t *obj)
{
    sout_stream_t *stream = (sout_stream_t *)obj;
    static const struct sout_stream_operations ops = {
        .add = ErrorCheckerAdd,
        .del = ErrorCheckerDel,
        .send = ErrorCheckerSend,
    };
    stream->ops = &ops;
    return HEIMDALL_SUCCESS;
}

static int OutputCheckerSend(sout_stream_t *stream, void *id, heimdall_frame_t *f)
{
    (void)stream; (void)id;
    struct transcode_scenario *scenario = &transcode_scenarios[current_scenario];

    assert(scenario->report_output != NULL);
    scenario->report_output(f);

    heimdall_frame_ChainRelease(f);

    return HEIMDALL_SUCCESS;
}

static void *OutputCheckerAdd(sout_stream_t *stream, const es_format_t *fmt,
                              const char *es_id)
{
    (void)stream; (void)fmt; (void)es_id;
    return (void*)0x42;
}

static void OutputCheckerDel(sout_stream_t *stream, void *id)
    { (void)stream; (void)id; }

static int OpenOutputChecker(heimdall_object_t *obj)
{
    sout_stream_t *stream = (sout_stream_t *)obj;

    static const struct sout_stream_operations ops = {
        .add = OutputCheckerAdd,
        .del = OutputCheckerDel,
        .send = OutputCheckerSend,
    };
    stream->ops = &ops;
    return HEIMDALL_SUCCESS;
}

static void on_state_changed(heimdall_player_t *player, enum heimdall_player_state state, void *opaque)
{
    (void)player; (void)state; (void) opaque;
    heimdall_cond_signal(&player_cond);
}

static void play_scenario(intf_thread_t *intf, struct transcode_scenario *scenario)
{
    transcode_scenario_init();
    input_item_t *media = input_item_New(scenario->source, "dummy");
    assert(media);

    /* TODO: Codec doesn't seem to have effect in transcode:
     * - add a test that --codec works?
     * - do not use --codec at all here? */
    input_item_AddOption(media, scenario->sout, HEIMDALL_INPUT_OPTION_TRUSTED);

    var_Create(intf, "codec", HEIMDALL_VAR_STRING);
    var_SetString(intf, "codec", MODULE_STRING);

    var_Create(intf, "sout-transcode-venc", HEIMDALL_VAR_STRING);
    var_SetString(intf, "sout-transcode-venc", MODULE_STRING);

    var_Create(intf, "sout-transcode-vcodec", HEIMDALL_VAR_STRING);
    var_SetString(intf, "sout-transcode-vcodec", "test");

    heimdall_player_t *player = heimdall_player_New(&intf->obj,
        HEIMDALL_PLAYER_LOCK_NORMAL);
    assert(player);

    static const struct heimdall_player_cbs player_cbs = {
        .on_state_changed = on_state_changed,
    };

    heimdall_player_Lock(player);
    heimdall_player_listener_id *listener =
        heimdall_player_AddListener(player, &player_cbs, NULL);
    heimdall_player_SetCurrentMedia(player, media);
    heimdall_player_Start(player);
    heimdall_player_Unlock(player);

    transcode_scenario_wait(scenario);

    heimdall_player_Lock(player);
    heimdall_player_Stop(player);

    while (heimdall_player_GetState(player) != HEIMDALL_PLAYER_STATE_STOPPED)
        heimdall_player_CondWait(player, &player_cond);

    heimdall_player_RemoveListener(player, listener);
    heimdall_player_Unlock(player);

    transcode_scenario_check(scenario);

    heimdall_player_Delete(player);
    input_item_Release(media);

    var_Destroy(intf, "sout-transcode-vcodec");
    var_Destroy(intf, "sout-transcode-venc");
}

static int OpenIntf(heimdall_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t*)obj;

    while (current_scenario < transcode_scenarios_count)
    {
        msg_Info(intf, " - Running transcode scenario %zu", current_scenario);
        play_scenario(intf, &transcode_scenarios[current_scenario]);
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
        set_callback(OpenErrorChecker)
        set_capability("sout filter", 0)
        add_shortcut("error_checker")

    add_submodule()
        set_callback(OpenOutputChecker)
        set_capability("sout output", 0)
        add_shortcut("output_checker")

    add_submodule()
        set_callback_dec_device(OpenDecoderDevice, 0)

    add_submodule()
        set_callback_video_filter(OpenFilter)

    add_submodule()
        set_callback_video_converter(OpenConverter, INT_MAX)

    add_submodule()
        set_callback(OpenEncoder)
        set_capability("video encoder", 0)

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
#ifndef ENABLE_SOUT
    (void) libheimdall_InternalPlay;
    return 77;
#endif
    test_init();

    const char * const args[] = {
        "-vvv", "--vout=dummy", "--aout=dummy", "--text-renderer=dummy",
        "--no-auto-preparse", dec_dev_arg
    };

    libheimdall_instance_t *heimdall = libheimdall_new(ARRAY_SIZE(args), args);

    libheimdall_InternalAddIntf(heimdall->p_libheimdall_int, MODULE_STRING);
    libheimdall_InternalPlay(heimdall->p_libheimdall_int);

    libheimdall_release(heimdall);
    assert(transcode_scenarios_count == current_scenario);
    return 0;
}
