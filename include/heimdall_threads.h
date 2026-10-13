/*****************************************************************************
 * heimdall_threads.h : threads implementation for the Lycaria client
 * This header provides portable declarations for mutexes & conditions
 *****************************************************************************
 * Copyright (C) 1999, 2002 VLC authors and VideoLAN
 * Copyright © 2007-2016 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Jean-Marc Dressler <polux@via.ecp.fr>
 *          Samuel Hocevar <sam@via.ecp.fr>
 *          Gildas Bazin <gbazin@netcourrier.com>
 *          Christophe Massiot <massiot@via.ecp.fr>
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

#ifndef HEIMDALL_THREADS_H_
#define HEIMDALL_THREADS_H_

#ifndef __cplusplus
#include <stdatomic.h>
#endif

#include <heimdall_tick.h>

/**
 * \ingroup os
 * \defgroup thread Threads and synchronization primitives
 * @{
 * \file
 * Thread primitive declarations
 */

/**
 * Issues an explicit deferred cancellation point.
 *
 * This has no effects if thread cancellation is disabled.
 * This can be called when there is a rather slow non-sleeping operation.
 * This is also used to force a cancellation point in a function that would
 * otherwise <em>not always</em> be one (block_FifoGet() is an example).
 */
HEIMDALL_API void heimdall_testcancel(void);

#if defined (_WIN32)
# include <process.h>
# ifndef ETIMEDOUT
#  define ETIMEDOUT 10060 /* This is the value in winsock.h. */
# endif

typedef struct heimdall_thread *heimdall_thread_t;
# define HEIMDALL_THREAD_CANCELED ((void*) UINTPTR_MAX)

typedef struct heimdall_threadvar *heimdall_threadvar_t;
typedef struct heimdall_timer *heimdall_timer_t;

#elif defined (__OS2__)
# include <errno.h>

typedef struct heimdall_thread *heimdall_thread_t;
#define HEIMDALL_THREAD_CANCELED ((void*) UINTPTR_MAX)

typedef struct heimdall_threadvar *heimdall_threadvar_t;
typedef struct heimdall_timer *heimdall_timer_t;

# define pthread_sigmask  sigprocmask

#elif defined (__ANDROID__)      /* pthreads subset without pthread_cancel() */
# include <unistd.h>
# include <pthread.h>
# include <poll.h>
# define LIBHEIMDALL_USE_PTHREAD_CLEANUP   1

typedef struct heimdall_thread *heimdall_thread_t;
#define HEIMDALL_THREAD_CANCELED ((void*) UINTPTR_MAX)
typedef pthread_key_t   heimdall_threadvar_t;
typedef struct heimdall_timer *heimdall_timer_t;

#else /* POSIX threads */
# include <unistd.h> /* _POSIX_SPIN_LOCKS */
# include <pthread.h>

/**
 * Whether LibHeimdall threads are based on POSIX threads.
 */
# define LIBHEIMDALL_USE_PTHREAD           1

/**
 * Whether LibHeimdall thread cancellation is based on POSIX threads.
 */
# define LIBHEIMDALL_USE_PTHREAD_CLEANUP   1

/**
 * Thread handle.
 */
typedef struct
{
    pthread_t handle;
} heimdall_thread_t;

/**
 * Return value of a canceled thread.
 */
#define HEIMDALL_THREAD_CANCELED PTHREAD_CANCELED

/**
 * Thread-local key handle.
 *
 * \ingroup threadvar
 */
typedef pthread_key_t   heimdall_threadvar_t;

/**
 * Threaded timer handle.
 *
 * \ingroup timer
 */
typedef struct heimdall_timer *heimdall_timer_t;

#endif

/**
 * \defgroup mutex Mutual exclusion locks
 * @{
 */
/**
 * Mutex.
 *
 * Storage space for a mutual exclusion lock.
 */
typedef struct
{
    union {
#ifndef __cplusplus
        struct {
            atomic_uint value;
            atomic_uint recursion;
            atomic_ulong owner;
        };
#endif
        struct {
            unsigned int value;
            unsigned int recursion;
            unsigned long owner;
        } dummy;
    };
} heimdall_mutex_t;

/**
 * Static initializer for (static) mutex.
 *
 * \note This only works in C code.
 * In C++, consider using a global ::heimdall::threads::mutex instance instead.
 */
#define HEIMDALL_STATIC_MUTEX { \
    .value = 0, \
    .recursion = 0, \
    .owner = 0, \
}

/**
 * Initializes a fast mutex.
 *
 * Recursive locking of a fast mutex is undefined behaviour. (In debug builds,
 * recursive locking will cause an assertion failure.)
 */
HEIMDALL_API void heimdall_mutex_init(heimdall_mutex_t *);

/**
 * Initializes a recursive mutex.
 * \warning This is strongly discouraged. Please use normal mutexes.
 */
HEIMDALL_API void heimdall_mutex_init_recursive(heimdall_mutex_t *);

/**
 * Acquires a mutex.
 *
 * If needed, this waits for any other thread to release it.
 *
 * \warning Beware of deadlocks when locking multiple mutexes at the same time,
 * or when using mutexes from callbacks.
 *
 * \note This function is not a cancellation point.
 */
HEIMDALL_API void heimdall_mutex_lock(heimdall_mutex_t *);

/**
 * Tries to acquire a mutex.
 *
 * This function acquires the mutex if and only if it is not currently held by
 * another thread. This function never sleeps and can be used in delay-critical
 * code paths.
 *
 * \note This function is not a cancellation point.
 *
 * \warning If this function fails, then the mutex is held... by another
 * thread. The calling thread must deal with the error appropriately. That
 * typically implies postponing the operations that would have required the
 * mutex. If the thread cannot defer those operations, then it must use
 * heimdall_mutex_lock(). If in doubt, use heimdall_mutex_lock() instead.
 *
 * @return 0 if the mutex could be acquired, an error code otherwise.
 */
HEIMDALL_API int heimdall_mutex_trylock( heimdall_mutex_t * ) HEIMDALL_USED;

/**
 * Releases a mutex.
 *
 * If the mutex is not held by the calling thread, the behaviour is undefined.
 *
 * \note This function is not a cancellation point.
 */
HEIMDALL_API void heimdall_mutex_unlock(heimdall_mutex_t *);

/**
 * Checks if a mutex is locked.
 *
 * This function checks if the calling thread holds a given mutual exclusion
 * lock. It has no side effects and is essentially intended for run-time
 * debugging.
 *
 * @note To assert that the calling thread holds a lock, the helper macro
 * heimdall_mutex_assert() should be used instead of this function.
 *
 * @note While it is nominally possible to implement recursive lock semantics
 * with this function, heimdall_mutex_init_recursive() should be used instead to
 * create a recursive mutex explicitly..
 *
 * @retval false the mutex is not locked by the calling thread
 * @retval true the mutex is locked by the calling thread
 */
HEIMDALL_API bool heimdall_mutex_held(const heimdall_mutex_t *) HEIMDALL_USED;

/**
 * Asserts that a mutex is locked by the calling thread.
 */
#define heimdall_mutex_assert(m) assert(heimdall_mutex_held(m))

/** @} */

/**
 * \defgroup condvar Condition variables
 *
 * The condition variable is the most common and generic mean for threads to
 * wait for events triggered by other threads.
 *
 * See also POSIX @c pthread_cond_t .
 * @{
 */

struct heimdall_cond_waiter;

/**
 * Condition variable.
 *
 * Storage space for a thread condition variable.
 */
typedef struct
{
    struct heimdall_cond_waiter *head;
    heimdall_mutex_t lock;
} heimdall_cond_t;

/**
 * Static initializer for (static) condition variable.
 */
#define HEIMDALL_STATIC_COND { NULL, HEIMDALL_STATIC_MUTEX }

/**
 * Initializes a condition variable.
 */
HEIMDALL_API void heimdall_cond_init(heimdall_cond_t *);

/**
 * Wakes up one thread waiting on a condition variable.
 *
 * If any thread is currently waiting on the condition variable, at least one
 * of those threads will be woken up. Otherwise, this function has no effects.
 *
 * \note This function is not a cancellation point.
 */
HEIMDALL_API void heimdall_cond_signal(heimdall_cond_t *);

/**
 * Wakes up all threads waiting on a condition variable.
 *
 * \note This function is not a cancellation point.
 */
HEIMDALL_API void heimdall_cond_broadcast(heimdall_cond_t *);

/**
 * Waits on a condition variable.
 *
 * The calling thread will be suspended until another thread calls
 * heimdall_cond_signal() or heimdall_cond_broadcast() on the same condition variable,
 * the thread is cancelled with heimdall_cancel(), or the system causes a
 * <em>spurious</em> unsolicited wake-up.
 *
 * A mutex is needed to wait on a condition variable. It must <b>not</b> be
 * a recursive mutex. Although it is possible to use the same mutex for
 * multiple condition, it is not valid to use different mutexes for the same
 * condition variable at the same time from different threads.
 *
 * The canonical way to use a condition variable to wait for event foobar is:
 @code
   heimdall_mutex_lock(&lock);

   while (!foobar)
       heimdall_cond_wait(&wait, &lock);

   // -- foobar is now true, do something about it here --

   heimdall_mutex_unlock(&lock);
  @endcode
 *
 * \param cond condition variable to wait on
 * \param mutex mutex which is unlocked while waiting,
 *              then locked again when waking up.
 */
HEIMDALL_API void heimdall_cond_wait(heimdall_cond_t *cond, heimdall_mutex_t *mutex);

/**
 * Waits on a condition variable up to a certain date.
 *
 * This works like heimdall_cond_wait() but with an additional time-out.
 * The time-out is expressed as an absolute timestamp using the same arbitrary
 * time reference as the heimdall_tick_now() and heimdall_tick_wait() functions.
 *
 * \param cond condition variable to wait on
 * \param mutex mutex which is unlocked while waiting,
 *              then locked again when waking up
 * \param deadline <b>absolute</b> timeout
 *
 * \return 0 if the condition was signaled, an error code in case of timeout.
 */
HEIMDALL_API int heimdall_cond_timedwait(heimdall_cond_t *cond, heimdall_mutex_t *mutex,
                               heimdall_tick_t deadline);

/** @} */

/**
 * \defgroup semaphore Semaphores
 *
 * The semaphore is the simplest thread synchronization primitive, consisting
 * of a simple counter.
 *
 * See also POSIX @c sem_t .
 *
 * @{
 */
/**
 * Semaphore.
 *
 * Storage space for a thread-safe semaphore.
 */
typedef struct
{
    union {
#ifndef __cplusplus
        atomic_uint value;
#endif
        int dummy;
   };
} heimdall_sem_t;

/**
 * Initializes a semaphore.
 *
 * @param sem a semaphore to initialize
 * @param count initial semaphore value (typically 0)
 */
HEIMDALL_API void heimdall_sem_init(heimdall_sem_t *sem, unsigned count);

/**
 * Increments the value of a semaphore.
 *
 * \note This function is not a cancellation point.
 *
 * \return 0 on success, EOVERFLOW in case of integer overflow.
 */
HEIMDALL_API int heimdall_sem_post(heimdall_sem_t *);

/**
 * Waits on a semaphore.
 *
 * This function atomically waits for the semaphore to become non-zero then
 * decrements it, and returns. If the semaphore is non-zero on entry, it is
 * immediately decremented.
 *
 * \note This function may be a point of cancellation.
 */
HEIMDALL_API void heimdall_sem_wait(heimdall_sem_t *);

/**
 * Tries to decrement a semaphore.
 *
 * This function decrements the semaphore if its value is not zero.
 *
 * \param sem semaphore to decrement
 *
 * \retval 0 the semaphore was decremented
 * \retval EAGAIN the semaphore was zero and could not be decremented
 */
HEIMDALL_API int heimdall_sem_trywait(heimdall_sem_t *sem) HEIMDALL_USED;

/**
 * Waits on a semaphore within a deadline.
 *
 * This function waits for the semaphore just like heimdall_sem_wait(), but only
 * up to a given deadline.
 *
 * \param sem semaphore to wait for
 * \param deadline deadline to wait until
 *
 * \retval 0 the semaphore was decremented
 * \retval ETIMEDOUT the deadline was reached
 */
HEIMDALL_API int heimdall_sem_timedwait(heimdall_sem_t *sem, heimdall_tick_t deadline) HEIMDALL_USED;

/** @} */

#ifndef __cplusplus
/**
 * \defgroup latch Latches
 *
 * The latch is a downward counter used to synchronise threads.
 *
 * @{
 */
/**
 * Latch.
 *
 * Storage space for a thread-safe latch.
 */
typedef struct
{
    atomic_size_t value;
    atomic_uint ready;
} heimdall_latch_t;

/**
 * Initializes a latch.
 *
 * @param latch a latch instance
 * @param value initial latch value (typically 1)
 */
HEIMDALL_API void heimdall_latch_init(heimdall_latch_t *latch, size_t value);

/**
 * Decrements the value of a latch.
 *
 * This function atomically decrements the value of a latch by the given
 * quantity. If the result is zero, then any thread waiting on the latch is
 * woken up.
 *
 * \warning If the result is (arithmetically) strictly negative, the behaviour
 * is undefined.
 *
 * \param latch an initialized latch
 * \param n quantity to subtract from the latch value (typically 1)
 *
 * \note This function is not a cancellation point.
 */
HEIMDALL_API void heimdall_latch_count_down(heimdall_latch_t *latch, size_t n);

/**
 * Decrements the value of a latch and waits on it.
 *
 * This function atomically decrements the value of a latch by the given
 * quantity. Then, if the result of the subtraction is strictly positive,
 * it waits until the value reaches zero.
 *
 * This function is equivalent to the succession of heimdall_latch_count_down()
 * then heimdall_latch_wait(), and is only an optimisation to combine the two.
 *
 * \warning If the result is strictly negative, the behaviour is undefined.
 *
 * \param latch an initialized latch
 * \param n number of times to decrement the value (typically 1)
 *
 * \note This function may be a cancellation point.
 */
HEIMDALL_API void heimdall_latch_count_down_and_wait(heimdall_latch_t *latch, size_t n);

/**
 * Checks if a latch is ready.
 *
 * This function compares the value of a latch with zero.
 *
 * \retval false if the latch value is non-zero
 * \retval true if the latch value equals zero
 */
HEIMDALL_API bool heimdall_latch_is_ready(const heimdall_latch_t *latch) HEIMDALL_USED;

/**
 * Waits on a latch.
 *
 * This function waits until the value of the latch reaches zero.
 *
 * \note This function may be a point of cancellation.
 */
HEIMDALL_API void heimdall_latch_wait(heimdall_latch_t *);

/** @} */

/**
 * One-time initialization.
 *
 * A one-time initialization object must always be initialized assigned to
 * \ref HEIMDALL_STATIC_ONCE before use.
 */
typedef struct
{
    atomic_uint value;
} heimdall_once_t;

/**
 * Static initializer for one-time initialization.
 */
#define HEIMDALL_STATIC_ONCE { 0 }

/**
 * Begins a one-time initialization.
 *
 * This function checks if a one-time initialization has completed:
 * - If this is the first time the function is called for the given one-time
 *   initialization object, it marks the beginning of the initialization and
 *   returns true. heimdall_once_complete() must be called to mark the completion
 *   of the initialisation.
 * - Otherwise, it waits until the initialization completes and returns false.
 * - In particular, if the initialization has already completed, the function
 *   returns false immediately without actually waiting.
 *
 * The specified one-time initialization object must have been initialized
 * with @ref HEIMDALL_STATIC_ONCE, which is a constant expression suitable as a
 * static initializer.
 *
 * \warning If this function is called twice without an intervening call to
 * heimdall_once_complete(), a dead lock will occur.
 *
 * \param once one-time initialisation object
 * \retval false on the first call (for the given object)
 * \retval true on subsequent calls (for the given object)
 */
HEIMDALL_API bool heimdall_once_begin(heimdall_once_t *restrict once);

static inline bool heimdall_once_begin_inline(heimdall_once_t *restrict once)
{
    /* Fast path: check if already initialized */
    if (unlikely(atomic_load_explicit(&once->value, memory_order_acquire) < 3))
        return heimdall_once_begin(once);
    return true;
}
#define heimdall_once_begin(once) heimdall_once_begin_inline(once)

/**
 * Completes a one-time initialization.
 *
 * This function marks the end of an ongoing one-time initialization.
 * If any thread is waiting for the completion of that initialization, its
 * execution will be resumed.
 *
 * \warning The behavior is undefined if the one-time initialization object
 * is uninitialized, if one-time initialization has not started, or
 * if one-time initialization has already completed.
 *
 * \param once one-time initialisation object
 */
HEIMDALL_API void heimdall_once_complete(heimdall_once_t *restrict once);

/**
 * Executes a function one time.
 *
 * The first time this function is called with a given one-time initialization
 * object, it executes the provided callback with the provided data pointer as
 * sole parameter. Any further call with the same object will be a no-op.
 *
 * In the corner case that the first time execution is ongoing in another
 * thread, then the function will wait for completion on the other thread
 * (and then synchronize memory) before it returns.
 * This ensures that, no matter what, the callback has been executed exactly
 * once and its side effects are visible after the function returns.
 *
 * \param once a one-time initialization object
 * \param cb callback to execute (the first time)
 * \param opaque data pointer for the callback
 */
static inline void heimdall_once(heimdall_once_t *restrict once, void (*cb)(void *),
                            void *opaque)
{
    if (unlikely(!heimdall_once_begin(once))) {
        cb(opaque);
        heimdall_once_complete(once);
    }
}
#endif

/**
 * \defgroup threadvar Thread-specific variables
 * @{
 */
/**
 * Allocates a thread-specific variable.
 *
 * @param key where to store the thread-specific variable handle
 * @param destr a destruction callback. It is called whenever a thread exits
 * and the thread-specific variable has a non-NULL value.
 *
 * @return 0 on success, a system error code otherwise.
 * This function can actually fail: on most systems, there is a fixed limit to
 * the number of thread-specific variables in a given process.
 */
HEIMDALL_API int heimdall_threadvar_create(heimdall_threadvar_t *key, void (*destr) (void *));

/**
 * Deallocates a thread-specific variable.
 */
HEIMDALL_API void heimdall_threadvar_delete(heimdall_threadvar_t *);

/**
 * Sets a thread-specific variable.

 * \param key thread-local variable key (created with heimdall_threadvar_create())
 * \param value new value for the variable for the calling thread
 * \return 0 on success, a system error code otherwise.
 */
HEIMDALL_API int heimdall_threadvar_set(heimdall_threadvar_t key, void *value);

/**
 * Gets the value of a thread-local variable for the calling thread.
 * This function cannot fail.
 *
 * \return the value associated with the given variable for the calling
 * or NULL if no value was set.
 */
HEIMDALL_API void *heimdall_threadvar_get(heimdall_threadvar_t);

/** @} */

/**
 * Creates and starts a new thread.
 *
 * The thread must be <i>joined</i> with heimdall_join() to reclaim resources
 * when it is not needed anymore.
 *
 * @param th storage space for the handle of the new thread (cannot be NULL)
 *           [OUT]
 * @param entry entry point for the thread
 * @param data data parameter given to the entry point
 * @return 0 on success, a standard error code on error.
 * @note In case of error, the value of *th is undefined.
 */
HEIMDALL_API int heimdall_clone(heimdall_thread_t *th, void *(*entry)(void *),
                      void *data) HEIMDALL_USED;

#if defined(__GNUC__) && !defined(__COVERITY__)
static
HEIMDALL_UNUSED_FUNC
HEIMDALL_WARN_CALL("thread name too big")
const char * heimdall_thread_name_too_big( const char * thread_name )
{
    return thread_name;
}

# define check_name_length( thread_name ) \
    ((__builtin_constant_p(__builtin_strlen(thread_name) > 15) && \
      __builtin_strlen(thread_name) > 15) \
      ? heimdall_thread_name_too_big(thread_name): thread_name)
#endif

/**
 * Set the thread name of the current thread.
 *
 * \param name the string to use as the thread name
 *
 * \note On Linux the name can be up to 16-byte long, including the terminating
 *       nul character. If larger, the name will be truncated.
 */
HEIMDALL_API void heimdall_thread_set_name(const char *name);
#if defined(check_name_length)
# define heimdall_thread_set_name(name)  heimdall_thread_set_name(check_name_length(name))
#endif

/**
 * Marks a thread as cancelled.
 *
 * Next time the target thread reaches a cancellation point (while not having
 * disabled cancellation), it will run its cancellation cleanup handler, the
 * thread variable destructors, and terminate.
 *
 * heimdall_join() must be used regardless of a thread being cancelled or not, to
 * avoid leaking resources.
 */
HEIMDALL_API void heimdall_cancel(heimdall_thread_t);

/**
 * Waits for a thread to complete (if needed), then destroys it.
 *
 * \note This is a cancellation point. In case of cancellation, the thread is
 * <b>not</b> joined.

 * \warning A thread cannot join itself (normally Heimdall will abort if this is
 * attempted).
 *
 * @param th thread handle
 * @param result [OUT] pointer to write the thread return value or NULL
 */
HEIMDALL_API void heimdall_join(heimdall_thread_t th, void **result);

/**
 * Disables thread cancellation.
 *
 * This functions saves the current cancellation state (enabled or disabled),
 * then disables cancellation for the calling thread. It must be called before
 * entering a piece of code that is not cancellation-safe, unless it can be
 * proven that the calling thread will not be cancelled.
 *
 * \note This function is not a cancellation point.
 *
 * \return Previous cancellation state (opaque value for heimdall_restorecancel()).
 */
HEIMDALL_API int heimdall_savecancel(void);

/**
 * Restores the cancellation state.
 *
 * This function restores the cancellation state of the calling thread to
 * a state previously saved by heimdall_savecancel().
 *
 * \note This function is not a cancellation point.
 *
 * \param state previous state as returned by heimdall_savecancel().
 */
HEIMDALL_API void heimdall_restorecancel(int state);

typedef struct heimdall_cleanup_t heimdall_cleanup_t;

/**
 * Internal handler for thread cancellation.
 *
 * Do not call this function directly. Use wrapper macros instead:
 * heimdall_cleanup_push(), heimdall_cleanup_pop().
 */
HEIMDALL_API void heimdall_control_cancel(heimdall_cleanup_t *);

/**
 * Thread identifier.
 *
 * This function returns a non-zero unique identifier of the calling thread.
 * The identifier cannot change for the entire lifetime of the thread, and two
 * concurrent threads cannot have the same identifier.
 *
 * The thread identifier has no defined semantics other than uniqueness,
 * and no particular purposes within LibHeimdall.
 * It is provided mainly for tracing and debugging.
 *
 * On some but not all supported platforms, the thread identifier is in fact
 * the OS/kernel thread identifier (a.k.a. task PID), and is temporally unique
 * not only within the process but across the entire system.
 *
 * \note
 * The `main()` thread identifier is typically identical to the process
 * identifier, but this is not portable.
 *
 * \return the thread identifier (cannot fail)
 */
HEIMDALL_API unsigned long heimdall_thread_id(void) HEIMDALL_USED;

/**
 * Precision monotonic clock.
 *
 * In principles, the clock has a precision of 1 MHz. But the actual resolution
 * may be much lower, especially when it comes to sleeping with heimdall_tick_wait() or
 * heimdall_tick_sleep(). Most general-purpose operating systems provide a resolution of
 * only 100 to 1000 Hz.
 *
 * \warning The origin date (time value "zero") is not specified. It is
 * typically the time the kernel started, but this is platform-dependent.
 * If you need wall clock time, use the standard timespec_get() instead.
 *
 * \return a timestamp in microseconds.
 */
HEIMDALL_API heimdall_tick_t heimdall_tick_now(void);

/**
 * Waits until a deadline.
 *
 * \param deadline timestamp to wait for (\ref heimdall_tick_now())
 *
 * \note The deadline may be exceeded due to OS scheduling.
 * \note This function is a cancellation point.
 */
HEIMDALL_API void heimdall_tick_wait(heimdall_tick_t deadline);

/**
 * Waits for an interval of time.
 *
 * \param delay how long to wait (in microseconds)
 *
 * \note The delay may be exceeded due to OS scheduling.
 * \note This function is a cancellation point.
 */
HEIMDALL_API void heimdall_tick_sleep(heimdall_tick_t delay);

#define HEIMDALL_HARD_MIN_SLEEP  HEIMDALL_TICK_FROM_MS(10)   /* 10 milliseconds = 1 tick at 100Hz */
#define HEIMDALL_SOFT_MIN_SLEEP  HEIMDALL_TICK_FROM_SEC(9)   /* 9 seconds */

#if defined(__GNUC__) && !defined(__COVERITY__)

/* Linux has 100, 250, 300 or 1000Hz
 *
 * HZ=100 by default on FreeBSD, but some architectures use a 1000Hz timer
 */

static
HEIMDALL_UNUSED_FUNC
HEIMDALL_ERROR_CALL("sorry, cannot sleep for such short a time")
heimdall_tick_t impossible_delay( heimdall_tick_t delay )
{
    (void) delay;
    return HEIMDALL_HARD_MIN_SLEEP;
}

static
HEIMDALL_UNUSED_FUNC
HEIMDALL_WARN_CALL("use proper event handling instead of short delay")
heimdall_tick_t harmful_delay( heimdall_tick_t delay )
{
    return delay;
}

# define check_delay( d ) \
    ((__builtin_constant_p(d < HEIMDALL_HARD_MIN_SLEEP) \
   && (d < HEIMDALL_HARD_MIN_SLEEP)) \
       ? impossible_delay(d) \
       : ((__builtin_constant_p(d < HEIMDALL_SOFT_MIN_SLEEP) \
       && (d < HEIMDALL_SOFT_MIN_SLEEP)) \
           ? harmful_delay(d) \
           : d))

static
HEIMDALL_UNUSED_FUNC
HEIMDALL_ERROR_CALL("deadlines can not be constant")
heimdall_tick_t impossible_deadline( heimdall_tick_t deadline )
{
    return deadline;
}

# define check_deadline( d ) \
    (__builtin_constant_p(d) ? impossible_deadline(d) : d)
#endif

#if defined(check_delay)
#define heimdall_tick_sleep(d) heimdall_tick_sleep(check_delay(d))
#endif
#if defined(check_deadline)
#define heimdall_tick_wait(d) heimdall_tick_wait(check_deadline(d))
#endif

/**
 * \defgroup timer Asynchronous/threaded timers
 * @{
 */
/**
 * Initializes an asynchronous timer.
 *
 * \param id pointer to timer to be initialized
 * \param func function that the timer will call
 * \param data parameter for the timer function
 * \return 0 on success, a system error code otherwise.
 *
 * \warning Asynchronous timers are processed from an unspecified thread.
 * \note Multiple occurrences of a single interval timer are serialized:
 * they cannot run concurrently.
 */
HEIMDALL_API int heimdall_timer_create(heimdall_timer_t *id, void (*func)(void *), void *data)
HEIMDALL_USED;

/**
 * Destroys an initialized timer.
 *
 * If needed, the timer is first disarmed. Behaviour is undefined if the
 * specified timer is not initialized.
 *
 * \warning This function <b>must</b> be called before the timer data can be
 * freed and before the timer callback function can be unmapped/unloaded.
 *
 * \param timer timer to destroy
 */
HEIMDALL_API void heimdall_timer_destroy(heimdall_timer_t timer);

#define HEIMDALL_TIMER_DISARM    (0)
#define HEIMDALL_TIMER_FIRE_ONCE (0)

/**
 * Arms or disarms an initialized timer.
 *
 * This functions overrides any previous call to itself.
 *
 * \note A timer can fire later than requested due to system scheduling
 * limitations. An interval timer can fail to trigger sometimes, either because
 * the system is busy or suspended, or because a previous iteration of the
 * timer is still running. See also heimdall_timer_getoverrun().
 *
 * \param timer initialized timer
 * \param absolute the timer value origin is the same as heimdall_tick_now() if true,
 *                 the timer value is relative to now if false.
 * \param value zero to disarm the timer, otherwise the initial time to wait
 *              before firing the timer.
 * \param interval zero to fire the timer just once, otherwise the timer
 *                 repetition interval.
 */
HEIMDALL_API void heimdall_timer_schedule(heimdall_timer_t timer, bool absolute,
                                heimdall_tick_t value, heimdall_tick_t interval);

static inline void heimdall_timer_disarm(heimdall_timer_t timer)
{
    heimdall_timer_schedule( timer, false, HEIMDALL_TIMER_DISARM, 0 );
}

static inline void heimdall_timer_schedule_asap(heimdall_timer_t timer, heimdall_tick_t interval)
{
    heimdall_timer_schedule(timer, false, 1, interval);
}

/**
 * Fetches and resets the overrun counter for a timer.
 *
 * This functions returns the number of times that the interval timer should
 * have fired, but the callback was not invoked due to scheduling problems.
 * The call resets the counter to zero.
 *
 * \param timer initialized timer
 * \return the timer overrun counter (typically zero)
 */
HEIMDALL_API unsigned heimdall_timer_getoverrun(heimdall_timer_t timer) HEIMDALL_USED;

/** @} */

/**
 * Count CPUs.
 *
 * \return number of available (logical) CPUs.
 */
HEIMDALL_API unsigned heimdall_GetCPUCount(void);

#if defined (LIBHEIMDALL_USE_PTHREAD_CLEANUP)
/**
 * Registers a thread cancellation handler.
 *
 * This pushes a function to run if the thread is cancelled (or otherwise
 * exits prematurely).
 *
 * If multiple procedures are registered,
 * they are handled in last-in first-out order.
 *
 * \note Any call to heimdall_cleanup_push() <b>must</b> paired with a call to
 * heimdall_cleanup_pop().
 * \warning Branching into or out of the block between these two function calls
 * is not allowed (read: it will likely crash the whole process).
 *
 * \param routine procedure to call if the thread ends
 * \param arg argument for the procedure
 */
# define heimdall_cleanup_push( routine, arg ) pthread_cleanup_push (routine, arg)

/**
 * Unregisters the last cancellation handler.
 *
 * This pops the cancellation handler that was last pushed with
 * heimdall_cleanup_push() in the calling thread.
 */
# define heimdall_cleanup_pop( ) pthread_cleanup_pop (0)

#else /* !LIBHEIMDALL_USE_PTHREAD_CLEANUP */
struct heimdall_cleanup_t
{
    heimdall_cleanup_t *next;
    void         (*proc) (void *);
    void          *data;
};

# ifndef __cplusplus
/* This macros opens a code block on purpose: It reduces the chance of
 * not pairing the push and pop. It also matches the POSIX Thread internals.
 * That way, Win32 developers will not accidentally break other platforms.
 */
# define heimdall_cleanup_push( routine, arg ) \
    do { \
        heimdall_control_cancel(&(heimdall_cleanup_t){ NULL, routine, arg })

#  define heimdall_cleanup_pop( ) \
        heimdall_control_cancel (NULL); \
    } while (0)
# else
#  define heimdall_cleanup_push(routine, arg) \
    static_assert(false, "don't use heimdall_cleanup_push in portable C++ code")
#  define heimdall_cleanup_pop() \
    static_assert(false, "don't use heimdall_cleanup_pop in portable C++ code")
# endif

#endif /* !LIBHEIMDALL_USE_PTHREAD_CLEANUP */

#ifdef __cplusplus
/**
 * Helper C++ class to lock a mutex.
 *
 * The mutex is locked when the object is created, and unlocked when the object
 * is destroyed.
 */
class heimdall_mutex_locker
{
    private:
        heimdall_mutex_t *lock;
    public:
        heimdall_mutex_locker (heimdall_mutex_t *m) : lock (m)
        {
            heimdall_mutex_lock (lock);
        }

        ~heimdall_mutex_locker (void)
        {
            heimdall_mutex_unlock (lock);
        }
};

#endif

enum
{
   HEIMDALL_AVCODEC_MUTEX = 0,
   HEIMDALL_GCRYPT_MUTEX,
   HEIMDALL_XLIB_MUTEX,
   HEIMDALL_MOSAIC_MUTEX,
#ifdef _WIN32
   HEIMDALL_MTA_MUTEX,
#endif
   /* Insert new entry HERE */
   HEIMDALL_MAX_MUTEX
};

/**
 * Internal handler for global mutexes.
 *
 * Do not use this function directly. Use helper macros instead:
 * heimdall_global_lock(), heimdall_global_unlock().
 */
HEIMDALL_API void heimdall_global_mutex(unsigned, bool);

/**
 * Acquires a global mutex.
 */
#define heimdall_global_lock( n ) heimdall_global_mutex(n, true)

/**
 * Releases a global mutex.
 */
#define heimdall_global_unlock( n ) heimdall_global_mutex(n, false)

/** @} */

#endif /* !_HEIMDALL_THREADS_H */
