// SPDX-License-Identifier: LGPL-2.1-or-later
/*****************************************************************************
 * preparser.c: preparse a media and return it's informations
 *****************************************************************************
 * Copyright © 2025 Videolabs, VideoLAN and VLC authors
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Gabriel Lafond Thenaille <gabriel@videolabs.io>
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>

#include <heimdall/heimdall.h>
#include <heimdall_common.h>
#include <heimdall_preparser.h>
#include <heimdall_preparser_ipc.h>
#include <heimdall_vector.h>
#include <heimdall_process.h>
#include <heimdall_threads.h>
#include <heimdall_fs.h>
#include <heimdall_url.h>
#include <heimdall_tls.h>

#include "preparser/cmdline.h"

#include "../lib/libheimdall_internal.h"

struct preparser {
    heimdall_sem_t sem;
    struct heimdall_preparser_msg res_msg;
    struct heimdall_preparser_msg req_msg;
    struct heimdall_preparser_msg_serdes *serdes;
    bool daemon;
    int status;

    struct heimdall_tls *tls_in;
    struct heimdall_tls *tls_out;
};


/****************************************************************************
 * parse Callbacks
 *****************************************************************************/

static void parse_OnEnded(struct heimdall_preparser_req *req, int status, void *userdata)
{
    struct preparser *pp = userdata;
    assert(pp != NULL);
    assert(pp->serdes != NULL);
    assert(pp->res_msg.type == HEIMDALL_PREPARSER_MSG_TYPE_RES);
    assert(pp->res_msg.req_type == HEIMDALL_PREPARSER_MSG_REQ_TYPE_PARSE);
    assert(req != NULL);
    input_item_t *item = heimdall_preparser_req_GetItem(req);
    assert(item != NULL && item == pp->res_msg.res.item);

    pp->res_msg.res.status = status;
    pp->status = status;
    heimdall_preparser_msg_serdes_Serialize(pp->serdes, &pp->res_msg, pp->tls_out);
    heimdall_sem_post(&pp->sem);
}

static void parse_OnAttachmentsAdded(struct heimdall_preparser_req *req,
                                     input_attachment_t *const *array,
                                     size_t count, void *userdata)
{
    struct preparser *pp = userdata;
    assert(pp != NULL);
    assert(pp->res_msg.type == HEIMDALL_PREPARSER_MSG_TYPE_RES);
    assert(pp->res_msg.req_type == HEIMDALL_PREPARSER_MSG_REQ_TYPE_PARSE);
    assert(req != NULL);
    input_item_t *item = heimdall_preparser_req_GetItem(req);
    assert(item != NULL && item == pp->res_msg.res.item);
    assert(array != NULL);

    for (size_t i = 0; i < count; i++) {
        assert(array[i] != NULL);
        input_attachment_t *a = heimdall_input_attachment_Hold(array[i]);
        heimdall_vector_push(&pp->res_msg.res.attachments, a);
    }
}

static void parse_OnSubtreeAdded(struct heimdall_preparser_req *req,
                                 input_item_node_t *subtree,
                                 void *userdata)
{
    struct preparser *pp = userdata;
    assert(pp != NULL);
    assert(pp->res_msg.type == HEIMDALL_PREPARSER_MSG_TYPE_RES);
    assert(pp->res_msg.req_type == HEIMDALL_PREPARSER_MSG_REQ_TYPE_PARSE);
    assert(req != NULL);
    input_item_t *item = heimdall_preparser_req_GetItem(req);
    assert(item != NULL && item == pp->res_msg.res.item);

    pp->res_msg.res.subtree = subtree;
}

/****************************************************************************
 * thumbnailer Callback
 *****************************************************************************/

static void thumbnailer_OnEnded(struct heimdall_preparser_req *req, int status,
                                picture_t* thumbnail, void *userdata)
{
    struct preparser *pp = userdata;
    assert(pp != NULL);
    assert(pp->serdes != NULL);
    assert(pp->res_msg.type == HEIMDALL_PREPARSER_MSG_TYPE_RES);
    assert(pp->res_msg.req_type == HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL);
    assert(req != NULL);
    input_item_t *item = heimdall_preparser_req_GetItem(req);
    assert(item != NULL && item == pp->res_msg.res.item);

    pp->res_msg.res.status = status;
    if (status == 0) {
        assert(thumbnail != NULL);
        pp->res_msg.res.pic = picture_Hold(thumbnail);
    }

    heimdall_preparser_msg_serdes_Serialize(pp->serdes, &pp->res_msg, NULL);
    heimdall_sem_post(&pp->sem);
}

/****************************************************************************
 * thumbnailer_to_files Callback
 *****************************************************************************/

static void thumbnailer_to_files_OnEnded(struct heimdall_preparser_req *req,
                                         int status, const bool *result_array,
                                         size_t result_count, void *userdata)
{
    struct preparser *pp = userdata;
    assert(pp != NULL);
    assert(pp->serdes != NULL);
    assert(pp->res_msg.type == HEIMDALL_PREPARSER_MSG_TYPE_RES);
    assert(pp->res_msg.req_type == HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL_TO_FILES);
    assert(req != NULL);
    input_item_t *item = heimdall_preparser_req_GetItem(req);
    assert(item != NULL && item == pp->res_msg.res.item);

    pp->res_msg.res.status = status;
    heimdall_vector_init(&pp->res_msg.res.result);

    if (result_count != 0) {
        assert(result_array != NULL);
        heimdall_vector_push_all(&pp->res_msg.res.result, result_array, result_count);
    }

    heimdall_preparser_msg_serdes_Serialize(pp->serdes, &pp->res_msg, NULL);
    heimdall_sem_post(&pp->sem);
}

/****************************************************************************
 * Preparser engine
 *****************************************************************************/

static struct heimdall_preparser_req *
preparser_req_BuildParse(heimdall_preparser_t *preparser,
                         struct preparser *pp)
{
    assert(pp->req_msg.req_type == HEIMDALL_PREPARSER_MSG_REQ_TYPE_PARSE);

    static const struct heimdall_preparser_cbs cbs = {
        .on_ended = parse_OnEnded,
        .on_attachments_added = parse_OnAttachmentsAdded,
        .on_subtree_added = parse_OnSubtreeAdded,
    };

    return heimdall_preparser_req_NewParse(preparser, pp->res_msg.res.item,
                                      pp->req_msg.req.options, &cbs, pp);
}

static struct heimdall_preparser_req *
preparser_req_BuildThumbnail(heimdall_preparser_t *preparser,
                             struct preparser *pp)
{
    assert(pp->req_msg.req_type == HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL);

    static const struct heimdall_thumbnailer_cbs cbs = {
        .on_ended = thumbnailer_OnEnded,
    };

    return heimdall_preparser_req_NewThumbnail(preparser, pp->res_msg.res.item,
                                          &pp->req_msg.req.arg, &cbs, pp);
}

static struct heimdall_preparser_req *
preparser_req_BuildThumbnailToFiles(heimdall_preparser_t *preparser,
                                    struct preparser *pp)
{
    assert(pp->req_msg.req_type == HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL_TO_FILES);

    static const struct heimdall_thumbnailer_to_files_cbs cbs = {
        .on_ended = thumbnailer_to_files_OnEnded,
    };

    return heimdall_preparser_req_NewThumbnailToFiles(preparser,
                                                 pp->res_msg.res.item,
                                                 &pp->req_msg.req.arg,
                                                 pp->req_msg.req.outputs.data,
                                                 pp->req_msg.req.outputs.size,
                                                 &cbs, pp);
}

static int
preparser_req_Preparse(heimdall_preparser_t *preparser, struct preparser *pp)
{
    assert(preparser != NULL);
    assert(pp != NULL);
    assert(pp->req_msg.type == HEIMDALL_PREPARSER_MSG_TYPE_REQ);
    assert(pp->req_msg.req.uri != NULL);

    int ret = HEIMDALL_EGENERIC;
    heimdall_preparser_msg_Init(&pp->res_msg, HEIMDALL_PREPARSER_MSG_TYPE_RES,
                           pp->req_msg.req_type);
    pp->res_msg.res.item = input_item_New(pp->req_msg.req.uri, NULL);
    if (pp->res_msg.res.item == NULL) {
        goto end;
    }

    struct heimdall_preparser_req *req = NULL;
    switch (pp->req_msg.req_type) {
        case HEIMDALL_PREPARSER_MSG_REQ_TYPE_PARSE:
            req = preparser_req_BuildParse(preparser, pp);
            break;
        case HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL:
            req = preparser_req_BuildThumbnail(preparser, pp);
            break;
        case HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL_TO_FILES:
            req = preparser_req_BuildThumbnailToFiles(preparser, pp);
            break;
        default:
            goto end;
    }

    if (req != NULL && heimdall_preparser_Submit(preparser, req) != HEIMDALL_SUCCESS) {
        heimdall_preparser_req_Release(req);
        req = NULL;
    }

    if (req == NULL) {
        if (pp->serdes != NULL) {
            ret = heimdall_preparser_msg_serdes_Serialize(pp->serdes, &pp->res_msg,
                                                     NULL);
        }
    } else {
        ret = HEIMDALL_SUCCESS;
        heimdall_preparser_req_Release(req);
        heimdall_sem_wait(&pp->sem);
    }

end:
    heimdall_preparser_msg_Clean(&pp->res_msg);
    return ret;
}

/****************************************************************************
 * Serdes Callbacks
 *****************************************************************************/

static ssize_t write_cbs(const void *data, size_t size, void *userdata)
{
    ssize_t ret = -1;
    if (userdata == NULL) {
        ret = write(STDOUT_FILENO, data, size);
        return ret;
    }
    struct heimdall_tls *tls = userdata;
    ret = heimdall_tls_Write(tls, data, size);
    return ret;
}

static ssize_t read_cbs(void *data, size_t size, void *userdata)
{
    ssize_t ret = -1;
    if (userdata == NULL) {
        ret = read(STDIN_FILENO, data, size);
        return ret;
    }
    struct heimdall_tls *tls = userdata;
    ret = heimdall_tls_Read(tls, data, size, false);
    return ret;
}

/****************************************************************************
 * Cmd line arg
 *****************************************************************************/

static int
preparser_args_Preparse(heimdall_preparser_t *preparser,
                        struct preparser *pp, char *uri,
                        struct preparser_args *args)
{
    assert(preparser != NULL);
    assert(pp != NULL);
    assert(uri != NULL);
    assert(args != NULL);

    if (args->types & HEIMDALL_PREPARSER_TYPE_PARSE) {
        int t = args->types & ~(HEIMDALL_PREPARSER_TYPE_THUMBNAIL |
                                HEIMDALL_PREPARSER_TYPE_THUMBNAIL_TO_FILES);
        heimdall_preparser_msg_Init(&pp->req_msg, HEIMDALL_PREPARSER_MSG_TYPE_REQ,
                               HEIMDALL_PREPARSER_MSG_REQ_TYPE_PARSE);
        pp->req_msg.req.options = t;
        pp->req_msg.req.uri = uri;
        preparser_req_Preparse(preparser, pp);
        pp->req_msg.req.uri = NULL;
        if (pp->status != HEIMDALL_SUCCESS) {
            fprintf(stderr, "Error while parsing '%s'\n", uri);
            heimdall_preparser_msg_Clean(&pp->req_msg);
            return HEIMDALL_EGENERIC;
        }
        heimdall_preparser_msg_Clean(&pp->req_msg);
    }
    if (args->types & HEIMDALL_PREPARSER_TYPE_THUMBNAIL) {
        heimdall_preparser_msg_Init(&pp->req_msg, HEIMDALL_PREPARSER_MSG_TYPE_REQ,
                               HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL);
        pp->req_msg.req.arg.seek.type = args->seek.type;
        if (args->seek.type == HEIMDALL_THUMBNAILER_SEEK_TIME) {
            pp->req_msg.req.arg.seek.time = args->seek.time;
        } else if (args->seek.type == HEIMDALL_THUMBNAILER_SEEK_POS) {
            pp->req_msg.req.arg.seek.pos = args->seek.pos;
        }
        pp->req_msg.req.arg.seek.speed = args->seek.speed;
        pp->req_msg.req.arg.hw_dec = false;
        pp->req_msg.req.uri = strdup(uri);
        preparser_req_Preparse(preparser, pp);
        pp->req_msg.req.uri = NULL;
        if (pp->status != HEIMDALL_SUCCESS) {
            fprintf(stderr, "Error while parsing '%s'\n", uri);
            heimdall_preparser_msg_Clean(&pp->req_msg);
            return HEIMDALL_EGENERIC;
        }
        heimdall_preparser_msg_Clean(&pp->req_msg);
    }
    if (args->types & HEIMDALL_PREPARSER_TYPE_THUMBNAIL_TO_FILES) {
        heimdall_preparser_msg_Init(&pp->req_msg, HEIMDALL_PREPARSER_MSG_TYPE_REQ,
                               HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL_TO_FILES);
        pp->req_msg.req.arg.seek.type = args->seek.type;
        if (args->seek.type == HEIMDALL_THUMBNAILER_SEEK_TIME) {
            pp->req_msg.req.arg.seek.time = args->seek.time;
        } else if (args->seek.type == HEIMDALL_THUMBNAILER_SEEK_POS) {
            pp->req_msg.req.arg.seek.pos = args->seek.pos;
        }
        pp->req_msg.req.arg.seek.speed = args->seek.speed;
        pp->req_msg.req.arg.hw_dec = false;
        struct heimdall_thumbnailer_output out = {
            .width = args->output.width,
            .height = args->output.height,
            .file_path = args->output.file_path,
            .format = args->output.format,
            .creat_mode = 0766,
            .crop = args->output.crop,
        };
        heimdall_vector_push(&pp->req_msg.req.outputs, out);
        pp->req_msg.req.uri = strdup(uri);
        preparser_req_Preparse(preparser, pp);
        pp->req_msg.req.uri = NULL;
        if (pp->status != HEIMDALL_SUCCESS) {
            fprintf(stderr, "Error while parsing '%s'\n", uri);
            heimdall_preparser_msg_Clean(&pp->req_msg);
            return HEIMDALL_EGENERIC;
        }
        heimdall_preparser_msg_Clean(&pp->req_msg);
    }

    return HEIMDALL_SUCCESS;
}


static int
preparser_args_Loop(heimdall_object_t *obj, heimdall_preparser_t *preparser,
                    char *const *argv, int argc, struct preparser_args *args)
{
    assert(preparser != NULL);
    assert(argv != NULL);
    assert(argc != 0);
    assert(args != 0);

    struct preparser pp;
    heimdall_sem_init(&pp.sem, 0);
    pp.status = 0;
    pp.daemon = false;
    pp.tls_in = NULL;
    pp.tls_out = NULL;

    static const struct heimdall_preparser_msg_serdes_cbs args_cbs = {
        .write = write_cbs,
        .read = NULL,
    };

    pp.serdes = heimdall_preparser_msg_serdes_Create(obj, &args_cbs, false);
    if (pp.serdes == NULL) {
        return HEIMDALL_EGENERIC;
    }

    for (int i = 0; i < argc; i++) {
        char *uri = NULL;
        if (strstr(argv[i], "://" ) == NULL) {
            uri = heimdall_path2uri(argv[i], NULL);
        } else {
            uri = strdup(argv[i]);
        }
        if (uri == NULL) {
            heimdall_preparser_msg_serdes_Delete(pp.serdes);
            return HEIMDALL_ENOMEM;
        }
        preparser_args_Preparse(preparser, &pp, uri, args);
        free(uri);
    }
    heimdall_preparser_msg_serdes_Delete(pp.serdes);
    return HEIMDALL_SUCCESS;
}

/****************************************************************************
 * Daemon
 *****************************************************************************/

static int
preparser_daemon_Loop(heimdall_object_t *obj, heimdall_preparser_t *preparser)
{
    assert(preparser != NULL);

    struct preparser pp;
    heimdall_sem_init(&pp.sem, 0);
    pp.status = 0;
    pp.daemon = true;

    static const struct heimdall_preparser_msg_serdes_cbs deamon_cbs = {
        .write = write_cbs,
        .read = read_cbs,
    };

    pp.serdes = heimdall_preparser_msg_serdes_Create(obj, &deamon_cbs, true);
    if (pp.serdes == NULL) {
        return HEIMDALL_EGENERIC;
    }

#ifndef _WIN32
    pp.tls_in = heimdall_tls_SocketOpen(STDIN_FILENO);
    if (pp.tls_in == NULL) {
        heimdall_preparser_msg_serdes_Delete(pp.serdes);
        return HEIMDALL_EGENERIC;
    }
    pp.tls_out = heimdall_tls_SocketOpen(STDOUT_FILENO);
    if (pp.tls_out == NULL) {
        pp.tls_in->ops->close(pp.tls_in);
        heimdall_preparser_msg_serdes_Delete(pp.serdes);
        return HEIMDALL_EGENERIC;
    }
#else
    pp.tls_in = NULL;
    pp.tls_out = NULL;
#endif

    int status = HEIMDALL_SUCCESS;
    while (status == HEIMDALL_SUCCESS) {
        status = heimdall_preparser_msg_serdes_Deserialize(pp.serdes, &pp.req_msg,
                                                      pp.tls_in);
        if (status != HEIMDALL_SUCCESS) {
            break;
        }
        if (pp.req_msg.req.uri != NULL) {
            status = preparser_req_Preparse(preparser, &pp);
        } else {
            status = HEIMDALL_EGENERIC;
        }
        heimdall_preparser_msg_Clean(&pp.req_msg);
    }

    if (pp.tls_in != NULL) {
        pp.tls_in->ops->close(pp.tls_in);
    }
    if (pp.tls_out != NULL) {
        pp.tls_out->ops->close(pp.tls_out);
    }
    heimdall_preparser_msg_serdes_Delete(pp.serdes);
    return status;
}

/****************************************************************************
 * Main
 *****************************************************************************/
const char heimdall_module_name[] = "heimdall-preparser";

/**
 * Wait for new request and call preparser_Preparse.
 */
int main(int argc, char **argv)
{
#ifdef TOP_BUILDDIR
    setenv("HEIMDALL_PLUGIN_PATH", TOP_BUILDDIR"/modules", 1);
    setenv("HEIMDALL_DATA_PATH", TOP_SRCDIR"/share", 1);
    setenv("HEIMDALL_LIB_PATH", TOP_BUILDDIR"/modules", 1);
#endif

#ifdef _WIN32
#include <fcntl.h>
    setmode(STDIN_FILENO, O_BINARY);
    setmode(STDOUT_FILENO, O_BINARY);
#endif

    heimdall_thread_set_name("heimdall-preparser");

    struct preparser_args args = {
        .timeout = HEIMDALL_TICK_INVALID,
        .types = 0,
        .daemon = false,
        .seek.type = HEIMDALL_THUMBNAILER_SEEK_NONE,
        .seek.speed = HEIMDALL_THUMBNAILER_SEEK_FAST,
        .output.file_path = NULL,
        .output.format = HEIMDALL_THUMBNAILER_FORMAT_PNG,
        .output.height = 0,
        .output.width = 0,
        .output.crop = false,
        .verbosity = "-1",
    };

    int ret = preparser_cmdline_Parse(argc, argv, &args);
    if (ret <= 0) {
        return ret == 0 ? 0 : 1;
    }

    const char *libheimdall_args[] = {
        "--verbose", args.verbosity, "--vout=vdummy", "--aout=adummy",
        "--text-renderer=tdummy",
    };

    libheimdall_instance_t *heimdall = libheimdall_new(ARRAY_SIZE(libheimdall_args), libheimdall_args);
    if (heimdall == NULL) {
        return 1;
    }
    heimdall_object_t *obj = HEIMDALL_OBJECT(heimdall->p_libheimdall_int);

    const struct heimdall_preparser_cfg cfg = {
        .types = args.types,
        .max_parser_threads = 1,
        .max_thumbnailer_threads = 1,
        .timeout = args.timeout,
        .external_process = false,
    };

    heimdall_preparser_t *preparser = heimdall_preparser_New(obj, &cfg);
    if (preparser == NULL) {
        libheimdall_release(heimdall);
        return 1;
    }

    if (args.daemon) {
        ret = preparser_daemon_Loop(obj, preparser);
    } else {
        ret = preparser_args_Loop(obj, preparser, argv + args.arg_idx,
                                  argc - args.arg_idx, &args);
    }

    heimdall_preparser_Delete(preparser);
    libheimdall_release(heimdall);
    return ret;
}
