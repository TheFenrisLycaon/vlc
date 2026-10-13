/*****************************************************************************
 * thread.c : Win32 back-end for LibHeimdall
 *****************************************************************************
 * Copyright (C) 1999-2016 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Jean-Marc Dressler <polux@via.ecp.fr>
 *          Samuel Hocevar <sam@zoy.org>
 *          Gildas Bazin <gbazin@netcourrier.com>
 *          Clément Sténac
 *          Rémi Denis-Courmont
 *          Pierre Ynard
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

#define _DECL_DLLMAIN
#include <windows.h>
#include <process.h>
#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_charset.h>

#include "../libheimdall.h"
#include <stdarg.h>
#include <stdatomic.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <heimdall_atomic.h>

#if WINAPI_FAMILY_PARTITION (WINAPI_PARTITION_DESKTOP)
#include <mmsystem.h>
#endif

#ifndef NTDDI_WIN10_RS1
#define NTDDI_WIN10_RS1  0x0A000002
#endif

#if NTDDI_VERSION >= NTDDI_WIN10_RS1 && defined(__MINGW64_VERSION_MAJOR) && __MINGW64_VERSION_MAJOR < 10
WINBASEAPI HRESULT WINAPI SetThreadDescription(HANDLE,PCWSTR);
#endif

/*** Static mutex and condition variable ***/
static SRWLOCK super_lock = SRWLOCK_INIT;

/*** Threads ***/
static thread_local struct heimdall_thread *current_thread_ctx = NULL;

struct heimdall_thread
{
    HANDLE         id;

    bool           killable;
    atomic_uint    killed;
    heimdall_cleanup_t *cleaners;

    void        *(*entry) (void *);
    void          *data;
};

/*** Thread-specific variables (TLS) ***/
struct heimdall_threadvar
{
    DWORD                 id;
    void                (*destroy) (void *);
    struct heimdall_threadvar *prev;
    struct heimdall_threadvar *next;
} *heimdall_threadvar_last = NULL;

int heimdall_threadvar_create (heimdall_threadvar_t *p_tls, void (*destr) (void *))
{
    struct heimdall_threadvar *var = malloc (sizeof (*var));
    if (unlikely(var == NULL))
        return errno;

    var->id = TlsAlloc();
    if (var->id == TLS_OUT_OF_INDEXES)
    {
        free (var);
        return EAGAIN;
    }
    var->destroy = destr;
    var->next = NULL;
    *p_tls = var;

    if (destr != NULL)
    {
        AcquireSRWLockExclusive(&super_lock);
        var->prev = heimdall_threadvar_last;
        if (var->prev)
            var->prev->next = var;

        heimdall_threadvar_last = var;
        ReleaseSRWLockExclusive(&super_lock);
    }
    return 0;
}

void heimdall_threadvar_delete (heimdall_threadvar_t *p_tls)
{
    struct heimdall_threadvar *var = *p_tls;

    if (var->destroy != NULL)
    {
        AcquireSRWLockExclusive(&super_lock);
        if (var->prev != NULL)
            var->prev->next = var->next;

        if (var->next != NULL)
            var->next->prev = var->prev;
        else
            heimdall_threadvar_last = var->prev;
        ReleaseSRWLockExclusive(&super_lock);
    }
    TlsFree (var->id);
    free (var);
}

int heimdall_threadvar_set (heimdall_threadvar_t key, void *value)
{
    int saved = GetLastError ();

    if (!TlsSetValue(key->id, value))
        return ENOMEM;

    SetLastError(saved);
    return 0;
}

void *heimdall_threadvar_get (heimdall_threadvar_t key)
{
    int saved = GetLastError ();
    void *value = TlsGetValue (key->id);

    SetLastError(saved);
    return value;
}

static void heimdall_threadvars_cleanup(void)
{
    const struct heimdall_threadvar *key;

retry:
    AcquireSRWLockShared(&super_lock);
    for (key = heimdall_threadvar_last; key != NULL; key = key->prev)
    {
        void *value = TlsGetValue(key->id);
        if (value != NULL)
        {
            ReleaseSRWLockShared(&super_lock);
            TlsSetValue(key->id, NULL);
            assert(key->destroy != NULL);
            key->destroy(value);
            goto retry;
        }
    }
    ReleaseSRWLockShared(&super_lock);
}

/*** Futeces^WAddress waits ***/
typedef HRESULT (WINAPI *SetThreadDescription_ptr)(HANDLE, PCWSTR);
static SetThreadDescription_ptr SetThreadDescription_;
#if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
typedef BOOL (WINAPI *WaitOnAddress_ptr)(VOID volatile *, PVOID, SIZE_T, DWORD);
static WaitOnAddress_ptr WaitOnAddress_;
#define WaitOnAddress (*WaitOnAddress_)
typedef VOID (WINAPI *WakeByAddressAll_ptr)(PVOID);
static WakeByAddressAll_ptr WakeByAddressAll_;
#define WakeByAddressAll (*WakeByAddressAll_)
typedef VOID (WINAPI *WakeByAddressSingle_ptr)(PVOID);
static WakeByAddressSingle_ptr WakeByAddressSingle_;
#define WakeByAddressSingle (*WakeByAddressSingle_)

static struct wait_addr_bucket
{
    SRWLOCK lock;
    CONDITION_VARIABLE wait;
} wait_addr_buckets[32];

static struct wait_addr_bucket *wait_addr_get_bucket(void volatile *addr)
{
    uintptr_t u = (uintptr_t)addr;

    return wait_addr_buckets + ((u >> 3) % ARRAY_SIZE(wait_addr_buckets));
}

static BOOL WINAPI WaitOnAddressFallback(void volatile *addr, void *value,
                                         SIZE_T size, DWORD ms)
{
    struct wait_addr_bucket *bucket = wait_addr_get_bucket(addr);
    uint64_t futex, val = 0;
    BOOL ret = FALSE;

    AcquireSRWLockExclusive(&bucket->lock);

    switch (size)
    {
        case 1:
            futex = atomic_load_explicit((atomic_char *)addr,
                                         memory_order_relaxed);
            val = *(const char *)value;
            break;
        case 2:
            futex = atomic_load_explicit((atomic_short *)addr,
                                         memory_order_relaxed);
            val = *(const short *)value;
            break;
        case 4:
            futex = atomic_load_explicit((atomic_int *)addr,
                                         memory_order_relaxed);
            val = *(const int *)value;
            break;
        case 8:
            futex = atomic_load_explicit((atomic_llong *)addr,
                                         memory_order_relaxed);
            val = *(const long long *)value;
            break;
        default:
            heimdall_assert_unreachable();
    }

    if (futex == val)
        ret = SleepConditionVariableSRW(&bucket->wait, &bucket->lock, ms, 0);

    ReleaseSRWLockExclusive(&bucket->lock);
    return ret;
}

static void WINAPI WakeByAddressFallback(void *addr)
{
    struct wait_addr_bucket *bucket = wait_addr_get_bucket(addr);

    /* Acquire the bucket critical section (only) to enforce proper sequencing.
     * The critical section does not protect any actual memory object. */
    AcquireSRWLockExclusive(&bucket->lock);
    /* No other threads can hold the lock for this bucket while it is held
     * here. Thus any other thread either:
     * - is already sleeping in SleepConditionVariableCS(), and to be woken up
     *   by the following WakeAllConditionVariable(), or
     * - has yet to retrieve the value at the wait address (with the
     *   'switch (size)' block). */
    ReleaseSRWLockExclusive(&bucket->lock);
    /* At this point, other threads can retrieve the value at the wait address.
     * But the value will have already been changed by our call site, thus
     * (futex == val) will be false, and the threads will not go to sleep. */

    /* Wake up any thread that was already sleeping. Since there are more than
     * one wait address per bucket, all threads must be woken up :-/ */
    WakeAllConditionVariable(&bucket->wait);
}

static void heimdall_wait_addr_init(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(wait_addr_buckets); i++)
    {
        struct wait_addr_bucket *bucket = wait_addr_buckets + i;

        InitializeSRWLock(&bucket->lock);
        InitializeConditionVariable(&bucket->wait);
    }
    WaitOnAddress_ = WaitOnAddressFallback;
    WakeByAddressAll_ = WakeByAddressFallback;
    WakeByAddressSingle_ = WakeByAddressFallback;
}
#endif

void heimdall_atomic_wait(void *addr, unsigned val)
{
    WaitOnAddress(addr, &val, sizeof (val), INFINITE);
}

int heimdall_atomic_timedwait(void *addr, unsigned val, heimdall_tick_t deadline)
{
    heimdall_tick_t delay;

    for(;;)
    {
        delay = deadline - heimdall_tick_now();

        if (delay < 0)
            return ETIMEDOUT; // deadline passed

        DWORD ms;
        int64_t idelay = MS_FROM_HEIMDALL_TICK(delay);
        static_assert(sizeof(unsigned long) <= sizeof(DWORD), "unknown max DWORD");
        if (unlikely(idelay > ULONG_MAX))
            ms = ULONG_MAX;
        else
            ms = idelay;

        if (WaitOnAddress(addr, &val, sizeof (val), ms))
            return 0;
        if (GetLastError() == ERROR_TIMEOUT)
            return ETIMEDOUT;
    }
}

void heimdall_atomic_notify_one(void *addr)
{
    WakeByAddressSingle(addr);
}

void heimdall_atomic_notify_all(void *addr)
{
    WakeByAddressAll(addr);
}

/*** Threads ***/
static
unsigned
__stdcall ThreadEntry (void *p)
{
    struct heimdall_thread *th = p;

    current_thread_ctx = th;
    th->killable = true;
    th->data = th->entry (th->data);
    assert(th->data != HEIMDALL_THREAD_CANCELED); // don't hijack our internal values
    current_thread_ctx = NULL;

    return 0;
}

int heimdall_clone (heimdall_thread_t *p_handle, void *(*entry) (void *),
               void *data)
{
    struct heimdall_thread *th = malloc (sizeof (*th));
    if (unlikely(th == NULL))
        return ENOMEM;
    th->entry = entry;
    th->data = data;
    th->killable = false; /* not until heimdall_entry() ! */
    atomic_init(&th->killed, false);
    th->cleaners = NULL;

    HANDLE h;
    /* When using the MSVCRT C library you have to use the _beginthreadex
     * function instead of CreateThread, otherwise you'll end up with
     * memory leaks and the signal functions not working (see Microsoft
     * Knowledge Base, article 104641) */
    h = (HANDLE)(uintptr_t) _beginthreadex (NULL, 0, ThreadEntry, th, 0, NULL);
    if (h == 0)
    {
        int err = errno;
        free (th);
        return err;
    }

    th->id = h;

    if (p_handle != NULL)
        *p_handle = th;

    return 0;
}

void heimdall_join (heimdall_thread_t th, void **result)
{
    DWORD ret;

    do
    {
        ret = WaitForSingleObject(th->id, INFINITE);
        assert(ret != WAIT_ABANDONED_0);
    }
    while (ret == WAIT_FAILED);

    if (result != NULL)
        *result = th->data;
    CloseHandle (th->id);
    free(th);
}

unsigned long heimdall_thread_id (void)
{
    return GetCurrentThreadId ();
}

void (heimdall_thread_set_name)(const char *name)
{
    if (SetThreadDescription_)
    {
        wchar_t *wname = ToWide(name);
        SetThreadDescription_(GetCurrentThread(), wname);
        free(wname);
    }
}

/*** Thread cancellation ***/

/* APC procedure for thread cancellation */
static void CALLBACK heimdall_cancel_self (ULONG_PTR self)
{
    (void) self;
}

void heimdall_cancel (heimdall_thread_t th)
{
    atomic_store_explicit(&th->killed, true, memory_order_release);
    heimdall_atomic_notify_one(&th->killed);

    QueueUserAPC (heimdall_cancel_self, th->id, (uintptr_t)th);
}

int heimdall_savecancel (void)
{
    struct heimdall_thread *th = current_thread_ctx;
    if (th == NULL)
        return false; /* Main thread - cannot be cancelled anyway */

    bool state = th->killable;
    th->killable = false;
    return state;
}

void heimdall_restorecancel (int state)
{
    struct heimdall_thread *th = current_thread_ctx;
    assert (state == false || state == true);

    if (th == NULL)
        return; /* Main thread - cannot be cancelled anyway */

    assert (!th->killable);
    th->killable = !!state;
}

_Noreturn static void heimdall_docancel(struct heimdall_thread *th)
{
    th->killable = false; /* Do not re-enter cancellation cleanup */

    for (heimdall_cleanup_t *p = th->cleaners; p != NULL; p = p->next)
        p->proc (p->data);

    th->data = HEIMDALL_THREAD_CANCELED;
    _endthreadex(0);
}

void heimdall_testcancel (void)
{
    struct heimdall_thread *th = current_thread_ctx;
    if (th == NULL)
        return; /* Main thread - cannot be cancelled anyway */
    if (!th->killable)
        return;
    if (!atomic_load_explicit(&th->killed, memory_order_relaxed))
        return;

    heimdall_docancel(th);
}

void heimdall_control_cancel (heimdall_cleanup_t *cleaner)
{
    /* NOTE: This function only modifies thread-specific data, so there is no
     * need to lock anything. */

    struct heimdall_thread *th = current_thread_ctx;
    if (th == NULL)
        return; /* Main thread - cannot be cancelled anyway */

    if (cleaner != NULL)
    {
        /* cleaner is a pointer to the caller stack, no need to allocate
            * and copy anything. As a nice side effect, this cannot fail. */
        cleaner->next = th->cleaners;
        th->cleaners = cleaner;
    }
    else
    {
        th->cleaners = th->cleaners->next;
    }
}

/*** Clock ***/
static union
{
    struct
    {
        LARGE_INTEGER freq;
    } perf;
} clk;

static heimdall_tick_t mdate_perf (void)
{
    /* We don't need the real date, just the value of a high precision timer */
    LARGE_INTEGER counter;
    if (unlikely(!QueryPerformanceCounter(&counter)))
        abort ();

    /* Convert to from (1/freq) to microsecond resolution */
    /* We need to split the division to avoid 63-bits overflow */
    return heimdall_tick_from_frac(counter.QuadPart, clk.perf.freq.QuadPart);
}

static heimdall_tick_t mdate_perf_100ns(void)
{
    /* We don't need the real date, just the value of a high precision timer */
    LARGE_INTEGER counter;
    if (unlikely(!QueryPerformanceCounter(&counter)))
        abort ();

    return HEIMDALL_TICK_FROM_MSFTIME(counter.QuadPart);
}

static heimdall_tick_t mdate_wall (void)
{
    FILETIME ts;
    ULARGE_INTEGER s;

#if _WIN32_WINNT >= _WIN32_WINNT_WIN8
    GetSystemTimePreciseAsFileTime (&ts);
#else // _WIN32_WINNT < _WIN32_WINNT_WIN8
    GetSystemTimeAsFileTime (&ts);
#endif // _WIN32_WINNT < _WIN32_WINNT_WIN8
    s.LowPart = ts.dwLowDateTime;
    s.HighPart = ts.dwHighDateTime;
    /* hundreds of nanoseconds */
    static_assert ((10000000 % CLOCK_FREQ) == 0, "Broken frequencies ratio");
    return HEIMDALL_TICK_FROM_MSFTIME(s.QuadPart);
}

#if WINAPI_FAMILY_PARTITION (WINAPI_PARTITION_DESKTOP)
static heimdall_tick_t mdate_multimedia(void)
{
    DWORD ts = timeGetTime ();

    /* milliseconds */
    static_assert ((CLOCK_FREQ % 1000) == 0, "Broken frequencies ratio");
    return HEIMDALL_TICK_FROM_MS(ts);
}
#endif

static heimdall_tick_t (*mdate_selected) (void) = mdate_wall;

heimdall_tick_t heimdall_tick_now (void)
{
    return mdate_selected ();
}

void (heimdall_tick_wait)(heimdall_tick_t deadline)
{
    heimdall_tick_t delay;
    struct heimdall_thread *th = current_thread_ctx;
    if (likely(th != NULL) && th->killable)
    {
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
        do
        {
            if (atomic_load_explicit(&th->killed, memory_order_acquire))
                heimdall_docancel(th);
        }
        while (heimdall_atomic_timedwait(&th->killed, false, deadline) == 0);

        return;
#else
        if (atomic_load_explicit(&th->killed, memory_order_relaxed))
            heimdall_docancel(th);
#endif
    }

    while ((delay = (deadline - heimdall_tick_now())) > 0)
    {
        int64_t idelay = MS_FROM_HEIMDALL_TICK(delay + HEIMDALL_TICK_FROM_MS(1)-1);
        DWORD delay_ms;
        static_assert(sizeof(unsigned long) <= sizeof(DWORD), "unknown max DWORD");
        if (unlikely(idelay > ULONG_MAX))
            delay_ms = ULONG_MAX;
        else
            delay_ms = idelay;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
        Sleep(delay_ms);
#else
        SleepEx(delay_ms, TRUE);

        if (likely(th != NULL) && th->killable)
        {
            if (atomic_load_explicit(&th->killed, memory_order_relaxed))
                heimdall_docancel(th);
        }
#endif
        if (delay_ms != ULONG_MAX)
            break;
    }
}

void (heimdall_tick_sleep)(heimdall_tick_t delay)
{
    heimdall_tick_wait (heimdall_tick_now () + delay);
}

static void SelectClockSource(libheimdall_int_t *obj)
{
    // speed comparison / granularity / counts during sleep
    // multimedia:102000 /    5 ms / no
    // perf:      155612 / ~100 ns / yes
    // wall:      181593 /  100 ns / yes

    char *str = var_InheritString(obj, "clock-source");
    const char *name = str != NULL ? str : "perf";
#if WINAPI_FAMILY_PARTITION (WINAPI_PARTITION_DESKTOP)
    if (!strcmp (name, "multimedia"))
    {
        TIMECAPS caps;

        msg_Dbg (obj, "using multimedia timers as clock source");
        if (timeGetDevCaps (&caps, sizeof (caps)) != MMSYSERR_NOERROR)
            abort();
        msg_Dbg (obj, " min period: %u ms, max period: %u ms",
                 caps.wPeriodMin, caps.wPeriodMax);
        mdate_selected = mdate_multimedia;

        timeBeginPeriod(__MAX(5, caps.wPeriodMin));
    }
    else
#endif
    if (!strcmp (name, "perf"))
    {
        msg_Dbg (obj, "using performance counters as clock source");
        if (!QueryPerformanceFrequency (&clk.perf.freq))
            abort ();
        msg_Dbg (obj, " frequency: %llu Hz", clk.perf.freq.QuadPart);
        if (clk.perf.freq.QuadPart == 10000000)
            mdate_selected = mdate_perf_100ns;
        else
            mdate_selected = mdate_perf;
    }
    else
    {
        msg_Err (obj, "invalid clock source \"%s\"", name);
        abort ();
    }
    free (str);
}


/*** CPU ***/
unsigned heimdall_GetCPUCount (void)
{
    SYSTEM_INFO systemInfo;

    GetNativeSystemInfo(&systemInfo);

    return systemInfo.dwNumberOfProcessors;
}


/*** Initialization ***/

void heimdall_threads_setup(libheimdall_int_t *heimdall)
{
    SelectClockSource(heimdall);

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
    /* Raise default priority of the current process */
#ifndef ABOVE_NORMAL_PRIORITY_CLASS
#   define ABOVE_NORMAL_PRIORITY_CLASS 0x00008000
#endif
    if (var_InheritBool(heimdall, "high-priority"))
    {
        if (SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS)
         || SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
            msg_Dbg(heimdall, "raised process priority");
        else
            msg_Dbg(heimdall, "could not raise process priority");
    }
#endif
}

#define LOOKUP(s) (((s##_) = (s##_ptr)GetProcAddress(h, #s)) != NULL)

int __stdcall DllMain (void *hinstDll, DWORD fdwReason, void *lpvReserved)
{
    (void) hinstDll;
    (void) lpvReserved;

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
#if NTDDI_VERSION >= NTDDI_WIN10_RS1
            SetThreadDescription_ = SetThreadDescription;
#else // !NTDDI_WIN10_RS1
            HMODULE h;
            h = GetModuleHandle(TEXT("kernelbase.dll"));
            if (h == NULL)
                h = GetModuleHandle(TEXT("api-ms-win-core-processthreads-l1-1-3.dll"));
            if (h != NULL)
                LOOKUP(SetThreadDescription);
            else
                SetThreadDescription_ = NULL;
#endif // !NTDDI_WIN10_RS1

#if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
            h = GetModuleHandle(TEXT("api-ms-win-core-synch-l1-2-0.dll"));
            if (h == NULL || !LOOKUP(WaitOnAddress)
             || !LOOKUP(WakeByAddressAll) || !LOOKUP(WakeByAddressSingle))
            {
                heimdall_wait_addr_init();
            }
#endif
            break;
        }

        case DLL_THREAD_DETACH:
            heimdall_threadvars_cleanup();
            break;
    }
    return TRUE;
}
