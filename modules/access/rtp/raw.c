/**
 * @file raw.c
 * @brief Real-Time Protocol raw video payload format parser
 */
/*****************************************************************************
 * Copyright © 2022 Rémi Denis-Courmont
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
#include <stdint.h>
#include <stdlib.h>

#include <heimdall_common.h>
#include <heimdall_block.h>
#include <heimdall_es.h>
#include <heimdall_plugin.h>
#include <heimdall_strings.h>

#include "rtp.h"
#include "fmtp.h"

enum heimdall_rtp_colorimetry {
    HEIMDALL_RTP_COLOR_UNKNOWN,
    HEIMDALL_RTP_COLOR_BT601_5,
    HEIMDALL_RTP_COLOR_BT709_2,
    HEIMDALL_RTP_COLOR_SMPTE240M,
};

/* video/raw: raw video ES */
struct rtp_raw {
    struct heimdall_logger *log;
    char *sampling;
    uint16_t width;
    uint16_t height;
    uint8_t depth;
    enum heimdall_rtp_colorimetry colorimetry;
};

static void *rtp_raw_begin(struct heimdall_rtp_pt *pt)
{
    struct rtp_raw *sys = pt->opaque;
    es_format_t fmt;

    es_format_Init(&fmt, VIDEO_ES, HEIMDALL_CODEC_RTP_VIDEO_RAW);
    fmt.video.i_width = sys->width;
    fmt.video.i_height = sys->height;

    switch (sys->colorimetry) {
        case HEIMDALL_RTP_COLOR_UNKNOWN:
            fmt.video.primaries = COLOR_PRIMARIES_UNDEF;
            fmt.video.transfer = TRANSFER_FUNC_UNDEF;
            fmt.video.space = COLOR_SPACE_UNDEF;
            break;
        case HEIMDALL_RTP_COLOR_BT601_5:
            fmt.video.primaries = COLOR_PRIMARIES_BT601_525;
            fmt.video.transfer = TRANSFER_FUNC_BT709;
            fmt.video.space = COLOR_SPACE_BT601;
            break;
        case HEIMDALL_RTP_COLOR_BT709_2:
            fmt.video.primaries = COLOR_PRIMARIES_BT709;
            fmt.video.transfer = TRANSFER_FUNC_BT709;
            fmt.video.space = COLOR_SPACE_BT709;
            break;
        case HEIMDALL_RTP_COLOR_SMPTE240M:
            fmt.video.primaries = COLOR_PRIMARIES_SMTPE_240;
            fmt.video.transfer = TRANSFER_FUNC_SMPTE_240;
            fmt.video.space = COLOR_SPACE_SMPTE_240;
            break;
    }

    fmt.i_level = sys->depth;
    /* Do not allocate anything and do not free anything there: */
    fmt.p_extra = sys->sampling;
    fmt.i_extra = strlen(sys->sampling) + 1;

    return heimdall_rtp_pt_request_es(pt, &fmt);
}

static void rtp_raw_end(struct heimdall_rtp_pt *pt, void *data)
{
    struct heimdall_rtp_es *es = data;

    heimdall_rtp_es_destroy(es);
    (void) pt;
}

static void rtp_raw_unwrap(struct heimdall_rtp_pt *pt, void *data, block_t *block,
                           const struct heimdall_rtp_pktinfo *restrict info)
{
    struct rtp_raw *sys = pt->opaque;
    struct heimdall_rtp_es *es = data;

    if (block->i_buffer < 8) {
        heimdall_warning(sys->log, "malformatted packet (%zu bytes)",
                    block->i_buffer);
        block_Release(block);
        return;
    }

    /* NOTE: The M bit flags the last packet of a frame or field.
     * It is not currently needed for anything. */
    (void) info;

    /* NOTE: The extended sequence header is ignored here. This would need to
     * be processed in the generic RTP code along the baseline sequence header.
     * We assume that reordering will not too severe to need that. */
    block->p_buffer += 2;
    block->i_buffer -= 2;
    heimdall_rtp_es_send(es, block);
    return;
}

static void rtp_raw_close(struct heimdall_rtp_pt *pt)
{
    struct rtp_raw *sys = pt->opaque;

    free(sys->sampling);
    free(sys);
}

static const struct heimdall_rtp_pt_operations rtp_raw_ops = {
    rtp_raw_close, rtp_raw_begin, rtp_raw_end, rtp_raw_unwrap,
};

static int rtp_raw_open(heimdall_object_t *obj, struct heimdall_rtp_pt *pt,
                        const struct heimdall_sdp_pt *desc)
{
    if (heimdall_ascii_strcasecmp(desc->name, "raw") != 0 /* RFC4175 */)
        return HEIMDALL_ENOTSUP;

    struct rtp_raw *sys = malloc(sizeof (*sys));
    if (unlikely(sys == NULL))
        return HEIMDALL_ENOMEM;

    sys->log = obj->logger;

    size_t slen, clen;
    const char *s = heimdall_sdp_fmtp_get_str(desc, "sampling", &slen);
    const char *c = heimdall_sdp_fmtp_get_str(desc, "colorimetry", &clen);
    if (s == NULL || c == NULL
     || heimdall_sdp_fmtp_get(desc, "width", &sys->width)
     || heimdall_sdp_fmtp_get(desc, "height", &sys->height)
     || heimdall_sdp_fmtp_get(desc, "depth", &sys->depth)) {
        heimdall_error(obj->logger, "missing parameters for raw video");
        free(sys);
        return HEIMDALL_EINVAL;
    }

    sys->sampling = strndup(s, slen);

    if (clen == 7 && strncmp(c, "BT601-5", 7) == 0)
        sys->colorimetry = HEIMDALL_RTP_COLOR_BT601_5;
    else if (clen == 7 && strncmp(c, "BT709-2", 7) == 0)
        sys->colorimetry = HEIMDALL_RTP_COLOR_BT709_2;
    else if (clen == 9 && strncmp(c, "SMPTE240M", 9) == 0)
        sys->colorimetry = HEIMDALL_RTP_COLOR_SMPTE240M;
    else {
        heimdall_warning(sys->log, "unknown colorimetry %.*s", (int)clen, c);
        sys->colorimetry = HEIMDALL_RTP_COLOR_UNKNOWN;
    }
    /* TODO: interlacing, chroma location (optional parameters) */

    pt->opaque = sys;
    pt->ops = &rtp_raw_ops;
    return HEIMDALL_SUCCESS;
}

heimdall_module_begin()
    set_shortname(N_("RTP raw"))
    set_description(N_("RTP raw video payload parser"))
    set_subcategory(SUBCAT_INPUT_DEMUX)
    set_rtp_parser_callback(rtp_raw_open)
    add_shortcut("video/raw")
heimdall_module_end()
