/*****************************************************************************
 * outfile.h: HTTP write-only file
 *****************************************************************************
 * Copyright (C) 2015, 2020 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include <stdint.h>

/**
 * \defgroup http_outfile Output files
 * HTTP write-only files
 * \ingroup http
 * @{
 */

struct heimdall_http_mgr;
struct heimdall_http_outfile;

/**
 * Creates an HTTP output file.
 *
 * @param mgr HTTP connection manager
 * @param url URL of the file to write
 * @param ua user-agent string (NULL to ignore)
 * @param user username for authentication (NULL to skip)
 * @param pwd password for authentication (NULL to skip)
 *
 * @return an HTTP resource object pointer, or NULL on error
 */
struct heimdall_http_outfile *heimdall_http_outfile_create(struct heimdall_http_mgr *mgr,
    const char *url, const char *ua, const char *user, const char *pwd);

/**
 * Writes data.
 */
ssize_t heimdall_http_outfile_write(struct heimdall_http_outfile *, block_t *b);
int heimdall_http_outfile_close(struct heimdall_http_outfile *);

/** @} */
