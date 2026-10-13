/*****************************************************************************
 * pcr_sync.c: PCR_Sync unit tests
 *****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
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

#undef NDEBUG

#include <assert.h>

#include <heimdall_common.h>

#include <heimdall_arrays.h>
#include <heimdall_frame.h>
#include <heimdall_tick.h>
#include <heimdall_vector.h>

#include "../modules/stream_out/transcode/pcr_sync.h"
#include "../modules/stream_out/transcode/pcr_helper.h"

typedef struct
{
    enum test_element_type
    {
        TEST_ELEM_TYPE_FRAME,
        TEST_ELEM_TYPE_PCR,
    } type;
    union
    {
        struct
        {
            heimdall_tick_t dts;
            unsigned int track_number;
            bool discontinuity;
        };
        heimdall_tick_t pcr;
    };
} test_element;

typedef struct
{
    size_t input_count;
    const test_element *input;
    size_t output_count;
    const test_element *output;

    size_t track_count;
} test_context;

#undef FRAME
#define FRAME(id, val)                                                                             \
    ((test_element){.type = TEST_ELEM_TYPE_FRAME, .dts = val, .track_number = id})
#undef DISCONTINUITY
#define DISCONTINUITY(id, val)                                                                     \
    ((test_element){                                                                               \
        .type = TEST_ELEM_TYPE_FRAME, .dts = val, .track_number = id, .discontinuity = true})
#undef PCR
#define PCR(val) ((test_element){.type = TEST_ELEM_TYPE_PCR, .pcr = val})

static void test_Sync(heimdall_pcr_sync_t *sync, const test_context context)
{
    // Add all tracks to the synchronizer and keep their id.
    struct HEIMDALL_VECTOR(unsigned int) track_ids_vec = HEIMDALL_VECTOR_INITIALIZER;
    bool alloc_success = heimdall_vector_reserve(&track_ids_vec, context.track_count);
    assert(alloc_success);
    for (size_t i = 0; i < context.track_count; ++i)
    {
        unsigned int id;
        alloc_success = heimdall_pcr_sync_NewESID(sync, &id) == HEIMDALL_SUCCESS;
        assert(alloc_success);
        alloc_success = heimdall_vector_push(&track_ids_vec, id);
        assert(alloc_success);
    }

    // Queue PCR events in the synchronizer and signal all the frames.
    for (size_t i = 0; i < context.input_count; ++i)
    {
        const test_element *elem = &context.input[i];
        if (elem->type == TEST_ELEM_TYPE_PCR)
        {
            assert(heimdall_pcr_sync_SignalPCR(sync, elem->pcr) == HEIMDALL_SUCCESS);
        }
        else
        {
            const heimdall_frame_t mock = {.i_dts = elem->dts,
                                      .i_flags =
                                          elem->discontinuity ? HEIMDALL_FRAME_FLAG_DISCONTINUITY : 0};
            heimdall_pcr_sync_SignalFrame(sync, track_ids_vec.data[elem->track_number], &mock);
        }
    }

    // Signal all the frame leaving and check that PCR events are properly aligned.
    heimdall_tick_t last_pcr;
    for (size_t i = 0; i < context.output_count; ++i)
    {
        const test_element *elem = &context.output[i];
        if (elem->type == TEST_ELEM_TYPE_FRAME)
        {
            const heimdall_frame_t mock = {.i_dts = elem->dts};
            last_pcr =
                heimdall_pcr_sync_SignalFrameOutput(sync, track_ids_vec.data[elem->track_number], &mock);
        }
        else
        {
            assert(elem->pcr == last_pcr);
        }
    }

    heimdall_vector_destroy(&track_ids_vec);
}

static void test_Run(void (*test)(heimdall_pcr_sync_t *))
{
    heimdall_pcr_sync_t *sync = heimdall_pcr_sync_New();
    assert(sync != NULL);

    test(sync);

    heimdall_pcr_sync_Delete(sync);
}

typedef enum
{
    VIDEO_TRACK,
    AUDIO_TRACK,
    SUB_TRACK,

    MAX_TRACKS
} track_ids;

static void test_Simple(heimdall_pcr_sync_t *sync)
{
    const test_element input[] = {FRAME(VIDEO_TRACK, 1), PCR(10), FRAME(VIDEO_TRACK, 20)};
    test_Sync(sync, (test_context){
                        .input = input,
                        .input_count = ARRAY_SIZE(input),
                        .output = input,
                        .output_count = ARRAY_SIZE(input),
                        .track_count = 1,
                    });
}

static void test_MultipleTracks(heimdall_pcr_sync_t *sync)
{
    const test_element input[] = {
        FRAME(VIDEO_TRACK, 1),  FRAME(AUDIO_TRACK, 1),  PCR(10),
        FRAME(VIDEO_TRACK, 15), FRAME(AUDIO_TRACK, 15), FRAME(SUB_TRACK, 20)};
    test_Sync(sync, (test_context){.input = input,
                                   .input_count = ARRAY_SIZE(input),
                                   .output = input,
                                   .output_count = ARRAY_SIZE(input),
                                   .track_count = MAX_TRACKS});
}

static void test_MultipleTracksButOnlyOneSends(heimdall_pcr_sync_t *sync)
{
    const test_element input[] = {FRAME(VIDEO_TRACK, 1), PCR(10), FRAME(VIDEO_TRACK, 15)};
    test_Sync(sync, (test_context){.input = input,
                                   .input_count = ARRAY_SIZE(input),
                                   .output = input,
                                   .output_count = ARRAY_SIZE(input),
                                   .track_count = MAX_TRACKS});
}

static void test_InvalidDTS(heimdall_pcr_sync_t *sync)
{
    const test_element input[] = {FRAME(VIDEO_TRACK, 1), FRAME(VIDEO_TRACK, HEIMDALL_TICK_INVALID),
                                  FRAME(VIDEO_TRACK, HEIMDALL_TICK_INVALID), PCR(5),
                                  FRAME(VIDEO_TRACK, 10)};
    const test_element output[] = {FRAME(VIDEO_TRACK, 1), PCR(5), FRAME(VIDEO_TRACK, 10)};
    test_Sync(sync, (test_context){.input = input,
                                   .input_count = ARRAY_SIZE(input),
                                   .output = output,
                                   .output_count = ARRAY_SIZE(output),
                                   .track_count = MAX_TRACKS});
}

static void test_LowDiscontinuity(heimdall_pcr_sync_t *sync)
{
    const test_element input[] = {FRAME(VIDEO_TRACK, 200), DISCONTINUITY(VIDEO_TRACK, 1), PCR(5),
                                  FRAME(VIDEO_TRACK, 10)};
    test_Sync(sync, (test_context){.input = input,
                                   .input_count = ARRAY_SIZE(input),
                                   .output = input,
                                   .output_count = ARRAY_SIZE(input),
                                   .track_count = MAX_TRACKS});
}

static void test_HighDiscontinuity(heimdall_pcr_sync_t *sync)
{
    const test_element input[] = {FRAME(VIDEO_TRACK, 1), DISCONTINUITY(VIDEO_TRACK, 200), PCR(210),
                                  FRAME(VIDEO_TRACK, 220)};
    test_Sync(sync, (test_context){.input = input,
                                   .input_count = ARRAY_SIZE(input),
                                   .output = input,
                                   .output_count = ARRAY_SIZE(input),
                                   .track_count = MAX_TRACKS});
}
static void test_TwoDiscontinuities(heimdall_pcr_sync_t *sync)
{
    const test_element input[] = {FRAME(VIDEO_TRACK, 200), DISCONTINUITY(VIDEO_TRACK, 1),
                                  DISCONTINUITY(VIDEO_TRACK, 210), PCR(210),
                                  FRAME(VIDEO_TRACK, 220)};
    test_Sync(sync, (test_context){.input = input,
                                   .input_count = ARRAY_SIZE(input),
                                   .output = input,
                                   .output_count = ARRAY_SIZE(input),
                                   .track_count = MAX_TRACKS});
}

static void test_FastForwardAtBeginning(heimdall_pcr_sync_t *sync)
{
    assert(heimdall_pcr_sync_SignalPCR(sync, 1u) == HEIMDALL_PCR_SYNC_FORWARD_PCR);
}

static void test_FastForwardMiddleOfStream(heimdall_pcr_sync_t *sync)
{
    // Launch a basic test scenario to mimic an usual stream flow.
    test_MultipleTracks(sync);

    // Ensure PCRs sent in the middle of the stream without any data in between
    // can be immediately forwarded.
    assert(heimdall_pcr_sync_SignalPCR(sync, 30u) == HEIMDALL_PCR_SYNC_FORWARD_PCR);
    assert(heimdall_pcr_sync_SignalPCR(sync, 40u) == HEIMDALL_PCR_SYNC_FORWARD_PCR);
    assert(heimdall_pcr_sync_SignalPCR(sync, 50u) == HEIMDALL_PCR_SYNC_FORWARD_PCR);
}

static void test_OneTrackWithDelay(heimdall_pcr_sync_t *sync)
{
    unsigned id;
    assert(heimdall_pcr_sync_NewESID(sync, &id) == HEIMDALL_SUCCESS);

    const heimdall_frame_t frame1 = {.i_dts = 1u};
    heimdall_pcr_sync_SignalFrame(sync, id, &frame1);
    assert(heimdall_pcr_sync_SignalPCR(sync, 10u) == HEIMDALL_SUCCESS);

    const heimdall_frame_t frame2 = {.i_dts = 11u};
    heimdall_pcr_sync_SignalFrame(sync, id, &frame2);
    assert(heimdall_pcr_sync_SignalFrameOutput(sync, id, &frame1) == 10u);
    assert(heimdall_pcr_sync_SignalPCR(sync, 20u) == HEIMDALL_SUCCESS);
}

static void test_SubsequentPCR(heimdall_pcr_sync_t *sync)
{
    unsigned id;
    assert(heimdall_pcr_sync_NewESID(sync, &id) == HEIMDALL_SUCCESS);

    const heimdall_frame_t frame = {.i_dts = 1u};
    heimdall_pcr_sync_SignalFrame(sync, id, &frame);
    assert(heimdall_pcr_sync_SignalPCR(sync, 10u) == HEIMDALL_SUCCESS);
    assert(heimdall_pcr_sync_SignalPCR(sync, 20u) == HEIMDALL_SUCCESS);
    assert(heimdall_pcr_sync_SignalPCR(sync, 30u) == HEIMDALL_SUCCESS);

    // Output the same frame until no PCR is returned.
    assert(heimdall_pcr_sync_SignalFrameOutput(sync, id, &frame) == 10u);
    assert(heimdall_pcr_sync_SignalFrameOutput(sync, id, &frame) == 20u);
    assert(heimdall_pcr_sync_SignalFrameOutput(sync, id, &frame) == 30u);
    assert(heimdall_pcr_sync_SignalFrameOutput(sync, id, &frame) == HEIMDALL_TICK_INVALID);
}

static void test_LateTrack(heimdall_pcr_sync_t *sync)
{
    unsigned audio, video;
    assert(heimdall_pcr_sync_NewESID(sync, &audio) == HEIMDALL_SUCCESS);
    assert(heimdall_pcr_sync_NewESID(sync, &video) == HEIMDALL_SUCCESS);

    for (heimdall_tick_t dts = 0; dts < 100; ++dts )
    {
        if (dts % 2 == 0)
        {
            const heimdall_frame_t v_frame = {.i_dts = dts + 1};
            heimdall_pcr_sync_SignalFrame(sync, video, &v_frame);
        }
        const heimdall_frame_t a_frame = {.i_dts = dts + 1};
        heimdall_pcr_sync_SignalFrame(sync, audio, &a_frame);
        if (dts % 10 == 0)
            assert(heimdall_pcr_sync_SignalPCR(sync, dts + 1) == HEIMDALL_SUCCESS);
    }

    // Unqueue the audio frames first to mimic fast decoding
    for (heimdall_tick_t dts = 0; dts < 100; ++dts)
    {
        const heimdall_frame_t a_frame = {.i_dts = dts + 1};
        const heimdall_tick_t pcr =
            heimdall_pcr_sync_SignalFrameOutput(sync, audio, &a_frame);
        assert(pcr == HEIMDALL_TICK_INVALID);
    }

    for (heimdall_tick_t dts = 0; dts < 100; ++dts )
    {
        if (dts % 2 != 0)
            continue;

        const heimdall_frame_t v_frame = {.i_dts = dts + 1};
        const heimdall_tick_t pcr =
            heimdall_pcr_sync_SignalFrameOutput(sync, video, &v_frame);
        if (dts % 10 == 0)
            assert(pcr == dts + 1);
        else
            assert(pcr == HEIMDALL_TICK_INVALID);
    }

    heimdall_pcr_sync_DelESID(sync, audio);
    heimdall_pcr_sync_DelESID(sync, video);
}

static void test_PCRHelper(void (*test)(heimdall_pcr_sync_t *,
                                        transcode_track_pcr_helper_t *))
{
    heimdall_pcr_sync_t *sync = heimdall_pcr_sync_New();
    assert(sync != NULL);

    transcode_track_pcr_helper_t *track_helper =
        transcode_track_pcr_helper_New(sync, HEIMDALL_TICK_FROM_MS(200));
    assert(track_helper != NULL);

    test(sync, track_helper);

    transcode_track_pcr_helper_Delete(track_helper);
    heimdall_pcr_sync_Delete(sync);
}

static void test_PCRHelperSimple(heimdall_pcr_sync_t *sync,
                                 transcode_track_pcr_helper_t *pcr_helper)
{
    const heimdall_frame_t in_frame = {.i_dts = 1u, .i_length = 10u};
    heimdall_tick_t dropped_frame_pcr;
    transcode_track_pcr_helper_SignalEnteringFrame(
        pcr_helper, &in_frame, &dropped_frame_pcr);
    assert(dropped_frame_pcr == HEIMDALL_TICK_INVALID);

    assert(heimdall_pcr_sync_SignalPCR(sync, 10u) == HEIMDALL_SUCCESS);

    const heimdall_frame_t out_frame = {.i_dts = 2000u, .i_length = 10u};
    heimdall_tick_t pcr;
    const int status = transcode_track_pcr_helper_SignalLeavingFrame(
        pcr_helper, &out_frame, &pcr);
    assert(status == HEIMDALL_SUCCESS);
    assert(pcr == 10u);
}

static void
test_PCRHelperMultipleTracks(heimdall_pcr_sync_t *sync,
                             transcode_track_pcr_helper_t *video)
{
    transcode_track_pcr_helper_t *audio =
        transcode_track_pcr_helper_New(sync, HEIMDALL_TICK_FROM_MS(200));
    heimdall_tick_t dropped_frame_pcr;

    for (heimdall_tick_t dts = 0; dts < 200; dts += 10)
    {
        if (dts % 20 == 0)
        {
            const heimdall_frame_t v_frame = {.i_dts = dts + 1, .i_length = 20u};
            transcode_track_pcr_helper_SignalEnteringFrame(
                video, &v_frame, &dropped_frame_pcr);
            assert(dropped_frame_pcr == HEIMDALL_TICK_INVALID);
        }
        const heimdall_frame_t a_frame = {.i_dts = dts + 1, .i_length = 10u};
        transcode_track_pcr_helper_SignalEnteringFrame(
            audio, &a_frame, &dropped_frame_pcr);
        assert(dropped_frame_pcr == HEIMDALL_TICK_INVALID);
        if (dts == 100)
            assert(heimdall_pcr_sync_SignalPCR(sync, 101) == HEIMDALL_SUCCESS);
    }

    heimdall_tick_t pcr;
    // Unqueue the audio frames first to mimic fast decoding
    for (heimdall_tick_t dts = 0; dts < 200; dts += 10)
    {
        const heimdall_frame_t a_frame = {.i_dts = dts + 1, .i_length = 10u};
        const int status = transcode_track_pcr_helper_SignalLeavingFrame(
            audio, &a_frame, &pcr);
        assert(status == HEIMDALL_SUCCESS);
        assert(pcr == HEIMDALL_TICK_INVALID);
    }

    for (heimdall_tick_t dts = 0; dts < 100; dts += 10)
    {
        const heimdall_frame_t v_frame = {.i_dts = (dts * 2) - 10 + 1,
                                     .i_length = 20u};
        const int status = transcode_track_pcr_helper_SignalLeavingFrame(
            video, &v_frame, &pcr);
        assert(status == HEIMDALL_SUCCESS);
        if (dts != 50)
            assert(pcr == HEIMDALL_TICK_INVALID);
        else
            assert(pcr == 91);
    }

    transcode_track_pcr_helper_Delete(audio);
}

static void
test_PCRHelperSplitFrameOutput(heimdall_pcr_sync_t *sync,
                               transcode_track_pcr_helper_t *pcr_helper)
{
    const heimdall_frame_t in_frame = {.i_dts = 1u, .i_length = 10u};
    heimdall_tick_t dropped_frame_pcr;
    transcode_track_pcr_helper_SignalEnteringFrame(
        pcr_helper, &in_frame, &dropped_frame_pcr);
    assert(dropped_frame_pcr == HEIMDALL_TICK_INVALID);

    assert(heimdall_pcr_sync_SignalPCR(sync, 10u) == HEIMDALL_SUCCESS);

    const heimdall_frame_t out_frame1 = {.i_dts = 2000u, .i_length = 5u};
    const heimdall_frame_t out_frame2 = {.i_dts = 2005u, .i_length = 5u};
    heimdall_tick_t pcr;
    int status = transcode_track_pcr_helper_SignalLeavingFrame(
        pcr_helper, &out_frame1, &pcr);
    assert(status == HEIMDALL_SUCCESS);
    assert(pcr == HEIMDALL_TICK_INVALID);

    status = transcode_track_pcr_helper_SignalLeavingFrame(
        pcr_helper, &out_frame2, &pcr);
    assert(status == HEIMDALL_SUCCESS);
    assert(pcr == 10u);
}

static void
test_PCRHelperSplitFrameInput(heimdall_pcr_sync_t *sync,
                              transcode_track_pcr_helper_t *pcr_helper)
{
    const heimdall_frame_t in_frame1 = {.i_dts = 2000u, .i_length = 10u};
    const heimdall_frame_t in_frame2 = {.i_dts = 2010u, .i_length = 10u};
    heimdall_tick_t dropped_frame_pcr;
    transcode_track_pcr_helper_SignalEnteringFrame(
        pcr_helper, &in_frame1, &dropped_frame_pcr);
    assert(dropped_frame_pcr == HEIMDALL_TICK_INVALID);
    transcode_track_pcr_helper_SignalEnteringFrame(
        pcr_helper, &in_frame2, &dropped_frame_pcr);
    assert(dropped_frame_pcr == HEIMDALL_TICK_INVALID);

    assert(heimdall_pcr_sync_SignalPCR(sync, 2010u) == HEIMDALL_SUCCESS);

    const heimdall_frame_t in_frame3 = {.i_dts = 2020u, .i_length = 10u};
    transcode_track_pcr_helper_SignalEnteringFrame(
        pcr_helper, &in_frame3, &dropped_frame_pcr);
    assert(dropped_frame_pcr == HEIMDALL_TICK_INVALID);

    const heimdall_frame_t out_frame1 = {.i_dts = 1u, .i_length = 20u};
    heimdall_tick_t pcr;
    int status = transcode_track_pcr_helper_SignalLeavingFrame(
        pcr_helper, &out_frame1, &pcr);
    assert(status == HEIMDALL_SUCCESS);
    // PCR should be re-synthetized.
    assert(pcr == 1u);
    const heimdall_frame_t out_frame2 = {.i_dts = 21u, .i_length = 10u};
    status = transcode_track_pcr_helper_SignalLeavingFrame(
        pcr_helper, &out_frame2, &pcr);
    assert(status == HEIMDALL_SUCCESS);
    assert(pcr == HEIMDALL_TICK_INVALID);
}

static void test_PCRHelperIOMismatch(heimdall_pcr_sync_t *sync,
                                     transcode_track_pcr_helper_t *pcr_helper)
{
    const heimdall_frame_t in_frame = {.i_dts = 1u, .i_length = 10u};
    heimdall_tick_t dropped_frame_pcr;
    transcode_track_pcr_helper_SignalEnteringFrame(
        pcr_helper, &in_frame, &dropped_frame_pcr);
    assert(dropped_frame_pcr == HEIMDALL_TICK_INVALID);

    assert(heimdall_pcr_sync_SignalPCR(sync, 10u) == HEIMDALL_SUCCESS);

    const heimdall_frame_t out_frame1 = {.i_dts = 2000u, .i_length = 10u};
    const heimdall_frame_t out_frame2 = {.i_dts = 2010u, .i_length = 10u};
    heimdall_tick_t pcr;
    int status = transcode_track_pcr_helper_SignalLeavingFrame(
        pcr_helper, &out_frame1, &pcr);
    assert(status == HEIMDALL_SUCCESS);
    assert(pcr == 10u);

    status = transcode_track_pcr_helper_SignalLeavingFrame(
        pcr_helper, &out_frame2, &pcr);
    assert(status == HEIMDALL_EGENERIC);
    assert(pcr == HEIMDALL_TICK_INVALID);
}

int main(void)
{
    test_Run(test_Simple);
    test_Run(test_MultipleTracks);
    test_Run(test_MultipleTracksButOnlyOneSends);
    test_Run(test_InvalidDTS);
    test_Run(test_LowDiscontinuity);
    test_Run(test_HighDiscontinuity);
    test_Run(test_TwoDiscontinuities);
    test_Run(test_FastForwardAtBeginning);
    test_Run(test_FastForwardMiddleOfStream);
    test_Run(test_OneTrackWithDelay);
    test_Run(test_SubsequentPCR);
    test_Run(test_LateTrack);
    test_PCRHelper(test_PCRHelperSimple);
    test_PCRHelper(test_PCRHelperMultipleTracks);
    test_PCRHelper(test_PCRHelperSplitFrameOutput);
    test_PCRHelper(test_PCRHelperSplitFrameInput);
    test_PCRHelper(test_PCRHelperIOMismatch);
}
