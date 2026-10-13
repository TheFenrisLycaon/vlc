/*****************************************************************************
 * prefetch.c: prefetchinging module for Heimdall
 *****************************************************************************
 * Copyright © 2015 Rémi Denis-Courmont
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <unistd.h>

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_stream.h>
#include <heimdall_fs.h>
#include <heimdall_interrupt.h>

struct stream_ctrl
{
    struct stream_ctrl *next;
    int query;
    union
    {
        struct
        {
            int id;
            bool state;
        } id_state;
    };
};

typedef struct
{
    heimdall_mutex_t  lock;
    heimdall_cond_t   wait_data;
    heimdall_cond_t   wait_space;
    heimdall_thread_t thread;
    heimdall_interrupt_t *interrupt;

    bool         eof;
    bool         error;
    bool         paused;

    bool         can_seek;
    bool         can_pace;
    bool         can_pause;
    uint64_t     size;
    uint64_t     mtime;
    heimdall_tick_t   pts_delay;
    char        *content_type;

    uint64_t     buffer_offset;
    uint64_t     stream_offset;
    size_t       buffer_length;
    size_t       buffer_size;
    char        *buffer;
    size_t       seek_threshold;

    struct stream_ctrl *controls;
} stream_sys_t;

static ssize_t ThreadRead(stream_t *stream, void *buf, size_t length)
{
    stream_sys_t *sys = stream->p_sys;

    heimdall_mutex_unlock(&sys->lock);
    assert(length > 0);

    ssize_t val = heimdall_stream_ReadPartial(stream->s, buf, length);

    heimdall_mutex_lock(&sys->lock);
    return val;
}

static int ThreadSeek(stream_t *stream, uint64_t seek_offset)
{
    stream_sys_t *sys = stream->p_sys;

    heimdall_mutex_unlock(&sys->lock);

    int val = heimdall_stream_Seek(stream->s, seek_offset);
    if (val != HEIMDALL_SUCCESS)
        msg_Err(stream, "cannot seek (to offset %"PRIu64")", seek_offset);

    heimdall_mutex_lock(&sys->lock);

    return (val == HEIMDALL_SUCCESS) ? 0 : -1;
}

static int ThreadControl(stream_t *stream, int query, ...)
{
    stream_sys_t *sys = stream->p_sys;

    heimdall_mutex_unlock(&sys->lock);

    va_list ap;
    int ret;

    va_start(ap, query);
    ret = heimdall_stream_vaControl(stream->s, query, ap);
    va_end(ap);

    heimdall_mutex_lock(&sys->lock);
    return ret;
}

static void *Thread(void *data)
{
    heimdall_thread_set_name("heimdall-prefetch");

    stream_t *stream = data;
    stream_sys_t *sys = stream->p_sys;
    bool paused = false;

    heimdall_interrupt_set(sys->interrupt);

    heimdall_mutex_lock(&sys->lock);
    while (!heimdall_killed())
    {
        struct stream_ctrl *ctrl = sys->controls;

        if (unlikely(ctrl != NULL))
        {
            sys->controls = ctrl->next;
            ThreadControl(stream, ctrl->query, ctrl->id_state.id,
                          ctrl->id_state.state);
            free(ctrl);
            continue;
        }

        if (sys->paused != paused)
        {   /* Update pause state */
            msg_Dbg(stream, paused ? "resuming" : "pausing");
            paused = sys->paused;
            ThreadControl(stream, STREAM_SET_PAUSE_STATE, paused);
            continue;
        }

        if (paused || sys->error)
        {   /* Wait for not paused and not failed */
            heimdall_cond_wait(&sys->wait_space, &sys->lock);
            continue;
        }

        uint_fast64_t stream_offset = sys->stream_offset;

        if (stream_offset < sys->buffer_offset)
        {   /* Need to seek backward */
            if (ThreadSeek(stream, stream_offset) == 0)
            {
                sys->buffer_offset = stream_offset;
                sys->buffer_length = 0;
                assert(!sys->error);
                sys->eof = false;
            }
            else
            {
                sys->error = true;
                heimdall_cond_signal(&sys->wait_data);
            }
            continue;
        }

        if (sys->eof)
        {   /* Do not attempt to read at EOF - would busy loop */
            heimdall_cond_wait(&sys->wait_space, &sys->lock);
            continue;
        }

        assert(stream_offset >= sys->buffer_offset);

        /* As long as there is space, the buffer will retain already read
         * ("historical") data. The data can be used if/when seeking backward.
         * Unread data is however given precedence if the buffer is full. */
        uint64_t history = stream_offset - sys->buffer_offset;

        /* If upstream supports seeking and if the downstream offset is far
         * beyond the upstream offset, then attempt to skip forward.
         * If it fails, assume upstream is well-behaved such that the failed
         * seek is a no-op, and continue as if seeking was not supported.
         * WARNING: Except problems with misbehaving access plug-ins. */
        if (sys->can_seek
         && history >= (sys->buffer_length + sys->seek_threshold))
        {
            if (ThreadSeek(stream, stream_offset) == 0)
            {
                sys->buffer_offset = stream_offset;
                sys->buffer_length = 0;
                assert(!sys->error);
                assert(!sys->eof);
            }
            else
            {   /* Seek failure is not necessarily fatal here. We could read
                 * data instead until the desired seek offset. But in practice,
                 * not all upstream accesses handle reads after failed seek
                 * correctly. Furthermore, sys->stream_offset and/or
                 * sys->paused might have changed in the mean time. */
                sys->error = true;
                heimdall_cond_signal(&sys->wait_data);
            }
            continue;
        }

        assert(sys->buffer_size >= sys->buffer_length);

        size_t len = sys->buffer_size - sys->buffer_length;
        if (len == 0)
        {   /* Buffer is full */
            if (history == 0)
            {   /* Wait for data to be read */
                heimdall_cond_wait(&sys->wait_space, &sys->lock);
                continue;
            }

            /* Discard some historical data to make room. */
            len = history > sys->buffer_length ? sys->buffer_length : history;

            sys->buffer_offset += len;
            sys->buffer_length -= len;
        }

        size_t offset = (sys->buffer_offset + sys->buffer_length)
                        % sys->buffer_size;
         /* Do not step past the sharp edge of the circular buffer */
        if (offset + len > sys->buffer_size)
            len = sys->buffer_size - offset;

        ssize_t val = ThreadRead(stream, sys->buffer + offset, len);
        if (val < 0)
            continue;
        if (val == 0)
        {
            assert(len > 0);
            msg_Dbg(stream, "end of stream");
            sys->eof = true;
        }

        assert((size_t)val <= len);
        sys->buffer_length += val;
        assert(sys->buffer_length <= sys->buffer_size);
        //msg_Dbg(stream, "buffer: %zu/%zu", sys->buffer_length,
        //        sys->buffer_size);
        heimdall_cond_signal(&sys->wait_data);
    }

    sys->error = true;
    heimdall_cond_signal(&sys->wait_data);
    heimdall_mutex_unlock(&sys->lock);
    return NULL;
}

static int Seek(stream_t *stream, uint64_t offset)
{
    stream_sys_t *sys = stream->p_sys;

    heimdall_mutex_lock(&sys->lock);
    sys->stream_offset = offset;
    sys->error = false;
    heimdall_cond_signal(&sys->wait_space);
    heimdall_mutex_unlock(&sys->lock);
    return 0;
}

static size_t BufferLevel(const stream_t *stream, bool *eof)
{
    stream_sys_t *sys = stream->p_sys;

    *eof = false;

    if (sys->stream_offset < sys->buffer_offset)
        return 0;
    if ((sys->stream_offset - sys->buffer_offset) >= sys->buffer_length)
    {
        *eof = sys->eof;
        return 0;
    }
    return sys->buffer_offset + sys->buffer_length - sys->stream_offset;
}

static ssize_t Read(stream_t *stream, void *buf, size_t buflen)
{
    stream_sys_t *sys = stream->p_sys;
    size_t copy, offset;
    bool eof;

    if (buflen == 0)
        return buflen;

    heimdall_mutex_lock(&sys->lock);
    if (sys->paused)
    {
        msg_Err(stream, "reading while paused (buggy demux?)");
        sys->paused = false;
        heimdall_cond_signal(&sys->wait_space);
    }

    while ((copy = BufferLevel(stream, &eof)) == 0 && !eof)
    {
        void *data[2];

        if (sys->error)
        {
            heimdall_mutex_unlock(&sys->lock);
            return 0;
        }

        heimdall_interrupt_forward_start(sys->interrupt, data);
        heimdall_cond_wait(&sys->wait_data, &sys->lock);
        heimdall_interrupt_forward_stop(data);
    }

    offset = sys->stream_offset % sys->buffer_size;
    if (copy > buflen)
        copy = buflen;
    /* Do not step past the sharp edge of the circular buffer */
    if (offset + copy > sys->buffer_size)
        copy = sys->buffer_size - offset;

    memcpy(buf, sys->buffer + offset, copy);
    sys->stream_offset += copy;
    heimdall_cond_signal(&sys->wait_space);
    heimdall_mutex_unlock(&sys->lock);
    return copy;
}

static int Control(stream_t *stream, int query, va_list args)
{
    stream_sys_t *sys = stream->p_sys;

    switch (query)
    {
        case STREAM_CAN_SEEK:
            *va_arg(args, bool *) = sys->can_seek;
            break;
        case STREAM_CAN_FASTSEEK:
            *va_arg(args, bool *) = false;
            break;
        case STREAM_CAN_PAUSE:
             *va_arg(args, bool *) = sys->can_pause;
            break;
        case STREAM_CAN_CONTROL_PACE:
            *va_arg (args, bool *) = sys->can_pace;
            break;
        case STREAM_GET_SIZE:
            if (sys->size == (uint64_t)-1)
                return HEIMDALL_EGENERIC;
            *va_arg(args, uint64_t *) = sys->size;
            break;
        case STREAM_GET_MTIME:
            if (sys->mtime == (uint64_t)-1)
                return HEIMDALL_EGENERIC;
            *va_arg(args, uint64_t *) = sys->mtime;
            break;
        case STREAM_GET_PTS_DELAY:
            *va_arg(args, heimdall_tick_t *) = sys->pts_delay;
            break;
        case STREAM_GET_TITLE_INFO:
        case STREAM_GET_TITLE:
        case STREAM_GET_SEEKPOINT:
        case STREAM_GET_META:
            return HEIMDALL_EGENERIC;
        case STREAM_GET_CONTENT_TYPE:
            if (sys->content_type == NULL)
                return HEIMDALL_EGENERIC;
            *va_arg(args, char **) = strdup(sys->content_type);
            return HEIMDALL_SUCCESS;
        case STREAM_GET_SIGNAL:
        case STREAM_GET_TAGS:
        case STREAM_GET_TYPE:
            return HEIMDALL_EGENERIC;
        case STREAM_SET_PAUSE_STATE:
        {
            bool paused = va_arg(args, unsigned);

            heimdall_mutex_lock(&sys->lock);
            sys->paused = paused;
            heimdall_cond_signal(&sys->wait_space);
            heimdall_mutex_unlock (&sys->lock);
            break;
        }
        case STREAM_SET_TITLE:
        case STREAM_SET_SEEKPOINT:
            return HEIMDALL_EGENERIC;
        case STREAM_SET_PRIVATE_ID_STATE:
        {
            struct stream_ctrl *ctrl = malloc(sizeof (*ctrl)), **pp;
            if (unlikely(ctrl == NULL))
                return HEIMDALL_ENOMEM;

            ctrl->next = NULL;
            ctrl->query = query;
            ctrl->id_state.id = va_arg(args, int);
            ctrl->id_state.state = va_arg(args, int);
            heimdall_mutex_lock(&sys->lock);
            for (pp = &sys->controls; *pp != NULL; pp = &((*pp)->next));
            *pp = ctrl;
            heimdall_cond_signal(&sys->wait_space);
            heimdall_mutex_unlock(&sys->lock);
            break;
        }
        case STREAM_SET_PRIVATE_ID_CA:
        case STREAM_GET_PRIVATE_ID_STATE:
            return HEIMDALL_EGENERIC;
        default:
            msg_Err(stream, "unimplemented query (%d) in control", query);
            return HEIMDALL_EGENERIC;
    }
    return HEIMDALL_SUCCESS;
}

static int Open(heimdall_object_t *obj)
{
    stream_t *stream = (stream_t *)obj;

    /* For local files, the operating system is likely to do a better work at
     * caching/prefetching. Also, prefetching with this module could cause
     * undesirable high load at start-up. Lastly, local files may require
     * support for title/seekpoint and meta control requests. */
    if (heimdall_stream_CanFastSeek(stream->s))
        return HEIMDALL_EGENERIC;

    /* PID-filtered streams are not suitable for prefetching, as they would
     * suffer excessive latency to enable a PID. DVB would also require support
     * for the signal level and Conditional Access controls.
     * TODO? For seekable streams, a forced could work around the problem. */
    if (heimdall_stream_GetPrivateIdState(stream->s, 0, &(bool){false}) ==
        HEIMDALL_SUCCESS)
        return HEIMDALL_EGENERIC;

    stream_sys_t *sys = malloc(sizeof (*sys));
    if (unlikely(sys == NULL))
        return HEIMDALL_ENOMEM;

    sys->can_seek = heimdall_stream_CanSeek(stream->s);
    sys->can_pause = heimdall_stream_CanPause(stream->s);
    sys->can_pace = heimdall_stream_CanPace(stream->s);

    if (heimdall_stream_GetSize(stream->s, &sys->size) != HEIMDALL_SUCCESS)
        sys->size = -1;
    if (heimdall_stream_GetMTime(stream->s, &sys->mtime) != HEIMDALL_SUCCESS)
        sys->mtime = -1;

    sys->pts_delay = heimdall_stream_GetPtsDelay(stream->s);
    if (heimdall_stream_GetContentType(stream->s, &sys->content_type) != HEIMDALL_SUCCESS)
        sys->content_type = NULL;

    sys->eof = false;
    sys->error = false;
    sys->paused = false;
    sys->buffer_offset = 0;
    sys->stream_offset = 0;
    sys->buffer_length = 0;
    sys->buffer_size = var_InheritInteger(obj, "prefetch-buffer-size") << 10u;
    sys->seek_threshold = var_InheritInteger(obj, "prefetch-seek-threshold");
    sys->controls = NULL;

    uint64_t size = stream_Size(stream->s);
    if (size > 0)
    {   /* No point allocating a buffer larger than the source stream */
        if (sys->buffer_size > size)
            sys->buffer_size = size;
    }

    sys->buffer = malloc(sys->buffer_size);
    if (sys->buffer == NULL)
        goto error;

    sys->interrupt = heimdall_interrupt_create();
    if (unlikely(sys->interrupt == NULL))
        goto error;

    heimdall_mutex_init(&sys->lock);
    heimdall_cond_init(&sys->wait_data);
    heimdall_cond_init(&sys->wait_space);

    stream->p_sys = sys;

    if (heimdall_clone(&sys->thread, Thread, stream))
    {
        heimdall_interrupt_destroy(sys->interrupt);
        goto error;
    }

    msg_Dbg(stream, "using %zu bytes buffer", sys->buffer_size);
    stream->pf_read = Read;
    stream->pf_seek = Seek;
    stream->pf_control = Control;
    return HEIMDALL_SUCCESS;

error:
    free(sys->buffer);
    free(sys->content_type);
    free(sys);
    return HEIMDALL_ENOMEM;
}


/**
 * Releases allocate resources.
 */
static void Close (heimdall_object_t *obj)
{
    stream_t *stream = (stream_t *)obj;
    stream_sys_t *sys = stream->p_sys;

    heimdall_mutex_lock(&sys->lock);
    heimdall_interrupt_kill(sys->interrupt);
    heimdall_cond_signal(&sys->wait_space);
    heimdall_mutex_unlock(&sys->lock);

    heimdall_join(sys->thread, NULL);
    heimdall_interrupt_destroy(sys->interrupt);

    while(sys->controls)
    {
        struct stream_ctrl *ctrl = sys->controls;
        sys->controls = ctrl->next;
        free(ctrl);
    }
    free(sys->buffer);
    free(sys->content_type);
    free(sys);
}

heimdall_module_begin()
    set_subcategory(SUBCAT_INPUT_STREAM_FILTER)
    set_capability("stream_filter", 0)

    set_description(N_("Stream prefetch filter"))
    set_callbacks(Open, Close)

    add_integer("prefetch-buffer-size", 1 << 14, N_("Buffer size"),
                N_("Prefetch buffer size (KiB)"))
        change_integer_range(4, 1 << 20)
    add_obsolete_integer("prefetch-read-size") /* since 4.0.0 */
    add_integer("prefetch-seek-threshold", 1 << 14, N_("Seek threshold"),
                N_("Prefetch forward seek threshold (bytes)"))
        change_integer_range(0, UINT64_C(1) << 60)
heimdall_module_end()
