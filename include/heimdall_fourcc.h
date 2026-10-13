/*****************************************************************************
 * heimdall_fourcc.h: Definition of various FOURCC and helpers
 *****************************************************************************
 * Copyright (C) 2009 Laurent Aimar
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Laurent Aimar <fenrir _AT_ lycaria _DOT_ com>
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

#ifndef HEIMDALL_FOURCC_H
#define HEIMDALL_FOURCC_H 1

#define HEIMDALL_CODEC_UNKNOWN         HEIMDALL_FOURCC('u','n','d','f')

/* Video codec */
#define HEIMDALL_CODEC_MPGV            HEIMDALL_FOURCC('m','p','g','v')
#define HEIMDALL_CODEC_MP4V            HEIMDALL_FOURCC('m','p','4','v')
#define HEIMDALL_CODEC_DIV1            HEIMDALL_FOURCC('D','I','V','1')
#define HEIMDALL_CODEC_DIV2            HEIMDALL_FOURCC('D','I','V','2')
#define HEIMDALL_CODEC_DIV3            HEIMDALL_FOURCC('D','I','V','3')
#define HEIMDALL_CODEC_SVQ1            HEIMDALL_FOURCC('S','V','Q','1')
#define HEIMDALL_CODEC_SVQ3            HEIMDALL_FOURCC('S','V','Q','3')
#define HEIMDALL_CODEC_H264            HEIMDALL_FOURCC('h','2','6','4')
#define HEIMDALL_CODEC_H263            HEIMDALL_FOURCC('h','2','6','3')
#define HEIMDALL_CODEC_H263I           HEIMDALL_FOURCC('I','2','6','3')
#define HEIMDALL_CODEC_H263P           HEIMDALL_FOURCC('I','L','V','R')
#define HEIMDALL_CODEC_FLV1            HEIMDALL_FOURCC('F','L','V','1')
#define HEIMDALL_CODEC_H261            HEIMDALL_FOURCC('h','2','6','1')
#define HEIMDALL_CODEC_VVC             HEIMDALL_FOURCC('h','2','6','6')
#define HEIMDALL_CODEC_MJPG            HEIMDALL_FOURCC('M','J','P','G')
#define HEIMDALL_CODEC_MJPGB           HEIMDALL_FOURCC('m','j','p','b')
#define HEIMDALL_CODEC_LJPG            HEIMDALL_FOURCC('L','J','P','G')
#define HEIMDALL_CODEC_WMV1            HEIMDALL_FOURCC('W','M','V','1')
#define HEIMDALL_CODEC_WMV2            HEIMDALL_FOURCC('W','M','V','2')
#define HEIMDALL_CODEC_WMV3            HEIMDALL_FOURCC('W','M','V','3')
#define HEIMDALL_CODEC_WMVA            HEIMDALL_FOURCC('W','M','V','A')
#define HEIMDALL_CODEC_WMVP            HEIMDALL_FOURCC('W','M','V','P')
#define HEIMDALL_CODEC_WMVP2           HEIMDALL_FOURCC('W','V','P','2')
#define HEIMDALL_CODEC_VC1             HEIMDALL_FOURCC('V','C','-','1')
#define HEIMDALL_CODEC_DAALA           HEIMDALL_FOURCC('d','a','a','l')
#define HEIMDALL_CODEC_THEORA          HEIMDALL_FOURCC('t','h','e','o')
#define HEIMDALL_CODEC_TARKIN          HEIMDALL_FOURCC('t','a','r','k')
#define HEIMDALL_CODEC_DIRAC           HEIMDALL_FOURCC('d','r','a','c')
#define HEIMDALL_CODEC_OGGSPOTS        HEIMDALL_FOURCC('S','P','O','T')
#define HEIMDALL_CODEC_CAVS            HEIMDALL_FOURCC('C','A','V','S')
#define HEIMDALL_CODEC_CAVS2           HEIMDALL_FOURCC('A','V','S','2')
#define HEIMDALL_CODEC_CAVS3           HEIMDALL_FOURCC('A','V','S','3')
#define HEIMDALL_CODEC_NUV             HEIMDALL_FOURCC('N','J','P','G')
#define HEIMDALL_CODEC_RV10            HEIMDALL_FOURCC('R','V','1','0')
#define HEIMDALL_CODEC_RV13            HEIMDALL_FOURCC('R','V','1','3')
#define HEIMDALL_CODEC_RV20            HEIMDALL_FOURCC('R','V','2','0')
#define HEIMDALL_CODEC_RV30            HEIMDALL_FOURCC('R','V','3','0')
#define HEIMDALL_CODEC_RV40            HEIMDALL_FOURCC('R','V','4','0')
#define HEIMDALL_CODEC_VP3             HEIMDALL_FOURCC('V','P','3',' ')
#define HEIMDALL_CODEC_VP5             HEIMDALL_FOURCC('V','P','5',' ')
#define HEIMDALL_CODEC_VP6             HEIMDALL_FOURCC('V','P','6','2')
#define HEIMDALL_CODEC_VP6F            HEIMDALL_FOURCC('V','P','6','F')
#define HEIMDALL_CODEC_VP6A            HEIMDALL_FOURCC('V','P','6','A')
#define HEIMDALL_CODEC_MSVIDEO1        HEIMDALL_FOURCC('M','S','V','C')
#define HEIMDALL_CODEC_FLIC            HEIMDALL_FOURCC('F','L','I','C')
#define HEIMDALL_CODEC_SP5X            HEIMDALL_FOURCC('S','P','5','X')
#define HEIMDALL_CODEC_DV              HEIMDALL_FOURCC('d','v',' ',' ')
#define HEIMDALL_CODEC_MSRLE           HEIMDALL_FOURCC('m','r','l','e')
#define HEIMDALL_CODEC_HUFFYUV         HEIMDALL_FOURCC('H','F','Y','U')
#define HEIMDALL_CODEC_FFVHUFF         HEIMDALL_FOURCC('F','F','V','H')
#define HEIMDALL_CODEC_ASV1            HEIMDALL_FOURCC('A','S','V','1')
#define HEIMDALL_CODEC_ASV2            HEIMDALL_FOURCC('A','S','V','2')
#define HEIMDALL_CODEC_FFV1            HEIMDALL_FOURCC('F','F','V','1')
#define HEIMDALL_CODEC_VCR1            HEIMDALL_FOURCC('V','C','R','1')
#define HEIMDALL_CODEC_CLJR            HEIMDALL_FOURCC('C','L','J','R')
#define HEIMDALL_CODEC_RPZA            HEIMDALL_FOURCC('r','p','z','a')
#define HEIMDALL_CODEC_SMC             HEIMDALL_FOURCC('s','m','c',' ')
#define HEIMDALL_CODEC_CINEPAK         HEIMDALL_FOURCC('C','V','I','D')
#define HEIMDALL_CODEC_TSCC            HEIMDALL_FOURCC('T','S','C','C')
#define HEIMDALL_CODEC_CSCD            HEIMDALL_FOURCC('C','S','C','D')
#define HEIMDALL_CODEC_ZMBV            HEIMDALL_FOURCC('Z','M','B','V')
#define HEIMDALL_CODEC_VMNC            HEIMDALL_FOURCC('V','M','n','c')
#define HEIMDALL_CODEC_FMVC            HEIMDALL_FOURCC('F','M','V','C')
#define HEIMDALL_CODEC_FRAPS           HEIMDALL_FOURCC('F','P','S','1')
#define HEIMDALL_CODEC_TRUEMOTION1     HEIMDALL_FOURCC('D','U','C','K')
#define HEIMDALL_CODEC_TRUEMOTION2     HEIMDALL_FOURCC('T','M','2','0')
#define HEIMDALL_CODEC_QTRLE           HEIMDALL_FOURCC('r','l','e',' ')
#define HEIMDALL_CODEC_QDRAW           HEIMDALL_FOURCC('q','d','r','w')
#define HEIMDALL_CODEC_QOI             HEIMDALL_FOURCC('q','o','i','f')
#define HEIMDALL_CODEC_QPEG            HEIMDALL_FOURCC('Q','P','E','G')
#define HEIMDALL_CODEC_ULTI            HEIMDALL_FOURCC('U','L','T','I')
#define HEIMDALL_CODEC_VIXL            HEIMDALL_FOURCC('V','I','X','L')
#define HEIMDALL_CODEC_LOCO            HEIMDALL_FOURCC('L','O','C','O')
#define HEIMDALL_CODEC_WNV1            HEIMDALL_FOURCC('W','N','V','1')
#define HEIMDALL_CODEC_AASC            HEIMDALL_FOURCC('A','A','S','C')
#define HEIMDALL_CODEC_INDEO2          HEIMDALL_FOURCC('I','V','2','0')
#define HEIMDALL_CODEC_INDEO3          HEIMDALL_FOURCC('I','V','3','1')
#define HEIMDALL_CODEC_INDEO4          HEIMDALL_FOURCC('I','V','4','1')
#define HEIMDALL_CODEC_INDEO5          HEIMDALL_FOURCC('I','V','5','0')
#define HEIMDALL_CODEC_FLASHSV         HEIMDALL_FOURCC('F','S','V','1')
#define HEIMDALL_CODEC_KMVC            HEIMDALL_FOURCC('K','M','V','C')
#define HEIMDALL_CODEC_SMACKVIDEO      HEIMDALL_FOURCC('S','M','K','2')
#define HEIMDALL_CODEC_DNXHD           HEIMDALL_FOURCC('A','V','d','n')
#define HEIMDALL_CODEC_8BPS            HEIMDALL_FOURCC('8','B','P','S')
#define HEIMDALL_CODEC_MIMIC           HEIMDALL_FOURCC('M','L','2','O')
#define HEIMDALL_CODEC_INTERPLAY       HEIMDALL_FOURCC('i','m','v','e')
#define HEIMDALL_CODEC_IDCIN           HEIMDALL_FOURCC('I','D','C','I')
#define HEIMDALL_CODEC_4XM             HEIMDALL_FOURCC('4','X','M','V')
#define HEIMDALL_CODEC_ROQ             HEIMDALL_FOURCC('R','o','Q','v')
#define HEIMDALL_CODEC_MDEC            HEIMDALL_FOURCC('M','D','E','C')
#define HEIMDALL_CODEC_VMDVIDEO        HEIMDALL_FOURCC('V','M','D','V')
#define HEIMDALL_CODEC_CDG             HEIMDALL_FOURCC('C','D','G',' ')
#define HEIMDALL_CODEC_FRWU            HEIMDALL_FOURCC('F','R','W','U')
#define HEIMDALL_CODEC_AMV             HEIMDALL_FOURCC('A','M','V',' ')
#define HEIMDALL_CODEC_VP4             HEIMDALL_FOURCC('V','P','4','0')
#define HEIMDALL_CODEC_VP7             HEIMDALL_FOURCC('V','P','7','0')
#define HEIMDALL_CODEC_VP8             HEIMDALL_FOURCC('V','P','8','0')
#define HEIMDALL_CODEC_VP8ALPHA_ES     HEIMDALL_FOURCC('V','P','8','a')
#define HEIMDALL_CODEC_VP9             HEIMDALL_FOURCC('V','P','9','0')
#define HEIMDALL_CODEC_VP9ALPHA_ES     HEIMDALL_FOURCC('V','P','9','a')
#define HEIMDALL_CODEC_VP10            HEIMDALL_FOURCC('V','P',':','0')
#define HEIMDALL_CODEC_AV1             HEIMDALL_FOURCC('a','v','0','1')
#define HEIMDALL_CODEC_JPEG2000        HEIMDALL_FOURCC('J','P','2','K')
#define HEIMDALL_CODEC_LAGARITH        HEIMDALL_FOURCC('L','A','G','S')
#define HEIMDALL_CODEC_FLASHSV2        HEIMDALL_FOURCC('F','S','V','2')
#define HEIMDALL_CODEC_PRORES          HEIMDALL_FOURCC('a','p','c','n')
#define HEIMDALL_CODEC_MXPEG           HEIMDALL_FOURCC('M','X','P','G')
#define HEIMDALL_CODEC_CDXL            HEIMDALL_FOURCC('C','D','X','L')
#define HEIMDALL_CODEC_BMVVIDEO        HEIMDALL_FOURCC('B','M','V','V')
#define HEIMDALL_CODEC_UTVIDEO         HEIMDALL_FOURCC('U','L','R','A')
#define HEIMDALL_CODEC_VBLE            HEIMDALL_FOURCC('V','B','L','E')
#define HEIMDALL_CODEC_DXTORY          HEIMDALL_FOURCC('x','t','o','r')
#define HEIMDALL_CODEC_MSS1            HEIMDALL_FOURCC('M','S','S','1')
#define HEIMDALL_CODEC_MSS2            HEIMDALL_FOURCC('M','S','S','2')
#define HEIMDALL_CODEC_MSA1            HEIMDALL_FOURCC('M','S','A','1')
#define HEIMDALL_CODEC_TSC2            HEIMDALL_FOURCC('T','S','C','2')
#define HEIMDALL_CODEC_MTS2            HEIMDALL_FOURCC('M','T','S','2')
#define HEIMDALL_CODEC_HEVC            HEIMDALL_FOURCC('h','e','v','c')
#define HEIMDALL_CODEC_ICOD            HEIMDALL_FOURCC('i','c','o','d')
#define HEIMDALL_CODEC_G2M2            HEIMDALL_FOURCC('G','2','M','2')
#define HEIMDALL_CODEC_G2M3            HEIMDALL_FOURCC('G','2','M','3')
#define HEIMDALL_CODEC_G2M4            HEIMDALL_FOURCC('G','2','M','4')
#define HEIMDALL_CODEC_BINKVIDEO       HEIMDALL_FOURCC('B','I','K','f')
#define HEIMDALL_CODEC_BINKAUDIO_DCT   HEIMDALL_FOURCC('B','A','U','1')
#define HEIMDALL_CODEC_BINKAUDIO_RDFT  HEIMDALL_FOURCC('B','A','U','2')
#define HEIMDALL_CODEC_XAN_WC4         HEIMDALL_FOURCC('X','x','a','n')
#define HEIMDALL_CODEC_LCL_MSZH        HEIMDALL_FOURCC('M','S','Z','H')
#define HEIMDALL_CODEC_LCL_ZLIB        HEIMDALL_FOURCC('Z','L','I','B')
#define HEIMDALL_CODEC_THP             HEIMDALL_FOURCC('T','H','P','0')
#define HEIMDALL_CODEC_ESCAPE124       HEIMDALL_FOURCC('E','1','2','4')
#define HEIMDALL_CODEC_KGV1            HEIMDALL_FOURCC('K','G','V','1')
#define HEIMDALL_CODEC_CLLC            HEIMDALL_FOURCC('C','L','L','C')
#define HEIMDALL_CODEC_AURA            HEIMDALL_FOURCC('A','U','R','A')
#define HEIMDALL_CODEC_FIC             HEIMDALL_FOURCC('F','I','C','V')
#define HEIMDALL_CODEC_TMV             HEIMDALL_FOURCC('T','M','A','V')
#define HEIMDALL_CODEC_XAN_WC3         HEIMDALL_FOURCC('X','A','N','3')
#define HEIMDALL_CODEC_WS_VQA          HEIMDALL_FOURCC('W','V','Q','A')
#define HEIMDALL_CODEC_MMVIDEO         HEIMDALL_FOURCC('M','M','V','I')
#define HEIMDALL_CODEC_AVS             HEIMDALL_FOURCC('A','V','S','V')
#define HEIMDALL_CODEC_DSICINVIDEO     HEIMDALL_FOURCC('D','C','I','V')
#define HEIMDALL_CODEC_TIERTEXSEQVIDEO HEIMDALL_FOURCC('T','S','E','Q')
#define HEIMDALL_CODEC_DXA             HEIMDALL_FOURCC('D','E','X','A')
#define HEIMDALL_CODEC_C93             HEIMDALL_FOURCC('I','C','9','3')
#define HEIMDALL_CODEC_BETHSOFTVID     HEIMDALL_FOURCC('B','V','I','D')
#define HEIMDALL_CODEC_VB              HEIMDALL_FOURCC('V','B','V','1')
#define HEIMDALL_CODEC_RL2             HEIMDALL_FOURCC('R','L','V','2')
#define HEIMDALL_CODEC_BFI             HEIMDALL_FOURCC('B','F','&','I')
#define HEIMDALL_CODEC_CMV             HEIMDALL_FOURCC('E','C','M','V')
#define HEIMDALL_CODEC_MOTIONPIXELS    HEIMDALL_FOURCC('M','P','I','X')
#define HEIMDALL_CODEC_TGV             HEIMDALL_FOURCC('T','G','V','V')
#define HEIMDALL_CODEC_TGQ             HEIMDALL_FOURCC('T','G','Q','V')
#define HEIMDALL_CODEC_TQI             HEIMDALL_FOURCC('T','Q','I','V')
#define HEIMDALL_CODEC_MAD             HEIMDALL_FOURCC('M','A','D','V')
#define HEIMDALL_CODEC_ANM             HEIMDALL_FOURCC('A','N','I','M')
#define HEIMDALL_CODEC_YOP             HEIMDALL_FOURCC('Y','O','P','V')
#define HEIMDALL_CODEC_JV              HEIMDALL_FOURCC('J','V','0','0')
#define HEIMDALL_CODEC_DFA             HEIMDALL_FOURCC('D','F','I','A')
#define HEIMDALL_CODEC_HNM4_VIDEO      HEIMDALL_FOURCC('H','N','M','4')
#define HEIMDALL_CODEC_TDSC            HEIMDALL_FOURCC('T','D','S','C')
#define HEIMDALL_CODEC_HQX             HEIMDALL_FOURCC('C','H','Q','X')
#define HEIMDALL_CODEC_HQ_HQA          HEIMDALL_FOURCC('C','U','V','C')
#define HEIMDALL_CODEC_HAP             HEIMDALL_FOURCC('H','A','P','1')
#define HEIMDALL_CODEC_DXV             HEIMDALL_FOURCC('D','X','D','3')
#define HEIMDALL_CODEC_CINEFORM        HEIMDALL_FOURCC('C','F','H','D')
#define HEIMDALL_CODEC_SPEEDHQ         HEIMDALL_FOURCC('S','H','Q','2')
#define HEIMDALL_CODEC_PIXLET          HEIMDALL_FOURCC('p','x','l','t')
#define HEIMDALL_CODEC_MAGICYUV        HEIMDALL_FOURCC('M','8','Y','0')
#define HEIMDALL_CODEC_IMM4            HEIMDALL_FOURCC('I','M','M','4')
#define HEIMDALL_CODEC_IMM5            HEIMDALL_FOURCC('I','M','M','5')
#define HEIMDALL_CODEC_AGM             HEIMDALL_FOURCC('A','G','M','0')
#define HEIMDALL_CODEC_NOTCHLC         HEIMDALL_FOURCC('n','c','l','c')
#define HEIMDALL_CODEC_RTP_VIDEO_RAW   HEIMDALL_FOURCC('R','T','P','V')
#define HEIMDALL_CODEC_APV             HEIMDALL_FOURCC('a','p','v','1')

/***********
 * Chromas
 ***********/

/* Planar YUV */

/* Planar YUV 4:1:0 Y:U:V */
#define HEIMDALL_CODEC_I410            HEIMDALL_FOURCC('I','4','1','0')
/* Planar YUV 4:1:1 Y:U:V */
#define HEIMDALL_CODEC_I411            HEIMDALL_FOURCC('I','4','1','1')

/* Planar YUV 4:2:0 Y:V:U */
#define HEIMDALL_CODEC_YV12            HEIMDALL_FOURCC('Y','V','1','2')
/* Planar YUV 4:2:0 Y:U:V 8-bit */
#define HEIMDALL_CODEC_I420            HEIMDALL_FOURCC('I','4','2','0')
/* Planar YUV 4:2:0 Y:U:V  9-bit stored on 16 bits */
#define HEIMDALL_CODEC_I420_9L         HEIMDALL_FOURCC('I','0','9','L')
#define HEIMDALL_CODEC_I420_9B         HEIMDALL_FOURCC('I','0','9','B')
/* Planar YUV 4:2:0 Y:U:V 10-bit stored on 16 bits LSB */
#define HEIMDALL_CODEC_I420_10L        HEIMDALL_FOURCC('I','0','A','L')
#define HEIMDALL_CODEC_I420_10B        HEIMDALL_FOURCC('I','0','A','B')
/* Planar YUV 4:2:0 Y:U:V 12-bit stored on 16 bits LSB */
#define HEIMDALL_CODEC_I420_12L        HEIMDALL_FOURCC('I','0','C','L')
#define HEIMDALL_CODEC_I420_12B        HEIMDALL_FOURCC('I','0','C','B')

/* Planar YUV 4:2:0 Y:U:V 16-bit stored on 16 bits */
#define HEIMDALL_CODEC_I420_16L        HEIMDALL_FOURCC('I','0','F','L')
#define HEIMDALL_CODEC_I420_16B        HEIMDALL_FOURCC('I','0','F','B')

/* Planar YUV 4:2:2 Y:U:V 8-bit */
#define HEIMDALL_CODEC_I422            HEIMDALL_FOURCC('I','4','2','2')
/* Planar YUV 4:2:2 Y:U:V  9-bit stored on 16 bits LSB */
#define HEIMDALL_CODEC_I422_9L         HEIMDALL_FOURCC('I','2','9','L')
#define HEIMDALL_CODEC_I422_9B         HEIMDALL_FOURCC('I','2','9','B')
/* Planar YUV 4:2:2 Y:U:V 10-bit stored on 16 bits LSB */
#define HEIMDALL_CODEC_I422_10L        HEIMDALL_FOURCC('I','2','A','L')
#define HEIMDALL_CODEC_I422_10B        HEIMDALL_FOURCC('I','2','A','B')
/* Planar YUV 4:2:2 Y:U:V 12-bit stored on 16 bits LSB */
#define HEIMDALL_CODEC_I422_12L        HEIMDALL_FOURCC('I','2','C','L')
#define HEIMDALL_CODEC_I422_12B        HEIMDALL_FOURCC('I','2','C','B')
/* Planar YUV 4:2:2 Y:U:V 16-bit stored on 16 bits */
#define HEIMDALL_CODEC_I422_16L        HEIMDALL_FOURCC('I','2','F','L')
#define HEIMDALL_CODEC_I422_16B        HEIMDALL_FOURCC('I','2','F','B')

/* Planar YUV 4:4:0 Y:U:V */
#define HEIMDALL_CODEC_I440            HEIMDALL_FOURCC('I','4','4','0')
/* Planar YUV 4:4:4 Y:U:V 8-bit */
#define HEIMDALL_CODEC_I444            HEIMDALL_FOURCC('I','4','4','4')
/* Planar YUV 4:4:4 Y:U:V  9-bit stored on 16 bits LSB */
#define HEIMDALL_CODEC_I444_9L         HEIMDALL_FOURCC('I','4','9','L')
#define HEIMDALL_CODEC_I444_9B         HEIMDALL_FOURCC('I','4','9','B')
/* Planar YUV 4:4:4 Y:U:V 10-bit stored on 16 bits LSB */
#define HEIMDALL_CODEC_I444_10L        HEIMDALL_FOURCC('I','4','A','L')
#define HEIMDALL_CODEC_I444_10B        HEIMDALL_FOURCC('I','4','A','B')
/* Planar YUV 4:4:4 Y:U:V 12-bit stored on 16 bits LSB */
#define HEIMDALL_CODEC_I444_12L        HEIMDALL_FOURCC('I','4','C','L')
#define HEIMDALL_CODEC_I444_12B        HEIMDALL_FOURCC('I','4','C','B')
/* Planar YUV 4:4:4 Y:U:V 16-bit */
#define HEIMDALL_CODEC_I444_16L        HEIMDALL_FOURCC('I','4','F','L')
#define HEIMDALL_CODEC_I444_16B        HEIMDALL_FOURCC('I','4','F','B')

/* Palettized YUV with palette 8-bit Y:U:V:A in memory order */
#define HEIMDALL_CODEC_YUVP            HEIMDALL_FOURCC('Y','U','V','P')

/* Planar YUV 4:4:4 Y:U:V:A */
#define HEIMDALL_CODEC_YUVA            HEIMDALL_FOURCC('Y','U','V','A')
/* Planar YUV 4:2:2 Y:U:V:A */
#define HEIMDALL_CODEC_YUV422A         HEIMDALL_FOURCC('I','4','2','A')
/* Planar YUV 4:2:0 Y:U:V:A */
#define HEIMDALL_CODEC_YUV420A         HEIMDALL_FOURCC('I','4','0','A')

/* Planar Y:U:V:A 4:4:4 10bits */
#define HEIMDALL_CODEC_YUVA_444_10L    HEIMDALL_FOURCC('Y','A','0','L')
#define HEIMDALL_CODEC_YUVA_444_10B    HEIMDALL_FOURCC('Y','A','0','B')

/* Planar Y:U:V:A 4:4:4 12bits */
#define HEIMDALL_CODEC_YUVA_444_12L    HEIMDALL_FOURCC('Y','A','2','L')
#define HEIMDALL_CODEC_YUVA_444_12B    HEIMDALL_FOURCC('Y','A','2','B')

/* Semi-planar Y/UV */

/* 2 planes Y/UV 4:2:0 */
#define HEIMDALL_CODEC_NV12            HEIMDALL_FOURCC('N','V','1','2')
/* 2 planes Y/VU 4:2:0 */
#define HEIMDALL_CODEC_NV21            HEIMDALL_FOURCC('N','V','2','1')
/* 2 planes Y/UV 4:2:2 */
#define HEIMDALL_CODEC_NV16            HEIMDALL_FOURCC('N','V','1','6')
/* 2 planes Y/VU 4:2:2 */
#define HEIMDALL_CODEC_NV61            HEIMDALL_FOURCC('N','V','6','1')
/* 2 planes Y/UV 4:4:4 */
#define HEIMDALL_CODEC_NV24            HEIMDALL_FOURCC('N','V','2','4')
/* 2 planes Y/VU 4:4:4 */
#define HEIMDALL_CODEC_NV42            HEIMDALL_FOURCC('N','V','4','2')
/* 2 planes Y/UV 4:2:0 10-bit MSB, little endian */
#define HEIMDALL_CODEC_P010            HEIMDALL_FOURCC('P','0','1','0')
/* 2 planes Y/UV 4:2:0 12-bit MSB, little endian */
#define HEIMDALL_CODEC_P012            HEIMDALL_FOURCC('P','0','1','2')
/* 2 planes Y/UV 4:2:0 16-bit, little endian */
#define HEIMDALL_CODEC_P016            HEIMDALL_FOURCC('P','0','1','6')

/* Packed YUV */

/* Packed YUV 4:2:0, U:V:Y */
#define HEIMDALL_CODEC_YUV4            HEIMDALL_FOURCC('y','u','v','4')
/* Packed YUV 4:2:2, U:Y:V:Y */
#define HEIMDALL_CODEC_UYVY            HEIMDALL_FOURCC('U','Y','V','Y')
/* Packed YUV 4:2:2, V:Y:U:Y */
#define HEIMDALL_CODEC_VYUY            HEIMDALL_FOURCC('V','Y','U','Y')
/* Packed YUV 4:2:2, Y:U:Y:V */
#define HEIMDALL_CODEC_YUYV            HEIMDALL_FOURCC('Y','U','Y','2')
/* Packed YUV 4:2:2, Y:U:Y:V, signed */
#define HEIMDALL_CODEC_YUV2            HEIMDALL_FOURCC('y','u','v','2')
/* Packed YUV 4:2:2, Y:V:Y:U */
#define HEIMDALL_CODEC_YVYU            HEIMDALL_FOURCC('Y','V','Y','U')
/* Packed YUV 2:1:1, Y:U:Y:V */
#define HEIMDALL_CODEC_Y211            HEIMDALL_FOURCC('Y','2','1','1')
/* Packed YUV 4:2:2, U:Y:V:Y, reverted */
#define HEIMDALL_CODEC_CYUV            HEIMDALL_FOURCC('c','y','u','v')
/* Packed YUV 4:2:2 10-bit U10:Y10:V10:Y10:X2 (12 on 4*32bits) */
#define HEIMDALL_CODEC_V210            HEIMDALL_FOURCC('v','2','1','0')
/* Packed YUV 4:4:4 */
#define HEIMDALL_CODEC_V308            HEIMDALL_FOURCC('v','3','0','8')
/* Packed YUVA 4:4:4:4 */
#define HEIMDALL_CODEC_V408            HEIMDALL_FOURCC('v','4','0','8')
/* Packed YUV 4:4:4 10-bit X2:U10:Y10:V10:Y10 */
#define HEIMDALL_CODEC_V410            HEIMDALL_FOURCC('v','4','1','0')
/* I420 packed for RTP (RFC 4175) */
#define HEIMDALL_CODEC_R420            HEIMDALL_FOURCC('r','4','2','0')
/* Packed YUV 4:2:2 10-bit V10:U10:Y10:A2 */
#define HEIMDALL_CODEC_Y210            HEIMDALL_FOURCC('Y','2','1','0')
/* Packed YUV 4:2:2 12-bit V12:U12:Y12:A2 */
#define HEIMDALL_CODEC_Y212            HEIMDALL_FOURCC('Y','2','1','2')
/* Packed YUV 4:4:4 10-bit V10:U10:Y10:A2 */
#define HEIMDALL_CODEC_Y410            HEIMDALL_FOURCC('Y','4','1','0')
/* Packed YUV 4:4:4 12-bit V12:U12:Y12:A2 */
#define HEIMDALL_CODEC_Y412            HEIMDALL_FOURCC('Y','4','1','2')
/* Packed YUV 4:4:4 V:U:Y:A */
#define HEIMDALL_CODEC_VUYA            HEIMDALL_FOURCC('V','U','Y','A')
/* Packed YUV 4:4:4 V:U:Y:X, alpha channel undefined */
#define HEIMDALL_CODEC_VUYX            HEIMDALL_FOURCC('V','U','Y','X')

/* RGB / RGBA */

/* Palettized 8-bit RGB with palette element 8-bit R:G:B:A in memory order */
#define HEIMDALL_CODEC_RGBP            HEIMDALL_FOURCC('R','G','B','P')

/* 32-bit RGB, in memory address order: "XRGB" ignoring the x component */
#define HEIMDALL_CODEC_XRGB            HEIMDALL_FOURCC('R','V','3','2')
/* 32-bit RGB, in memory address order: "RGBX" ignoring the x component */
#define HEIMDALL_CODEC_RGBX            HEIMDALL_FOURCC('R','G','B','X')
/* 32-bit BGR, in memory address order: "BGRX" ignoring the x component */
#define HEIMDALL_CODEC_BGRX            HEIMDALL_FOURCC('B','G','R','X')
/* 32-bit BGR, in memory address order: "XBGR" ignoring the x component */
#define HEIMDALL_CODEC_XBGR            HEIMDALL_FOURCC('X','B','G','R')

/* 24-bit RGB, in memory address order: "RGB" */
#define HEIMDALL_CODEC_RGB24           HEIMDALL_FOURCC('R','V','2','4')
/* 24-bit BGR, in memory address order: "BGR" */
#define HEIMDALL_CODEC_BGR24           HEIMDALL_FOURCC('B','G','R','3')

/* 16-bit RGB Big-Endian: "R5G6B5" */
#define HEIMDALL_CODEC_RGB565BE          HEIMDALL_FOURCC('R','B','1','6')
/* 16-bit RGB Little-Endian: "g3B5 R5G3" */
#define HEIMDALL_CODEC_RGB565LE          HEIMDALL_FOURCC('R','V','1','6')
/* 16-bit BGR Big-Endian: "B5G6R5" */
#define HEIMDALL_CODEC_BGR565BE          HEIMDALL_FOURCC('B','B','1','6')
/* 16-bit BGR Little-Endian: "g3R5 B5G3" */
#define HEIMDALL_CODEC_BGR565LE          HEIMDALL_FOURCC('B','L','1','6')

#ifdef WORDS_BIGENDIAN
/* 16-bit RGB with host endianness */
#define HEIMDALL_CODEC_RGB565    HEIMDALL_CODEC_RGB565BE
/* 16-bit BGR with host endianness */
#define HEIMDALL_CODEC_BGR565    HEIMDALL_CODEC_BGR565BE
#else
/* 16-bit RGB with host endianness */
#define HEIMDALL_CODEC_RGB565    HEIMDALL_CODEC_RGB565LE
/* 16-bit BGR with host endianness */
#define HEIMDALL_CODEC_BGR565    HEIMDALL_CODEC_BGR565LE
#endif

/* 15-bit RGB Big-Endian: "0R5G5B5" */
#define HEIMDALL_CODEC_RGB555BE          HEIMDALL_FOURCC('R','B','1','5')
/* 15-bit RGB Little-Endian: "g3B5 0R5G2" */
#define HEIMDALL_CODEC_RGB555LE          HEIMDALL_FOURCC('R','V','1','5')
/* 15-bit BGR Big-Endian: "0B5G5R5" */
#define HEIMDALL_CODEC_BGR555BE          HEIMDALL_FOURCC('B','B','1','5')
/* 15-bit BGR Little-Endian: "g3R5 0B5G2" */
#define HEIMDALL_CODEC_BGR555LE          HEIMDALL_FOURCC('B','L','1','5')

#ifdef WORDS_BIGENDIAN
/* 15-bit RGB with host endianness */
#define HEIMDALL_CODEC_RGB555    HEIMDALL_CODEC_RGB555BE
/* 15-bit BGR with host endianness */
#define HEIMDALL_CODEC_BGR555    HEIMDALL_CODEC_BGR555BE
#else
/* 15-bit RGB with host endianness */
#define HEIMDALL_CODEC_RGB555    HEIMDALL_CODEC_RGB555LE
/* 15-bit BGR with host endianness */
#define HEIMDALL_CODEC_BGR555    HEIMDALL_CODEC_BGR555LE
#endif


/* 8-bit RGB "R3G3B2" */
#define HEIMDALL_CODEC_RGB332           HEIMDALL_FOURCC('R','3','3','2')
/* 8-bit RGB "R2G3B3" */
#define HEIMDALL_CODEC_RGB233           HEIMDALL_FOURCC('R','G','B','8')
/* 8-bit BGR "B2G3R3" */
#define HEIMDALL_CODEC_BGR233           HEIMDALL_FOURCC('B','2','3','3')

/* 32-bit RGBA, in memory address order: "RGBA" */
#define HEIMDALL_CODEC_RGBA            HEIMDALL_FOURCC('R','G','B','A')
/* 32-bit ARGB, in memory address order: "ARGB" */
#define HEIMDALL_CODEC_ARGB            HEIMDALL_FOURCC('A','R','G','B')
/* 32-bit ARGB, in memory address order: "BGRA" */
#define HEIMDALL_CODEC_BGRA            HEIMDALL_FOURCC('B','G','R','A')
/* 32-bit RGBA, in memory address order: "ABGR" */
#define HEIMDALL_CODEC_ABGR            HEIMDALL_FOURCC('A','B','G','R')
/* 32 bits Little-Endian RGBA with 10-bit colors "A2B6 b4G4 g6R2 r8" */
#define HEIMDALL_CODEC_RGBA10LE        HEIMDALL_FOURCC('R','G','A','0')
/* 64 bits RGBA */
#define HEIMDALL_CODEC_RGBA64          HEIMDALL_FOURCC('R','G','A','4')

/* Planar GBR 4:4:4 8 bits */
#define HEIMDALL_CODEC_GBR_PLANAR      HEIMDALL_FOURCC('G','B','R','8')
#define HEIMDALL_CODEC_GBR_PLANAR_9B   HEIMDALL_FOURCC('G','B','9','B')
#define HEIMDALL_CODEC_GBR_PLANAR_9L   HEIMDALL_FOURCC('G','B','9','L')
#define HEIMDALL_CODEC_GBR_PLANAR_10B  HEIMDALL_FOURCC('G','B','A','B')
#define HEIMDALL_CODEC_GBR_PLANAR_10L  HEIMDALL_FOURCC('G','B','A','L')
#define HEIMDALL_CODEC_GBR_PLANAR_12B  HEIMDALL_FOURCC('G','B','B','B')
#define HEIMDALL_CODEC_GBR_PLANAR_12L  HEIMDALL_FOURCC('G','B','B','L')
#define HEIMDALL_CODEC_GBR_PLANAR_14B  HEIMDALL_FOURCC('G','B','D','B')
#define HEIMDALL_CODEC_GBR_PLANAR_14L  HEIMDALL_FOURCC('G','B','D','L')
#define HEIMDALL_CODEC_GBR_PLANAR_16L  HEIMDALL_FOURCC('G','B','F','L')
#define HEIMDALL_CODEC_GBR_PLANAR_16B  HEIMDALL_FOURCC('G','B','F','B')
#define HEIMDALL_CODEC_GBRA_PLANAR     HEIMDALL_FOURCC('G','B','0','8')
#define HEIMDALL_CODEC_GBRA_PLANAR_10B HEIMDALL_FOURCC('G','B','0','B')
#define HEIMDALL_CODEC_GBRA_PLANAR_10L HEIMDALL_FOURCC('G','B','0','L')
#define HEIMDALL_CODEC_GBRA_PLANAR_12B HEIMDALL_FOURCC('G','B','C','B')
#define HEIMDALL_CODEC_GBRA_PLANAR_12L HEIMDALL_FOURCC('G','B','C','L')
#define HEIMDALL_CODEC_GBRA_PLANAR_16L HEIMDALL_FOURCC('G','B','E','L')
#define HEIMDALL_CODEC_GBRA_PLANAR_16B HEIMDALL_FOURCC('G','B','E','B')

/* 8 bits grey */
#define HEIMDALL_CODEC_GREY            HEIMDALL_FOURCC('G','R','E','Y')
/* 10 bits grey */
#define HEIMDALL_CODEC_GREY_10L        HEIMDALL_FOURCC('G','0','F','L')
#define HEIMDALL_CODEC_GREY_10B        HEIMDALL_FOURCC('G','0','F','B')
/* 12 bits grey */
#define HEIMDALL_CODEC_GREY_12L        HEIMDALL_FOURCC('G','2','F','L')
#define HEIMDALL_CODEC_GREY_12B        HEIMDALL_FOURCC('G','2','F','B')
/* 16 bits grey */
#define HEIMDALL_CODEC_GREY_16L        HEIMDALL_FOURCC('G','R','F','L')
#define HEIMDALL_CODEC_GREY_16B        HEIMDALL_FOURCC('G','R','F','B')

/* VDPAU video surface YCbCr */
#define HEIMDALL_CODEC_VDPAU_VIDEO     HEIMDALL_FOURCC('V','D','V','Y')
/* VDPAU output surface RGBA */
#define HEIMDALL_CODEC_VDPAU_OUTPUT    HEIMDALL_FOURCC('V','D','O','R')

/* VAAPI opaque surface */
#define HEIMDALL_CODEC_VAAPI_420 HEIMDALL_FOURCC('V','A','O','P') /* 4:2:0  8 bpc */
#define HEIMDALL_CODEC_VAAPI_420_10BPP HEIMDALL_FOURCC('V','A','O','0') /* 4:2:0 10 bpc */
#define HEIMDALL_CODEC_VAAPI_420_12BPP HEIMDALL_FOURCC('V','A','O','2') /* 4:2:0 12 bpc */
#define HEIMDALL_CODEC_VAAPI_422_10BPP HEIMDALL_FOURCC('V','A','2','0') /* 4:2:2 10 bpc */
#define HEIMDALL_CODEC_VAAPI_422_12BPP HEIMDALL_FOURCC('V','A','2','2') /* 4:2:2 12 bpc */
#define HEIMDALL_CODEC_VAAPI_444       HEIMDALL_FOURCC('V','A','4','P') /* 4:4:4 8 bpc */
#define HEIMDALL_CODEC_VAAPI_444_10BPP HEIMDALL_FOURCC('V','A','4','0') /* 4:4:4 10 bpc */
#define HEIMDALL_CODEC_VAAPI_444_12BPP HEIMDALL_FOURCC('V','A','4','2') /* 4:4:4 12 bpc */

/* MediaCodec/IOMX opaque buffer type */
#define HEIMDALL_CODEC_ANDROID_OPAQUE  HEIMDALL_FOURCC('A','N','O','P')

/* Broadcom MMAL opaque buffer type */
#define HEIMDALL_CODEC_MMAL_OPAQUE     HEIMDALL_FOURCC('M','M','A','L')

/* DXVA2 opaque video surface for use with D3D9 */
#define HEIMDALL_CODEC_D3D9_OPAQUE     HEIMDALL_FOURCC('D','X','A','9') /* 4:2:0  8 bpc */
#define HEIMDALL_CODEC_D3D9_OPAQUE_10B HEIMDALL_FOURCC('D','X','A','0') /* 4:2:0 10 bpc */

/* D3D11VA opaque video surface for use with D3D11 */
#define HEIMDALL_CODEC_D3D11_OPAQUE          HEIMDALL_FOURCC('D','X','1','1') /* 4:2:0  8 bpc */
#define HEIMDALL_CODEC_D3D11_OPAQUE_10B      HEIMDALL_FOURCC('D','X','1','0') /* 4:2:0 10 bpc */
#define HEIMDALL_CODEC_D3D11_OPAQUE_RGBA     HEIMDALL_FOURCC('D','X','R','G')
#define HEIMDALL_CODEC_D3D11_OPAQUE_BGRA     HEIMDALL_FOURCC('D','A','G','R')
#define HEIMDALL_CODEC_D3D11_OPAQUE_ALPHA    HEIMDALL_FOURCC('D','A','1','1') /* 4:2:0  8 bpc with extra alpha plane */

/* NVDEC opaque video format for use the NVDec API */
#define HEIMDALL_CODEC_NVDEC_OPAQUE          HEIMDALL_FOURCC('N','V','D','8') /* 4:2:0  8 bpc */
#define HEIMDALL_CODEC_NVDEC_OPAQUE_10B      HEIMDALL_FOURCC('N','V','D','0') /* 4:2:0 10 bpc */
#define HEIMDALL_CODEC_NVDEC_OPAQUE_16B      HEIMDALL_FOURCC('N','V','D','6') /* 4:2:0 16 bpc */
#define HEIMDALL_CODEC_NVDEC_OPAQUE_444      HEIMDALL_FOURCC('N','V','4','8') /* 4:4:4  8 bpc */
#define HEIMDALL_CODEC_NVDEC_OPAQUE_444_16B  HEIMDALL_FOURCC('N','V','4','6') /* 4:4:4 16 bpc */

/* CVPixelBuffer opaque buffer type */
#define HEIMDALL_CODEC_CVPX_NV12       HEIMDALL_FOURCC('C','V','P','N')
#define HEIMDALL_CODEC_CVPX_UYVY       HEIMDALL_FOURCC('C','V','P','Y')
#define HEIMDALL_CODEC_CVPX_I420       HEIMDALL_FOURCC('C','V','P','I')
#define HEIMDALL_CODEC_CVPX_BGRA       HEIMDALL_FOURCC('C','V','P','B')
#define HEIMDALL_CODEC_CVPX_P010       HEIMDALL_FOURCC('C','V','P','P')

/* GStreamer Memory opaque buffer type */
#define HEIMDALL_CODEC_GST_MEM_OPAQUE  HEIMDALL_FOURCC('G','S','T','M')

/* Image codec (video) */
#define HEIMDALL_CODEC_PNG             HEIMDALL_FOURCC('p','n','g',' ')
#define HEIMDALL_CODEC_PPM             HEIMDALL_FOURCC('p','p','m',' ')
#define HEIMDALL_CODEC_PGM             HEIMDALL_FOURCC('p','g','m',' ')
#define HEIMDALL_CODEC_PGMYUV          HEIMDALL_FOURCC('p','g','m','y')
#define HEIMDALL_CODEC_PAM             HEIMDALL_FOURCC('p','a','m',' ')
#define HEIMDALL_CODEC_JPEG            HEIMDALL_FOURCC('j','p','e','g')
#define HEIMDALL_CODEC_BPG             HEIMDALL_FOURCC('B','P','G',0xFB)
#define HEIMDALL_CODEC_JPEGLS          HEIMDALL_FOURCC('M','J','L','S')
#define HEIMDALL_CODEC_BMP             HEIMDALL_FOURCC('b','m','p',' ')
#define HEIMDALL_CODEC_FARBFELD        HEIMDALL_FOURCC('f','a','r','b')
#define HEIMDALL_CODEC_TIFF            HEIMDALL_FOURCC('t','i','f','f')
#define HEIMDALL_CODEC_GIF             HEIMDALL_FOURCC('g','i','f',' ')
#define HEIMDALL_CODEC_TARGA           HEIMDALL_FOURCC('t','g','a',' ')
#define HEIMDALL_CODEC_SVG             HEIMDALL_FOURCC('s','v','g',' ')
#define HEIMDALL_CODEC_SGI             HEIMDALL_FOURCC('s','g','i',' ')
#define HEIMDALL_CODEC_PNM             HEIMDALL_FOURCC('p','n','m',' ')
#define HEIMDALL_CODEC_PCX             HEIMDALL_FOURCC('p','c','x',' ')
#define HEIMDALL_CODEC_XWD             HEIMDALL_FOURCC('X','W','D',' ')
#define HEIMDALL_CODEC_TXD             HEIMDALL_FOURCC('T','X','D',' ')
#define HEIMDALL_CODEC_WEBP            HEIMDALL_FOURCC('W','E','B','P')


/* Audio codec */
#define HEIMDALL_CODEC_MPGA                       HEIMDALL_FOURCC('m','p','g','a')
#define HEIMDALL_CODEC_MP4A                       HEIMDALL_FOURCC('m','p','4','a')
#define HEIMDALL_CODEC_MPEGH                      HEIMDALL_FOURCC('M','H','A','S')
#define HEIMDALL_CODEC_ALS                        HEIMDALL_FOURCC('a','l','s',' ')
#define HEIMDALL_CODEC_A52                        HEIMDALL_FOURCC('a','5','2',' ')
#define HEIMDALL_CODEC_EAC3                       HEIMDALL_FOURCC('e','a','c','3')
#define HEIMDALL_CODEC_AC4                        HEIMDALL_FOURCC('a','c','-','4')
#define HEIMDALL_CODEC_DTS                        HEIMDALL_FOURCC('d','t','s',' ')
/* Only used by outputs and filters */
#define HEIMDALL_CODEC_DTSHD                      HEIMDALL_FOURCC('d','t','s','h')
#define HEIMDALL_CODEC_WMA1                       HEIMDALL_FOURCC('W','M','A','1')
#define HEIMDALL_CODEC_WMA2                       HEIMDALL_FOURCC('W','M','A','2')
#define HEIMDALL_CODEC_WMAP                       HEIMDALL_FOURCC('W','M','A','P')
#define HEIMDALL_CODEC_WMAL                       HEIMDALL_FOURCC('W','M','A','L')
#define HEIMDALL_CODEC_WMAS                       HEIMDALL_FOURCC('W','M','A','S')
#define HEIMDALL_CODEC_FLAC                       HEIMDALL_FOURCC('f','l','a','c')
#define HEIMDALL_CODEC_MLP                        HEIMDALL_FOURCC('m','l','p',' ')
#define HEIMDALL_CODEC_TRUEHD                     HEIMDALL_FOURCC('m','l','p','a')
#define HEIMDALL_CODEC_DVAUDIO                    HEIMDALL_FOURCC('d','v','a','u')
#define HEIMDALL_CODEC_SPEEX                      HEIMDALL_FOURCC('s','p','x',' ')
#define HEIMDALL_CODEC_CODEC2                     HEIMDALL_FOURCC('C','0','D','E')
#define HEIMDALL_CODEC_OPUS                       HEIMDALL_FOURCC('O','p','u','s')
#define HEIMDALL_CODEC_VORBIS                     HEIMDALL_FOURCC('v','o','r','b')
#define HEIMDALL_CODEC_MACE3                      HEIMDALL_FOURCC('M','A','C','3')
#define HEIMDALL_CODEC_MACE6                      HEIMDALL_FOURCC('M','A','C','6')
#define HEIMDALL_CODEC_MUSEPACK7                  HEIMDALL_FOURCC('M','P','C',' ')
#define HEIMDALL_CODEC_MUSEPACK8                  HEIMDALL_FOURCC('M','P','C','K')
#define HEIMDALL_CODEC_RA_144                     HEIMDALL_FOURCC('1','4','_','4')
#define HEIMDALL_CODEC_RA_288                     HEIMDALL_FOURCC('2','8','_','8')
#define HEIMDALL_CODEC_INTERPLAY_DPCM             HEIMDALL_FOURCC('i','d','p','c')
#define HEIMDALL_CODEC_ROQ_DPCM                   HEIMDALL_FOURCC('R','o','Q','a')
#define HEIMDALL_CODEC_DSICINAUDIO                HEIMDALL_FOURCC('D','C','I','A')
#define HEIMDALL_CODEC_ADPCM_4XM                  HEIMDALL_FOURCC('4','x','m','a')
#define HEIMDALL_CODEC_ADPCM_EA                   HEIMDALL_FOURCC('A','D','E','A')
#define HEIMDALL_CODEC_ADPCM_XA                   HEIMDALL_FOURCC('x','a',' ',' ')
#define HEIMDALL_CODEC_ADPCM_ADX                  HEIMDALL_FOURCC('a','d','x',' ')
#define HEIMDALL_CODEC_ADPCM_IMA_WS               HEIMDALL_FOURCC('A','I','W','S')
#define HEIMDALL_CODEC_ADPCM_G722                 HEIMDALL_FOURCC('g','7','2','2')
#define HEIMDALL_CODEC_ADPCM_G726                 HEIMDALL_FOURCC('g','7','2','6')
#define HEIMDALL_CODEC_ADPCM_G726_LE              HEIMDALL_FOURCC('g','7','2','s')
#define HEIMDALL_CODEC_ADPCM_SWF                  HEIMDALL_FOURCC('S','W','F','a')
#define HEIMDALL_CODEC_ADPCM_MS                   HEIMDALL_FOURCC('m','s',0x00,0x02)
#define HEIMDALL_CODEC_ADPCM_IMA_WAV              HEIMDALL_FOURCC('m','s',0x00,0x11)
#define HEIMDALL_CODEC_ADPCM_IMA_AMV              HEIMDALL_FOURCC('i','m','a','v')
#define HEIMDALL_CODEC_ADPCM_IMA_QT               HEIMDALL_FOURCC('i','m','a','4')
#define HEIMDALL_CODEC_ADPCM_YAMAHA               HEIMDALL_FOURCC('m','s',0x00,0x20)
#define HEIMDALL_CODEC_ADPCM_DK3                  HEIMDALL_FOURCC('m','s',0x00,0x62)
#define HEIMDALL_CODEC_ADPCM_DK4                  HEIMDALL_FOURCC('m','s',0x00,0x61)
#define HEIMDALL_CODEC_ADPCM_CREATIVE             HEIMDALL_FOURCC('m','s',0x00,0xC0)
#define HEIMDALL_CODEC_ADPCM_SBPRO_2              HEIMDALL_FOURCC('m','s',0x00,0xC2)
#define HEIMDALL_CODEC_ADPCM_SBPRO_3              HEIMDALL_FOURCC('m','s',0x00,0xC3)
#define HEIMDALL_CODEC_ADPCM_SBPRO_4              HEIMDALL_FOURCC('m','s',0x00,0xC4)
#define HEIMDALL_CODEC_ADPCM_THP                  HEIMDALL_FOURCC('T','H','P','A')
#define HEIMDALL_CODEC_ADPCM_XA_EA                HEIMDALL_FOURCC('X','A','J', 0)
#define HEIMDALL_CODEC_G723_1                     HEIMDALL_FOURCC('g','7','2', 0x31)
#define HEIMDALL_CODEC_G729                       HEIMDALL_FOURCC('g','7','2','9')
#define HEIMDALL_CODEC_VMDAUDIO                   HEIMDALL_FOURCC('v','m','d','a')
#define HEIMDALL_CODEC_AMR_NB                     HEIMDALL_FOURCC('s','a','m','r')
#define HEIMDALL_CODEC_AMR_WB                     HEIMDALL_FOURCC('s','a','w','b')
#define HEIMDALL_CODEC_ALAC                       HEIMDALL_FOURCC('a','l','a','c')
#define HEIMDALL_CODEC_QDM2                       HEIMDALL_FOURCC('Q','D','M','2')
#define HEIMDALL_CODEC_QDMC                       HEIMDALL_FOURCC('Q','D','M','C')
#define HEIMDALL_CODEC_QOA                        HEIMDALL_FOURCC('q','o','a','f')
#define HEIMDALL_CODEC_COOK                       HEIMDALL_FOURCC('c','o','o','k')
#define HEIMDALL_CODEC_SIPR                       HEIMDALL_FOURCC('s','i','p','r')
#define HEIMDALL_CODEC_TTA                        HEIMDALL_FOURCC('T','T','A','1')
#define HEIMDALL_CODEC_SHORTEN                    HEIMDALL_FOURCC('s','h','n',' ')
#define HEIMDALL_CODEC_WAVPACK                    HEIMDALL_FOURCC('W','V','P','K')
#define HEIMDALL_CODEC_GSM                        HEIMDALL_FOURCC('g','s','m',' ')
#define HEIMDALL_CODEC_GSM_MS                     HEIMDALL_FOURCC('a','g','s','m')
#define HEIMDALL_CODEC_ATRAC1                     HEIMDALL_FOURCC('a','t','r','1')
#define HEIMDALL_CODEC_ATRAC3                     HEIMDALL_FOURCC('a','t','r','c')
#define HEIMDALL_CODEC_ATRAC3P                    HEIMDALL_FOURCC('a','t','r','p')
#define HEIMDALL_CODEC_ATRAC3AL                   HEIMDALL_FOURCC('a','t','r','l')
#define HEIMDALL_CODEC_ATRAC3PAL                  HEIMDALL_FOURCC('a','t','p','l')
#define HEIMDALL_CODEC_ATRAC9                     HEIMDALL_FOURCC('a','t','r','9')
#define HEIMDALL_CODEC_IMC                        HEIMDALL_FOURCC(0x1,0x4,0x0,0x0)
#define HEIMDALL_CODEC_TRUESPEECH                 HEIMDALL_FOURCC(0x22,0x0,0x0,0x0)
#define HEIMDALL_CODEC_NELLYMOSER                 HEIMDALL_FOURCC('N','E','L','L')
#define HEIMDALL_CODEC_APE                        HEIMDALL_FOURCC('A','P','E',' ')
#define HEIMDALL_CODEC_QCELP                      HEIMDALL_FOURCC('Q','c','l','p')
#define HEIMDALL_CODEC_302M                       HEIMDALL_FOURCC('3','0','2','m')
#define HEIMDALL_CODEC_DVD_LPCM                   HEIMDALL_FOURCC('l','p','c','m')
#define HEIMDALL_CODEC_DVDA_LPCM                  HEIMDALL_FOURCC('a','p','c','m')
#define HEIMDALL_CODEC_BD_LPCM                    HEIMDALL_FOURCC('b','p','c','m')
#define HEIMDALL_CODEC_WIDI_LPCM                  HEIMDALL_FOURCC('w','p','c','m')
#define HEIMDALL_CODEC_SDDS                       HEIMDALL_FOURCC('s','d','d','s')
#define HEIMDALL_CODEC_MIDI                       HEIMDALL_FOURCC('M','I','D','I')
#define HEIMDALL_CODEC_RALF                       HEIMDALL_FOURCC('R','A','L','F')

#define HEIMDALL_CODEC_S8                         HEIMDALL_FOURCC('s','8',' ',' ')
#define HEIMDALL_CODEC_U8                         HEIMDALL_FOURCC('u','8',' ',' ')
#define HEIMDALL_CODEC_S16L                       HEIMDALL_FOURCC('s','1','6','l')
#define HEIMDALL_CODEC_S16L_PLANAR                HEIMDALL_FOURCC('s','1','l','p')
#define HEIMDALL_CODEC_S16B                       HEIMDALL_FOURCC('s','1','6','b')
#define HEIMDALL_CODEC_U16L                       HEIMDALL_FOURCC('u','1','6','l')
#define HEIMDALL_CODEC_U16B                       HEIMDALL_FOURCC('u','1','6','b')
#define HEIMDALL_CODEC_S20B                       HEIMDALL_FOURCC('s','2','0','b')
#define HEIMDALL_CODEC_S24L                       HEIMDALL_FOURCC('s','2','4','l')
#define HEIMDALL_CODEC_S24B                       HEIMDALL_FOURCC('s','2','4','b')
#define HEIMDALL_CODEC_U24L                       HEIMDALL_FOURCC('u','2','4','l')
#define HEIMDALL_CODEC_U24B                       HEIMDALL_FOURCC('u','2','4','b')
#define HEIMDALL_CODEC_S24L32                     HEIMDALL_FOURCC('s','2','4','4')
#define HEIMDALL_CODEC_S24B32                     HEIMDALL_FOURCC('S','2','4','4')
#define HEIMDALL_CODEC_S32L                       HEIMDALL_FOURCC('s','3','2','l')
#define HEIMDALL_CODEC_S32B                       HEIMDALL_FOURCC('s','3','2','b')
#define HEIMDALL_CODEC_U32L                       HEIMDALL_FOURCC('u','3','2','l')
#define HEIMDALL_CODEC_U32B                       HEIMDALL_FOURCC('u','3','2','b')
#define HEIMDALL_CODEC_F32L                       HEIMDALL_FOURCC('f','3','2','l')
#define HEIMDALL_CODEC_F32B                       HEIMDALL_FOURCC('f','3','2','b')
#define HEIMDALL_CODEC_F64L                       HEIMDALL_FOURCC('f','6','4','l')
#define HEIMDALL_CODEC_F64B                       HEIMDALL_FOURCC('f','6','4','b')

#define HEIMDALL_CODEC_ALAW                       HEIMDALL_FOURCC('a','l','a','w')
#define HEIMDALL_CODEC_MULAW                      HEIMDALL_FOURCC('m','l','a','w')
#define HEIMDALL_CODEC_DAT12                      HEIMDALL_FOURCC('L','P','1','2')
#define HEIMDALL_CODEC_S24DAUD                    HEIMDALL_FOURCC('d','a','u','d')
#define HEIMDALL_CODEC_TWINVQ                     HEIMDALL_FOURCC('T','W','I','N')
#define HEIMDALL_CODEC_BMVAUDIO                   HEIMDALL_FOURCC('B','M','V','A')
#define HEIMDALL_CODEC_ULEAD_DV_AUDIO_NTSC        HEIMDALL_FOURCC('m','s',0x02,0x15)
#define HEIMDALL_CODEC_ULEAD_DV_AUDIO_PAL         HEIMDALL_FOURCC('m','s',0x02,0x16)
#define HEIMDALL_CODEC_INDEO_AUDIO                HEIMDALL_FOURCC('m','s',0x04,0x02)
#define HEIMDALL_CODEC_METASOUND                  HEIMDALL_FOURCC('m','s',0x00,0x75)
#define HEIMDALL_CODEC_ON2AVC                     HEIMDALL_FOURCC('m','s',0x05,0x00)
#define HEIMDALL_CODEC_TAK                        HEIMDALL_FOURCC('t','a','k',' ')
#define HEIMDALL_CODEC_SMACKAUDIO                 HEIMDALL_FOURCC('S','M','K','A')
#define HEIMDALL_CODEC_ADPCM_IMA_EA_SEAD          HEIMDALL_FOURCC('S','E','A','D')
#define HEIMDALL_CODEC_ADPCM_EA_R1                HEIMDALL_FOURCC('E','A','R','1')
#define HEIMDALL_CODEC_ADPCM_EA_R3                HEIMDALL_FOURCC('E','A','R','3')
#define HEIMDALL_CODEC_ADPCM_IMA_APC              HEIMDALL_FOURCC('A','I','P','C')
#define HEIMDALL_CODEC_DSD_LSBF                   HEIMDALL_FOURCC('D','S','D','l')
#define HEIMDALL_CODEC_DSD_LSBF_PLANAR            HEIMDALL_FOURCC('D','S','F','l')
#define HEIMDALL_CODEC_DSD_MSBF                   HEIMDALL_FOURCC('D','S','D',' ')
#define HEIMDALL_CODEC_DSD_MSBF_PLANAR            HEIMDALL_FOURCC('D','S','F','m')
#define HEIMDALL_CODEC_DFPWM                      HEIMDALL_FOURCC('D','F','P','W')

/* Subtitle */
#define HEIMDALL_CODEC_SPU       HEIMDALL_FOURCC('s','p','u',' ')
#define HEIMDALL_CODEC_DVBS      HEIMDALL_FOURCC('d','v','b','s')
#define HEIMDALL_CODEC_SUBT      HEIMDALL_FOURCC('s','u','b','t')
#define HEIMDALL_CODEC_XSUB      HEIMDALL_FOURCC('X','S','U','B')
#define HEIMDALL_CODEC_SSA       HEIMDALL_FOURCC('s','s','a',' ')
#define HEIMDALL_CODEC_TEXT      HEIMDALL_FOURCC('T','E','X','T')
#define HEIMDALL_CODEC_TELETEXT  HEIMDALL_FOURCC('t','e','l','x')
#define HEIMDALL_CODEC_KATE      HEIMDALL_FOURCC('k','a','t','e')
#define HEIMDALL_CODEC_CMML      HEIMDALL_FOURCC('c','m','m','l')
#define HEIMDALL_CODEC_ITU_T140  HEIMDALL_FOURCC('t','1','4','0')
#define HEIMDALL_CODEC_USF       HEIMDALL_FOURCC('u','s','f',' ')
#define HEIMDALL_CODEC_OGT       HEIMDALL_FOURCC('o','g','t',' ')
#define HEIMDALL_CODEC_CVD       HEIMDALL_FOURCC('c','v','d',' ')
#define HEIMDALL_CODEC_QTXT      HEIMDALL_FOURCC('q','t','x','t')
#define HEIMDALL_CODEC_TX3G      HEIMDALL_FOURCC('t','x','3','g')
#define HEIMDALL_CODEC_ARIB_A    HEIMDALL_FOURCC('a','r','b','a')
#define HEIMDALL_CODEC_ARIB_C    HEIMDALL_FOURCC('a','r','b','c')
/* Blu-ray Presentation Graphics */
#define HEIMDALL_CODEC_BD_PG     HEIMDALL_FOURCC('b','d','p','g')
#define HEIMDALL_CODEC_BD_TEXT   HEIMDALL_FOURCC('b','d','t','x')
/* EBU STL (TECH. 3264-E) */
#define HEIMDALL_CODEC_EBU_STL   HEIMDALL_FOURCC('S','T','L',' ')
#define HEIMDALL_CODEC_SCTE_18   HEIMDALL_FOURCC('S','C','1','8')
#define HEIMDALL_CODEC_SCTE_27   HEIMDALL_FOURCC('S','C','2','7')
/* EIA/CEA-608/708 */
#define HEIMDALL_CODEC_CEA608    HEIMDALL_FOURCC('c','6','0','8')
#define HEIMDALL_CODEC_CEA708    HEIMDALL_FOURCC('c','7','0','8')
#define HEIMDALL_CODEC_TTML      HEIMDALL_FOURCC('s','t','p','p')
#define HEIMDALL_CODEC_TTML_TS   HEIMDALL_FOURCC('s','t','p','P') /* special for EN.303.560 */
#define HEIMDALL_CODEC_WEBVTT    HEIMDALL_FOURCC('w','v','t','t')

/* XYZ colorspace 12 bits packed in 16 bits, organisation |XXX0|YYY0|ZZZ0| */
#define HEIMDALL_CODEC_XYZ_12L   HEIMDALL_FOURCC('X','Y','c','L')
#define HEIMDALL_CODEC_XYZ_12B   HEIMDALL_FOURCC('X','Y','c','B')


/* Special endian dependent values
 * The suffic N means Native
 * The suffix I means Inverted (ie non native) */
#ifdef WORDS_BIGENDIAN
#   define HEIMDALL_CODEC_S16N HEIMDALL_CODEC_S16B
#   define HEIMDALL_CODEC_U16N HEIMDALL_CODEC_U16B
#   define HEIMDALL_CODEC_S24N HEIMDALL_CODEC_S24B
#   define HEIMDALL_CODEC_U24N HEIMDALL_CODEC_U24B
#   define HEIMDALL_CODEC_S32N HEIMDALL_CODEC_S32B
#   define HEIMDALL_CODEC_U32N HEIMDALL_CODEC_U32B
#   define HEIMDALL_CODEC_FL32 HEIMDALL_CODEC_F32B
#   define HEIMDALL_CODEC_FL64 HEIMDALL_CODEC_F64B

#   define HEIMDALL_CODEC_S16I HEIMDALL_CODEC_S16L
#   define HEIMDALL_CODEC_U16I HEIMDALL_CODEC_U16L
#   define HEIMDALL_CODEC_S24I HEIMDALL_CODEC_S24L
#   define HEIMDALL_CODEC_U24I HEIMDALL_CODEC_U24L
#   define HEIMDALL_CODEC_S32I HEIMDALL_CODEC_S32L
#   define HEIMDALL_CODEC_U32I HEIMDALL_CODEC_U32L

#else
#   define HEIMDALL_CODEC_S16N HEIMDALL_CODEC_S16L
#   define HEIMDALL_CODEC_U16N HEIMDALL_CODEC_U16L
#   define HEIMDALL_CODEC_S24N HEIMDALL_CODEC_S24L
#   define HEIMDALL_CODEC_U24N HEIMDALL_CODEC_U24L
#   define HEIMDALL_CODEC_S32N HEIMDALL_CODEC_S32L
#   define HEIMDALL_CODEC_U32N HEIMDALL_CODEC_U32L
#   define HEIMDALL_CODEC_FL32 HEIMDALL_CODEC_F32L
#   define HEIMDALL_CODEC_FL64 HEIMDALL_CODEC_F64L

#   define HEIMDALL_CODEC_S16I HEIMDALL_CODEC_S16B
#   define HEIMDALL_CODEC_U16I HEIMDALL_CODEC_U16B
#   define HEIMDALL_CODEC_S24I HEIMDALL_CODEC_S24B
#   define HEIMDALL_CODEC_U24I HEIMDALL_CODEC_U24B
#   define HEIMDALL_CODEC_S32I HEIMDALL_CODEC_S32B
#   define HEIMDALL_CODEC_U32I HEIMDALL_CODEC_U32B
#endif

/* Non official codecs, used to force a profile in an encoder */
/* MPEG-1 video */
#define HEIMDALL_CODEC_MP1V      HEIMDALL_FOURCC('m','p','1','v')
/* MPEG-2 video */
#define HEIMDALL_CODEC_MP2V      HEIMDALL_FOURCC('m','p','2','v')
/* MPEG-I/II layer 2 audio */
#define HEIMDALL_CODEC_MP2       HEIMDALL_FOURCC('m','p','2',' ')
/* MPEG-I/II layer 3 audio */
#define HEIMDALL_CODEC_MP3       HEIMDALL_FOURCC('m','p','3',' ')

/**
 * It returns the codec associated to a fourcc within an ES category.
 *
 * If not found, it will return the given fourcc.
 * If found, it will always be one of the HEIMDALL_CODEC_ defined above.
 *
 * You may use UNKNOWN_ES for the ES category if you don't have the information.
 */
HEIMDALL_API heimdall_fourcc_t heimdall_fourcc_GetCodec( int i_cat, heimdall_fourcc_t i_fourcc );

/**
 * It returns the codec associated to a fourcc stored in a zero terminated
 * string.
 *
 * If the string is NULL or has more than 4 characters or doesn't correspond
 * to a string associated with a HEIMDALL_CODEC_, it will return 0, otherwise it
 * will one of the HEIMDALL_CODEC_ defined above.
 *
 * You may use UNKNOWN_ES for the ES category if you don't have the information.
 *
 * Provided for convenience.
 */
HEIMDALL_API heimdall_fourcc_t heimdall_fourcc_GetCodecFromString( int i_cat, const char * );

/**
 * It converts the given fourcc to an audio codec when possible.
 *
 * The fourccs converted are aflt, araw/pcm , twos, sowt. When an incompatible i_bits
 * is detected, 0 is returned.
 * The other fourccs go through heimdall_fourcc_GetCodec and i_bits is not checked.
 */
HEIMDALL_API heimdall_fourcc_t heimdall_fourcc_GetCodecAudio( heimdall_fourcc_t i_fourcc, int i_bits );

/**
 * It returns the description of the given fourcc or an empty string.
 *
 * You may use UNKNOWN_ES for the ES category if you don't have the information.
 */
HEIMDALL_API const char * heimdall_fourcc_GetDescription( int i_cat, heimdall_fourcc_t i_fourcc );

/**
 * It returns a list (terminated with the value 0) of YUV fourccs in
 * decreasing priority order for the given chroma.
 *
 * It can return a NULL pointer, it must be freed.
 */
HEIMDALL_API heimdall_fourcc_t * heimdall_fourcc_GetYUVFallback( heimdall_fourcc_t );

/**
 * It returns a list (terminated with the value 0) of RGB fourccs in
 * decreasing priority order for the given chroma.
 *
 * It can return a NULL pointer, it must be freed.
 */
HEIMDALL_API heimdall_fourcc_t * heimdall_fourcc_GetRGBFallback( heimdall_fourcc_t );

/**
 * It returns a list (terminated with the value 0) of fourccs in decreasing
 * priority order for the given chroma. It will return either YUV or RGB
 * fallbacks depending on whether or not the fourcc given is YUV.
 *
 * It can return a NULL pointer, it must be freed.
 */
HEIMDALL_API heimdall_fourcc_t * heimdall_fourcc_GetFallback( heimdall_fourcc_t );

/**
 * Chroma subtype
 */
enum heimdall_chroma_subtype
{
    HEIMDALL_CHROMA_SUBTYPE_OTHER,
    HEIMDALL_CHROMA_SUBTYPE_YUV444,
    HEIMDALL_CHROMA_SUBTYPE_YUV440,
    HEIMDALL_CHROMA_SUBTYPE_YUV422,
    HEIMDALL_CHROMA_SUBTYPE_YUV420,
    HEIMDALL_CHROMA_SUBTYPE_YUV411,
    HEIMDALL_CHROMA_SUBTYPE_YUV410,
    HEIMDALL_CHROMA_SUBTYPE_YUV211,
    HEIMDALL_CHROMA_SUBTYPE_RGB,
    HEIMDALL_CHROMA_SUBTYPE_GREY,
};

/**
 * Chroma related information.
 */
typedef struct {
    heimdall_fourcc_t fcc;
    enum heimdall_chroma_subtype subtype;
    unsigned plane_count;
    struct {
        heimdall_rational_t w;
        heimdall_rational_t h;
    } p[4];
    unsigned pixel_size;        /* Number of bytes per pixel for a plane */
    unsigned pixel_bits;        /* Number of bits actually used bits per pixel for a plane */
    float color_bits;           /* Average number of bits used by one color */
} heimdall_chroma_description_t;

/**
 * It returns a heimdall_chroma_description_t describing the requested fourcc or NULL
 * if not found.
 */
HEIMDALL_API const heimdall_chroma_description_t * heimdall_fourcc_GetChromaDescription( heimdall_fourcc_t fourcc ) HEIMDALL_USED;

/**
 * Returns true if the chroma description is YUV
 */
static inline bool
heimdall_chroma_description_IsYUV(const heimdall_chroma_description_t *desc)
{
    switch (desc->subtype)
    {
        case HEIMDALL_CHROMA_SUBTYPE_YUV444:
        case HEIMDALL_CHROMA_SUBTYPE_YUV440:
        case HEIMDALL_CHROMA_SUBTYPE_YUV422:
        case HEIMDALL_CHROMA_SUBTYPE_YUV420:
        case HEIMDALL_CHROMA_SUBTYPE_YUV410:
        case HEIMDALL_CHROMA_SUBTYPE_YUV411:
        case HEIMDALL_CHROMA_SUBTYPE_YUV211:
            return true;
        default:
            return false;
    }
}

/**
 * It returns true if the given fourcc is YUV and false otherwise.
 */
static inline bool heimdall_fourcc_IsYUV(heimdall_fourcc_t fcc)
{
    const heimdall_chroma_description_t *desc = heimdall_fourcc_GetChromaDescription(fcc);
    return desc == NULL ? false : heimdall_chroma_description_IsYUV(desc);
}

/**
 * Get the average usable bits per pixel for a chroma.
 * \note it may return 0 for opaque or compressed heimdall_fourcc_t
 */
static inline unsigned heimdall_fourcc_GetChromaBPP( heimdall_fourcc_t fourcc )
{
    const heimdall_chroma_description_t *desc = heimdall_fourcc_GetChromaDescription( fourcc );
    if (desc == NULL || desc->plane_count == 0)
        return 0;

    unsigned bpp = 0;
    for (size_t plane=0; plane < desc->plane_count; plane++)
    {
        bpp += 2 * desc->pixel_bits * desc->p[plane].h.num * desc->p[plane].w.num /
                                        (desc->p[plane].h.den * desc->p[plane].w.den);
    }
    bpp /= 2; // for chromas that might have 4*4 denominators
    return bpp;
}

#endif /* _HEIMDALL_FOURCC_H */
