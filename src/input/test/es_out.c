/*****************************************************************************
 * es_out.c: test for es_out state machine
 *****************************************************************************
 * Copyright (C) 2024      VideoLabs
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

#undef NDEBUG

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Define a builtin module for mocked parts */
#define MODULE_NAME test_es_out_mock
#undef HEIMDALL_DYNAMIC_PLUGIN

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_threads.h>
#include <heimdall_input.h>
#include <heimdall_es_out.h>
#include <heimdall_input_item.h>
#include <heimdall_list.h>

#include <limits.h>

#include "../src/libheimdall.h"
#include "../lib/libheimdall_internal.h"
#include "../src/input/es_out.h"
#include "../src/input/decoder.h"
#include "../src/input/source.h"
#include "../src/clock/clock.h"

const char heimdall_module_name[] = MODULE_STRING;

struct heimdall_input_decoder_t {
    struct heimdall_list node;

    heimdall_clock_t *clock;
    bool drained;
    bool started;
};

static struct heimdall_list opened_decoders;

HEIMDALL_EXPORT heimdall_input_decoder_t *
heimdall_input_decoder_Create(heimdall_object_t *, const es_format_t *, const char *es_id,
                         struct heimdall_clock_t *, input_resource_t *) HEIMDALL_USED;

heimdall_input_decoder_t *
heimdall_input_decoder_Create(heimdall_object_t *parent, const es_format_t *fmt, const char *es_id,
                         struct heimdall_clock_t *clock, input_resource_t *p_resource)
{
    const struct heimdall_input_decoder_cfg cfg = {
        .fmt = fmt,
        .str_id = es_id,
        .clock = clock,
        .resource = p_resource,
        .sout = NULL,
        .input_type = INPUT_TYPE_PLAYBACK,
        .cbs = NULL, .cbs_data = NULL,
    };
    return heimdall_input_decoder_New(parent, &cfg);
}


heimdall_input_decoder_t *
heimdall_input_decoder_New(heimdall_object_t *parent,
                      const struct heimdall_input_decoder_cfg *cfg)
{
    msg_Dbg(parent, "Creating input decoder from test");

    struct heimdall_input_decoder_t *dec = malloc(sizeof(struct heimdall_input_decoder_t));
    assert(dec != NULL);

    dec->clock = cfg->clock;
    dec->drained = false;
    dec->started = false;
    heimdall_list_append(&dec->node, &opened_decoders);
    return dec;
}

void heimdall_input_decoder_Delete(heimdall_input_decoder_t *owner)
{
    heimdall_list_remove(&owner->node);
    free(owner);
}

void heimdall_input_decoder_ChangeDelay(
    heimdall_input_decoder_t *owner,
    heimdall_tick_t delay)
{
    (void)owner; (void)delay;
}

void heimdall_input_decoder_ChangePause(
    heimdall_input_decoder_t *owner,
    bool paused,
    heimdall_tick_t date)
{
    (void)owner; (void)paused; (void)date;
}

void heimdall_input_decoder_ChangeRate(
    heimdall_input_decoder_t *owner,
    float rate)
{
    (void)owner; (void)rate;
}

void heimdall_input_decoder_StartWait(heimdall_input_decoder_t *owner)
{
    (void)owner;
}

void heimdall_input_decoder_Wait(heimdall_input_decoder_t *owner)
{
    (void)owner;
}

void heimdall_input_decoder_StopWait(heimdall_input_decoder_t *owner)
{
    (void)owner;
    owner->started = true;
}

bool heimdall_input_decoder_IsDrained(heimdall_input_decoder_t *owner)
{
    (void)owner;
    return owner->drained;
}

bool heimdall_input_decoder_IsEmpty(heimdall_input_decoder_t *owner)
{
    return heimdall_input_decoder_IsDrained(owner);
}

void heimdall_input_decoder_DecodeWithStatus(
    heimdall_input_decoder_t *owner,
    heimdall_frame_t *frame,
    bool do_pace,
    struct heimdall_input_decoder_status *status)
{
    (void)owner; (void)do_pace;

    if (status != NULL)
        *status = (struct heimdall_input_decoder_status){ .format.changed = false };
    heimdall_frame_Release(frame);
}

void heimdall_input_decoder_Drain(heimdall_input_decoder_t *owner)
{
    owner->drained = true;
}

void heimdall_input_decoder_Flush(heimdall_input_decoder_t *owner)
{
    (void)owner;
}

size_t heimdall_input_decoder_GetFifoSize(heimdall_input_decoder_t *owner)
{
    (void)owner;
    return 0;
}

/* VBI */

int heimdall_input_decoder_GetVbiPage(
    heimdall_input_decoder_t *owner,
    bool *opaque)
{
    (void)owner; (void)opaque;
    return 0;
}

int heimdall_input_decoder_SetVbiPage(
    heimdall_input_decoder_t *owner,
    unsigned page)
{
    (void)owner; (void)page;
    return 0;
}

int heimdall_input_decoder_SetVbiOpaque(
    heimdall_input_decoder_t *owner, 
    bool opaque)
{
    (void)owner; (void)opaque;
    return 0;
}

/* Vout */

void heimdall_input_decoder_SetVoutMouseEvent(
    heimdall_input_decoder_t *owner,
    heimdall_mouse_event event,
    void *opaque)
{
    (void)owner; (void)event; (void)opaque;
}

int heimdall_input_decoder_AddVoutOverlay(
    heimdall_input_decoder_t *owner,
    subpicture_t *subpic,
    size_t *_)
{
    (void)owner; (void)subpic; (void)_;
    return 0;
}

int heimdall_input_decoder_DelVoutOverlay(
    heimdall_input_decoder_t *owner,
    size_t _)
{
    (void)owner; (void)_;
    return 0;
}

void heimdall_input_decoder_FrameNext(heimdall_input_decoder_t *p_dec)
{
    (void)p_dec;
}

void heimdall_input_decoder_FramePrevious(heimdall_input_decoder_t *p_dec)
{
    (void)p_dec;
}

void heimdall_input_decoder_StopFrameNext(heimdall_input_decoder_t *p_dec)
{
    (void)p_dec;
}

heimdall_input_decoder_t *
heimdall_input_decoder_CreateSubDec(heimdall_input_decoder_t *dec,
                               const struct heimdall_input_decoder_cfg *cfg)
{
    (void)dec; (void)cfg;
    return NULL;
}

void var_OptionParse(heimdall_object_t *obj, const char *chain, bool trusted)
{
    (void)obj; (void)chain; (void)trusted;
}

static bool can_pace_control = false;

bool input_CanPaceControl(input_thread_t *input)
{
    (void)input;
    return can_pace_control;
}

int input_GetAttachments(input_thread_t *input,
                         input_attachment_t ***attachments)
{
    (void)input; (void)attachments;
    return HEIMDALL_SUCCESS;
}

void input_ExtractAttachmentAndCacheArt(input_thread_t *p_input,
                                        const char *name)
{
    (void)p_input; (void)name;
}

void input_resource_StopFreeVout(input_resource_t *p_resource);
void input_resource_StopFreeVout(input_resource_t *p_resource)
{
    (void)p_resource;
}

void input_rate_Add(input_rate_t *counter, uintmax_t val)
{
    (void)counter; (void)val;
}

void heimdall_subdec_desc_Clean(struct heimdall_subdec_desc *desc)
{
    (void)desc;
}

bool input_Stopped(input_thread_t *input)
{
    (void)input;
    return false;
}

int input_ControlPush(input_thread_t *input,
                      int type, const input_control_param_t *param)
{
    (void)input; (void)type; (void)param;
    return 0;
}

input_item_t * input_GetItem(input_thread_t *input)
{
    return input_priv(input)->p_item;
}

sout_stream_t *sout_NewInstance(heimdall_object_t *p_parent, const char *psz_dest);
sout_stream_t *sout_NewInstance(heimdall_object_t *p_parent, const char *psz_dest)
{
    (void)p_parent; (void)psz_dest;
    return NULL;
}

static input_source_t *InputSourceNew(void)
{
    input_source_t *in = calloc(1, sizeof(*in));
    if (unlikely(in == NULL))
        return NULL;

    heimdall_atomic_rc_init( &in->rc );
    in->i_normal_time = HEIMDALL_TICK_0;

    return in;
}

static void LogText(void *opaque, int type, const heimdall_log_t *meta,
                    const char *format, va_list ap)
{
    (void)opaque;

    static const char msg_type[4][9] = { "", " error", " warning", " debug" };

    flockfile(stderr);
    fprintf(stderr, "%s%s: ", meta->psz_module, msg_type[type]);
    vfprintf(stderr, format, ap);
    putc_unlocked('\n', stderr);
    funlockfile(stderr);
}

static const struct heimdall_logger_operations test_logger_operations = {
    .log = LogText,
};

struct test_ctx
{
    struct heimdall_logger logger;
    heimdall_object_t *root;
    input_item_t *item;
    input_thread_t *input;
    input_source_t *source;
    struct heimdall_input_es_out *out;
};

static void test_ctx_Init(struct test_ctx *ctx, float rate,
                          heimdall_tick_t pts_delay)
{
    heimdall_list_init(&opened_decoders);
    ctx->logger = (struct heimdall_logger) { .ops = &test_logger_operations };
    libheimdall_priv_t *libheimdall = (heimdall_object_create)(NULL, sizeof(*libheimdall));
    ctx->root = &libheimdall->public_data.obj;
    ctx->root->logger = &ctx->logger;

    var_Create(ctx->root, "clock-master", HEIMDALL_VAR_STRING);
    var_SetString(ctx->root, "clock-master", "auto");

    var_Create(ctx->root, "captions", HEIMDALL_VAR_INTEGER);
    var_SetInteger(ctx->root, "captions", 608);

    var_Create(ctx->root, "mouse-pause-policy", HEIMDALL_VAR_STRING);
    var_SetString(ctx->root, "mouse-pause-policy", "auto");

    ctx->item = input_item_NewStream("mock://", "mock", 0);

    input_thread_private_t *priv = heimdall_object_create(ctx->root, sizeof(*priv));
    assert(priv != NULL);
    priv->p_item = ctx->item;

    ctx->input = &priv->input;
    var_Create(ctx->input, "video", HEIMDALL_VAR_BOOL);
    var_SetBool(ctx->input, "video", true);
    var_Create(ctx->input, "audio", HEIMDALL_VAR_BOOL);
    var_SetBool(ctx->input, "audio", true);

    ctx->source = InputSourceNew();

    ctx->out = input_EsOutNew(ctx->input, ctx->source, rate,
                              INPUT_TYPE_PLAYBACK);
    assert(ctx->out != NULL);

    es_out_SetMode(ctx->out, ES_OUT_MODE_AUTO);
    es_out_SetJitter(ctx->out, pts_delay, 0,
                     40 * pts_delay / DEFAULT_PTS_DELAY);
}

static void test_ctx_Destroy(struct test_ctx *ctx)
{
    es_out_SetMode(ctx->out, ES_OUT_MODE_END);
    es_out_Delete(&ctx->out->out);

    input_source_Release(ctx->source);
    input_item_Release(ctx->item);
    heimdall_object_delete(&ctx->input->obj);
    heimdall_object_delete(ctx->root);
}

static es_out_id_t *AddVideoTrack(struct heimdall_input_es_out *out)
{
    es_format_t fmt;
    es_format_Init(&fmt, VIDEO_ES, HEIMDALL_CODEC_RGBA);
    fmt.i_id = 1;
    fmt.i_group = 1;
    video_format_Init(&fmt.video, HEIMDALL_CODEC_RGBA);
    fmt.video.i_width = fmt.video.i_visible_width = 64;
    fmt.video.i_height = fmt.video.i_visible_height = 64;
    es_out_id_t *track = es_out_Add(&out->out, &fmt);
    assert(track != NULL);
    es_format_Clean(&fmt);
    return track;
}

static es_out_id_t *AddAudioTrack(struct heimdall_input_es_out *out)
{
    es_format_t fmt;
    es_format_Init(&fmt, AUDIO_ES, HEIMDALL_CODEC_F32L);
    fmt.i_id = 2;
    fmt.audio.i_format = HEIMDALL_CODEC_F32L;
    fmt.audio.i_rate = 44100;
    fmt.audio.i_physical_channels = 2;
    fmt.audio.i_channels = 2;
    es_out_id_t *track = es_out_Add(&out->out, &fmt);
    assert(track != NULL);
    es_format_Clean(&fmt);
    return track;
}

static void SendBlock(struct heimdall_input_es_out *out, es_out_id_t *track)
{
    block_t *block = block_Alloc(10);
    assert(block);
    es_out_Send(&out->out, track, block);
}

static void test_playback(void)
{
    struct test_ctx ctx;
    test_ctx_Init(&ctx, 1.f, HEIMDALL_TICK_FROM_MS(300));
    struct heimdall_input_es_out *out = ctx.out;

    es_out_id_t *video_track = AddVideoTrack(out);
    es_out_id_t *audio_track = AddAudioTrack(out);

    SendBlock(out, video_track);
    SendBlock(out, audio_track);

    es_out_SetPCR(&out->out, HEIMDALL_TICK_0);
    es_out_SetPCR(&out->out, HEIMDALL_TICK_0 + HEIMDALL_TICK_FROM_MS(100));
    es_out_SetPCR(&out->out, HEIMDALL_TICK_0 + HEIMDALL_TICK_FROM_MS(200));
    es_out_SetPCR(&out->out, HEIMDALL_TICK_0 + HEIMDALL_TICK_FROM_MS(300) - 1);

    {
        size_t count = 0;
        struct heimdall_input_decoder_t *dec;
        heimdall_list_foreach(dec, &opened_decoders, node)
        {
            assert(dec->started == false);
            count++;
        }
        assert(count == 2);
    }

    es_out_SetPCR(&out->out, HEIMDALL_TICK_0 + HEIMDALL_TICK_FROM_MS(300));

    {
        size_t count = 0;
        struct heimdall_input_decoder_t *dec;
        heimdall_list_foreach(dec, &opened_decoders, node)
        {
            assert(dec->started == true);
            count++;
        }
        assert(count == 2);
    }

    es_out_Del(&out->out, audio_track);
    es_out_Del(&out->out, video_track);

    test_ctx_Destroy(&ctx);
}

static void test_multiple_programs(void)
{
    struct test_ctx ctx;
    test_ctx_Init(&ctx, 1.f, HEIMDALL_TICK_FROM_MS(300));
    struct heimdall_input_es_out *out = ctx.out;

    es_out_id_t *video_track = AddVideoTrack(out);
    es_out_id_t *audio_track = AddAudioTrack(out);

    SendBlock(out, video_track);
    SendBlock(out, audio_track);

    es_out_Control(&out->out, ES_OUT_SET_GROUP_PCR, 1, HEIMDALL_TICK_0);
    es_out_Control(&out->out, ES_OUT_SET_GROUP_PCR, 1, HEIMDALL_TICK_0 + HEIMDALL_TICK_FROM_MS(100));
    es_out_Control(&out->out, ES_OUT_SET_GROUP_PCR, 2, HEIMDALL_TICK_0 + HEIMDALL_TICK_FROM_MS(200));
    es_out_Control(&out->out, ES_OUT_SET_GROUP_PCR, 2, HEIMDALL_TICK_0 + HEIMDALL_TICK_FROM_MS(300) - 1);

    es_out_Del(&out->out, audio_track);
    es_out_Del(&out->out, video_track);

    test_ctx_Destroy(&ctx);
}

static void test_preroll_rate(float rate)
{
    /* es_out only computes a wakeup date when the input controls its pace */
    can_pace_control = true;

    /* Default file-caching */
    const heimdall_tick_t pts_delay = HEIMDALL_TICK_FROM_SEC(1);

    struct test_ctx ctx;
    test_ctx_Init(&ctx, rate, pts_delay);
    struct heimdall_input_es_out *out = ctx.out;

    es_out_id_t *video_track = AddVideoTrack(out);
    SendBlock(out, video_track);

    /* Precise seek: the demuxer restarts 4s before the target */
    const heimdall_tick_t target = HEIMDALL_TICK_0 + HEIMDALL_TICK_FROM_SEC(4);
    es_out_Control(&out->out, ES_OUT_SET_NEXT_DISPLAY_TIME, target);

    const heimdall_tick_t before = heimdall_tick_now();
    for (heimdall_tick_t pcr = HEIMDALL_TICK_0; pcr <= target + pts_delay;
         pcr += HEIMDALL_TICK_FROM_MS(100))
        es_out_SetPCR(&out->out, pcr);

    struct heimdall_input_decoder_t *dec =
        heimdall_list_first_entry_or_null(&opened_decoders,
                                     struct heimdall_input_decoder_t, node);
    assert(dec != NULL);
    assert(dec->started);

    /* The first frame past the preroll is due right away */
    heimdall_clock_Lock(dec->clock);
    const heimdall_tick_t now = heimdall_tick_now();
    const heimdall_tick_t date =
        heimdall_clock_ConvertToSystem(dec->clock, now, target, rate, NULL);
    heimdall_clock_Unlock(dec->clock);
    assert(date >= now);
    assert(date < now + pts_delay);

    /* The demuxer is due to read now, not more than pts_delay ago, which
     * would make the next PCR late */
    const heimdall_tick_t wakeup = es_out_GetWakeup(out);
    assert(wakeup <= heimdall_tick_now());
    assert(wakeup >= before - pts_delay);

    es_out_Del(&out->out, video_track);

    test_ctx_Destroy(&ctx);

    can_pace_control = false;
}

int main(void)
{
    test_playback();
    test_multiple_programs();
    test_preroll_rate(0.25f);
    test_preroll_rate(2.f);
    return 0;
}
