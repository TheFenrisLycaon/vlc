/*****************************************************************************
 * heimdall_ancillary.h: ancillary management functions
 *****************************************************************************
 * Copyright (C) 2021 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
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

#ifndef HEIMDALL_ANCILLARY_H
#define HEIMDALL_ANCILLARY_H 1

#include <heimdall_vector.h>
#include <stdint.h>

/**
 * \defgroup ancillary Ancillary
 * \ingroup input
 *
 * Ancillary that can be attached to any heimdall_frame_t or picture_t.
 *
 * Ancillaries can be created from:
 *  - packetized demuxer modules,
 *  - packetizer modules,
 *  - decoder modules.
 *
 *  @warning Ancillaries should not be attached from a non packetized demuxer
 *  module since the attachment to the heimdall_frame will be lost by the packetizer
 *  module that will be automatically inserted.
 *
 * Ancillaries are automatically forwarded from a heimdall_frame_t to an other
 * heimdall_frame_t and from a picture_t to an other picture_t. This allow to keep
 * ancillaries untouched when audio filters or video filters are used (these
 * filters don't have to know about the ancillary).
 *
 * Ancillary readers can be either:
 *  - A decoder module,
 *  - An audio output,
 *  - A video output,
 *  - A video or audio filter.
 *
 * @{
 * \file
 * Ancillary definition and functions
 * \defgroup ancillary_api Ancillary API
 * @{
 */

/**
 * Ancillary opaque struct, refcounted struct that hold user data with a free
 * callback.
 */
struct heimdall_ancillary;

typedef struct HEIMDALL_VECTOR(struct heimdall_ancillary *) heimdall_ancillary_array;
#define HEIMDALL_ANCILLARY_ARRAY_INITIALIZER HEIMDALL_VECTOR_INITIALIZER

/**
 * ID of an ancillary. Each ancillary user can create its own unique ID via
 * HEIMDALL_ANCILLARY_ID.
 */
typedef uint32_t heimdall_ancillary_id;
#define HEIMDALL_ANCILLARY_ID(a,b,c,d) HEIMDALL_FOURCC(a,b,c,d)

/**
 * Callback to free an ancillary data
 */
typedef void (*heimdall_ancillary_free_cb)(void *data);

/**
 * Create an ancillary
 *
 * @param data an opaque ancillary, can't be NULL
 * @param id id of ancillary
 * @param free_cb callback to release the data, can be NULL
 * @return a valid heimdall_ancillary pointer or NULL in case of allocation error
 */
HEIMDALL_API struct heimdall_ancillary *
heimdall_ancillary_CreateWithFreeCb(void *data, heimdall_ancillary_id id,
                               heimdall_ancillary_free_cb free_cb);

/**
 * Helper to create an ancillary holding an allocated data
 */
static inline struct heimdall_ancillary *
heimdall_ancillary_Create(void *data, heimdall_ancillary_id id)
{
    return heimdall_ancillary_CreateWithFreeCb(data, id, free);
}

/**
 * Release an ancillary
 *
 * If the refcount reaches 0, the free_cb provided by
 * heimdall_ancillary_CreateWithFreeCb() is called.
 *
 * @param ancillary ancillary to release
 */
HEIMDALL_API void
heimdall_ancillary_Release(struct heimdall_ancillary *ancillary);

/**
 * Hold an ancillary
 *
 * @param ancillary ancillary to hold
 * @return the same ancillary
 */
HEIMDALL_API struct heimdall_ancillary *
heimdall_ancillary_Hold(struct heimdall_ancillary *ancillary);

/**
 * Get the data of the ancillary
 *
 * @param ancillary ancillary to get data from
 * @return data used when created the ancillary, same lifetime than the ancillary
 */
HEIMDALL_API void *
heimdall_ancillary_GetData(const struct heimdall_ancillary *ancillary);

/**
 * @}
 * \defgroup ancillary_array Ancillary array API
 * @{
 */

/**
 * Init an ancillary array
 *
 * @param array pointer to the ancillary array to initialize
 */
static inline void
heimdall_ancillary_array_Init(heimdall_ancillary_array *array)
{
    heimdall_vector_init(array);
}

/**
 * Clear an ancillary array
 *
 * This will release the refcount on all ancillaries and free the vector data
 *
 * @param array pointer to the ancillary array to clear
 */
HEIMDALL_API void
heimdall_ancillary_array_Clear(heimdall_ancillary_array *array);

/**
 * Merge two ancillary arrays
 *
 * Copy all ancillaries from src_array to dst_array, preserving all previous
 * ancillaries. In case of ancillary id conflict, the one from src_array will
 * have precedence.
 *
 * @param dst_array pointer to an initialized ancillary array, if not empty,
 * previous ancillaries will be preserved.
 * @param src_array pointer to the source ancillary array
 * @return HEIMDALL_SUCCESS in case of success, HEIMDALL_ENOMEM in case of alloc error
 */
HEIMDALL_API int
heimdall_ancillary_array_Merge(heimdall_ancillary_array *dst_array,
                          const heimdall_ancillary_array *src_array);

/**
 * Merge and clear two ancillary arrays
 *
 * The src array will be moved to the dst array if the dst array is empty (fast
 * path). Otherwise, both arrays will be merged into dst_array and the
 * src_array will be cleared afterward.
 *
 * @param dst_array pointer to a valid ancillary array, if not empty, previous
 * ancillaries will be preserved.
 * @param src_array pointer to the source ancillary array, will point to empty
 * data after this call.
 * @return HEIMDALL_SUCCESS in case of success, HEIMDALL_ENOMEM in case of alloc error
 */
HEIMDALL_API int
heimdall_ancillary_array_MergeAndClear(heimdall_ancillary_array *dst_array,
                                  heimdall_ancillary_array *src_array);

/**
 * Insert a new ancillary in the array
 *
 * @note Several ancillaries can be attached to an array, but if two ancillaries
 * are identified by the same ID, only the last one take precedence.
 *
 * @param array pointer to the ancillary array
 * @param ancillary pointer to the ancillary to add
 * @return HEIMDALL_SUCCESS in case of success, HEIMDALL_ENOMEM in case of alloc error
 */
HEIMDALL_API int
heimdall_ancillary_array_Insert(heimdall_ancillary_array *array,
                           struct heimdall_ancillary *ancillary);

/**
 * Get a specific ancillary from the array
 *
 * @param array pointer to the ancillary array
 * @param id id of the ancillary
 * @return a valid ancillary or NULL if not found, no need to release it.
 */
HEIMDALL_API struct heimdall_ancillary *
heimdall_ancillary_array_Get(const heimdall_ancillary_array *array,
                        heimdall_ancillary_id id);

/**
 * @}
 * \defgroup ancillary_data Ancillary IDs and data
 * @{
 */

/**
 * Dolby Vision metadata description
 */
enum heimdall_dovi_reshape_method_t
{
    HEIMDALL_DOVI_RESHAPE_POLYNOMIAL = 0,
    HEIMDALL_DOVI_RESHAPE_MMR = 1,
};

enum heimdall_dovi_nlq_method_t
{
    HEIMDALL_DOVI_NLQ_NONE = -1,
    HEIMDALL_DOVI_NLQ_LINEAR_DZ = 0,
};

#define HEIMDALL_ANCILLARY_ID_DOVI HEIMDALL_FOURCC('D','o','V','i')

typedef struct heimdall_video_dovi_metadata_t
{
    /* Common header fields */
    uint8_t coef_log2_denom;
    uint8_t bl_bit_depth;
    uint8_t el_bit_depth;
    enum heimdall_dovi_nlq_method_t nlq_method_idc;

    /* Colorspace metadata */
    float nonlinear_offset[3];
    float nonlinear_matrix[9];
    float linear_matrix[9];
    uint16_t source_min_pq; /* 12-bit PQ values */
    uint16_t source_max_pq;

    /**
     * Do not reorder or modify the following structs, they are intentionally
     * specified to be identical to AVDOVIReshapingCurve / AVDOVINLQParams.
     */
    struct heimdall_dovi_reshape_t {
        uint8_t num_pivots;
        uint16_t pivots[9];
        enum heimdall_dovi_reshape_method_t mapping_idc[8];
        uint8_t poly_order[8];
        int64_t poly_coef[8][3];
        uint8_t mmr_order[8];
        int64_t mmr_constant[8];
        int64_t mmr_coef[8][3][7];
    } curves[3];

    struct heimdall_dovi_nlq_t {
        uint8_t offset_depth; /* bit depth of offset value */
        uint16_t offset;
        uint64_t hdr_in_max;
        uint64_t dz_slope;
        uint64_t dz_threshold;
    } nlq[3];
} heimdall_video_dovi_metadata_t;

/**
 * HDR10+ Dynamic metadata (based on ATSC A/341 Amendment 2094-40)
 *
 * This is similar to SMPTE ST2094-40:2016, but omits the mastering display and
 * target display actual peak luminance LUTs, the rectangular boundaries and
 * ellipse coefficients, and support for multiple processing windows, as these
 * are intentionally left unused in this version of the specification.
 */

#define HEIMDALL_ANCILLARY_ID_HDR10PLUS HEIMDALL_FOURCC('H','D','R','+')

typedef struct heimdall_video_hdr_dynamic_metadata_t
{
    uint8_t country_code;           /* ITU-T T.35 Annex A */
    uint8_t application_version;
    float targeted_luminance;       /* in cd/m² */

    /* parameters for the first processing window (encompassing the frame) */
    float maxscl[3];                /* in linearized range [0,1] */
    float average_maxrgb;           /* in linearized range [0,1] */
    uint8_t num_histogram;          /* in range [0,15] */
    struct {
        uint8_t percentage;         /* in range [1,100] */
        float percentile;           /* in linearized range [0,1] */
    } histogram[15];
    float fraction_bright_pixels;/* in range [0,1] */
    uint8_t tone_mapping_flag;
    float knee_point_x;             /* in ootf range [0,1] */
    float knee_point_y;             /* in ootf range [0,1] */
    uint8_t num_bezier_anchors;     /* in range [1,15] */
    float bezier_curve_anchors[15]; /* in range [0,1] */
} heimdall_video_hdr_dynamic_metadata_t;

/**
 * Embedded ICC profiles
 */

#define HEIMDALL_ANCILLARY_ID_ICC HEIMDALL_FOURCC('i','C','C','P')

typedef struct heimdall_icc_profile_t
{
    size_t size;
    uint8_t data[]; /* binary profile data, see ICC.1:2022 (or later) */
} heimdall_icc_profile_t;

/**
 * VPx alpha data
 */

#define HEIMDALL_ANCILLARY_ID_VPX_ALPHA HEIMDALL_FOURCC('v','p','x','A')

typedef struct heimdall_vpx_alpha_t
{
    size_t  size;
    uint8_t *data;
} heimdall_vpx_alpha_t;

/**
 * @}
 * @}
 */
#endif /* HEIMDALL_ANCILLARY_H */
