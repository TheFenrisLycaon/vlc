/*****************************************************************************
 * heimdall_pipewire.h: common PipeWire code
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

#include <stdint.h>

struct heimdall_pw_context;
struct heimdall_logger;
struct spa_dict;
struct spa_hook;
struct pw_properties;
struct pw_registry_events;

HEIMDALL_EXPORT void heimdall_pw_log(struct heimdall_pw_context *ctx, int prio,
                const char *file, unsigned int line, const char *func,
                const char *fmt, ...);
HEIMDALL_EXPORT int heimdall_pw_perror(struct heimdall_pw_context *ctx, const char *file,
                  unsigned int line, const char *func, const char *desc);

#define heimdall_pw_log(ctx, prio, ...) \
        heimdall_pw_log(ctx, prio, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define heimdall_pw_error(ctx, ...) \
        heimdall_pw_log(ctx, HEIMDALL_MSG_ERR, __VA_ARGS__)
#define heimdall_pw_warn(ctx, ...) \
        heimdall_pw_log(ctx, HEIMDALL_MSG_WARN, __VA_ARGS__)
#define heimdall_pw_debug(ctx, ...) \
        heimdall_pw_log(ctx, HEIMDALL_MSG_DBG, __VA_ARGS__)
#define heimdall_pw_perror(ctx, desc) \
        heimdall_pw_perror(ctx, __FILE__, __LINE__, __func__, desc)

HEIMDALL_EXPORT void heimdall_pw_lock(struct heimdall_pw_context *ctx);
HEIMDALL_EXPORT void heimdall_pw_unlock(struct heimdall_pw_context *ctx);
HEIMDALL_EXPORT void heimdall_pw_signal(struct heimdall_pw_context *ctx);
HEIMDALL_EXPORT void heimdall_pw_wait(struct heimdall_pw_context *ctx);

HEIMDALL_EXPORT struct pw_stream *heimdall_pw_stream_new(struct heimdall_pw_context *ctx,
                                    const char *name, struct pw_properties *);

HEIMDALL_EXPORT void heimdall_pw_roundtrip_unlocked(struct heimdall_pw_context *ctx);

HEIMDALL_EXPORT int heimdall_pw_registry_listen(struct heimdall_pw_context *ctx, struct spa_hook *hook,
                           const struct pw_registry_events *, void *);

HEIMDALL_EXPORT void heimdall_pw_disconnect(struct heimdall_pw_context *ctx);
HEIMDALL_EXPORT struct heimdall_pw_context *heimdall_pw_connect(heimdall_object_t *obj, const char *name);
