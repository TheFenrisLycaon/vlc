/*****************************************************************************
 * thread.c : pthread back-end for LibHeimdall
 *****************************************************************************
 * Copyright (C) 1999-2009 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Jean-Marc Dressler <polux@via.ecp.fr>
 *          Samuel Hocevar <sam@zoy.org>
 *          Gildas Bazin <gbazin@netcourrier.com>
 *          Clément Sténac
 *          Rémi Denis-Courmont
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
#include <heimdall_threads.h>

#include "../libheimdall.h"
#include <stdarg.h>
#include <stdatomic.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <assert.h>

#include <sys/types.h>
#include <unistd.h> /* fsync() */
#include <pthread.h>
#include <sched.h>

#ifdef HAVE_EXECINFO_H
# include <execinfo.h>
#endif
#if defined(__SunOS)
# include <sys/processor.h>
# include <sys/pset.h>
#endif

static unsigned heimdall_clock_prec;

static void heimdall_clock_setup_once (void)
{
    struct timespec res;
    if (unlikely(clock_getres(CLOCK_MONOTONIC, &res) != 0 || res.tv_sec != 0))
        abort ();
    heimdall_clock_prec = (res.tv_nsec + 500) / 1000;
}

/**
 * Print a backtrace to the standard error for debugging purpose.
 */
void heimdall_trace (const char *fn, const char *file, unsigned line)
{
     fprintf (stderr, "at %s:%u in %s\n", file, line, fn);
     fflush (stderr); /* needed before switch to low-level I/O */
#ifdef HAVE_BACKTRACE
     void *stack[20];
     int len = backtrace (stack, ARRAY_SIZE (stack) );
     backtrace_symbols_fd (stack, len, 2);
#endif
     fsync (2);
}

#ifndef NDEBUG
/**
 * Reports a fatal error from the threading layer, for debugging purposes.
 */
static void
heimdall_thread_fatal (const char *action, int error,
                  const char *function, const char *file, unsigned line)
{
    int canc = heimdall_savecancel ();
    fprintf (stderr, "LibHeimdall fatal error %s (%d) in thread %lu ",
             action, error, heimdall_thread_id ());
    heimdall_trace (function, file, line);
    perror ("Thread error");
    fflush (stderr);

    heimdall_restorecancel (canc);
    abort ();
}

# define HEIMDALL_THREAD_ASSERT( action ) \
    if (unlikely(val)) \
        heimdall_thread_fatal (action, val, __func__, __FILE__, __LINE__)
#else
# define HEIMDALL_THREAD_ASSERT( action ) ((void)val)
#endif

int heimdall_threadvar_create (heimdall_threadvar_t *key, void (*destr) (void *))
{
    return pthread_key_create (key, destr);
}

void heimdall_threadvar_delete (heimdall_threadvar_t *p_tls)
{
    pthread_key_delete (*p_tls);
}

int heimdall_threadvar_set (heimdall_threadvar_t key, void *value)
{
    return pthread_setspecific (key, value);
}

void *heimdall_threadvar_get (heimdall_threadvar_t key)
{
    return pthread_getspecific (key);
}

void heimdall_threads_setup (libheimdall_int_t *p_libheimdall)
{
    (void) p_libheimdall;
}

static int heimdall_clone_attr (heimdall_thread_t *th, pthread_attr_t *attr,
                           void *(*entry) (void *), void *data)
{
    int ret;

    /* Block the signals that signals interface plugin handles.
     * If the LibHeimdall caller wants to handle some signals by itself, it should
     * block these before whenever invoking LibHeimdall. And it must obviously not
     * start the Heimdall signals interface plugin.
     *
     * LibHeimdall will normally ignore any interruption caused by an asynchronous
     * signal during a system call. But there may well be some buggy cases
     * where it fails to handle EINTR (bug reports welcome). Some underlying
     * libraries might also not handle EINTR properly.
     */
    sigset_t oldset;
    {
        sigset_t set;
        sigemptyset (&set);
        sigdelset (&set, SIGHUP);
        sigaddset (&set, SIGINT);
        sigaddset (&set, SIGQUIT);
        sigaddset (&set, SIGTERM);

        sigaddset (&set, SIGPIPE); /* We don't want this one, really! */
        pthread_sigmask (SIG_BLOCK, &set, &oldset);
    }

    /* The thread stack size.
     * The lower the value, the less address space per thread, the highest
     * maximum simultaneous threads per process. Too low values will cause
     * stack overflows and weird crashes. Set with caution. Also keep in mind
     * that 64-bits platforms consume more stack than 32-bits one.
     *
     * Thanks to on-demand paging, thread stack size only affects address space
     * consumption. In terms of memory, threads only use what they need
     * (rounded up to the page boundary).
     *
     * For example, on Linux i386, the default is 2 mega-bytes, which supports
     * about 320 threads per processes. */
#define HEIMDALL_STACKSIZE (128 * sizeof (void *) * 1024)

#ifdef HEIMDALL_STACKSIZE
    ret = pthread_attr_setstacksize (attr, HEIMDALL_STACKSIZE);
    assert (ret == 0); /* fails iff HEIMDALL_STACKSIZE is invalid */
#endif

    ret = pthread_create(&th->handle, attr, entry, data);
    pthread_sigmask (SIG_SETMASK, &oldset, NULL);
    pthread_attr_destroy (attr);
    return ret;
}

int heimdall_clone (heimdall_thread_t *th, void *(*entry) (void *), void *data)
{
    pthread_attr_t attr;

    pthread_attr_init (&attr);
    return heimdall_clone_attr (th, &attr, entry, data);
}

void heimdall_join(heimdall_thread_t th, void **result)
{
    int val = pthread_join(th.handle, result);
    HEIMDALL_THREAD_ASSERT ("joining thread");
}

HEIMDALL_WEAK unsigned long heimdall_thread_id(void)
{
     static thread_local unsigned char dummy;

     static_assert (UINTPTR_MAX <= ULONG_MAX, "Type size mismatch");
     return (uintptr_t)(void *)&dummy;
}

HEIMDALL_WEAK void (heimdall_thread_set_name)(const char *name)
{
    HEIMDALL_UNUSED(name);
}

void heimdall_cancel(heimdall_thread_t th)
{
    pthread_cancel(th.handle);
}

int heimdall_savecancel (void)
{
    int state;
    int val = pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, &state);

    HEIMDALL_THREAD_ASSERT ("saving cancellation");
    return state;
}

void heimdall_restorecancel (int state)
{
#ifndef NDEBUG
    int oldstate, val;

    val = pthread_setcancelstate (state, &oldstate);
    /* This should fail if an invalid value for given for state */
    HEIMDALL_THREAD_ASSERT ("restoring cancellation");

    if (unlikely(oldstate != PTHREAD_CANCEL_DISABLE))
         heimdall_thread_fatal ("restoring cancellation while not disabled", EINVAL,
                           __func__, __FILE__, __LINE__);
#else
    pthread_setcancelstate (state, NULL);
#endif
}

void heimdall_testcancel (void)
{
    pthread_testcancel ();
}

heimdall_tick_t heimdall_tick_now (void)
{
    struct timespec ts;

    if (unlikely(clock_gettime(CLOCK_MONOTONIC, &ts) != 0))
        abort ();

    return heimdall_tick_from_timespec( &ts );
}

#undef heimdall_tick_wait
void heimdall_tick_wait (heimdall_tick_t deadline)
{
    static pthread_once_t heimdall_clock_once = PTHREAD_ONCE_INIT;

    /* If the deadline is already elapsed, or within the clock precision,
     * do not even bother the system timer. */
    pthread_once(&heimdall_clock_once, heimdall_clock_setup_once);
    deadline -= heimdall_clock_prec;

    struct timespec ts;

    heimdall_tick_to_timespec(&ts, deadline);
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL) == EINTR);
}

#undef heimdall_tick_sleep
void heimdall_tick_sleep (heimdall_tick_t delay)
{
    struct timespec ts;

    heimdall_tick_to_timespec(&ts, delay);
    while (clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &ts) == EINTR);
}

unsigned heimdall_GetCPUCount(void)
{
#if defined(HAVE_SCHED_GETAFFINITY)
    cpu_set_t cpu;

    CPU_ZERO(&cpu);
    if (sched_getaffinity (0, sizeof (cpu), &cpu) < 0)
        return 1;

    return CPU_COUNT (&cpu);

#elif defined(__SunOS)
    unsigned count = 0;
    int type;
    u_int numcpus;
    processor_info_t cpuinfo;

    processorid_t *cpulist = heimdall_alloc (sysconf(_SC_NPROCESSORS_MAX), sizeof (*cpulist));
    if (unlikely(cpulist == NULL))
        return 1;

    if (pset_info(PS_MYID, &type, &numcpus, cpulist) == 0)
    {
        for (u_int i = 0; i < numcpus; i++)
            if (processor_info (cpulist[i], &cpuinfo) == 0)
                count += (cpuinfo.pi_state == P_ONLINE);
    }
    else
        count = sysconf (_SC_NPROCESSORS_ONLN);
    free (cpulist);
    return count ? count : 1;
#elif defined(_SC_NPROCESSORS_ONLN)
    return sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROCESSORS_CONF)
    return sysconf(_SC_NPROCESSORS_CONF);
#else
#   warning "heimdall_GetCPUCount is not implemented for your platform"
    return 1;
#endif
}
