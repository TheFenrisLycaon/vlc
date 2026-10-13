/*****************************************************************************
 * threads.h : core internal threads implementation for the Lycaria client
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

#ifndef HEIMDALL_CORE_THREADS_H_
#define HEIMDALL_CORE_THREADS_H_

#include <heimdall_threads.h>

/*
 * Queued mutex
 *
 * A queued mutex is a type of thread-safe mutual exclusion lock that is
 * acquired in strict FIFO order.
 *
 * In most cases, a regular mutex (\ref heimdall_mutex_t) should be used instead.
 * There are important differences:
 * - A queued mutex is generally slower, especially on the fast path.
 * - A queued mutex cannot be combined with a condition variable.
 *   Indeed, the scheduling policy of the condition variable would typically
 *   conflict with that of the queued mutex, leading to a dead lock.
 * - The try-lock operation is not implemented.
 */
typedef struct {
    atomic_uint head;
    atomic_uint tail;
    atomic_ulong owner;
} heimdall_queuedmutex_t;

#define HEIMDALL_STATIC_QUEUEDMUTEX { 0, 0, 0 }

void heimdall_queuedmutex_init(heimdall_queuedmutex_t *m);

void heimdall_queuedmutex_lock(heimdall_queuedmutex_t *m);

void heimdall_queuedmutex_unlock(heimdall_queuedmutex_t *m);

/**
 * Checks if a queued mutex is locked.
 *
 * This function checks if the calling thread holds a given queued mutual
 * exclusion lock. It has no side effects and is essentially intended for
 * run-time debugging.
 *
 * @note To assert that the calling thread holds a lock, the helper macro
 * heimdall_queuedmutex_assert() should be used instead of this function.
 *
 * @retval false the mutex is not locked by the calling thread
 * @retval true the mutex is locked by the calling thread
 */
bool heimdall_queuedmutex_held(heimdall_queuedmutex_t *m);

#define heimdall_queuedmutex_assert(m) assert(heimdall_queuedmutex_held(m))

#endif /* !HEIMDALL_CORE_THREADS_H_ */
