/**
 * @file shm.c
 * @brief Shared memory frame buffer capture module for Heimdall media player
 */
/*****************************************************************************
 * Copyright © 2011 Rémi Denis-Courmont
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
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 ****************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdarg.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef HAVE_SYS_SHM_H
# include <sys/ipc.h>
# include <sys/shm.h>
#endif

#include <heimdall_common.h>
#include <heimdall_demux.h>
#include <heimdall_fs.h>
#include <heimdall_plugin.h>

#define FPS_TEXT N_("Frame rate")
#define FPS_LONGTEXT N_( \
    "How many times the screen content should be refreshed per second.")

#define DEPTH_TEXT N_("Frame buffer depth")
#define DEPTH_LONGTEXT N_( \
    "Pixel depth of the frame buffer, or zero for XWD file")

#define WIDTH_TEXT N_("Frame buffer width")
#define WIDTH_LONGTEXT N_( \
    "Pixel width of the frame buffer (ignored for XWD file)")

#define HEIGHT_TEXT N_("Frame buffer height")
#define HEIGHT_LONGTEXT N_( \
    "Pixel height of the frame buffer (ignored for XWD file)")

#define ID_TEXT N_("Frame buffer segment ID")
#define ID_LONGTEXT N_( \
    "System V shared memory segment ID of the frame buffer " \
    "(this is ignored if --shm-file is specified).")

#define FILE_TEXT N_("Frame buffer file")
#define FILE_LONGTEXT N_( \
    "Path of the memory mapped file of the frame buffer")

static int  Open (heimdall_object_t *);
static void Close (heimdall_object_t *);

static const int depths[] = {
    0, 8, 15, 16, 24, 32,
};

static const char *const depth_texts[] = {
    N_("XWD file (autodetect)"),
    N_("8 bits"), N_("15 bits"), N_("16 bits"), N_("24 bits"), N_("32 bits"),
};

/*
 * Module descriptor
 */
heimdall_module_begin ()
    set_shortname (N_("Framebuffer input"))
    set_description (N_("Shared memory framebuffer"))
    set_subcategory (SUBCAT_INPUT_ACCESS)
    set_capability ("access", 0)
    set_callbacks (Open, Close)

    add_float ("shm-fps", 10.0, FPS_TEXT, FPS_LONGTEXT)
    add_integer ("shm-depth", 0, DEPTH_TEXT, DEPTH_LONGTEXT)
        change_integer_list (depths, depth_texts)
        change_safe ()
    add_integer ("shm-width", 800, WIDTH_TEXT, WIDTH_LONGTEXT)
        change_integer_range (0, 65535)
        change_safe ()
    add_integer ("shm-height", 480, HEIGHT_TEXT, HEIGHT_LONGTEXT)
        change_integer_range (0, 65535)
        change_safe ()

    /* We need to "trust" the memory segment. If it were shrunk while we copy
     * its content our process may crash - or worse. So we pass the shared
     * memory location via an unsafe variable rather than the URL. */
    add_string ("shm-file", NULL, FILE_TEXT, FILE_LONGTEXT)
        change_volatile ()
#ifdef HAVE_SYS_SHM_H
    add_integer ("shm-id", (int64_t)IPC_PRIVATE, ID_TEXT, ID_LONGTEXT)
        change_volatile ()
#endif
    add_shortcut ("shm")
heimdall_module_end ()

typedef struct demux_sys_t demux_sys_t;

static int Control (demux_t *, int, va_list);
static void DemuxFile (void *);
static void CloseFile (demux_sys_t *);
#ifdef HAVE_SYS_SHM_H
static void DemuxIPC (void *);
static void CloseIPC (demux_sys_t *);
#endif

struct demux_sys_t
{
    /* Everything is read-only when timer is armed. */
    union
    {
        int fd;
        struct
        {
             const void  *addr;
             size_t       length;
        } mem;
    };
    es_out_id_t *es;
    heimdall_timer_t  timer;
    void (*detach) (demux_sys_t *);
};

static int Open (heimdall_object_t *obj)
{
    demux_t *demux = (demux_t *)obj;
    if (demux->out == NULL)
        return HEIMDALL_EGENERIC;

    demux_sys_t *sys = heimdall_obj_malloc(obj, sizeof (*sys));
    if (unlikely(sys == NULL))
        return HEIMDALL_ENOMEM;

    uint32_t chroma;
    uint16_t width = 0, height = 0;
    uint8_t bpp;
    switch (var_InheritInteger (demux, "shm-depth"))
    {
        case 32:
            chroma = HEIMDALL_CODEC_XRGB; bpp = 32;
            break;
        case 24:
            chroma = HEIMDALL_CODEC_RGB24; bpp = 24;
            break;
        case 16:
            chroma = HEIMDALL_CODEC_RGB565; bpp = 16;
            break;
        case 15:
            chroma = HEIMDALL_CODEC_RGB555; bpp = 16;
            break;
        case 8:
            chroma = HEIMDALL_CODEC_RGB233; bpp = 8;
            break;
        case 0:
            chroma = HEIMDALL_CODEC_XWD; bpp = 0;
            break;
        default:
            return HEIMDALL_EGENERIC;
    }
    if (bpp != 0)
    {
        width = var_InheritInteger (demux, "shm-width");
        height = var_InheritInteger (demux, "shm-height");
    }

    static void (*Demux) (void *);

    char *path = var_InheritString (demux, "shm-file");
    if (path != NULL)
    {
        sys->fd = heimdall_open (path, O_RDONLY);
        if (sys->fd == -1)
            msg_Err (demux, "cannot open file %s: %s", path,
                     heimdall_strerror_c(errno));
        free (path);
        if (sys->fd == -1)
            return HEIMDALL_EGENERIC;

        sys->detach = CloseFile;
        Demux = DemuxFile;
    }
    else
    {
#ifdef HAVE_SYS_SHM_H
        sys->mem.length = width * height * (bpp >> 3);
        if (sys->mem.length == 0)
            return HEIMDALL_EGENERIC;

        int id = var_InheritInteger (demux, "shm-id");
        if (id == IPC_PRIVATE)
            return HEIMDALL_EGENERIC;
        void *mem = shmat (id, NULL, SHM_RDONLY);

        if (mem == (const void *)(-1))
        {
            msg_Err (demux, "cannot attach segment %d: %s", id,
                     heimdall_strerror_c(errno));
            return HEIMDALL_EGENERIC;
        }
        sys->mem.addr = mem;
        sys->detach = CloseIPC;
        Demux = DemuxIPC;
#else
        goto error;
#endif
    }

    /* Initializes format */
    float rate = var_InheritFloat (obj, "shm-fps");
    if (rate <= 0.f)
        goto error;

    heimdall_tick_t interval = llroundf((float)CLOCK_FREQ / rate);
    if (!interval)
        goto error;

    es_format_t fmt;
    es_format_Init (&fmt, VIDEO_ES, chroma);
    video_format_Setup(&fmt.video, chroma, width, height, width, height, 1, 1);
    fmt.video.i_frame_rate = 1000 * rate;
    fmt.video.i_frame_rate_base = 1000;

    sys->es = es_out_Add (demux->out, &fmt);

    /* Initializes demux */
    if (heimdall_timer_create (&sys->timer, Demux, demux))
        goto error;
    heimdall_timer_schedule_asap (sys->timer, interval);

    demux->p_sys = sys;
    demux->pf_demux   = NULL;
    demux->pf_control = Control;
    return HEIMDALL_SUCCESS;

error:
    sys->detach (sys);
    return HEIMDALL_EGENERIC;
}


/**
 * Releases resources
 */
static void Close (heimdall_object_t *obj)
{
    demux_t *demux = (demux_t *)obj;
    demux_sys_t *sys = demux->p_sys;

    heimdall_timer_destroy (sys->timer);
    sys->detach (sys);
}

/**
 * Control callback
 */
static int Control (demux_t *demux, int query, va_list args)
{
    switch (query)
    {
        case DEMUX_GET_POSITION:
        {
            float *v = va_arg (args, float *);
            *v = 0.;
            return HEIMDALL_SUCCESS;
        }

        case DEMUX_GET_LENGTH:
        case DEMUX_GET_TIME:
        {
            *va_arg (args, heimdall_tick_t *) = 0;
            return HEIMDALL_SUCCESS;
        }

        case DEMUX_GET_PTS_DELAY:
        {
            *va_arg (args, heimdall_tick_t *) =
                HEIMDALL_TICK_FROM_MS( var_InheritInteger (demux, "live-caching") );
            return HEIMDALL_SUCCESS;
        }

        case DEMUX_CAN_PAUSE:
        case DEMUX_CAN_CONTROL_PACE:
        case DEMUX_CAN_CONTROL_RATE:
        case DEMUX_CAN_SEEK:
        {
            bool *v = va_arg (args, bool *);
            *v = false;
            return HEIMDALL_SUCCESS;
        }

        case DEMUX_SET_PAUSE_STATE:
            return HEIMDALL_SUCCESS; /* should not happen */
    }

    return HEIMDALL_EGENERIC;
}

/**
 * Processing callback
 */
static void DemuxFile (void *data)
{
    demux_t *demux = data;
    demux_sys_t *sys = demux->p_sys;

    /* Copy frame */
    block_t *block = block_File(sys->fd, true);
    if (block == NULL)
        return;
    block->i_pts = block->i_dts = heimdall_tick_now ();

    /* Send block */
    es_out_SetPCR(demux->out, block->i_pts);
    es_out_Send (demux->out, sys->es, block);
}

static void CloseFile (demux_sys_t *sys)
{
    heimdall_close (sys->fd);
}

#ifdef HAVE_SYS_SHM_H
static void DemuxIPC (void *data)
{
    demux_t *demux = data;
    demux_sys_t *sys = demux->p_sys;

    /* Copy frame */
    block_t *block = block_Alloc (sys->mem.length);
    if (block == NULL)
        return;
    memcpy (block->p_buffer, sys->mem.addr, sys->mem.length);
    block->i_pts = block->i_dts = heimdall_tick_now ();

    /* Send block */
    es_out_SetPCR(demux->out, block->i_pts);
    es_out_Send (demux->out, sys->es, block);
}

static void CloseIPC (demux_sys_t *sys)
{
    shmdt (sys->mem.addr);
}
#endif
