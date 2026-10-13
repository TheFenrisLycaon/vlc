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

static const staticentry_t p_list_video[] = {

    B(HEIMDALL_CODEC_MP1V, "MPEG-1 Video"),
        A("mp1v"),
        A("m1v "),
        A("mpg1"),
        A("BW10"),
        E("XMPG", "Xing MPEG-1 Intra"),

    B(HEIMDALL_CODEC_MPGV, "MPEG-1/2 Video"),
    B(HEIMDALL_CODEC_MP2V, "MPEG-2 Video"),
        A("mpeg"),
        A("mp2v"),
        A("MPEG"),
        A("mpg2"),
        A("MPG2"),
        A("H262"),

        E("PIM1", "Pinnacle DC1000 (MPEG-1 Video)"),

        E("hdv1", "HDV 720p30 (MPEG-2 Video)"),
        E("hdv2", "Sony HDV 1080i60 (MPEG-2 Video)"),
        E("hdv3", "FCP HDV 1080i50 (MPEG-2 Video)"),
        E("hdv4", "HDV 720p24 (MPEG-2 Video)"),
        E("hdv5", "HDV 720p25 (MPEG-2 Video)"),
        E("hdv6", "HDV 1080p24 (MPEG-2 Video)"),
        E("hdv7", "HDV 1080p25 (MPEG-2 Video)"),
        E("hdv8", "HDV 1080p30 (MPEG-2 Video)"),
        E("hdv9", "HDV 720p60 JVC (MPEG-2 Video)"),
        E("hdva", "HDV 720p50 (MPEG-2 Video)"),

        E("mx5n", "MPEG2 IMX NTSC 525/60 50Mb/s (FCP)"),
        E("mx5p", "MPEG2 IMX PAL 625/60 50Mb/s (FCP)"),
        E("mx4n", "MPEG2 IMX NTSC 525/60 40Mb/s (FCP)"),
        E("mx4p", "MPEG2 IMX PAL 625/50 40Mb/s (FCP)"),
        E("mx3n", "MPEG2 IMX NTSC 525/60 30Mb/s (FCP)"),
        E("mx3p", "MPEG2 IMX NTSC 625/50 30Mb/s (FCP)"),

        E("xdv1", "XDCAM HD 720p30 35Mb/s"),
        E("xdv2", "XDCAM HD 1080i60 35Mb/s"),
        E("xdv3", "XDCAM HD 1080i50 35Mb/s"),
        E("xdv4", "XDCAM HD 720p24 35Mb/s"),
        E("xdv5", "XDCAM HD 720p25 35Mb/s"),
        E("xdv6", "XDCAM HD 1080p24 35Mb/s"),
        E("xdv7", "XDCAM HD 1080p25 35Mb/s"),
        E("xdv8", "XDCAM HD 1080p30 35Mb/s"),
        E("xdv9", "XDCAM HD 720p60 35Mb/s"),
        E("xdva", "XDCAM HD 720p50 35Mb/s"),

        E("xdvb", "XDCAM EX 1080i60 50Mb/s CBR"),
        E("xdvc", "XDCAM EX 1080i50 50Mb/s CBR"),
        E("xdvd", "XDCAM EX 1080p24 50Mb/s CBR"),
        E("xdve", "XDCAM EX 1080p25 50Mb/s CBR"),
        E("xdvf", "XDCAM EX 1080p30 50Mb/s CBR"),

        E("xd51", "XDCAM HD422 720p30 50Mb/s CBR"),
        E("xd54", "XDCAM HD422 720p24 50Mb/s CBR"),
        E("xd55", "XDCAM HD422 720p25 50Mb/s CBR"),
        E("xd59", "XDCAM HD422 720p60 50Mb/s CBR"),
        E("xd5a", "XDCAM HD422 720p50 50Mb/s CBR"),
        E("xd5b", "XDCAM HD422 1080i60 50Mb/s CBR"),
        E("xd5c", "XDCAM HD422 1080i50 50Mb/s CBR"),
        E("xd5d", "XDCAM HD422 1080p24 50Mb/s CBR"),
        E("xd5e", "XDCAM HD422 1080p25 50Mb/s CBR"),
        E("xd5f", "XDCAM HD422 1080p30 50Mb/s CBR"),

        E("xdhd", "XDCAM HD 540p"),
        E("xdh2", "XDCAM HD422 540p"),

        E("AVmp", "AVID IMX PAL"),
        E("MMES", "Matrox MPEG-2"),
        E("mmes", "Matrox MPEG-2"),
        E("PIM2", "Pinnacle MPEG-2"),
        E("LMP2", "Lead MPEG-2"),

        E("VCR2", "ATI VCR-2"),

    B(HEIMDALL_CODEC_MP4V, "MPEG-4 Video"),
        A("mp4v"),
        A("DIVX"),
        A("divx"),
        A("MP4S"),
        A("mp4s"),
        A("M4S2"),
        A("m4s2"),
        A("MP4V"),
        A("\x04\x00\x00\x00"),
        A("m4cc"),
        A("M4CC"),
        A("FMP4"),
        A("fmp4"),
        A("DCOD"),
        A("MVXM"),
        A("PM4V"),
        A("M4T3"),
        A("GEOX"),
        A("GEOV"),
        A("DMK2"),
        A("WV1F"),
        A("DIGI"),
        A("INMC"),
        A("SN40"),
        A("EPHV"),
        A("DM4V"),
        A("SM4V"),
        A("DYM4"),
        /* XVID flavours */
        E("xvid", "Xvid MPEG-4 Video"),
        E("XVID", "Xvid MPEG-4 Video"),
        E("XviD", "Xvid MPEG-4 Video"),
        E("XVIX", "Xvid MPEG-4 Video"),
        E("xvix", "Xvid MPEG-4 Video"),
        /* DX50 */
        E("DX50", "DivX MPEG-4 Video"),
        E("dx50", "DivX MPEG-4 Video"),
        E("BLZ0", "Blizzard MPEG-4 Video"),
        E("DXGM", "Electronic Arts Game MPEG-4 Video"),
        E("DreX", "DreX Mpeg-4"),
        /* 3ivx delta 4 */
        E("3IV2", "3ivx MPEG-4 Video"),
        E("3iv2", "3ivx MPEG-4 Video"),
        /* Various */
        E("UMP4", "UB MPEG-4 Video"),
        E("SEDG", "Samsung MPEG-4 Video"),
        E("RMP4", "REALmagic MPEG-4 Video"),
        E("LMP4", "Lead MPEG-4 Video"),
        E("HDX4", "Jomigo HDX4 (MPEG-4 Video)"),
        E("hdx4", "Jomigo HDX4 (MPEG-4 Video)"),
        E("SMP4", "Samsung SMP4 (MPEG-4 Video)"),
        E("smp4", "Samsung SMP4 (MPEG-4 Video)"),
        E("fvfw", "libavcodec MPEG-4"),
        E("FVFW", "libavcodec MPEG-4"),
        E("FFDS", "FFDShow MPEG-4"),
        E("VIDM", "vidm 4.01 codec"),
        E("DP02", "DynaPel MPEG-4 codec"),
        E("PLV1", "Pelco DVR MPEG-4"),
        E("QMP4", "QNAP Systems MPEG-4"),
        E("qMP4", "QNAP Systems MPEG-4"),
        E("wMP4", "QNAP Systems MPEG-4"),
        E("G264", "GeoVision MPEG-4"),
        E("GM40", "GeoVision MPEG-4"),
        E("GMP4", "GeoVision MPEG-4"),
        /* 3ivx delta 3.5 Unsupported
         * putting it here gives extreme distorted images */
        //E("3IV1", "3ivx delta 3.5 MPEG-4 Video"),
        //E("3iv1", "3ivx delta 3.5 MPEG-4 Video"),

    /* MSMPEG4 v1 */
    B(HEIMDALL_CODEC_DIV1, "MS MPEG-4 Video v1"),
        A("DIV1"),
        A("div1"),
        A("MPG4"),
        A("mpg4"),
        A("mp41"),

    /* MSMPEG4 v2 */
    B(HEIMDALL_CODEC_DIV2, "MS MPEG-4 Video v2"),
        A("DIV2"),
        A("div2"),
        A("MP42"),
        A("mp42"),

    /* MSMPEG4 v3 / M$ mpeg4 v3 */
    B(HEIMDALL_CODEC_DIV3, "MS MPEG-4 Video v3"),
        A("DIV3"),
        A("MPG3"),
        A("mpg3"),
        A("div3"),
        A("MP43"),
        A("mp43"),
        /* DivX 3.20 */
        A("DIV4"),
        A("div4"),
        A("DIV5"),
        A("div5"),
        A("DIV6"),
        A("div6"),
        E("divf", "DivX 4.12"),
        E("DIVF", "DivX 4.12"),
        /* Cool Codec */
        A("COL1"),
        A("col1"),
        A("COL0"),
        A("col0"),
        /* AngelPotion stuff */
        A("AP41"),
        /* 3ivx doctered divx files */
        A("3IVD"),
        A("3ivd"),
        /* who knows? */
        A("3VID"),
        A("3vid"),
        A("DVX1"),
        A("DVX3"),

    /* Sorenson v1 */
    B(HEIMDALL_CODEC_SVQ1, "SVQ-1 (Sorenson Video v1)"),
        A("SVQ1"),
        A("svq1"),
        A("svqi"),

    /* Sorenson v3 */
    B(HEIMDALL_CODEC_SVQ3, "SVQ-3 (Sorenson Video v3)"),
        A("SVQ3"),

    /* VVC / H.266 */
    B(HEIMDALL_CODEC_VVC, "MPEG-I Part3/VVC (H.266)"),
        A("vvc1"),

    /* HEVC / H.265 */
    B(HEIMDALL_CODEC_HEVC, "MPEG-H Part2/HEVC (H.265)"),
        A("hevc"),
        A("HEVC"),
        A("h265"),
        A("H265"),
        A("x265"),
        A("X265"),
        A("hev1"),
        A("hvc1"),
        A("HM10"),
        E("dvhe", "Dolby Vision HEVC (H.265)"),
        E("GHEC", "GeoVision HEVC"),
        /* E("dvh1", "Dolby Vision HEVC (H.265)"), Collides with DV */

    /* h264 */
    B(HEIMDALL_CODEC_H264, "H264 - MPEG-4 AVC (part 10)"),
        A("H264"),
        A("h264"),
        A("x264"),
        A("X264"),
        A("V264"),
        /* avc1: special case h264 */
        A("avc1"),
        A("AVC1"),
        A("AVCB"), /* smooth streaming alias */
        A("avc3"),
        E("ai5p", "AVC-Intra  50M 720p24/30/60"),
        E("ai5q", "AVC-Intra  50M 720p25/50"),
        E("ai52", "AVC-Intra  50M 1080p25/50"),
        E("ai53", "AVC-Intra  50M 1080p24/30/60"),
        E("ai55", "AVC-Intra  50M 1080i50"),
        E("ai56", "AVC-Intra  50M 1080i60"),
        E("ai1p", "AVC-Intra 100M 720p24/30/60"),
        E("ai1q", "AVC-Intra 100M 720p25/50"),
        E("ai12", "AVC-Intra 100M 1080p25/50"),
        E("ai13", "AVC-Intra 100M 1080p24/30/60"),
        E("ai15", "AVC-Intra 100M 1080i50"),
        E("ai16", "AVC-Intra 100M 1080i60"),
        E("dvav", "Dolby Vision H264"),
        E("dva1", "Dolby Vision H264"),
        E("VSSH", "Vanguard VSS H264"),
        E("VSSW", "Vanguard VSS H264"),
        E("vssh", "Vanguard VSS H264"),
        E("DAVC", "Dicas MPEGable H.264/MPEG-4 AVC"),
        E("davc", "Dicas MPEGable H.264/MPEG-4 AVC"),
        E("x3eV", "DreX H.264"),
        E("GAVC", "GeoVision MPEG-4 AVC"),
        E("Q264", "QNAP H.264/MPEG-4 AVC"),
        E("q264", "QNAP H.264/MPEG-4 AVC"),
        E("w264", "QNAP H.264/MPEG-4 AVC"),
        A("UMSV"),
        A("SMV2"),
        A("tshd"),
        A("rv64"),

    /* H263 and H263i */
    /* H263(+) is also known as Real Video 1.0 */

    /* H263 */
    B(HEIMDALL_CODEC_H263, "H263"),
        A("H263"),
        A("h263"),
        A("VX1K"),
        A("s263"),
        A("S263"),
        A("u263"),
        A("lsvm"),
        E("T263", "March H263"),
        E("D263", "DEC H263"),
        E("d263", "DEC H263"),
        E("L263", "LEAD H263"),
        E("M263", "Microsoft H263"),
        E("X263", "Xirlink H263"),
        /* Zygo (partial) */
        E("ZyGo", "ITU H263+"),
        A("HAVI"),

    /* H263i */
    B(HEIMDALL_CODEC_H263I, "I263.I"),
        A("I263"),
        A("i263"),

    /* H263P */
    B(HEIMDALL_CODEC_H263P, "ITU H263+"),
        E("ILVR", "ITU H263+"),
        E("viv1", "H263+"),
        E("vivO", "H263+"),
        E("viv2", "H263+"),
        E("VIVO", "H263+"),
        E("U263", "UB H263+"),

    /* Flash (H263) variant */
    B(HEIMDALL_CODEC_FLV1, "Flash Video"),
        A("FLV1"),
        A("flv "),

    /* H261 */
    B(HEIMDALL_CODEC_H261, "H.261"),
        A("H261"),
        A("h261"),

    B(HEIMDALL_CODEC_FLIC, "Flic Video"),
        A("FLIC"),
        A("AFLC"),

    /* MJPEG */
    B(HEIMDALL_CODEC_MJPG, "Motion JPEG Video"),
        A("MJPG"),
        A("MJPx"),
        A("mjpg"),
        A("mJPG"),
        A("mjpa"),
        A("JFIF"),
        A("JPGL"),
        A("MMJP"),
        A("FLJP"),
        A("FMJP"),
        A("SJPG"),
        E("QIVG", "QNAP Motion JPEG"),
        E("qIVG", "QNAP Motion JPEG"),
        E("wIVG", "QNAP Motion JPEG"),
        E("AVRn", "Avid Motion JPEG"),
        E("AVDJ", "Avid Motion JPEG"),
        E("ADJV", "Avid Motion JPEG"),
        E("dmb1", "Motion JPEG OpenDML Video"),
        E("DMB1", "Motion JPEG OpenDML Video"),
        E("ijpg", "Intergraph JPEG Video"),
        E("IJPG", "Intergraph JPEG Video"),
        E("ACDV", "ACD Systems Digital"),
        E("SLMJ", "SL M-JPEG"),

    B(HEIMDALL_CODEC_MJPGB, "Motion JPEG B Video"),
        A("mjpb"),

    B(HEIMDALL_CODEC_LJPG, "Lead Motion JPEG Video"),
        E("Ljpg", "Lead Motion JPEG"),

    /* SP5x */
    B(HEIMDALL_CODEC_SP5X, "Sunplus Motion JPEG Video"),
        A("SP5X"),
        A("SP53"),
        A("SP54"),
        A("SP55"),
        A("SP56"),
        A("SP57"),
        A("SP58"),

    /* DV */
    B(HEIMDALL_CODEC_DV, "DV Video"),
        A("dv  "),
        A("dvsl"),
        A("DVSD"),
        A("dvsd"),
        A("DVCS"),
        A("dvcs"),
        A("dvhd"),
        A("dvhq"),
        A("dvh1"),
        E("dvh2", "DV Video 720p24"),
        E("dvh3", "DV Video 720p25"),
        E("dvh4", "DV Video 720p30"),
        A("dv25"),
        A("dc25"),
        A("dvs1"),
        A("dvis"),
        A("CDV2"),
        A("CDVP"),
        A("PDVC"),
        A("IPDV"),
        A("ipdv"),
        A("pdvc"),
        A("SL25"),
        E("dvcp", "DV Video PAL"),
        E("dvc ", "DV Video NTSC" ),
        E("dvp ", "DV Video Pro"),
        E("dvpp", "DV Video Pro PAL"),
        E("dv50", "DV Video C Pro 50"),
        E("dv5p", "DV Video C Pro 50 PAL"),
        E("dv5n", "DV Video C Pro 50 NTSC"),
        E("dv1p", "DV Video C Pro 100 PAL" ),
        E("dv1n", "DV Video C Pro 100 NTSC" ),
        E("dvhp", "DV Video C Pro HD 720p" ),
        E("dvh5", "DV Video C Pro HD 1080i50" ),
        E("dvh6", "DV Video C Pro HD 1080i60" ),
        E("AVdv", "AVID DV"),
        E("AVd1", "AVID DV"),
        E("CDVC", "Canopus DV Video"),
        E("cdvc", "Canopus DV Video"),
        E("CDVH", "Canopus DV Video"),
        E("cdvh", "Canopus DV Video"),
        E("CDV5", "Canopus DV Video"),
        E("SLDV", "SoftLab DVCAM codec"),

    /* Windows Media Video */
    B(HEIMDALL_CODEC_WMV1, "Windows Media Video 7"),
        A("WMV1"),
        A("wmv1"),

    B(HEIMDALL_CODEC_WMV2, "Windows Media Video 8"),
        A("WMV2"),
        A("wmv2"),
        A("GXVE"),

    B(HEIMDALL_CODEC_WMV3, "Windows Media Video 9"),
        A("WMV3"),
        A("wmv3"),

    /* WMVA is the VC-1 codec before the standardization process,
     * it is not bitstream compatible and deprecated  */
    B(HEIMDALL_CODEC_WMVA, "Windows Media Video Advanced Profile"),
        A("WMVA"),
        A("wmva"),

    B(HEIMDALL_CODEC_VC1, "Windows Media Video VC1"),
        A("WVC1"),
        A("wvc1"),
        A("vc-1"),
        A("VC-1"),

    B(HEIMDALL_CODEC_WMVP, "Windows Media Video Presentation"),
        A("WMVP"),
        A("wmvp"),

    B(HEIMDALL_CODEC_WMVP2, "Windows Media Video Presentation, v2"),
        A("WVP2"),
        A("wvp2"),

    /* Microsoft Video 1 */
    B(HEIMDALL_CODEC_MSVIDEO1, "Microsoft Video 1"),
        A("MSVC"),
        A("msvc"),
        A("CRAM"),
        A("cram"),
        A("WHAM"),
        A("wham"),

    /* Microsoft RLE */
    B(HEIMDALL_CODEC_MSRLE, "Microsoft RLE Video"),
        A("mrle"),
        A("WRLE"),
        A("\x01\x00\x00\x00"),
        A("\x02\x00\x00\x00"),

    /* Indeo Video Codecs (Quality of this decoder on ppc is not good) */
    B(HEIMDALL_CODEC_INDEO3, "Indeo Video v3"),
        A("IV31"),
        A("iv31"),
        A("IV32"),
        A("iv32"),

    /* Huff YUV */
    B(HEIMDALL_CODEC_HUFFYUV, "Huff YUV Video"),
        A("HFYU"),

    B(HEIMDALL_CODEC_FFVHUFF, "Huff YUV Video"),
        A("FFVH"),

    /* On2 VP3 Video Codecs */
    B(HEIMDALL_CODEC_VP3, "On2's VP3 Video"),
        A("VP3 "),
        A("VP30"),
        A("vp30"),
        A("VP31"),
        A("vp31"),

    /* On2 VP4 Video Codec */
    B(HEIMDALL_CODEC_VP4, "On2's VP4 Video"),
        A("VP40"),

    /* On2  VP5, VP6 codecs */
    B(HEIMDALL_CODEC_VP5, "On2's VP5 Video"),
        A("VP5 "),
        A("VP50"),

    B(HEIMDALL_CODEC_VP6, "On2's VP6.2 Video"),
        A("VP62"),
        A("vp62"),
        E("VP60", "On2's VP6.0 Video"),
        E("VP61", "On2's VP6.1 Video"),

    B(HEIMDALL_CODEC_VP6F, "On2's VP6.2 Video (Flash)"),
        A("VP6F"),
        A("FLV4"),

    B(HEIMDALL_CODEC_VP6A, "On2's VP6 A Video"),
        A("VP6A"),

    B(HEIMDALL_CODEC_VP7, "Google/On2's VP7 Video"),
        A("VP70"),
        A("VP71"),

    B(HEIMDALL_CODEC_VP8, "Google/On2's VP8 Video"),
        A("VP80"),

    B(HEIMDALL_CODEC_VP8ALPHA_ES, "Google/On2's VP8 Alpha"),
        A("VP8a"),

    B(HEIMDALL_CODEC_VP9, "Google/On2's VP9 Video"),
        A("VP90"),

    B(HEIMDALL_CODEC_VP9ALPHA_ES, "Google/On2's VP9 Alpha"),
        A("VP9a"),

    B(HEIMDALL_CODEC_AV1, "AOMedia's AV1 Video"),
        A("av10"),
        A("AV01"),

    /* Xiph.org theora */
    B(HEIMDALL_CODEC_THEORA, "Xiph.org's Theora Video"),
        A("theo"),
        A("THEO"),
        A("Thra"),

    /* Xiph.org tarkin */
    B(HEIMDALL_CODEC_TARKIN, "Xiph.org's Tarkin Video"),
        A("tark"),

    /* Asus Video (Another thing that doesn't work on PPC) */
    B(HEIMDALL_CODEC_ASV1, "Asus V1 Video"),
        A("ASV1"),
    B(HEIMDALL_CODEC_ASV2, "Asus V2 Video"),
        A("ASV2"),

    /* FF video codec 1 (lossless codec) */
    B(HEIMDALL_CODEC_FFV1, "FF video codec 1"),
        A("FFV1"),

    /* ATI VCR1 */
    B(HEIMDALL_CODEC_VCR1, "ATI VCR1 Video"),
        A("VCR1"),

    /* Cirrus Logic AccuPak */
    B(HEIMDALL_CODEC_CLJR, "Creative Logic AccuPak"),
        A("CLJR"),

    /* Real Video */
    B(HEIMDALL_CODEC_RV10, "RealVideo 1.0"),
        A("RV10"),
        A("rv10"),

    B(HEIMDALL_CODEC_RV13, "RealVideo 1.3"),
        A("RV13"),
        A("rv13"),

    B(HEIMDALL_CODEC_RV20, "RealVideo G2 (2.0)"),
        A("RV20"),
        A("rv20"),

    B(HEIMDALL_CODEC_RV30, "RealVideo 8 (3.0)"),
        A("RV30"),
        A("rv30"),

    B(HEIMDALL_CODEC_RV40, "RealVideo 9/10 (4.0)"),
        A("RV40"),
        A("rv40"),

    /* Apple Video */
    B(HEIMDALL_CODEC_RPZA, "Apple Video"),
        A("rpza"),
        A("azpr"),
        A("RPZA"),
        A("AZPR"),

    B(HEIMDALL_CODEC_SMC, "Apple graphics"),
        A("smc "),

    B(HEIMDALL_CODEC_CINEPAK, "Cinepak Video"),
        A("CVID"),
        A("cvid"),

    /* Screen Capture Video Codecs */
    B(HEIMDALL_CODEC_TSCC, "TechSmith Camtasia Screen Capture"),
        A("TSCC"),
        A("tscc"),

    B(HEIMDALL_CODEC_CSCD, "CamStudio Screen Codec"),
        A("CSCD"),
        A("cscd"),

    B(HEIMDALL_CODEC_ZMBV, "DosBox Capture Codec"),
        A("ZMBV"),

    B(HEIMDALL_CODEC_VMNC, "VMware Video"),
        A("VMnc"),

    B(HEIMDALL_CODEC_FMVC, "FM Screen Capture Codec"),
        A("FMVC"),

    B(HEIMDALL_CODEC_FRAPS, "FRAPS: Realtime Video Capture"),
        A("FPS1"),
        A("fps1"),

    /* Duck TrueMotion */
    B(HEIMDALL_CODEC_TRUEMOTION1, "Duck TrueMotion v1 Video"),
        A("DUCK"),
        A("PVEZ"),
    B(HEIMDALL_CODEC_TRUEMOTION2, "Duck TrueMotion v2.0 Video"),
        A("TM20"),

    B(HEIMDALL_CODEC_QTRLE, "Apple QuickTime RLE Video"),
        A("rle "),

    B(HEIMDALL_CODEC_QDRAW, "Apple QuickDraw Video"),
        A("qdrw"),

    B(HEIMDALL_CODEC_QOI, "Quite OK Image Format"),
        A("qoif"),

    B(HEIMDALL_CODEC_QPEG, "QPEG Video"),
        A("QPEG"),
        A("Q1.0"),
        A("Q1.1"),

    B(HEIMDALL_CODEC_ULTI, "IBM Ultimotion Video"),
        A("ULTI"),

    B(HEIMDALL_CODEC_VIXL, "Miro/Pinnacle VideoXL Video"),
        A("VIXL"),
        A("XIXL"),
        E("PIXL", "Pinnacle VideoXL Video"),

    B(HEIMDALL_CODEC_LOCO, "LOCO Video"),
        A("LOCO"),

    B(HEIMDALL_CODEC_WNV1, "Winnov WNV1 Video"),
        A("WNV1"),
        A("YUV8"),

    B(HEIMDALL_CODEC_AASC, "Autodesc RLE Video"),
        A("AASC"),
        E("AAS4", "Autodesc RLE Video 24bit"),

    B(HEIMDALL_CODEC_INDEO2, "Indeo Video v2"),
        A("IV20"),
        E("RT21", "Indeo Video 2.1" ),

    /* Flash Screen Video */
    B(HEIMDALL_CODEC_FLASHSV, "Flash Screen Video"),
        A("FSV1"),
    B(HEIMDALL_CODEC_FLASHSV2, "Flash Screen Video 2"),
        A("FSV2"),

    B(HEIMDALL_CODEC_KMVC, "Karl Morton's Video Codec (Worms)"),
        A("KMVC"),

    B(HEIMDALL_CODEC_NUV, "Nuppel Video"),
        A("RJPG"),
        A("NUV1"),

    /* CODEC_ID_SMACKVIDEO */
    B(HEIMDALL_CODEC_SMACKVIDEO, "Smacker Video"),
        A("SMK2"),
        A("SMK4"),

    /* Chinese AVS - Untested */
    B(HEIMDALL_CODEC_CAVS, "Chinese AVS"),
        A("CAVS"),
        A("AVs2"),
        A("avs2"),

    B(HEIMDALL_CODEC_CAVS2, "AVS2-P2 video (IEEE 1857.4)"),
        A("AVS2"),

    B(HEIMDALL_CODEC_CAVS3, "AVS3-P2 video (IEEE 1857.10)"),

    B(HEIMDALL_CODEC_AMV, "AMV"),

    B(HEIMDALL_CODEC_BINKVIDEO, "Bink Video"),

    B(HEIMDALL_CODEC_BINKAUDIO_DCT, "Bink Audio (DCT)"),

    B(HEIMDALL_CODEC_BINKAUDIO_RDFT, "Bink Audio (RDFT)"),

    /* */
    B(HEIMDALL_CODEC_DNXHD, "DNxHD"),
        A("AVdn"),
        E("AVdh", "DNxHR"),
    B(HEIMDALL_CODEC_8BPS, "8BPS"),
        A("8BPS"),
    B(HEIMDALL_CODEC_MIMIC, "Mimic"),
        A("ML2O"),

    B(HEIMDALL_CODEC_CDG, "CD-G Video"),
        A("CDG "),

    B(HEIMDALL_CODEC_FRWU, "Forward Uncompressed" ),
        A("FRWU"),

    B(HEIMDALL_CODEC_INDEO4, "Indeo Video v4"),
        A("IV41"),
        A("iv41"),

    B(HEIMDALL_CODEC_INDEO5, "Indeo Video v5"),
        A("IV50"),
        A("iv50"),

    B(HEIMDALL_CODEC_PRORES, "Apple ProRes"),
        E("apcn", "Apple ProRes 422 Standard"),
        E("apch", "Apple ProRes 422 HQ"),
        E("apcs", "Apple ProRes 422 LT"),
        E("apco", "Apple ProRes 422 Proxy"),
        E("ap4c", "Apple ProRes 4444"),
        E("ap4h", "Apple ProRes 4444"),
        E("ap4x", "Apple ProRes 4444 XQ"),

    B(HEIMDALL_CODEC_ICOD, "Apple Intermediate Codec"),
        A("icod"),

    B(HEIMDALL_CODEC_G2M2, "GoTo Meeting Codec 2"),
        A("G2M2"),

    B(HEIMDALL_CODEC_G2M3, "GoTo Meeting Codec 3"),
        A("G2M3"),

    B(HEIMDALL_CODEC_G2M4, "GoTo Meeting Codec 4"),
        A("G2M4"),
        A("G2M5"),

    B(HEIMDALL_CODEC_FIC, "Mirillis FIC video"),
        A("FICV"),

    B(HEIMDALL_CODEC_TDSC, "TDSC"),

    B(HEIMDALL_CODEC_HQX, "Canopus HQX"),

    B(HEIMDALL_CODEC_HQ_HQA, "Canopus HQ"),

    B(HEIMDALL_CODEC_HAP, "Vidvox Hap"),
        A("Hap1"),
        E("Hap5", "Vidvox Hap Alpha"),
        E("HapY", "Vidvox Hap Q"),
        E("HapA", "Vidvox Hap Alpha-Only"),
        E("HapM", "Vidvox Hap Q Alpha"),

    B(HEIMDALL_CODEC_DXV, "Resolume DXV"),
        A("DXDI"),
        E("DXD3", "Resolume DXV version 3"),

    /* */
    B(HEIMDALL_CODEC_YV12, "Planar 4:2:0 YVU"),
        A("YV12"),
        A("yv12"),
    B(HEIMDALL_CODEC_I410, "Planar 4:1:0 YUV"),
        A("I410"),
    B(HEIMDALL_CODEC_I411, "Planar 4:1:1 YUV"),
        A("I411"),
        A("Y41B"),
    B(HEIMDALL_CODEC_I420, "Planar 4:2:0 YUV"),
        A("I420"),
        A("IYUV"),
    B(HEIMDALL_CODEC_I422, "Planar 4:2:2 YUV"),
        A("I422"),
        A("Y42B"),
    B(HEIMDALL_CODEC_I440, "Planar 4:4:0 YUV"),
        A("I440"),
    B(HEIMDALL_CODEC_I444, "Planar 4:4:4 YUV"),
        A("I444"),

    B(HEIMDALL_CODEC_YUVP, "Palettized YUV with palette element Y:U:V:A"),
        A("YUVP"),

    B(HEIMDALL_CODEC_YUVA, "Planar YUV 4:4:4 Y:U:V:A"),
        A("YUVA"),
    B(HEIMDALL_CODEC_YUV420A, "Planar YUV 4:2:0 Y:U:V:A"),
        A("I40A"),
    B(HEIMDALL_CODEC_YUV422A, "Planar YUV 4:2:2 Y:U:V:A"),
        A("I42A"),
    B(HEIMDALL_CODEC_YUVA_444_10L, "Planar YUV 4:4:4 Y:U:V:A 10bits LE"),
    B(HEIMDALL_CODEC_YUVA_444_10B, "Planar YUV 4:4:4 Y:U:V:A 10bits BE"),
    B(HEIMDALL_CODEC_YUVA_444_12L, "Planar YUV 4:4:4 Y:U:V:A 12bits LE"),
    B(HEIMDALL_CODEC_YUVA_444_12B, "Planar YUV 4:4:4 Y:U:V:A 12bits BE"),

    B(HEIMDALL_CODEC_RGBP, "Palettized RGB with palette element R:G:B:A"),
        A("RGBP"),

    B(HEIMDALL_CODEC_RGB233, "8 bits RGB 2:3:3"),
        A("RGB8"),
    B(HEIMDALL_CODEC_RGB332, "8 bits RGB 3:3:2"),
        A("R332"),
    B(HEIMDALL_CODEC_BGR233, "8 bits BGR 2:3:3"),
        A("B233"),
    B(HEIMDALL_CODEC_RGBA, "32 bits RGBA"),
        A("RGBA"),
    B(HEIMDALL_CODEC_ARGB, "32 bits ARGB"),
        A("ARGB"),
        A("AV32"),
    B(HEIMDALL_CODEC_BGRA, "32 bits BGRA"),
        A("BGRA"),
    B(HEIMDALL_CODEC_ABGR, "32 bits ABGR"),
    B(HEIMDALL_CODEC_RGBA10LE, "32 bits RGB 10bits A 2bits LE"),
        A("RGA0"),
    B(HEIMDALL_CODEC_RGBA64, "64 bits RGBA"),
        A("RGA4"),
    B(HEIMDALL_CODEC_XRGB, "32 bits xRGB"),
        A("RV32"),
    B(HEIMDALL_CODEC_RGBX, "32 bits RGBx"),
        A("RGBX"),
    B(HEIMDALL_CODEC_XBGR, "32 bits xBGR"),
        A("XBGR"),
    B(HEIMDALL_CODEC_BGRX, "32 bits BGRx"),
        A("BGRX"),
    B(HEIMDALL_CODEC_RGB24, "24 bits RGB"),
        A("RV24"),
    B(HEIMDALL_CODEC_BGR24, "24 bits BGR"),
        A("BGR3"),
    B(HEIMDALL_CODEC_RGB565LE, "16 bits R5G6G5 LE"),
        A("RV16"),
    B(HEIMDALL_CODEC_RGB565BE, "16 bits R5G6G5 BE"),
        A("RB16"),
    B(HEIMDALL_CODEC_BGR565LE, "16 bits B5G6R5 LE"),
        A("BL16"),
    B(HEIMDALL_CODEC_BGR565BE, "16 bits B5G6R5 BE"),
        A("BB16"),
    B(HEIMDALL_CODEC_RGB555LE, "15 bits R5G5G5 LE"),
        A("RV15"),
    B(HEIMDALL_CODEC_RGB555BE, "15 bits R5G5G5 BE"),
        A("RB15"),
    B(HEIMDALL_CODEC_BGR555LE, "15 bits B5G5R5 LE"),
        A("BL15"),
    B(HEIMDALL_CODEC_BGR555LE, "15 bits B5G5R5 BE"),
        A("BB15"),

    B(HEIMDALL_CODEC_GREY, "8 bits greyscale"),
        A("GREY"),
        A("Y800"),
        A("Y8  "),

    B(HEIMDALL_CODEC_GREY_10L, "10 bits greyscale LE"),
        A("G0FL"),

    B(HEIMDALL_CODEC_GREY_10B, "10 bits greyscale BE"),
        A("G0FB"),

    B(HEIMDALL_CODEC_GREY_12L, "12 bits greyscale LE"),
        A("G2FL"),

    B(HEIMDALL_CODEC_GREY_12B, "12 bits greyscale BE"),
        A("G2FB"),

    B(HEIMDALL_CODEC_GREY_16L, "16 bits greyscale LE"),
        A("GRFL"),

    B(HEIMDALL_CODEC_GREY_16B, "16 bits greyscale BE"),
        A("GRFB"),

    B(HEIMDALL_CODEC_UYVY, "Packed YUV 4:2:2, U:Y:V:Y"),
        A("UYVY"),
        A("UYNV"),
        A("UYNY"),
        A("Y422"),
        A("HDYC"),
        A("AVUI"),
        A("uyv1"),
        A("2vuy"),
        A("2Vuy"),
        A("2Vu1"),
    B(HEIMDALL_CODEC_VYUY, "Packed YUV 4:2:2, V:Y:U:Y"),
        A("VYUY"),
    B(HEIMDALL_CODEC_YUYV, "Packed YUV 4:2:2, Y:U:Y:V"),
        A("YUY2"),
        A("YUYV"),
        A("YUNV"),
        A("V422"),

    B(HEIMDALL_CODEC_YUV2, "Packed YUV 4:2:2, Y:U:Y:V signed"),
        A("yuv2"),
        A("yuvu"),

    B(HEIMDALL_CODEC_YVYU, "Packed YUV 4:2:2, Y:V:Y:U"),
        A("YVYU"),

    B(HEIMDALL_CODEC_Y211, "Packed YUV 2:1:1, Y:U:Y:V "),
        A("Y211"),
    B(HEIMDALL_CODEC_CYUV, "Creative Packed YUV 4:2:2, U:Y:V:Y, reverted"),
        A("cyuv"),
        A("CYUV"),

    B(HEIMDALL_CODEC_V210, "10-bit 4:2:2 Component YCbCr"),
        A("v210"),

    B(HEIMDALL_CODEC_Y210, "Packed 10-bit YUV 4:2:2"),
        A("Y210"),
    B(HEIMDALL_CODEC_Y212, "Packed 12-bit YUV 4:2:2"),
        A("Y212"),
    B(HEIMDALL_CODEC_VUYA, "Packed YUV 4:4:4, V:U:Y:A"),
        A("VUYA"),
    B(HEIMDALL_CODEC_VUYX, "Packed YUV 4:4:4, V:U:Y:X"),
        A("VUYX"),
    B(HEIMDALL_CODEC_Y410, "Packed 10-bit YUV 4:4:4"),
        A("Y410"),
    B(HEIMDALL_CODEC_Y412, "Packed 12-bit YUV 4:4:4"),
        A("Y412"),

    B(HEIMDALL_CODEC_NV12, "Biplanar 4:2:0 Y/UV"),
        A("NV12"),
    B(HEIMDALL_CODEC_NV21, "Biplanar 4:2:0 Y/VU"),
        A("NV21"),
    B(HEIMDALL_CODEC_P010, "Biplanar 4:2:0 Y/VU 10-bit LE"),
        A("P010"),
    B(HEIMDALL_CODEC_P012, "Biplanar 4:2:0 Y/VU 12-bit LE"),
        A("P012"),
    B(HEIMDALL_CODEC_P016, "Biplanar 4:2:0 Y/VU 16-bit LE"),
        A("P016"),
    B(HEIMDALL_CODEC_NV16, "Biplanar 4:2:2 Y/UV"),
        A("NV16"),
    B(HEIMDALL_CODEC_NV61, "Biplanar 4:2:2 Y/VU"),
        A("NV61"),
    B(HEIMDALL_CODEC_NV24, "Biplanar 4:4:4 Y/UV"),
        A("NV24"),
    B(HEIMDALL_CODEC_NV42, "Biplanar 4:4:4 Y/VU"),
        A("NV42"),

    B(HEIMDALL_CODEC_I420_9L, "Planar 4:2:0 YUV 9-bit LE"),
        A("I09L"),
    B(HEIMDALL_CODEC_I420_9B, "Planar 4:2:0 YUV 9-bit BE"),
        A("I09B"),
    B(HEIMDALL_CODEC_I422_9L, "Planar 4:2:2 YUV 9-bit LE"),
        A("I29L"),
    B(HEIMDALL_CODEC_I422_9B, "Planar 4:2:2 YUV 9-bit BE"),
        A("I29B"),
    B(HEIMDALL_CODEC_I444_9L, "Planar 4:4:4 YUV 9-bit LE"),
        A("I49L"),
    B(HEIMDALL_CODEC_I444_9B, "Planar 4:4:4 YUV 9-bit BE"),
        A("I49B"),

    B(HEIMDALL_CODEC_I420_10L, "Planar 4:2:0 YUV 10-bit LE"),
        A("I0AL"),
    B(HEIMDALL_CODEC_I420_10B, "Planar 4:2:0 YUV 10-bit BE"),
        A("I0AB"),
    B(HEIMDALL_CODEC_I422_10L, "Planar 4:2:2 YUV 10-bit LE"),
        A("I2AL"),
    B(HEIMDALL_CODEC_I422_10B, "Planar 4:2:2 YUV 10-bit BE"),
        A("I2AB"),
    B(HEIMDALL_CODEC_I444_10L, "Planar 4:4:4 YUV 10-bit LE"),
        A("I4AL"),
    B(HEIMDALL_CODEC_I444_10B, "Planar 4:4:4 YUV 10-bit BE"),
        A("I4AB"),

    B(HEIMDALL_CODEC_I420_12L, "Planar 4:2:0 YUV 12-bit LE"),
        A("I0CL"),
    B(HEIMDALL_CODEC_I420_12B, "Planar 4:2:0 YUV 12-bit BE"),
        A("I0CB"),
    B(HEIMDALL_CODEC_I422_12L, "Planar 4:2:2 YUV 12-bit LE"),
        A("I2CL"),
    B(HEIMDALL_CODEC_I422_12B, "Planar 4:2:2 YUV 12-bit BE"),
        A("I2CB"),
    B(HEIMDALL_CODEC_I444_12L, "Planar 4:4:4 YUV 12-bit LE"),
        A("I4CL"),
    B(HEIMDALL_CODEC_I444_12B, "Planar 4:4:4 YUV 12-bit BE"),
        A("I4CB"),

    B(HEIMDALL_CODEC_I420_16L, "Planar 4:2:0 YUV 16-bit LE"),
        A("I0FL"),
    B(HEIMDALL_CODEC_I420_16B, "Planar 4:2:0 YUV 16-bit BE"),
        A("I0FB"),
    B(HEIMDALL_CODEC_I444_16L, "Planar 4:4:4 YUV 16-bit LE"),
        A("I4FL"),
    B(HEIMDALL_CODEC_I444_16B, "Planar 4:4:4 YUV 16-bit BE"),
        A("I4FB"),


    /* XYZ color space */
    B(HEIMDALL_CODEC_XYZ_12L, "Packed XYZ 12-bit LE"),

    B(HEIMDALL_CODEC_XYZ_12B, "Packed XYZ 12-bit BE"),
        A("XY12"),

    /* Videogames Codecs */

    /* Interplay MVE */
    B(HEIMDALL_CODEC_INTERPLAY, "Interplay MVE Video"),
        A("imve"),
        A("INPV"),

    /* Id Quake II CIN */
    B(HEIMDALL_CODEC_IDCIN, "Id Quake II CIN Video"),
        A("IDCI"),

    /* 4X Technologies */
    B(HEIMDALL_CODEC_4XM, "4X Technologies Video"),
        A("4XMV"),
        A("4xmv"),

    /* Id RoQ */
    B(HEIMDALL_CODEC_ROQ, "Id RoQ Video"),
        A("RoQv"),

    /* Sony Playstation MDEC */
    B(HEIMDALL_CODEC_MDEC, "PSX MDEC Video"),
        A("MDEC"),

    /* Sierra VMD */
    B(HEIMDALL_CODEC_VMDVIDEO, "Sierra VMD Video"),
        A("VMDV"),
        A("vmdv"),

    B(HEIMDALL_CODEC_DIRAC, "Dirac" ),
        A("drac"),

    /* Image */
    B(HEIMDALL_CODEC_PNG, "PNG Image"),
        A("png "),

    B(HEIMDALL_CODEC_PPM, "PPM Image"),
        A("ppm "),

    B(HEIMDALL_CODEC_PGM, "PGM Image"),
        A("pgm "),

    B(HEIMDALL_CODEC_PGMYUV, "PGM YUV Image"),
        A("pgmy"),

    B(HEIMDALL_CODEC_PAM, "PAM Image"),
        A("pam "),

    B(HEIMDALL_CODEC_JPEGLS, "JPEG-LS"),
        A("MJLS"),

    B(HEIMDALL_CODEC_JPEG, "JPEG"),
        A("jpeg"),
        A("JPEG"),

    B(HEIMDALL_CODEC_BPG, "BPG Image"),
        A("BPG "),

    B(HEIMDALL_CODEC_BMP, "BMP Image"),
        A("bmp "),

    B(HEIMDALL_CODEC_FARBFELD, "Farbfeld Image"),
        A("farb"),

    B(HEIMDALL_CODEC_TIFF, "TIFF Image"),
        A("tiff"),

    B(HEIMDALL_CODEC_GIF, "GIF Image"),
        A("gif "),


    B(HEIMDALL_CODEC_TARGA, "Truevision Targa Image"),
        A("tga "),
        A("mtga"),
        A("MTGA"),

    B(HEIMDALL_CODEC_SGI, "SGI Image"),
        A("sgi "),

    B(HEIMDALL_CODEC_SVG, "SVG Scalable Vector Graphics Image"),
        A("svg "),

    B(HEIMDALL_CODEC_PNM, "Portable Anymap Image"),
        A("pnm "),

    B(HEIMDALL_CODEC_PCX, "Personal Computer Exchange Image"),
        A("pcx "),

    B(HEIMDALL_CODEC_XWD, "X Window system raster image"),

    B(HEIMDALL_CODEC_JPEG2000, "JPEG 2000 Image"),
        A("JP2K"),
        A("mjp2"),
        A("MJP2"),
        A("MJ2C"),
        A("LJ2C"),
        A("LJ2K"),

    B(HEIMDALL_CODEC_WEBP, "WebP Image"),
        A("WEBP"),

    B(HEIMDALL_CODEC_LAGARITH, "Lagarith Lossless"),
        A("LAGS"),

    B(HEIMDALL_CODEC_MXPEG, "Mxpeg"),
        A("MXPG"),

    B(HEIMDALL_CODEC_CDXL, "Commodore CDXL video format"),
        A("CDXL"),

    B(HEIMDALL_CODEC_BMVVIDEO, "Discworld II BMV video"),
        A("BMVV"),

    B(HEIMDALL_CODEC_UTVIDEO, "Ut Video"),
        A("ULRA"),
        A("ULRG"),
        A("ULY0"),
        A("ULY2"),
        A("ULY4"),
        A("ULH0"),
        A("ULH2"),
        A("ULH4"),
        A("UQY2"),
        A("UQRA"),
        A("UQRG"),

    B(HEIMDALL_CODEC_VBLE, "VBLE Lossless"),
        A("VBLE"),

    B(HEIMDALL_CODEC_DXTORY, "Dxtory capture format"),
        A("xtor"),

    B(HEIMDALL_CODEC_MSS1, "Windows Media Video 7 Screen"),
        A("MSS1"),
        A("mss1"),

    B(HEIMDALL_CODEC_MSS2, "Windows Media Video 9 Screen"),
        A("MSS2"),
        A("mss2"),

    B(HEIMDALL_CODEC_MSA1, "Microsoft Application Screen Decoder 1"),
        A("MSA1"),

    B(HEIMDALL_CODEC_TSC2, "TechSmith Screen Codec 2"),
        A("tsc2"),

    B(HEIMDALL_CODEC_MTS2, "Microsoft Expression Encoder Screen"),
        A("MTS2"),

    B(HEIMDALL_CODEC_XAN_WC4, "Wing Commander IV Xan video"),
        A("Xxan"),

    B(HEIMDALL_CODEC_LCL_MSZH, "Loss-Less Codec Library AVImszh"),
        A("MSZH"),

    B(HEIMDALL_CODEC_LCL_ZLIB, "Loss-Less Codec Library AVIzlib"),
        A("ZLIB"),

    B(HEIMDALL_CODEC_THP, "GameCube THP video"),

    B(HEIMDALL_CODEC_TXD, "RenderWare TXD"),

    B(HEIMDALL_CODEC_ESCAPE124, "Escape 124 video"),

    B(HEIMDALL_CODEC_KGV1, "Kega Game Video (KGV1)"),
        A("KGV1"),

    B(HEIMDALL_CODEC_CLLC, "Canopus Lossless"),
        A("CLLC"),

    B(HEIMDALL_CODEC_AURA, "Auravision Aura"),
        A("AURA"),

    B(HEIMDALL_CODEC_TMV, "8088flex TMV"),

    B(HEIMDALL_CODEC_XAN_WC3, "Wing Commander III video"),

    B(HEIMDALL_CODEC_WS_VQA, "Westwood Studios VQA"),

    B(HEIMDALL_CODEC_MMVIDEO, "American Laser Games MM Video"),

    B(HEIMDALL_CODEC_AVS, "Creature Shock AVS"),

    B(HEIMDALL_CODEC_DSICINVIDEO, "Delphine CIN video"),

    B(HEIMDALL_CODEC_TIERTEXSEQVIDEO, "Tiertex Limited SEQ video"),

    B(HEIMDALL_CODEC_DXA, "Feeble Files/ScummVM DXA"),

    B(HEIMDALL_CODEC_C93, "Interplay C93"),

    B(HEIMDALL_CODEC_BETHSOFTVID, "Bethesda VID"),

    B(HEIMDALL_CODEC_VB, "Beam Software VB"),

    B(HEIMDALL_CODEC_RL2, "RL2 video"),

    B(HEIMDALL_CODEC_BFI, "Brute Force & Ignorance (BFI) video"),

    B(HEIMDALL_CODEC_CMV, "Electronic Arts CMV"),

    B(HEIMDALL_CODEC_MOTIONPIXELS, "Sirius Publishing Motion Pixels"),

    B(HEIMDALL_CODEC_TGV, "Electronic Arts TGV"),

    B(HEIMDALL_CODEC_TGQ, "Electronic Arts TGQ"),

    B(HEIMDALL_CODEC_TQI, "Electronic Arts TQI"),

    B(HEIMDALL_CODEC_MAD, "Electronic Arts MAD"),

    B(HEIMDALL_CODEC_ANM, "DeluxePaint animation"),

    B(HEIMDALL_CODEC_YOP, "Psygnosis YOP"),

    B(HEIMDALL_CODEC_JV, "Bitmap Brothers JV"),

    B(HEIMDALL_CODEC_DFA, "Chronomaster DFA"),

    B(HEIMDALL_CODEC_HNM4_VIDEO, "Cryo Interactive Entertainment HNM4"),

    B(HEIMDALL_CODEC_CINEFORM, "CineForm" ),

    B(HEIMDALL_CODEC_SPEEDHQ, "NewTek SpeedHQ" ),
        A("SHQ0"),
        A("SHQ1"),
        A("SHQ2"),
        A("SHQ3"),
        A("SHQ4"),
        A("SHQ5"),
        A("SHQ7"),
        A("SHQ9"),

    B(HEIMDALL_CODEC_PIXLET, "Apple Pixlet" ),
        A("pxlt"),

    B(HEIMDALL_CODEC_MAGICYUV, "MagicYUV lossless" ),
        A("M8RG"),
        A("M8RA"),
        A("M8G0"),
        A("M8Y0"),
        A("M8Y2"),
        A("M8Y4"),
        A("M8YA"),

    B(HEIMDALL_CODEC_IMM4, "Infinity IMM4"),
        A("IMM4"),

    B(HEIMDALL_CODEC_IMM5, "Infinity IMM5"),
        A("IMM5"),

    B(HEIMDALL_CODEC_VDPAU_VIDEO, "VDPAU video"),

    B(HEIMDALL_CODEC_VDPAU_OUTPUT, "VDPAU output"),

    B(HEIMDALL_CODEC_VAAPI_420, "4:2:0 VAAPI opaque"),
        A("VAOP"),

    B(HEIMDALL_CODEC_VAAPI_420_10BPP, "4:2:0 10bits VAAPI opaque"),
        A("VAO0"),

    B(HEIMDALL_CODEC_VAAPI_420_12BPP, "4:2:0 12bits VAAPI opaque"),
        A("VAO2"),

    B(HEIMDALL_CODEC_VAAPI_422_10BPP, "4:2:2 10bits VAAPI opaque"),
        A("VA20"),

    B(HEIMDALL_CODEC_VAAPI_422_12BPP, "4:2:2 12bits VAAPI opaque"),
        A("VA22"),

    B(HEIMDALL_CODEC_VAAPI_444,       "4:4:4 8bits VAAPI opaque"),
        A("VA4P"),

    B(HEIMDALL_CODEC_VAAPI_444_10BPP, "4:4:4 10bits VAAPI opaque"),
        A("VA40"),

    B(HEIMDALL_CODEC_VAAPI_444_12BPP, "4:4:4 12bits VAAPI opaque"),
        A("VA42"),

    B(HEIMDALL_CODEC_ANDROID_OPAQUE, "Android opaque"),
        A("ANOP"),

    B(HEIMDALL_CODEC_MMAL_OPAQUE, "MMAL opaque"),
        A("MMAL"),

    B(HEIMDALL_CODEC_D3D9_OPAQUE, "4:2:0 D3D9 opaque"),
        A("DXA9"),

    B(HEIMDALL_CODEC_D3D9_OPAQUE_10B, "4:2:0 10bits D3D9 opaque"),
        A("DXA0"),

    B(HEIMDALL_CODEC_D3D11_OPAQUE, "4:2:0 D3D11 opaque"),
        A("DX11"),

    B(HEIMDALL_CODEC_D3D11_OPAQUE_10B, "4:2:0 10bits D3D11 opaque"),
        A("DX10"),

    B(HEIMDALL_CODEC_D3D11_OPAQUE_RGBA, "RGBA D3D11 opaque"),
        A("DXRG"),

    B(HEIMDALL_CODEC_D3D11_OPAQUE_BGRA, "BGRA D3D11 opaque"),
        A("DAGR"),

    B(HEIMDALL_CODEC_D3D11_OPAQUE_ALPHA, "4:2:0 D3D11 opaque with alpha"),
        A("DA11"),

    B(HEIMDALL_CODEC_NVDEC_OPAQUE, "4:2:0 NVDEC opaque"),
        A("NVD8"),

    B(HEIMDALL_CODEC_NVDEC_OPAQUE_10B, "4:2:0 10bits NVDEC opaque"),
        A("NVD0"),

    B(HEIMDALL_CODEC_NVDEC_OPAQUE_16B, "4:2:0 16bits NVDEC opaque"),
        A("NVD6"),

    B(HEIMDALL_CODEC_NVDEC_OPAQUE_444, "4:4:4 NVDEC opaque"),
        A("NV48"),

    B(HEIMDALL_CODEC_NVDEC_OPAQUE_444_16B, "4:4:4 16bits NVDEC opaque"),
        A("NV46"),

    B(HEIMDALL_CODEC_AGM, "AmuseGraphics Movie"),
        A("AGM0"),
        A("AGM1"),
        A("AGM2"),
        A("AGM3"),
        A("AGM4"),
        A("AGM5"),
        A("AGM6"),
        A("AGM7"),

    B(HEIMDALL_CODEC_NOTCHLC, "NotchLC"),

    B(HEIMDALL_CODEC_RTP_VIDEO_RAW, "RTP raw video"),

    B(HEIMDALL_CODEC_APV, "Advanced Professional Video (APV)"),
};

static const staticentry_t p_list_audio[] = {

    /* Windows Media Audio 1 */
    B(HEIMDALL_CODEC_WMA1, "Windows Media Audio 1"),
        A("WMA1"),
        A("wma1"),

    /* Windows Media Audio 2 */
    B(HEIMDALL_CODEC_WMA2, "Windows Media Audio 2"),
        A("WMA2"),
        A("wma2"),
        A("wma "),

    /* Windows Media Audio Professional */
    B(HEIMDALL_CODEC_WMAP, "Windows Media Audio Professional"),
        A("WMAP"),
        A("wmap"),

    /* Windows Media Audio Lossless */
    B(HEIMDALL_CODEC_WMAL, "Windows Media Audio Lossless"),
        A("WMAL"),
        A("wmal"),

    /* Windows Media Audio Speech */
    B(HEIMDALL_CODEC_WMAS, "Windows Media Audio Voice (Speech)"),
        A("WMAS"),
        A("wmas"),

    /* DV Audio */
    B(HEIMDALL_CODEC_DVAUDIO, "DV Audio"),
        A("dvau"),
        A("vdva"),
        A("dvca"),
        A("RADV"),

    /* MACE-3 Audio */
    B(HEIMDALL_CODEC_MACE3, "MACE-3 Audio"),
        A("MAC3"),

    /* MACE-6 Audio */
    B(HEIMDALL_CODEC_MACE6, "MACE-6 Audio"),
        A("MAC6"),

    /* MUSEPACK7 Audio */
    B(HEIMDALL_CODEC_MUSEPACK7, "MUSEPACK7 Audio"),
        A("MPC "),

    /* MUSEPACK8 Audio */
    B(HEIMDALL_CODEC_MUSEPACK8, "MUSEPACK8 Audio"),
        A("MPCK"),
        A("MPC8"),

    /* RealAudio 1.0 */
    B(HEIMDALL_CODEC_RA_144, "RealAudio 1.0"),
        A("14_4"),
        A("lpcJ"),

    /* RealAudio 2.0 */
    B(HEIMDALL_CODEC_RA_288, "RealAudio 2.0"),
        A("28_8"),

    B(HEIMDALL_CODEC_SIPR, "RealAudio Sipr"),
        A("sipr"),

    /* MPEG Audio layer 1/2 */
    B(HEIMDALL_CODEC_MPGA, "MPEG Audio layer 1/2"),
        A("mpga"),
        A("mp2a"),
        A(".mp1"),
        A(".mp2"),
        A("LAME"),
        A("ms\x00\x50"),
        A("ms\x00\x55"),

    /* MPEG Audio layer 3 */
    B(HEIMDALL_CODEC_MP3, "MPEG Audio layer 3"),
        A("mp3 "),
        A(".mp3"),
        A("MP3 "),

    /* A52 Audio (aka AC3) */
    B(HEIMDALL_CODEC_A52, "Audio Coding 3 (AC-3)"),
        A("a52 "),
        A("a52b"),
        A("ac-3"),
        A("sac3"),
        A("ms\x20\x00"),

    B(HEIMDALL_CODEC_EAC3, "Enhanced Audio Coding 3 (E-AC-3)"),
        A("ec-3"),

    B(HEIMDALL_CODEC_AC4,  "Audio Coding 4 (AC-4)"),

    /* DTS Audio */
    B(HEIMDALL_CODEC_DTS, "DTS Audio"),
        A("dts "),
        A("DTS "),
        A("dtsb"),
        A("dtsc"),
        E("dtse", "DTS Express"),
        E("dtsh", "DTS-HD High Resolution Audio"),
        E("dtsl", "DTS-HD Lossless"),
        A("ms\x20\x01"),

    /* AAC audio */
    B(HEIMDALL_CODEC_MP4A, "MPEG AAC Audio"),
        A("mp4a"),
        A("aac "),
        A("AACL"),
        A("AACH"),
        A("AACP"), /* smooth streaming alias */

    B(HEIMDALL_CODEC_MPEGH, "MPEG-H Audio"),
        A("mha1"),
        A("mha2"),
        A("mhm1"),
        A("mhm2"),

    /* ALS audio */
    B(HEIMDALL_CODEC_ALS, "MPEG-4 Audio Lossless (ALS)"),
        A("als "),

    /* 4X Technologies */
    B(HEIMDALL_CODEC_ADPCM_4XM, "4X Technologies Audio"),
        A("4xma"),

    /* EA ADPCM */
    B(HEIMDALL_CODEC_ADPCM_EA, "EA ADPCM Audio"),
        A("ADEA"),

    /* Interplay DPCM */
    B(HEIMDALL_CODEC_INTERPLAY_DPCM, "Interplay DPCM Audio"),
        A("idpc"),

    /* Id RoQ */
    B(HEIMDALL_CODEC_ROQ_DPCM, "Id RoQ DPCM Audio"),
        A("RoQa"),

    /* DCIN Audio */
    B(HEIMDALL_CODEC_DSICINAUDIO, "Delphine CIN Audio"),
        A("DCIA"),

    /* Sony Playstation XA ADPCM */
    B(HEIMDALL_CODEC_ADPCM_XA, "PSX XA ADPCM Audio"),
        A("xa  "),

    /* ADX ADPCM */
    B(HEIMDALL_CODEC_ADPCM_ADX, "ADX ADPCM Audio"),
        A("adx "),

    /* Westwood ADPCM */
    B(HEIMDALL_CODEC_ADPCM_IMA_WS, "Westwood IMA ADPCM audio"),
        A("AIWS"),

    /* MS ADPCM */
    B(HEIMDALL_CODEC_ADPCM_MS, "MS ADPCM audio"),
        A("ms\x00\x02"),

    /* Sierra VMD */
    B(HEIMDALL_CODEC_VMDAUDIO, "Sierra VMD Audio"),
        A("vmda"),

    /* G.726 ADPCM */
    B(HEIMDALL_CODEC_ADPCM_G726, "G.726 ADPCM Audio"),
        A("g726"),

    /* G.726 ADPCM */
    B(HEIMDALL_CODEC_ADPCM_G726_LE, "G.726 ADPCM Audio (little-endian)"),
        A("g72s"),

    /* G.722 ADPCM */
    B(HEIMDALL_CODEC_ADPCM_G722, "G.722 ADPCM Audio"),
        A("g722"),

    /* Flash ADPCM */
    B(HEIMDALL_CODEC_ADPCM_SWF, "Flash ADPCM Audio"),
        A("SWFa"),

    B(HEIMDALL_CODEC_ADPCM_IMA_WAV, "IMA WAV ADPCM Audio"),
        A("ms\x00\x11"),

    B(HEIMDALL_CODEC_ADPCM_IMA_AMV, "IMA AMV ADPCM Audio"),
        A("imav"),

    B(HEIMDALL_CODEC_ADPCM_IMA_QT, "IMA QT ADPCM Audio"),
        A("ima4"),

    B(HEIMDALL_CODEC_ADPCM_YAMAHA, "Yamaha ADPCM Audio" ),
        A("ms\x00\x20"),

    B(HEIMDALL_CODEC_ADPCM_DK3, "Duck DK3 ADPCM"),
        A("ms\x00\x62"),

    B(HEIMDALL_CODEC_ADPCM_DK4, "Duck DK4 ADPCM"),
        A("ms\x00\x61"),

    B(HEIMDALL_CODEC_ADPCM_THP, "GameCube THP ADPCM"),

    B(HEIMDALL_CODEC_ADPCM_XA_EA, "EA-XA ADPCM"),
        A("XAJ\x00"),

    /* AMR */
    B(HEIMDALL_CODEC_AMR_NB, "AMR narrow band"),
        A("samr"),

    B(HEIMDALL_CODEC_AMR_WB, "AMR wide band"),
        A("sawb"),

    /* FLAC */
    B(HEIMDALL_CODEC_FLAC, "FLAC (Free Lossless Audio Codec)"),
        A("flac"),

    /* ALAC */
    B(HEIMDALL_CODEC_ALAC, "Apple Lossless Audio Codec"),
        A("alac"),

    /* QDM2 */
    B(HEIMDALL_CODEC_QDM2, "QDM2 Audio"),
        A("QDM2"),

    /* QDMC */
    B(HEIMDALL_CODEC_QDMC, "QDMC Audio"),
        A("QDMC"),

    /* QOA */
    B(HEIMDALL_CODEC_QOA, "Quite OK Audio Format"),
        A("qoaf"),

    /* COOK */
    B(HEIMDALL_CODEC_COOK, "Cook Audio"),
        A("cook"),

    /* TTA: The Lossless True Audio */
    B(HEIMDALL_CODEC_TTA, "The Lossless True Audio"),
        A("TTA1"),

    /* Shorten */
    B(HEIMDALL_CODEC_SHORTEN, "Shorten Lossless Audio"),
        A("shn "),
        A("shrn"),

    B(HEIMDALL_CODEC_WAVPACK, "WavPack"),
        A("WVPK"),
        A("wvpk"),

    B(HEIMDALL_CODEC_GSM, "GSM Audio"),
        A("gsm "),

    B(HEIMDALL_CODEC_GSM_MS, "Microsoft GSM Audio"),
        A("agsm"),

    B(HEIMDALL_CODEC_ATRAC1, "atrac 1"),
        A("atr1"),

    B(HEIMDALL_CODEC_ATRAC3, "atrac 3"),
        A("atrc"),
        A("\x70\x02\x00\x00"),

    B(HEIMDALL_CODEC_ATRAC3P, "atrac 3+"),
        A("atrp"),

    B(HEIMDALL_CODEC_ATRAC3AL, "atrac 3 advanced lossless"),
        A("atrl"),

    B(HEIMDALL_CODEC_ATRAC3PAL, "atrac 3+ advanced lossless"),
        A("atpl"),

    B(HEIMDALL_CODEC_ATRAC9, "atrac 9"),
        A("atr9"),

    B(HEIMDALL_CODEC_IMC, "IMC" ),
        A("\x01\x04\x00\x00"),

    B(HEIMDALL_CODEC_TRUESPEECH,"TrueSpeech"),
        A("\x22\x00\x00\x00"),

    B(HEIMDALL_CODEC_NELLYMOSER, "NellyMoser ASAO"),
        A("NELL"),
        A("nmos"),

    B(HEIMDALL_CODEC_APE, "Monkey's Audio"),
        A("APE "),

    B(HEIMDALL_CODEC_MLP, "MLP/TrueHD Audio"),
        A("mlp "),

    B(HEIMDALL_CODEC_TRUEHD, "TrueHD Audio"),
        A("trhd"),

    B(HEIMDALL_CODEC_QCELP, "QCELP Audio"),
        A("Qclp"),
        A("Qclq"),
        A("sqcp"),

    B(HEIMDALL_CODEC_CODEC2, "Codec 2 Audio"),

    B(HEIMDALL_CODEC_SPEEX, "Speex Audio"),
        A("spx "),
        A("spxr"),

    B(HEIMDALL_CODEC_VORBIS, "Vorbis Audio"),
        A("vorb"),
        A("vor1"),

    B(HEIMDALL_CODEC_OPUS, "Opus Audio"),
        A("Opus"),
        A("opus"),

    B(HEIMDALL_CODEC_302M, "302M Audio"),
        A("302m"),

    B(HEIMDALL_CODEC_DVD_LPCM, "DVD LPCM Audio"),
        A("lpcm"),

    B(HEIMDALL_CODEC_DVDA_LPCM, "DVD-Audio LPCM Audio"),
        A("apcm"),

    B(HEIMDALL_CODEC_BD_LPCM, "BD LPCM Audio"),
        A("bpcm"),

    B(HEIMDALL_CODEC_SDDS, "SDDS Audio"),
        A("sdds"),
        A("sddb"),

    B(HEIMDALL_CODEC_MIDI, "MIDI Audio"),
        A("MIDI"),

    B(HEIMDALL_CODEC_RALF, "RealAudio Lossless"),
        A("LSD:"),

    /* G.723.1 */
    B(HEIMDALL_CODEC_G723_1, "G.723.1 Audio"),
        A("g72\x31"),

    /* PCM */
    B(HEIMDALL_CODEC_S8, "PCM S8"),
        A("s8  "),

    B(HEIMDALL_CODEC_U8, "PCM U8"),
        A("u8  "),

    B(HEIMDALL_CODEC_S16L, "PCM S16 LE"),
        A("s16l"),

    B(HEIMDALL_CODEC_S16L_PLANAR, "PCM S16 LE planar"),

    B(HEIMDALL_CODEC_S16B, "PCM S16 BE"),
        A("s16b"),

    B(HEIMDALL_CODEC_U16L, "PCM U16 LE"),
        A("u16l"),

    B(HEIMDALL_CODEC_U16B, "PCM U16 BE"),
        A("u16b"),

    B(HEIMDALL_CODEC_S20B, "PCM S20 BE"),

    B(HEIMDALL_CODEC_S24L, "PCM S24 LE"),
        A("s24l"),
        A("42ni"),  /* Quicktime */

    B(HEIMDALL_CODEC_S24B, "PCM S24 BE"),
        A("s24b"),
        A("in24"),  /* Quicktime */

    B(HEIMDALL_CODEC_U24L, "PCM U24 LE"),
        A("u24l"),

    B(HEIMDALL_CODEC_U24B, "PCM U24 BE"),
        A("u24b"),

    B(HEIMDALL_CODEC_S24L32, "PCM S24 in 32 LE"),

    B(HEIMDALL_CODEC_S24B32, "PCM S24 in 32 BE"),

    B(HEIMDALL_CODEC_S32L, "PCM S32 LE"),
        A("s32l"),
        A("23ni"),  /* Quicktime */

    B(HEIMDALL_CODEC_S32B, "PCM S32 BE"),
        A("s32b"),
        A("in32"),  /* Quicktime */

    B(HEIMDALL_CODEC_U32L, "PCM U32 LE"),
        A("u32l"),

    B(HEIMDALL_CODEC_U32B, "PCM U32 BE"),
        A("u32b"),

    B(HEIMDALL_CODEC_ALAW, "PCM ALAW"),
        A("alaw"),

    B(HEIMDALL_CODEC_MULAW, "PCM MU-LAW"),
        A("mlaw"),
        A("ulaw"),

    B(HEIMDALL_CODEC_DAT12, "12 bits DAT audio"),

    B(HEIMDALL_CODEC_S24DAUD, "PCM DAUD"),
        A("daud"),

    B(HEIMDALL_CODEC_F32L, "32 bits float LE"),
        A("f32l"),
        A("fl32"),

    B(HEIMDALL_CODEC_F32B, "32 bits float BE"),
        A("f32b"),

    B(HEIMDALL_CODEC_F64L, "64 bits float LE"),
        A("f64l"),

    B(HEIMDALL_CODEC_F64B, "64 bits float BE"),
        A("f64b"),

    B(HEIMDALL_CODEC_TWINVQ, "TwinVQ"),
        A("TWIN"),

    B(HEIMDALL_CODEC_BMVAUDIO, "Discworld II BMV audio"),
        A("BMVA"),

    B(HEIMDALL_CODEC_ULEAD_DV_AUDIO_NTSC, "Ulead DV audio NTSC"),
        A("ms\x02\x15"),
    B(HEIMDALL_CODEC_ULEAD_DV_AUDIO_PAL, "Ulead DV audio PAL"),
        A("ms\x02\x16"),

    B(HEIMDALL_CODEC_INDEO_AUDIO, "Indeo Audio Coder"),
        A("ms\x04\x02"),

    B(HEIMDALL_CODEC_TAK, "TAK (Tom's lossless Audio Kompressor)"),

    B(HEIMDALL_CODEC_SMACKAUDIO, "Smacker audio"),

    B(HEIMDALL_CODEC_ADPCM_IMA_EA_SEAD, "ADPCM IMA Electronic Arts SEAD"),

    B(HEIMDALL_CODEC_ADPCM_EA_R1, "ADPCM Electronic Arts R1"),

    B(HEIMDALL_CODEC_ADPCM_EA_R3, "ADPCM Electronic Arts R3"),

    B(HEIMDALL_CODEC_ADPCM_IMA_APC, "ADPCM APC"),

    B(HEIMDALL_CODEC_DSD_LSBF, "DSD (Direct Stream Digital) LSB first"),

    B(HEIMDALL_CODEC_DSD_MSBF, "DSD (Direct Stream Digital) MSB first"),

    B(HEIMDALL_CODEC_DSD_LSBF_PLANAR, "DSD (Direct Stream Digital) LSB first, planar"),

    B(HEIMDALL_CODEC_DSD_MSBF_PLANAR, "DSD (Direct Stream Digital) MSB first, planar"),

    B(HEIMDALL_CODEC_DFPWM, "DFPWM1a audio"),
};

static const staticentry_t p_list_spu[] = {

    B(HEIMDALL_CODEC_SPU, "DVD Subtitles"),
        A("spu "),
        A("spub"),

    B(HEIMDALL_CODEC_DVBS, "DVB Subtitles"),
        A("dvbs"),

    B(HEIMDALL_CODEC_SUBT, "Text subtitles with various tags"),
        A("subt"),

    B(HEIMDALL_CODEC_XSUB, "DivX XSUB subtitles"),
        A("XSUB"),
        A("xsub"),
        A("DXSB"),
        A("DXSA"),

    B(HEIMDALL_CODEC_SSA, "SubStation Alpha subtitles"),
        A("ssa "),

    B(HEIMDALL_CODEC_TEXT, "Plain text subtitles"),
        A("TEXT"),

    B(HEIMDALL_CODEC_TELETEXT, "Teletext"),
        A("telx"),

    B(HEIMDALL_CODEC_KATE, "Kate subtitles"),
        A("kate"),

    B(HEIMDALL_CODEC_CMML, "CMML annotations/metadata"),
        A("cmml"),

    B(HEIMDALL_CODEC_ITU_T140, "ITU T.140 subtitles"),
        A("t140"),

    B(HEIMDALL_CODEC_USF, "USF subtitles"),
        A("usf "),

    B(HEIMDALL_CODEC_OGT, "OGT subtitles"),
        A("ogt "),

    B(HEIMDALL_CODEC_CVD, "CVD subtitles"),
        A("cvd "),

    B(HEIMDALL_CODEC_ARIB_A, "ARIB subtitles (A-profile)"),
        A("arba"),

    B(HEIMDALL_CODEC_ARIB_C, "ARIB subtitles (C-profile)"),
        A("arbc"),

    B(HEIMDALL_CODEC_BD_PG, "BD PGS subtitles"),
        A("bdpg"),

    B(HEIMDALL_CODEC_BD_TEXT, "BD Text subtitles"),
        A("bdtx"),

    B(HEIMDALL_CODEC_EBU_STL, "EBU STL subtitles"),
        A("STL "),

    B(HEIMDALL_CODEC_SCTE_27, "SCTE-27 subtitles"),
        A("SC27"),

    B(HEIMDALL_CODEC_CEA608,  "EIA-608 subtitles"),
        A("cc1 "), /* acquisition devices */
        A("cc2 "),
        A("cc3 "),
        A("cc4 "),

    B(HEIMDALL_CODEC_TTML, "TTML subtitles"),
        A("ttml"),

    B(HEIMDALL_CODEC_TTML_TS, "TTML subtitles (EN.303.560)"),

    B(HEIMDALL_CODEC_WEBVTT, "WEBVTT subtitles"),
};
