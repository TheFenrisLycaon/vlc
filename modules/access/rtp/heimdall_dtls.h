/*****************************************************************************
 * datagram.h:
 *****************************************************************************
 * Copyright (C) 2020 Rémi Denis-Courmont
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

#ifndef HEIMDALL_DATAGRAM_SOCKET_H
# define HEIMDALL_DATAGRAM_SOCKET_H

struct iovec;

/**
 * Datagram socket
 */
struct heimdall_dtls {
    const struct heimdall_dtls_operations *ops;
};

struct heimdall_dtls_operations {
    void (*close)(struct heimdall_dtls *);

    int (*get_fd)(struct heimdall_dtls *, short *events);
    ssize_t (*readv)(struct heimdall_dtls *, struct iovec *iov, unsigned len,
                     bool *restrict truncated);
    ssize_t (*writev)(struct heimdall_dtls *, const struct iovec *iov, unsigned len);
};

static inline void heimdall_dtls_Close(struct heimdall_dtls *dgs)
{
    dgs->ops->close(dgs);
}

static inline int heimdall_dtls_GetPollFD(struct heimdall_dtls *dgs, short *restrict ev)
{
    return dgs->ops->get_fd(dgs, ev);
}

static inline ssize_t heimdall_dtls_Recv(struct heimdall_dtls *dgs, void *buf, size_t len,
                                   bool *restrict truncated)
{
    struct iovec iov = { .iov_base = buf, .iov_len = len };

    return dgs->ops->readv(dgs, &iov, 1, truncated);
}

static inline ssize_t heimdall_dtls_Send(struct heimdall_dtls *dgs, const void *buf,
                                   size_t len)
{
    struct iovec iov = { .iov_base = (void *)buf, .iov_len = len };

    return dgs->ops->writev(dgs, &iov, 1);
}

struct heimdall_dtls *heimdall_datagram_CreateFD(int fd);
struct heimdall_dtls *heimdall_dccp_CreateFD(int fd);

#endif
