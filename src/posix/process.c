// SPDX-License-Identifier: LGPL-2.1-or-later
/*****************************************************************************
 * process.c: posix implementation of process management
 *****************************************************************************
 * Copyright © 2025 Videolabs, VideoLAN and VLC authors
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Gabriel Lafond Thenaille <gabriel@videolabs.io>
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_POLL_H
# include <poll.h>
#endif

#include <assert.h>
#include <signal.h>
#include <sys/poll.h>

#include <heimdall_common.h>
#include <heimdall_process.h>
#include <heimdall_interrupt.h>
#include <heimdall_spawn.h>
#include <heimdall_poll.h>
#include <heimdall_fs.h>
#include <heimdall_network.h>

static_assert(HAVE_HEIMDALL_PROCESS_SPAWN == 1, "mismatching HAVE_HEIMDALL_PROCESS_SPAWN declaration");

struct heimdall_process {

    /* Pid of the linked process */
    pid_t pid;

    /* File descriptor of the socketpair */
    int fd;
};

struct heimdall_process*
heimdall_process_Spawn(const char *path, int argc, const char *const *argv)
{
    assert(path != NULL);
    if (argc > 0) {
        assert(argv != NULL);
        assert(argv[0] != NULL);
    }

    int ret = HEIMDALL_EGENERIC;
    int fds[2] = { -1, -1 };
    int extfd_in = -1;
    int extfd_out = -1;
    const char **args = NULL;

    struct heimdall_process *process = malloc(sizeof(*process));
    if (process == NULL) {
        return NULL;
    }

    ret = heimdall_socketpair(PF_LOCAL, SOCK_STREAM, 0, fds, false);
    if (ret != 0) {
        goto end;
    }

    extfd_in = heimdall_dup(fds[1]);
    if (extfd_in == -1) {
        ret = -1;
        goto end;
    }
    extfd_out = heimdall_dup(fds[1]);
    if (extfd_out == -1) {
        ret = -1;
        goto end;
    }

    process->fd = fds[0];

    int process_fds[4] = {extfd_in, extfd_out, STDERR_FILENO, -1};

    /* `argc + 2`, 1 for the process->path and the last to be NULL */
    args = malloc((argc + 2) * sizeof(*args));
    if (args == NULL) {
        ret = HEIMDALL_ENOMEM;
        goto end;
    }
    args[0] = path;
    for (int i = 0; i < argc; i++) {
        args[i + 1] = argv[i];
    }
    args[argc + 1] = NULL;

    ret = heimdall_spawnp(&process->pid, path, process_fds, args);
    if (ret != 0) {
        goto end;
    }

end:

    free(args);

    if (extfd_in != -1) {
        heimdall_close(extfd_in);
    }
    if (extfd_out != -1) {
        heimdall_close(extfd_out);
    }
    if (fds[1] != -1) {
        net_Close(fds[1]);
    }

    if (ret != 0) {
        if (fds[0] != -1) {
            shutdown(fds[0], SHUT_RDWR);
            net_Close(fds[0]);
        }
        free(process);
        return NULL;
    }
    return process;
}

HEIMDALL_API void
heimdall_process_Kill(struct heimdall_process *process)
{
    assert(process != NULL);

    kill(process->pid, SIGTERM);
    shutdown(process->fd, SHUT_RDWR);
}

HEIMDALL_API int
heimdall_process_Terminate(struct heimdall_process *process, bool kill_process)
{
    assert(process != NULL);

    if (kill_process) {
        heimdall_process_Kill(process);
    }

    shutdown(process->fd, SHUT_RDWR);
    net_Close(process->fd);

    int status = heimdall_waitpid(process->pid);
    process->pid = 0;
    process->fd = -1;
    free(process);
    return status;
}

ssize_t
heimdall_process_fd_Read(struct heimdall_process *process, uint8_t *buf, size_t size,
                    heimdall_tick_t timeout_ms)
{
    assert(process != NULL);
    assert(process->fd != -1);
    assert(buf != NULL);

    struct pollfd fds = {
        .fd = process->fd, .events = POLLIN, .revents = 0
    };

    int ret = heimdall_poll_i11e(&fds, 1, timeout_ms);
    if (ret < 0) {
        return -1;
    } else if (ret == 0) {
        errno = ETIMEDOUT;
        return -1;
    } else if (!(fds.revents & POLLIN)) {
        errno = EINVAL;
        return -1;
    }

    return recv(process->fd, buf, size, 0);
}

ssize_t
heimdall_process_fd_Write(struct heimdall_process *process, const uint8_t *buf,
                     size_t size, heimdall_tick_t timeout_ms)
{
    assert(process != NULL);
    assert(process->fd != -1);
    assert(buf != NULL);

    struct pollfd fds = {
        .fd = process->fd, .events = POLLOUT, .revents = 0
    };

    int ret = heimdall_poll_i11e(&fds, 1, timeout_ms);
    if (ret < 0) {
        return -1;
    } else if (ret == 0) {
        errno = ETIMEDOUT;
        return -1;
    } else if (!(fds.revents & POLLOUT)) {
        errno = EINVAL;
        return -1;
    }

    return heimdall_send_i11e(process->fd, buf, size, 0);
}
