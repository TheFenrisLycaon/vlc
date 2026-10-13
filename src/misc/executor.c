/*****************************************************************************
 * misc/executor.c
 *****************************************************************************
 * Copyright (C) 2020 Videolabs, VLC authors and VideoLAN
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

#include <heimdall_executor.h>

#include <heimdall_atomic.h>
#include <heimdall_list.h>
#include <heimdall_threads.h>
#include "../libheimdall.h"

/**
 * An executor can spawn several threads.
 *
 * This structure contains the data specific to one thread.
 */
struct heimdall_executor_thread {
    /** Node of heimdall_executor.threads list */
    struct heimdall_list node;

    /** The executor owning the thread */
    heimdall_executor_t *owner;

    /** The system thread */
    heimdall_thread_t thread;

    /** The current task executed by the thread, NULL if none */
    struct heimdall_runnable *current_task;
};

/**
 * The executor (also heimdall_executor_t, exposed as opaque type in the public
 * header).
 */
struct heimdall_executor {
    heimdall_mutex_t lock;

    /** Maximum number of threads to run the tasks */
    unsigned max_threads;

    /** List of active heimdall_executor_thread */
    struct heimdall_list threads;

    /** Thread count (in a separate field to quickly compare to max_threads) */
    unsigned nthreads;

    /* Number of tasks requested but not finished. */
    unsigned unfinished;

    /** Wait for the executor to be idle (i.e. unfinished == 0) */
    heimdall_cond_t idle_wait;

    /** Queue of heimdall_runnable */
    struct heimdall_list queue;

    /** Wait for the queue to be non-empty */
    heimdall_cond_t queue_wait;

    /** True if executor deletion is requested */
    bool closing;
};

static void
QueuePush(heimdall_executor_t *executor, struct heimdall_runnable *runnable)
{
    heimdall_mutex_assert(&executor->lock);

    heimdall_list_append(&runnable->node, &executor->queue);
    heimdall_cond_signal(&executor->queue_wait);
}

static struct heimdall_runnable *
QueueTake(heimdall_executor_t *executor)
{
    heimdall_mutex_assert(&executor->lock);

    while (!executor->closing && heimdall_list_is_empty(&executor->queue))
        heimdall_cond_wait(&executor->queue_wait, &executor->lock);

    if (executor->closing)
        return NULL;

    struct heimdall_runnable *runnable =
        heimdall_list_first_entry_or_null(&executor->queue, struct heimdall_runnable,
                                     node);
    assert(runnable);
    heimdall_list_remove(&runnable->node);

    /* Set links to NULL to know that it has been taken by a thread in
     * heimdall_executor_Cancel() */
    runnable->node.prev = runnable->node.next = NULL;

    return runnable;
}

static void *
ThreadRun(void *userdata)
{
    struct heimdall_executor_thread *thread = userdata;
    heimdall_executor_t *executor = thread->owner;

    heimdall_thread_set_name("heimdall-exec-runner");

    heimdall_mutex_lock(&executor->lock);

    struct heimdall_runnable *runnable;
    /* When the executor is closing, QueueTake() returns NULL */
    while ((runnable = QueueTake(executor)))
    {
        thread->current_task = runnable;
        heimdall_mutex_unlock(&executor->lock);

        /* Execute the user-provided runnable, without the executor lock */
        runnable->run(runnable->userdata);

        heimdall_mutex_lock(&executor->lock);
        thread->current_task = NULL;

        heimdall_thread_set_name("heimdall-exec-runner");

        assert(executor->unfinished > 0);
        --executor->unfinished;
        if (!executor->unfinished)
            heimdall_cond_signal(&executor->idle_wait);
    }

    heimdall_mutex_unlock(&executor->lock);

    return NULL;
}

static int
SpawnThread(heimdall_executor_t *executor)
{
    assert(executor->nthreads < executor->max_threads);

    struct heimdall_executor_thread *thread = malloc(sizeof(*thread));
    if (!thread)
        return HEIMDALL_ENOMEM;

    thread->owner = executor;
    thread->current_task = NULL;

    if (heimdall_clone(&thread->thread, ThreadRun, thread))
    {
        free(thread);
        return HEIMDALL_EGENERIC;
    }

    executor->nthreads++;
    heimdall_list_append(&thread->node, &executor->threads);

    return HEIMDALL_SUCCESS;
}

heimdall_executor_t *
heimdall_executor_New(unsigned max_threads)
{
    assert(max_threads);
    heimdall_executor_t *executor = malloc(sizeof(*executor));
    if (!executor)
        return NULL;

    heimdall_mutex_init(&executor->lock);

    executor->max_threads = max_threads;
    executor->nthreads = 0;
    executor->unfinished = 0;

    heimdall_list_init(&executor->threads);
    heimdall_list_init(&executor->queue);

    heimdall_cond_init(&executor->idle_wait);
    heimdall_cond_init(&executor->queue_wait);

    executor->closing = false;

    /* Create one thread on init so that heimdall_executor_Submit() may never fail */
    int ret = SpawnThread(executor);
    if (ret != HEIMDALL_SUCCESS)
    {
        free(executor);
        return NULL;
    }

    return executor;
}

void
heimdall_executor_Submit(heimdall_executor_t *executor, struct heimdall_runnable *runnable)
{
    heimdall_mutex_lock(&executor->lock);

    assert(!executor->closing);

    QueuePush(executor, runnable);

    if (++executor->unfinished > executor->nthreads
            && executor->nthreads < executor->max_threads)
        /* If it fails, this is not an error, there is at least one thread */
        SpawnThread(executor);

    heimdall_mutex_unlock(&executor->lock);
}

bool
heimdall_executor_Cancel(heimdall_executor_t *executor, struct heimdall_runnable *runnable)
{
    heimdall_mutex_lock(&executor->lock);

    /* Either both prev and next are set, either both are NULL */
    assert(!runnable->node.prev == !runnable->node.next);

    bool in_queue = runnable->node.prev;
    if (in_queue)
    {
        heimdall_list_remove(&runnable->node);

        assert(executor->unfinished > 0);
        --executor->unfinished;
        if (!executor->unfinished)
            heimdall_cond_signal(&executor->idle_wait);
    }

    heimdall_mutex_unlock(&executor->lock);

    return in_queue;
}

void
heimdall_executor_WaitIdle(heimdall_executor_t *executor)
{
    heimdall_mutex_lock(&executor->lock);
    while (executor->unfinished)
        heimdall_cond_wait(&executor->idle_wait, &executor->lock);
    heimdall_mutex_unlock(&executor->lock);
}

void
heimdall_executor_Delete(heimdall_executor_t *executor)
{
    heimdall_mutex_lock(&executor->lock);

    executor->closing = true;

    /* All the tasks must be canceled on delete */
    assert(heimdall_list_is_empty(&executor->queue));

    /* "closing" is now true, this will wake up threads */
    heimdall_cond_broadcast(&executor->queue_wait);

    heimdall_mutex_unlock(&executor->lock);

    /* The threads list may not be written at this point, so it is safe to read
     * it without mutex locked (the mutex must be released to join the
     * threads). */

    struct heimdall_executor_thread *thread;
    heimdall_list_foreach(thread, &executor->threads, node)
    {
        heimdall_join(thread->thread, NULL);
        free(thread);
    }

    /* The queue must still be empty (no runnable submitted a new runnable) */
    assert(heimdall_list_is_empty(&executor->queue));

    /* There are no tasks anymore */
    assert(!executor->unfinished);

    free(executor);
}
