/*****************************************************************************
 * chroma_probe.c: test for chroma_probe
 *****************************************************************************
 * Copyright (C) 2025 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Define a builtin module for mocked parts */
#define MODULE_NAME test_chroma_probe
#undef HEIMDALL_DYNAMIC_PLUGIN
#include "../../libheimdall/test.h"

#include <heimdall/heimdall.h>

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_chroma_probe.h>
#include <heimdall_fourcc.h>

#include <assert.h>

const char heimdall_module_name[] = MODULE_STRING;

#define RESULT_MAX 6
struct scenario_result
{
    unsigned cost;
    unsigned quality;
    heimdall_fourcc_t chain[HEIMDALL_CHROMA_CONV_CHAIN_COUNT_MAX - 1 /* exclude 'from' */];
};

struct scenario
{
    unsigned max_indirect_steps;
    int flags;
    heimdall_fourcc_t in;
    heimdall_fourcc_t out;
    struct scenario_result results[RESULT_MAX];
    size_t result_count;
};

static const struct scenario scenario_array[] =
{
#define COST HEIMDALL_CHROMA_CONV_FLAG_SORT_COST
#define ONLY_YUV HEIMDALL_CHROMA_CONV_FLAG_ONLY_YUV
#define ONLY_RGB HEIMDALL_CHROMA_CONV_FLAG_ONLY_RGB
#define RESULT(cost, quality, chain0, chain1 ) \
    { cost, quality, { chain0, chain1 } }

#define SCENARIO0(max_indirect_steps_, from, to) { \
    .max_indirect_steps = max_indirect_steps_, \
    .flags = 0, \
    .in = from, .out = to, \
    .result_count = 0, \
}

#define SCENARIO1(max_indirect_steps_, sort_, from, to, cost, quality, chain0) { \
    .max_indirect_steps = max_indirect_steps_, \
    .flags = sort_, \
    .in = from, .out = to, \
    .results = { RESULT(cost, quality, chain0, 0) }, \
    .result_count = 1, \
}

#define SCENARIO2(max_indirect_steps_, sort_, from, to, \
                  result0_cost, result0_quality, result0_chain0, \
                  result1_cost, result1_quality, result1_chain0) { \
    .max_indirect_steps = max_indirect_steps_, \
    .flags = sort_, \
    .in = from, .out = to, \
    .results = { RESULT(result0_cost, result0_quality, result0_chain0, 0), \
                 RESULT(result1_cost, result1_quality, result1_chain0, 0), }, \
    .result_count = 2, \
}

#define SCENARIOX(max_indirect_steps_, sort_, from, to, count, ...) { \
    .max_indirect_steps = max_indirect_steps_, \
    .flags = sort_, \
    .in = from, .out = to, \
    .results = { __VA_ARGS__ }, \
    .result_count = count, \
}

    /* Success with a depth of 0 (Direct conversion) */
    SCENARIO1(0, 0, HEIMDALL_CODEC_VAAPI_420, HEIMDALL_CODEC_I420, 26, 100, 0),

    /* Success with a depth of 1 */
    SCENARIO1(1, 0, HEIMDALL_CODEC_VAAPI_420_10BPP, HEIMDALL_CODEC_I420,
              47, 80, HEIMDALL_CODEC_P010),

    /* Fail because it require a depth of 1 */
    SCENARIO0(0, HEIMDALL_CODEC_VAAPI_420_10BPP, HEIMDALL_CODEC_I420),

    /* Check duplicated entries are removed and that we keep the lowest cost */
    SCENARIO1(1, 0, HEIMDALL_CODEC_NV12, HEIMDALL_CODEC_I420,
              18, 100, 0),

    /* Check two_way is doing as expected */
    SCENARIO1(1, 0, HEIMDALL_CODEC_I420, HEIMDALL_CODEC_VAAPI_420_10BPP,
              47, 100, HEIMDALL_CODEC_P010),

    /* Fail because it requires a depth of 2 */
    SCENARIO0(1, HEIMDALL_CODEC_CVPX_P010, HEIMDALL_CODEC_P010),

    /* Fail because conversion is not two-way */
    SCENARIO0(1, HEIMDALL_CODEC_P010, HEIMDALL_CODEC_CVPX_P010),

    /* Check low cost of GPU <-> GPU */
    SCENARIO1(1, 0, HEIMDALL_CODEC_CVPX_P010, HEIMDALL_CODEC_CVPX_BGRA, 11, 80, 0),

    /* Check cost and quality of direct conversion */
    SCENARIO1(0, 0, HEIMDALL_CODEC_YUVA_444_12L, HEIMDALL_CODEC_I420, 60, 33, 0),

    /* Check 1 depth conversions are correctly sorted */
    SCENARIOX(1, 0, HEIMDALL_CODEC_VAAPI_420_10BPP, 0, 6,
              RESULT(33, 100, HEIMDALL_CODEC_P010, 0),
              RESULT(33, 100, HEIMDALL_CODEC_I420_10L, 0),
              RESULT(66, 100, HEIMDALL_CODEC_P010, HEIMDALL_CODEC_I420_10L),
              RESULT(66, 100, HEIMDALL_CODEC_I420_10L, HEIMDALL_CODEC_P010),
              RESULT(47, 80, HEIMDALL_CODEC_P010, HEIMDALL_CODEC_I420),
              RESULT(84, 72, HEIMDALL_CODEC_P010, HEIMDALL_CODEC_RGBA)),

    /* Check default QUALITY order */
    SCENARIOX(0, 0, HEIMDALL_CODEC_YUVA_444_12L, 0, 4,
              RESULT(112, 90, HEIMDALL_CODEC_RGBA64, 0),
              RESULT(88, 83, HEIMDALL_CODEC_YUVA_444_10L, 0),
              RESULT(80, 60, HEIMDALL_CODEC_RGBA, 0),
              RESULT(60, 33, HEIMDALL_CODEC_I420, 0)),

    /* Check ONLY_YUV */
    SCENARIOX(0, ONLY_YUV, HEIMDALL_CODEC_YUVA_444_12L, 0, 2,
              RESULT(88, 83, HEIMDALL_CODEC_YUVA_444_10L, 0),
              RESULT(60, 33, HEIMDALL_CODEC_I420, 0)),

    /* Check ONLY_RGB */
    SCENARIOX(0, ONLY_RGB, HEIMDALL_CODEC_YUVA_444_12L, 0, 2,
              RESULT(112, 90, HEIMDALL_CODEC_RGBA64, 0),
              RESULT(80, 60,  HEIMDALL_CODEC_RGBA, 0)),

    /* Check COST order */
    SCENARIOX(0, COST, HEIMDALL_CODEC_YUVA_444_12L, 0, 4,
              RESULT(60, 33, HEIMDALL_CODEC_I420, 0),
              RESULT(80, 60,  HEIMDALL_CODEC_RGBA, 0),
              RESULT(88, 83, HEIMDALL_CODEC_YUVA_444_10L, 0),
              RESULT(112, 90, HEIMDALL_CODEC_RGBA64, 0)),

    /* Check HEIMDALL_CHROMA_CONV_ADD_IN_OUTTLIST, and quality order with smaller
     * RGB chromas */
    SCENARIOX(0, 0, HEIMDALL_CODEC_YV12, 0, 5,
              RESULT(36, 90, HEIMDALL_CODEC_XRGB, 0),
              RESULT(28, 59, HEIMDALL_CODEC_RGB565, 0),
              RESULT(28, 59, HEIMDALL_CODEC_BGR565, 0),
              RESULT(27, 56, HEIMDALL_CODEC_RGB555, 0),
              RESULT(27, 56, HEIMDALL_CODEC_BGR555, 0)),

    /* Check HEIMDALL_CHROMA_CONV_ADD_OUT_INLIST */
    SCENARIO1(0, 0, HEIMDALL_CODEC_NV16, HEIMDALL_CODEC_I422, 32, 100, 0),
    SCENARIO1(0, 0, HEIMDALL_CODEC_YUYV, HEIMDALL_CODEC_I422, 32, 100, 0),
    SCENARIO1(0, 0, HEIMDALL_CODEC_UYVY, HEIMDALL_CODEC_I422, 32, 100, 0),

    /* Check HEIMDALL_CHROMA_CONV_ADD_ALL */
    SCENARIO1(1, 0, HEIMDALL_CODEC_I444_12L, HEIMDALL_CODEC_I444, 60, 66, 0),
    SCENARIO1(1, 0, HEIMDALL_CODEC_I444, HEIMDALL_CODEC_I444_12L, 60, 100, 0),
    SCENARIO1(1, 0, HEIMDALL_CODEC_I444_12L, HEIMDALL_CODEC_I444_10L, 66, 83, 0),
    SCENARIO1(1, 0, HEIMDALL_CODEC_I444_10L, HEIMDALL_CODEC_I444_12L, 66, 100, 0),
};

static void ProbeChroma(heimdall_chroma_conv_vec *vec)
{
    heimdall_chroma_conv_add(vec, 1.1, HEIMDALL_CODEC_VAAPI_420, HEIMDALL_CODEC_I420, true);
    heimdall_chroma_conv_add(vec, 1.1, HEIMDALL_CODEC_VAAPI_420_10BPP, HEIMDALL_CODEC_P010, true);
    heimdall_chroma_conv_add(vec, 1.1, HEIMDALL_CODEC_VAAPI_420_10BPP, HEIMDALL_CODEC_I420_10L, true);

    heimdall_chroma_conv_add(vec, 0.75, HEIMDALL_CODEC_I420, HEIMDALL_CODEC_NV12, true);
    heimdall_chroma_conv_add(vec, 0.75, HEIMDALL_CODEC_I420, HEIMDALL_CODEC_P010, true);
    heimdall_chroma_conv_add(vec, 1, HEIMDALL_CODEC_I420_10L, HEIMDALL_CODEC_P010, true);
    heimdall_chroma_conv_add(vec, 1, HEIMDALL_CODEC_RGBA, HEIMDALL_CODEC_P010, true);
    heimdall_chroma_conv_add(vec, 1, HEIMDALL_CODEC_RGBA, HEIMDALL_CODEC_NV12, true);

    /* Test duplicated entries are removed */
    heimdall_chroma_conv_add(vec, 1.0, HEIMDALL_CODEC_I420, HEIMDALL_CODEC_NV12, true);

    /* Don't change this order as this is used to test to cost sort
     * (we don't want the result to be naturally sorted) */
    heimdall_chroma_conv_add(vec, 1, HEIMDALL_CODEC_YUVA_444_12L, HEIMDALL_CODEC_RGBA, true);
    heimdall_chroma_conv_add(vec, 1, HEIMDALL_CODEC_YUVA_444_12L, HEIMDALL_CODEC_I420, true);

    heimdall_chroma_conv_add(vec, 1, HEIMDALL_CODEC_YUVA_444_12L, HEIMDALL_CODEC_YUVA_444_10L, true);
    heimdall_chroma_conv_add(vec, 1, HEIMDALL_CODEC_YUVA_444_12L, HEIMDALL_CODEC_RGBA64, true);

    heimdall_chroma_conv_add(vec, 0.25, HEIMDALL_CODEC_CVPX_NV12, HEIMDALL_CODEC_CVPX_BGRA, false);
    heimdall_chroma_conv_add(vec, 1.1, HEIMDALL_CODEC_CVPX_NV12, HEIMDALL_CODEC_NV12, false);
    heimdall_chroma_conv_add(vec, 0.25, HEIMDALL_CODEC_CVPX_P010, HEIMDALL_CODEC_CVPX_BGRA, false);
    heimdall_chroma_conv_add(vec, 1.1, HEIMDALL_CODEC_CVPX_BGRA, HEIMDALL_CODEC_RGBA, false);

    heimdall_chroma_conv_add_in_outlist(vec, 1, HEIMDALL_CODEC_YV12, HEIMDALL_CODEC_XRGB,
                                    HEIMDALL_CODEC_RGB565, HEIMDALL_CODEC_BGR565,
                                    HEIMDALL_CODEC_RGB555, HEIMDALL_CODEC_BGR555);

    heimdall_chroma_conv_add_out_inlist(vec, 1, HEIMDALL_CODEC_I422, HEIMDALL_CODEC_NV16,
            HEIMDALL_CODEC_YUYV, HEIMDALL_CODEC_UYVY);

    heimdall_chroma_conv_add(vec, 1, HEIMDALL_CODEC_I444, HEIMDALL_CODEC_I444_10L, true);
    heimdall_chroma_conv_add(vec, 1, HEIMDALL_CODEC_I444, HEIMDALL_CODEC_I444_12L, true);
    heimdall_chroma_conv_add(vec, 1, HEIMDALL_CODEC_I444_10L, HEIMDALL_CODEC_I444_12L, true);

    /* Test duplicated entries are removed */
    heimdall_chroma_conv_add(vec, 1.0, HEIMDALL_CODEC_I420, HEIMDALL_CODEC_NV12, true);
}

heimdall_module_begin()
    set_callback_chroma_conv_probe(ProbeChroma)
heimdall_module_end()

HEIMDALL_EXPORT heimdall_plugin_cb heimdall_static_modules[] = {
    HEIMDALL_SYMBOL(heimdall_entry),
    NULL
};

static void
print_results(const struct heimdall_chroma_conv_result *array, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        const struct heimdall_chroma_conv_result *res = &array[i];
        char *res_str = heimdall_chroma_conv_result_ToString(res);
        assert(res_str != NULL);
        fprintf(stderr, "\tres[%zu]: %s\n", i, res_str);
        free(res_str);
    }
}

static void
check_results(const struct scenario *scr,
              const struct heimdall_chroma_conv_result *results)
{
    for (size_t i = 0; i < scr->result_count; ++i)
    {
        const struct heimdall_chroma_conv_result *result = &results[i];
        const struct scenario_result *scr_result = &scr->results[i];

        assert(result->chain_count > 1);
        assert(result->chain_count <= HEIMDALL_CHROMA_CONV_CHAIN_COUNT_MAX);

        /*  Reconstruct the expected fourcc array from the scenario */
        heimdall_fourcc_t scr_chain[HEIMDALL_CHROMA_CONV_CHAIN_COUNT_MAX];
        bool end_reached = false;
        size_t scr_count = 1;
        scr_chain[0] = scr->in;
        for (size_t j = 1; j < HEIMDALL_CHROMA_CONV_CHAIN_COUNT_MAX; ++j)
        {
            if (end_reached)
            {
                scr_chain[j] = 0;
                continue;
            }

            if (scr_result->chain[j - 1] != 0)
            {
                scr_chain[j] = scr_result->chain[j - 1];
                scr_count++;
            }
            else
            {
                if (scr->out != 0)
                {
                    scr_chain[j] = scr->out;
                    scr_count++;
                }
                end_reached = true;
            }
        }

        assert(scr_count == result->chain_count);
        size_t j;
        for (j = 0; j < result->chain_count; ++j)
            assert(result->chain[j] == scr_chain[j]);
        for (; j < HEIMDALL_CHROMA_CONV_CHAIN_COUNT_MAX - 1; ++j)
            assert(scr_result->chain[j - 1] == 0);

        assert(result->cost == scr_result->cost);
        assert(result->quality == scr_result->quality);
    }
}

int main(int argc, const char *argv[])
{
    test_init();

    if (argc > 1 && strlen(argv[1]) >= 4)
    {
        /* Disable test module (use all Heimdall modules) */
        heimdall_static_modules[0] = NULL;

        unsigned max_indirect_steps = 1;
        if (argc > 2)
            max_indirect_steps = atoi(argv[2]);

        int flags = 0;
        if (argc > 3)
            flags = atoi(argv[3]);

        const char *f = argv[1];
        heimdall_fourcc_t from_fourcc = HEIMDALL_FOURCC(f[0], f[1], f[2], f[3]);
        heimdall_fourcc_t to_fourcc = 0;
        if (f[4] == '-' && strlen(f) >= 9)
            to_fourcc = HEIMDALL_FOURCC(f[5], f[6], f[7], f[8]);
        libheimdall_instance_t *heimdall = libheimdall_new(0, NULL);
        assert(heimdall != NULL);

        size_t count;
        struct heimdall_chroma_conv_result *results =
            heimdall_chroma_conv_Probe(from_fourcc, to_fourcc, 0, 0,
                                  max_indirect_steps, flags, &count);
        assert(results != NULL);
        print_results(results, count);
        free(results);

        libheimdall_release(heimdall);
        return 0;
    }

    /* Disable all modules except the one from this test */
    const char *libheimdall_argv[] = {
        "--no-plugins-cache",
        "--no-plugins-scan",
    };
    int libheimdall_argc = ARRAY_SIZE(libheimdall_argv);

    libheimdall_instance_t *heimdall = libheimdall_new(libheimdall_argc, libheimdall_argv);
    assert(heimdall != NULL);

    size_t scenario_count = ARRAY_SIZE(scenario_array);
    for (size_t i = 0; i < scenario_count; i++)
    {
        const struct scenario *scr = &scenario_array[i];

        fprintf(stderr, "scenario: %4.4s -> %4.4s flags: 0x%x, "
                "max_indirect_steps: %u result_count: %zu\n",
                (const char *)&scr->in, (const char *)&scr->out,
                scr->flags,
                scr->max_indirect_steps, scr->result_count);

        size_t count;
        struct heimdall_chroma_conv_result *results =
            heimdall_chroma_conv_Probe(scr->in, scr->out, 0, 0,
                                  scr->max_indirect_steps, scr->flags, &count);
        if (results == NULL)
        {
            assert(scr->result_count == 0);
            continue;
        }
        print_results(results, count);
        assert(count == scr->result_count);
        check_results(scr, results);
        free(results);
    }

    libheimdall_release(heimdall);

    return 0;
}
