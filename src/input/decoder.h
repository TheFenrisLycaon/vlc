/*****************************************************************************
 * decoder.h: Input decoder functions
 *****************************************************************************
 * Copyright (C) 1998-2008 VLC authors and VideoLAN
 * Copyright (C) 2008 Laurent Aimar
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
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

#ifndef LIBHEIMDALL_INPUT_DECODER_H
#define LIBHEIMDALL_INPUT_DECODER_H 1

#include <heimdall_common.h>
#include <heimdall_decoder.h>
#include <heimdall_codec.h>
#include <heimdall_mouse.h>

#include "input_internal.h"

struct heimdall_clock_t;

struct heimdall_input_decoder_callbacks {
    /* notifications */
    void (*on_vout_started)(heimdall_input_decoder_t *decoder, vout_thread_t *vout,
                            enum heimdall_vout_order vout_order,
                            void *userdata);
    void (*on_vout_stopped)(heimdall_input_decoder_t *decoder, vout_thread_t *vout,
                            void *userdata);
    void (*on_output_paused)(heimdall_input_decoder_t *decoder, bool paused,
                             heimdall_tick_t pause_date,
                             void *userdata);

    void (*on_thumbnail_ready)(heimdall_input_decoder_t *decoder, picture_t *pic,
                               void *userdata);

    void (*on_new_video_stats)(heimdall_input_decoder_t *decoder, unsigned decoded,
                               unsigned lost, unsigned displayed, unsigned late,
                               void *userdata);
    void (*on_new_audio_stats)(heimdall_input_decoder_t *decoder, unsigned decoded,
                               unsigned lost, unsigned played, void *userdata);
    void (*frame_next_status)(heimdall_input_decoder_t *decoder, int status,
                              void *userdata);
    void (*frame_next_need_data)(heimdall_input_decoder_t *decoder, bool need_data,
                                 void *userdata);
    void (*frame_previous_status)(heimdall_input_decoder_t *decoder, int status,
                                  void *userdata);
    void (*frame_previous_seek)(heimdall_input_decoder_t *decoder, heimdall_tick_t pts,
                                unsigned frame_rate, unsigned frame_rate_base,
                                int steps, bool failed, void *userdata);
    /* requests */
    int (*get_attachments)(heimdall_input_decoder_t *decoder,
                           input_attachment_t ***ppp_attachment,
                           void *userdata);
};

struct heimdall_input_decoder_cfg
{
    const es_format_t *fmt;
    const char *str_id;
    struct heimdall_clock_t *clock;
    input_resource_t *resource;
    sout_stream_t *sout;
    enum input_type input_type;
    bool hw_dec;
    unsigned cc_decoder;
    const struct heimdall_input_decoder_callbacks *cbs;
    void *cbs_data;
};

heimdall_input_decoder_t *
heimdall_input_decoder_New( heimdall_object_t *parent,
                       const struct heimdall_input_decoder_cfg *cfg );

/**
 * This function changes the pause state.
 * The date parameter MUST hold the exact date at which the change has been
 * done for proper vout/aout pausing.
 */
void heimdall_input_decoder_ChangePause( heimdall_input_decoder_t *, bool b_paused, heimdall_tick_t i_date );

/**
 * Changes the decoder rate.
 *
 * This function changes rate of the intended playback speed to nominal speed.
 * \param dec decoder
 * \param rate playback rate (default is 1)
 */
void heimdall_input_decoder_ChangeRate( heimdall_input_decoder_t *dec, float rate );

/**
 * This function makes the decoder start waiting for a valid data block from its fifo.
 */
void heimdall_input_decoder_StartWait( heimdall_input_decoder_t * );

/**
 * This function waits for the decoder to actually receive data.
 */
void heimdall_input_decoder_Wait( heimdall_input_decoder_t * );

/**
 * This function exits the waiting mode of the decoder.
 */
void heimdall_input_decoder_StopWait( heimdall_input_decoder_t * );

/**
 * This function returns true if the decoder fifo is empty and false otherwise.
 */
bool heimdall_input_decoder_IsEmpty( heimdall_input_decoder_t * );

/**
 * This function forces the display of the next picture
 */
void heimdall_input_decoder_FrameNext( heimdall_input_decoder_t *p_dec );

/**
 * This function forces the display of the previous picture
 */
void heimdall_input_decoder_FramePrevious( heimdall_input_decoder_t *p_dec );

/*
 * This function reset the decoder for normal playback after a previous or a
 * next frame call.
 */
void heimdall_input_decoder_StopFrameNext( heimdall_input_decoder_t *p_dec );

/**
 * This function Creates and adds the requested SubDec.
 *
 * The sub decoder returned by this function must be deleted with
 * heimdall_input_decoder_Delete() before the parent is deleted.
 */
heimdall_input_decoder_t *
heimdall_input_decoder_CreateSubDec(heimdall_input_decoder_t *dec,
                               const struct heimdall_input_decoder_cfg *cfg);

struct heimdall_subdec_desc
{
    es_format_t *fmt_array;
    size_t fmt_count;
};

void heimdall_subdec_desc_Clean(struct heimdall_subdec_desc *desc);


struct heimdall_input_decoder_status
{
    struct {
        /* True if the ES format or meta data have changed since the last call.
         * */
        bool changed;
        /* If changed is true, a copy of the current es_format_t, MUST be freed
         * with es_format_Clean() */
        es_format_t fmt;
        /* If changed is true, a copy of the current description, can be NULL,
         * MUST be freed with heimdall_meta_Delete.() */
        heimdall_meta_t *meta;
    } format;

    struct heimdall_subdec_desc subdec_desc;
};

/**
 * Get the last status of the decoder.
 */
void heimdall_input_decoder_DecodeWithStatus(heimdall_input_decoder_t *p_dec,
                                        heimdall_frame_t *frame, bool do_pace,
                                        struct heimdall_input_decoder_status *status);

/**
 * This function returns the current size in bytes of the decoder fifo
 */
size_t heimdall_input_decoder_GetFifoSize( heimdall_input_decoder_t *p_dec );

int heimdall_input_decoder_GetVbiPage( heimdall_input_decoder_t *, bool *opaque );
int heimdall_input_decoder_SetVbiPage( heimdall_input_decoder_t *, unsigned page );
int heimdall_input_decoder_SetVbiOpaque( heimdall_input_decoder_t *, bool opaque );

void heimdall_input_decoder_SetVoutMouseEvent( heimdall_input_decoder_t *, heimdall_mouse_event, void * );
int  heimdall_input_decoder_AddVoutOverlay( heimdall_input_decoder_t *, subpicture_t *, size_t * );
int  heimdall_input_decoder_DelVoutOverlay( heimdall_input_decoder_t *, size_t );

#endif
