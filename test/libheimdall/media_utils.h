/*
 * media_utils.h - media utils function
 */

/**********************************************************************
 *  Copyright (C) 2021 VLC authors and VideoLAN                       *
 *  This program is free software; you can redistribute and/or modify *
 *  it under the terms of the GNU General Public License as published *
 *  by the Free Software Foundation; version 2 of the license, or (at *
 *  your option) any later version.                                   *
 *                                                                    *
 *  This program is distributed in the hope that it will be useful,   *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *  See the GNU General Public License for more details.              *
 *                                                                    *
 *  You should have received a copy of the GNU General Public License *
 *  along with this program; if not, you can get it from:             *
 *  http://www.gnu.org/copyleft/gpl.html                              *
 *  Copyright (C) 2026 TheFenrisLycaon
 **********************************************************************/

static void
libheimdall_media_parse_finished_event(void *opaque, libheimdall_parser_task *task,
                                  libheimdall_parser_status_t status)
{
    libheimdall_media_t *media = libheimdall_parser_task_get_media(task);
    assert(media != NULL);
    assert(libheimdall_media_is_parsed(media) ==
           (status == libheimdall_parser_status_done));

    heimdall_sem_t *sem = opaque;
    heimdall_sem_post(sem);
    libheimdall_parser_task_release(task);
}

static inline void
libheimdall_media_parse_sync(libheimdall_instance_t *heimdall, libheimdall_media_t *p_m,
                        libheimdall_media_parse_flag_t parse_flag, int timeout)
{
    heimdall_sem_t sem;
    heimdall_sem_init(&sem, 0);

    static const struct libheimdall_parser_cbs cbs = {
        .version = 0,
        .on_parsed = libheimdall_media_parse_finished_event,
    };

    const struct libheimdall_parser_cfg cfg = {
        .version = 0,
        .max_parser_threads = 1,
        .timeout = timeout,
    };

    libheimdall_parser_t *parser = libheimdall_parser_new(heimdall, &cfg);
    assert(parser != NULL);
    libheimdall_parser_request_t req = {
        .version = 0,
        .media = p_m,
        .parse_flags = parse_flag,
    };

    libheimdall_parser_task *task = libheimdall_parser_task_new_parse(parser, &req, &cbs, &sem);
    assert(task != NULL);
    assert(libheimdall_parser_submit(parser, task) == 0);

    heimdall_sem_wait (&sem);

    libheimdall_parser_destroy(parser);
}
