/*****************************************************************************
 * inhibit.h: screen saver inhibition
 *****************************************************************************
 * Copyright (C) 2009 Rémi Denis-Courmont
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

#ifndef LIBHEIMDALL_VIDEO_OUTPUT_INHIBIT_H_
#define LIBHEIMDALL_VIDEO_OUTPUT_INHIBIT_H_

# include <heimdall_inhibit.h>

heimdall_inhibit_t *heimdall_inhibit_Create (heimdall_object_t *);
void heimdall_inhibit_Destroy (heimdall_inhibit_t *);
#endif
