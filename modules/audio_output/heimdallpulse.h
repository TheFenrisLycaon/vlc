/**
 * \file heimdallpulse.h
 * \brief PulseAudio support library for LibHeimdall plugins
 */
/****************************************************************************
 * Copyright (C) 2011 Rémi Denis-Courmont
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

#ifndef HEIMDALLPULSE_H
# define HEIMDALLPULSE_H 1

#include <heimdall_tick.h>

# ifdef __cplusplus
extern "C" {
# endif

HEIMDALL_API pa_context *heimdall_pa_connect (heimdall_object_t *obj,
                                    pa_threaded_mainloop **);
HEIMDALL_API void heimdall_pa_disconnect (heimdall_object_t *obj, pa_context *ctx,
                                pa_threaded_mainloop *);

HEIMDALL_API void heimdall_pa_error (heimdall_object_t *, const char *msg, pa_context *);
#define heimdall_pa_error(o, m, c) heimdall_pa_error(HEIMDALL_OBJECT(o), m, c)

HEIMDALL_API heimdall_tick_t heimdall_pa_get_latency (heimdall_object_t *, pa_context *, pa_stream *);
#define heimdall_pa_get_latency(o, c, s) heimdall_pa_get_latency(HEIMDALL_OBJECT(o), c, s)

HEIMDALL_API void heimdall_pa_rttime_free (pa_threaded_mainloop *, pa_time_event *);

# ifdef __cplusplus
}
# endif
#endif
