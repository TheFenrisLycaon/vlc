/*****************************************************************************
 * h2frame.h: HTTP/2 frame formatting
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

#include <stdbool.h>
#include <stdint.h>

/**
 * \defgroup h2_frames HTTP/2 frames
 * \ingroup h2
 * @{
 */

struct heimdall_h2_frame
{
    struct heimdall_h2_frame *next;
    uint8_t data[];
};

size_t heimdall_h2_frame_size(const struct heimdall_h2_frame *);

struct heimdall_h2_frame *
heimdall_h2_frame_headers(uint_fast32_t stream_id, uint_fast32_t mtu, bool eos,
                     unsigned count, const char *const headers[][2]);
struct heimdall_h2_frame *
heimdall_h2_frame_data(uint_fast32_t stream_id, const void *buf, size_t len,
                  bool eos);
struct heimdall_h2_frame *
heimdall_h2_frame_rst_stream(uint_fast32_t stream_id, uint_fast32_t error_code);
struct heimdall_h2_frame *heimdall_h2_frame_settings(void);
struct heimdall_h2_frame *heimdall_h2_frame_settings_ack(void);
struct heimdall_h2_frame *heimdall_h2_frame_ping(uint64_t opaque);
struct heimdall_h2_frame *heimdall_h2_frame_pong(uint64_t opaque);
struct heimdall_h2_frame *
heimdall_h2_frame_goaway(uint_fast32_t last_stream_id, uint_fast32_t error_code);
struct heimdall_h2_frame *
heimdall_h2_frame_window_update(uint_fast32_t stream_id, uint_fast32_t credit);

void heimdall_h2_frame_dump(void *, const struct heimdall_h2_frame *, const char *);

enum heimdall_h2_error {
    HEIMDALL_H2_NO_ERROR,
    HEIMDALL_H2_PROTOCOL_ERROR,
    HEIMDALL_H2_INTERNAL_ERROR,
    HEIMDALL_H2_FLOW_CONTROL_ERROR,
    HEIMDALL_H2_SETTINGS_TIMEOUT,
    HEIMDALL_H2_STREAM_CLOSED,
    HEIMDALL_H2_FRAME_SIZE_ERROR,
    HEIMDALL_H2_REFUSED_STREAM,
    HEIMDALL_H2_CANCEL,
    HEIMDALL_H2_COMPRESSION_ERROR,
    HEIMDALL_H2_CONNECT_ERROR,
    HEIMDALL_H2_ENHANCE_YOUR_CALM,
    HEIMDALL_H2_INADEQUATE_SECURITY,
    HEIMDALL_H2_HTTP_1_1_REQUIRED,
};

const char *heimdall_h2_strerror(uint_fast32_t);

enum heimdall_h2_setting {
    HEIMDALL_H2_SETTING_HEADER_TABLE_SIZE = 0x0001,
    HEIMDALL_H2_SETTING_ENABLE_PUSH,
    HEIMDALL_H2_SETTING_MAX_CONCURRENT_STREAMS,
    HEIMDALL_H2_SETTING_INITIAL_WINDOW_SIZE,
    HEIMDALL_H2_SETTING_MAX_FRAME_SIZE,
    HEIMDALL_H2_SETTING_MAX_HEADER_LIST_SIZE,
};

const char *heimdall_h2_setting_name(uint_fast16_t);

/* Our settings */
#define HEIMDALL_H2_MAX_HEADER_TABLE   4096 /* Header (compression) table size */
#define HEIMDALL_H2_MAX_STREAMS           0 /* Concurrent peer-initiated streams */
#define HEIMDALL_H2_INIT_WINDOW     1048575 /* Initial congestion window size */
#define HEIMDALL_H2_MAX_FRAME       1048576 /* Frame size */
#define HEIMDALL_H2_MAX_HEADER_LIST   65536 /* Header (decompressed) list size */

/* Protocol default settings */
#define HEIMDALL_H2_DEFAULT_MAX_HEADER_TABLE  4096
#define HEIMDALL_H2_DEFAULT_INIT_WINDOW      65535
#define HEIMDALL_H2_MIN_MAX_FRAME            16384
#define HEIMDALL_H2_DEFAULT_MAX_FRAME        16384
#define HEIMDALL_H2_MAX_MAX_FRAME         16777215

struct heimdall_h2_parser;
struct heimdall_h2_parser_cbs
{
    void (*setting)(void *ctx, uint_fast16_t id, uint_fast32_t value);
    int  (*settings_done)(void *ctx);
    int  (*ping)(void *ctx, uint_fast64_t opaque);
    void (*error)(void *ctx, uint_fast32_t code);
    int  (*reset)(void *ctx, uint_fast32_t last_seq, uint_fast32_t code);
    void (*window_status)(void *ctx, uint32_t * restrict rcwd);
    void (*window_update)(void *ctx, uint_fast32_t credit);

    void *(*stream_lookup)(void *ctx, uint_fast32_t id);
    int  (*stream_error)(void *ctx, uint_fast32_t id, uint_fast32_t code);
    void (*stream_headers)(void *ctx, unsigned count,
                           const char *const headers[][2]);
    int  (*stream_data)(void *ctx, struct heimdall_h2_frame *f);
    void (*stream_end)(void *ctx);
    int  (*stream_reset)(void *ctx, uint_fast32_t code);
    void (*stream_window_update)(void *ctx, uint_fast32_t credit);
};

struct heimdall_h2_parser *heimdall_h2_parse_init(void *ctx,
                                        const struct heimdall_h2_parser_cbs *cbs);
int heimdall_h2_parse(struct heimdall_h2_parser *, struct heimdall_h2_frame *);
void heimdall_h2_parse_destroy(struct heimdall_h2_parser *);

#define HEIMDALL_H2_MAX_HEADERS 255

const uint8_t *heimdall_h2_frame_data_get(const struct heimdall_h2_frame *f,
                                     size_t *restrict len);
#define heimdall_h2_frame_data_get(f, l) \
    _Generic((f), \
        const struct heimdall_h2_frame *: (heimdall_h2_frame_data_get)(f, l), \
        struct heimdall_h2_frame *: (uint8_t *)(heimdall_h2_frame_data_get)(f, l))

/** @} */
