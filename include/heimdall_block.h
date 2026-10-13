/*****************************************************************************
 * heimdall_block.h: Data blocks management functions
 *****************************************************************************
 * Copyright (C) 2003 VLC authors and VideoLAN
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

#ifndef HEIMDALL_BLOCK_H
#define HEIMDALL_BLOCK_H 1

#include <heimdall_frame.h>

/**
 * \defgroup block Blocks
 * \ingroup input
 *
 * Blocks of binary data.
 *
 * @ref block_t is a generic structure to represent a binary blob within Heimdall.
 * The primary goal of the structure is to avoid memory copying as data is
 * passed around.
 *
 * It is notably used in:
 *  - access_t
 *  - stream_t
 *  - demux_t (read block_t but send heimdall_frame_t from es_out_Send)A
 *
 * TODO: remove the heimdall_frame_t typedef and create a block_t struct like the
 * following:
 * @verbatim
 * struct block_t
 * {
 *     struct block_t *p_next;
 *     uint8_t    *p_buffer;
 *     size_t      i_buffer;
 *     uint8_t    *p_start;
 *     size_t      i_size;
 *     const struct block_callbacks *cbs;
 * } @endverbatim
 */

#define BLOCK_FLAG_DISCONTINUITY HEIMDALL_FRAME_FLAG_DISCONTINUITY

#define BLOCK_FLAG_TYPE_I HEIMDALL_FRAME_FLAG_TYPE_I
#define BLOCK_FLAG_TYPE_P HEIMDALL_FRAME_FLAG_TYPE_P
#define BLOCK_FLAG_TYPE_B HEIMDALL_FRAME_FLAG_TYPE_B
#define BLOCK_FLAG_TYPE_PB HEIMDALL_FRAME_FLAG_TYPE_PB
#define BLOCK_FLAG_HEADER HEIMDALL_FRAME_FLAG_HEADER
#define BLOCK_FLAG_END_OF_SEQUENCE HEIMDALL_FRAME_FLAG_END_OF_SEQUENCE
#define BLOCK_FLAG_SCRAMBLED HEIMDALL_FRAME_FLAG_SCRAMBLED
#define BLOCK_FLAG_PREROLL HEIMDALL_FRAME_FLAG_PREROLL
#define BLOCK_FLAG_CORRUPTED HEIMDALL_FRAME_FLAG_CORRUPTED
#define BLOCK_FLAG_AU_END HEIMDALL_FRAME_FLAG_AU_END
#define BLOCK_FLAG_TOP_FIELD_FIRST HEIMDALL_FRAME_FLAG_TOP_FIELD_FIRST
#define BLOCK_FLAG_BOTTOM_FIELD_FIRST HEIMDALL_FRAME_FLAG_BOTTOM_FIELD_FIRST
#define BLOCK_FLAG_SINGLE_FIELD HEIMDALL_FRAME_FLAG_SINGLE_FIELD
#define BLOCK_FLAG_INTERLACED_MASK HEIMDALL_FRAME_FLAG_INTERLACED_MASK
#define BLOCK_FLAG_TYPE_MASK HEIMDALL_FRAME_FLAG_TYPE_MASK
#define BLOCK_FLAG_CORE_PRIVATE_MASK HEIMDALL_FRAME_FLAG_CORE_PRIVATE_MASK
#define BLOCK_FLAG_CORE_PRIVATE_SHIFT HEIMDALL_FRAME_FLAG_CORE_PRIVATE_SHIFT
#define BLOCK_FLAG_PRIVATE_MASK HEIMDALL_FRAME_FLAG_PRIVATE_MASK
#define BLOCK_FLAG_PRIVATE_SHIFT HEIMDALL_FRAME_FLAG_PRIVATE_SHIFT

#define heimdall_block_callbacks heimdall_frame_callbacks

#define block_Init heimdall_frame_Init
#define block_New heimdall_frame_New
#define block_Alloc heimdall_frame_Alloc
#define block_TryRealloc heimdall_frame_TryRealloc
#define block_Realloc heimdall_frame_Realloc
#define block_Release heimdall_frame_Release
#define block_CopyProperties heimdall_frame_CopyProperties
#define block_Duplicate heimdall_frame_Duplicate
#define block_heap_Alloc heimdall_frame_heap_Alloc
#define block_mmap_Alloc heimdall_frame_mmap_Alloc
#define block_shm_Alloc heimdall_frame_shm_Alloc
#define block_File heimdall_frame_File
#define block_FilePath heimdall_frame_FilePath
#define block_Cleanup heimdall_frame_Cleanup
#define block_cleanup_push heimdall_frame_cleanup_push
#define block_ChainAppend heimdall_frame_ChainAppend
#define block_ChainLastAppend heimdall_frame_ChainLastAppend
#define block_ChainRelease heimdall_frame_ChainRelease
#define block_ChainExtract heimdall_frame_ChainExtract
#define block_ChainProperties heimdall_frame_ChainProperties
#define block_ChainGather heimdall_frame_ChainGather

#define block_FifoPut heimdall_fifo_Put
#define block_FifoNew heimdall_fifo_New
#define block_FifoRelease heimdall_fifo_Delete
#define block_FifoSize heimdall_fifo_Size
#define block_FifoGet heimdall_fifo_Get
#define block_FifoCount heimdall_fifo_Count
#define block_FifoEmpty heimdall_fifo_Empty
#define block_FifoShow heimdall_fifo_Show

#endif /* HEIMDALL_BLOCK_H */
