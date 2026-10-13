/*****************************************************************************
 * resource.h: HTTP resource common code
 *****************************************************************************
 * Copyright (C) 2015 Rémi Denis-Courmont
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

#ifndef HEIMDALL_HTTP_RESOURCE_H
#define HEIMDALL_HTTP_RESOURCE_H 1

/**
 * \defgroup http_res Resources
 * Remote HTTP resources identified by a URL
 * \ingroup http
 * @{
 */

struct heimdall_http_msg;
struct heimdall_http_mgr;
struct heimdall_http_resource;

struct heimdall_http_resource_cbs
{
    int (*request_format)(const struct heimdall_http_resource *,
                          struct heimdall_http_msg *, void *);
    int (*response_validate)(const struct heimdall_http_resource *,
                             const struct heimdall_http_msg *, void *);
};

struct heimdall_http_resource
{
    const struct heimdall_http_resource_cbs *cbs;
    struct heimdall_http_msg *response;
    struct heimdall_http_mgr *manager;
    bool secure;
    bool negotiate;
    bool failure;
    char *host;
    unsigned port;
    char *authority;
    char *path;
    char *username;
    char *password;
    char *agent;
    char *referrer;
};

int heimdall_http_res_init(struct heimdall_http_resource *,
                      const struct heimdall_http_resource_cbs *cbs,
                      struct heimdall_http_mgr *mgr,
                      const char *uri, const char *ua, const char *ref);

/**
 * Destroys an HTTP resource.
 *
 * Releases all underlying resources allocated or held by the HTTP resource
 * object.
 */
void heimdall_http_res_destroy(struct heimdall_http_resource *);

struct heimdall_http_msg *heimdall_http_res_open(struct heimdall_http_resource *res, void *);
int heimdall_http_res_get_status(struct heimdall_http_resource *res);

/**
 * Gets redirection URL.
 *
 * Checks if the resource URL lead to a redirection. If so, return the redirect
 * location.
 *
 * @return Heap-allocated URL or NULL if no redirection.
 */
char *heimdall_http_res_get_redirect(struct heimdall_http_resource *);

/**
 * Gets MIME type.
 *
 * @return Heap-allocated MIME type string, or NULL if unknown.
 */
char *heimdall_http_res_get_type(struct heimdall_http_resource *);

/**
 * Reads data.
 */
block_t *heimdall_http_res_read(struct heimdall_http_resource *);

int heimdall_http_res_set_login(struct heimdall_http_resource *res,
                           const char *username, const char *password);
char *heimdall_http_res_get_basic_realm(struct heimdall_http_resource *res);

/** @} */
#endif
