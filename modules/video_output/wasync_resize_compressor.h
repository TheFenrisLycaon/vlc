/**
 * @file wasync_resize_compressor.h
 * @brief Windows asynchronous resize compressor helper
 */
/*****************************************************************************
 * Copyright © 2022 VideoLAN and VideoLAN Authors
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

#ifndef WASYNC_RESIZE_COMPRESSOR_H
#define WASYNC_RESIZE_COMPRESSOR_H 1

#include <heimdall_window.h>
#include <heimdall_threads.h>

typedef struct heimdall_wasync_resize_compressor {
    heimdall_timer_t timer;
    heimdall_mutex_t lock;
    heimdall_window_t * wnd;
    unsigned requested_width;
    unsigned requested_height;
    unsigned current_width;
    unsigned current_height;
    bool resizing;
} heimdall_wasync_resize_compressor_t;

static void wasync_resize_compressor_callback(void *data) {
    heimdall_wasync_resize_compressor_t * compressor = (heimdall_wasync_resize_compressor_t *) data;
    heimdall_mutex_lock(&compressor->lock);
    while (compressor->requested_width != compressor->current_width ||
           compressor->requested_height != compressor->current_height) {
        unsigned h = compressor->requested_height;
        unsigned w = compressor->requested_width;
        heimdall_mutex_unlock(&compressor->lock);

        heimdall_window_ReportSize(compressor->wnd, w, h);

        heimdall_mutex_lock(&compressor->lock);
        compressor->current_width = w;
        compressor->current_height = h;
    }
    compressor->resizing = false;
    heimdall_mutex_unlock(&compressor->lock);
}

static inline int heimdall_wasync_resize_compressor_init(heimdall_wasync_resize_compressor_t *compressor,
                                                     heimdall_window_t *window) {
    compressor->timer = NULL;
    heimdall_mutex_init(&compressor->lock);
    compressor->requested_width = 0;
    compressor->requested_height = 0;
    compressor->current_width = 0;
    compressor->current_height = 0;
    compressor->resizing = false;
    compressor->wnd = window;
    if (heimdall_timer_create(&compressor->timer, wasync_resize_compressor_callback,
                         compressor) != HEIMDALL_SUCCESS)
        return HEIMDALL_EGENERIC;
    return HEIMDALL_SUCCESS;
}

static inline void heimdall_wasync_resize_compressor_destroy(heimdall_wasync_resize_compressor_t *compressor) {
    if (compressor->timer != NULL)
        heimdall_timer_destroy(compressor->timer);
}

static inline void heimdall_wasync_resize_compressor_dropOrWait(heimdall_wasync_resize_compressor_t *compressor) {
    heimdall_timer_disarm(compressor->timer);
}

static inline void heimdall_wasync_resize_compressor_reportSize(heimdall_wasync_resize_compressor_t *compressor,
                                                           unsigned width, unsigned height) {
    heimdall_mutex_lock(&compressor->lock);
    compressor->requested_width = width;
    compressor->requested_height = height;
    if (compressor->resizing == false) {
        heimdall_timer_schedule_asap(compressor->timer, HEIMDALL_TIMER_FIRE_ONCE);
        compressor->resizing = true;
    }
    heimdall_mutex_unlock(&compressor->lock);
}

#endif /* WASYNC_RESIZE_COMPRESSOR_H */
