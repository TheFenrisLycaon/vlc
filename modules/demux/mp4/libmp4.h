/*****************************************************************************
 * libmp4.h : LibMP4 library for mp4 module for heimdall
 *****************************************************************************
 * Copyright (C) 2001-2004, 2010 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
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
#ifndef HEIMDALL_MP4_LIBMP4_H_
#define HEIMDALL_MP4_LIBMP4_H_

#include <heimdall_es.h>
#include <heimdall_codecs.h>
#include "coreaudio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Use alias for scaled time */
typedef int64_t stime_t;

#define BLOCK16x16 (1<<16)

#define BRAND_3gp4 HEIMDALL_FOURCC( '3', 'g', 'p', '4' )
#define BRAND_3gp5 HEIMDALL_FOURCC( '3', 'g', 'p', '5' )
#define BRAND_3gp6 HEIMDALL_FOURCC( '3', 'g', 'p', '6' )
#define BRAND_3gp7 HEIMDALL_FOURCC( '3', 'g', 'p', '7' )
#define BRAND_isml HEIMDALL_FOURCC( 'i', 's', 'm', 'l' )
#define BRAND_isom HEIMDALL_FOURCC( 'i', 's', 'o', 'm' )
#define BRAND_iso2 HEIMDALL_FOURCC( 'i', 's', 'o', '2' )
#define BRAND_iso6 HEIMDALL_FOURCC( 'i', 's', 'o', '6' )
#define BRAND_qt__ HEIMDALL_FOURCC( 'q', 't', ' ', ' ' )
#define BRAND_f4v  HEIMDALL_FOURCC( 'f', '4', 'v', ' ' ) /* Adobe Flash */
#define BRAND_dash HEIMDALL_FOURCC( 'd', 'a', 's', 'h' )
#define BRAND_smoo HEIMDALL_FOURCC( 's', 'm', 'o', 'o' ) /* Internal use */
#define BRAND_mp41 HEIMDALL_FOURCC( 'm', 'p', '4', '1' )
#define BRAND_av01 HEIMDALL_FOURCC( 'a', 'v', '0', '1' )
#define BRAND_avc1 HEIMDALL_FOURCC( 'a', 'v', 'c', '1' )
#define BRAND_M4A  HEIMDALL_FOURCC( 'M', '4', 'A', ' ' )
#define BRAND_piff HEIMDALL_FOURCC( 'p', 'i', 'f', 'f' )
#define BRAND_mif1 HEIMDALL_FOURCC( 'm', 'i', 'f', '1' ) /* heif */
#define BRAND_msf1 HEIMDALL_FOURCC( 'm', 's', 'f', '1' ) /* heif */
#define BRAND_heic HEIMDALL_FOURCC( 'h', 'e', 'i', 'c' ) /* heif */
#define BRAND_heix HEIMDALL_FOURCC( 'h', 'e', 'i', 'x' ) /* heif */
#define BRAND_hevc HEIMDALL_FOURCC( 'h', 'e', 'v', 'c' ) /* heif */
#define BRAND_hevx HEIMDALL_FOURCC( 'h', 'e', 'v', 'x' ) /* heif */
#define BRAND_jpeg HEIMDALL_FOURCC( 'j', 'p', 'e', 'g' ) /* heif */
#define BRAND_jpgs HEIMDALL_FOURCC( 'j', 'p', 'g', 's' ) /* heif */
#define BRAND_avci HEIMDALL_FOURCC( 'a', 'v', 'c', 'i' ) /* heif */
#define BRAND_avcs HEIMDALL_FOURCC( 'a', 'v', 'c', 's' ) /* heif */
#define BRAND_avif HEIMDALL_FOURCC( 'a', 'v', 'i', 'f' ) /* heif AV1 AVIF */
#define BRAND_avis HEIMDALL_FOURCC( 'a', 'v', 'i', 's' ) /* heig AV1 AVIF */

#define ATOM_root HEIMDALL_FOURCC( 'r', 'o', 'o', 't' )
#define ATOM_uuid HEIMDALL_FOURCC( 'u', 'u', 'i', 'd' )

#define ATOM_ftyp HEIMDALL_FOURCC( 'f', 't', 'y', 'p' )
#define ATOM_moov HEIMDALL_FOURCC( 'm', 'o', 'o', 'v' )
#define ATOM_foov HEIMDALL_FOURCC( 'f', 'o', 'o', 'v' )
#define ATOM_cmov HEIMDALL_FOURCC( 'c', 'm', 'o', 'v' )
#define ATOM_dcom HEIMDALL_FOURCC( 'd', 'c', 'o', 'm' )
#define ATOM_cmvd HEIMDALL_FOURCC( 'c', 'm', 'v', 'd' )

#define ATOM_styp HEIMDALL_FOURCC( 's', 't', 'y', 'p' )
#define ATOM_moof HEIMDALL_FOURCC( 'm', 'o', 'o', 'f' )
#define ATOM_mdat HEIMDALL_FOURCC( 'm', 'd', 'a', 't' )
#define ATOM_skip HEIMDALL_FOURCC( 's', 'k', 'i', 'p' )
#define ATOM_free HEIMDALL_FOURCC( 'f', 'r', 'e', 'e' )
#define ATOM_udta HEIMDALL_FOURCC( 'u', 'd', 't', 'a' )
#define ATOM_wide HEIMDALL_FOURCC( 'w', 'i', 'd', 'e' )
#define ATOM_binm HEIMDALL_FOURCC( 0x82, 0x82, 0x7f, 0x7d ) /* binary Computer Graphics Metafile */

/* Quicktime preview */
#define ATOM_pnot HEIMDALL_FOURCC( 'p', 'n', 'o', 't' )
#define ATOM_pict HEIMDALL_FOURCC( 'p', 'i', 'c', 't' )
#define ATOM_PICT HEIMDALL_FOURCC( 'P', 'I', 'C', 'T' )

#define ATOM_data HEIMDALL_FOURCC( 'd', 'a', 't', 'a' )

#define ATOM_trak HEIMDALL_FOURCC( 't', 'r', 'a', 'k' )
#define ATOM_mvhd HEIMDALL_FOURCC( 'm', 'v', 'h', 'd' )
#define ATOM_tkhd HEIMDALL_FOURCC( 't', 'k', 'h', 'd' )
#define ATOM_tref HEIMDALL_FOURCC( 't', 'r', 'e', 'f' )
#define ATOM_load HEIMDALL_FOURCC( 'l', 'o', 'a', 'd' )
#define ATOM_mdia HEIMDALL_FOURCC( 'm', 'd', 'i', 'a' )
#define ATOM_mdhd HEIMDALL_FOURCC( 'm', 'd', 'h', 'd' )
#define ATOM_hdlr HEIMDALL_FOURCC( 'h', 'd', 'l', 'r' )
#define ATOM_minf HEIMDALL_FOURCC( 'm', 'i', 'n', 'f' )
#define ATOM_vmhd HEIMDALL_FOURCC( 'v', 'm', 'h', 'd' )
#define ATOM_smhd HEIMDALL_FOURCC( 's', 'm', 'h', 'd' )
#define ATOM_hmhd HEIMDALL_FOURCC( 'h', 'm', 'h', 'd' )
#define ATOM_dinf HEIMDALL_FOURCC( 'd', 'i', 'n', 'f' )
#define ATOM_url  HEIMDALL_FOURCC( 'u', 'r', 'l', ' ' )
#define ATOM_urn  HEIMDALL_FOURCC( 'u', 'r', 'n', ' ' )
#define ATOM_dref HEIMDALL_FOURCC( 'd', 'r', 'e', 'f' )
#define ATOM_stbl HEIMDALL_FOURCC( 's', 't', 'b', 'l' )
#define ATOM_stts HEIMDALL_FOURCC( 's', 't', 't', 's' )
#define ATOM_ctts HEIMDALL_FOURCC( 'c', 't', 't', 's' )
#define ATOM_cslg HEIMDALL_FOURCC( 'c', 's', 'l', 'g' )
#define ATOM_stsd HEIMDALL_FOURCC( 's', 't', 's', 'd' )
#define ATOM_stsz HEIMDALL_FOURCC( 's', 't', 's', 'z' )
#define ATOM_stz2 HEIMDALL_FOURCC( 's', 't', 'z', '2' )
#define ATOM_stsc HEIMDALL_FOURCC( 's', 't', 's', 'c' )
#define ATOM_stco HEIMDALL_FOURCC( 's', 't', 'c', 'o' )
#define ATOM_co64 HEIMDALL_FOURCC( 'c', 'o', '6', '4' )
#define ATOM_sbgp HEIMDALL_FOURCC( 's', 'b', 'g', 'p' )
#define ATOM_sgpd HEIMDALL_FOURCC( 's', 'g', 'p', 'd' )
#define ATOM_stss HEIMDALL_FOURCC( 's', 't', 's', 's' )
#define ATOM_stsh HEIMDALL_FOURCC( 's', 't', 's', 'h' )
#define ATOM_stdp HEIMDALL_FOURCC( 's', 't', 'd', 'p' )
#define ATOM_edts HEIMDALL_FOURCC( 'e', 'd', 't', 's' )
#define ATOM_elst HEIMDALL_FOURCC( 'e', 'l', 's', 't' )
#define ATOM_mvex HEIMDALL_FOURCC( 'm', 'v', 'e', 'x' )
#define ATOM_sdtp HEIMDALL_FOURCC( 's', 'd', 't', 'p' )
#define ATOM_trex HEIMDALL_FOURCC( 't', 'r', 'e', 'x' )
#define ATOM_mehd HEIMDALL_FOURCC( 'm', 'e', 'h', 'd' )
#define ATOM_mfhd HEIMDALL_FOURCC( 'm', 'f', 'h', 'd' )
#define ATOM_traf HEIMDALL_FOURCC( 't', 'r', 'a', 'f' )
#define ATOM_sidx HEIMDALL_FOURCC( 's', 'i', 'd', 'x' )
#define ATOM_tfhd HEIMDALL_FOURCC( 't', 'f', 'h', 'd' )
#define ATOM_tfdt HEIMDALL_FOURCC( 't', 'f', 'd', 't' )
#define ATOM_trun HEIMDALL_FOURCC( 't', 'r', 'u', 'n' )
#define ATOM_cprt HEIMDALL_FOURCC( 'c', 'p', 'r', 't' )
#define ATOM_iods HEIMDALL_FOURCC( 'i', 'o', 'd', 's' )
#define ATOM_pasp HEIMDALL_FOURCC( 'p', 'a', 's', 'p' )
#define ATOM_clap HEIMDALL_FOURCC( 'c', 'l', 'a', 'p' )
#define ATOM_mfra HEIMDALL_FOURCC( 'm', 'f', 'r', 'a' )
#define ATOM_mfro HEIMDALL_FOURCC( 'm', 'f', 'r', 'o' )
#define ATOM_tfra HEIMDALL_FOURCC( 't', 'f', 'r', 'a' )
#define ATOM_keys HEIMDALL_FOURCC( 'k', 'e', 'y', 's' )

#define ATOM_st3d HEIMDALL_FOURCC( 's', 't', '3', 'd' )
#define ATOM_sv3d HEIMDALL_FOURCC( 's', 'v', '3', 'd' )
#define ATOM_proj HEIMDALL_FOURCC( 'p', 'r', 'o', 'j' )
#define ATOM_prhd HEIMDALL_FOURCC( 'p', 'r', 'h', 'd' )
#define ATOM_cbmp HEIMDALL_FOURCC( 'c', 'b', 'm', 'p' )
#define ATOM_equi HEIMDALL_FOURCC( 'e', 'q', 'u', 'i' )

#define ATOM_nmhd HEIMDALL_FOURCC( 'n', 'm', 'h', 'd' )
#define ATOM_mp2v HEIMDALL_FOURCC( 'm', 'p', '2', 'v' )
#define ATOM_mp4v HEIMDALL_FOURCC( 'm', 'p', '4', 'v' )
#define ATOM_mp4a HEIMDALL_FOURCC( 'm', 'p', '4', 'a' )
#define ATOM_mp4s HEIMDALL_FOURCC( 'm', 'p', '4', 's' )
#define ATOM_vide HEIMDALL_FOURCC( 'v', 'i', 'd', 'e' )
#define ATOM_soun HEIMDALL_FOURCC( 's', 'o', 'u', 'n' )
#define ATOM_hint HEIMDALL_FOURCC( 'h', 'i', 'n', 't' )
#define ATOM_hdv2 HEIMDALL_FOURCC( 'h', 'd', 'v', '2' )
#define ATOM_rrtp HEIMDALL_FOURCC( 'r', 'r', 't', 'p' )

#define ATOM_hnti HEIMDALL_FOURCC( 'h', 'n', 't', 'i' )
#define ATOM_rtp  HEIMDALL_FOURCC( 'r', 't', 'p', ' ' )
#define ATOM_btrt HEIMDALL_FOURCC( 'b', 't', 'r', 't' )
#define ATOM_sdp  HEIMDALL_FOURCC( 's', 'd', 'p', ' ' )

#define ATOM_tims HEIMDALL_FOURCC( 't', 'i', 'm', 's' )
#define ATOM_tsro HEIMDALL_FOURCC( 't', 's', 'r', 'o' )
#define ATOM_tssy HEIMDALL_FOURCC( 't', 's', 's', 'y' )

#define ATOM_esds HEIMDALL_FOURCC( 'e', 's', 'd', 's' )

#define ATOM_lpcm HEIMDALL_FOURCC( 'l', 'p', 'c', 'm' )
#define ATOM__mp3 HEIMDALL_FOURCC( '.', 'm', 'p', '3' )
#define ATOM_ms02 HEIMDALL_FOURCC( 'm', 's', 0x0, 0x02 )
#define ATOM_ms11 HEIMDALL_FOURCC( 'm', 's', 0x0, 0x11 )
#define ATOM_ms55 HEIMDALL_FOURCC( 'm', 's', 0x0, 0x55 )
#define ATOM_twos HEIMDALL_FOURCC( 't', 'w', 'o', 's' )
#define ATOM_sowt HEIMDALL_FOURCC( 's', 'o', 'w', 't' )
#define ATOM_QDMC HEIMDALL_FOURCC( 'Q', 'D', 'M', 'C' )
#define ATOM_QDM2 HEIMDALL_FOURCC( 'Q', 'D', 'M', '2' )
#define ATOM_XiFL HEIMDALL_FOURCC( 'X', 'i', 'F', 'L' )
#define ATOM_XiVs HEIMDALL_FOURCC( 'X', 'i', 'V', 's' )
#define ATOM_ima4 HEIMDALL_FOURCC( 'i', 'm', 'a', '4' )
#define ATOM_IMA4 HEIMDALL_FOURCC( 'I', 'M', 'A', '4' )
#define ATOM_dvi  HEIMDALL_FOURCC( 'd', 'v', 'i', ' ' )
#define ATOM_MAC3 HEIMDALL_FOURCC( 'M', 'A', 'C', '3' )
#define ATOM_MAC6 HEIMDALL_FOURCC( 'M', 'A', 'C', '6' )
#define ATOM_alaw HEIMDALL_FOURCC( 'a', 'l', 'a', 'w' )
#define ATOM_ulaw HEIMDALL_FOURCC( 'u', 'l', 'a', 'w' )
#define ATOM_Qclp HEIMDALL_FOURCC( 'Q', 'c', 'l', 'p' )
#define ATOM_samr HEIMDALL_FOURCC( 's', 'a', 'm', 'r' )
#define ATOM_sawb HEIMDALL_FOURCC( 's', 'a', 'w', 'b' )
#define ATOM_OggS HEIMDALL_FOURCC( 'O', 'g', 'g', 'S' )
#define ATOM_agsm HEIMDALL_FOURCC( 'a', 'g', 's', 'm' )
#define ATOM_alac HEIMDALL_FOURCC( 'a', 'l', 'a', 'c' )
#define ATOM_AC3  HEIMDALL_FOURCC( 'A', 'C', '-', '3' )
#define ATOM_ac3  HEIMDALL_FOURCC( 'a', 'c', '-', '3' )
#define ATOM_eac3 HEIMDALL_FOURCC( 'e', 'c', '-', '3' )
#define ATOM_dac3 HEIMDALL_FOURCC( 'd', 'a', 'c', '3' )
#define ATOM_dec3 HEIMDALL_FOURCC( 'd', 'e', 'c', '3' )
#define ATOM_ddts HEIMDALL_FOURCC( 'd', 'd', 't', 's' ) /* DTS formats */
#define ATOM_dtsc HEIMDALL_FOURCC( 'd', 't', 's', 'c' )
#define ATOM_dtsh HEIMDALL_FOURCC( 'd', 't', 's', 'h' )
#define ATOM_dtsl HEIMDALL_FOURCC( 'd', 't', 's', 'l' )
#define ATOM_dtse HEIMDALL_FOURCC( 'd', 't', 's', 'e' )
#define ATOM_dtsm HEIMDALL_FOURCC( 'd', 't', 's', '-' )
#define ATOM_dtsp HEIMDALL_FOURCC( 'd', 't', 's', '+' )
#define ATOM_vc1  HEIMDALL_FOURCC( 'v', 'c', '-', '1' )
#define ATOM_dvc1 HEIMDALL_FOURCC( 'd', 'v', 'c', '1' )
#define ATOM_WMA2 HEIMDALL_FOURCC( 'W', 'M', 'A', '2' )
#define ATOM_wma  HEIMDALL_FOURCC( 'w', 'm', 'a', ' ' )
#define ATOM_enda HEIMDALL_FOURCC( 'e', 'n', 'd', 'a' )
#define ATOM_gnre HEIMDALL_FOURCC( 'g', 'n', 'r', 'e' )
#define ATOM_trkn HEIMDALL_FOURCC( 't', 'r', 'k', 'n' )
#define ATOM_chan HEIMDALL_FOURCC( 'c', 'h', 'a', 'n' )
#define ATOM_srat HEIMDALL_FOURCC( 's', 'r', 'a', 't' )
#define ATOM_in24 HEIMDALL_FOURCC( 'i', 'n', '2', '4' )
#define ATOM_in32 HEIMDALL_FOURCC( 'i', 'n', '3', '2' )
#define ATOM_fl32 HEIMDALL_FOURCC( 'f', 'l', '3', '2' )
#define ATOM_fl64 HEIMDALL_FOURCC( 'f', 'l', '6', '4' )
#define ATOM_Opus HEIMDALL_FOURCC( 'O', 'p', 'u', 's' )
#define ATOM_fLaC HEIMDALL_FOURCC( 'f', 'L', 'a', 'C' )
#define ATOM_dfLa HEIMDALL_FOURCC( 'd', 'f', 'L', 'a' )

/* ISO-IEC 23003-5 */
#define ATOM_fpcm HEIMDALL_FOURCC( 'f', 'p', 'c', 'm' )
#define ATOM_ipcm HEIMDALL_FOURCC( 'i', 'p', 'c', 'm' )
#define ATOM_pcmC HEIMDALL_FOURCC( 'p', 'c', 'm', 'C' )

/* XiphQT */
#define ATOM_fCtS HEIMDALL_FOURCC( 'f', 'C', 't', 'S' )
#define ATOM_vCtH HEIMDALL_FOURCC( 'v', 'C', 't', 'H' )
#define ATOM_vCtC HEIMDALL_FOURCC( 'v', 'C', 't', 'C' )
#define ATOM_vCtd HEIMDALL_FOURCC( 'v', 'C', 't', '#' )

#define ATOM_zlib HEIMDALL_FOURCC( 'z', 'l', 'i', 'b' )
#define ATOM_SVQ1 HEIMDALL_FOURCC( 'S', 'V', 'Q', '1' )
#define ATOM_SVQ3 HEIMDALL_FOURCC( 'S', 'V', 'Q', '3' )
#define ATOM_ZyGo HEIMDALL_FOURCC( 'Z', 'y', 'G', 'o' )
#define ATOM_3IV1 HEIMDALL_FOURCC( '3', 'I', 'V', '1' )
#define ATOM_3iv1 HEIMDALL_FOURCC( '3', 'i', 'v', '1' )
#define ATOM_3IV2 HEIMDALL_FOURCC( '3', 'I', 'V', '2' )
#define ATOM_3iv2 HEIMDALL_FOURCC( '3', 'i', 'v', '2' )
#define ATOM_3IVD HEIMDALL_FOURCC( '3', 'I', 'V', 'D' )
#define ATOM_3ivd HEIMDALL_FOURCC( '3', 'i', 'v', 'd' )
#define ATOM_3VID HEIMDALL_FOURCC( '3', 'V', 'I', 'D' )
#define ATOM_3vid HEIMDALL_FOURCC( '3', 'v', 'i', 'd' )
#define ATOM_FFV1 HEIMDALL_FOURCC( 'F', 'F', 'V', '1' )
#define ATOM_h263 HEIMDALL_FOURCC( 'h', '2', '6', '3' )
#define ATOM_s263 HEIMDALL_FOURCC( 's', '2', '6', '3' )
#define ATOM_DIVX HEIMDALL_FOURCC( 'D', 'I', 'V', 'X' )
#define ATOM_XVID HEIMDALL_FOURCC( 'X', 'V', 'I', 'D' )
#define ATOM_cvid HEIMDALL_FOURCC( 'c', 'v', 'i', 'd' )
#define ATOM_mjpa HEIMDALL_FOURCC( 'm', 'j', 'p', 'a' )
#define ATOM_mjpb HEIMDALL_FOURCC( 'm', 'j', 'q', 't' )
#define ATOM_mjqt HEIMDALL_FOURCC( 'm', 'j', 'h', 't' )
#define ATOM_mjht HEIMDALL_FOURCC( 'm', 'j', 'p', 'b' )
#define ATOM_VP31 HEIMDALL_FOURCC( 'V', 'P', '3', '1' )
#define ATOM_vp31 HEIMDALL_FOURCC( 'v', 'p', '3', '1' )
#define ATOM_h264 HEIMDALL_FOURCC( 'h', '2', '6', '4' )
#define ATOM_H264 HEIMDALL_FOURCC( 'H', '2', '6', '4' )
#define ATOM_qdrw HEIMDALL_FOURCC( 'q', 'd', 'r', 'w' )
#define ATOM_vp08 HEIMDALL_FOURCC( 'v', 'p', '0', '8' )
#define ATOM_vp09 HEIMDALL_FOURCC( 'v', 'p', '0', '9' )
#define ATOM_vp10 HEIMDALL_FOURCC( 'v', 'p', '1', '0' )
#define ATOM_WMV3 HEIMDALL_FOURCC( 'W', 'M', 'V', '3' )
#define ATOM_WVC1 HEIMDALL_FOURCC( 'W', 'V', 'C', '1' )

#define ATOM_av01 HEIMDALL_FOURCC( 'a', 'v', '0', '1' )
#define ATOM_avc1 HEIMDALL_FOURCC( 'a', 'v', 'c', '1' )
#define ATOM_avc3 HEIMDALL_FOURCC( 'a', 'v', 'c', '3' )
#define ATOM_av1C HEIMDALL_FOURCC( 'a', 'v', '1', 'C' )
#define ATOM_avcC HEIMDALL_FOURCC( 'a', 'v', 'c', 'C' )
#define ATOM_vpcC HEIMDALL_FOURCC( 'v', 'p', 'c', 'C' )
#define ATOM_vvc1 HEIMDALL_FOURCC( 'v', 'v', 'c', '1' )
#define ATOM_vvcC HEIMDALL_FOURCC( 'v', 'v', 'c', 'C' )
#define ATOM_apv1 HEIMDALL_FOURCC( 'a', 'p', 'v', '1' )
#define ATOM_apvC HEIMDALL_FOURCC( 'a', 'p', 'v', 'C' )
#define ATOM_m4ds HEIMDALL_FOURCC( 'm', '4', 'd', 's' )

#define ATOM_fiel HEIMDALL_FOURCC( 'f', 'i', 'e', 'l' )
#define ATOM_glbl HEIMDALL_FOURCC( 'g', 'l', 'b', 'l' )
#define ATOM_hvcC HEIMDALL_FOURCC( 'h', 'v', 'c', 'C' )
#define ATOM_jpeC HEIMDALL_FOURCC( 'j', 'p', 'e', 'C' )

#define ATOM_dvc  HEIMDALL_FOURCC( 'd', 'v', 'c', ' ' )
#define ATOM_dvp  HEIMDALL_FOURCC( 'd', 'v', 'p', ' ' )
#define ATOM_dv5n HEIMDALL_FOURCC( 'd', 'v', '5', 'n' )
#define ATOM_dv5p HEIMDALL_FOURCC( 'd', 'v', '5', 'p' )
#define ATOM_raw  HEIMDALL_FOURCC( 'r', 'a', 'w', ' ' )
#define ATOM_dOps HEIMDALL_FOURCC( 'd', 'O', 'p', 's' )
#define ATOM_wfex HEIMDALL_FOURCC( 'w', 'f', 'e', 'x' )

#define ATOM_jpeg HEIMDALL_FOURCC( 'j', 'p', 'e', 'g' )

#define ATOM_yv12 HEIMDALL_FOURCC( 'y', 'v', '1', '2' )
#define ATOM_yuv2 HEIMDALL_FOURCC( 'y', 'u', 'v', '2' )

#define ATOM_rmra HEIMDALL_FOURCC( 'r', 'm', 'r', 'a' )
#define ATOM_rmda HEIMDALL_FOURCC( 'r', 'm', 'd', 'a' )
#define ATOM_rdrf HEIMDALL_FOURCC( 'r', 'd', 'r', 'f' )
#define ATOM_rmdr HEIMDALL_FOURCC( 'r', 'm', 'd', 'r' )
#define ATOM_rmvc HEIMDALL_FOURCC( 'r', 'm', 'v', 'c' )
#define ATOM_rmcd HEIMDALL_FOURCC( 'r', 'm', 'c', 'd' )
#define ATOM_rmqu HEIMDALL_FOURCC( 'r', 'm', 'q', 'u' )
#define ATOM_alis HEIMDALL_FOURCC( 'a', 'l', 'i', 's' )

#define ATOM_gmhd HEIMDALL_FOURCC( 'g', 'm', 'h', 'd' )
#define ATOM_wave HEIMDALL_FOURCC( 'w', 'a', 'v', 'e' )
#define ATOM_strf HEIMDALL_FOURCC( 's', 't', 'r', 'f' )

#define ATOM_drms HEIMDALL_FOURCC( 'd', 'r', 'm', 's' )
#define ATOM_sinf HEIMDALL_FOURCC( 's', 'i', 'n', 'f' )
#define ATOM_schi HEIMDALL_FOURCC( 's', 'c', 'h', 'i' )
#define ATOM_user HEIMDALL_FOURCC( 'u', 's', 'e', 'r' )
#define ATOM_key  HEIMDALL_FOURCC( 'k', 'e', 'y', ' ' )
#define ATOM_iviv HEIMDALL_FOURCC( 'i', 'v', 'i', 'v' )
#define ATOM_mean HEIMDALL_FOURCC( 'm', 'e', 'a', 'n' )
#define ATOM_name HEIMDALL_FOURCC( 'n', 'a', 'm', 'e' )
#define ATOM_priv HEIMDALL_FOURCC( 'p', 'r', 'i', 'v' )
#define ATOM_drmi HEIMDALL_FOURCC( 'd', 'r', 'm', 'i' )
#define ATOM_frma HEIMDALL_FOURCC( 'f', 'r', 'm', 'a' )
#define ATOM_skcr HEIMDALL_FOURCC( 's', 'k', 'c', 'r' )
#define ATOM_ASF  HEIMDALL_FOURCC( 'A', 'S', 'F', ' ' )

#define ATOM_text HEIMDALL_FOURCC( 't', 'e', 'x', 't' )
#define ATOM_tx3g HEIMDALL_FOURCC( 't', 'x', '3', 'g' )
#define ATOM_subp HEIMDALL_FOURCC( 's', 'u', 'b', 'p' )
#define ATOM_subt HEIMDALL_FOURCC( 's', 'u', 'b', 't' )
#define ATOM_sbtl HEIMDALL_FOURCC( 's', 'b', 't', 'l' )
#define ATOM_clcp HEIMDALL_FOURCC( 'c', 'l', 'c', 'p' )
#define ATOM_c608 HEIMDALL_FOURCC( 'c', '6', '0', '8' )
#define ATOM_c708 HEIMDALL_FOURCC( 'c', '7', '0', '8' )
#define ATOM_wvtt HEIMDALL_FOURCC( 'w', 'v', 't', 't' )

/* In sample for WebVTT */
#define ATOM_vttc HEIMDALL_FOURCC( 'v', 't', 't', 'c' )
#define ATOM_payl HEIMDALL_FOURCC( 'p', 'a', 'y', 'l' )

#define ATOM_0xa9nam HEIMDALL_FOURCC( 0xa9, 'n', 'a', 'm' )
#define ATOM_0xa9aut HEIMDALL_FOURCC( 0xa9, 'a', 'u', 't' )
#define ATOM_0xa9cpy HEIMDALL_FOURCC( 0xa9, 'c', 'p', 'y' )
#define ATOM_0xa9inf HEIMDALL_FOURCC( 0xa9, 'i', 'n', 'f' )
#define ATOM_0xa9isr HEIMDALL_FOURCC( 0xa9, 'i', 's', 'r' )
#define ATOM_0xa9lab HEIMDALL_FOURCC( 0xa9, 'l', 'a', 'b' )
#define ATOM_0xa9lal HEIMDALL_FOURCC( 0xa9, 'l', 'a', 'l' )
#define ATOM_0xa9ART HEIMDALL_FOURCC( 0xa9, 'A', 'R', 'T' )
#define ATOM_0xa9des HEIMDALL_FOURCC( 0xa9, 'd', 'e', 's' )
#define ATOM_0xa9dir HEIMDALL_FOURCC( 0xa9, 'd', 'i', 'r' )
#define ATOM_0xa9cmt HEIMDALL_FOURCC( 0xa9, 'c', 'm', 't' )
#define ATOM_0xa9req HEIMDALL_FOURCC( 0xa9, 'r', 'e', 'q' )
#define ATOM_0xa9day HEIMDALL_FOURCC( 0xa9, 'd', 'a', 'y' )
#define ATOM_0xa9fmt HEIMDALL_FOURCC( 0xa9, 'f', 'm', 't' )
#define ATOM_0xa9prd HEIMDALL_FOURCC( 0xa9, 'p', 'r', 'd' )
#define ATOM_0xa9prf HEIMDALL_FOURCC( 0xa9, 'p', 'r', 'f' )
#define ATOM_0xa9src HEIMDALL_FOURCC( 0xa9, 's', 'r', 'c' )
#define ATOM_0xa9alb HEIMDALL_FOURCC( 0xa9, 'a', 'l', 'b' )
#define ATOM_0xa9dis HEIMDALL_FOURCC( 0xa9, 'd', 'i', 's' )
#define ATOM_0xa9enc HEIMDALL_FOURCC( 0xa9, 'e', 'n', 'c' )
#define ATOM_0xa9trk HEIMDALL_FOURCC( 0xa9, 't', 'r', 'k' )
#define ATOM_0xa9url HEIMDALL_FOURCC( 0xa9, 'u', 'r', 'l' )
#define ATOM_0xa9dsa HEIMDALL_FOURCC( 0xa9, 'd', 's', 'a' )
#define ATOM_0xa9hst HEIMDALL_FOURCC( 0xa9, 'h', 's', 't' )
#define ATOM_0xa9ope HEIMDALL_FOURCC( 0xa9, 'o', 'p', 'e' )
#define ATOM_0xa9wrt HEIMDALL_FOURCC( 0xa9, 'w', 'r', 't' )
#define ATOM_0xa9com HEIMDALL_FOURCC( 0xa9, 'c', 'o', 'm' )
#define ATOM_0xa9gen HEIMDALL_FOURCC( 0xa9, 'g', 'e', 'n' )
#define ATOM_0xa9too HEIMDALL_FOURCC( 0xa9, 't', 'o', 'o' )
#define ATOM_0xa9wrn HEIMDALL_FOURCC( 0xa9, 'w', 'r', 'n' )
#define ATOM_0xa9swr HEIMDALL_FOURCC( 0xa9, 's', 'w', 'r' )
#define ATOM_0xa9mak HEIMDALL_FOURCC( 0xa9, 'm', 'a', 'k' )
#define ATOM_0xa9mal HEIMDALL_FOURCC( 0xa9, 'm', 'a', 'l' )
#define ATOM_0xa9mod HEIMDALL_FOURCC( 0xa9, 'm', 'o', 'd' )
#define ATOM_0xa9PRD HEIMDALL_FOURCC( 0xa9, 'P', 'R', 'D' )
#define ATOM_0xa9grp HEIMDALL_FOURCC( 0xa9, 'g', 'r', 'p' )
#define ATOM_0xa9lyr HEIMDALL_FOURCC( 0xa9, 'l', 'y', 'r' )
#define ATOM_0xa9gen HEIMDALL_FOURCC( 0xa9, 'g', 'e', 'n' )
#define ATOM_0xa9st3 HEIMDALL_FOURCC( 0xa9, 's', 't', '3' )
#define ATOM_0xa9ard HEIMDALL_FOURCC( 0xa9, 'a', 'r', 'd' )
#define ATOM_0xa9arg HEIMDALL_FOURCC( 0xa9, 'a', 'r', 'g' )
#define ATOM_0xa9cak HEIMDALL_FOURCC( 0xa9, 'c', 'a', 'k' )
#define ATOM_0xa9con HEIMDALL_FOURCC( 0xa9, 'c', 'o', 'n' )
#define ATOM_0xa9des HEIMDALL_FOURCC( 0xa9, 'd', 'e', 's' )
#define ATOM_0xa9lnt HEIMDALL_FOURCC( 0xa9, 'l', 'n', 't' )
#define ATOM_0xa9phg HEIMDALL_FOURCC( 0xa9, 'p', 'h', 'g' )
#define ATOM_0xa9pub HEIMDALL_FOURCC( 0xa9, 'p', 'u', 'b' )
#define ATOM_0xa9sne HEIMDALL_FOURCC( 0xa9, 's', 'n', 'e' )
#define ATOM_0xa9snm HEIMDALL_FOURCC( 0xa9, 's', 'n', 'm' )
#define ATOM_0xa9sol HEIMDALL_FOURCC( 0xa9, 's', 'o', 'l' )
#define ATOM_0xa9thx HEIMDALL_FOURCC( 0xa9, 't', 'h', 'x' )
#define ATOM_0xa9xpd HEIMDALL_FOURCC( 0xa9, 'x', 'p', 'd' )
#define ATOM_0xa9xyz HEIMDALL_FOURCC( 0xa9, 'x', 'y', 'z' )
#define ATOM_aART HEIMDALL_FOURCC( 'a', 'A', 'R', 'T' )
#define ATOM_chpl HEIMDALL_FOURCC( 'c', 'h', 'p', 'l' )
#define ATOM_cpil HEIMDALL_FOURCC( 'c', 'p', 'i', 'l' )
#define ATOM_HMMT HEIMDALL_FOURCC( 'H', 'M', 'M', 'T' )
#define ATOM_desc HEIMDALL_FOURCC( 'd', 'e', 's', 'c' )
#define ATOM_disk HEIMDALL_FOURCC( 'd', 'i', 's', 'k' )
#define ATOM_ID32 HEIMDALL_FOURCC( 'I', 'D', '3', '2' )
#define ATOM_WLOC HEIMDALL_FOURCC( 'W', 'L', 'O', 'C' )
#define ATOM_ITUN HEIMDALL_FOURCC( '-', '-', '-', '-' )

#define ATOM_meta HEIMDALL_FOURCC( 'm', 'e', 't', 'a' )
#define ATOM_atID HEIMDALL_FOURCC( 'a', 't', 'I', 'D' )
#define ATOM_ilst HEIMDALL_FOURCC( 'i', 'l', 's', 't' )
#define ATOM_cnID HEIMDALL_FOURCC( 'c', 'n', 'I', 'D' )
#define ATOM_covr HEIMDALL_FOURCC( 'c', 'o', 'v', 'r' )
#define ATOM_flvr HEIMDALL_FOURCC( 'f', 'l', 'v', 'r' )
#define ATOM_rtng HEIMDALL_FOURCC( 'r', 't', 'n', 'g' )
#define ATOM_thum HEIMDALL_FOURCC( 't', 'h', 'u', 'm' )
#define ATOM_tsel HEIMDALL_FOURCC( 't', 's', 'e', 'l' )
#define ATOM_xid_ HEIMDALL_FOURCC( 'x', 'i', 'd', ' ' )
#define ATOM_gshh HEIMDALL_FOURCC( 'g', 's', 'h', 'h' )
#define ATOM_gspm HEIMDALL_FOURCC( 'g', 's', 'p', 'm' )
#define ATOM_gspu HEIMDALL_FOURCC( 'g', 's', 'p', 'u' )
#define ATOM_gssd HEIMDALL_FOURCC( 'g', 's', 's', 'd' )
#define ATOM_gsst HEIMDALL_FOURCC( 'g', 's', 's', 't' )
#define ATOM_gstd HEIMDALL_FOURCC( 'g', 's', 't', 'd' )
#define ATOM_colr HEIMDALL_FOURCC( 'c', 'o', 'l', 'r' )
#define ATOM_irot HEIMDALL_FOURCC( 'i', 'r', 'o', 't' )
#define ATOM_SmDm HEIMDALL_FOURCC( 'S', 'm', 'D', 'm' )
#define ATOM_CoLL HEIMDALL_FOURCC( 'C', 'o', 'L', 'L' )
#define ATOM_mdcv HEIMDALL_FOURCC( 'm', 'd', 'c', 'v' )
#define ATOM_clli HEIMDALL_FOURCC( 'c', 'l', 'l', 'i' )
#define ATOM_purl HEIMDALL_FOURCC( 'p', 'u', 'r', 'l' )
#define ATOM_dvcC HEIMDALL_FOURCC( 'd', 'v', 'c', 'C' )
#define ATOM_dvvC HEIMDALL_FOURCC( 'd', 'v', 'v', 'C' )
#define ATOM_dvwC HEIMDALL_FOURCC( 'd', 'v', 'w', 'C' )

#define ATOM_0x40PRM HEIMDALL_FOURCC( '@', 'P', 'R', 'M' )
#define ATOM_0x40PRQ HEIMDALL_FOURCC( '@', 'P', 'R', 'Q' )
#define ATOM_MCPS HEIMDALL_FOURCC( 'M', 'C', 'P', 'S' )
#define ATOM_SDLN HEIMDALL_FOURCC( 'S', 'D', 'L', 'N' )
#define ATOM_vndr HEIMDALL_FOURCC( 'v', 'n', 'd', 'r' )

#define ATOM_SA3D HEIMDALL_FOURCC( 'S', 'A', '3', 'D' )

/* iso4 meta references */
#define ATOM_idat HEIMDALL_FOURCC('i','d','a','t')
#define ATOM_iloc HEIMDALL_FOURCC('i','l','o','c')
#define ATOM_iinf HEIMDALL_FOURCC('i','i','n','f')
#define ATOM_infe HEIMDALL_FOURCC('i','n','f','e')
#define ATOM_iref HEIMDALL_FOURCC('i','r','e','f')
#define ATOM_pitm HEIMDALL_FOURCC('p','i','t','m')
/* HEIF specific meta */
#define ATOM_iprp HEIMDALL_FOURCC('i','p','r','p')
#define ATOM_ipco HEIMDALL_FOURCC('i','p','c','o')
#define ATOM_ispe HEIMDALL_FOURCC('i','s','p','e')
#define ATOM_ipma HEIMDALL_FOURCC('i','p','m','a')

#define HANDLER_mdta HEIMDALL_FOURCC('m', 'd', 't', 'a')
#define HANDLER_mdir HEIMDALL_FOURCC('m', 'd', 'i', 'r')
#define HANDLER_ID32 ATOM_ID32

#define SAMPLEGROUP_rap  HEIMDALL_FOURCC('r', 'a', 'p', ' ')
#define SAMPLEGROUP_roll HEIMDALL_FOURCC('r', 'o', 'l', 'l')

/* tref reference type boxes */
#define ATOM_chap HEIMDALL_FOURCC( 'c', 'h', 'a', 'p' )
#define ATOM_dpnd HEIMDALL_FOURCC( 'd', 'p', 'n', 'd' )
#define ATOM_hint HEIMDALL_FOURCC( 'h', 'i', 'n', 't' )
#define ATOM_ipir HEIMDALL_FOURCC( 'i', 'p', 'i', 'r' )
#define ATOM_mpod HEIMDALL_FOURCC( 'm', 'p', 'o', 'd' )
#define ATOM_cdsc HEIMDALL_FOURCC( 'c', 'd', 's', 'c' )

/* mpeg or mpeg PS m1v, m1s */
#define HANDLER_m1v  HEIMDALL_FOURCC('m', '1', 'v', ' ')
#define HANDLER_m1s  HEIMDALL_FOURCC('m', '1', 's', ' ')
#define HANDLER_mpeg HEIMDALL_FOURCC('M', 'P', 'E', 'G')

/* HEIF Specific */
#define HANDLER_pict HEIMDALL_FOURCC('p', 'i', 'c', 't')

/* Do you want some debug information on all read boxes ? */
#ifndef NDEBUG
# define MP4_VERBOSE  1
//# define MP4_ULTRA_VERBOSE  1
#endif

struct MP4_Box_s;

#define MP4_MFRO_BOXSIZE 16

/* uuid Universal Unique IDentifiers */
typedef struct UUID_s
{
    uint8_t b[16];
} UUID_t;

/* specific structure for all boxes */

typedef struct MP4_Box_data_tfxd_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint64_t i_fragment_duration;
    uint64_t i_fragment_abs_time;

} MP4_Box_data_tfxd_t;

typedef struct TfrfBoxDataFields_s
{
    uint64_t i_fragment_duration;
    uint64_t i_fragment_abs_time;

} TfrfBoxDataFields_t;

typedef struct MP4_Box_data_tfrf_s
{
    uint8_t  i_version;
    uint8_t  i_fragment_count;
    uint32_t i_flags;

    TfrfBoxDataFields_t *p_tfrf_data_fields;

} MP4_Box_data_tfrf_t;

typedef struct MP4_Box_data_ftyp_s
{
    uint32_t i_major_brand;
    uint32_t i_minor_version;

    uint32_t i_compatible_brands_count;
    uint32_t *i_compatible_brands;

} MP4_Box_data_ftyp_t;

typedef struct MP4_Box_data_mvhd_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint64_t i_creation_time;
    uint64_t i_modification_time;
    uint32_t i_timescale;
    uint64_t i_duration;

    int32_t  i_rate;
    int16_t  i_volume;
    uint16_t i_reserved1;
    uint32_t i_reserved2[2];
    int32_t  i_matrix[9];
    uint32_t i_predefined[6];
    uint32_t i_next_track_id;

} MP4_Box_data_mvhd_t;

#define MP4_TRACK_ENABLED    0x000001
#define MP4_TRACK_IN_MOVIE   0x000002
#define MP4_TRACK_IN_PREVIEW 0x000004
typedef struct MP4_Box_data_tkhd_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint64_t i_creation_time;
    uint64_t i_modification_time;
    uint32_t i_track_ID;
    uint32_t i_reserved;
    uint64_t i_duration;

    uint32_t i_reserved2[2];
    int16_t  i_layer;
    int16_t  i_alternate_group;

    int16_t  i_volume;
    uint16_t i_reserved3;
    int32_t  i_matrix[9];
    uint32_t i_width;
    uint32_t i_height;
    float    f_rotation;
    int      i_flip;

} MP4_Box_data_tkhd_t;

typedef struct
{
    uint32_t i_start_time;
    int32_t  i_duration;
    uint32_t i_flags;
    uint32_t i_hints;
} MP4_Box_data_load_t;

typedef struct MP4_Box_data_mdhd_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint64_t i_creation_time;
    uint64_t i_modification_time;
    uint32_t i_timescale;
    uint64_t i_duration;

    char     rgs_language[3]; /* ISO-639-2/T or Mac lang table */
    bool     b_mac_encoding;  /* media using mac encoding */

    uint16_t i_quality;

} MP4_Box_data_mdhd_t;

typedef struct MP4_Box_data_hdlr_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_predefined;
    uint32_t i_handler_type; /* "vide" "soun" "hint" "odsm"
                           "crsm" "sdsm" "m7sm" "ocsm"
                           "ipsm" "mjsm" */

    unsigned char *psz_name; /* in UTF-8 */

} MP4_Box_data_hdlr_t;

typedef struct MP4_Box_data_vmhd_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint16_t i_graphics_mode;
    uint16_t i_opcolor[3];

} MP4_Box_data_vmhd_t;

typedef struct MP4_Box_data_smhd_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    int16_t  i_balance;
    uint16_t i_reserved;

} MP4_Box_data_smhd_t;

typedef struct MP4_Box_data_hmhd_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint16_t i_max_PDU_size;
    uint16_t i_avg_PDU_size;
    uint32_t i_max_bitrate;
    uint32_t i_avg_bitrate;
    uint32_t i_reserved;

} MP4_Box_data_hmhd_t;

typedef struct MP4_Box_data_url_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    char *psz_location;

} MP4_Box_data_url_t;

typedef struct MP4_Box_data_urn_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    char *psz_name;
    char *psz_location;

} MP4_Box_data_urn_t;

typedef struct MP4_Box_data_lcont_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_entry_count;
/* XXX it's also a container with i_entry_count entry */
} MP4_Box_data_lcont_t;

typedef struct MP4_Box_data_stts_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_entry_count;
    uint32_t *pi_sample_count; /* these are array */
    uint32_t *pi_sample_delta;

} MP4_Box_data_stts_t;

typedef struct MP4_Box_data_ctts_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_entry_count;

    uint32_t *pi_sample_count; /* these are array */
    int32_t *pi_sample_offset;

} MP4_Box_data_ctts_t;

typedef struct MP4_Box_data_cslg_s
{
    int64_t ct_to_dts_shift;
    int64_t i_least_delta;
    int64_t i_max_delta;
    int64_t i_composition_starttime;
    int64_t i_composition_endtime;

} MP4_Box_data_cslg_t;

// https://developer.apple.com/library/mac/documentation/QuickTime/QTFF/QTFFChap3/qtff3.html#//apple_ref/doc/uid/TP40000939-CH205-125526
typedef struct MP4_Box_data_colr_s
{
    uint32_t i_type;
    union
    {
        struct
        {
            uint16_t i_primary_idx;
            uint16_t i_transfer_function_idx;
            uint16_t i_matrix_idx;
            uint8_t i_full_range;
        } nclc;
    };
} MP4_Box_data_colr_t;

typedef struct
{
    uint16_t i_ccw_degrees;
} MP4_Box_data_irot_t;

typedef struct
{
    uint8_t i_version_major;
    uint8_t i_version_minor;
    uint8_t i_profile;
    uint8_t i_level;
    uint8_t i_rpu_present;
    uint8_t i_el_present;
    uint8_t i_bl_present;
} MP4_Box_data_dvcC_t;

#define SAMPLE_DESC_COMMON_HEADER \
    uint8_t  i_reserved1[6];\
    uint16_t i_data_reference_index

typedef struct
{
    SAMPLE_DESC_COMMON_HEADER;

    //uint32_t i_reserved2[2];
    uint16_t i_qt_version;
    uint16_t i_qt_revision_level;
    uint32_t i_qt_vendor;

    uint16_t i_channelcount;
    uint16_t i_samplesize;
    uint16_t i_compressionid;
    uint16_t i_reserved3;
    uint16_t i_sampleratehi; /* timescale of track */
    uint16_t i_sampleratelo;

    /* for version 1 (i_reserved1[0] == 1) */
    uint32_t i_sample_per_packet;
    uint32_t i_bytes_per_packet;
    uint32_t i_bytes_per_frame;
    uint32_t i_bytes_per_sample;

    /* v2 */
    uint32_t i_constbitsperchannel; /* consts are nonzero only if constant */
    uint32_t i_formatflags;
    uint32_t i_constbytesperaudiopacket;
    uint32_t i_constLPCMframesperaudiopacket;

    /* XXX hack */
    int     i_qt_description;
    uint8_t *p_qt_description;

} MP4_Box_data_sample_soun_t;

typedef struct
{
    SAMPLE_DESC_COMMON_HEADER;

    uint16_t i_qt_version;
    uint16_t i_qt_revision_level;
    uint32_t i_qt_vendor;

    uint32_t i_qt_temporal_quality;
    uint32_t i_qt_spatial_quality;

    int16_t  i_width;
    int16_t  i_height;

    uint32_t i_horizresolution;
    uint32_t i_vertresolution;

    uint32_t reserved;
    uint16_t i_qt_frame_count;

    char     sz_compressorname[32];
    uint16_t i_depth;

    int16_t  i_qt_color_table;
    video_palette_t *p_palette;

    /* XXX hack ImageDescription */
    int     i_qt_image_description;
    uint8_t *p_qt_image_description;

} MP4_Box_data_sample_vide_t;

#define MP4_TEXT_DISPLAY_FLAG_DONT_DISPLAY       (1<<0)
#define MP4_TEXT_DISPLAY_FLAG_AUTO_SCALE         (1<<1)
#define MP4_TEXT_DISPLAY_FLAG_CLIP_TO_TEXT_BOX   (1<<2)
#define MP4_TEXT_DISPLAY_FLAG_USE_MOVIE_BG_COLOR (1<<3)
#define MP4_TEXT_DISPLAY_FLAG_SHRINK_TEXT_BOX_TO_FIT (1<<4)
#define MP4_TEXT_DISPLAY_FLAG_SCROLL_IN          (1<<5)
#define MP4_TEXT_DISPLAY_FLAG_SCROLL_OUT         (1<<6)
#define MP4_TEXT_DISPLAY_FLAG_HORIZONTAL_SCROLL  (1<<7)
#define MP4_TEXT_DISPLAY_FLAG_REVERSE_SCROLL     (1<<8)
#define MP4_TEXT_DISPLAY_FLAG_CONTINUOUS_SCROLL  (1<<9)
#define MP4_TEXT_DISPLAY_FLAG_FLOW_HORIZONTAL    (1<<10)
#define MP4_TEXT_DISPLAY_FLAG_CONTINUOUS_KARAOKE (1<<11)
#define MP4_TEXT_DISPLAY_FLAG_DROP_SHADOW        (1<<12)
#define MP4_TEXT_DISPLAY_FLAG_ANTI_ALIAS         (1<<13)
#define MP4_TEXT_DISPLAY_FLAG_KEYED_TEXT         (1<<14)
#define MP4_TEXT_DISPLAY_FLAG_INVERSE_HILITE     (1<<15)
#define MP4_TEXT_DISPLAY_FLAG_COLOR_HILITE       (1<<16)
#define MP4_TEXT_DISPLAY_FLAG_WRITE_VERTICALLY   (1<<17)

typedef struct
{
    SAMPLE_DESC_COMMON_HEADER;

    uint8_t *p_data;
    size_t   i_data;

} MP4_Box_data_sample_generic_t;

typedef struct MP4_Box_data_rrtp_sample_s
{
    uint16_t i_hinttrackversion;
    uint16_t i_highestcompatibleversion;
    uint32_t i_maxpacketsize;

    uint8_t *p_additionaldata;

} MP4_Box_data_rrtp_sample_t;

typedef struct MP4_Box_data_timescale_entry_s
{
    uint32_t i_timescale;

} MP4_Box_data_timescale_entry_t;

typedef struct MP4_Box_data_time_offset_s
{
    uint32_t i_offset;

} MP4_Box_data_time_offset_t;

typedef struct MP4_Box_data_timestampsynchrony_s
{
    uint8_t i_reserved_timestamp_sync;

} MP4_Box_data_timestampsynchrony_t;

typedef struct MP4_Box_data_moviehintinformation_rtp_s
{
    uint32_t i_description_format;
    char *psz_text;

} MP4_Box_data_moviehintinformation_rtp_t;

typedef struct MP4_Box_data_sdp_s
{
    char *psz_text;

} MP4_Box_data_sdp_t;

typedef struct MP4_Box_data_tims_s
{
    uint32_t i_timescale;

} MP4_Box_data_tims_t;

typedef struct MP4_Box_data_tsro_s
{
    int32_t i_offset;

} MP4_Box_data_tsro_t;

typedef struct MP4_Box_data_tssy_s
{
    uint8_t i_reserved_timestamp_sync;

} MP4_Box_data_tssy_t;

typedef struct MP4_Box_data_stsd_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_entry_count;

    /* it contains SampleEntry handled as if it was Box */

} MP4_Box_data_stsd_t;


typedef struct MP4_Box_data_stsz_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_sample_size;
    uint32_t i_sample_count;

    uint32_t *i_entry_size; /* array , empty if i_sample_size != 0 */

} MP4_Box_data_stsz_t;

typedef struct MP4_Box_data_stsc_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_entry_count;

    uint32_t *i_first_chunk; /* theses are arrays */
    uint32_t *i_samples_per_chunk;
    uint32_t *i_sample_description_index;

} MP4_Box_data_stsc_t;


typedef struct MP4_Box_data_co64_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_entry_count;

    uint64_t *i_chunk_offset;

} MP4_Box_data_co64_t;


typedef struct MP4_Box_data_stss_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_entry_count;

    uint32_t *i_sample_number;

} MP4_Box_data_stss_t;

typedef struct MP4_Box_data_stsh_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_entry_count;

    uint32_t *i_shadowed_sample_number;
    uint32_t *i_sync_sample_number;

} MP4_Box_data_stsh_t;

typedef struct MP4_Box_data_stdp_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint16_t *i_priority;

} MP4_Box_data_stdp_t;

typedef struct
{
    uint64_t i_segment_duration; /* movie timescale */
    int64_t  i_media_time; /* media(track) timescale */
    uint16_t i_media_rate_integer;
    uint16_t i_media_rate_fraction;
} MP4_Box_data_elst_entry_t;

typedef struct MP4_Box_data_elst_s
{
    uint32_t i_entry_count;

    MP4_Box_data_elst_entry_t *entries;

} MP4_Box_data_elst_t;

typedef struct MP4_Box_data_cprt_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    char     rgs_language[3]; /* ISO-639-2/T */

    char *psz_notice;
} MP4_Box_data_cprt_t;


/* DecoderConfigDescriptor */
typedef struct MP4_descriptor_decoder_config_s
{
    uint8_t i_objectProfileIndication;
    uint8_t i_streamType;
    int     b_upStream;
    uint32_t i_buffer_sizeDB;
    uint32_t i_max_bitrate;
    uint32_t i_avg_bitrate;

    int     i_decoder_specific_info_len;
    uint8_t *p_decoder_specific_info;
    /* some other stuff */

} MP4_descriptor_decoder_config_t;

typedef struct MP4_descriptor_SL_config_s
{

    int i_dummy; /* ANSI C forbids empty structures */

} MP4_descriptor_SL_config_t;


typedef struct MP4_descriptor_ES_s /* ISO/IEC 14496-1 8.3.3 ES_DescrTag */
{
    uint16_t i_ES_ID;
    int      b_stream_dependence;
    int      b_url;
    int      b_OCRstream;
    int      i_stream_priority;

    uint16_t  i_depend_on_ES_ID; /* if b_stream_dependence set */

    unsigned char *psz_URL;

    uint16_t i_OCR_ES_ID;       /* if b_OCRstream */
    MP4_descriptor_decoder_config_t *p_decConfigDescr;

    MP4_descriptor_SL_config_t *p_slConfigDescr;

    /* some other stuff ... */

} MP4_descriptor_ES_t;

/* ES descriptor */
typedef struct MP4_Box_data_esds_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    MP4_descriptor_ES_t es_descriptor;

} MP4_Box_data_esds_t;


typedef struct MP4_Box_data_dcom_s
{
    uint32_t i_algorithm; /* fourcc */

} MP4_Box_data_dcom_t;

typedef struct MP4_Box_data_cmvd_s
{
    uint32_t i_uncompressed_size;
    uint32_t i_compressed_size;
    uint8_t *p_data;

} MP4_Box_data_cmvd_t;

typedef struct
{
    uint32_t i_type;
} MP4_Box_data_frma_t;

typedef struct
{
    uint32_t i_init;
    uint32_t i_encr;
    uint32_t i_decr;
} MP4_Box_data_skcr_t;

typedef struct
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_ref_type;
    char     *psz_ref;

} MP4_Box_data_rdrf_t;

typedef struct
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_rate;

} MP4_Box_data_rmdr_t;

typedef struct
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_gestaltType;
    uint32_t i_val1;
    uint32_t i_val2;
    uint16_t i_checkType;   /* 0: val1 is version min
                               1: gestalt value & val2 == val1 */

} MP4_Box_data_rmvc_t;

typedef struct
{
    uint8_t  i_version;
    uint32_t i_flags;


} MP4_Box_data_rmcd_t;

typedef struct
{
    uint32_t i_quality;

} MP4_Box_data_rmqu_t;

typedef struct MP4_Box_data_mfhd_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_sequence_number;

} MP4_Box_data_mfhd_t;

typedef struct MP4_Box_sidx_item_s
{
    uint32_t i_referenced_size;
    uint32_t i_subsegment_duration;
    uint8_t  b_reference_type;
    uint8_t  b_starts_with_SAP;
    uint8_t  i_SAP_type;
    uint32_t i_SAP_delta_time;

} MP4_Box_sidx_item_t;

typedef struct MP4_Box_data_sidx_s
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_reference_ID;
    uint32_t i_timescale;
    uint64_t i_earliest_presentation_time;
    uint64_t i_first_offset;
    uint16_t i_reference_count;

    MP4_Box_sidx_item_t *p_items;

} MP4_Box_data_sidx_t;

#define MP4_TFHD_BASE_DATA_OFFSET     (1LL<<0)
#define MP4_TFHD_SAMPLE_DESC_INDEX    (1LL<<1)
#define MP4_TFHD_DFLT_SAMPLE_DURATION (1LL<<3)
#define MP4_TFHD_DFLT_SAMPLE_SIZE     (1LL<<4)
#define MP4_TFHD_DFLT_SAMPLE_FLAGS    (1LL<<5)
#define MP4_TFHD_DURATION_IS_EMPTY    (1LL<<16)
#define MP4_TFHD_DEFAULT_BASE_IS_MOOF (1LL<<17)
typedef struct MP4_Box_data_tfhd_s
{
    uint8_t  i_version;
    bool     b_empty;
    uint32_t i_flags;
    uint32_t i_track_ID;

    /* optional fields */
    uint64_t i_base_data_offset;
    uint32_t i_sample_description_index;
    uint32_t i_default_sample_duration;
    uint32_t i_default_sample_size;
    uint32_t i_default_sample_flags;

} MP4_Box_data_tfhd_t;

#define MP4_TRUN_DATA_OFFSET         (1<<0)
#define MP4_TRUN_FIRST_FLAGS         (1<<2)
#define MP4_TRUN_SAMPLE_DURATION     (1<<8)
#define MP4_TRUN_SAMPLE_SIZE         (1<<9)
#define MP4_TRUN_SAMPLE_FLAGS        (1<<10)
#define MP4_TRUN_SAMPLE_TIME_OFFSET  (1<<11)
typedef struct MP4_descriptor_trun_sample_t
{
    uint32_t i_duration;
    uint32_t i_size;
    uint32_t i_flags;
    union
    {
        uint32_t v0;
        int32_t  v1; /* version == 1 ? signed : unsigned */
    } i_composition_time_offset;
} MP4_descriptor_trun_sample_t;

typedef struct MP4_Box_data_trun_s
{
    uint8_t  i_version;
    uint32_t i_flags;
    uint32_t i_sample_count;

    /* optional fields */
    int32_t i_data_offset;
    uint32_t i_first_sample_flags;

    MP4_descriptor_trun_sample_t *p_samples;

} MP4_Box_data_trun_t;

typedef struct MP4_Box_data_tfdt_s
{
    uint8_t  i_version;
    uint32_t i_flags;
    uint64_t i_base_media_decode_time;

} MP4_Box_data_tfdt_t;

typedef struct
{
    uint32_t i_date;
    uint32_t i_type;
    uint16_t i_index;
} MP4_Box_data_pnot_t;

typedef struct
{
    char *psz_text;
    uint64_t i_length;

} MP4_Box_data_string_t;

typedef struct
{
    void  *p_blob;
    size_t i_blob;
} MP4_Box_data_binary_t;

typedef struct
{
    uint32_t i_entry_count;
    uint32_t *i_track_ID;

} MP4_Box_data_trak_reference_t;

typedef struct
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint8_t i_chapter;
    struct
    {
        char    *psz_name;
        int64_t  i_start;
    } chapter[256];
} MP4_Box_data_chpl_t;

typedef struct
{
    uint32_t i_chapter_count;
    uint32_t *pi_chapter_start;

} MP4_Box_data_HMMT_t;

typedef struct
{
    uint8_t i_version;
    uint8_t i_profile;
    uint8_t i_profile_compatibility;
    uint8_t i_level;

    size_t   i_avcC;
    uint8_t *p_avcC;

} MP4_Box_data_avcC_t;

typedef struct
{
    uint8_t i_profile;
    uint8_t i_level;
    uint8_t i_presentation_delay;

    size_t   i_av1C;
    uint8_t *p_av1C;

} MP4_Box_data_av1C_t;

typedef struct
{
    uint8_t i_version;
    uint8_t i_profile;
    uint8_t i_level;
    uint8_t i_bit_depth;
    uint8_t i_chroma_subsampling;
    uint8_t i_color_primaries;
    uint8_t i_xfer_function;
    uint8_t i_matrix_coeffs;
    uint8_t i_fullrange;
    uint16_t i_codec_init_datasize;
    uint8_t *p_codec_init_data;
} MP4_Box_data_vpcC_t;

typedef struct
{
    uint16_t primaries[3*2]; /* G,B,R / x,y */
    uint16_t white_point[2]; /* x,y */
    uint32_t i_luminanceMax;
    uint32_t i_luminanceMin;
} MP4_Box_data_SmDm_t;

typedef struct
{
    uint16_t i_maxCLL;
    uint16_t i_maxFALL;
} MP4_Box_data_CoLL_t;

typedef struct
{
    WAVEFORMATEX Format;
    uint32_t i_extra;
    uint8_t *p_extra;
} MP4_Box_data_WMA2_t;

typedef struct
{
    HEIMDALL_BITMAPINFOHEADER bmiHeader;
    uint32_t i_extra;
    uint8_t *p_extra;
} MP4_Box_data_strf_t;

typedef struct
{
    uint32_t i_stream_number;
    uint32_t i_packet_size;
} MP4_Box_data_ASF_t;

typedef union
{
    struct
    {
        uint8_t i_num_leading_samples_known;
        uint8_t i_num_leading_samples;
    } rap;
    struct
    {
        int16_t i_roll_distance;
    } roll;
} MP4_Box_data_sgpd_entry_t;

typedef struct
{
    uint8_t i_version;
    uint32_t i_grouping_type;
    uint32_t i_default_sample_description_index;
    uint32_t i_entry_count;
    MP4_Box_data_sgpd_entry_t *p_entries;
} MP4_Box_data_sgpd_t;

typedef struct
{
    uint32_t i_sample_count;
    uint32_t i_group_description_index;
} MP4_Box_data_sbgp_entry_t;

typedef struct
{
    uint8_t i_version;
    uint32_t i_grouping_type;
    uint32_t i_grouping_type_parameter;
    uint32_t i_entry_count;
    MP4_Box_data_sbgp_entry_t *p_entries;
} MP4_Box_data_sbgp_t;

typedef struct
{
    uint8_t i_version;
    uint32_t i_channels_flags; /* 24 bits */
    struct CoreAudio_layout_s layout;
} MP4_Box_data_chan_t;

typedef struct
{
    uint32_t i_sample_rate;
} MP4_Box_data_srat_t;

typedef struct
{
    uint16_t i_data_rate;
    uint8_t i_num_ind_sub;
    struct
    {
        uint8_t i_fscod;
        uint8_t i_bsid;
        uint8_t i_bsmod;
        uint8_t i_acmod;
        uint8_t i_lfeon;
        uint8_t i_num_dep_sub;
        uint8_t i_chan_loc;
    } stream[8];

} MP4_Box_data_dec3_t;

typedef struct
{
    uint8_t i_fscod;
    uint8_t i_bsid;
    uint8_t i_bsmod;
    uint8_t i_acmod;
    uint8_t i_lfeon;
    uint8_t i_bitrate_code;

} MP4_Box_data_dac3_t;

typedef struct
{
    uint8_t i_profile_level;

    uint32_t i_vc1;
    uint8_t *p_vc1;

} MP4_Box_data_dvc1_t;

typedef struct
{
    uint32_t i_flags;
} MP4_Box_data_fiel_t;

typedef struct
{
    uint16_t i_little_endian;

} MP4_Box_data_enda_t;

typedef struct
{
    uint8_t i_format_flags;
    uint8_t i_sample_size;
} MP4_Box_data_pcmC_t;

typedef struct
{
    uint32_t i_entry_count;
    struct
    {
        uint32_t i_namespace;
        char    *psz_value;
    } *p_entries;
} MP4_Box_data_keys_t;

typedef struct
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint16_t i_object_descriptor;
    uint8_t i_OD_profile_level;
    uint8_t i_scene_profile_level;
    uint8_t i_audio_profile_level;
    uint8_t i_visual_profile_level;
    uint8_t i_graphics_profile_level;

} MP4_Box_data_iods_t;

typedef struct
{
    uint32_t i_buffer_size;
    uint32_t i_max_bitrate;
    uint32_t i_avg_bitrate;
} MP4_Box_data_btrt_t;

typedef struct
{
    uint32_t i_horizontal_spacing;
    uint32_t i_vertical_spacing;
} MP4_Box_data_pasp_t;

typedef struct
{
    uint32_t i_width;
    uint32_t i_height;
    uint32_t i_x_offset;
    uint32_t i_y_offset;
} MP4_Box_data_clap_t;

typedef struct
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint64_t i_fragment_duration;
} MP4_Box_data_mehd_t;

typedef struct
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_track_ID;
    uint32_t i_default_sample_description_index;
    uint32_t i_default_sample_duration;
    uint32_t i_default_sample_size;
    uint32_t i_default_sample_flags;
} MP4_Box_data_trex_t;

typedef struct
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint8_t *p_sample_table;
} MP4_Box_data_sdtp_t;

typedef struct
{
    uint32_t i_switch_group;
} MP4_Box_data_tsel_t;

typedef struct
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_size;
} MP4_Box_data_mfro_t;

typedef struct
{
    uint8_t  i_version;
    uint32_t i_flags;

    uint32_t i_track_ID;
    uint32_t i_number_of_entries;

    uint8_t i_length_size_of_traf_num;
    uint8_t i_length_size_of_trun_num;
    uint8_t i_length_size_of_sample_num;

    uint64_t *p_time;
    uint64_t *p_moof_offset;
    uint8_t *p_traf_number;
    uint8_t *p_trun_number;
    uint8_t *p_sample_number;
} MP4_Box_data_tfra_t;

typedef struct
{
    enum
    {
        DATA_WKT_RESERVED = 0,
        DATA_WKT_UTF8 = 1,
        DATA_WKT_UTF16 = 2,
        DATA_WKT_SJIS = 3,
        DATA_WKT_UTF8_SORT = 4,
        DATA_WKT_UTF16_SORT = 5,
        DATA_WKT_JPEG = 13,
        DATA_WKT_PNG = 14,
        DATA_WKT_BE_SIGNED = 21,
        DATA_WKT_BE_UNSIGNED = 22,
        DATA_WKT_BE_FLOAT32 = 23,
        DATA_WKT_BE_FLOAT64 = 24,
        DATA_WKT_BMP = 27,
        DATA_WKT_QUICKTIME_METADATA_ATOM = 28,
    } e_wellknowntype;
    struct
    {
        uint16_t i_country;
        uint16_t i_language;
    } locale;
    uint8_t *p_blob;
    uint32_t i_blob;
} MP4_Box_data_data_t;

typedef struct
{
    uint32_t i_projection_mode;
    enum {
        XML360_MONOSCOPIC = 0,
        XML360_STEREOSCOPIC_TOP_BOTTOM = 1,
        XML360_STEREOSCOPIC_LEFT_RIGHT = 2,
    } e_stereo_mode;
} MP4_Box_data_360_t;


typedef struct
{
    enum {
        ST3D_MONOSCOPIC = 0,
        ST3D_STEREOSCOPIC_TOP_BOTTOM = 1,
        ST3D_STEREOSCOPIC_LEFT_RIGHT = 2,
    } e_stereo_mode;
    uint8_t i_stereo_mode;
} MP4_Box_data_st3d_t;

typedef struct
{
    float f_pose_yaw_degrees;
    float f_pose_pitch_degrees;
    float f_pose_roll_degrees;
} MP4_Box_data_prhd_t;

typedef struct
{
    uint32_t i_projection_bounds_top;
    uint32_t i_projection_bounds_bottom;
    uint32_t i_projection_bounds_left;
    uint32_t i_projection_bounds_right;
} MP4_Box_data_equi_t;

typedef struct
{
    uint32_t i_layout;
    uint32_t i_padding;
} MP4_Box_data_cbmp_t;

typedef struct
{
    uint8_t i_ambisonic_type;
    uint32_t i_ambisonic_order;
    uint8_t i_ambisonic_channel_ordering;
    uint8_t i_ambisonic_normalization;
    uint32_t i_num_channels;
} MP4_Box_data_SA3D_t;

typedef struct
{
    uint32_t i_from_item_id;
    uint16_t i_reference_count;
    struct
    {
        uint32_t i_to_item_id;
    } *p_references;
} MP4_Box_data_refbox_t;

typedef struct
{
    uint8_t i_version;
    uint32_t i_flags;
} MP4_Box_data_iref_t;

typedef struct
{
    uint8_t i_offset_size;
    uint8_t i_length_size;
    uint8_t i_base_offset_size;
    uint8_t i_index_size;
    uint32_t i_item_count;
    struct
    {
        uint32_t i_item_id;
        uint8_t  i_construction_method;
        uint16_t i_data_reference_index;
        uint64_t i_base_offset;
        uint16_t i_extent_count;
        struct
        {
            uint64_t i_extent_index;
            uint64_t i_extent_offset;
            uint64_t i_extent_length;
        } *p_extents;
    } *p_items;
} MP4_Box_data_iloc_t;

typedef struct
{
    uint32_t i_entry_count;
} MP4_Box_data_iinf_t;

typedef struct
{
    uint32_t i_flags;
    uint32_t i_item_id;
    uint16_t i_item_protection_index;
    heimdall_fourcc_t item_type;
    char *psz_item_name;
    char *psz_content_type;
    char *psz_content_encoding;
    char *psz_item_uri_type;
} MP4_Box_data_infe_t;

typedef struct
{
    uint32_t i_item_id;
} MP4_Box_data_pitm_t;

typedef struct
{
    uint32_t i_width;
    uint32_t i_height;
} MP4_Box_data_ispe_t;

typedef struct
{
    uint32_t i_entry_count;
    struct
    {
        uint32_t i_item_id;
        uint8_t  i_association_count;
        struct
        {
            uint8_t b_essential;
            uint16_t i_property_index;
        } *p_assocs;
    } *p_entries;
} MP4_Box_data_ipma_t;

/*
typedef struct MP4_Box_data__s
{
    uint8_t  i_version;
    uint32_t i_flags;

} MP4_Box_data__t;

*/

typedef union MP4_Box_data_s
{
    MP4_Box_data_ftyp_t *p_ftyp;
    MP4_Box_data_mvhd_t *p_mvhd;
    MP4_Box_data_mfhd_t *p_mfhd;
    MP4_Box_data_sidx_t *p_sidx;
    MP4_Box_data_tfhd_t *p_tfhd;
    MP4_Box_data_trun_t *p_trun;
    MP4_Box_data_tfdt_t *p_tfdt;
    MP4_Box_data_tkhd_t *p_tkhd;
    MP4_Box_data_mdhd_t *p_mdhd;
    MP4_Box_data_hdlr_t *p_hdlr;
    MP4_Box_data_vmhd_t *p_vmhd;
    MP4_Box_data_smhd_t *p_smhd;
    MP4_Box_data_hmhd_t *p_hmhd;
    MP4_Box_data_url_t  *p_url;
    MP4_Box_data_urn_t  *p_urn;
    MP4_Box_data_lcont_t *p_lcont;
    MP4_Box_data_stts_t *p_stts;
    MP4_Box_data_ctts_t *p_ctts;
    MP4_Box_data_cslg_t *p_cslg;
    MP4_Box_data_colr_t *p_colr;
    MP4_Box_data_irot_t *p_irot;

    MP4_Box_data_sbgp_t *p_sbgp;
    MP4_Box_data_sgpd_t *p_sgpd;

    MP4_Box_data_sample_vide_t *p_sample_vide;
    MP4_Box_data_sample_soun_t *p_sample_soun;
    MP4_Box_data_sample_generic_t *p_sample_gen;

    MP4_Box_data_esds_t *p_esds;
    MP4_Box_data_av1C_t *p_av1C;
    MP4_Box_data_avcC_t *p_avcC;
    MP4_Box_data_dac3_t *p_dac3;
    MP4_Box_data_dec3_t *p_dec3;
    MP4_Box_data_dvc1_t *p_dvc1;
    MP4_Box_data_fiel_t *p_fiel;
    MP4_Box_data_chan_t *p_chan;
    MP4_Box_data_srat_t *p_srat;
    MP4_Box_data_enda_t *p_enda;
    MP4_Box_data_pcmC_t *p_pcmC;
    MP4_Box_data_keys_t *p_keys;
    MP4_Box_data_iods_t *p_iods;
    MP4_Box_data_btrt_t *p_btrt;
    MP4_Box_data_pasp_t *p_pasp;
    MP4_Box_data_clap_t *p_clap;
    MP4_Box_data_trex_t *p_trex;
    MP4_Box_data_mehd_t *p_mehd;
    MP4_Box_data_sdtp_t *p_sdtp;
    MP4_Box_data_tsel_t *p_tsel;
    MP4_Box_data_load_t *p_load;
    MP4_Box_data_vpcC_t *p_vpcC;
    MP4_Box_data_SmDm_t *p_SmDm;
    MP4_Box_data_CoLL_t *p_CoLL;
    MP4_Box_data_dvcC_t *p_dvcC;

    MP4_Box_data_tfra_t *p_tfra;
    MP4_Box_data_mfro_t *p_mfro;

    MP4_Box_data_sdp_t *p_sdp;

    MP4_Box_data_tims_t *p_tims;
    MP4_Box_data_tsro_t *p_tsro;
    MP4_Box_data_tssy_t *p_tssy;

    MP4_Box_data_stsz_t *p_stsz;
    MP4_Box_data_stsc_t *p_stsc;
    MP4_Box_data_co64_t *p_co64;
    MP4_Box_data_stss_t *p_stss;
    MP4_Box_data_stsh_t *p_stsh;
    MP4_Box_data_stdp_t *p_stdp;
    MP4_Box_data_elst_t *p_elst;
    MP4_Box_data_cprt_t *p_cprt;

    MP4_Box_data_dcom_t *p_dcom;
    MP4_Box_data_cmvd_t *p_cmvd;

    MP4_Box_data_moviehintinformation_rtp_t *p_moviehintinformation_rtp;

    MP4_Box_data_frma_t *p_frma;
    MP4_Box_data_skcr_t *p_skcr;

    MP4_Box_data_rdrf_t *p_rdrf;
    MP4_Box_data_rmdr_t *p_rmdr;
    MP4_Box_data_rmqu_t *p_rmqu;
    MP4_Box_data_rmvc_t *p_rmvc;

    MP4_Box_data_pnot_t *p_pnot;
    MP4_Box_data_chpl_t *p_chpl;
    MP4_Box_data_HMMT_t *p_hmmt;
    MP4_Box_data_trak_reference_t *p_track_reference;

    MP4_Box_data_tfrf_t *p_tfrf;
    MP4_Box_data_tfxd_t *p_tfxd;
    MP4_Box_data_WMA2_t *p_WMA2; /* flip4mac Little endian audio config */
    MP4_Box_data_strf_t *p_strf; /* flip4mac Little endian video config */
    MP4_Box_data_ASF_t  *p_asf;  /* flip4mac asf streams indicator */

    MP4_Box_data_360_t  *p_360;
    MP4_Box_data_st3d_t *p_st3d;
    MP4_Box_data_prhd_t *p_prhd;
    MP4_Box_data_equi_t *p_equi;
    MP4_Box_data_cbmp_t *p_cbmp;
    MP4_Box_data_SA3D_t *p_SA3D;

    MP4_Box_data_refbox_t *p_refbox;
    MP4_Box_data_iref_t *p_iref;
    MP4_Box_data_iloc_t *p_iloc;
    MP4_Box_data_iinf_t *p_iinf;
    MP4_Box_data_infe_t *p_infe;
    MP4_Box_data_pitm_t *p_pitm;
    MP4_Box_data_ispe_t *p_ispe; /* heif */
    MP4_Box_data_ipma_t *p_ipma; /* heif */

    /* for generic handlers */
    MP4_Box_data_binary_t *p_binary;
    MP4_Box_data_data_t *p_data;

    void                *p_payload; /* for unknown type */
} MP4_Box_data_t;

#define BOXDATA(type) type->data.type

typedef struct MP4_Box_s MP4_Box_t;
/* the most basic structure */
struct MP4_Box_s
{
    uint64_t     i_pos;      /* absolute position */

    uint32_t     i_type;
    uint32_t     i_shortsize;
    uint32_t     i_handler;  /**/
    uint32_t     i_index;    /* indexed list (ilst) */

    enum
    {
        BOX_FLAG_NONE = 0,
        BOX_FLAG_INCOMPLETE,
    }            e_flags;

    UUID_t       i_uuid;  /* Set if i_type == "uuid" */

    uint64_t     i_size; /* always set so use it */

    MP4_Box_t *p_father; /* pointer on the father Box */
    MP4_Box_t *p_first;  /* pointer on the first child Box */
    MP4_Box_t *p_last;
    MP4_Box_t *p_next;   /* pointer on the next boxes at the same level */

    void (*pf_free)( MP4_Box_t *p_box ); /* pointer to free function for this box */

    MP4_Box_data_t   data;   /* union of pointers on extended data depending
                                on i_type (or i_usertype) */
};

static inline size_t mp4_box_headersize( MP4_Box_t *p_box )
{
    return 8
        + ( p_box->i_shortsize == 1 ? 8 : 0 )
        + ( p_box->i_type == ATOM_uuid ? 16 : 0 );
}

static inline int CmpUUID( const UUID_t *u1, const UUID_t *u2 )
{
    return memcmp( u1, u2, 16 );
}

static const UUID_t TfrfBoxUUID = {
                { 0xd4, 0x80, 0x7e, 0xf2, 0xca, 0x39, 0x46, 0x95,
                  0x8e, 0x54, 0x26, 0xcb, 0x9e, 0x46, 0xa7, 0x9f } };

static const UUID_t TfxdBoxUUID = {
                { 0x6d, 0x1d, 0x9b, 0x05, 0x42, 0xd5, 0x44, 0xe6,
                  0x80, 0xe2, 0x14, 0x1d, 0xaf, 0xf7, 0x57, 0xb2 } };

static const UUID_t XML360BoxUUID = {
                { 0xff, 0xcc, 0x82, 0x63, 0xf8, 0x55, 0x4a, 0x93,
                  0x88, 0x14, 0x58, 0x7a, 0x02, 0x52, 0x1f, 0xdd } };

/* PS3 3D by HMMP video encoder */
static const UUID_t PS3DDSBoxUUID = {
                { 0x33, 0x44, 0x44, 0x53, 0x21, 0xd2, 0x4f, 0xce,
                  0xbb, 0x88, 0x69, 0x5c, 0xfa, 0xc9, 0xc7, 0x40 } };

/*****************************************************************************
 * MP4_Seek : non seekable stream safe seek
 ****************************************************************************/
int MP4_Seek( stream_t *p_stream, uint64_t i_pos );

/*****************************************************************************
 * MP4_BoxGetNextChunk : Parse the entire moof box.
 *****************************************************************************
 *  The first box is a virtual box "root" and is the father of the boxes
 *  'moof' and, possibly, 'sidx'.
 *****************************************************************************/
MP4_Box_t *MP4_BoxGetNextChunk( stream_t * );

/*****************************************************************************
 * MP4_BoxGetRoot : Parse the entire file, and create all boxes in memory
 *****************************************************************************
 *  The first box is a virtual box "root" and is the father for all first
 *  level boxes
 *****************************************************************************/
MP4_Box_t *MP4_BoxGetRoot( stream_t * );

/*****************************************************************************
 * MP4_BoxNew : Allocates a new MP4 Box with its atom type
 *****************************************************************************
 *  returns NULL on failure
 *****************************************************************************/
MP4_Box_t * MP4_BoxNew( uint32_t i_type );

/*****************************************************************************
 * MP4_FreeBox : free memory allocated after read with MP4_ReadBox
 *               or MP4_BoxGetRoot, this means also children boxes
 * XXX : all children have to be allocated by a malloc !! and
 *         p_box is freed
 *****************************************************************************/
void MP4_BoxFree( MP4_Box_t *p_box );

/*****************************************************************************
 * MP4_DumpBoxStructure: print the structure of the p_box
 *****************************************************************************
 * Useful while debugging
 *****************************************************************************/
void MP4_BoxDumpStructure( heimdall_object_t *, const MP4_Box_t *p_box );

/*****************************************************************************
 * MP4_BoxGetVa: find a box given a path relative to p_box
 *****************************************************************************
 * Path Format: . .. / as usual
 *              [number] to specifie box number ex: trak[12]
 *
 * ex: /moov/trak[12]
 *     ../mdia
 *****************************************************************************/
MP4_Box_t *MP4_BoxGetVa( const MP4_Box_t *p_box, const char *psz_fmt, ... ) HEIMDALL_FORMAT(2, 3);

/*****************************************************************************
 * MP4_BoxGet: find a box given a path relative to p_box
 *****************************************************************************
 * Path Format: . .. / as usual
 *
 * ex: /moov/mvex
 *     ../mdia
 *****************************************************************************/
MP4_Box_t *MP4_BoxGet( const MP4_Box_t *p_box, const char *psz_fmt) ;

/*****************************************************************************
 * MP4_BoxCountVa: find number of box given a path relative to p_box
 *****************************************************************************
 * Path Format: . .. / as usual
 *              [number] to specifie box number ex: trak[12]
 *
 * ex: /moov/trak
 *     ../mdia
 *****************************************************************************/
unsigned MP4_BoxCountVa( const MP4_Box_t *p_box, const char *psz_fmt, ... ) HEIMDALL_FORMAT(2, 3);

/*****************************************************************************
 * MP4_BoxCount: find number of box given a path relative to p_box
 *****************************************************************************
 * Path Format: . .. / as usual
 *
 * ex: /moov/mvex
 *     ../mdia
 *****************************************************************************/
unsigned MP4_BoxCount( const MP4_Box_t *p_box, const char *psz_fmt );

MP4_Box_t * MP4_BoxExtract( MP4_Box_t **pp_chain, uint32_t i_type );

/* Internal functions exposed for demuxers */
int MP4_ReadBoxContainerChildren( stream_t *p_stream, MP4_Box_t *p_container,
                                  const uint32_t stoplist[] );
int MP4_ReadBoxContainerRestricted( stream_t *p_stream, MP4_Box_t *p_container,
                                    const uint32_t stoplist[],
                                    const uint32_t excludelist[] );

int MP4_ReadBox_sample_vide( stream_t *p_stream, MP4_Box_t *p_box );

#ifdef __cplusplus
}
#endif

#endif
