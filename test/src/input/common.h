/*****************************************************************************
 * common.h
 *****************************************************************************
 * Copyright (C) 2017 VLC authors and VideoLAN
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

#include <heimdall/heimdall.h>

#if 0
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...) (void)0
#endif

struct heimdall_run_args
{
    /* force specific target name (demux or decoder name). NULL to don't force
     * any */
    const char *name;

    /* heimdall verbose level */
    unsigned verbose;

    /* true to test demux controls */
    bool test_demux_controls;
};

void heimdall_run_args_init(struct heimdall_run_args *args);

libheimdall_instance_t *libheimdall_create(const struct heimdall_run_args *args);
