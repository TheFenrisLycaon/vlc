/*****************************************************************************
 * clock/clock_start.c: test for the heimdall clock
 *****************************************************************************
 * Copyright (C) 2024 Videolabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Alexandre Janniaux <ajanni@videolabs.io>
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
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_tick.h>
#include <heimdall_es.h>
#include <heimdall_tracer.h>

#include "../../../src/clock/clock.h"

#include <heimdall/heimdall.h>
#include "../../libheimdall/test.h"
#include "../../../lib/libheimdall_internal.h"

#define MODULE_NAME test_clock_clock
#undef HEIMDALL_DYNAMIC_PLUGIN

#include <heimdall_plugin.h>
#include <heimdall_vector.h>

/* Define a builtin module for mocked parts */
const char heimdall_module_name[] = MODULE_STRING;

/**
 * Fixture structure containing the clocks used for each test.
 */
struct clock_fixture_simple {
    heimdall_clock_main_t *main;
    heimdall_clock_t *input;
    heimdall_clock_t *master;
    heimdall_clock_t *slave;
};

struct clock_point
{
    heimdall_tick_t system;
    heimdall_tick_t stream;
};

/**
 * Initialize the fixture.
 *
 * It must be released using \ref destroy_fixture_simple.
 */
static struct clock_fixture_simple
init_fixture_simple(struct heimdall_logger *logger,
                    bool is_input_master,
                    heimdall_tick_t input_dejitter,
                    heimdall_tick_t output_dejitter)
{
    struct clock_fixture_simple f = {
        .main = heimdall_clock_main_New(logger, NULL),
    };
    assert(f.main != NULL);

    heimdall_clock_main_Lock(f.main);
    heimdall_clock_main_SetInputDejitter(f.main, input_dejitter);
    heimdall_clock_main_SetDejitter(f.main, output_dejitter);

    if (is_input_master)
    {
        f.input = heimdall_clock_main_CreateInputMaster(f.main);
        f.master = f.input;
    }
    else
    {
        f.input = heimdall_clock_main_CreateInputSlave(f.main);
        f.master = heimdall_clock_main_CreateMaster(f.main, "Driving", NULL, NULL);
    }
    assert(f.input != NULL);
    assert(f.master != NULL);

    f.slave = heimdall_clock_main_CreateSlave(f.main, "Output", AUDIO_ES,
                                         NULL, NULL);
    assert(f.slave != NULL);
    heimdall_clock_main_Unlock(f.main);

    return f;
}

static void destroy_fixture_simple(struct clock_fixture_simple *f)
{
    heimdall_clock_Delete(f->slave);
    if (f->master != f->input)
        heimdall_clock_Delete(f->master);
    heimdall_clock_Delete(f->input);
    heimdall_clock_main_Delete(f->main);
}

/**
 * Check that conversion doesn't work as long as clock has not
 * been started.
 **/
#if 0
static void test_clock_without_start(struct heimdall_logger *logger)
{
    struct clock_fixture_simple f = init_fixture_simple(logger, true, 0, 0);

    heimdall_clock_main_Lock(f.main);
    const heimdall_tick_t now = HEIMDALL_TICK_FROM_MS(1000);
    {
        heimdall_clock_Update(f.master, now, now * 1000, 1.);
        heimdall_tick_t result = heimdall_clock_ConvertToSystem(f.slave, now, now * 1000, 1., NULL);
        //assert(result == HEIMDALL_TICK_INVALID);
    }

    heimdall_clock_Start(f.slave, now, now * 1000);

    {
        heimdall_tick_t result = heimdall_clock_ConvertToSystem(f.slave, now, now * 1000, 1., NULL);
        assert(result == HEIMDALL_TICK_INVALID);
    }
    heimdall_clock_main_Unlock(f.main);

    destroy_fixture_simple(&f);
}
#endif

/**
 * Check that conversion works and is valid after the clock start.
 **/
static void test_clock_with_start(struct heimdall_logger *logger)
{
    fprintf(stderr, "%s:\n", __func__);
    struct clock_fixture_simple f = init_fixture_simple(logger, true, 0, 0);
    const heimdall_tick_t now = HEIMDALL_TICK_FROM_MS(1000);

    heimdall_clock_main_Lock(f.main);
    heimdall_clock_Update(f.master, now, now * 1000, 1.);
    heimdall_clock_Start(f.input, now, now * 1000);
    heimdall_clock_Start(f.slave, now, now * 1000);

    heimdall_tick_t result = heimdall_clock_ConvertToSystem(f.slave, now, now * 1000, 1., NULL);
    assert(result != HEIMDALL_TICK_INVALID);
    assert(result == now);
    heimdall_clock_main_Unlock(f.main);

    destroy_fixture_simple(&f);
}

/**
 * Check that master clock can start.
 **/
static void test_clock_with_start_master(struct heimdall_logger *logger)
{
    fprintf(stderr, "%s:\n", __func__);
    struct clock_fixture_simple f = init_fixture_simple(logger, false, 0, 0);
    const heimdall_tick_t now = HEIMDALL_TICK_FROM_MS(1000);

    heimdall_clock_main_Lock(f.main);
    heimdall_clock_Start(f.input, now, now * 1000);
    heimdall_clock_Start(f.master, now, now * 1000);
    heimdall_clock_Start(f.slave, now, now * 1000);
    heimdall_clock_Update(f.master, now, now * 1000, 1.);

    heimdall_tick_t result = heimdall_clock_ConvertToSystem(f.slave, now, now * 1000, 1., NULL);
    assert(result != HEIMDALL_TICK_INVALID);
    assert(result == now);
    heimdall_clock_main_Unlock(f.main);

    destroy_fixture_simple(&f);
}

/**
 * Check that conversion works and is valid after the clock start,
 * when start is done later.
 **/
static void test_clock_with_output_dejitter(struct heimdall_logger *logger)
{
    fprintf(stderr, "%s:\n", __func__);
    struct clock_fixture_simple f = init_fixture_simple(logger, true, 0, 0);
    heimdall_clock_main_Lock(f.main);

    const heimdall_tick_t now = HEIMDALL_TICK_FROM_MS(1000);
    heimdall_clock_Update(f.master, now, now * 1000, 1.);
    heimdall_clock_Start(f.input, 2 * now, now * 1000);
    heimdall_clock_Start(f.slave, 2 * now, now * 1000);

    heimdall_tick_t result = heimdall_clock_ConvertToSystem(f.slave, now, now * 1000, 1., NULL);
    fprintf(stderr, "%s:%d: now=%" PRId64 " converted=%" PRId64 "\n",
            __func__, __LINE__, now, result);
    assert(result != HEIMDALL_TICK_INVALID);
    assert(result == 2 * now);
    heimdall_clock_main_Unlock(f.main);

    destroy_fixture_simple(&f);
}

/**
 * Check that clock start is working correctly with an output driving
 * the clock bus.
 **/
static void test_clock_with_output_clock_start(struct heimdall_logger *logger)
{
    fprintf(stderr, "%s:\n", __func__);
    struct clock_fixture_simple f = init_fixture_simple(logger, false, 0, 0);
    heimdall_clock_main_Lock(f.main);

    /* When the input is not driving the clock bus, we should be able to start
     * without any reference point since the output will provide it later. */
    const heimdall_tick_t now = HEIMDALL_TICK_FROM_MS(1000);
    heimdall_clock_Start(f.input, now, now * 1000);
    heimdall_clock_Start(f.slave, now, now * 1000);

    /* We provide the clock point to the clock bus. The system time should
     * approximately match the start date. */
    heimdall_clock_Update(f.master, now, now * 1000, 1.);

    heimdall_tick_t result = heimdall_clock_ConvertToSystem(f.slave, now, now * 1000, 1., NULL);
    assert(result != HEIMDALL_TICK_INVALID);
    assert(result == now);
    heimdall_clock_main_Unlock(f.main);

    destroy_fixture_simple(&f);
}

/**
 * Check that monotonic clock start is working correctly.
 **/
static void test_clock_with_monotonic_clock(struct heimdall_logger *logger)
{
    fprintf(stderr, "%s:\n", __func__);
    struct clock_fixture_simple f = init_fixture_simple(logger, false, 0, 0);
    heimdall_clock_main_Lock(f.main);

    /* When the input is not driving the clock bus, we should be able to start
     * without any reference point since the output will provide it later. */
    const heimdall_tick_t now = HEIMDALL_TICK_FROM_MS(1000);
    heimdall_clock_Start(f.input, now, now * 1000);

    /* The output wants to start before a reference point is found. */
    heimdall_clock_Start(f.slave, now, now * 1000);

    // TODO
    heimdall_tick_t result = heimdall_clock_ConvertToSystem(f.slave, now, now * 1000, 1., NULL);
    assert(result != HEIMDALL_TICK_INVALID);
    assert(result == now);
    heimdall_clock_main_Unlock(f.main);


    destroy_fixture_simple(&f);
}

/**
 * Check that monotonic clock start accounts for the start date.
 **/
static heimdall_tick_t test_clock_with_monotonic_clock_start(
    struct heimdall_logger *logger,
    heimdall_tick_t input_dejitter,
    heimdall_tick_t output_dejitter,
    struct clock_point start_point,
    struct clock_point output_point,
    struct clock_point convert_point
){
    fprintf(stderr, "%s:\n", __func__);
    struct clock_fixture_simple f = init_fixture_simple(logger, false, input_dejitter, output_dejitter);
    heimdall_clock_main_Lock(f.main);

    /* When the input is not driving the clock bus, we should be able to start
     * without any reference point since the output will provide it later. */
    heimdall_clock_Start(f.input, start_point.system, start_point.stream);

    /* The output wants to start before a reference point is found. */
    heimdall_clock_Start(f.slave, output_point.system, output_point.stream);

    heimdall_tick_t result = heimdall_clock_ConvertToSystem(f.slave, convert_point.system, convert_point.stream, 1., NULL);
    heimdall_clock_main_Unlock(f.main);

    destroy_fixture_simple(&f);
    return result;
}

static heimdall_tick_t test_clock_slave_update(
    struct heimdall_logger *logger,
    struct clock_point start_point,
    struct clock_point output_point
){
    fprintf(stderr, "%s:\n", __func__);
    struct clock_fixture_simple f = init_fixture_simple(logger, false, 0, 0);
    heimdall_clock_main_Lock(f.main);

    /* When the input is not driving the clock bus, we should be able to start
     * without any reference point since the output will provide it later. */
    heimdall_clock_Start(f.input, start_point.system, start_point.stream);
    heimdall_clock_Update(f.master, start_point.system, start_point.stream, 1.);

    /* The output wants to start before a reference point is found. */
    heimdall_clock_Start(f.slave, output_point.system, output_point.stream);

    heimdall_tick_t result = heimdall_clock_Update(f.slave, output_point.system, output_point.stream, 1.);
    heimdall_clock_main_Unlock(f.main);

    destroy_fixture_simple(&f);
    return result;
}

static void test_clock_start_reset(
    struct heimdall_logger *logger
){
    fprintf(stderr, "%s:\n", __func__);
    struct clock_fixture_simple f = init_fixture_simple(logger, false, 0, 0);
    heimdall_clock_main_Lock(f.main);

    const heimdall_tick_t now = HEIMDALL_TICK_FROM_MS(1000);
    struct clock_point start_point;
    start_point = (struct clock_point){ now, now * 1000 };

    /* When the input is not driving the clock bus, we should be able to start
     * without any reference point since the output will provide it later. */
    heimdall_clock_Start(f.input, start_point.system, start_point.stream);
    heimdall_clock_Update(f.master, start_point.system, start_point.stream, 1.);
    heimdall_clock_main_Reset(f.main);

    heimdall_tick_t result;
    result = heimdall_clock_ConvertToSystem(f.slave, start_point.system, start_point.stream, 1., NULL);
    //assert(result == HEIMDALL_TICK_INVALID);

    start_point.system *= 2;
    heimdall_clock_Start(f.input, start_point.system, start_point.stream);
    heimdall_clock_Update(f.master, start_point.system, start_point.stream, 1.);
    result = heimdall_clock_ConvertToSystem(f.slave, start_point.system, start_point.stream, 1., NULL);
    assert(result == start_point.system);
    heimdall_clock_main_Unlock(f.main);

    destroy_fixture_simple(&f);
}

static void test_clock_slave_only(
    struct heimdall_logger *logger
){
    fprintf(stderr, "%s:\n", __func__);
    struct clock_fixture_simple f = init_fixture_simple(logger, false, 0, 0);
    heimdall_clock_main_Lock(f.main);

    const heimdall_tick_t now = HEIMDALL_TICK_FROM_MS(1000);
    struct clock_point start_point = { now, now * 1000 };

    /* When the input is not driving the clock bus, we should be able to start
     * without any reference point since the output will provide it later. */
    heimdall_clock_Start(f.input, start_point.system, start_point.stream);

    heimdall_clock_Start(f.slave, start_point.system, start_point.stream);
    heimdall_tick_t result;
    result = heimdall_clock_ConvertToSystem(f.slave, start_point.system, start_point.stream, 1., NULL);
    assert(result == start_point.system);

    result = heimdall_clock_ConvertToSystem(f.slave, start_point.system, start_point.stream + 1000, 1., NULL);
    assert(result == start_point.system + 1000);
    heimdall_clock_main_Unlock(f.main);

    destroy_fixture_simple(&f);
}
int main(void)
{
    libheimdall_instance_t *libheimdall = libheimdall_new(0, NULL);
    assert(libheimdall != NULL);

    libheimdall_int_t *heimdall = libheimdall->p_libheimdall_int;
    struct heimdall_logger *logger = heimdall->obj.logger;

    const heimdall_tick_t now = HEIMDALL_TICK_FROM_MS(1000);

    //test_clock_without_start(logger);
    test_clock_with_start(logger);
    test_clock_with_start_master(logger);
    test_clock_with_output_dejitter(logger);
    test_clock_with_output_clock_start(logger);
    test_clock_with_monotonic_clock(logger);

    struct clock_point start_point, output_point;
    start_point = (struct clock_point){ now, now * 1000 };
    output_point = (struct clock_point){ now, now * 1000 };

    heimdall_tick_t result = test_clock_with_monotonic_clock_start(
        logger, 0, 0, start_point, start_point, start_point);
    fprintf(stderr, "%" PRId64 " = %" PRId64 "\n", result, start_point.system);
    assert(result == start_point.system);

    /* Monotonic clock must start at the start point. */
    start_point = (struct clock_point){ 2 * now, now * 1000 };
    result = test_clock_with_monotonic_clock_start(
        logger, 0, 0, start_point, output_point, output_point);
    fprintf(stderr, "%" PRId64 " = %" PRId64 "\n", result, start_point.system);
    assert(result == start_point.system);

    /* A point later in the future will be started later also. */
    start_point = (struct clock_point){ now, now * 1000 };
    output_point = (struct clock_point){ now, now * 1000 + 100};
    result = test_clock_with_monotonic_clock_start(
        logger, 0, 0, start_point, output_point, output_point);
    fprintf(stderr, "result(%" PRId64 ") = start_point.system(%" PRId64 ")\n", result, start_point.system + 100);
    assert(result == start_point.system + 100);

    /* Input dejitter and PCR delay are accounted together. */
    result = test_clock_with_monotonic_clock_start(
        logger, 30, 0, start_point, output_point, output_point);
    fprintf(stderr, "%" PRId64 " = %" PRId64 "\n", result, start_point.system + 130);
    assert(result == start_point.system + 130);

    /* Output dejitter will not account the current PCR delay. */
    result = test_clock_with_monotonic_clock_start(
        logger, 0, 100, start_point, output_point, output_point);
    fprintf(stderr, "%" PRId64 " = %" PRId64 "\n", result, start_point.system + 100);
    assert(result == start_point.system + 100);

    /* The highest dejitter value is used. */
    result = test_clock_with_monotonic_clock_start(
        logger, 33, 55, start_point, output_point, output_point);
    fprintf(stderr, "%" PRId64 " = %" PRId64 "\n", result, start_point.system + 133);
    assert(result == start_point.system + 133);

    /* The highest dejitter value is used. */
    result = test_clock_with_monotonic_clock_start(
        logger, 33, 555, start_point, output_point, output_point);
    fprintf(stderr, "%" PRId64 " = %" PRId64 "\n", result, start_point.system + 555);
    assert(result == start_point.system + 555);

    start_point = (struct clock_point){ now, now * 1000 };
    output_point = (struct clock_point){ now, now * 1000 };
    result = test_clock_slave_update(logger, start_point, output_point);
    fprintf(stderr, "%" PRId64 " = %" PRId64 "\n", result, (heimdall_tick_t)0);
    assert(result == 0);

    output_point = (struct clock_point){ now + 10, now * 1000 };
    result = test_clock_slave_update(logger, start_point, output_point);
    fprintf(stderr, "%" PRId64 " = %" PRId64 "\n", result, (heimdall_tick_t)-10);
    assert(result == -10);

    output_point = (struct clock_point){ now, now * 1000 + 10 };
    result = test_clock_slave_update(logger, start_point, output_point);
    fprintf(stderr, "%" PRId64 " = %" PRId64 "\n", result, (heimdall_tick_t)10);
    assert(result == 10);

    test_clock_start_reset(logger);
    test_clock_slave_only(logger);

    libheimdall_release(libheimdall);
}

