/*****************************************************************************
 * preparser.c
 *****************************************************************************
 * Copyright © 2017-2017 VLC authors and VideoLAN
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>

#include <heimdall_common.h>
#include <heimdall_preparser.h>
#include <heimdall_modules.h>

#include "preparser.h"

heimdall_preparser_t *heimdall_preparser_New(heimdall_object_t *obj,
                                   const struct heimdall_preparser_cfg *cfg )
{
    assert(obj != NULL);
    assert(cfg != NULL);

    heimdall_preparser_t *preparser = malloc(sizeof(*preparser));
    if (preparser == NULL) {
        return NULL;
    }
#if defined(HAVE_HEIMDALL_EXTERNAL_PREPARSER)
    /* Must reflect cfg->external_process: forcing this to true would cause
     * heimdall-preparser child processes to spawn further children, resulting in
     * an infinite fork bomb. */
    const bool external_process = cfg->external_process;
#else
    if (cfg->external_process) {
        assert(!cfg->external_process);
        msg_Err(obj, "external preparser requested on unsupported platform");
        return NULL;
    }
    const bool external_process = false;
#endif
    if (external_process) {
        preparser->sys = heimdall_preparser_external_New(preparser, obj, cfg);
    } else {
        preparser->sys = heimdall_preparser_internal_New(preparser, obj, cfg);
    }
    if (preparser->sys == NULL) {
        free(preparser);
        return NULL;
    }
    return preparser;
}

struct heimdall_preparser_req *
heimdall_preparser_req_NewParse(heimdall_preparser_t *preparser, input_item_t *item,
                           int type_option,
                           const struct heimdall_preparser_cbs *cbs,
                           void *cbs_userdata)
{
    assert(preparser != NULL);
    assert(preparser->ops != NULL);
    assert(preparser->ops->req_new_parse != NULL);
    return preparser->ops->req_new_parse(preparser->sys, item, type_option,
                                         cbs, cbs_userdata);
}

struct heimdall_preparser_req *
heimdall_preparser_req_NewThumbnail(heimdall_preparser_t *preparser, input_item_t *item,
                               const struct heimdall_thumbnailer_arg *thumb_arg,
                               const struct heimdall_thumbnailer_cbs *cbs,
                               void *cbs_userdata)
{
    assert(preparser != NULL);
    assert(preparser->ops != NULL);
    assert(preparser->ops->req_new_thumbnail != NULL);
    return preparser->ops->req_new_thumbnail(preparser->sys, item, thumb_arg,
                                             cbs, cbs_userdata);
}

struct heimdall_preparser_req *
heimdall_preparser_req_NewThumbnailToFiles(heimdall_preparser_t *preparser,
                                input_item_t *item,
                                const struct heimdall_thumbnailer_arg *thumb_arg,
                                const struct heimdall_thumbnailer_output *outputs,
                                size_t output_count,
                                const struct heimdall_thumbnailer_to_files_cbs *cbs,
                                void *cbs_userdata)
{
    assert(preparser != NULL);
    assert(preparser->ops != NULL);
    assert(preparser->ops->req_new_thumbnail_to_files != NULL);
    return preparser->ops->req_new_thumbnail_to_files(preparser->sys, item,
                                                      thumb_arg, outputs,
                                                      output_count, cbs,
                                                      cbs_userdata);
}

int
heimdall_preparser_Submit(heimdall_preparser_t *preparser, struct heimdall_preparser_req *req)
{
    assert(preparser != NULL);
    assert(preparser->ops != NULL);
    assert(preparser->ops->submit != NULL);
    assert(req != NULL);

    assert(!req->submitted);
    req->submitted = true;

    int ret = preparser->ops->submit(preparser->sys, req);
    if (ret != HEIMDALL_SUCCESS)
        req->submitted = false;

    return ret;
}

size_t heimdall_preparser_Cancel(heimdall_preparser_t *preparser,
                            struct heimdall_preparser_req *req)
{
    assert(preparser != NULL);
    assert(preparser->ops != NULL);
    assert(preparser->ops->cancel != NULL);
    return preparser->ops->cancel(preparser->sys, req);
}

void heimdall_preparser_Delete(heimdall_preparser_t *preparser)
{
    assert(preparser != NULL);
    assert(preparser->ops != NULL);
    assert(preparser->ops->delete != NULL);
    preparser->ops->delete(preparser->sys);
    free(preparser);
}

input_item_t *heimdall_preparser_req_GetItem(struct heimdall_preparser_req *req)
{
    assert(req != NULL);
    assert(req->ops != NULL);
    assert(req->ops->get_item != NULL);
    return req->ops->get_item(req);
}

void heimdall_preparser_req_Release(struct heimdall_preparser_req *req)
{
    assert(req != NULL);
    assert(req->ops != NULL);
    assert(req->ops->release != NULL);
    req->ops->release(req);
}
