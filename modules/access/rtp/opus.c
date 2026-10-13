/**
 * @file opus.c
 * @brief Real-Time Protocol Opus payload format parser
 */
/*****************************************************************************
 * Copyright © 2022 VideoLabs, Videolan and VLC Authors
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 ****************************************************************************/
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include <heimdall_common.h>
#include <heimdall_block.h>
#include <heimdall_es.h>
#include <heimdall_plugin.h>
#include <heimdall_strings.h>

#include "rtp.h"

static void *rtp_opus_init(struct heimdall_rtp_pt *pt)
{
    HEIMDALL_UNUSED(pt);

    es_format_t fmt;
    es_format_Init(&fmt, AUDIO_ES, HEIMDALL_CODEC_OPUS);
    static const uint8_t header[] = { 'O', 'p', 'u', 's', 'H', 'e', 'a', 'd',
                                       0x01, 0x02, /* version / 2 channels */
                                       0x00, 0x00, /* preskip */
                                       0x80, 0xbb, 0x00, 0x00, /* rate 48000 */
                                       0x00, 0x00, 0x00 }; /* gain / mapping */
    fmt.p_extra = malloc(sizeof(header));
    if(!fmt.p_extra)
        return NULL;
    fmt.i_extra = sizeof(header);
    memcpy(fmt.p_extra, header, sizeof(header));

    struct heimdall_rtp_es *es = heimdall_rtp_pt_request_es(pt, &fmt);
    es_format_Clean(&fmt);

    return es;
}

static void rtp_opus_destroy(struct heimdall_rtp_pt *pt, void *data)
{
    HEIMDALL_UNUSED(pt);
    struct heimdall_rtp_es *es = data;
    heimdall_rtp_es_destroy(es);
}

static void rtp_opus_decode(struct heimdall_rtp_pt *pt, void *data, block_t *block,
                           const struct heimdall_rtp_pktinfo *restrict info)
{
    HEIMDALL_UNUSED(pt); HEIMDALL_UNUSED(info);
    struct heimdall_rtp_es *es = data;

    if (block->i_buffer == 0)
    {
        block_Release(block);
        return;
    }

    heimdall_rtp_es_send(es, block);
}

static const struct heimdall_rtp_pt_operations rtp_opus_ops = {
    NULL, rtp_opus_init, rtp_opus_destroy, rtp_opus_decode,
};

static int rtp_opus_open(heimdall_object_t *obj, struct heimdall_rtp_pt *pt,
                        const struct heimdall_sdp_pt *desc)
{
    HEIMDALL_UNUSED(obj);

    if (heimdall_ascii_strcasecmp(desc->name, "OPUS") ||
        desc->clock_rate != 48000 ||
        desc->channel_count != 2)
        return HEIMDALL_ENOTSUP;

    pt->ops = &rtp_opus_ops;

    return HEIMDALL_SUCCESS;
}

heimdall_module_begin()
    set_shortname(N_("RTP Opus"))
    set_description(N_("RTP Opus audio payload parser"))
    set_subcategory(SUBCAT_INPUT_DEMUX)
    set_rtp_parser_callback(rtp_opus_open)
    add_shortcut("audio/opus")
heimdall_module_end()
