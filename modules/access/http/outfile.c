/*****************************************************************************
 * resource.c: HTTP resource common code
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <heimdall_common.h>
#include <heimdall_url.h>
#include <heimdall_strings.h>
#include "message.h"
#include "connmgr.h"
#include "outfile.h"

struct heimdall_http_outfile *heimdall_http_outfile_create(struct heimdall_http_mgr *mgr,
    const char *uri, const char *ua, const char *user, const char *pwd)
{
    struct heimdall_http_msg *resp = NULL;
    heimdall_url_t url;
    bool secure;
    char* url_path_with_options = NULL;

    if (heimdall_UrlParse(&url, uri))
        goto error;
    if (url.psz_protocol == NULL || url.psz_host == NULL)
    {
        errno = EINVAL;
        goto error;
    }

    if (!heimdall_ascii_strcasecmp(url.psz_protocol, "https"))
        secure = true;
    else if (!heimdall_ascii_strcasecmp(url.psz_protocol, "http"))
        secure = false;
    else
    {
        errno = ENOTSUP;
        goto error;
    }

    char *authority = heimdall_http_authority(url.psz_host, url.i_port);
    if (unlikely(authority == NULL))
        goto error;

    if (url.psz_option)
    {
        if (asprintf(&url_path_with_options, "%s?%s", url.psz_path, url.psz_option) < 0)
        {
            free(authority);
            goto error;
        }
    }

    struct heimdall_http_msg *req = heimdall_http_req_create("PUT", url.psz_protocol,
                                                   authority, url_path_with_options ? url_path_with_options : url.psz_path);
    free(authority);
    free(url_path_with_options);
    if (unlikely(req == NULL))
        goto error;

    heimdall_http_msg_add_header(req, "Expect", "100-continue");

    if (user != NULL && pwd != NULL)
        heimdall_http_msg_add_creds_basic(req, false, user, pwd);
    if (ua != NULL)
        heimdall_http_msg_add_agent(req, ua);

    heimdall_http_msg_add_cookies(req, heimdall_http_mgr_get_jar(mgr));

    resp = heimdall_http_mgr_request(mgr, secure, url.psz_host, url.i_port, req,
                                false, true);
    heimdall_http_msg_destroy(req);
    if (resp == NULL)
        goto error;

    int status = heimdall_http_msg_get_status(resp);

    /* FIXME: check that HTTP version >= 1.1 */

    if (status < 100 || status >= 200)
    {
        heimdall_http_msg_destroy(resp);
        resp = NULL;
    }

error:
    heimdall_UrlClean(&url);
    return (struct heimdall_http_outfile *)resp;
}

ssize_t heimdall_http_outfile_write(struct heimdall_http_outfile *f, block_t *b)
{
    struct heimdall_http_msg *msg = (struct heimdall_http_msg *)f;

    return heimdall_http_msg_write(msg, b, false);
}

int heimdall_http_outfile_close(struct heimdall_http_outfile *f)
{
    struct heimdall_http_msg *msg = (struct heimdall_http_msg *)f;
    int ret = heimdall_http_msg_write(msg, NULL, true);

    if (ret < 0)
    {
        heimdall_http_msg_destroy(msg);
        return -1;
    }

    msg = heimdall_http_msg_iterate(msg);
    if (msg == NULL)
        return -1;

    int status = heimdall_http_msg_get_status(msg);

    /* TODO: store cookies? */
    heimdall_http_msg_destroy(msg);

    return (status >= 200 && status < 300) ? 0 : -1;
}
