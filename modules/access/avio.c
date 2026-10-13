/*****************************************************************************
 * avio.c: access using libavformat library
 *****************************************************************************
 * Copyright (C) 2009 Laurent Aimar
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Laurent Aimar <fenrir _AT_ lycaria _DOT_ org>
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

#include <heimdall_common.h>
#include <heimdall_configuration.h>
#include <heimdall_plugin.h>
#include <heimdall_access.h>
#include <heimdall_sout.h>
#include <heimdall_avcodec.h>
#include <heimdall_interrupt.h>

#include "avio.h"
#include "../codec/avcodec/avcommon.h"

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
#ifndef MERGE_FFMPEG
heimdall_module_begin()
    AVIO_MODULE
heimdall_module_end()
#endif

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static ssize_t Read   (stream_t *, void *, size_t);
static int     Seek   (stream_t *, uint64_t);
static int     Control(stream_t *, int, va_list);
static ssize_t Write(sout_access_out_t *, block_t *);
static int     OutControl(sout_access_out_t *, int, va_list);
static int     OutSeek (sout_access_out_t *, uint64_t);

static int UrlInterruptCallback(void *access)
{
    /* NOTE: This works so long as libavformat invokes the callback from the
     * same thread that invokes libavformat. Currently libavformat does not
     * create internal threads at all. This is not proper event handling in any
     * case; libavformat needs fixing. */
    (void) access;
    return heimdall_killed();
}

typedef struct
{
    AVIOContext *context;
    int64_t size;
} access_sys_t;

typedef struct
{
    AVIOContext *context;
} sout_access_out_sys_t;


/* */

/* */

int OpenAvio(heimdall_object_t *object)
{
    stream_t *access = (stream_t*)object;
    access_sys_t *sys = heimdall_obj_malloc(object, sizeof(*sys));
    if (!sys)
        return HEIMDALL_ENOMEM;
    sys->context = NULL;

    /* We accept:
     * - avio://full_url
     * - url (only a subset of available protocols).
     */
    char *url;
    if (!strcmp(access->psz_name, "avio"))
        url = strdup(access->psz_location);
    else if (asprintf(&url, "%s://%s", access->psz_name,
                      access->psz_location) < 0)
        url = NULL;

    if (!url)
        return HEIMDALL_ENOMEM;

    /* */
    heimdall_init_avformat(object);

    int ret;
    AVIOInterruptCB cb = {
        .callback = UrlInterruptCallback,
        .opaque = access,
    };
    AVDictionary *options = NULL;
    char *psz_opts = var_InheritString(access, "avio-options");
    if (psz_opts) {
        heimdall_av_get_options(psz_opts, &options);
        free(psz_opts);
    }
    ret = avio_open2(&sys->context, url, AVIO_FLAG_READ, &cb, &options);
    AVDictionaryEntry *t = NULL;
    while ((t = av_dict_get(options, "", t, AV_DICT_IGNORE_SUFFIX)))
        msg_Err( access, "unknown option \"%s\"", t->key );
    av_dict_free(&options);
    if (ret < 0) {
        msg_Err(access, "Failed to open %s: %s", url,
                heimdall_strerror_c(AVUNERROR(ret)));
        free(url);
        return HEIMDALL_EGENERIC;
    }
    free(url);

    sys->size = avio_size(sys->context);

    bool seekable;
    seekable = sys->context->seekable;
    msg_Dbg(access, "%sseekable, size=%"PRIi64, seekable ? "" : "not ",
            sys->size);

    /* */
    access->pf_read = Read;
    access->pf_block = NULL;
    access->pf_control = Control;
    access->pf_seek = Seek;
    access->p_sys = sys;

    return HEIMDALL_SUCCESS;
}

/* */

static const char *const ppsz_sout_options[] = {
    "options",
    NULL,
};

int OutOpenAvio(heimdall_object_t *object)
{
    sout_access_out_t *access = (sout_access_out_t*)object;

    config_ChainParse( access, "sout-avio-", ppsz_sout_options, access->p_cfg );

    sout_access_out_sys_t *sys = heimdall_obj_malloc(object, sizeof(*sys));
    if (!sys)
        return HEIMDALL_ENOMEM;
    sys->context = NULL;

    /* */
    heimdall_init_avformat(object);

    if (!access->psz_path)
        return HEIMDALL_EGENERIC;

    int ret;
    AVDictionary *options = NULL;
    char *psz_opts = var_InheritString(access, "sout-avio-options");
    if (psz_opts) {
        heimdall_av_get_options(psz_opts, &options);
        free(psz_opts);
    }
    ret = avio_open2(&sys->context, access->psz_path, AVIO_FLAG_WRITE,
                     NULL, &options);
    AVDictionaryEntry *t = NULL;
    while ((t = av_dict_get(options, "", t, AV_DICT_IGNORE_SUFFIX)))
        msg_Err( access, "unknown option \"%s\"", t->key );
    av_dict_free(&options);
    if (ret < 0) {
        errno = AVUNERROR(ret);
        msg_Err(access, "Failed to open %s", access->psz_path);
        return HEIMDALL_EGENERIC;
    }

    access->pf_write = Write;
    access->pf_control = OutControl;
    access->pf_seek = OutSeek;
    access->p_sys = sys;

    return HEIMDALL_SUCCESS;
}

void CloseAvio(heimdall_object_t *object)
{
    stream_t *access = (stream_t*)object;
    access_sys_t *sys = access->p_sys;

    avio_close(sys->context);
}

void OutCloseAvio(heimdall_object_t *object)
{
    sout_access_out_t *access = (sout_access_out_t*)object;
    sout_access_out_sys_t *sys = access->p_sys;

    avio_close(sys->context);
}

static ssize_t Read(stream_t *access, void *data, size_t size)
{
    access_sys_t *sys = access->p_sys;

    int r = avio_read(sys->context, data, size);
    if (r < 0)
        r = 0;
    return r;
}

/*****************************************************************************
 * Write:
 *****************************************************************************/
static ssize_t Write(sout_access_out_t *p_access, block_t *p_buffer)
{
    sout_access_out_sys_t *p_sys = (sout_access_out_sys_t*)p_access->p_sys;
    size_t i_write = 0;
    int val;

    while (p_buffer != NULL) {
        block_t *p_next = p_buffer->p_next;

        avio_write(p_sys->context, p_buffer->p_buffer, p_buffer->i_buffer);
        avio_flush(p_sys->context);
        if ((val = p_sys->context->error) != 0) {
            p_sys->context->error = 0; /* FIXME? */
            goto error;
        }
        i_write += p_buffer->i_buffer;

        block_Release(p_buffer);

        p_buffer = p_next;
    }

    return i_write;

error:
    msg_Err(p_access, "Wrote only %zu bytes: %s", i_write,
            heimdall_strerror_c(AVUNERROR(val)));
    block_ChainRelease( p_buffer );
    return i_write;
}

static int Seek(stream_t *access, uint64_t position)
{
    access_sys_t *sys = access->p_sys;
    int ret;

#ifndef EOVERFLOW
# define EOVERFLOW EFBIG
#endif

    if (position > INT64_MAX)
        ret = AVERROR(EOVERFLOW);
    else
        ret = avio_seek(sys->context, position, SEEK_SET);
    if (ret < 0) {
        msg_Err(access, "Seek to %"PRIu64" failed: %s", position,
                heimdall_strerror_c(AVUNERROR(ret)));
        if (sys->size < 0 || position != (uint64_t)sys->size)
            return HEIMDALL_EGENERIC;
    }
    return HEIMDALL_SUCCESS;
}

static int OutSeek(sout_access_out_t *p_access, uint64_t i_pos)
{
    sout_access_out_sys_t *sys = p_access->p_sys;

    if (avio_seek(sys->context, i_pos, SEEK_SET) < 0)
        return HEIMDALL_EGENERIC;
    return HEIMDALL_SUCCESS;
}

static int OutControl(sout_access_out_t *p_access, int i_query, va_list args)
{
    sout_access_out_sys_t *p_sys = p_access->p_sys;

    HEIMDALL_UNUSED(p_sys);
    switch (i_query) {
    case ACCESS_OUT_CONTROLS_PACE: {
        bool *pb = va_arg(args, bool *);
        //*pb = strcmp(p_access->psz_name, "stream");
        *pb = false;
        break;
    }

    default:
        return HEIMDALL_EGENERIC;
    }
    return HEIMDALL_SUCCESS;
}

static int Control(stream_t *access, int query, va_list args)
{
    access_sys_t *sys = access->p_sys;
    bool *b;

    switch (query) {
    case STREAM_CAN_SEEK:
    case STREAM_CAN_FASTSEEK: /* FIXME how to do that ? */
        b = va_arg(args, bool *);
        *b = sys->context->seekable;
        return HEIMDALL_SUCCESS;
    case STREAM_CAN_PAUSE:
        b = va_arg(args, bool *);
        *b = sys->context->read_pause != NULL;
        return HEIMDALL_SUCCESS;
    case STREAM_CAN_CONTROL_PACE:
        b = va_arg(args, bool *);
        *b = true; /* FIXME */
        return HEIMDALL_SUCCESS;
    case STREAM_GET_SIZE:
        if (sys->size < 0)
            return HEIMDALL_EGENERIC;
        *va_arg(args, uint64_t *) = sys->size;
        return HEIMDALL_SUCCESS;
    case STREAM_GET_PTS_DELAY:
        *va_arg(args, heimdall_tick_t *) =
            HEIMDALL_TICK_FROM_MS(var_InheritInteger(access, "network-caching"));
        return HEIMDALL_SUCCESS;

    case STREAM_SET_PAUSE_STATE: {
        bool is_paused = va_arg(args, int);
        if (avio_pause(sys->context, is_paused)< 0)
            return HEIMDALL_EGENERIC;
        return HEIMDALL_SUCCESS;
    }
    default:
        return HEIMDALL_EGENERIC;
    }
}
