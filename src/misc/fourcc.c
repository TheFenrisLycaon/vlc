/*****************************************************************************
 * fourcc.c: fourcc helpers functions
 *****************************************************************************
 * Copyright © 2009-2011 Laurent Aimar
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Laurent Aimar <fenrir@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_fourcc.h>
#include <heimdall_es.h>
#include <heimdall_chroma_probe.h>
#include <assert.h>

#include "fourcc_tables.h"

static int fourcc_cmp(const void *key, const void *ent)
{
    return memcmp(key, ent, 4);
}

static heimdall_fourcc_t Lookup(heimdall_fourcc_t fourcc, const char **restrict dsc,
                           const struct fourcc_mapping *mapv, size_t mapc,
                           const struct fourcc_desc *dscv, size_t dscc)
{
    const struct fourcc_mapping *mapping;
    const struct fourcc_desc *desc;

    mapping = bsearch(&fourcc, mapv, mapc, sizeof (*mapv), fourcc_cmp);
    if (mapping != NULL)
    {
        if (dsc != NULL)
        {
            desc = bsearch(&fourcc, dscv, dscc, sizeof (*dscv), fourcc_cmp);
            if (desc != NULL)
            {
                *dsc = desc->desc;
                return mapping->fourcc;
            }
        }
        fourcc = mapping->fourcc;
    }

    desc = bsearch(&fourcc, dscv, dscc, sizeof (*dscv), fourcc_cmp);
    if (desc == NULL)
        return 0; /* Unknown FourCC */
    if (dsc != NULL)
        *dsc = desc->desc;
    return fourcc; /* Known FourCC (has a description) */
}

static heimdall_fourcc_t LookupVideo(heimdall_fourcc_t fourcc, const char **restrict dsc)
{
    return Lookup(fourcc, dsc, mapping_video, ARRAY_SIZE(mapping_video),
                  desc_video, ARRAY_SIZE(desc_video));
}

static heimdall_fourcc_t LookupAudio(heimdall_fourcc_t fourcc, const char **restrict dsc)
{
    return Lookup(fourcc, dsc, mapping_audio, ARRAY_SIZE(mapping_audio),
                  desc_audio, ARRAY_SIZE(desc_audio));
}

static heimdall_fourcc_t LookupSpu(heimdall_fourcc_t fourcc, const char **restrict dsc)
{
    return Lookup(fourcc, dsc, mapping_spu, ARRAY_SIZE(mapping_spu),
                  desc_spu, ARRAY_SIZE(desc_spu));
}

static heimdall_fourcc_t LookupCat(heimdall_fourcc_t fourcc, const char **restrict dsc,
                              int cat)
{
    switch (cat)
    {
        case VIDEO_ES:
            return LookupVideo(fourcc, dsc);
        case AUDIO_ES:
            return LookupAudio(fourcc, dsc);
        case SPU_ES:
            return LookupSpu(fourcc, dsc);
    }

    heimdall_fourcc_t ret = LookupVideo(fourcc, dsc);
    if (!ret)
        ret = LookupAudio(fourcc, dsc);
    if (!ret)
        ret = LookupSpu(fourcc, dsc);
    return ret;
}

heimdall_fourcc_t heimdall_fourcc_GetCodec(int cat, heimdall_fourcc_t fourcc)
{
    heimdall_fourcc_t codec = LookupCat(fourcc, NULL, cat);
    return codec ? codec : fourcc;
}

heimdall_fourcc_t heimdall_fourcc_GetCodecFromString( int i_cat, const char *psz_fourcc )
{
    if( psz_fourcc == NULL )
        return 0;
    size_t s = strlen(psz_fourcc);
    if (s > 4)
        return 0;
    return heimdall_fourcc_GetCodec( i_cat,
                                HEIMDALL_FOURCC( psz_fourcc[0],
                                             s > 1 ? psz_fourcc[1] : ' ',
                                             s > 2 ? psz_fourcc[2] : ' ',
                                             s > 3 ? psz_fourcc[3] : ' ') );
}

heimdall_fourcc_t heimdall_fourcc_GetCodecAudio( heimdall_fourcc_t i_fourcc, int i_bits )
{
    if( i_fourcc == HEIMDALL_FOURCC( 'a', 'f', 'l', 't' ) )
    {
        if ( i_bits >= 25 && i_bits <= 32 )
            return HEIMDALL_CODEC_FL32;
        if ( i_bits >= 57 && i_bits <= 64 )
            return HEIMDALL_CODEC_FL64;
        return 0;
    }
    if( i_fourcc == HEIMDALL_FOURCC( 'a', 'r', 'a', 'w' ) )
    {
        if ( i_bits >= 1 && i_bits <= 8 )
            return HEIMDALL_CODEC_U8;
        if ( i_bits >= 9 && i_bits <= 16 )
            return HEIMDALL_CODEC_S16L;
        if ( i_bits >= 17 && i_bits <= 24 )
            return HEIMDALL_CODEC_S24L;
        if ( i_bits >= 25 && i_bits <= 32 )
            return HEIMDALL_CODEC_S32L;
        return 0;
    }
    if( i_fourcc == HEIMDALL_FOURCC( 't', 'w', 'o', 's' ) )
    {
        if ( i_bits >= 1 && i_bits <= 8 )
            return HEIMDALL_CODEC_S8;
        if ( i_bits >= 9 && i_bits <= 16 )
            return HEIMDALL_CODEC_S16B;
        if ( i_bits >= 17 && i_bits <= 24 )
            return HEIMDALL_CODEC_S24B;
        if ( i_bits >= 25 && i_bits <= 32 )
            return HEIMDALL_CODEC_S32B;
        return 0;
    }
    if( i_fourcc == HEIMDALL_FOURCC( 's', 'o', 'w', 't' ) )
    {
        if ( i_bits >= 1 && i_bits <= 8 )
            return HEIMDALL_CODEC_S8;
        if ( i_bits >= 9 && i_bits <= 16 )
            return HEIMDALL_CODEC_S16L;
        if ( i_bits >= 17 && i_bits <= 24 )
            return HEIMDALL_CODEC_S24L;
        if ( i_bits >= 25 && i_bits <= 32 )
            return HEIMDALL_CODEC_S32L;
        return 0;
    }
    return heimdall_fourcc_GetCodec( AUDIO_ES, i_fourcc );
}

const char *heimdall_fourcc_GetDescription(int cat, heimdall_fourcc_t fourcc)
{
    const char *ret;

    return LookupCat(fourcc, &ret, cat) ? ret : "";
}

static heimdall_fourcc_t *GetFallback( heimdall_fourcc_t i_fourcc, int flags )
{
    size_t count;
    struct heimdall_chroma_conv_result *results =
        heimdall_chroma_conv_Probe(i_fourcc, 0, 0, 0, 0, flags, &count);
    if (results == NULL)
        return NULL;

    heimdall_fourcc_t *list = heimdall_alloc(count + 1, sizeof(heimdall_fourcc_t));
    if (list == NULL)
    {
        free(results);
        return NULL;
    }

    for (size_t i = 0; i < count; ++i)
    {
        struct heimdall_chroma_conv_result *res = &results[i];
        assert(res->chain_count == 2);

        list[i] = res->chain[1];
    }
    list[count] = 0;
    free(results);
    return list;
}

heimdall_fourcc_t *heimdall_fourcc_GetYUVFallback( heimdall_fourcc_t i_fourcc )
{
    return GetFallback( i_fourcc, HEIMDALL_CHROMA_CONV_FLAG_ONLY_YUV );
}
heimdall_fourcc_t *heimdall_fourcc_GetRGBFallback( heimdall_fourcc_t i_fourcc )
{
    return GetFallback( i_fourcc, HEIMDALL_CHROMA_CONV_FLAG_ONLY_RGB );
}

heimdall_fourcc_t *heimdall_fourcc_GetFallback( heimdall_fourcc_t i_fourcc )
{
    return heimdall_fourcc_IsYUV( i_fourcc)
            ? heimdall_fourcc_GetYUVFallback( i_fourcc )
            : heimdall_fourcc_GetRGBFallback( i_fourcc );
}

#define PLANAR(subtype_, n, w_den, h_den, bits) \
      .subtype = HEIMDALL_CHROMA_SUBTYPE_##subtype_, \
      .plane_count = n, \
      .p = { {.w = {1,    1}, .h = {1,    1}}, \
             {.w = {1,w_den}, .h = {1,h_den}}, \
             {.w = {1,w_den}, .h = {1,h_den}}, \
             {.w = {1,    1}, .h = {1,    1}} }, \
      .pixel_size = ((bits + 7) / 8), \
      .pixel_bits = bits, \
      .color_bits = bits

#define PLANAR_8(subtype_, n, w_den, h_den) PLANAR(subtype_, n, w_den, h_den, 8)
#define PLANAR_16(subtype_, n, w_den, h_den, bits) PLANAR(subtype_, n, w_den, h_den, bits)

#define SEMIPLANAR(subtype_, w_den, h_den, bits) \
      .subtype = HEIMDALL_CHROMA_SUBTYPE_##subtype_, \
      .plane_count = 2, \
      .p = { {.w = {1,    1}, .h = {1,    1}}, \
             {.w = {2,w_den}, .h = {1,h_den}} }, \
      .pixel_size = ((bits + 7) / 8), \
      .pixel_bits = bits, \
      .color_bits = bits

#define PACKED_FMT(subtype_, size, bits, color_bits_) \
      .subtype = HEIMDALL_CHROMA_SUBTYPE_##subtype_, \
      .plane_count = 1, \
      .p = { {.w = {1,1}, .h = {1,1}} }, \
      .pixel_size = size, \
      .pixel_bits = bits, \
      .color_bits = color_bits_

/* Zero planes for hardware picture handles. Cannot be manipulated directly. */
#define GPU_FMT(subtype_, color_bits_) \
      .subtype = HEIMDALL_CHROMA_SUBTYPE_##subtype_, \
      .plane_count = 0, \
      .p = { {.w = {1,1}, .h = {1,1}} }, \
      .pixel_size = 0, \
      .pixel_bits = 0, \
      .color_bits = color_bits_

static const heimdall_chroma_description_t p_list_chroma_description[] = {
    { HEIMDALL_CODEC_I411,                  PLANAR_8(YUV411, 3, 4, 1) },
    { HEIMDALL_CODEC_I410,                  PLANAR_8(YUV411, 3, 4, 4) },
    { HEIMDALL_CODEC_I420,                  PLANAR_8(YUV420, 3, 2, 2) },
    { HEIMDALL_CODEC_YV12,                  PLANAR_8(YUV420, 3, 2, 2) },
    { HEIMDALL_CODEC_NV12,                  SEMIPLANAR(YUV420, 2, 2, 8) },
    { HEIMDALL_CODEC_NV21,                  SEMIPLANAR(YUV420, 2, 2, 8) },
    { HEIMDALL_CODEC_I422,                  PLANAR_8(YUV422, 3, 2, 1) },
    { HEIMDALL_CODEC_NV16,                  SEMIPLANAR(YUV422, 2, 1, 8) },
    { HEIMDALL_CODEC_NV61,                  SEMIPLANAR(YUV422, 2, 1, 8) },
    { HEIMDALL_CODEC_I440,                  PLANAR_8(YUV440, 3, 1, 2) },
    { HEIMDALL_CODEC_I444,                  PLANAR_8(YUV444, 3, 1, 1) },
    { HEIMDALL_CODEC_NV24,                  SEMIPLANAR(YUV444, 1, 1, 8) },
    { HEIMDALL_CODEC_NV42,                  SEMIPLANAR(YUV444, 1, 1, 8) },
    { HEIMDALL_CODEC_YUVA,                  PLANAR_8(YUV444, 4, 1, 1) },
    { HEIMDALL_CODEC_YUV420A,               PLANAR_8(YUV420, 4, 2, 2) },
    { HEIMDALL_CODEC_YUV422A,               PLANAR_8(YUV422, 4, 2, 1) },

    { HEIMDALL_CODEC_GBR_PLANAR,            PLANAR_8(RGB, 3, 1, 1) },
    { HEIMDALL_CODEC_GBR_PLANAR_9L,         PLANAR_16(RGB, 3, 1, 1, 9) },
    { HEIMDALL_CODEC_GBR_PLANAR_9B,         PLANAR_16(RGB, 3, 1, 1, 9) },
    { HEIMDALL_CODEC_GBR_PLANAR_10L,        PLANAR_16(RGB, 3, 1, 1, 10) },
    { HEIMDALL_CODEC_GBR_PLANAR_10B,        PLANAR_16(RGB, 3, 1, 1, 10) },
    { HEIMDALL_CODEC_GBR_PLANAR_12L,        PLANAR_16(RGB, 3, 1, 1, 12) },
    { HEIMDALL_CODEC_GBR_PLANAR_12B,        PLANAR_16(RGB, 3, 1, 1, 12) },
    { HEIMDALL_CODEC_GBR_PLANAR_14L,        PLANAR_16(RGB, 3, 1, 1, 14) },
    { HEIMDALL_CODEC_GBR_PLANAR_14B,        PLANAR_16(RGB, 3, 1, 1, 14) },
    { HEIMDALL_CODEC_GBR_PLANAR_16L,        PLANAR_16(RGB, 3, 1, 1, 16) },
    { HEIMDALL_CODEC_GBR_PLANAR_16B,        PLANAR_16(RGB, 3, 1, 1, 16) },
    { HEIMDALL_CODEC_GBRA_PLANAR,           PLANAR_8(RGB, 4, 1, 1) },
    { HEIMDALL_CODEC_GBRA_PLANAR_10L,       PLANAR_16(RGB, 4, 1, 1, 10) },
    { HEIMDALL_CODEC_GBRA_PLANAR_10B,       PLANAR_16(RGB, 4, 1, 1, 10) },
    { HEIMDALL_CODEC_GBRA_PLANAR_12L,       PLANAR_16(RGB, 4, 1, 1, 12) },
    { HEIMDALL_CODEC_GBRA_PLANAR_12B,       PLANAR_16(RGB, 4, 1, 1, 12) },
    { HEIMDALL_CODEC_GBRA_PLANAR_16L,       PLANAR_16(RGB, 4, 1, 1, 16) },
    { HEIMDALL_CODEC_GBRA_PLANAR_16B,       PLANAR_16(RGB, 4, 1, 1, 16) },

    { HEIMDALL_CODEC_I420_16L,              PLANAR_16(YUV420, 3, 2, 2, 16) },
    { HEIMDALL_CODEC_I420_16B,              PLANAR_16(YUV420, 3, 2, 2, 16) },
    { HEIMDALL_CODEC_I420_12L,              PLANAR_16(YUV420, 3, 2, 2, 12) },
    { HEIMDALL_CODEC_I420_12B,              PLANAR_16(YUV420, 3, 2, 2, 12) },
    { HEIMDALL_CODEC_I420_10L,              PLANAR_16(YUV420, 3, 2, 2, 10) },
    { HEIMDALL_CODEC_I420_10B,              PLANAR_16(YUV420, 3, 2, 2, 10) },
    { HEIMDALL_CODEC_I420_9L,               PLANAR_16(YUV420, 3, 2, 2,  9) },
    { HEIMDALL_CODEC_I420_9B,               PLANAR_16(YUV420, 3, 2, 2,  9) },
    { HEIMDALL_CODEC_I422_16L,              PLANAR_16(YUV422, 3, 2, 1, 16) },
    { HEIMDALL_CODEC_I422_16B,              PLANAR_16(YUV422, 3, 2, 1, 16) },
    { HEIMDALL_CODEC_I422_12L,              PLANAR_16(YUV422, 3, 2, 1, 12) },
    { HEIMDALL_CODEC_I422_12B,              PLANAR_16(YUV422, 3, 2, 1, 12) },
    { HEIMDALL_CODEC_I422_10L,              PLANAR_16(YUV422, 3, 2, 1, 10) },
    { HEIMDALL_CODEC_I422_10B,              PLANAR_16(YUV422, 3, 2, 1, 10) },
    { HEIMDALL_CODEC_I422_9L,               PLANAR_16(YUV422, 3, 2, 1,  9) },
    { HEIMDALL_CODEC_I422_9B,               PLANAR_16(YUV422, 3, 2, 1,  9) },
    { HEIMDALL_CODEC_I444_12L,              PLANAR_16(YUV444, 3, 1, 1, 12) },
    { HEIMDALL_CODEC_I444_12B,              PLANAR_16(YUV444, 3, 1, 1, 12) },
    { HEIMDALL_CODEC_I444_10L,              PLANAR_16(YUV444, 3, 1, 1, 10) },
    { HEIMDALL_CODEC_I444_10B,              PLANAR_16(YUV444, 3, 1, 1, 10) },
    { HEIMDALL_CODEC_I444_9L,               PLANAR_16(YUV444, 3, 1, 1,  9) },
    { HEIMDALL_CODEC_I444_9B,               PLANAR_16(YUV444, 3, 1, 1,  9) },
    { HEIMDALL_CODEC_I444_16L,              PLANAR_16(YUV444, 3, 1, 1, 16) },
    { HEIMDALL_CODEC_I444_16B,              PLANAR_16(YUV444, 3, 1, 1, 16) },
    { HEIMDALL_CODEC_YUVA_444_10L,          PLANAR_16(YUV444, 4, 1, 1, 10) },
    { HEIMDALL_CODEC_YUVA_444_10B,          PLANAR_16(YUV444, 4, 1, 1, 10) },
    { HEIMDALL_CODEC_YUVA_444_12L,          PLANAR_16(YUV444, 4, 1, 1, 12) },
    { HEIMDALL_CODEC_YUVA_444_12B,          PLANAR_16(YUV444, 4, 1, 1, 12) },
    { HEIMDALL_CODEC_P010,                  SEMIPLANAR(YUV420, 2, 2, 10) },
    { HEIMDALL_CODEC_P012,                  SEMIPLANAR(YUV420, 2, 2, 12) },
    { HEIMDALL_CODEC_P016,                  SEMIPLANAR(YUV420, 2, 2, 16) },

    { HEIMDALL_CODEC_V308,                  PACKED_FMT(YUV444, 1, 24, 8) },
    { HEIMDALL_CODEC_YUYV,                  PACKED_FMT(YUV422, 2, 16, 8) },
    { HEIMDALL_CODEC_YVYU,                  PACKED_FMT(YUV422, 2, 16, 8) },
    { HEIMDALL_CODEC_UYVY,                  PACKED_FMT(YUV422, 2, 16, 8) },
    { HEIMDALL_CODEC_VYUY,                  PACKED_FMT(YUV422, 2, 16, 8) },
    { HEIMDALL_CODEC_YUV2,                  PACKED_FMT(YUV422, 2, 16, 8) },
    { HEIMDALL_CODEC_RGB233,                PACKED_FMT(RGB, 1, 8, 2.6) },
    { HEIMDALL_CODEC_BGR233,                PACKED_FMT(RGB, 1, 8, 2.6) },
    { HEIMDALL_CODEC_RGB332,                PACKED_FMT(RGB, 1, 8, 2.6) },
    { HEIMDALL_CODEC_YUVP,                  PACKED_FMT(OTHER, 1, 8, 0) },
    { HEIMDALL_CODEC_RGBP,                  PACKED_FMT(OTHER, 1, 8, 0) },
    { HEIMDALL_CODEC_GREY,                  PACKED_FMT(GREY, 1, 8, 8) },
    { HEIMDALL_CODEC_GREY_10L,              PACKED_FMT(GREY, 2, 10, 10) },
    { HEIMDALL_CODEC_GREY_10B,              PACKED_FMT(GREY, 2, 10, 10) },
    { HEIMDALL_CODEC_GREY_12L,              PACKED_FMT(GREY, 2, 12, 12) },
    { HEIMDALL_CODEC_GREY_12B,              PACKED_FMT(GREY, 2, 12, 12) },
    { HEIMDALL_CODEC_GREY_16L,              PACKED_FMT(GREY, 2, 16, 16) },
    { HEIMDALL_CODEC_GREY_16B,              PACKED_FMT(GREY, 2, 16, 16) },

    { HEIMDALL_CODEC_RGB555BE,              PACKED_FMT(RGB, 2, 15, 5) },
    { HEIMDALL_CODEC_RGB555LE,              PACKED_FMT(RGB, 2, 15, 5) },
    { HEIMDALL_CODEC_BGR555LE,              PACKED_FMT(RGB, 2, 15, 5) },
    { HEIMDALL_CODEC_BGR555BE,              PACKED_FMT(RGB, 2, 15, 5) },
    { HEIMDALL_CODEC_RGB565LE,              PACKED_FMT(RGB, 2, 16, 5.3) },
    { HEIMDALL_CODEC_RGB565BE,              PACKED_FMT(RGB, 2, 16, 5.3) },
    { HEIMDALL_CODEC_BGR565LE,              PACKED_FMT(RGB, 2, 16, 5.3), },
    { HEIMDALL_CODEC_BGR565BE,              PACKED_FMT(RGB, 2, 16, 5.3) },
    { HEIMDALL_CODEC_RGB24,                 PACKED_FMT(RGB, 3, 24, 8) },
    { HEIMDALL_CODEC_BGR24,                 PACKED_FMT(RGB, 3, 24, 8) },
    { HEIMDALL_CODEC_RGBX,                  PACKED_FMT(RGB, 4, 24, 8) },
    { HEIMDALL_CODEC_XRGB,                  PACKED_FMT(RGB, 4, 24, 8) },
    { HEIMDALL_CODEC_BGRX,                  PACKED_FMT(RGB, 4, 24, 8) },
    { HEIMDALL_CODEC_XBGR,                  PACKED_FMT(RGB, 4, 24, 8) },
    { HEIMDALL_CODEC_RGBA,                  PACKED_FMT(RGB, 4, 32, 8) },
    { HEIMDALL_CODEC_ARGB,                  PACKED_FMT(RGB, 4, 32, 8) },
    { HEIMDALL_CODEC_BGRA,                  PACKED_FMT(RGB, 4, 32, 8) },
    { HEIMDALL_CODEC_ABGR,                  PACKED_FMT(RGB, 4, 32, 8) },
    { HEIMDALL_CODEC_RGBA10LE,              PACKED_FMT(RGB, 4, 32, 10) },
    { HEIMDALL_CODEC_RGBA64,                PACKED_FMT(RGB, 8, 64, 16) },
    { HEIMDALL_CODEC_VUYA,                  PACKED_FMT(YUV444, 4, 32, 8) },
    { HEIMDALL_CODEC_VUYX,                  PACKED_FMT(YUV444, 4, 32, 8) },
    { HEIMDALL_CODEC_Y210,                  PACKED_FMT(YUV422, 4, 32, 10) },
    { HEIMDALL_CODEC_Y212,                  PACKED_FMT(YUV422, 4, 32, 12) },
    { HEIMDALL_CODEC_Y410,                  PACKED_FMT(YUV444, 4, 32, 10) },
    { HEIMDALL_CODEC_Y412,                  PACKED_FMT(YUV444, 8, 64, 12) },
    { HEIMDALL_CODEC_Y211, HEIMDALL_CHROMA_SUBTYPE_YUV211,1, { {{1,4}, {1,1}} }, 4, 32, 8 },
    { HEIMDALL_CODEC_XYZ_12L,               PACKED_FMT(OTHER, 6, 48, 12) },
    { HEIMDALL_CODEC_XYZ_12B,               PACKED_FMT(OTHER, 6, 48, 12) },

    { HEIMDALL_CODEC_VDPAU_VIDEO,           GPU_FMT(YUV420, 8) },
    { HEIMDALL_CODEC_VDPAU_OUTPUT,          GPU_FMT(RGB, 8) },
    { HEIMDALL_CODEC_ANDROID_OPAQUE,        GPU_FMT(OTHER, 0) },
    { HEIMDALL_CODEC_MMAL_OPAQUE,           GPU_FMT(OTHER, 0) },
    { HEIMDALL_CODEC_D3D9_OPAQUE,           GPU_FMT(YUV420, 8) },
    { HEIMDALL_CODEC_D3D11_OPAQUE,          GPU_FMT(YUV420, 8) },
    { HEIMDALL_CODEC_D3D9_OPAQUE_10B,       GPU_FMT(YUV420, 10) },
    { HEIMDALL_CODEC_D3D11_OPAQUE_10B,      GPU_FMT(YUV420, 10) },
    { HEIMDALL_CODEC_D3D11_OPAQUE_RGBA,     GPU_FMT(RGB, 8) },
    { HEIMDALL_CODEC_D3D11_OPAQUE_BGRA,     GPU_FMT(RGB, 8) },
    { HEIMDALL_CODEC_D3D11_OPAQUE_ALPHA,    GPU_FMT(YUV420, 8) },

    { HEIMDALL_CODEC_NVDEC_OPAQUE_16B,      GPU_FMT(YUV420, 16) },
    { HEIMDALL_CODEC_NVDEC_OPAQUE_10B,      GPU_FMT(YUV420, 10) },
    { HEIMDALL_CODEC_NVDEC_OPAQUE,          GPU_FMT(YUV420, 8) },

    { HEIMDALL_CODEC_NVDEC_OPAQUE_444,      GPU_FMT(YUV444, 8) },
    { HEIMDALL_CODEC_NVDEC_OPAQUE_444_16B,  GPU_FMT(YUV444, 16) },

    { HEIMDALL_CODEC_CVPX_NV12,             GPU_FMT(YUV420, 8) },
    { HEIMDALL_CODEC_CVPX_UYVY,             GPU_FMT(YUV422, 8) },
    { HEIMDALL_CODEC_CVPX_I420,             GPU_FMT(YUV420, 8) },
    { HEIMDALL_CODEC_CVPX_BGRA,             GPU_FMT(RGB, 8) },

    { HEIMDALL_CODEC_CVPX_P010,             GPU_FMT(YUV420, 10) },

    { HEIMDALL_CODEC_GST_MEM_OPAQUE,        GPU_FMT(OTHER, 0) },

    { HEIMDALL_CODEC_VAAPI_420,             GPU_FMT(YUV420, 8) },
    { HEIMDALL_CODEC_VAAPI_420_10BPP,       GPU_FMT(YUV420, 10) },
    { HEIMDALL_CODEC_VAAPI_420_12BPP,       GPU_FMT(YUV420, 12) },
    { HEIMDALL_CODEC_VAAPI_422_10BPP,       GPU_FMT(YUV422, 10) },
    { HEIMDALL_CODEC_VAAPI_422_12BPP,       GPU_FMT(YUV422, 12) },
    { HEIMDALL_CODEC_VAAPI_444,             GPU_FMT(YUV444, 8) },
    { HEIMDALL_CODEC_VAAPI_444_10BPP,       GPU_FMT(YUV444, 10) },
    { HEIMDALL_CODEC_VAAPI_444_12BPP,       GPU_FMT(YUV444, 12) },
};

#undef PACKED_FMT
#undef PLANAR_16
#undef PLANAR_8
#undef PLANAR

const heimdall_chroma_description_t *heimdall_fourcc_GetChromaDescription( heimdall_fourcc_t i_fourcc )
{
    for( size_t i = 0; i < ARRAY_SIZE(p_list_chroma_description); i++ )
    {
        if( p_list_chroma_description[i].fcc == i_fourcc )
            return &p_list_chroma_description[i];
    }
    return NULL;
}
