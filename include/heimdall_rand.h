/*****************************************************************************
 * heimdall_rand.h: RNG
 *****************************************************************************
 * Copyright © 2007 Rémi Denis-Courmont
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

#ifndef HEIMDALL_RAND_H
# define HEIMDALL_RAND_H

/**
 * \file
 * This file defined random number generator function in heimdall
 */

HEIMDALL_API void heimdall_rand_bytes(void *buf, size_t len);

/* Interlocked (but not reproducible) functions for the POSIX PRNG */
HEIMDALL_API double heimdall_drand48(void) HEIMDALL_USED;
HEIMDALL_API long heimdall_lrand48(void) HEIMDALL_USED;
HEIMDALL_API long heimdall_mrand48(void) HEIMDALL_USED;

#endif
