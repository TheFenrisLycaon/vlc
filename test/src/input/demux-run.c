/**
 * @file demux-run.c
 */
/*****************************************************************************
 * Copyright © 2016 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Rémi Denis-Courmont reserves the right to redistribute this file under
 * the terms of the GNU Lesser General Public License as published by the
 * the Free Software Foundation; either version 2.1 or the License, or
 * (at his option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <heimdall_common.h>
#include <heimdall_access.h>
#include <heimdall_block.h>
#include <heimdall_demux.h>
#include <heimdall_input.h>
#include <heimdall_meta.h>
#include <heimdall_es_out.h>
#include <heimdall_url.h>
#include "../lib/libheimdall_internal.h"

#include <heimdall/heimdall.h>

#include "demux-run.h"
#include "decoder.h"

void heimdall_object_InitInputConfig(heimdall_object_t *obj,
                                bool playback, bool do_inherit);

struct test_es_out_t
{
    struct es_out_t out;
    struct es_out_id_t *ids;
#ifdef HAVE_DECODERS
    heimdall_object_t *parent;
#endif
};

struct es_out_id_t
{
    struct es_out_id_t *next;
#ifdef HAVE_DECODERS
    decoder_t *decoder;
    es_format_t fmt;
#endif
};

static es_out_id_t *EsOutAdd(es_out_t *out, input_source_t* in, const es_format_t *fmt)
{
    (void)in;
    struct test_es_out_t *ctx = (struct test_es_out_t *) out;

    if (fmt->i_group < 0)
        return NULL;

    es_out_id_t *id = malloc(sizeof (*id));
    if (unlikely(id == NULL))
        return NULL;

    id->next = ctx->ids;
    ctx->ids = id;
#ifdef HAVE_DECODERS
    es_format_Copy(&id->fmt, fmt);
    id->decoder = test_decoder_create(ctx->parent, &id->fmt);
    if (id->decoder == NULL)
        es_format_Clean(&id->fmt);
#endif

    debug("[%p] Added   ES\n", (void *)id);
    return id;
}

static void EsOutCheckId(struct test_es_out_t *ctx, es_out_id_t *id)
{
    for (es_out_id_t *ids = ctx->ids; ids != NULL; ids = ids->next)
        if (ids == id)
            return;

    abort();
}

static int EsOutSend(es_out_t *out, es_out_id_t *id, block_t *block)
{
    struct test_es_out_t *ctx = (struct test_es_out_t *) out;

    //debug("[%p] Sent    ES: %zu\n", (void *)idd, block->i_buffer);
    EsOutCheckId(ctx, id);
#ifdef HAVE_DECODERS
    if (id->decoder)
        test_decoder_process(id->decoder, block);
    else
#endif
        block_Release(block);
    return HEIMDALL_SUCCESS;
}

static void IdDelete(es_out_id_t *id)
{
#ifdef HAVE_DECODERS
    if (id->decoder)
    {
        /* Drain */
        test_decoder_process(id->decoder, NULL);
        test_decoder_destroy(id->decoder);
        es_format_Clean(&id->fmt);
    }
#endif
    free(id);
}

static void EsOutDelete(es_out_t *out, es_out_id_t *id)
{
    struct test_es_out_t *ctx = (struct test_es_out_t *) out;
    es_out_id_t **pp = &ctx->ids;

    while (*pp != id)
    {
        if (*pp == NULL)
            abort();
        pp = &((*pp)->next);
    }

    debug("[%p] Deleted ES\n", (void *)id);
    *pp = id->next;
    IdDelete(id);
}

static int EsOutControl(es_out_t *out, input_source_t* in, int query, va_list args)
{
    (void)in;
    struct test_es_out_t *ctx = (struct test_es_out_t *) out;

    switch (query)
    {
        case ES_OUT_SET_ES:
            break;
        case ES_OUT_RESTART_ES:
        {
#ifdef HAVE_DECODERS
            es_out_id_t* id = va_arg(args, es_out_id_t*);
            EsOutCheckId(ctx, id);
            test_decoder_destroy(id->decoder);
            test_decoder_create(ctx->parent, &id->fmt);
#endif
            break;
        }
        case ES_OUT_SET_ES_DEFAULT:
        case ES_OUT_SET_ES_STATE:
            break;
        case ES_OUT_GET_ES_STATE:
            EsOutCheckId(ctx, va_arg(args, es_out_id_t *));
            *va_arg(args, bool *) = true;
            break;
        case ES_OUT_SET_ES_CAT_POLICY:
            break;
        case ES_OUT_SET_GROUP:
        case ES_OUT_SET_PCR:
        case ES_OUT_SET_GROUP_PCR:
        case ES_OUT_RESET_PCR:
        case ES_OUT_SET_ES_FMT:
        case ES_OUT_SET_NEXT_DISPLAY_TIME:
        case ES_OUT_SET_GROUP_META:
        case ES_OUT_SET_GROUP_EPG:
        case ES_OUT_DEL_GROUP:
        case ES_OUT_SET_ES_SCRAMBLED_STATE:
        case ES_OUT_DRAIN:
            break;
        case ES_OUT_IS_EMPTY:
            *va_arg(args, bool *) = true;
            break;
        case ES_OUT_SET_META:
            break;
        default:
            return HEIMDALL_EGENERIC;
    }
    return HEIMDALL_SUCCESS;
}

static void EsOutDestroy(es_out_t *out)
{
    struct test_es_out_t *ctx = (struct test_es_out_t *)out;
    es_out_id_t *id;

    while ((id = ctx->ids) != NULL)
    {
        ctx->ids = id->next;
        IdDelete(id);
    }
    free(ctx);
}

static const struct es_out_callbacks es_out_cbs =
{
    .add = EsOutAdd,
    .send = EsOutSend,
    .del = EsOutDelete,
    .control = EsOutControl,
    .destroy = EsOutDestroy,
};

static es_out_t *test_es_out_create(heimdall_object_t *parent)
{
    heimdall_object_InitInputConfig(parent, true, false);

    struct test_es_out_t *ctx = malloc(sizeof (*ctx));
    if (ctx == NULL)
    {
        fprintf(stderr, "Error: cannot create ES output.\n");
        return NULL;
    }

    ctx->ids = NULL;

    es_out_t *out = &ctx->out;
    out->cbs = &es_out_cbs;
#ifdef HAVE_DECODERS
    ctx->parent = parent;
#else
    (void) parent;
#endif
    return out;
}

static unsigned demux_test_and_clear_flags(demux_t *demux, unsigned flags)
{
    unsigned update = flags;
    if (demux_Control(demux, DEMUX_TEST_AND_CLEAR_FLAGS, &update))
        return 0;
    return update;
}

static void demux_get_title_list(demux_t *demux)
{
    int title;
    int title_offset;
    int seekpoint_offset;
    input_title_t **title_list;

    if (demux_Control(demux, DEMUX_GET_TITLE_INFO, &title_list, &title,
                      &title_offset, &seekpoint_offset) == HEIMDALL_SUCCESS)
    {
        for (int i = 0; i < title; i++)
            heimdall_input_title_Delete(title_list[i]);
    }
}

static void demux_get_meta(demux_t *demux)
{
    heimdall_meta_t *p_meta = heimdall_meta_New();
    if (unlikely(p_meta == NULL) )
        return;

    input_attachment_t **attachment;
    int i_attachment;

    demux_Control(demux, DEMUX_GET_META, p_meta);
    demux_Control(demux, DEMUX_GET_ATTACHMENTS, &attachment, &i_attachment);

    heimdall_meta_Delete(p_meta);
}

static int demux_process_stream(const struct heimdall_run_args *args, stream_t *s)
{
    const char *name = args->name;
    if (name == NULL)
        name = "any";

    if (s == NULL)
        return -1;

    es_out_t *out = test_es_out_create(heimdall_object_parent(s));
    if (out == NULL)
        return -1;

    demux_t *demux = demux_New(HEIMDALL_OBJECT(s), name, "heimdall://nop", s, out);
    if (demux == NULL)
    {
        es_out_Delete(out);
        heimdall_stream_Delete(s);
        fprintf(stderr, "Error: cannot create demultiplexer: %s\n", name);
        return -1;
    }

    uintmax_t i = 0;
    int val;

    while ((val = demux_Demux(demux)) == HEIMDALL_DEMUXER_SUCCESS)
    {
        if (args->test_demux_controls)
        {
            if (demux_test_and_clear_flags(demux, INPUT_UPDATE_TITLE_LIST))
                demux_get_title_list(demux);

            if (demux_test_and_clear_flags(demux, INPUT_UPDATE_META))
                demux_get_meta(demux);

            int seekpoint = 0;
            double position = 0.0;
            heimdall_tick_t time = 0;
            heimdall_tick_t length = 0;

            /* Call controls for increased code coverage */
            demux_Control(demux, DEMUX_GET_SEEKPOINT, &seekpoint);
            demux_Control(demux, DEMUX_GET_POSITION, &position);
            demux_Control(demux, DEMUX_GET_TIME, &time);
            demux_Control(demux, DEMUX_GET_LENGTH, &length);
        }
        i++;
    }

    demux_Delete(demux);
    es_out_Delete(out);

    debug("Completed with %" PRIuMAX " iteration(s).\n", i);

    return val == HEIMDALL_DEMUXER_EOF ? 0 : -1;
}

int heimdall_demux_process_url(const struct heimdall_run_args *args, const char *url)
{
    libheimdall_instance_t *heimdall = libheimdall_create(args);
    if (heimdall == NULL)
        return -1;

    stream_t *s = heimdall_access_NewMRL(HEIMDALL_OBJECT(heimdall->p_libheimdall_int), url);
    if (s == NULL)
        fprintf(stderr, "Error: cannot create input stream: %s\n", url);

    int ret = demux_process_stream(args, s);
    libheimdall_release(heimdall);
    return ret;
}

int heimdall_demux_process_path(const struct heimdall_run_args *args, const char *path)
{
    char *url = heimdall_path2uri(path, NULL);
    if (url == NULL)
    {
        fprintf(stderr, "Error: cannot convert path to URL: %s\n", path);
        return -1;
    }

    int ret = heimdall_demux_process_url(args, url);
    free(url);
    return ret;
}

int libheimdall_demux_process_memory(libheimdall_instance_t *heimdall,
                                const struct heimdall_run_args *args,
                                const unsigned char *buf, size_t length)
{
    stream_t *s = heimdall_stream_MemoryNew(HEIMDALL_OBJECT(heimdall->p_libheimdall_int),
                                       (void *)buf, length, true);
    if (s == NULL)
        fprintf(stderr, "Error: cannot create input stream\n");

    return demux_process_stream(args, s);
}

int heimdall_demux_process_memory(const struct heimdall_run_args *args,
                             const unsigned char *buf, size_t length)
{
    libheimdall_instance_t *heimdall = libheimdall_create(args);
    if (heimdall == NULL)
        return -1;

    int ret = libheimdall_demux_process_memory(heimdall, args, buf, length);
    libheimdall_release(heimdall);
    return ret;
}

#ifdef HAVE_STATIC_MODULES
# include <heimdall_plugin.h>

#ifdef HAVE_DECODERS
#define DECODER_PLUGINS(f) \
    f(codec_adpcm) \
    f(codec_aes3) \
    f(codec_araw) \
    f(codec_g711) \
    f(codec_lpcm) \
    f(codec_uleaddvaudio) \
    f(codec_rawvideo) \
    f(codec_cc) \
    f(codec_cvdsub) \
    f(codec_dvbsub) \
    f(codec_scte18) \
    f(codec_scte27) \
    f(codec_spudec_spudec) \
    f(codec_stl) \
    f(codec_subsdec) \
    f(codec_subsusf) \
    f(codec_svcdsub) \
    f(codec_textst) \
    f(codec_substx3g)
#else
#define DECODER_PLUGINS(f)
#endif

#define PLUGINS(f) \
    f(misc_xml_xml) \
    f(logger_console) \
    f(access_filesystem) \
    f(demux_aiff) \
    f(demux_asf_asf) \
    f(demux_au) \
    f(demux_avi_avi) \
    f(demux_caf) \
    f(demux_demux_cdg) \
    f(demux_mpeg_es) \
    f(demux_flacsys) \
    f(demux_mpeg_h26x) \
    f(demux_mjpeg) \
    PLUGIN_MKV(f) \
    f(demux_mp4_mp4) \
    f(demux_nsv) \
    f(demux_mpeg_ps) \
    f(demux_pva) \
    f(services_discovery_sap) \
    f(demux_smf) \
    f(demux_subtitle) \
    PLUGIN_TS(f) \
    f(demux_tta) \
    f(codec_ttml_ttml) \
    f(demux_ty) \
    f(demux_voc) \
    f(demux_wav) \
    f(codec_webvtt_webvtt) \
    f(demux_xa) \
    f(packetizer_a52) \
    f(packetizer_copy) \
    f(packetizer_dts) \
    f(packetizer_flac) \
    f(packetizer_h264) \
    f(packetizer_hevc) \
    f(packetizer_mlp) \
    f(packetizer_mpeg4audio) \
    f(packetizer_mpeg4video) \
    f(packetizer_mpegaudio) \
    f(packetizer_mpegvideo) \
    f(packetizer_vc1) \
    f(demux_rawvid) \
    f(demux_rawaud) \
    f(demux_ogg) \
    DECODER_PLUGINS(f)

#ifdef HAVE_DVBPSI
# define PLUGIN_TS(f) f(demux_mpeg_ts)
#else
# define PLUGIN_TS(f)
#endif

#ifdef HAVE_MATROSKA
# define PLUGIN_MKV(f) f(demux_mkv_mkv)
#else
# define PLUGIN_MKV(f)
#endif

#define DECL_PLUGIN(p)   HEIMDALL_DECL_MODULE_ENTRY(p);

#define FUNC_PLUGIN(p)   HEIMDALL_MODULE_ENTRY(p),

PLUGINS(DECL_PLUGIN)

HEIMDALL_EXPORT const heimdall_plugin_cb heimdall_static_modules[] = {
    PLUGINS(FUNC_PLUGIN)
    NULL
};
#endif /* HAVE_STATIC_MODULES */
