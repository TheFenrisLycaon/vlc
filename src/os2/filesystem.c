/*****************************************************************************
 * filesystem.c: OS/2 file system helpers
 *****************************************************************************
 * Copyright (C) 2005-2006 VLC authors and VideoLAN
 * Copyright © 2005-2008 Rémi Denis-Courmont
 * Copyright (C) 2012 KO Myung-Hun
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Rémi Denis-Courmont
 *          KO Myung-Hun <komh@chollian.net>
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

#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#include <unistd.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <signal.h>

#include <heimdall_common.h>
#include <heimdall_charset.h>
#include <heimdall_fs.h>
#include <heimdall_network.h>
#include "../libheimdall.h" /* heimdall_mkdir */

int heimdall_open (const char *filename, int flags, ...)
{
    unsigned int mode = 0;
    va_list ap;

    va_start (ap, flags);
    if (flags & O_CREAT)
        mode = va_arg (ap, unsigned int);
    va_end (ap);

    const char *local_name = ToLocaleDup (filename);

    if (local_name == NULL)
    {
        errno = ENOENT;
        return -1;
    }

    int fd = open (local_name, flags, mode);
    if (fd != -1)
        fcntl (fd, F_SETFD, FD_CLOEXEC);

    free (local_name);
    return fd;
}

int heimdall_openat (int dir, const char *filename, int flags, ...)
{
    errno = ENOSYS;

    return -1;
}

int heimdall_memfd (void)
{
    errno = ENOSYS;
    return -1;
}

int heimdall_close (int fd)
{
    return close (fd);
}

int heimdall_mkdir (const char *dirname, mode_t mode)
{
    char *locname = ToLocaleDup (dirname);
    if (unlikely(locname == NULL))
    {
        errno = ENOENT;
        return -1;
    }

    int res = mkdir (locname, mode);
    free (locname);
    return res;
}

DIR *heimdall_opendir (const char *dirname)
{
    const char *locname = ToLocaleDup (dirname);
    if (unlikely(locname == NULL))
    {
        errno = ENOENT;
        return NULL;
    }

    DIR *dir = opendir (locname);

    free (locname);

    return dir;
}

void heimdall_closedir(DIR *dir)
{
    closedir(dir);
}

const char *heimdall_readdir(DIR *dir)
{
    /* Beware that readdir_r() assumes <buf> is large enough to hold the result
     * dirent including the file name. A buffer overflow could occur otherwise.
     * In particular, pathconf() and _POSIX_NAME_MAX cannot be used here. */
    struct dirent *ent;
    char *path = NULL;

    /* In the implementation of Innotek LIBC, aka kLIBC on OS/2,
     * fpathconf (_PC_NAME_MAX) is broken, and errno is set to EBADF.
     * Moreover, d_name is not the last member of struct dirent.
     * So just allocate as many as the size of struct dirent. */
#if 1
    long len = sizeof (struct dirent);
#else
    long len = fpathconf (dirfd (dir), _PC_NAME_MAX);
    len += offsetof (struct dirent, d_name) + 1;
#endif

    struct dirent *buf = malloc (len);
    if (unlikely(buf == NULL))
        return NULL;

    int val = readdir_r (dir, buf, &ent);
    if (val != 0)
        errno = val;
    else if (ent != NULL)
        path = FromCharset ("", ent->d_name, strlen(ent->d_name));
    free (buf);
    return path;
}

void heimdall_rewinddir(DIR *dir)
{
    rewinddir(dir);
}

static int heimdall_statEx (const char *filename, struct stat *buf, bool deref)
{
    const char *local_name = ToLocaleDup (filename);
    if (unlikely(local_name == NULL))
    {
        errno = ENOENT;
        return -1;
    }

    int res = deref ? stat (local_name, buf)
                    : lstat (local_name, buf);
    free (local_name);
    return res;
}

int heimdall_stat (const char *filename, struct stat *buf)
{
    return heimdall_statEx (filename, buf, true);
}

int heimdall_lstat (const char *filename, struct stat *buf)
{
    return heimdall_statEx (filename, buf, false);
}

int heimdall_unlink (const char *filename)
{
    const char *local_name = ToLocaleDup (filename);
    if (unlikely(local_name == NULL))
    {
        errno = ENOENT;
        return -1;
    }

    int ret = unlink (local_name);
    free (local_name);
    return ret;
}

int heimdall_rename (const char *oldpath, const char *newpath)
{
    const char *lo = ToLocaleDup (oldpath);
    if (lo == NULL)
        goto error;

    const char *ln = ToLocaleDup (newpath);
    if (ln == NULL)
    {
        free (lo);
error:
        errno = ENOENT;
        return -1;
    }

    int ret = rename (lo, ln);
    free (lo);
    free (ln);
    return ret;
}

char *heimdall_getcwd (void)
{
    /* Try $PWD */
    const char *pwd = getenv ("PWD");
    if (pwd != NULL)
    {
        struct stat s1, s2;
        /* Make sure $PWD is correct */
        if (stat (pwd, &s1) == 0 && stat (".", &s2) == 0
         && s1.st_dev == s2.st_dev && s1.st_ino == s2.st_ino)
            return ToLocaleDup (pwd);
    }

    /* Otherwise iterate getcwd() until the buffer is big enough */
    long path_max = pathconf (".", _PC_PATH_MAX);
    size_t size = (path_max == -1 || path_max > 4096) ? 4096 : path_max;

    for (;; size *= 2)
    {
        char *buf = malloc (size);
        if (unlikely(buf == NULL))
            break;

        if (getcwd (buf, size) != NULL)
        {
            char *ret = ToLocaleDup (buf);
            free (buf);
            return ret; /* success */
        }
        free (buf);

        if (errno != ERANGE)
            break;
    }
    return NULL;
}

int heimdall_dup (int oldfd)
{
    int newfd;

    newfd = dup (oldfd);
    if (likely(newfd != -1))
        fcntl (newfd, F_SETFD, FD_CLOEXEC);

    return newfd;
}

int heimdall_dup2 (int oldfd, int newfd)
{
    int fd = dup2 (oldfd, newfd);
    if (fd != -1)
        setmode (fd, O_BINARY);

    return fd;
}

int heimdall_pipe (int fds[2])
{
    if (heimdall_socketpair (AF_LOCAL, SOCK_STREAM, 0, fds, false))
        return -1;

    shutdown (fds[0], SHUT_WR);
    shutdown (fds[1], SHUT_RD);

    setmode (fds[0], O_BINARY);
    setmode (fds[1], O_BINARY);

    return 0;
}

ssize_t heimdall_write(int fd, const void *buf, size_t len)
{
    struct iovec iov = { .iov_base = (void *)buf, .iov_len = len };

    return heimdall_writev(fd, &iov, 1);
}

ssize_t heimdall_writev(int fd, const struct iovec *iov, int count)
{
    sigset_t set, oset;

    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &set, &oset);

    ssize_t val = writev(fd, iov, count);
    if (val < 0 && errno == EPIPE)
    {
        siginfo_t info;
        struct timespec ts = { 0, 0 };

        while (sigtimedwait(&set, &info, &ts) >= 0 || errno != EAGAIN);
    }

    if (!sigismember(&oset, SIGPIPE)) /* Restore the signal mask if changed */
        pthread_sigmask(SIG_SETMASK, &oset, NULL);

    return val;
}

static void heimdall_socket_setup(int fd, bool nonblock)
{
    fcntl(fd, F_SETFD, FD_CLOEXEC);

    if (nonblock)
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

int heimdall_socket (int pf, int type, int proto, bool nonblock)
{
    int fd = socket(pf, type, proto);
    if (fd != -1)
        heimdall_socket_setup(fd, nonblock);
    return fd;
}

int heimdall_socketpair(int pf, int type, int proto, int fds[2], bool nonblock)
{
    if (socketpair(pf, type, proto, fds))
        return -1;

    heimdall_socket_setup(fds[0], nonblock);
    heimdall_socket_setup(fds[1], nonblock);
    return 0;
}

int heimdall_accept (int lfd, struct sockaddr *addr, socklen_t *alen, bool nonblock)
{
    do
    {
        int fd = accept (lfd, addr, alen);
        if (fd != -1)
        {
            fcntl (fd, F_SETFD, FD_CLOEXEC);
            if (nonblock)
                fcntl (fd, F_SETFL, fcntl (fd, F_GETFL, 0) | O_NONBLOCK);
            return fd;
        }
    }
    while (errno == EINTR);

    return -1;
}

ssize_t heimdall_send(int fd, const void *buf, size_t len, int flags)
{
    return send(fd, buf, len, flags);
}

ssize_t heimdall_sendto(int fd, const void *buf, size_t len, int flags,
                   const struct sockaddr *dst, socklen_t dstlen)
{
    return sendto(fd, buf, len, flags, dst, dstlen);
}

ssize_t heimdall_sendmsg(int fd, const struct msghdr *msg, int flags)
{
    return sendmsg(fd, msg, flags);
}
