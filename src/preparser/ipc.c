// SPDX-License-Identifier: LGPL-2.1-or-later
/*****************************************************************************
 * ipc.h: definition of heimdall_preparser_ipc functions
 *****************************************************************************
 * Copyright © 2025 Videolabs, VideoLAN and VLC authors
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Gabriel Lafond Thenaille <gabriel@videolabs.io>
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#include <heimdall_common.h>
#include <heimdall_modules.h>
#include <heimdall_preparser_ipc.h>
#include <heimdall_input_item.h>
#include <heimdall_list.h>
#include <heimdall_vector.h>
#include <heimdall_atomic.h>
#include <heimdall_preparser.h>
#include <heimdall_threads.h>
#include <heimdall_memstream.h>
#include <heimdall_poll.h>
#include <heimdall_interrupt.h>

struct heimdall_preparser_msg_serdes *
heimdall_preparser_msg_serdes_Create(heimdall_object_t *obj,
                                const struct heimdall_preparser_msg_serdes_cbs *cbs,
                                bool bin_data)
{
    assert(cbs != NULL);

    struct heimdall_preparser_msg_serdes *msg_serdes = malloc(sizeof(*msg_serdes));
    if (msg_serdes == NULL) {
        return NULL;
    }
    msg_serdes->owner.cbs = cbs;

    module_t **mods = NULL;
    size_t strict = 0;
    ssize_t n = heimdall_module_match("preparser msg serdes", "any", true, &mods,
                                 &strict);

    for (ssize_t i = 0; i < n; i++) {
        heimdall_preparser_msg_serdes_module cb = heimdall_module_map(obj->logger,
                                                            mods[i]);
        if (cb == NULL) {
            continue;
        }

        int ret = cb(msg_serdes, bin_data);
        if (ret == HEIMDALL_SUCCESS) {
            free(mods);
            return msg_serdes;
        }
    }
    free(mods);
    free(msg_serdes);
    return NULL;
}

/*****************************************************************************
 *
 *****************************************************************************/

void
heimdall_preparser_msg_Init(struct heimdall_preparser_msg *msg, int msg_type,
                       enum heimdall_preparser_msg_req_type req_type)
{
    assert(msg != NULL);
    assert(msg_type == HEIMDALL_PREPARSER_MSG_TYPE_REQ ||
           msg_type == HEIMDALL_PREPARSER_MSG_TYPE_RES);
    assert(req_type == HEIMDALL_PREPARSER_MSG_REQ_TYPE_PARSE ||
           req_type == HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL ||
           req_type == HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL_TO_FILES);

    msg->type = msg_type;
    msg->req_type = req_type;
    if (msg->type == HEIMDALL_PREPARSER_MSG_TYPE_REQ) {
        msg->req.type = req_type;
        msg->req.options = 0;
        msg->req.arg.seek.type = HEIMDALL_THUMBNAILER_SEEK_NONE;
        msg->req.arg.seek.speed = HEIMDALL_THUMBNAILER_SEEK_PRECISE;
        msg->req.arg.hw_dec = false;
        heimdall_vector_init(&msg->req.outputs);
        heimdall_vector_init(&msg->req.outputs_path);
        msg->req.uri = NULL;
    } else {
        msg->res.type = req_type;
        heimdall_vector_init(&msg->res.attachments);
        msg->res.subtree = NULL;
        msg->res.pic = NULL;
        heimdall_vector_init(&msg->res.result);
        msg->res.status = -1;
        msg->res.item = NULL;
    }
}

void
heimdall_preparser_msg_Clean(struct heimdall_preparser_msg *msg)
{
    assert(msg != NULL);

    if (msg->type == HEIMDALL_PREPARSER_MSG_TYPE_REQ) {
        heimdall_vector_clear(&msg->req.outputs);
        char *ptr = NULL;
        heimdall_vector_foreach(ptr, &msg->req.outputs_path) {
            free(ptr);
        }
        heimdall_vector_clear(&msg->req.outputs_path);
        if (msg->req.uri != NULL) {
            free(msg->req.uri);
            msg->req.uri = NULL;
        }
    } else {
        input_attachment_t *att = NULL;
        heimdall_vector_foreach(att, &msg->res.attachments) {
            heimdall_input_attachment_Release(att);
        }
        heimdall_vector_clear(&msg->res.attachments);
        if (msg->res.subtree != NULL) {
            input_item_node_Delete(msg->res.subtree);
            msg->res.subtree = NULL;
        }
        if (msg->res.pic != NULL) {
            picture_Release(msg->res.pic);
            msg->res.pic = NULL;
        }
        heimdall_vector_clear(&msg->res.result);
        if (msg->res.item != NULL) {
            input_item_Release(msg->res.item);
            msg->res.item = NULL;
        }
    }
}
