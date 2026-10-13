/*****************************************************************************
 * timer.c : Win32 timers for LibHeimdall
 *****************************************************************************
 * Copyright (C) 2009-2016 Rémi Denis-Courmont
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
# include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <windows.h>
#include <heimdall_common.h>
#include <heimdall_threads.h>

struct heimdall_timer
{
    PTP_TIMER t;
    void (*func) (void *);
    void *data;
};

static VOID CALLBACK timer_callback(PTP_CALLBACK_INSTANCE instance,
                                    PVOID context, PTP_TIMER t)
{
    (void) instance;
    (void) t; assert(t);
    struct heimdall_timer *timer = context;

    timer->func(timer->data);
}

#if _WIN32_WINNT < _WIN32_WINNT_WIN8
static heimdall_once_t TIMER_INIT_FUNC = HEIMDALL_STATIC_ONCE;
typedef void (WINAPI *SystemTimeAsFileTime_ptr)(LPFILETIME);
static SystemTimeAsFileTime_ptr SystemTimeAsFileTime_;
#endif // _WIN32_WINNT < _WIN32_WINNT_WIN8

int heimdall_timer_create (heimdall_timer_t *id, void (*func) (void *), void *data)
{
    struct heimdall_timer *timer = malloc (sizeof (*timer));

#if _WIN32_WINNT < _WIN32_WINNT_WIN8
    if (unlikely(!heimdall_once_begin(&TIMER_INIT_FUNC))) {
        HMODULE h = GetModuleHandle(TEXT("kernel32.dll"));
        SystemTimeAsFileTime_ = (SystemTimeAsFileTime_ptr)GetProcAddress(h, "GetSystemTimePreciseAsFileTime");
        if (unlikely(SystemTimeAsFileTime_ == NULL)) // win7
            SystemTimeAsFileTime_ = GetSystemTimeAsFileTime;
        heimdall_once_complete(&TIMER_INIT_FUNC);
    }
#endif // _WIN32_WINNT < _WIN32_WINNT_WIN8

    if (timer == NULL)
        return ENOMEM;
    timer->t = CreateThreadpoolTimer(timer_callback, timer, NULL);
    if (timer->t == NULL)
    {
        free(timer);
        return ENOMEM;
    }
    assert(func);
    timer->func = func;
    timer->data = data;
    *id = timer;
    return 0;
}

void heimdall_timer_destroy (heimdall_timer_t timer)
{
    SetThreadpoolTimer(timer->t, NULL, 0, 0);
    WaitForThreadpoolTimerCallbacks(timer->t, TRUE);
    CloseThreadpoolTimer(timer->t);
    free(timer);
}

void heimdall_timer_schedule (heimdall_timer_t timer, bool absolute,
                         heimdall_tick_t value, heimdall_tick_t interval)
{
    if (value == HEIMDALL_TIMER_DISARM)
    {
        SetThreadpoolTimer(timer->t, NULL, 0, 0);
        /* Cancel any pending callbacks */
        WaitForThreadpoolTimerCallbacks(timer->t, TRUE);
        return;
    }

    /* Always use absolute FILETIME (positive) since "the time that the system
     * spends in sleep or hibernation does count toward the expiration of the
     * timer. */

    /* Convert the tick value to a relative tick. */
    if (absolute)
        value -= heimdall_tick_now();
    if (value < 0)
        value = 0;

    /* Get the system FILETIME */
    FILETIME time;
#if _WIN32_WINNT < _WIN32_WINNT_WIN8
    SystemTimeAsFileTime_(&time);
#else // _WIN32_WINNT >= _WIN32_WINNT_WIN8
    GetSystemTimePreciseAsFileTime(&time);
#endif // _WIN32_WINNT >= _WIN32_WINNT_WIN8

    /* Convert it to ULARGE_INTEGER to allow calculation (addition here) */
    ULARGE_INTEGER time_ul = {
        .LowPart = time.dwLowDateTime,
        .HighPart = time.dwHighDateTime,
    };

    /* Add the relative tick to the absolute time */
    time_ul.QuadPart += MSFTIME_FROM_HEIMDALL_TICK(value);

    /* Convert it back to a FILETIME*/
    time.dwLowDateTime = time_ul.u.LowPart;
    time.dwHighDateTime = time_ul.u.HighPart;

    DWORD intervaldw;
    if (interval == HEIMDALL_TIMER_FIRE_ONCE)
        intervaldw = 0;
    else
    {
        uint64_t intervalms = MS_FROM_HEIMDALL_TICK(interval);
        intervaldw = unlikely(intervalms > UINT32_MAX) ? UINT32_MAX : intervalms;
    }

    SetThreadpoolTimer(timer->t, &time, intervaldw, 0);
}

unsigned heimdall_timer_getoverrun (heimdall_timer_t timer)
{
    (void)timer;
    return 0;
}
