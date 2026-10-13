/*****************************************************************************
 * v4l2.h : Video4Linux2 input module for heimdall
 *****************************************************************************
 * Copyright (C) 2002-2011 VLC authors and VideoLAN
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

#if defined(HAVE_LINUX_VIDEODEV2_H)
#   include <linux/videodev2.h>
#elif defined(HAVE_SYS_VIDEOIO_H)
#   include <sys/ioccom.h>
#   include <sys/videoio.h>
#endif

/* libv4l2 functions */
extern int v4l2_fd_open(int, int);
extern int (*v4l2_close) (int);
extern int (*v4l2_ioctl) (int, unsigned long int, ...);
extern ssize_t (*v4l2_read) (int, void *, size_t);
extern void * (*v4l2_mmap) (void *, size_t, int, int, int, int64_t);
extern int (*v4l2_munmap) (void *, size_t);

#define CFG_PREFIX "v4l2-"

typedef struct heimdall_v4l2_ctrl heimdall_v4l2_ctrl_t;

#include <heimdall_atomic.h>
#include <heimdall_block.h>

struct heimdall_v4l2_buffer {
    block_t block;
    struct heimdall_v4l2_buffers *pool;
    uint32_t index;
};

struct heimdall_v4l2_buffers {
    size_t count;
    struct heimdall_v4l2_buffer **bufs;

    int fd;
    heimdall_atomic_rc_t refs;
    _Atomic size_t unused;
    heimdall_mutex_t lock;
};

/* v4l2.c */
void ParseMRL(heimdall_object_t *, const char *);
int OpenDevice (heimdall_object_t *, const char *, uint32_t *);
v4l2_std_id var_InheritStandard (heimdall_object_t *, const char *);

/* video.c */
int SetupTuner (heimdall_object_t *, int fd, uint32_t);
int SetupVideo(heimdall_object_t *, int fd, uint32_t,
               es_format_t *, uint32_t *, uint32_t *);

struct heimdall_v4l2_buffers *StartMmap(heimdall_object_t *, int);
void StopMmap(struct heimdall_v4l2_buffers *);

heimdall_tick_t GetBufferPTS (const struct v4l2_buffer *);
block_t* GrabVideo(heimdall_object_t *, struct heimdall_v4l2_buffers *);

#ifdef ZVBI_COMPILED
/* vbi.c */
typedef struct heimdall_v4l2_vbi heimdall_v4l2_vbi_t;

heimdall_v4l2_vbi_t *OpenVBI (demux_t *, const char *);
int GetFdVBI (heimdall_v4l2_vbi_t *);
void GrabVBI (demux_t *p_demux, heimdall_v4l2_vbi_t *);
void CloseVBI (heimdall_v4l2_vbi_t *);
#endif

/* demux.c */
int DemuxOpen(heimdall_object_t *);
void DemuxClose(heimdall_object_t *);
float GetAbsoluteMaxFrameRate(heimdall_object_t *, int fd, uint32_t fmt);
void GetMaxDimensions(heimdall_object_t *, int fd, uint32_t fmt, float fps_min,
                      uint32_t *pwidth, uint32_t *pheight);

/* access.c */
int AccessOpen(heimdall_object_t *);
void AccessClose(heimdall_object_t *);

/* radio.c */
int RadioOpen(heimdall_object_t *);
void RadioClose(heimdall_object_t *);

/* controls.c */
heimdall_v4l2_ctrl_t *ControlsInit(heimdall_object_t *, int fd);
void ControlsDeinit(heimdall_object_t *, heimdall_v4l2_ctrl_t *);
