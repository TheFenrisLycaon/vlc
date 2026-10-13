/*****************************************************************************
 * webvtt.c: WebVTT Muxer unit testing
 *****************************************************************************
 * Copyright (C) 2024 VideoLabs, VideoLAN and VLC Authors
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

#include "../../libheimdall/test.h"
#include "../lib/libheimdall_internal.h"

struct test_scenario
{
    heimdall_fourcc_t codec;

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

static void send_basic(sout_mux_t *mux, sout_input_t *input)
{
    for (heimdall_tick_t time = HEIMDALL_TICK_0; time < HEIMDALL_TICK_FROM_MS(1300);
         time += HEIMDALL_TICK_FROM_MS(300))
    {
        char *txt = strdup("Hello world");
        assert(txt != NULL);

        heimdall_frame_t *spu = heimdall_frame_heap_Alloc(txt, strlen(txt));
        assert(spu != NULL);
        spu->i_pts = time + HEIMDALL_TICK_FROM_SEC(15 * 60);
        spu->i_length = HEIMDALL_TICK_INVALID;
        const int status = sout_MuxSendBuffer(mux, input, spu);
        assert(status == HEIMDALL_SUCCESS);
    }
}

static void send_ephemer(sout_mux_t *mux, sout_input_t *input)
{
    for (heimdall_tick_t time = HEIMDALL_TICK_0; time < HEIMDALL_TICK_FROM_MS(300);
         time += HEIMDALL_TICK_FROM_MS(100))
    {
        char *txt = strdup("Hello world");
        assert(txt != NULL);

        heimdall_frame_t *spu = heimdall_frame_heap_Alloc(txt, strlen(txt));
        assert(spu != NULL);
        spu->i_pts = time;
        spu->i_length = HEIMDALL_TICK_INVALID;
        const int status = sout_MuxSendBuffer(mux, input, spu);
        assert(status == HEIMDALL_SUCCESS);
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

#define BASIC_TEXT                                                             \
    "WEBVTT\n\n"                                                               \
    "00:15:00.000 --> 00:15:00.300\n"                                          \
    "Hello world\n\n"                                                          \
    "00:15:00.300 --> 00:15:00.600\n"                                          \
    "Hello world\n\n"                                                          \
    "00:15:00.600 --> 00:15:00.900\n"                                          \
    "Hello world\n\n"                                                          \
    "00:15:00.900 --> 00:15:01.200\n"                                          \
    "Hello world\n\n"

#define EPHEMER_TEXT                                                           \
    "WEBVTT\n\n"                                                               \
    "00:00:00.000 --> 00:00:00.100\n"                                          \
    "Hello world\n\n"                                                          \
    "00:00:00.100 --> 00:00:00.200\n"                                          \
    "Hello world\n\n"

static struct test_scenario TEST_SCENARIOS[] = {
    {
        .codec = HEIMDALL_CODEC_TEXT,
        .send_sub = send_basic,
        .report_sub = default_report,
        .expected = BASIC_TEXT,
    },
    {
        .codec = HEIMDALL_CODEC_TEXT,
        .send_sub = send_ephemer,
        .report_sub = default_report,
        .expected = EPHEMER_TEXT,
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

        sout_mux_t *mux = sout_MuxNew(access, "webvtt");
        assert(mux != NULL);

        es_format_t fmt;
        es_format_Init(&fmt, SPU_ES, CURRENT_SCENARIO->codec);
        sout_input_t *input = sout_MuxAddStream(mux, &fmt);
        assert(input != NULL);

        // Disable mux caching.
        mux->b_waiting_stream = false;

        CURRENT_SCENARIO->send_sub(mux, input);

        assert(CURRENT_SCENARIO->bytes_reported ==
               strlen(CURRENT_SCENARIO->expected));

        sout_MuxDeleteStream(mux, input);
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
