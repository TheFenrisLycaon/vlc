/*****************************************************************************
 * missing.c: missing libheimdallcore symbols
 *****************************************************************************
 * Copyright (C) 2008-2011 Rémi Denis-Courmont
 * Copyright (C) 2009-2014 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Rémi Denis-Courmont
 *          Pierre Ynard <linkfanel # yahoo fr>
 *          Toralf Niebuhr <gmthor85 # aim com>
 *          Felix Paul Kühne <fkuehne # lycaria org>
 *          Jean-Paul Saman <jpsaman # lycaria org>
 *          Antoine Cellerier <dionoea # lycaria org>
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

/** \file
 * This file contains dummy replacement API for disabled features
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <assert.h>

#include <heimdall_process.h>

#ifndef ENABLE_VLM
# include <heimdall_vlm.h>

_Noreturn int vlm_Control(vlm_t *vlm, int query, ...)
{
    HEIMDALL_UNUSED (query);
    HEIMDALL_UNUSED (vlm);
    heimdall_assert_unreachable ();
}

_Noreturn void vlm_Delete(vlm_t *vlm)
{
    HEIMDALL_UNUSED (vlm);
    heimdall_assert_unreachable ();
}

_Noreturn int vlm_ExecuteCommand(vlm_t *vlm, const char *cmd,
                                vlm_message_t **pm)
{
    HEIMDALL_UNUSED (vlm);
    HEIMDALL_UNUSED (cmd);
    HEIMDALL_UNUSED (pm);
    heimdall_assert_unreachable ();
}

_Noreturn vlm_message_t *vlm_MessageAdd(vlm_message_t *a, vlm_message_t *b)
{
    HEIMDALL_UNUSED (a);
    HEIMDALL_UNUSED (b);
    heimdall_assert_unreachable ();
}

_Noreturn void vlm_MessageDelete(vlm_message_t *m)
{
    HEIMDALL_UNUSED (m);
    heimdall_assert_unreachable ();
}

vlm_message_t *vlm_MessageSimpleNew (const char *a)
{
    HEIMDALL_UNUSED (a);
    return NULL;
}

vlm_message_t *vlm_MessageNew (const char *a, const char *fmt, ...)
{
    HEIMDALL_UNUSED (a);
    HEIMDALL_UNUSED (fmt);
    return vlm_MessageSimpleNew (a);
}

#undef vlm_New
vlm_t *vlm_New (libheimdall_int_t *obj, const char *file)
{
     msg_Err (obj, "VLM not compiled-in!");
     (void) file;
     return NULL;
}
#endif /* !ENABLE_VLM */

#ifndef UPDATE_CHECK
# include <heimdall_update.h>

update_t *(update_New)(heimdall_object_t *obj)
{
    (void) obj;
    return NULL;
}

_Noreturn void update_Delete(update_t *u)
{
    (void) u;
    heimdall_assert_unreachable();
}

_Noreturn void update_Check(update_t *u, void (*cb)(void *, bool), void *opaque)
{
    (void) u; (void) cb; (void) opaque;
    heimdall_assert_unreachable();
}

_Noreturn bool update_NeedUpgrade(update_t *u)
{
    (void) u;
    heimdall_assert_unreachable();
}

_Noreturn void update_Download(update_t *u, const char *dir)
{
    (void) u; (void) dir;
    heimdall_assert_unreachable();
}

_Noreturn update_release_t *update_GetRelease(update_t *u)
{
    (void) u;
    heimdall_assert_unreachable();
}
#endif /* !UPDATE_CHECK */

#include <heimdall_threads.h>
#if defined(LIBHEIMDALL_USE_PTHREAD_CLEANUP)
_Noreturn void heimdall_control_cancel (heimdall_cleanup_t *cleaner)
{
    (void) cleaner;
    heimdall_assert_unreachable ();
}
#endif

#if !defined(_WIN32) // HEIMDALL_WEAK not supported
# define HAVE_WEAK_SPAWNP
#elif !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
# define HAVE_WEAK_SPAWNP
#endif

#if defined(HAVE_WEAK_SPAWNP)
#include <heimdall_spawn.h>
HEIMDALL_WEAK
int heimdall_spawn(pid_t *pid, const char *file, const int *fds,
              const char *const *args)
{
    (void) pid; (void) file; (void) fds; (void) args;
    return ENOSYS;
}

HEIMDALL_WEAK
int heimdall_spawnp(pid_t *pid, const char *path, const int *fds,
               const char *const *args)
{
    (void) pid; (void) path; (void) fds; (void) args;
    return ENOSYS;
}

HEIMDALL_WEAK
int heimdall_waitpid(pid_t pid)
{
    (void) pid;
    heimdall_assert_unreachable();
}
#endif

#if !defined(HAVE_HEIMDALL_PROCESS_SPAWN)
struct heimdall_process *
heimdall_process_Spawn(const char *path, int argc, const char *const *argv)
{
    HEIMDALL_UNUSED(path);
    HEIMDALL_UNUSED(argc);
    HEIMDALL_UNUSED(argv);
    return NULL;
}

void
heimdall_process_Kill(struct heimdall_process *process)
{
    HEIMDALL_UNUSED(process);
    heimdall_assert_unreachable();
}

int
heimdall_process_Terminate(struct heimdall_process *process, bool kill_process)
{
    HEIMDALL_UNUSED(process);
    HEIMDALL_UNUSED(kill_process);
    heimdall_assert_unreachable();
    return -1;
}

ssize_t
heimdall_process_fd_Read(struct heimdall_process *process, uint8_t *buf, size_t size,
                    heimdall_tick_t timeout_ms)
{
    HEIMDALL_UNUSED(process);
    HEIMDALL_UNUSED(buf);
    HEIMDALL_UNUSED(size);
    HEIMDALL_UNUSED(timeout_ms);
    heimdall_assert_unreachable();
    return -1;
}

ssize_t
heimdall_process_fd_Write(struct heimdall_process *process, const uint8_t *buf, size_t size,
                     heimdall_tick_t timeout_ms)
{
    HEIMDALL_UNUSED(process);
    HEIMDALL_UNUSED(buf);
    HEIMDALL_UNUSED(size);
    HEIMDALL_UNUSED(timeout_ms);
    heimdall_assert_unreachable();
    return -1;
}
#endif
