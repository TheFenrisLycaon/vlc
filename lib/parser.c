/*****************************************************************************
 * parser.c: Libheimdall parser API
 *****************************************************************************
 * Copyright (C) 2025 VLC authors and VideoLAN
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
#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_media.h>
#include <heimdall/libheimdall_parser.h>
#include <heimdall/libheimdall_picture.h>

#include <heimdall_common.h>
#include <heimdall_atomic.h>
#include <heimdall_preparser.h>

#include "../src/libheimdall.h"
#include "libheimdall_internal.h"
#include "media_internal.h"
#include "picture_internal.h"

static_assert(HEIMDALL_THUMBNAILER_SEEK_NONE == (int) libheimdall_thumbnailer_seek_none &&
              HEIMDALL_THUMBNAILER_SEEK_TIME == (int) libheimdall_thumbnailer_seek_time &&
              HEIMDALL_THUMBNAILER_SEEK_POS  == (int) libheimdall_thumbnailer_seek_pos,
              "lib/heimdall_thumbnailer seek type mismatch");

static_assert(HEIMDALL_THUMBNAILER_SEEK_PRECISE == (int) libheimdall_media_thumbnail_seek_precise &&
              HEIMDALL_THUMBNAILER_SEEK_FAST    == (int) libheimdall_media_thumbnail_seek_fast,
              "lib/heimdall_thumbnailer seek speed mismatch");

struct libheimdall_parser_t
{
    libheimdall_instance_t *libheimdall;
    heimdall_preparser_t *preparser;
};

union libheimdall_parser_cbs_internal
{
    const struct libheimdall_parser_cbs *parser;
    const struct libheimdall_thumbnailer_cbs *thumbnailer;
};

struct libheimdall_parser_task
{
    /* LibHeimdall instance */
    libheimdall_instance_t *instance;

    /* media to parse/thumbnail */
    libheimdall_media_t *media;

    /* Dimensions (for thumbnailing) */
    unsigned int width;
    unsigned int height;

    /* True to enable crop (false by default) (for thumbnailing) */
    bool crop;

    /* Picture type (for thumbnailing) */
    libheimdall_picture_type_t type;

    /* parser/thumbnailer callbacks */
    union libheimdall_parser_cbs_internal cbs;

    /* opaque data for cbs */
    void *cbs_opaque;

    /* preparser request handle */
    heimdall_preparser_req *preparser_req;

    /* task reference count */
    heimdall_atomic_rc_t rc;
};

/* Internal function to copy the required members of the request object into a heap-allocated
   structure for further processing. The object is allocated on the heap to ensure it remains
   valid for the entire duration of the parsing or thumbnailing operation. */
static struct libheimdall_parser_task *
libheimdall_parser_task_new(libheimdall_media_t *media,
                       const libheimdall_thumbnailer_request_t *thumbnailer_req,
                       union libheimdall_parser_cbs_internal cbs,
                       void *cbs_opaque)
{
    struct libheimdall_parser_task *task = malloc(sizeof(*task));
    if (task == NULL)
        return NULL;

    task->instance = NULL;
    task->media = libheimdall_media_retain(media);
    task->cbs = cbs;
    task->cbs_opaque = cbs_opaque;
    task->preparser_req = NULL;
    heimdall_atomic_rc_init(&task->rc);
    if (thumbnailer_req != NULL)
    {
        task->width = thumbnailer_req->width;
        task->height = thumbnailer_req->height;
        task->crop = thumbnailer_req->crop;
        task->type = thumbnailer_req->type;
    }
    return task;
}

/* Internal function to destroy the heap allocated task object */
static void libheimdall_parser_task_destroy(struct libheimdall_parser_task *task)
{
    if (task->instance != NULL)
        libheimdall_release(task->instance);

    if (task->preparser_req != NULL)
        heimdall_preparser_req_Release(task->preparser_req);

    libheimdall_media_release(task->media);
    free(task);
}

static void heimdall_preparser_subtree_added(heimdall_preparser_req *req,
                                        input_item_node_t *node,
                                        void *user_data)
{
    struct libheimdall_parser_task *task = user_data;
    input_item_t *item = heimdall_preparser_req_GetItem(req);
    assert(task->media->p_input_item == item);
    libheimdall_media_add_subtree(task->media, node);
    input_item_node_Delete(node);
}

static void
heimdall_preparser_ended(heimdall_preparser_req *req, int status, void *user_data)
{
    struct libheimdall_parser_task *task = user_data;
    input_item_t *item = heimdall_preparser_req_GetItem(req);
    assert(task->media->p_input_item == item);

    libheimdall_parser_status_t parser_status;
    switch (status)
    {
        case HEIMDALL_EGENERIC:
            parser_status = libheimdall_parser_status_failed;
            break;
        case HEIMDALL_ETIMEOUT:
            parser_status = libheimdall_parser_status_timeout;
            break;
        case -EINTR:
            parser_status = libheimdall_parser_status_cancelled;
            break;
        case HEIMDALL_SUCCESS:
            parser_status = libheimdall_parser_status_done;
            break;
        default:
            heimdall_assert_unreachable();
    }

    /* The media only tracks whether it ended up parsed; the detailed
       outcome is reported to the caller through parser_status below. A
       failed/timed out/cancelled parse leaves the media unparsed. */
    atomic_store(&task->media->parsed_status,
                 parser_status == libheimdall_parser_status_done
                     ? libheimdall_media_parsed_status_done
                     : libheimdall_media_parsed_status_none);

    task->cbs.parser->on_parsed(
        task->cbs_opaque, task, parser_status);

    libheimdall_parser_task_release(task);
}

static void heimdall_preparser_attachments_added(heimdall_preparser_req *req,
                                            input_attachment_t *const *array,
                                            size_t count,
                                            void *user_data)
{
    struct libheimdall_parser_task *task = user_data;
    input_item_t *item = heimdall_preparser_req_GetItem(req);
    assert(task->media->p_input_item == item);

    if (task->cbs.parser->on_attachments_added == NULL)
        return;

    libheimdall_picture_list_t *list =
        libheimdall_picture_list_from_attachments(array, count);
    if (!list)
        return;
    if (!libheimdall_picture_list_count(list))
    {
        libheimdall_picture_list_destroy(list);
        return;
    }

    task->cbs.parser->on_attachments_added(task->cbs_opaque,
                                           task,
                                           list);
    libheimdall_picture_list_destroy(list);
}

static const struct heimdall_preparser_cbs preparser_callbacks = {
    .on_ended = heimdall_preparser_ended,
    .on_subtree_added = heimdall_preparser_subtree_added,
    .on_attachments_added = heimdall_preparser_attachments_added,
};

static void heimdall_thumbnailer_ended(heimdall_preparser_req *req, int status, picture_t *thumbnail, void *data)
{
    HEIMDALL_UNUSED(status);
    struct libheimdall_parser_task *task = data;
    input_item_t *item = heimdall_preparser_req_GetItem(req);
    assert(task->media->p_input_item == item);

    libheimdall_picture_t* pic = NULL;
    if (thumbnail != NULL)
        pic = libheimdall_picture_new(HEIMDALL_OBJECT(task->instance->p_libheimdall_int),
                                 thumbnail, task->type, task->width,
                                 task->height, task->crop);

    task->cbs.thumbnailer->on_ended(task->cbs_opaque, task, pic);
    if (pic != NULL)
        libheimdall_picture_release(pic);

    libheimdall_parser_task_release(task);
}

static const struct heimdall_thumbnailer_cbs thumbnailer_callbacks = {
    .on_ended = heimdall_thumbnailer_ended,
};

/* Calculate a combination of HEIMDALL_PREPARSER_TYPE_* and HEIMDALL_PREPARSER_OPTION_* flags
   (to be passed to heimdall_preparser_req_NewParse) from libheimdall_media_parse_flag_t. Return -1 to skip parsing */
static int get_parser_type_options(libheimdall_media_parse_flag_t parse_flag)
{
    int parse_scope = 0;
    bool do_parse = false, do_fetch = false;

    if (parse_flag & libheimdall_media_parse)
    {
        do_parse = true;
        parse_scope |= HEIMDALL_PREPARSER_TYPE_PARSE;
    }

    if (parse_flag & libheimdall_media_fetch_local)
    {
        do_fetch = true;
        parse_scope |= HEIMDALL_PREPARSER_TYPE_FETCHMETA_LOCAL;
    }

    if (parse_flag & libheimdall_media_fetch_network)
    {
        do_fetch = true;
        parse_scope |= HEIMDALL_PREPARSER_TYPE_FETCHMETA_NET;
    }

    if (!do_parse && !do_fetch)
        return -1; /* nothing to parse/fetch */

    if (parse_flag & libheimdall_media_do_interact)
        parse_scope |= HEIMDALL_PREPARSER_OPTION_INTERACT;

    parse_scope |= HEIMDALL_PREPARSER_OPTION_SUBITEMS;

    return parse_scope;
}

/* retain a reference to the parser task */
static struct libheimdall_parser_task *
libheimdall_parser_task_retain(struct libheimdall_parser_task *task)
{
    heimdall_atomic_rc_inc(&task->rc);
    return task;
}

libheimdall_parser_task *
libheimdall_parser_task_new_parse(libheimdall_parser_t *parser,
                             const libheimdall_parser_request_t *req,
                             const struct libheimdall_parser_cbs *cbs,
                             void *cbs_opaque)
{
    assert(parser != NULL);
    assert(req != NULL && req->media != NULL);
    assert(cbs != NULL && cbs->on_parsed != NULL);

    /* No different versions to handle for now */
    assert(req->version <= 0);
    assert(cbs->version <= 0);

    int type_options = HEIMDALL_PREPARSER_TYPE_PARSE | HEIMDALL_PREPARSER_OPTION_SUBITEMS;
    if (req->parse_flags != 0)
    {
        type_options = get_parser_type_options(req->parse_flags);
        if (type_options == -1) /* nothing to parse/fetch */
            return NULL;
    }

    libheimdall_media_t *media = req->media;
    input_item_t *item = media->p_input_item;

    union libheimdall_parser_cbs_internal req_cbs = {
        .parser = cbs,
    };

    struct libheimdall_parser_task *task = libheimdall_parser_task_new(media, NULL, req_cbs, cbs_opaque);
    if (task == NULL)
        return NULL;

    task->preparser_req = heimdall_preparser_req_NewParse(parser->preparser,
                                                     item,
                                                     type_options,
                                                     &preparser_callbacks,
                                                     task);

    if (task->preparser_req == NULL)
    {
        libheimdall_parser_task_destroy(task);
        return NULL;
    }

    return task;
}

libheimdall_parser_task *
libheimdall_parser_task_new_thumbnail(libheimdall_parser_t *parser,
                                 const libheimdall_thumbnailer_request_t *req,
                                 const struct libheimdall_thumbnailer_cbs *cbs,
                                 void *cbs_opaque)
{
    assert(parser != NULL);
    assert(req != NULL && req->media != NULL);
    assert(cbs != NULL && cbs->on_ended != NULL);

    /* No different versions to handle for now */
    assert(req->version <= 0);
    assert(cbs->version <= 0);

    input_item_t *item = req->media->p_input_item;

    union libheimdall_parser_cbs_internal req_cbs = {
        .thumbnailer = cbs,
    };

    struct libheimdall_parser_task *task = libheimdall_parser_task_new(req->media, req, req_cbs, cbs_opaque);

    if (task == NULL)
        return NULL;

    task->instance = libheimdall_retain(parser->libheimdall);
    libheimdall_thumbnailer_seek_type_t seek_type = req->seek.type;
    struct heimdall_thumbnailer_arg thumb_arg;
    thumb_arg.hw_dec = req->hw_dec;

    switch (seek_type)
    {
        case libheimdall_thumbnailer_seek_time:
            thumb_arg.seek.type = HEIMDALL_THUMBNAILER_SEEK_TIME;
            thumb_arg.seek.time = heimdall_tick_from_libheimdall_time(req->seek.value.time);
            break;
        case libheimdall_thumbnailer_seek_pos:
            thumb_arg.seek.type = HEIMDALL_THUMBNAILER_SEEK_POS;
            thumb_arg.seek.pos = req->seek.value.pos;
            break;
        default:
            thumb_arg.seek.type = HEIMDALL_THUMBNAILER_SEEK_NONE;
            break;
    }

    thumb_arg.seek.speed = req->seek.speed == libheimdall_media_thumbnail_seek_fast
                         ? HEIMDALL_THUMBNAILER_SEEK_FAST : HEIMDALL_THUMBNAILER_SEEK_PRECISE;

    task->preparser_req =
        heimdall_preparser_req_NewThumbnail(parser->preparser, item, &thumb_arg,
                                       &thumbnailer_callbacks, task);

    if (task->preparser_req == NULL)
    {
        libheimdall_parser_task_destroy(task);
        return NULL;
    }

    return task;
}

int libheimdall_parser_submit(libheimdall_parser_t *parser,
                         libheimdall_parser_task *task)
{
    assert(parser != NULL);
    assert(task != NULL);

    libheimdall_parser_task_retain(task);

    if (heimdall_preparser_Submit(parser->preparser, task->preparser_req)
            != HEIMDALL_SUCCESS)
    {
        libheimdall_parser_task_release(task);
        return -1;
    }

    return 0;
}

size_t libheimdall_parser_cancel_request(libheimdall_parser_t *parser,
                                    libheimdall_parser_task *task)
{
    if (task == NULL)
        return heimdall_preparser_Cancel(parser->preparser, NULL);

    return heimdall_preparser_Cancel(parser->preparser, task->preparser_req);
}

libheimdall_parser_t *libheimdall_parser_new(libheimdall_instance_t *inst,
                                   const struct libheimdall_parser_cfg *cfg)
{
    assert(inst != NULL);
    assert(cfg != NULL);
    libheimdall_time_t timeout;

    /* No different versions to handle for now */
    assert(cfg->version <= 0);

    libheimdall_parser_t *parser = malloc(sizeof(*parser));
    if (parser == NULL)
        return NULL;

    if (cfg->timeout == -1)
        /* "preparse-timeout" is in ms; libheimdall_time_t is in us */
        timeout = var_InheritInteger(inst->p_libheimdall_int, "preparse-timeout") * INT64_C(1000);
    else
        timeout = cfg->timeout;

    const struct heimdall_preparser_cfg preparser_cfg = {
        .types = HEIMDALL_PREPARSER_TYPE_PARSE | HEIMDALL_PREPARSER_TYPE_FETCHMETA_ALL |
                 HEIMDALL_PREPARSER_TYPE_THUMBNAIL,
        .max_parser_threads = cfg->max_parser_threads,
        .max_thumbnailer_threads = cfg->max_thumbnailer_threads,
        .timeout = heimdall_tick_from_libheimdall_time(timeout),
        .external_process = false,
    };
    parser->preparser =
        heimdall_preparser_New(HEIMDALL_OBJECT(inst->p_libheimdall_int), &preparser_cfg);
    if (parser->preparser == NULL)
    {
        free(parser);
        return NULL;
    }
    parser->libheimdall = libheimdall_retain(inst);

    return parser;
}

libheimdall_media_t *libheimdall_parser_task_get_media(libheimdall_parser_task *task)
{
    assert(task != NULL);
    return task->media;
}

void libheimdall_parser_task_release(libheimdall_parser_task *task)
{
    assert(task != NULL);
    if (!heimdall_atomic_rc_dec(&task->rc))
        return;

    libheimdall_parser_task_destroy(task);
}

void libheimdall_parser_destroy(libheimdall_parser_t *parser)
{
    heimdall_preparser_Delete(parser->preparser);
    libheimdall_release(parser->libheimdall);
    free(parser);
}
