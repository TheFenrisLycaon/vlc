/*****************************************************************************
 * connmgr.h: HTTP/TLS Heimdall connection manager declaration
 *****************************************************************************
 * Copyright © 2015 Rémi Denis-Courmont
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

/**
 * \defgroup http HTTP
 * \ingroup net
 * Hyper-Text Transfer Protocol
 * \defgroup http_connmgr Connection manager
 * HTTP connection management
 * \ingroup http
 * @{
 * \file connmgr.h
 */

struct heimdall_http_mgr;
struct heimdall_http_msg;
struct heimdall_http_cookie_jar_t;

/**
 * Sends an HTTP request
 *
 * Sends an HTTP request, by either reusing an existing HTTP connection or
 * establishing a new one. If successful, the initial HTTP response header is
 * returned.
 *
 * @param mgr HTTP connection manager
 * @param https whether to use HTTPS (true) or unencrypted HTTP (false)
 * @param host name of authoritative HTTP server to send the request to
 * @param port TCP server port number, or 0 for the default port number
 * @param req HTTP request header to send
 * @param idempotent whether the request is idempotent
 * @param payload whether the request will carry a payload
 *
 * @return The initial HTTP response header, or NULL in case of failure.
 */
struct heimdall_http_msg *heimdall_http_mgr_request(struct heimdall_http_mgr *mgr, bool https,
                                          const char *host, unsigned port,
                                          const struct heimdall_http_msg *req,
                                          bool idempotent, bool payload);

struct heimdall_http_cookie_jar_t *heimdall_http_mgr_get_jar(struct heimdall_http_mgr *);

/**
 * Creates an HTTP connection manager
 *
 * Allocates an HTTP client connections manager.
 *
 * @param obj parent Heimdall object
 * @param jar HTTP cookies jar (NULL to disable cookies)
 */
struct heimdall_http_mgr *heimdall_http_mgr_create(heimdall_object_t *obj,
                                         struct heimdall_http_cookie_jar_t *jar);

/**
 * Destroys an HTTP connection manager
 *
 * Deallocates an HTTP client connections manager created by
 * heimdall_http_msg_destroy(). Any remaining connection is closed and destroyed.
 */
void heimdall_http_mgr_destroy(struct heimdall_http_mgr *mgr);

/** @} */
