/*****************************************************************************
 * input_decoder.c: test for heimdall_input_decoder state machine
 *****************************************************************************
 * Copyright (C) 2022-2023 VideoLabs
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
#define MODULE_NAME test_input_decoder_mock
#undef HEIMDALL_DYNAMIC_PLUGIN

#include "../../../libheimdall/test.h"
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
#include <heimdall_vout_display.h>
#include <heimdall_sout.h>

#include <limits.h>

#include "input_decoder.h"
#include "../lib/libheimdall_internal.h"

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

static int DecoderDecode(decoder_t *dec, block_t *block)
{
    if (block == NULL)
        return HEIMDALL_SUCCESS;

    picture_t *pic = picture_NewFromFormat(&dec->fmt_out.video);
    assert(pic);
    pic->date = block->i_pts;
    pic->b_progressive = true;

    struct input_decoder_scenario *scenario = &input_decoder_scenarios[current_scenario];
    assert(scenario->decoder_decode != NULL);
    int ret = scenario->decoder_decode(dec, pic);
    if (ret != HEIMDALLDEC_RELOAD)
        block_Release(block);
    return ret;
}

static int CcDecoderDecode(decoder_t *dec, heimdall_frame_t *frame)
{
    if (frame == NULL)
        return HEIMDALL_SUCCESS;

    struct input_decoder_scenario *scenario = &input_decoder_scenarios[current_scenario];
    assert(scenario->cc_decoder_decode != NULL);
    int ret = scenario->cc_decoder_decode(dec, frame);
    if (ret != HEIMDALLDEC_RELOAD)
        heimdall_frame_Release(frame);
    return ret;
}

static void DecoderFlush(decoder_t *dec)
{
    struct input_decoder_scenario *scenario = &input_decoder_scenarios[current_scenario];
    if (scenario->decoder_flush != NULL)
        scenario->decoder_flush(dec);
}

static void CloseDecoder(heimdall_object_t *obj)
{
    struct input_decoder_scenario *scenario = &input_decoder_scenarios[current_scenario];
    decoder_t *dec = (decoder_t*)obj;

    if (scenario->decoder_destroy != NULL)
        scenario->decoder_destroy(dec);
    struct heimdall_video_context *vctx = dec->p_sys;
    if (vctx)
        heimdall_video_context_Release(vctx);
}

static void CloseCcDecoder(heimdall_object_t *obj)
{
    struct input_decoder_scenario *scenario = &input_decoder_scenarios[current_scenario];
    decoder_t *dec = (decoder_t*)obj;

    if (scenario->cc_decoder_destroy != NULL)
        scenario->cc_decoder_destroy(dec);
}

static heimdall_frame_t *PacketizerPacketize(decoder_t *dec, heimdall_frame_t **in)
{
    (void)dec;
    if (in == NULL)
        return NULL;

    heimdall_frame_t *ret = *in;
    if (ret != NULL)
        *in = NULL;
    return ret;
}

static heimdall_frame_t *PacketizerGetCC(decoder_t *dec, decoder_cc_desc_t *cc_desc)
{
    struct input_decoder_scenario *scenario = &input_decoder_scenarios[current_scenario];
    if (scenario->packetizer_getcc != NULL)
        return scenario->packetizer_getcc(dec, cc_desc);
    return NULL;
}

static int OpenPacketizer(heimdall_object_t *obj)
{
    decoder_t *dec = (decoder_t*)obj;

    dec->pf_packetize = PacketizerPacketize;
    dec->pf_get_cc = PacketizerGetCC;
    dec->pf_flush = NULL;
    es_format_Clean(&dec->fmt_out);
    es_format_Copy(&dec->fmt_out, dec->fmt_in);

    return HEIMDALL_SUCCESS;
}

static int OpenDecoder(heimdall_object_t *obj)
{
    decoder_t *dec = (decoder_t*)obj;

    struct heimdall_decoder_device *device = decoder_GetDecoderDevice(dec);
    assert(device);
    heimdall_decoder_device_Release(device);

    dec->pf_decode = DecoderDecode;
    dec->pf_get_cc = NULL;
    dec->pf_flush = DecoderFlush;
    es_format_Clean(&dec->fmt_out);
    es_format_Copy(&dec->fmt_out, dec->fmt_in);

    struct input_decoder_scenario *scenario = &input_decoder_scenarios[current_scenario];
    assert(scenario->decoder_setup != NULL);
    scenario->decoder_setup(dec);

    msg_Dbg(obj, "Decoder chroma %4.4s -> %4.4s size %ux%u",
            (const char *)&dec->fmt_in->i_codec,
            (const char *)&dec->fmt_out.i_codec,
            dec->fmt_out.video.i_width, dec->fmt_out.video.i_height);

    return HEIMDALL_SUCCESS;
}

static int OpenCcDecoder(heimdall_object_t *obj)
{
    struct input_decoder_scenario *scenario = &input_decoder_scenarios[current_scenario];
    if (scenario->cc_decoder_setup == NULL)
        return HEIMDALL_EGENERIC;

    decoder_t *dec = (decoder_t*)obj;

    dec->pf_decode = CcDecoderDecode;
    dec->pf_get_cc = NULL;
    dec->pf_flush = NULL;
    es_format_Clean(&dec->fmt_out);
    es_format_Copy(&dec->fmt_out, dec->fmt_in);

    scenario->cc_decoder_setup(dec);

    msg_Dbg(obj, "Decoder chroma %4.4s -> %4.4s",
            (const char *)&dec->fmt_in->i_codec,
            (const char *)&dec->fmt_out.i_codec);

    return HEIMDALL_SUCCESS;
}

static void DisplayPrepare(vout_display_t *vd, picture_t *picture,
        const struct heimdall_render_subpicture *subpic, heimdall_tick_t date)
{
    (void)vd; (void)subpic; (void)date;

    struct input_decoder_scenario *scenario = &input_decoder_scenarios[current_scenario];
    assert(scenario->display_prepare != NULL);
    scenario->display_prepare(vd, picture);
}

static int OpenDisplay(vout_display_t *vd, video_format_t *fmtp, heimdall_video_context *context)
{
    (void)fmtp; (void)context;

    struct input_decoder_scenario *scenario = &input_decoder_scenarios[current_scenario];
    static const struct heimdall_display_operations ops = {
        .prepare = DisplayPrepare,
    };
    vd->ops = &ops;

    vd->info.subpicture_chromas = scenario->subpicture_chromas;

    return HEIMDALL_SUCCESS;
}

static int OpenWindow(heimdall_window_t *wnd)
{
    static const struct heimdall_window_operations ops =
    {
        .resize = NULL,
    };
    wnd->ops = &ops;
    return HEIMDALL_SUCCESS;
}

static subpicture_region_t *TextRendererRender(filter_t *filter,
                              const subpicture_region_t *region_in,
                              const heimdall_fourcc_t *chroma_list)
{
    (void) chroma_list;
    struct input_decoder_scenario *scenario = &input_decoder_scenarios[current_scenario];
    if (scenario->text_renderer_render != NULL)
        scenario->text_renderer_render(filter, region_in);
    return NULL;
}

static int OpenTextRenderer(filter_t *filter)
{
    static const struct heimdall_filter_operations ops =
    {
        .render = TextRendererRender,
    };

    filter->ops = &ops;
    return HEIMDALL_SUCCESS;
}

static void *SoutFilterAdd(sout_stream_t *stream, const es_format_t *fmt,
                           const char *es_id)
{
    (void)stream; (void)es_id;
    heimdall_fourcc_t *id = malloc(sizeof(*id));
    assert(id != NULL);
    *id = fmt->i_codec;
    return id;
}

static void SoutFilterDel(sout_stream_t *stream, void *id)
{
    (void)stream;
    free(id);
}

static int SoutFilterSend(sout_stream_t *stream, void *id, block_t *block)
{
    struct input_decoder_scenario *scenario = &input_decoder_scenarios[current_scenario];
    if (scenario->sout_filter_send != NULL)
        return scenario->sout_filter_send(stream, id, block);
    block_Release(block);
    return HEIMDALL_SUCCESS;
}

static void SoutFilterFlush(sout_stream_t *stream, void *id)
{
    struct input_decoder_scenario *scenario = &input_decoder_scenarios[current_scenario];
    if (scenario->sout_filter_flush != NULL)
        scenario->sout_filter_flush(stream, id);
}

static int SoutFilterControl(sout_stream_t *stream, int query, va_list args)
{
    (void)stream;
    if (query != SOUT_STREAM_WANTS_SUBSTREAMS)
        return HEIMDALL_EGENERIC;
    *va_arg(args, bool *) = true;
    return HEIMDALL_SUCCESS;
}

static int OpenSoutFilter(heimdall_object_t *obj)
{
    sout_stream_t *stream = (sout_stream_t *)obj;
    static const struct sout_stream_operations ops = {
        .add = SoutFilterAdd,
        .del = SoutFilterDel,
        .send = SoutFilterSend,
        .flush = SoutFilterFlush,
        .control = SoutFilterControl,
    };
    stream->ops = &ops;
    return HEIMDALL_SUCCESS;
};

static void on_state_changed(heimdall_player_t *player, enum heimdall_player_state state, void *opaque)
{
    (void)player; (void)state; (void) opaque;
    heimdall_cond_signal(&player_cond);
}

static void on_track_list_changed(heimdall_player_t *player,
        enum heimdall_player_list_action action,
        const struct heimdall_player_track *track,
        void *data)
{
    (void)player; (void)data;
    struct input_decoder_scenario *scenario = &input_decoder_scenarios[current_scenario];
    if (scenario->on_track_list_changed != NULL)
        scenario->on_track_list_changed(action, track);
}

static void play_scenario(intf_thread_t *intf, struct input_decoder_scenario *scenario)
{
    assert(scenario->name != NULL);
    fprintf(stderr, "\nChecking '%s'\n\n", scenario->name);
    input_decoder_scenario_init();
    input_item_t *media = input_item_New(scenario->source, "dummy");
    assert(media);

    var_Create(intf, "codec", HEIMDALL_VAR_STRING);
    var_SetString(intf, "codec", MODULE_STRING);

    if (scenario->item_option != NULL)
        input_item_AddOption(media, scenario->item_option, HEIMDALL_INPUT_OPTION_TRUSTED);

    heimdall_player_t *player = heimdall_player_New(&intf->obj,
        HEIMDALL_PLAYER_LOCK_NORMAL);
    assert(player);

    intf->p_sys = (intf_sys_t *)player;

    static const struct heimdall_player_cbs player_cbs = {
        .on_state_changed = on_state_changed,
        .on_track_list_changed = on_track_list_changed,
    };

    heimdall_player_Lock(player);
    heimdall_player_listener_id *listener =
        heimdall_player_AddListener(player, &player_cbs, NULL);
    heimdall_player_SetCurrentMedia(player, media);
    if (scenario->player_setup_before_start != NULL)
        scenario->player_setup_before_start(player);
    heimdall_player_Start(player);
    heimdall_player_Unlock(player);

    input_decoder_scenario_wait(intf, scenario);

    heimdall_player_Lock(player);
    heimdall_player_Stop(player);

    while (heimdall_player_GetState(player) != HEIMDALL_PLAYER_STATE_STOPPED)
        heimdall_player_CondWait(player, &player_cond);

    heimdall_player_RemoveListener(player, listener);
    heimdall_player_Unlock(player);

    input_decoder_scenario_check(scenario);

    heimdall_player_Delete(player);
    input_item_Release(media);

    var_Destroy(intf, "codec");
}

static int OpenIntf(heimdall_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t*)obj;

    while (current_scenario < input_decoder_scenarios_count)
    {
        msg_Info(intf, " - Running scenario %zu", current_scenario);
        play_scenario(intf, &input_decoder_scenarios[current_scenario]);
        current_scenario++;
    }

    return HEIMDALL_SUCCESS;
}

/**
 * Inject the mocked modules as a static plugin:
 *  - access for triggering the correct decoder
 *  - decoder for generating video format and context
 *  - filter for generating video format and context
 **/
heimdall_module_begin()
    set_callback(OpenIntf)
    set_capability("interface", 0)


    add_submodule()
        set_callback_dec_device(OpenDecoderDevice, 0)

    add_submodule()
        set_callbacks(OpenDecoder, CloseDecoder)
        set_capability("video decoder", INT_MAX)

    add_submodule()
        set_callbacks(OpenCcDecoder, CloseCcDecoder)
        set_capability("spu decoder", INT_MAX)

    add_submodule()
        set_callback(OpenPacketizer)
        set_capability("video packetizer", INT_MAX)

    add_submodule()
        set_callback(OpenPacketizer)
        set_capability("audio packetizer", INT_MAX)

    add_submodule()
        set_callback(OpenPacketizer)
        set_capability("spu packetizer", INT_MAX)

    add_submodule()
        set_callback(OpenWindow)
        set_capability("vout window", INT_MAX)

    add_submodule()
        set_callback_display(OpenDisplay, 0)

    add_submodule()
        set_callback_text_renderer(OpenTextRenderer, INT_MAX)

    add_submodule()
        set_callback(OpenSoutFilter)
        set_capability("sout output", 0)

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
        "-vvv",
        "--vout=" MODULE_STRING,
        "--dec-dev=" MODULE_STRING,
        "--aout=dummy",
        "--no-auto-preparse",
        "--no-osd",
    };

    libheimdall_instance_t *heimdall = libheimdall_new(ARRAY_SIZE(args), args);

    libheimdall_InternalAddIntf(heimdall->p_libheimdall_int, MODULE_STRING);
    libheimdall_InternalPlay(heimdall->p_libheimdall_int);

    libheimdall_release(heimdall);
    assert(input_decoder_scenarios_count == current_scenario);
    return 0;
}
