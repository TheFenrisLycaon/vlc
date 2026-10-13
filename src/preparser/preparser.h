// SPDX-License-Identifier: LGPL-2.1-or-later
/*****************************************************************************
 * preparser.h: internal header for preparser
 *****************************************************************************
 * Copyright © 2025 Videolabs, VideoLAN and VLC authors
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Gabriel Lafond Thenaille <gabriel@videolabs.io>
 *****************************************************************************/

#ifndef PREPARSER_INTERNAL_H
#define PREPARSER_INTERNAL_H 1

#include <heimdall_common.h>
#include <heimdall_preparser.h>
#include <heimdall_modules.h>

/**
 * Preparser's operations
 */
struct heimdall_preparser_operations {
    /** Called by `heimdall_preparser_req_NewParse`. */
    struct heimdall_preparser_req *(*req_new_parse)
                                  (void *opaque, input_item_t *item,
                                   int type_options,
                                   const struct heimdall_preparser_cbs *cbs,
                                   void *cbs_userdata);

    /** Called by `heimdall_preparser_req_NewThumbnail`. */
    struct heimdall_preparser_req *(*req_new_thumbnail)
                                  (void *opaque, input_item_t *item,
                                   const struct heimdall_thumbnailer_arg *thumb_arg,
                                   const struct heimdall_thumbnailer_cbs *cbs,
                                   void *cbs_userdata);

    /** Called by `heimdall_preparser_req_NewThumbnailToFiles`. */
    struct heimdall_preparser_req *(*req_new_thumbnail_to_files)
                                  (void *opaque, input_item_t *item,
                                   const struct heimdall_thumbnailer_arg *thumb_arg,
                                   const struct heimdall_thumbnailer_output *outputs,
                                   size_t output_count,
                                   const struct heimdall_thumbnailer_to_files_cbs *cbs,
                                   void *cbs_userdata);

    /** Called by `heimdall_preparser_Submit`. */
    int (*submit)(void *opaque, struct heimdall_preparser_req *req);

    /** Called by `heimdall_preparser_Cancel`. */
    size_t (*cancel)(void *opaque, struct heimdall_preparser_req *req);

    /** Called by `heimdall_preparser_Delete`. */
    void (*delete)(void *opaque);
};

struct heimdall_preparser_t {
    const struct heimdall_preparser_operations *ops;
    void *sys;
};

void *heimdall_preparser_internal_New(heimdall_preparser_t *preparser,
                                 heimdall_object_t *parent,
                                 const struct heimdall_preparser_cfg *cfg);

void *heimdall_preparser_external_New(heimdall_preparser_t *preparser,
                                 heimdall_object_t *parent,
                                 const struct heimdall_preparser_cfg *cfg);

/* Preparser Request */
struct heimdall_preparser_req_operations {
    /* Called by `heimdall_preparser_req_GetItem`. */
    input_item_t *(*get_item)(struct heimdall_preparser_req *req);

    /* Called by `heimdall_preparser_req_Release`. */
    void (*release)(struct heimdall_preparser_req *req);
};

struct heimdall_preparser_req {
    const struct heimdall_preparser_req_operations *ops;

    /* re-submission guard for debug builds */
    bool submitted;
};

#endif /* PREPARSER_INTERNAL_H */
