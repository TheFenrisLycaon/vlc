/*****************************************************************************
 * heimdall_chroma_probe.h: chroma conversion probing
 *****************************************************************************
 * Copyright (C) 2025 VLC authors and VideoLAN
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

#ifndef HEIMDALL_CHROMA_PROBE_H
#define HEIMDALL_CHROMA_PROBE_H 1

#include <heimdall_common.h>
#include <heimdall_vector.h>

/**
 * \defgroup chroma_probe Chroma conversion probing
 * \ingroup filter
 * @{
 * \file
 * Chroma conversion probing
 *
 * \defgroup chroma_probe_api Chroma probing API
 * \ingroup chroma_probe
 *
 * @{
 */

#define HEIMDALL_CHROMA_CONV_MAX_INDIRECT_STEPS 1
#define HEIMDALL_CHROMA_CONV_CHAIN_COUNT_MAX (2 /* in + out */ + HEIMDALL_CHROMA_CONV_MAX_INDIRECT_STEPS)

/**
 * Chroma conversion result structure
 */
struct heimdall_chroma_conv_result
{
    /**
     * Array of chromas used to achieve the conversion
     *
     * 'chain[0]' is always equals to the 'in' argument of the
     * heimdall_chroma_conv_Probe() function.
     *
     * if the out argument of the heimdall_chroma_conv_Probe() is valid,
     * chain[chain_count - 1] is equals to 'out'
     */
    heimdall_fourcc_t chain[HEIMDALL_CHROMA_CONV_CHAIN_COUNT_MAX];

    /** Number of chromas in the chain */
    size_t chain_count;

    /**
     * Cost of the full conversion, lower is better.
     */
    unsigned cost;

    /**
     * Quality of the conversion, higher is better.
     *
     * A quality of 100 means there are no quality loss: same color size and
     * same heimdall_chroma_subtype (or same YUV subsampling for video).
     */
    unsigned quality;
};

/** Only accept YUV output chromas (the input chroma can be RGB) */
#define HEIMDALL_CHROMA_CONV_FLAG_ONLY_YUV 0x1
/** Only accept RGB output chromas (the input chroma can be YUV) */
#define HEIMDALL_CHROMA_CONV_FLAG_ONLY_RGB 0x2
/** Sort results by cost instead of quality */
#define HEIMDALL_CHROMA_CONV_FLAG_SORT_COST  0x4

/**
 * Probe possible chroma conversions

 * Results are sorted by quality, unless HEIMDALL_CHROMA_CONV_FLAG_SORT_COST is
 * specified in flags.

 * @param in the input chroma to convert from, must be valid
 * @param out the output chroma to convert to, if 0, the function will find all
 * possible conversion from in to x
 * @param width video width, used for finer cost calculation, can be 0
 * @param height video height, used for finer cost calculation, can be 0
 * @param max_indirect_steps maximum number of indirect conversion steps, must
 * be lower or equal to @ref HEIMDALL_CHROMA_CONV_MAX_INDIRECT_STEPS, if in and out
 * chromas are CPU chromas, the steps will be automatically lowered to 0
 * @param flags bitwise flags, cf. HEIMDALL_CHROMA_CONV_FLAG_*
 * @param count pointer to the number of results, must be valid
 * @return a pointer to an array of results, must be released with free(), can
 * be NULL
 */
HEIMDALL_API struct heimdall_chroma_conv_result *
heimdall_chroma_conv_Probe(heimdall_fourcc_t in, heimdall_fourcc_t out,
                      unsigned width, unsigned height,
                      unsigned max_indirect_steps, int flags, size_t *count);

/**
 * Get a string representing the result
 *
 * @param res pointer to a valid result
 * @return a string or NULL, must be released with free()
 */
HEIMDALL_API char *
heimdall_chroma_conv_result_ToString(const struct heimdall_chroma_conv_result *res);

/**
 * @}
 *
 * \defgroup chroma_probe_module Chroma probing module implementation
 * \ingroup chroma_probe
 *
 * @{
 */

/**
 * Chroma conversion entry structure
 */
struct heimdall_chroma_conv_entry
{
    /** Cost factor, 0.25 for GPU<->GPU conversions, 0.75 for SIMD, 1 for CPU */
    float cost_factor;
    /** input chroma */
    heimdall_fourcc_t in;
    /** output chroma */
    heimdall_fourcc_t out;
};
typedef struct HEIMDALL_VECTOR(struct heimdall_chroma_conv_entry) heimdall_chroma_conv_vec;

/**
 * Module probe function signature
 *
 * @param vec pointer to an allocated vector
 * @return a Heimdall error code
 */
typedef void (*heimdall_chroma_conv_probe)(heimdall_chroma_conv_vec *vec);

#define set_callback_chroma_conv_probe(activate) \
    { \
        heimdall_chroma_conv_probe activate__ = activate; \
        (void) activate__; \
        set_callback(activate) \
    } \
    set_capability("chroma probe", 100)

/**
 * Helper that add a chroma conversion
 *
 * Must be called inside heimdall_chroma_conv_probe()
 *
 * @param vec pointer to the vector of chromas
 * @param cost_factor cf. heimdall_chroma_conv_entry.cost_factor
 * @param in cf. heimdall_chroma_conv_entry.in
 * @param out cf. heimdall_chroma_conv_entry.out
 * @param twoway if true, 'out' can also be converted to 'in'
 */
static inline void
heimdall_chroma_conv_add(heimdall_chroma_conv_vec *vec, float cost_factor,
                    heimdall_fourcc_t in, heimdall_fourcc_t out, bool twoway)
{
    {
        const struct heimdall_chroma_conv_entry entry = {
            cost_factor, in, out
        };
        heimdall_vector_push(vec, entry);
    }

    if (twoway)
    {
        const struct heimdall_chroma_conv_entry entry = {
            cost_factor, out, in
        };
        heimdall_vector_push(vec, entry);
    }
}

/**
 * Helper that add an array of out chroma conversions
 *
 * Must be called inside heimdall_chroma_conv_probe()
 *
 * @param vec pointer to the vector of chromas
 * @param cost_factor cf. heimdall_chroma_conv_entry.cost_factor
 * @param in cf. heimdall_chroma_conv_entry.in
 * @param out_array a list of out chromas
 * @param out_count number of elements in the out_array
 */
static inline void
heimdall_chroma_conv_add_in_outarray(heimdall_chroma_conv_vec *vec, float cost_factor,
                                heimdall_fourcc_t in,
                                const heimdall_fourcc_t *out_array, size_t out_count)
{
    for (size_t i = 0; i < out_count; i++)
    {
        const struct heimdall_chroma_conv_entry entry = {
            cost_factor, in, out_array[i],
        };
        heimdall_vector_push(vec, entry);
    }
}

/**
 * Helper that add a list of out chroma conversions
 */
#define heimdall_chroma_conv_add_in_outlist(vec, cost_factor, in, ...) do { \
    static const heimdall_fourcc_t out_array[] = { __VA_ARGS__ }; \
    size_t count = ARRAY_SIZE(out_array); \
    heimdall_chroma_conv_add_in_outarray(vec, cost_factor, in, out_array, count); \
} while(0)

/**
 * Helper that add an array of in chroma conversions
 *
 * Must be called inside heimdall_chroma_conv_probe()
 *
 * @param vec pointer to the vector of chromas
 * @param cost_factor cf. heimdall_chroma_conv_entry.cost_factor
 * @param out cf. heimdall_chroma_conv_entry.out
 * @param in_array a list of out chromas
 * @param in_count number of elements in the in_array
 */
static inline void
heimdall_chroma_conv_add_out_inarray(heimdall_chroma_conv_vec *vec, float cost_factor,
                                heimdall_fourcc_t out,
                                const heimdall_fourcc_t *in_array, size_t in_count)
{
    for (size_t i = 0; i < in_count; i++)
    {
        const struct heimdall_chroma_conv_entry entry = {
            cost_factor, in_array[i], out,
        };
        heimdall_vector_push(vec, entry);
    }
}

/**
 * Helper that add a list of in chroma conversions
 */
#define heimdall_chroma_conv_add_out_inlist(vec, cost_factor, out, ...) do { \
    static const heimdall_fourcc_t in_array[] = { __VA_ARGS__ }; \
    size_t count = ARRAY_SIZE(in_array); \
    heimdall_chroma_conv_add_out_inarray(vec, cost_factor, out, in_array, count); \
} while(0)

/**
 * @}
 * @}
 */

#endif /* HEIMDALL_CHROMA_PROBE_H */
