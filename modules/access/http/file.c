/*****************************************************************************
 * file.c: HTTP read-only file
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

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <heimdall_common.h>
#include <heimdall_block.h>
#include <heimdall_strings.h>
#include "message.h"
#include "resource.h"
#include "file.h"

#pragma GCC visibility push(default)

struct heimdall_http_file
{
    struct heimdall_http_resource resource;
    uintmax_t offset;
};

static int heimdall_http_file_req(const struct heimdall_http_resource *res,
                             struct heimdall_http_msg *req, void *opaque)
{
    struct heimdall_http_file *file = (struct heimdall_http_file *)res;
    const uintmax_t *offset = opaque;

    if (file->resource.response != NULL)
    {
        const char *str = heimdall_http_msg_get_header(file->resource.response,
                                                  "ETag");
        if (str != NULL)
        {
            if (!memcmp(str, "W/", 2))
                str += 2; /* skip weak mark */
            heimdall_http_msg_add_header(req, "If-Match", "%s", str);
        }
        else
        {
            time_t mtime = heimdall_http_msg_get_mtime(file->resource.response);
            if (mtime != -1)
                heimdall_http_msg_add_time(req, "If-Unmodified-Since", &mtime);
        }
    }

    if (heimdall_http_msg_add_header(req, "Range", "bytes=%" PRIuMAX "-", *offset)
     && *offset != 0)
        return -1;
    return 0;
}

static int heimdall_http_file_resp(const struct heimdall_http_resource *res,
                              const struct heimdall_http_msg *resp, void *opaque)
{
    const uintmax_t *offset = opaque;

    if (heimdall_http_msg_get_status(resp) == 206)
    {
        const char *str = heimdall_http_msg_get_header(resp, "Content-Range");
        if (str == NULL)
            /* A multipart/byteranges response. This is not what we asked for
             * and we do not support it. */
            goto fail;

        uintmax_t start, end;
        if (sscanf(str, "bytes %" SCNuMAX "-%" SCNuMAX, &start, &end) != 2
         || start != *offset || start > end)
            /* A single range response is what we asked for, but not at that
             * start offset. */
            goto fail;
    }

    (void) res;
    return 0;

fail:
    errno = EIO;
    return -1;
}

static const struct heimdall_http_resource_cbs heimdall_http_file_callbacks =
{
    heimdall_http_file_req,
    heimdall_http_file_resp,
};

struct heimdall_http_resource *heimdall_http_file_create(struct heimdall_http_mgr *mgr,
                                               const char *uri, const char *ua,
                                               const char *ref)
{
    struct heimdall_http_file *file = malloc(sizeof (*file));
    if (unlikely(file == NULL))
        return NULL;

    if (heimdall_http_res_init(&file->resource, &heimdall_http_file_callbacks, mgr,
                          uri, ua, ref))
    {
        free(file);
        return NULL;
    }

    file->offset = 0;
    return &file->resource;
}

static uintmax_t heimdall_http_msg_get_file_size(const struct heimdall_http_msg *resp)
{
    int status = heimdall_http_msg_get_status(resp);
    const char *range = heimdall_http_msg_get_header(resp, "Content-Range");

    if (status == 206 /* Partial Content */)
    {   /* IETF RFC7233 §4.1 */
        assert(range != NULL); /* checked by heimdall_http_file_resp() */

        uintmax_t end, total;

        switch (sscanf(range, "bytes %*u-%" SCNuMAX "/%" SCNuMAX, &end, &total))
        {
            case 1:
                if (unlikely(end == UINTMAX_MAX))
                    return -1; /* avoid wrapping to zero */
                return end + 1;
            case 2:
                return total;
        }
        heimdall_assert_unreachable(); /* checked by heimdall_http_file_resp() */
    }

    if (status == 416 /* Range Not Satisfiable */)
    {   /* IETF RFC7233 §4.4 */
        uintmax_t total;

        if (range == NULL)
            return -1; /* valid but helpless response */

        if (sscanf(range, "bytes */%" SCNuMAX, &total) == 1)
            return total; /* this occurs when seeking beyond EOF */
    }

    return -1;
}

static bool heimdall_http_msg_can_seek(const struct heimdall_http_msg *resp)
{
    int status = heimdall_http_msg_get_status(resp);
    if (status == 206 || status == 416)
        return true; /* Partial Content */

    return heimdall_http_msg_get_token(resp, "Accept-Ranges", "bytes") != NULL;
}

uintmax_t heimdall_http_file_get_size(struct heimdall_http_resource *res)
{
    int status = heimdall_http_res_get_status(res);
    if (status < 0)
        return -1;

    uintmax_t ret = heimdall_http_msg_get_file_size(res->response);
    if (ret != (uintmax_t)-1)
        return ret;

    if (status >= 300 || status == 201)
        return -1; /* Error or redirection, size is unknown/irrelevant. */

    /* Content-Range is meaningless here (see RFC7233 B), so check if the size
     * of the response entity body is known. */
    return heimdall_http_msg_get_size(res->response);
}

bool heimdall_http_file_can_seek(struct heimdall_http_resource *res)
{   /* See IETF RFC7233 */
    int status = heimdall_http_res_get_status(res);
    if (status < 0)
        return false;
    return heimdall_http_msg_can_seek(res->response);
}

int heimdall_http_file_seek(struct heimdall_http_resource *res, uintmax_t offset)
{
    struct heimdall_http_msg *resp = heimdall_http_res_open(res, &offset);
    if (resp == NULL)
        return -1;

    struct heimdall_http_file *file = (struct heimdall_http_file *)res;

    int status = heimdall_http_msg_get_status(resp);
    if (res->response != NULL)
    {   /* Accept the new and ditch the old one if:
         * - requested succeeded and range was accepted (206),
         * - requested failed due to out-of-range (416),
         * - request succeeded and seek offset is zero (2xx).
         */
        if (status != 206 && status != 416 && (offset != 0 || status >= 300))
        {
            heimdall_http_msg_destroy(resp);
            return -1;
        }
        heimdall_http_msg_destroy(res->response);
    }

    res->response = resp;
    file->offset = offset;
    return 0;
}

block_t *heimdall_http_file_read(struct heimdall_http_resource *res)
{
    struct heimdall_http_file *file = (struct heimdall_http_file *)res;
    block_t *block = heimdall_http_res_read(res);

    if (block == heimdall_http_error)
        block = NULL;

    /* Automatically resume on short response or error if possible */
    if (block == NULL && res->response != NULL
     && heimdall_http_msg_can_seek(res->response)
     && file->offset < heimdall_http_msg_get_file_size(res->response)
     && heimdall_http_file_seek(res, file->offset) == 0)
    {
        block = heimdall_http_res_read(res);

        if (block == heimdall_http_error)
            block = NULL; /* Non-recovered error */
    }

    if (block != NULL)
        file->offset += block->i_buffer;
    return block;
}
