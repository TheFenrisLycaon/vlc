/*****************************************************************************
 * buffers.c: Video4Linux2 input module for heimdall
 *****************************************************************************
 * Copyright (C) 2002-2009 VLC authors and VideoLAN
 * Copyright (C) 2011-2012 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Benjamin Pracht <bigben at lycaria dot org>
 *          Richard Hosking <richard at hovis dot net>
 *          Antoine Cellerier <dionoea at lycaria d.t org>
 *          Dennis Lou <dlou99 at yahoo dot com>
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
#include <errno.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <heimdall_common.h>
#include <heimdall_block.h>

#include "v4l2.h"

heimdall_tick_t GetBufferPTS(const struct v4l2_buffer *buf)
{
    heimdall_tick_t pts;

    switch (buf->flags & V4L2_BUF_FLAG_TIMESTAMP_MASK)
    {
        case V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC:
            pts = heimdall_tick_from_timeval(&buf->timestamp);
            break;
        case V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN:
        default:
            pts = heimdall_tick_now();
            break;
    }
    return pts;
}

static void DestroyBuffer(struct heimdall_v4l2_buffers *pool,
                          struct heimdall_v4l2_buffer *buf)
{
    block_t *block = &buf->block;

    v4l2_munmap(block->p_start, block->i_size);
    free(buf);

    if (heimdall_atomic_rc_dec(&pool->refs)) {
        free(pool->bufs);
        free(pool);
    }
}

static void ReleaseBuffer(block_t *block)
{
    struct heimdall_v4l2_buffer *buf = container_of(block, struct heimdall_v4l2_buffer,
                                               block);
    struct heimdall_v4l2_buffers *pool = buf->pool;
    struct v4l2_buffer buf_req = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
        .index = buf->index,
    };
    int fd;

    assert(buf->index < pool->count);
    assert(pool->bufs[buf->index] == NULL);

    heimdall_mutex_lock(&pool->lock);
    fd = pool->fd;

    if (likely(fd >= 0)) {
        pool->bufs[buf->index] = buf;
        v4l2_ioctl(pool->fd, VIDIOC_QBUF, &buf_req);
        atomic_fetch_add(&pool->unused, 1);
    }
    heimdall_mutex_unlock(&pool->lock);

    if (unlikely(fd < 0))
        DestroyBuffer(pool, buf);
}

static const struct heimdall_block_callbacks heimdall_v4l2_buffer_cbs = {
    ReleaseBuffer,
};

static struct heimdall_v4l2_buffer *AllocateBuffer(struct heimdall_v4l2_buffers *pool,
                                              uint32_t index)
{
    struct v4l2_buffer buf_req = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
        .index = index,
    };
    int fd = pool->fd;

    if (v4l2_ioctl(fd, VIDIOC_QUERYBUF, &buf_req) < 0)
        return NULL;

    struct heimdall_v4l2_buffer *buf = malloc(sizeof (*buf));
    if (unlikely(buf == NULL))
        return NULL;

    void *base = v4l2_mmap(NULL, buf_req.length, PROT_READ | PROT_WRITE,
                           MAP_SHARED, fd, buf_req.m.offset);
    if (base == MAP_FAILED) {
        free(buf);
        return NULL;
    }

    block_Init(&buf->block, &heimdall_v4l2_buffer_cbs, base, buf_req.length);
    buf->pool = pool;
    buf->index = index;
    heimdall_atomic_rc_inc(&pool->refs);

    assert(buf->index < pool->count);
    assert(pool->bufs[index] == NULL);
    pool->bufs[index] = buf;

    if (v4l2_ioctl(fd, VIDIOC_QBUF, &buf_req) < 0) {
        DestroyBuffer(pool, buf);
        buf = NULL;
    }

    return buf;
}

block_t *GrabVideo(heimdall_object_t *demux, struct heimdall_v4l2_buffers *restrict pool)
{
    int fd = pool->fd;
    struct v4l2_buffer buf_req = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
    };

    /* Wait for next frame */
    if (v4l2_ioctl(fd, VIDIOC_DQBUF, &buf_req) < 0)
    {
        switch (errno)
        {
            case EAGAIN:
                return NULL;
            case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */
            default:
                msg_Err(demux, "dequeue error: %s", heimdall_strerror_c(errno));
                return NULL;
        }
    }

    assert(buf_req.index < pool->count);

    struct heimdall_v4l2_buffer *const buf = pool->bufs[buf_req.index];

    assert(buf != NULL);
    assert(buf->index == buf_req.index);
    pool->bufs[buf_req.index] = NULL;

    block_t *block = &buf->block;
    /* Reinitialise the buffer */
    block->p_buffer = block->p_start;
    assert(buf_req.bytesused <= block->i_size);
    block->i_buffer = buf_req.bytesused;
    block->p_next = NULL;

    if (atomic_fetch_sub(&pool->unused, 1) <= 2) {
        /* Running out of buffers! Memory copy forced. */
        block = block_Duplicate(block);
        block_Release(&buf->block);
    }

    block->i_pts = block->i_dts = GetBufferPTS(&buf_req);
    return block;
}

/**
 * Allocates memory-mapped buffers, queues them and start streaming.
 * @return array of allocated buffers (use free()), or NULL on error.
 */
struct heimdall_v4l2_buffers *StartMmap(heimdall_object_t *obj, int fd)
{
    struct heimdall_v4l2_buffers *pool;
    struct v4l2_requestbuffers req = {
        .count = 16,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
    };

    if (v4l2_ioctl(fd, VIDIOC_REQBUFS, &req) < 0)
    {
        msg_Err(obj, "cannot allocate buffers: %s", heimdall_strerror_c(errno));
        return NULL;
    }

    if (req.count < 2)
    {
        msg_Err(obj, "cannot allocate enough buffers");
        return NULL;
    }

    pool = malloc(sizeof (*pool));
    if (unlikely(pool == NULL))
        return NULL;

    pool->bufs = calloc(req.count, sizeof (pool->bufs[0]));
    if (unlikely(pool->bufs == NULL)) {
        free(pool);
        return NULL;
    }

    pool->count = req.count;
    for (size_t i = 0; i < req.count; i++)
        pool->bufs[i] = NULL;

    pool->fd = fd;
    heimdall_atomic_rc_init(&pool->refs);
    heimdall_mutex_init(&pool->lock);

    for (uint32_t index = 0; index < req.count; index++)
    {
        struct heimdall_v4l2_buffer *buf = AllocateBuffer(pool, index);

        if (unlikely(buf == NULL)) {
            msg_Err(obj, "cannot allocate buffer %"PRIu32": %s", index,
                    heimdall_strerror_c(errno));
            goto error;
        }
    }

    atomic_init(&pool->unused, pool->count);

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (v4l2_ioctl(fd, VIDIOC_STREAMON, &type) < 0)
    {
        msg_Err (obj, "cannot start streaming: %s", heimdall_strerror_c(errno));
        goto error;
    }
    return pool;
error:
    StopMmap(pool);
    return NULL;
}

void StopMmap(struct heimdall_v4l2_buffers *pool)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    const size_t count = pool->count;
    int fd = pool->fd;

    heimdall_mutex_lock(&pool->lock);
    pool->fd = -1;
    heimdall_mutex_unlock(&pool->lock);

    /* STREAMOFF implicitly dequeues all buffers */
    v4l2_ioctl(fd, VIDIOC_STREAMOFF, &type);

    for (size_t i = 0; i < count; i++) {
        struct heimdall_v4l2_buffer *const buf = pool->bufs[i];

        if (buf != NULL)
            DestroyBuffer(pool, buf);
    }

    if (heimdall_atomic_rc_dec(&pool->refs)) {
        free(pool->bufs);
        free(pool);
    }
}
