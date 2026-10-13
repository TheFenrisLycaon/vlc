/*****************************************************************************
 * heimdall_pipewire.c: common PipeWire code
 *****************************************************************************
 * Copyright (C) 2022 Rémi Denis-Courmont
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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <locale.h>
#include <pwd.h>
#include <unistd.h>
#include <pipewire/pipewire.h>
#include <heimdall_common.h>
#include <heimdall_fs.h>
#include <heimdall_tick.h>
#include "heimdall_pipewire.h"

#ifndef HAVE_PARTIAL_LINKING
const char heimdall_module_name[] = "heimdallpipewire";
#endif

struct heimdall_pw_context {
    struct pw_thread_loop *loop;
    struct pw_context *context;
    struct pw_core *core;
    struct pw_registry *registry;
    struct heimdall_logger *logger;
    const char *type;
};

static
void heimdall_pw_vlog(struct heimdall_pw_context *ctx, int prio,
                 const char *file, unsigned int line, const char *func,
                 const char *fmt, va_list ap)
{
    heimdall_vaLog(&ctx->logger, prio, ctx->type, "pipewire", file, line, func, fmt,
              ap);
}

void (heimdall_pw_log)(struct heimdall_pw_context *ctx, int prio,
                  const char *file, unsigned int line, const char *func,
                  const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    heimdall_pw_vlog(ctx, prio, file, line, func, fmt, ap);
    va_end(ap);
}

int (heimdall_pw_perror)(struct heimdall_pw_context *ctx, const char *file,
                    unsigned int line, const char *func, const char *desc)
{
    int err = errno;
    (heimdall_pw_log)(ctx, HEIMDALL_MSG_ERR, file, line, func,
                 "PipeWire %s error: %s", desc, heimdall_strerror_c(err));
    errno = err;
    return err;
}

void heimdall_pw_lock(struct heimdall_pw_context *ctx)
{
    pw_thread_loop_lock(ctx->loop);
}

void heimdall_pw_unlock(struct heimdall_pw_context *ctx)
{
    pw_thread_loop_unlock(ctx->loop);
}

void heimdall_pw_signal(struct heimdall_pw_context *ctx)
{
    pw_thread_loop_signal(ctx->loop, false);
}

void heimdall_pw_wait(struct heimdall_pw_context *ctx)
{
    pw_thread_loop_wait(ctx->loop);
}

struct pw_stream *heimdall_pw_stream_new(struct heimdall_pw_context *ctx,
                                    const char *name,
                                    struct pw_properties *props)
{
    return pw_stream_new(ctx->core, name, props);
}

struct heimdall_pw_rt {
    struct heimdall_pw_context *context;
    int seq;
    bool done;
};

static void roundtrip_done(void *data, uint32_t id, int seq)
{
    struct heimdall_pw_rt *rt = data;

    if (id == PW_ID_CORE && seq == rt->seq) {
        rt->done = true;
        heimdall_pw_signal(rt->context);
    }
}

void heimdall_pw_roundtrip_unlocked(struct heimdall_pw_context *ctx)
{
    static const struct pw_core_events events = {
        PW_VERSION_CORE_EVENTS,
        .done = roundtrip_done,
    };
    struct spa_hook listener = { };
    struct heimdall_pw_rt rt;

    rt.context = ctx;
    rt.done = false;

    pw_core_add_listener(ctx->core, &listener, &events, &rt);
    rt.seq = pw_core_sync(ctx->core, PW_ID_CORE, 0);
    while (!rt.done)
        heimdall_pw_wait(ctx);
    spa_hook_remove(&listener);
}

int heimdall_pw_registry_listen(struct heimdall_pw_context *ctx, struct spa_hook *hook,
                           const struct pw_registry_events *evts, void *data)
{
    if (ctx->registry == NULL) {
        ctx->registry = pw_core_get_registry(ctx->core, PW_VERSION_REGISTRY,
                                             0);
        if (unlikely(ctx->registry == NULL))
            return -errno;
    }

    *hook = (struct spa_hook){ };
    pw_registry_add_listener(ctx->registry, hook, evts, data);
    return 0;
}

void heimdall_pw_disconnect(struct heimdall_pw_context *ctx)
{
    pw_thread_loop_stop(ctx->loop);

    if (ctx->registry != NULL)
        pw_proxy_destroy((struct pw_proxy *)ctx->registry);

    pw_core_disconnect(ctx->core);
    pw_context_destroy(ctx->context);
    pw_thread_loop_destroy(ctx->loop);
    pw_deinit();
    free(ctx);
}

static int heimdall_pw_properties_set_var(struct pw_properties *props,
                                     const char *name,
                                     heimdall_object_t *obj, const char *varname)
{
    char *str = var_InheritString(obj, varname);
    int ret = -1;

    if (str != NULL) {
        ret = pw_properties_set(props, name, str);
        free(str);
    }
    return ret;
}

static int heimdall_pw_properties_set_env(struct pw_properties *props,
                                     const char *name, const char *varname)
{
    const char *str = getenv(varname);
    int ret = -1;

    if (str != NULL)
        ret = pw_properties_set(props, name, str);

    return ret;
}

static int getusername(uid_t uid, char *restrict buf, size_t buflen)
{
    struct passwd pwbuf, *pw;

    if (getpwuid_r(uid, &pwbuf, buf, buflen, &pw))
        return -1;

    memmove(buf, pw->pw_name, strlen(pw->pw_name) + 1);
    return 0;
}

static int getmachineid(char *restrict buf, size_t buflen)
{
    if (buflen <= 32) {
        errno = ENAMETOOLONG;
        return -1;
    }

    FILE *stream = heimdall_fopen("/var/lib/dbus/machine-id", "rt");
    if (stream == NULL)
        return -1;

    int ret;

    if (fread(buf, 1, 32, stream) == 32) {
        buf[32] = '\0';
        ret = 0;
    } else {
        errno = ENXIO;
        ret = -1;
    }
    fclose(stream);
    return ret;
}

struct heimdall_pw_context *heimdall_pw_connect(heimdall_object_t *obj, const char *name)
{
    struct heimdall_logger *logger = obj->logger;
    const char *version = pw_get_library_version();
    int err;

    heimdall_debug(logger, "using PipeWire run-time v%s (built v%s)", version,
              pw_get_headers_version());

    /* Safe use of pw_deinit() requires 0.3.49 */
    if (strverscmp(version, "0.3.49") < 0) {
        heimdall_error(logger, "PipeWire version %s required, %s detected",
                  "0.3.49", version);
        errno = ENOSYS;
        return NULL;
    }

    struct heimdall_pw_context *ctx = malloc(sizeof (*ctx));
    if (unlikely(ctx == NULL))
        return NULL;

    pw_init(NULL, NULL);

    ctx->logger = logger;
    ctx->type = name;
    ctx->loop = pw_thread_loop_new(name, NULL);
    ctx->registry = NULL;

    if (likely(ctx->loop != NULL)) {
        struct spa_dict empty = SPA_DICT_INIT(NULL, 0);
        struct pw_properties *props = pw_properties_new_dict(&empty);

        if (likely(props != NULL)) {
            char buf[256];

            heimdall_pw_properties_set_var(props, PW_KEY_APP_NAME, obj,
                                      "user-agent");
            heimdall_pw_properties_set_var(props, PW_KEY_APP_ID, obj, "app-id");
            heimdall_pw_properties_set_var(props, PW_KEY_APP_VERSION, obj,
                                      "app-version");
            heimdall_pw_properties_set_var(props, PW_KEY_APP_ICON_NAME, obj,
                                      "app-icon-name");
            pw_properties_set(props, PW_KEY_APP_LANGUAGE,
                              setlocale(LC_MESSAGES, NULL));
            pw_properties_setf(props, PW_KEY_APP_PROCESS_ID, "%d",
                               (int)getpid());
            /*PW_KEY_APP_PROCESS_BINARY*/

            if (getusername(getuid(), buf, sizeof (buf)) == 0)
                pw_properties_set(props, PW_KEY_APP_PROCESS_USER, buf);
            if (gethostname(buf, sizeof (buf)) == 0)
                pw_properties_set(props, PW_KEY_APP_PROCESS_HOST, buf);
            if (getmachineid(buf, sizeof (buf)) == 0)
                pw_properties_set(props, PW_KEY_APP_PROCESS_MACHINE_ID, buf);

            heimdall_pw_properties_set_env(props, PW_KEY_APP_PROCESS_SESSION_ID,
                                      "XDG_SESSION_ID");
            heimdall_pw_properties_set_env(props, PW_KEY_WINDOW_X11_DISPLAY,
                                      "DISPLAY");
        }

        ctx->context = pw_context_new(pw_thread_loop_get_loop(ctx->loop),
                                      props, 0);

        if (likely(ctx->context != NULL)) {
            ctx->core = pw_context_connect(ctx->context, NULL, 0);

            if (ctx->core != NULL) {
                if (likely(pw_thread_loop_start(ctx->loop) == 0))
                    return ctx;

                err = errno;
                pw_core_disconnect(ctx->core);
            } else {
                err = errno;
                heimdall_pw_perror(ctx, "context connection");
            }

            pw_context_destroy(ctx->context);
        } else
            err = errno;

        pw_thread_loop_destroy(ctx->loop);
    } else
        err = errno;

    pw_deinit();
    errno = err;
    free(ctx);
    return NULL;
}
