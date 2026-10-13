/**
 * @file mpeg4.c
 */
/*****************************************************************************
 * Copyright (C) 2024 VideoLabs, VLC authors and VideoLAN
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

#include "h26x.h"
#include "fmtp.h"
#include "../../packetizer/mpeg4audio.h"
#include "../../packetizer/mpeg4systems.h"

#include <heimdall_plugin.h>
#include <heimdall_codec.h>
#include <heimdall_bits.h>

struct mpeg4_pt_opaque
{
    // RFC 3640
    uint8_t streamType;
    uint8_t sizeLength;
    uint8_t indexLength;
    uint8_t indexDeltaLength;
    uint8_t constantSize;
    uint8_t CTSDeltaLength;
    uint8_t DTSDeltaLength;
    uint8_t randomAccessIndication;
    uint8_t auxiliaryDataSizeLength;
    // RFC 6416
    uint8_t object_id;
    // unsigned profile_level_id;
    block_t *config;
    heimdall_object_t *obj;
};

static void *rtp_mpeg4_init(struct heimdall_rtp_pt *pt, es_format_t *fmt)
{
    struct mpeg4_pt_opaque *opaque = pt->opaque;
    block_t *config = opaque->config;
    enum es_format_category_e es_cat = fmt->i_cat;
    struct rtp_h26x_sys *sys = malloc(sizeof(*sys));
    if(!sys)
        return NULL;
    rtp_h26x_init(sys);

    sys->p_packetizer = demux_PacketizerNew(opaque->obj, fmt, "rtp packetizer");
    if(!sys->p_packetizer)
    {
        free(sys);
        return NULL;
    }

    sys->es = heimdall_rtp_pt_request_es(pt, &sys->p_packetizer->fmt_out);
    if(config && es_cat == VIDEO_ES)
        sys->xps = block_Duplicate(config);

    return sys;
}

static void *rtp_mpeg4v_init(struct heimdall_rtp_pt *pt)
{
    es_format_t fmt;
    es_format_Init (&fmt, VIDEO_ES, HEIMDALL_CODEC_MP4V);
    fmt.b_packetized = false;
    return rtp_mpeg4_init(pt, &fmt);
}

static heimdall_fourcc_t audioObjectTypeToCodec(uint8_t objectType)
{
    switch(objectType)
    {
    case MPEG4_AOT_AAC_MAIN:
    case MPEG4_AOT_AAC_LC:
    case MPEG4_AOT_AAC_SSR:
    case MPEG4_AOT_AAC_LTP:
    case MPEG4_AOT_AAC_SBR:
    case MPEG4_AOT_AAC_SC:
    case MPEG4_AOT_SSC:
    case MPEG4_AOT_AAC_PS:
    case MPEG4_AOT_ER_AAC_LC:
    case MPEG4_AOT_ER_AAC_LTP:
    case MPEG4_AOT_ER_AAC_SC:
    case MPEG4_AOT_ER_BSAC:
    case MPEG4_AOT_ER_AAC_LD:
    case MPEG4_AOT_ER_AAC_ELD:
        return HEIMDALL_CODEC_MP4A;
    case MPEG4_AOT_CELP:
    case MPEG4_AOT_ER_CELP:
        return HEIMDALL_CODEC_QCELP;
    default:
        return 0;
    }
}

static void *rtp_mpeg4a_init(struct heimdall_rtp_pt *pt)
{
    struct mpeg4_pt_opaque *opaque = pt->opaque;
    es_format_t fmt;
    es_format_Init(&fmt, AUDIO_ES, 0);
    fmt.b_packetized = false;

    if(!opaque->sizeLength && !opaque->constantSize) // RFC 6416 is MP4A-LATM
    {
        fmt.i_codec = audioObjectTypeToCodec(opaque->object_id);
        fmt.i_original_fourcc = HEIMDALL_FOURCC('L','A','T','M');
    }
    else // RFC 3640 MPEG4-GENERIC
    {
        const block_t *config = opaque->config;
        if(config)
        {
            bs_t bs;
            bs_init(&bs, config->p_buffer, config->i_buffer);
            MPEG4_asc_t cfg = {0};
            if(MPEG4_read_AudioSpecificConfig(&bs, &cfg, false) == HEIMDALL_SUCCESS)
            {
                fmt.i_codec = audioObjectTypeToCodec(cfg.i_object_type);
                fmt.audio.i_rate = cfg.i_samplerate;
            }
            else fmt.i_codec = HEIMDALL_CODEC_MP4A;
        }
        else
        {
            fmt.i_codec = HEIMDALL_CODEC_MP4A;
        }
    }

    return rtp_mpeg4_init(pt, &fmt);
}

static void rtp_mpeg4_destroy(struct heimdall_rtp_pt *pt, void *data)
{
    HEIMDALL_UNUSED(pt);
    struct rtp_h26x_sys *sys = data;
    if(sys)
    {
        if(sys->p_packetizer)
            demux_PacketizerDestroy(sys->p_packetizer);
        heimdall_rtp_es_destroy(sys->es);
        rtp_h26x_clear(sys);
        free(sys);
    }
}

static void rtp_mpeg4_decode(struct heimdall_rtp_pt *pt, void *data, block_t *block,
                             const struct heimdall_rtp_pktinfo *restrict info)
{
    struct mpeg4_pt_opaque *opaque = pt->opaque;
    struct rtp_h26x_sys *sys = data;

    if(opaque->sizeLength && block->i_buffer > 2)
    {
        uint32_t auIndex = (uint32_t) -1;

        uint16_t auHeadersLength = GetWBE(block->p_buffer);
        size_t data_section_offset = 2 + (auHeadersLength + 7) / 8;
        if(data_section_offset + 2 >= block->i_buffer)
            goto drop;

        /* AUX header parsing */
        if(opaque->auxiliaryDataSizeLength)
        {
            const uint8_t *auxheader_data = &block->p_buffer[data_section_offset];
            size_t auxheader_data_size = block->i_buffer - data_section_offset;
            bs_t bs;
            bs_init(&bs, auxheader_data, auxheader_data_size);
            uint32_t auxSize = bs_read(&bs, opaque->auxiliaryDataSizeLength);
            data_section_offset += (auxheader_data_size + auxSize + 7) / 8;
            if(data_section_offset >= block->i_buffer)
                goto drop;
        }

        const uint8_t *data_section = &block->p_buffer[data_section_offset];
        size_t data_section_size = block->i_buffer - data_section_offset;

        /* AU header parsing */
        bs_t bs;
        bs_init(&bs, &block->p_buffer[2], block->i_buffer - 2);

        while(auHeadersLength >= 8)
        {
            /* Get values from current AU header */
            size_t pos = bs_pos(&bs);
            uint32_t auSize = opaque->constantSize
                            ? opaque->constantSize
                            : bs_read(&bs, opaque->sizeLength);
            if(opaque->indexLength)
            {
                uint32_t auDeltaIndex = 0;
                if(auIndex == (uint32_t) -1)
                    auIndex = bs_read(&bs, opaque->indexLength);
                else
                    auDeltaIndex = bs_read(&bs, opaque->indexDeltaLength);
                HEIMDALL_UNUSED(auDeltaIndex); // for when we'll reorder
            }

            uint32_t CTSDelta = 0, DTSDelta = 0, RAPFlag = 0;
            if(opaque->CTSDeltaLength && bs_read(&bs, 1))
                CTSDelta = bs_read(&bs, opaque->CTSDeltaLength);

            if(opaque->DTSDeltaLength && bs_read(&bs, 1))
                DTSDelta = bs_read(&bs, opaque->DTSDeltaLength);

            if(opaque->randomAccessIndication)
                RAPFlag = bs_read(&bs, 1);

            if(data_section_size < auSize)
                goto drop;

            /* Extract AU payload data using the current AU header */
            block_t *au = block_Alloc(auSize);
            if(au)
            {
                memcpy(au->p_buffer, data_section, auSize);
                if(CTSDelta)
                    block->i_pts += heimdall_tick_from_samples(CTSDelta, pt->frequency);
                if(DTSDelta)
                    block->i_dts = block->i_pts - heimdall_tick_from_samples(DTSDelta, pt->frequency);
                if(RAPFlag)
                    block->i_flags |= BLOCK_FLAG_TYPE_I;
                h26x_output(sys, au, block->i_pts, true, info->m);
            }
            data_section += auSize;
            data_section_size -= auSize;

            if(bs_eof(&bs) || bs_error(&bs))
                break;
            auHeadersLength -= bs_pos(&bs) - pos;
        }
    }
    else
    {
        h26x_output(sys, block, block->i_pts, true, info->m);
        return;
    }

drop:
    block_Release(block);
}

static void rtp_mpeg4_release(struct heimdall_rtp_pt *pt)
{
    struct mpeg4_pt_opaque *opaque = pt->opaque;
    if(opaque->config)
        block_Release(opaque->config);
    free(opaque);
}

static const struct heimdall_rtp_pt_operations rtp_mpeg4v_ops = {
    rtp_mpeg4_release, rtp_mpeg4v_init, rtp_mpeg4_destroy, rtp_mpeg4_decode,
};

static const struct heimdall_rtp_pt_operations rtp_mpeg4a_ops = {
    rtp_mpeg4_release, rtp_mpeg4a_init, rtp_mpeg4_destroy, rtp_mpeg4_decode,
};

static char hex_to_dec(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return  c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return  0;
}

static block_t * mpeg4_decode_config(const char *psz, size_t len)
{
    len &= ~1U;
    if(len == 0)
        return NULL;
    block_t *config = block_Alloc(len/2);
    if(config)
    {
        config->i_buffer = 0;
        for(size_t i=0; i<len; i+=2)
        {
            uint8_t v = hex_to_dec(psz[i]) << 4;
            v |= hex_to_dec(psz[i+1]);
            config->p_buffer[config->i_buffer++] = v;
        }
    }
    return config;
}

static int rtp_mpeg4_open(heimdall_object_t *obj, struct heimdall_rtp_pt *pt,
                          const struct heimdall_sdp_pt *desc)
{
    HEIMDALL_UNUSED(obj);
    size_t sprop_len;
    const char *sprop;
    struct mpeg4_pt_opaque tmpOpaque = {0};

    /* IETF RFC 6416 */
    if (heimdall_ascii_strcasecmp(desc->name, "MP4V-ES") == 0)
        pt->ops = &rtp_mpeg4v_ops;
    else if (heimdall_ascii_strcasecmp(desc->name, "MP4A-LATM") == 0)
        pt->ops = &rtp_mpeg4a_ops;
    /* IETF RFC 3640 */
    else if (heimdall_ascii_strcasecmp(desc->name, "MPEG4-GENERIC") == 0)
    {
        sprop = heimdall_sdp_fmtp_get_str(desc, "streamType", &sprop_len);
        if (sprop && sprop_len)
        {
            tmpOpaque.streamType = strtod(sprop, NULL);
            if(tmpOpaque.streamType == MPEG4_ST_VISUAL_STREAM)
                pt->ops = &rtp_mpeg4v_ops;
            else if(tmpOpaque.streamType == MPEG4_ST_AUDIO_STREAM)
                pt->ops = &rtp_mpeg4a_ops;
            else
                return HEIMDALL_ENOTSUP;
        }
        /* FFmpeg does not set the streamType accordingly */
        else pt->ops = &rtp_mpeg4a_ops;

        /* Get variables for our AU header */
        heimdall_sdp_fmtp_get(desc, "sizeLength", &tmpOpaque.sizeLength);
        if (tmpOpaque.sizeLength)
        {
            heimdall_sdp_fmtp_get(desc, "indexLength", &tmpOpaque.indexLength);
            heimdall_sdp_fmtp_get(desc, "indexDeltaLength", &tmpOpaque.indexDeltaLength);
        }
        else
        {
            heimdall_sdp_fmtp_get(desc, "constantSize", &tmpOpaque.constantSize);
        }

        heimdall_sdp_fmtp_get(desc, "CTSDeltaLength", &tmpOpaque.CTSDeltaLength);
        heimdall_sdp_fmtp_get(desc, "DTSDeltaLength", &tmpOpaque.DTSDeltaLength);
        heimdall_sdp_fmtp_get(desc, "randomAccessIndication", &tmpOpaque.randomAccessIndication);
        heimdall_sdp_fmtp_get(desc, "auxiliaryDataSizeLength", &tmpOpaque.auxiliaryDataSizeLength);

        // if(!tmpOpaque.constantSize && !tmpOpaque.sizeLength)
        //     return HEIMDALL_EINVAL;
    }
    else
        return HEIMDALL_ENOTSUP;

    struct mpeg4_pt_opaque *opaque = calloc(1, sizeof(*opaque));
    if(!opaque)
        return HEIMDALL_ENOMEM;
    pt->opaque = opaque;

    *opaque = tmpOpaque;
    opaque->obj = obj;

    // heimdall_sdp_fmtp_get(desc, "profile-level-id", &opaque->profile_level_id);
    heimdall_sdp_fmtp_get(desc, "object", &opaque->object_id);

    /*
     * Video:
     *  - RFC 3640: Not defined (generic)
     *  - RFC 6416: MPEG-4 Visual configuration
     * Audio:
     *  - RFC 3640: AudioSpecificConfig() element
     *  - RFC 6416: StreamMuxConfig() element
     */
    sprop = heimdall_sdp_fmtp_get_str(desc, "config", &sprop_len);
    if (sprop && sprop_len)
        opaque->config = mpeg4_decode_config(sprop, sprop_len);

    return HEIMDALL_SUCCESS;
}

heimdall_module_begin()
    set_shortname(N_("RTP MPEG-4"))
    set_description(N_("RTP MPEG-4 Visual and Audio payload parser"))
    set_subcategory(SUBCAT_INPUT_DEMUX)
    set_rtp_parser_callback(rtp_mpeg4_open)
    add_shortcut("video/MP4V-ES", "audio/MP4A-LATM", "video/MPEG4-GENERIC", "audio/MPEG4-GENERIC")
    heimdall_module_end()
