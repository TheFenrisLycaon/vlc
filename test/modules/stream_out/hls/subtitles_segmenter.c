/*****************************************************************************
 * subtitles_segmenter.c: HLS subtitle segmentation unit tests
 *****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <heimdall_common.h>

#include <heimdall_block.h>
#include <heimdall_plugin.h>
#include <heimdall_sout.h>

#include "../../../libheimdall/test.h"
#include "../../../../modules/stream_out/hls/hls.h"
#include "../lib/libheimdall_internal.h"

struct test_scenario
{
    heimdall_tick_t segment_length;

    unsigned bytes_reported;
    const char *expected;

    void (*report_sub)(block_t *);
    void (*send_sub)(sout_mux_t *, sout_input_t *);
};

static struct test_scenario *CURRENT_SCENARIO = NULL;

static ssize_t AccessOutWrite(sout_access_out_t *access, block_t *block)
{
    struct test_scenario *cb = access->p_sys;
    assert(block->p_next == NULL);

    cb->report_sub(block);

    const ssize_t r = block->i_buffer;
    block_ChainRelease(block);
    return r;
}

static sout_access_out_t *CreateAccessOut(heimdall_object_t *parent,
                                          const struct test_scenario *cb)
{
    sout_access_out_t *access = heimdall_object_create(parent, sizeof(*access));
    if (unlikely(access == NULL))
        return NULL;

    access->psz_access = strdup("mock");
    if (unlikely(access->psz_access == NULL))
    {
        heimdall_object_delete(access);
        return NULL;
    }

    access->p_cfg = NULL;
    access->p_module = NULL;
    access->p_sys = (void *)cb;
    access->psz_path = NULL;

    access->pf_control = NULL;
    access->pf_read = NULL;
    access->pf_seek = NULL;
    access->pf_write = AccessOutWrite;
    return access;
}

static heimdall_frame_t *
make_spu(const char *text, heimdall_tick_t start, heimdall_tick_t stop)
{
    char *owned_txt = strdup(text);
    assert(owned_txt != NULL);

    heimdall_frame_t *spu = heimdall_frame_heap_Alloc(owned_txt, strlen(owned_txt));
    spu->i_pts = spu->i_dts = start + HEIMDALL_TICK_0;
    if (stop != HEIMDALL_TICK_INVALID)
        spu->i_length = stop - start;
    else
        spu->i_length = HEIMDALL_TICK_INVALID;
    return spu;
}
static heimdall_frame_t *make_ephemer_spu(const char *text, heimdall_tick_t start)
{
    return make_spu(text, start, HEIMDALL_TICK_INVALID);
}

#define NO_SPLIT_EXPECTED                                                      \
    "WEBVTT\n\n"                                                               \
    "00:00:00.000 --> 00:00:03.000\n"                                          \
    "Hello world\n\n"
static void send_no_split(sout_mux_t *mux, sout_input_t *input)
{
    heimdall_frame_t *spu = make_spu("Hello world", 0, HEIMDALL_TICK_FROM_SEC(3));
    const int status = sout_MuxSendBuffer(mux, input, spu);
    assert(status == HEIMDALL_SUCCESS);
}

#define ONE_SPLIT_EXPECTED                                                     \
    "WEBVTT\n\n"                                                               \
    "00:00:00.000 --> 00:00:04.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "00:00:04.000 --> 00:00:06.000\n"                                          \
    "Hello world\n\n"
static void send_one_split(sout_mux_t *mux, sout_input_t *input)
{
    heimdall_frame_t *spu = make_spu("Hello world", 0, HEIMDALL_TICK_FROM_SEC(6));
    const int status = sout_MuxSendBuffer(mux, input, spu);
    assert(status == HEIMDALL_SUCCESS);
}

#define FIVE_SPLITS_EXPECTED                                                   \
    "WEBVTT\n\n"                                                               \
    "00:00:01.500 --> 00:00:02.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "00:00:02.000 --> 00:00:04.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "00:00:04.000 --> 00:00:06.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "00:00:06.000 --> 00:00:08.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "00:00:08.000 --> 00:00:09.500\n"                                          \
    "Hello world\n\n"
static void send_five_splits(sout_mux_t *mux, sout_input_t *input)
{
    heimdall_frame_t *spu =
        make_spu("Hello world", HEIMDALL_TICK_FROM_MS(1500), HEIMDALL_TICK_FROM_MS(9500));
    const int status = sout_MuxSendBuffer(mux, input, spu);
    assert(status == HEIMDALL_SUCCESS);
}

#define NOTHING_FOR_FIVE_SEC_EXPECTED                                          \
    "WEBVTT\n\n"                                                               \
    "WEBVTT\n\n"                                                               \
    "WEBVTT\n\n"
static void send_nothing_for_five_sec(sout_mux_t *mux, sout_input_t *input)
{
    for (heimdall_tick_t stream_time = HEIMDALL_TICK_0;
         stream_time < HEIMDALL_TICK_FROM_MS(5300);
         stream_time += HEIMDALL_TICK_FROM_MS(150))
    {
        hls_sub_segmenter_SignalStreamUpdate(mux, stream_time);
    }
    (void)input;
}

#define BEGIN_HOLE_EXPECTED                                                    \
    "WEBVTT\n\n"                                                               \
    "WEBVTT\n\n"                                                               \
    "WEBVTT\n\n"                                                               \
    "WEBVTT\n\n"                                                               \
    "00:00:07.500 --> 00:00:08.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "00:00:08.300 --> 00:00:10.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "00:00:10.000 --> 00:00:11.500\n"                                          \
    "Hello world\n\n"
static void send_begin_hole(sout_mux_t *mux, sout_input_t *input)
{
    for (heimdall_tick_t stream_time = HEIMDALL_TICK_0;
         stream_time < HEIMDALL_TICK_FROM_MS(6150);
         stream_time += HEIMDALL_TICK_FROM_MS(150))
    {
        hls_sub_segmenter_SignalStreamUpdate(mux, stream_time);
    }

    heimdall_frame_t *spu =
        make_spu("Hello world", HEIMDALL_TICK_FROM_MS(7500), HEIMDALL_TICK_FROM_MS(8000));
    int status = sout_MuxSendBuffer(mux, input, spu);
    assert(status == HEIMDALL_SUCCESS);

    spu = make_spu(
        "Hello world", HEIMDALL_TICK_FROM_MS(8300), HEIMDALL_TICK_FROM_MS(11500));
    status = sout_MuxSendBuffer(mux, input, spu);
    assert(status == HEIMDALL_SUCCESS);
}

#define MIDDLE_HOLE_EXPECTED                                                   \
    "WEBVTT\n\n"                                                               \
    "00:00:01.500 --> 00:00:02.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "00:00:02.000 --> 00:00:02.300\n"                                          \
    "Hello world\n\n"                                                          \
    "00:00:02.300 --> 00:00:03.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "WEBVTT\n\n"                                                               \
    "WEBVTT\n\n"                                                               \
    "00:00:11.000 --> 00:00:11.500\n"                                          \
    "Hello world\n\n"
static void send_middle_hole(sout_mux_t *mux, sout_input_t *input)
{
    heimdall_frame_t *spu =
        make_spu("Hello world", HEIMDALL_TICK_FROM_MS(1500), HEIMDALL_TICK_FROM_MS(2300));
    int status = sout_MuxSendBuffer(mux, input, spu);
    assert(status == HEIMDALL_SUCCESS);

    spu =
        make_spu("Hello world", HEIMDALL_TICK_FROM_MS(2300), HEIMDALL_TICK_FROM_MS(3000));
    status = sout_MuxSendBuffer(mux, input, spu);
    assert(status == HEIMDALL_SUCCESS);

    for (heimdall_tick_t stream_time = HEIMDALL_TICK_FROM_MS(3000) + HEIMDALL_TICK_0;
         stream_time < HEIMDALL_TICK_FROM_MS(10000);
         stream_time += HEIMDALL_TICK_FROM_MS(150))
    {
        hls_sub_segmenter_SignalStreamUpdate(mux, stream_time);
    }

    spu = make_spu(
        "Hello world", HEIMDALL_TICK_FROM_MS(11000), HEIMDALL_TICK_FROM_MS(11500));
    status = sout_MuxSendBuffer(mux, input, spu);
    assert(status == HEIMDALL_SUCCESS);
}

#define END_HOLE_EXPECTED                                                      \
    "WEBVTT\n\n"                                                               \
    "00:00:01.500 --> 00:00:02.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "00:00:02.000 --> 00:00:02.300\n"                                          \
    "Hello world\n\n"                                                          \
    "00:00:02.300 --> 00:00:03.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "WEBVTT\n\n"                                                               \
    "WEBVTT\n\n"
static void send_end_hole(sout_mux_t *mux, sout_input_t *input)
{
    heimdall_frame_t *spu =
        make_spu("Hello world", HEIMDALL_TICK_FROM_MS(1500), HEIMDALL_TICK_FROM_MS(2300));
    int status = sout_MuxSendBuffer(mux, input, spu);
    assert(status == HEIMDALL_SUCCESS);

    spu =
        make_spu("Hello world", HEIMDALL_TICK_FROM_MS(2300), HEIMDALL_TICK_FROM_MS(3000));
    status = sout_MuxSendBuffer(mux, input, spu);
    assert(status == HEIMDALL_SUCCESS);

    for (heimdall_tick_t stream_time = HEIMDALL_TICK_FROM_MS(3000) + HEIMDALL_TICK_0;
         stream_time < HEIMDALL_TICK_FROM_MS(10000);
         stream_time += HEIMDALL_TICK_FROM_MS(150))
    {
        hls_sub_segmenter_SignalStreamUpdate(mux, stream_time);
    }
}

#define EPHEMER_NO_SPLIT_EXPECTED                                              \
    "WEBVTT\n\n"                                                               \
    "00:00:01.500 --> 00:00:02.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "00:00:02.000 --> 00:00:02.300\n"                                          \
    "Hello world\n\n"                                                          \
    "00:00:02.300 --> 00:00:03.100\n"                                          \
    "Hello world\n\n"                                                          \
    "00:00:03.100 --> 00:00:04.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "00:00:04.000 --> 00:00:05.500\n"                                          \
    "Hello world\n\n"
static void send_ephemer_no_split(sout_mux_t *mux, sout_input_t *input)
{
    heimdall_frame_t *spu = make_ephemer_spu("Hello world", HEIMDALL_TICK_FROM_MS(1500));
    int status = sout_MuxSendBuffer(mux, input, spu);
    assert(status == HEIMDALL_SUCCESS);

    spu = make_ephemer_spu("Hello world", HEIMDALL_TICK_FROM_MS(2300));
    status = sout_MuxSendBuffer(mux, input, spu);
    assert(status == HEIMDALL_SUCCESS);
    hls_sub_segmenter_SignalStreamUpdate(mux,
                                         HEIMDALL_TICK_FROM_MS(2300) + HEIMDALL_TICK_0);

    spu = make_ephemer_spu("Hello world", HEIMDALL_TICK_FROM_MS(3100));
    status = sout_MuxSendBuffer(mux, input, spu);
    assert(status == HEIMDALL_SUCCESS);
    hls_sub_segmenter_SignalStreamUpdate(mux,
                                         HEIMDALL_TICK_FROM_MS(3100) + HEIMDALL_TICK_0);

    spu = make_ephemer_spu("Hello world", HEIMDALL_TICK_FROM_MS(4000));
    status = sout_MuxSendBuffer(mux, input, spu);
    assert(status == HEIMDALL_SUCCESS);
    hls_sub_segmenter_SignalStreamUpdate(mux,
                                         HEIMDALL_TICK_FROM_MS(4000) + HEIMDALL_TICK_0);

    hls_sub_segmenter_SignalStreamUpdate(mux,
                                         HEIMDALL_TICK_FROM_MS(5500) + HEIMDALL_TICK_0);
}

#define LONG_EPHEMER_EXPECTED                                                  \
    "WEBVTT\n\n"                                                               \
    "00:00:01.500 --> 00:00:02.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "00:00:02.000 --> 00:00:04.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "00:00:04.000 --> 00:00:06.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "00:00:06.000 --> 00:00:08.000\n"                                          \
    "Hello world\n\n"                                                          \
    "WEBVTT\n\n"                                                               \
    "00:00:08.000 --> 00:00:09.500\n"                                          \
    "Hello world\n\n"
static void send_long_ephemer(sout_mux_t *mux, sout_input_t *input)
{
    heimdall_frame_t *spu = make_ephemer_spu("Hello world", HEIMDALL_TICK_FROM_MS(1500));
    int status = sout_MuxSendBuffer(mux, input, spu);
    assert(status == HEIMDALL_SUCCESS);

    for (heimdall_tick_t stream_time = HEIMDALL_TICK_FROM_MS(1500);
         stream_time <= HEIMDALL_TICK_FROM_MS(9500);
         stream_time += HEIMDALL_TICK_FROM_MS(100))
    {
        hls_sub_segmenter_SignalStreamUpdate(mux, stream_time + HEIMDALL_TICK_0);
    }
}

static void default_report(block_t *sub)
{
    const char *expected =
        CURRENT_SCENARIO->expected + CURRENT_SCENARIO->bytes_reported;

    const int cmp =
        strncmp(expected, (const char *)sub->p_buffer, sub->i_buffer);
    assert(cmp == 0);
    CURRENT_SCENARIO->bytes_reported += sub->i_buffer;
}

static struct test_scenario TEST_SCENARIOS[] = {
    {
        .segment_length = HEIMDALL_TICK_FROM_SEC(4),
        .send_sub = send_no_split,
        .report_sub = default_report,
        .expected = NO_SPLIT_EXPECTED,
    },
    {
        .segment_length = HEIMDALL_TICK_FROM_SEC(4),
        .send_sub = send_one_split,
        .report_sub = default_report,
        .expected = ONE_SPLIT_EXPECTED,
    },
    {
        .segment_length = HEIMDALL_TICK_FROM_SEC(2),
        .send_sub = send_five_splits,
        .report_sub = default_report,
        .expected = FIVE_SPLITS_EXPECTED,
    },
    {
        .segment_length = HEIMDALL_TICK_FROM_SEC(2),
        .send_sub = send_nothing_for_five_sec,
        .report_sub = default_report,
        .expected = NOTHING_FOR_FIVE_SEC_EXPECTED,
    },
    {
        .segment_length = HEIMDALL_TICK_FROM_SEC(2),
        .send_sub = send_begin_hole,
        .report_sub = default_report,
        .expected = BEGIN_HOLE_EXPECTED,
    },
    {
        .segment_length = HEIMDALL_TICK_FROM_SEC(2),
        .send_sub = send_middle_hole,
        .report_sub = default_report,
        .expected = MIDDLE_HOLE_EXPECTED,
    },
    {
        .segment_length = HEIMDALL_TICK_FROM_SEC(2),
        .send_sub = send_end_hole,
        .report_sub = default_report,
        .expected = END_HOLE_EXPECTED,
    },
    {
        .segment_length = HEIMDALL_TICK_FROM_SEC(2),
        .send_sub = send_ephemer_no_split,
        .report_sub = default_report,
        .expected = EPHEMER_NO_SPLIT_EXPECTED,
    },
    {
        .segment_length = HEIMDALL_TICK_FROM_SEC(2),
        .send_sub = send_long_ephemer,
        .report_sub = default_report,
        .expected = LONG_EPHEMER_EXPECTED,
    },
};

static void RunTests(libheimdall_instance_t *instance)
{
    for (size_t i = 0; i < ARRAY_SIZE(TEST_SCENARIOS); ++i)
    {
        CURRENT_SCENARIO = &TEST_SCENARIOS[i];
        sout_access_out_t *access = CreateAccessOut(
            HEIMDALL_OBJECT(instance->p_libheimdall_int), CURRENT_SCENARIO);
        assert(access != NULL);

        const struct hls_config config = {
            .segment_length = CURRENT_SCENARIO->segment_length,
        };
        sout_mux_t *mux = CreateSubtitleSegmenter(access, &config);
        assert(mux != NULL);

        es_format_t fmt;
        es_format_Init(&fmt, SPU_ES, HEIMDALL_CODEC_TEXT);
        sout_input_t *input = sout_MuxAddStream(mux, &fmt);
        assert(input != NULL);

        // Disable mux caching.
        mux->b_waiting_stream = false;

        CURRENT_SCENARIO->send_sub(mux, input);

        sout_MuxDeleteStream(mux, input);

        assert(CURRENT_SCENARIO->bytes_reported ==
               strlen(CURRENT_SCENARIO->expected));

        sout_MuxDelete(mux);
        sout_AccessOutDelete(access);
    }
}

int main(void)
{
    test_init();

    const char *const args[] = {
        "-vvv",
    };
    libheimdall_instance_t *heimdall = libheimdall_new(ARRAY_SIZE(args), args);
    if (heimdall == NULL)
        return 1;

    RunTests(heimdall);

    libheimdall_release(heimdall);
}
