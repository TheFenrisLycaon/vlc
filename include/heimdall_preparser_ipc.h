// SPDX-License-Identifier: LGPL-2.1-or-later
/*****************************************************************************
 * heimdall_preparser_serializer.h: preparser serializer
 *****************************************************************************
 * Copyright © 2025 Videolabs, VideoLAN and VLC authors
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Gabriel Lafond Thenaille <gabriel@videolabs.io>
 *****************************************************************************/

#ifndef HEIMDALL_PREPARSER_IPC_H
#define HEIMDALL_PREPARSER_IPC_H

#include <heimdall_common.h>
#include <heimdall_vector.h>
#include <heimdall_input.h>
#include <heimdall_input_item.h>
#include <heimdall_preparser.h>
#include <heimdall_interrupt.h>

/**
 * @defgroup preparser_ipc Preparser IPC
 * @ingroup preparser
 * @{
 * @file
 * Heimdall Preparser IPC API
 *
 * @defgroup preparser_msg preparser message api
 * @ingroup preparser_ipc
 * @{
 */

/**
 * Request types
 */
enum heimdall_preparser_msg_req_type {
    /** 
     * Type of the request emitted by a `heimdall_preparser_req_NewParse` call.
     */
    HEIMDALL_PREPARSER_MSG_REQ_TYPE_PARSE,
    /**
     * Type of the request emitted by a `heimdall_preparser_req_NewThumbnail`
     * call.
     */
    HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL,
    /**
     * Type of the request emitted by a
     * `heimdall_preparser_req_NewThumbnailToFiles` call.
     */
    HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL_TO_FILES,
};

/**
 * Preparser request.
 */
struct heimdall_preparser_msg_req {
    /**
     * Type of the request.
     */
    enum heimdall_preparser_msg_req_type type;

    /**
     * Used only by request of type `HEIMDALL_PREPARSER_MSG_REQ_TYPE_PARSE`.
     */
    int options;

    /**
     * Used by both type `HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL` and
     * `HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL_TO_FILES`.
     */
    struct heimdall_thumbnailer_arg arg;

    /**
     * Used only by request of type
     * `HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL_TO_FILES`.
     */
    /* all `output_path` will be freed so they must be heap allocated or set to
     * NULL before a `heimdall_preparser_msg_Clean` call. */
    struct HEIMDALL_VECTOR(char *) outputs_path;
    struct HEIMDALL_VECTOR(struct heimdall_thumbnailer_output) outputs;

    /* `uri` will be freed so it must be heap allocated or set to
     * NULL before a `heimdall_preparser_msg_Clean` call. */
    char *uri;
};

/**
 * Preparser Response
 */
struct heimdall_preparser_msg_res {
    /**
     * Type of the response (As the response answering a request they share the
     * same type).
     */
    enum heimdall_preparser_msg_req_type type;

    /**
     * Used only by request of type `HEIMDALL_PREPARSER_MSG_REQ_TYPE_PARSE`.
     */
    struct HEIMDALL_VECTOR(input_attachment_t *) attachments;
    input_item_node_t *subtree;

    /**
     * Used only by request of type `HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL`.
     */
    picture_t *pic;

    /**
     * Used only by request of type
     * `HEIMDALL_PREPARSER_MSG_REQ_TYPE_THUMBNAIL_TO_FILES`.
     */
    struct HEIMDALL_VECTOR(bool) result;

    /**
     * Used by all types of request.
     */
    int status;
    input_item_t *item;
};

/**
 * Preparser message.
 */
struct heimdall_preparser_msg {
    /**
     * Type of the message can be a request or a response.
     */
    enum {
        HEIMDALL_PREPARSER_MSG_TYPE_REQ,
        HEIMDALL_PREPARSER_MSG_TYPE_RES,
    } type;

    /**
     * Type of the underling request or response.
     */
    enum heimdall_preparser_msg_req_type req_type;
    union {
        struct heimdall_preparser_msg_req req;
        struct heimdall_preparser_msg_res res;
    };
};

/**
 * Initialize a preparser message.
 * 
 * @info    All data specific to each request/response have to be initialized 
 *          by hand.
 *
 * @param   msg         message to initialize.
 * @param   msg_type    message type (request or response).
 * @param   req_type    request/response type (see enum heimdall_preparser_req_type
 *                      for more information).
 */
HEIMDALL_API void
heimdall_preparser_msg_Init(struct heimdall_preparser_msg *msg, int msg_type,
                       enum heimdall_preparser_msg_req_type req_type);

/**
 * Clean all memory used by a message.
 * 
 * @info    This function don't free the `msg` pointer.
 *
 * @param   msg         message to release.
 */
HEIMDALL_API void
heimdall_preparser_msg_Clean(struct heimdall_preparser_msg *msg);

/**
 * @} preparser_msg
 *
 * @defgroup preparser_serdes preparser serializer api
 * @ingroup preparser_ipc
 *
 * @{
 */
#define HEIMDALL_PREPARSER_MSG_SERDES_TYPE_DATA              0x1
#define HEIMDALL_PREPARSER_MSG_SERDES_TYPE_ATTACHMENT        0x2
#define HEIMDALL_PREPARSER_MSG_SERDES_TYPE_END_DATA          0x4
#define HEIMDALL_PREPARSER_MSG_SERDES_TYPE_END_ATTACHMENT    0x8

struct heimdall_preparser_msg_serdes_cbs {
    /**
     * Write callback.
     *
     * @param [in]  data        buffer to write.
     * @param [in]  size        number of bytes to write.
     * @param [in]  userdata    callback userdata.
     *
     * @return      the number of bytes writen or an error code on failure.
     */
    ssize_t (*write)(const void *data, size_t size, void *userdata);

    /**
     * Read callback.
     *
     * @param [out] data        buffer to read into.
     * @param [in]  size        number of bytes to read.
     * @param [in]  userdata    callback userdata.
     *
     * @return      the number of bytes read or an error code on failure.
     */
    ssize_t (*read)(void *data, size_t size, void *userdata);
};

struct heimdall_preparser_msg_serdes;

struct heimdall_preparser_msg_serdes_operations {
    /**
     * Serialize `msg` and call the write callback with serialized data.
     *
     * @param [in]  serdes      serializer internal structure.
     * @param [in]  msg         message to serialize.
     * @param [in]  userdata    context for the write callbacks
     *
     * @return      HEIMDALL_SUCCESS or an error code on failure.
     */
    int (*serialize)(struct heimdall_preparser_msg_serdes *serdes,
                     const struct heimdall_preparser_msg *msg,
                     void *userdata);

    /**
     * Deserialize `msg` and call the read callback to get data to deserialize.
     *
     * @param [in]  serdes      serializer internal structure.
     * @param [out] msg         message to deserialize.
     * @param [in]  userdata    context for the read callbacks
     *
     * @return      HEIMDALL_SUCCESS or an error code on failure.
     */
    int (*deserialize)(struct heimdall_preparser_msg_serdes *serdes,
                       struct heimdall_preparser_msg *msg,
                       void *userdata);

    /**
     * Close the serializer/deserialier and release all used memory.
     *
     * @param [in]  serdes  preparser msg serdes internal struture.
     */
    void (*close)(struct heimdall_preparser_msg_serdes *serdes);
};

/**
 * Internal structure used by serializer.
 */
struct heimdall_preparser_msg_serdes {
    /** Operations */
    const struct heimdall_preparser_msg_serdes_operations *ops;

    struct {
        /** Callbacks */
        const struct heimdall_preparser_msg_serdes_cbs *cbs;
        /** Used by the serializer module. */
        void *sys;
    } owner;
};

typedef int (*heimdall_preparser_msg_serdes_module)
                (struct heimdall_preparser_msg_serdes *, bool);

#define set_callback_preparser_msg_serdes(activate, priority) \
    {\
        heimdall_preparser_msg_serdes_module open__ = activate;\
        (void)open__;\
        set_callback(activate)\
    }\
    set_capability("preparser msg serdes", priority)

/**
 * \copydoc heimdall_preparser_msg_serdes_operations::serialize
 */
static inline int
heimdall_preparser_msg_serdes_Serialize(struct heimdall_preparser_msg_serdes *serdes,
                                   const struct heimdall_preparser_msg *msg,
                                   void *userdata)
{
    assert(serdes != NULL);

    if (serdes->ops != NULL && serdes->ops->serialize != NULL) {
        return serdes->ops->serialize(serdes, msg, userdata);
    }
    return HEIMDALL_EGENERIC;
}

/**
 * \copydoc heimdall_preparser_msg_serdes_operations::deserialize
 */
static inline int
heimdall_preparser_msg_serdes_Deserialize(struct heimdall_preparser_msg_serdes *serdes,
                                     struct heimdall_preparser_msg *msg,
                                     void *userdata)
{
    assert(serdes!= NULL);

    if (serdes->ops != NULL && serdes->ops->deserialize != NULL) {
        return serdes->ops->deserialize(serdes, msg, userdata);
    }
    return HEIMDALL_EGENERIC;
}

/**
 * Free the msg_serdes struct.
 *
 * @param [in]  msg_serdes
 */
static inline void
heimdall_preparser_msg_serdes_Delete(struct heimdall_preparser_msg_serdes *serdes)
{
    assert(serdes != NULL);

    if (serdes->ops != NULL && serdes->ops->close != NULL) {
        serdes->ops->close(serdes);
    }
    free(serdes);
}

/**
 * Create a heimdall_preparser_msg_serdes object and load a preparser msg_serdes
 * module.
 *
 * @param [in]  obj         heimdall object
 * @param [in]  c           serializer's callbacks
 * @param [in]  bin_data    describe if the serializer and deserializer use 
 *                          binary data (intput_attachment_t or plane_t)
 *
 * @return      a heimdall_preparser_msg_serdes object or NULL on failure.
 */
HEIMDALL_API struct heimdall_preparser_msg_serdes *
heimdall_preparser_msg_serdes_Create(heimdall_object_t *obj,
                                const struct heimdall_preparser_msg_serdes_cbs *c,
                                bool bin_data);

/**
 * @} preparser_serdes
 * @} preparser_ipc
 */

#endif /* HEIMDALL_PREPARSER_IPC */
