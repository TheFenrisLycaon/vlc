/*****************************************************************************
 * messages.c: messages interface
 * This library provides an interface to the message queue to be used by other
 * modules, especially intf modules. See heimdall_config.h for output configuration.
 *****************************************************************************
 * Copyright (C) 1998-2005 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Vincent Seguin <seguin@via.ecp.fr>
 *          Samuel Hocevar <sam@zoy.org>
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdarg.h>                                       /* va_list for BSD */
#include <unistd.h>
#include <assert.h>

#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_interface.h>
#include <heimdall_charset.h>
#include <heimdall_modules.h>
#include "rcu.h"
#include "../libheimdall.h"

#ifndef CONFIGURE_LINE
// generated C file
extern const char CONFIGURE_LINE[];
#endif

#ifdef _WIN32
static const char msg_type[4][9] = { "", " error", " warning", " debug" };

static void Win32DebugOutputMsg (int type, const heimdall_log_t *p_item,
                                 const char *format, va_list dol)
{
    char *msg = NULL;
    int msg_len = vasprintf(&msg, format, dol);

    if (unlikely(msg_len == -1))
        return;

    const char *ending = msg_len > 1 && msg[msg_len-1] == '\n' ? "" : "\n";

    char* psz_msg = NULL;
    if (asprintf(&psz_msg, "%s %s%s: %s%s", p_item->psz_module,
                p_item->psz_object_type, msg_type[type], msg, ending) > 0) {
        wchar_t* wmsg = ToWide(psz_msg);
        if (likely(wmsg != NULL))
        {
            OutputDebugStringW(wmsg);
            free(wmsg);
        }
        free(psz_msg);
    }
    free(msg);
}
#endif

static void heimdall_LogSpam(heimdall_object_t *obj)
{
    /* Announce who we are */
    msg_Dbg(obj, "Heimdall media player - %s", VERSION_MESSAGE);
    msg_Dbg(obj, "%s", COPYRIGHT_MESSAGE);
    msg_Dbg(obj, "revision %s", psz_heimdall_changeset);
    msg_Dbg(obj, "configured with %s", CONFIGURE_LINE);
}

static void heimdall_vaLogCallback(heimdall_logger_t *logger, int type,
                              const heimdall_log_t *item, const char *format,
                              va_list ap)
{
    if (logger != NULL) {
#ifdef _WIN32
        va_list dol;

        va_copy (dol, ap);
        Win32DebugOutputMsg (type, item, format, dol);
        va_end (dol);
#endif

        int canc = heimdall_savecancel();

        logger->ops->log(logger, type, item, format, ap);
        heimdall_restorecancel(canc);
    }
}

static void heimdall_LogCallback(heimdall_logger_t *logger, int type,
                            const heimdall_log_t *item, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    heimdall_vaLogCallback(logger, type, item, format, ap);
    va_end(ap);
}

void heimdall_vaLog(struct heimdall_logger *const *loggerp, int type,
               const char *typename, const char *module,
               const char *file, unsigned line, const char *func,
               const char *format, va_list args)
{
    struct heimdall_logger *logger = *loggerp;
    if (logger == NULL)
        // nothing to do
        return;

    /* Get basename from the module filename */
    const char *p = strrchr(module, '/');
    if (p != NULL)
        module = p + 1;
    p = strchr(module, '.');

    size_t modlen = (p != NULL) ? (p - module) : 0;
    char modulebuf[modlen + 1];
    if (p != NULL)
    {
        memcpy(modulebuf, module, modlen);
        modulebuf[modlen] = '\0';
        module = modulebuf;
    }

    /* Fill message information fields */
    heimdall_log_t msg;

    msg.i_object_id = (uintptr_t)(void *)loggerp;
    msg.psz_object_type = typename;
    msg.psz_module = module;
    msg.psz_header = NULL;
    msg.file = file;
    msg.line = line;
    msg.func = func;
    msg.tid = heimdall_thread_id();

    /* Pass message to the callback */
    heimdall_vaLogCallback(logger, type, &msg, format, args);
}

void heimdall_Log(struct heimdall_logger *const *logger, int type,
             const char *typename, const char *module,
             const char *file, unsigned line, const char *func,
             const char *format, ... )
{
    va_list ap;

    va_start(ap, format);
    heimdall_vaLog(logger, type, typename, module, file, line, func, format, ap);
    va_end(ap);
}

/**
 * Early (latched) message log.
 *
 * A message log that stores messages in memory until another log is available.
 */
typedef struct heimdall_log_early_t
{
    struct heimdall_log_early_t *next;
    int type;
    heimdall_log_t meta;
    char *msg;
} heimdall_log_early_t;

typedef struct heimdall_logger_early {
    heimdall_mutex_t lock;
    heimdall_log_early_t *head;
    heimdall_log_early_t **tailp;
    heimdall_logger_t *sink;
    struct heimdall_logger logger;
} heimdall_logger_early_t;

static void heimdall_vaLogEarly(void *d, int type, const heimdall_log_t *item,
                           const char *format, va_list ap)
{
    struct heimdall_logger *logger = d;
    struct heimdall_logger_early *early =
        container_of(logger, struct heimdall_logger_early, logger);

    heimdall_log_early_t *log = malloc(sizeof (*log));
    if (unlikely(log == NULL))
        return;

    log->next = NULL;
    log->type = type;
    log->meta.i_object_id = item->i_object_id;
    /* NOTE: Object types MUST be static constant - no need to copy them. */
    log->meta.psz_object_type = item->psz_object_type;
    log->meta.psz_module = item->psz_module; /* Ditto. */
    log->meta.psz_header = item->psz_header ? strdup(item->psz_header) : NULL;
    log->meta.file = item->file;
    log->meta.line = item->line;
    log->meta.func = item->func;

    if (vasprintf(&log->msg, format, ap) == -1)
        log->msg = NULL;

    heimdall_mutex_lock(&early->lock);
    assert(early->tailp != NULL);
    assert(*(early->tailp) == NULL);
    *(early->tailp) = log;
    early->tailp = &log->next;
    heimdall_mutex_unlock(&early->lock);
}

static void heimdall_LogEarlyClose(void *d)
{
    struct heimdall_logger *logger = d;
    struct heimdall_logger_early *early =
        container_of(logger, struct heimdall_logger_early, logger);
    heimdall_logger_t *sink = early->sink;

    /* Drain early log messages */
    for (heimdall_log_early_t *log = early->head, *next; log != NULL; log = next)
    {
        heimdall_LogCallback(sink, log->type, &log->meta, "%s",
                        (log->msg != NULL) ? log->msg : "message lost");
        free(log->msg);
        free((char*)log->meta.psz_header); // local copy
        next = log->next;
        free(log);
    }

    free(early);
}

static const struct heimdall_logger_operations early_ops = {
    heimdall_vaLogEarly,
    heimdall_LogEarlyClose,
};

static struct heimdall_logger *heimdall_LogEarlyOpen(struct heimdall_logger *logger)
{
    heimdall_logger_early_t *early = malloc(sizeof (*early));
    if (unlikely(early == NULL))
        return NULL;

    early->logger.ops = &early_ops;
    heimdall_mutex_init(&early->lock);
    early->head = NULL;
    early->tailp = &early->head;
    early->sink = logger;
    return &early->logger;
}

static void heimdall_vaLogDiscard(void *d, int type, const heimdall_log_t *item,
                             const char *format, va_list ap)
{
    (void) d; (void) type; (void) item; (void) format; (void) ap;
}

static void heimdall_LogDiscardClose(void *d)
{
    (void) d;
}

static const struct heimdall_logger_operations discard_ops = {
    heimdall_vaLogDiscard,
    heimdall_LogDiscardClose,
};

static struct heimdall_logger discard_log = { &discard_ops };

/**
 * Switchable message log.
 *
 * A message log that can be redirected live.
 */
struct heimdall_logger_switch {
    struct heimdall_logger *_Atomic backend;
    struct heimdall_logger frontend;
};

static void heimdall_vaLogSwitch(void *d, int type, const heimdall_log_t *item,
                            const char *format, va_list ap)
{
    struct heimdall_logger *logger = d;
    struct heimdall_logger_switch *logswitch =
        container_of(logger, struct heimdall_logger_switch, frontend);
    struct heimdall_logger *backend;

    heimdall_rcu_read_lock();
    backend = atomic_load_explicit(&logswitch->backend, memory_order_acquire);
    backend->ops->log(backend, type, item, format, ap);
    heimdall_rcu_read_unlock();
}

static void heimdall_LogSwitch(heimdall_logger_t *logger, heimdall_logger_t *new_logger);

static void heimdall_LogSwitchClose(void *d)
{
    struct heimdall_logger *logger = d;
    struct heimdall_logger_switch *logswitch =
        container_of(logger, struct heimdall_logger_switch, frontend);

    /* Discard any further messages. */
    heimdall_LogSwitch(logger, &discard_log);
    free(logswitch);
}

static const struct heimdall_logger_operations switch_ops = {
    heimdall_vaLogSwitch,
    heimdall_LogSwitchClose,
};

static void heimdall_LogSwitch(heimdall_logger_t *logger, heimdall_logger_t *new_logger)
{
    struct heimdall_logger_switch *logswitch =
        container_of(logger, struct heimdall_logger_switch, frontend);
    struct heimdall_logger *old_logger;

    assert(logger->ops == &switch_ops);

    if (new_logger == NULL)
        new_logger = &discard_log;

    old_logger = atomic_exchange_explicit(&logswitch->backend, new_logger,
                                          memory_order_acq_rel);
    heimdall_rcu_synchronize();
    old_logger->ops->destroy(old_logger);
}

static struct heimdall_logger *heimdall_LogSwitchCreate(void)
{
    struct heimdall_logger_switch *logswitch = malloc(sizeof (*logswitch));
    if (unlikely(logswitch == NULL))
        return NULL;

    logswitch->frontend.ops = &switch_ops;
    atomic_init(&logswitch->backend, &discard_log);
    return &logswitch->frontend;
}

/**
 * Module-based message log.
 */
struct heimdall_logger_module {
    struct heimdall_object_t obj;
    struct heimdall_logger frontend;
    const struct heimdall_logger_operations *ops;
    void *opaque;
};

static int heimdall_logger_load(void *func, bool forced, va_list ap)
{
    const struct heimdall_logger_operations *(*activate)(heimdall_object_t *,
                                                    void **) = func;
    struct heimdall_logger_module *module = va_arg(ap, struct heimdall_logger_module *);

    (void) forced;
    module->ops = activate(HEIMDALL_OBJECT(module), &module->opaque);
    return (module->ops != NULL) ? HEIMDALL_SUCCESS : HEIMDALL_EGENERIC;
}

static void heimdall_vaLogModule(void *d, int type, const heimdall_log_t *item,
                            const char *format, va_list ap)
{
    struct heimdall_logger *logger = d;
    struct heimdall_logger_module *module =
        container_of(logger, struct heimdall_logger_module, frontend);

    module->ops->log(module->opaque, type, item, format, ap);
}

static void heimdall_LogModuleClose(void *d)
{
    struct heimdall_logger *logger = d;
    struct heimdall_logger_module *module =
        container_of(logger, struct heimdall_logger_module, frontend);

    if (module->ops->destroy != NULL)
        module->ops->destroy(module->opaque);

    heimdall_object_delete(HEIMDALL_OBJECT(module));
}

static const struct heimdall_logger_operations module_ops = {
    heimdall_vaLogModule,
    heimdall_LogModuleClose,
};

static struct heimdall_logger *heimdall_LogModuleCreate(heimdall_object_t *parent)
{
    struct heimdall_logger_module *module;

    module = heimdall_custom_create(parent, sizeof (*module), "logger");
    if (unlikely(module == NULL))
        return NULL;

    /* TODO: module configuration item */
    if (heimdall_module_load(heimdall_object_logger(module), "logger", NULL, false,
                        heimdall_logger_load, module) == NULL) {
        heimdall_object_delete(HEIMDALL_OBJECT(module));
        return NULL;
    }

    module->frontend.ops = &module_ops;
    return &module->frontend;
}

/**
 * Initializes the messages logging subsystem and drain the early messages to
 * the configured log.
 */
void heimdall_LogInit(libheimdall_int_t *heimdall)
{
    struct heimdall_logger *logger = heimdall_LogModuleCreate(HEIMDALL_OBJECT(heimdall));
    if (logger == NULL)
        logger = &discard_log;

    heimdall_LogSwitch(heimdall->obj.logger, logger);
}

/**
 * Performs preinitialization of the messages logging subsystem.
 *
 * Early log messages will be stored in memory until the subsystem is fully
 * initialized with heimdall_LogInit(). This enables logging before the
 * configuration and modules bank are ready.
 *
 * \return 0 on success, -1 on error.
 */
int heimdall_LogPreinit(libheimdall_int_t *heimdall)
{
    heimdall_logger_t *logger = heimdall_LogSwitchCreate();
    if (unlikely(logger == NULL))
        return -1;
    heimdall->obj.logger = logger;

    struct heimdall_logger *early = heimdall_LogEarlyOpen(logger);
    if (early != NULL) {
        heimdall_LogSwitch(logger, early);
        heimdall_LogSpam(HEIMDALL_OBJECT(heimdall));
    }
    return 0;
}

/**
 * Message log with "header".
 */
struct heimdall_logger_header {
    struct heimdall_logger logger;
    struct heimdall_logger *parent;
    char header[];
};

static void heimdall_vaLogHeader(void *d, int type, const heimdall_log_t *item,
                            const char *format, va_list ap)
{
    struct heimdall_logger *logger = d;
    struct heimdall_logger_header *header =
        container_of(logger, struct heimdall_logger_header, logger);
    struct heimdall_logger *parent = header->parent;
    heimdall_log_t hitem = *item;

    hitem.psz_header = header->header;
    parent->ops->log(parent, type, &hitem, format, ap);
}

static const struct heimdall_logger_operations header_ops = {
    heimdall_vaLogHeader,
    free,
};

struct heimdall_logger *heimdall_LogHeaderCreate(struct heimdall_logger *parent,
                                       const char *str)
{
    if (parent == NULL)
        return NULL;

    size_t len = strlen(str) + 1;
    struct heimdall_logger_header *header = malloc(sizeof (*header) + len);
    if (unlikely(header == NULL))
        return NULL;

    header->logger.ops = &header_ops;
    header->parent = parent;
    memcpy(header->header, str, len);
    return &header->logger;
}

/**
 * External custom log callback
 */
struct heimdall_logger_external {
    struct heimdall_logger logger;
    const struct heimdall_logger_operations *ops;
    void *opaque;
};

static void heimdall_vaLogExternal(void *d, int type, const heimdall_log_t *item,
                              const char *format, va_list ap)
{
    struct heimdall_logger_external *ext = d;

    ext->ops->log(ext->opaque, type, item, format, ap);
}

static void heimdall_LogExternalClose(void *d)
{
    struct heimdall_logger_external *ext = d;

    if (ext->ops->destroy != NULL)
        ext->ops->destroy(ext->opaque);
    free(ext);
}

static const struct heimdall_logger_operations external_ops = {
    heimdall_vaLogExternal,
    heimdall_LogExternalClose,
};

static struct heimdall_logger *
heimdall_LogExternalCreate(const struct heimdall_logger_operations *ops, void *opaque)
{
    struct heimdall_logger_external *ext = malloc(sizeof (*ext));
    if (unlikely(ext == NULL))
        return NULL;

    ext->logger.ops = &external_ops;
    ext->ops = ops;
    ext->opaque = opaque;
    return &ext->logger;
}

/**
 * Sets the message logging callback.
 *
 * \param heimdall the libheimdall private instance to replace the logger for
 * \param ops message callback, or NULL to clear
 * \param opaque data pointer for the message callback
 */
void heimdall_LogSet(libheimdall_int_t *heimdall, const struct heimdall_logger_operations *ops,
                void *opaque)
{
    struct heimdall_logger *logger;

    if (ops != NULL)
        logger = heimdall_LogExternalCreate(ops, opaque);
    else
        logger = NULL;

    if (logger == NULL)
        logger = &discard_log;

    heimdall_LogSwitch(heimdall->obj.logger, logger);
    heimdall_LogSpam(HEIMDALL_OBJECT(heimdall));
}

void heimdall_LogDestroy(heimdall_logger_t *logger)
{
    logger->ops->destroy(logger);
}
