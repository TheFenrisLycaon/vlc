/*
 * media.c - libheimdall media smoke test
 *
 */

/**********************************************************************
 *  Copyright (C) 2010 Pierre d'Herbemont.                            *
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

#include "test.h"
#include "../lib/libheimdall_internal.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <heimdall_threads.h>
#include <heimdall_fs.h>
#include <heimdall_input_item.h>
#include <heimdall_preparser.h>

static void media_parse_ended(void *opaque, libheimdall_parser_task *task,
                              libheimdall_parser_status_t status)
{
    libheimdall_media_t *media = libheimdall_parser_task_get_media(task);
    assert(media != NULL);
    assert(libheimdall_media_is_parsed(media) ==
           (status == libheimdall_parser_status_done));

    heimdall_sem_t *sem = opaque;
    heimdall_sem_post (sem);
    libheimdall_parser_task_release(task);
}

static void print_media(libheimdall_media_t *media)
{
    static const libheimdall_track_type_t types[] =
        { libheimdall_track_audio, libheimdall_track_video, libheimdall_track_text,
          libheimdall_track_unknown };
    size_t nb_tracks = 0;

    for (size_t i = 0; i < ARRAY_SIZE(types); ++i)
    {
        const libheimdall_track_type_t type = types[i];
        libheimdall_media_tracklist_t *tracklist =
            libheimdall_media_get_tracklist(media, type);
        assert(tracklist);

        for (size_t j = 0; j < libheimdall_media_tracklist_count(tracklist); ++j)
        {
            const libheimdall_media_track_t *p_track =
                libheimdall_media_tracklist_at(tracklist, j);

            assert(p_track);
            assert(p_track->i_type == type);
            nb_tracks ++;

            test_log("\ttrack(%zu/%d): codec: %4.4s/%4.4s, ", j, p_track->i_id,
                (const char *)&p_track->i_codec,
                (const char *)&p_track->i_original_fourcc);
            switch (p_track->i_type)
            {
            case libheimdall_track_audio:
                printf("audio: channels: %u, rate: %u\n",
                       p_track->u.audio->i_channels, p_track->u.audio->i_rate);
                break;
            case libheimdall_track_video:
                printf("video: %ux%u, sar: %u/%u, fps: %u/%u\n",
                       p_track->u.video->i_width, p_track->u.video->i_height,
                       p_track->u.video->i_sar_num, p_track->u.video->i_sar_den,
                       p_track->u.video->i_frame_rate_num, p_track->u.video->i_frame_rate_den);
                break;
            case libheimdall_track_text:
                printf("text: %s\n", p_track->u.subtitle->psz_encoding);
                break;
            case libheimdall_track_unknown:
                printf("unknown\n");
                break;
            default:
                heimdall_assert_unreachable();
            }
        }
        libheimdall_media_tracklist_delete(tracklist);
    }

    if (nb_tracks == 0)
        test_log("\tmedia doesn't have any tracks\n");

    for (enum libheimdall_meta_t i = libheimdall_meta_Title;
         i <= libheimdall_meta_Compilation; ++i)
    {
        char *psz_meta = libheimdall_media_get_meta(media, i);
        if (psz_meta != NULL)
            test_log("\tmeta(%d): '%s'\n", i, psz_meta);
        free(psz_meta);
    }
}

static libheimdall_media_t *
test_media_preparsed_ext(libheimdall_instance_t *heimdall, const char *path,
                         const char *location,
                         libheimdall_media_parse_flag_t parse_flags,
                         bool b_expected_parsed)
{
    test_log ("test_media_preparsed: %s, expected: %d\n", path ? path : location,
              b_expected_parsed);

    libheimdall_media_t *media;
    if (path != NULL)
        media = libheimdall_media_new_path(path);
    else
        media = libheimdall_media_new_location(location);
    assert (media != NULL);

    heimdall_sem_t sem;
    heimdall_sem_init (&sem, 0);

    static const struct libheimdall_parser_cbs cbs = {
        .version = 0,
        .on_parsed = media_parse_ended,
    };

    const struct libheimdall_parser_cfg cfg = {
        .version = 0,
        .max_parser_threads = 1,
        .timeout = -1,
    };

    libheimdall_parser_t *parser = libheimdall_parser_new(heimdall, &cfg);
    assert(parser != NULL);

    libheimdall_parser_request_t req = {
        .version = 0,
        .media = media,
        .parse_flags = parse_flags,
    };

    libheimdall_parser_task *task = libheimdall_parser_task_new_parse(parser, &req, &cbs, &sem);
    assert(task != NULL);
    assert(libheimdall_parser_submit(parser, task) == 0);

    // Wait for preparsed event
    heimdall_sem_wait (&sem);

    // We are good, now check Elementary Stream info.
    assert (libheimdall_media_is_parsed(media) == b_expected_parsed);
    if (b_expected_parsed)
        print_media(media);

    libheimdall_parser_destroy(parser);

    return media;
}

static void test_media_preparsed(libheimdall_instance_t *heimdall, const char *path,
                                 const char *location,
                                 libheimdall_media_parse_flag_t parse_flags,
                                 bool b_expected_parsed)
{
    libheimdall_media_t *media = test_media_preparsed_ext(heimdall, path, location,
                                                     parse_flags,
                                                     b_expected_parsed);
    libheimdall_media_release (media);
}

static void test_media_tracks(libheimdall_instance_t *heimdall)
{
    const char *url =
        "mock://video_track_count=4;audio_track_count=42;"
        "video_width=100;video_height=50;"
        "audio_channels=2;audio_rate=48000;";
    libheimdall_media_t *media =
        test_media_preparsed_ext(heimdall, NULL, url,
                                 libheimdall_media_parse,
                                 true);

    char buf[32];
    libheimdall_media_tracklist_t *tracklist;

    tracklist = libheimdall_media_get_tracklist(media, libheimdall_track_video);
    assert(tracklist != NULL);
    assert(libheimdall_media_tracklist_count(tracklist) == 4);
    for (size_t i = 0; i < libheimdall_media_tracklist_count(tracklist); ++i)
    {
        libheimdall_media_track_t *track = libheimdall_media_tracklist_at(tracklist, i);
        assert(track->i_type == libheimdall_track_video);
        assert(track->u.video != NULL);
        assert(track->u.video->i_width == 100);
        assert(track->u.video->i_height == 50);

        sprintf(buf, "video/%zu", i);
        assert(strcmp(track->psz_id, buf) == 0);
        assert(track->id_stable);
    }
    libheimdall_media_tracklist_delete(tracklist);

    tracklist = libheimdall_media_get_tracklist(media, libheimdall_track_audio);
    assert(tracklist != NULL);
    assert(libheimdall_media_tracklist_count(tracklist) == 42);
    for (size_t i = 0; i < libheimdall_media_tracklist_count(tracklist); ++i)
    {
        libheimdall_media_track_t *track = libheimdall_media_tracklist_at(tracklist, i);
        assert(track->i_type == libheimdall_track_audio);
        assert(track->u.audio != NULL);
        assert(track->u.audio->i_channels == 2);
        assert(track->u.audio->i_rate == 48000);

        sprintf(buf, "audio/%zu", i);
        assert(strcmp(track->psz_id, buf) == 0);
        assert(track->id_stable);
    }
    libheimdall_media_tracklist_delete(tracklist);

    tracklist = libheimdall_media_get_tracklist(media, libheimdall_track_text);
    assert(tracklist != NULL);
    assert(libheimdall_media_tracklist_count(tracklist) == 0);
    libheimdall_media_tracklist_delete(tracklist);

    libheimdall_media_release (media);
}

static void input_item_preparse_timeout( heimdall_preparser_req *req,
                                         int status, void *user_data )
{
    (void)req;
    heimdall_sem_t *p_sem = user_data;

    assert( status == HEIMDALL_ETIMEOUT );
    heimdall_sem_post(p_sem);
}

static void input_item_preparse_cancel( heimdall_preparser_req *req,
                                        int status, void *user_data )
{
    (void)req;
    heimdall_sem_t *p_sem = user_data;

    assert( status == -EINTR );
    heimdall_sem_post(p_sem);
}

static void test_input_metadata_timeout(libheimdall_instance_t *heimdall, int timeout,
                                        int wait_and_cancel)
{
    test_log ("test_input_metadata_timeout: timeout: %d, wait_and_cancel: %d ms\n",
         timeout, wait_and_cancel);

    int i_ret, p_pipe[2];
    i_ret = heimdall_pipe(p_pipe);
    assert(i_ret == 0 && p_pipe[1] >= 0);

    char psz_fd_uri[strlen("fd://") + 11];
    sprintf(psz_fd_uri, "fd://%u", (unsigned) p_pipe[1]);
    input_item_t *p_item = input_item_NewFile(psz_fd_uri, "test timeout", 0,
                                              ITEM_LOCAL);
    assert(p_item != NULL);

    heimdall_sem_t sem;
    heimdall_sem_init (&sem, 0);
    const struct heimdall_preparser_cbs cbs = {
        .on_ended = wait_and_cancel > 0 ? input_item_preparse_cancel
                                        : input_item_preparse_timeout,
    };

    int options = HEIMDALL_PREPARSER_TYPE_PARSE | HEIMDALL_PREPARSER_TYPE_FETCHMETA_LOCAL;

    const struct heimdall_preparser_cfg cfg = {
        .types = options,
        .max_parser_threads = 1,
        .timeout = HEIMDALL_TICK_FROM_MS(timeout),
        .external_process = false,
    };
    heimdall_preparser_t *parser = heimdall_preparser_New(HEIMDALL_OBJECT(heimdall->p_libheimdall_int),
                                                &cfg);
    assert(parser != NULL);
    heimdall_preparser_req *req =
        heimdall_preparser_req_NewParse(parser, p_item, options, &cbs, &sem);
    assert(req != NULL);
    assert(heimdall_preparser_Submit(parser, req) == HEIMDALL_SUCCESS);

    if (wait_and_cancel > 0)
    {
        heimdall_tick_sleep( HEIMDALL_TICK_FROM_MS(wait_and_cancel) );
        size_t count = heimdall_preparser_Cancel(parser, req);
        assert(count == 1);
    }
    heimdall_sem_wait(&sem);

    heimdall_preparser_req_Release(req);

    input_item_Release(p_item);
    heimdall_preparser_Delete(parser);
    heimdall_close(p_pipe[0]);
    heimdall_close(p_pipe[1]);
}

static struct
{
    const char *file;
    libheimdall_media_type_t type;
} test_media_subitems_list[] =
{
    { "directory", libheimdall_media_type_directory, },
    { "file.jpg", libheimdall_media_type_file },
    { "file.mkv", libheimdall_media_type_file },
    { "file.mp3", libheimdall_media_type_file },
    { "file.png", libheimdall_media_type_file },
    { "file.ts", libheimdall_media_type_file },
};

static void check_subitems(libheimdall_media_t *m, bool *subitems_found)
{
#ifdef _WIN32
#define FILE_SEPARATOR   '\\'
#else
#define FILE_SEPARATOR   '/'
#endif
    assert (m);

    char *mrl = libheimdall_media_get_mrl (m);
    assert (mrl);

    const char *file = strrchr (mrl, FILE_SEPARATOR);
    assert (file);
    file++;
    test_log ("subitem_added, file: %s\n", file);

    for (unsigned i = 0; i < ARRAY_SIZE(test_media_subitems_list); ++i)
    {
        if (strcmp (test_media_subitems_list[i].file, file) == 0)
        {
            assert (!subitems_found[i]);
            assert (libheimdall_media_get_type(m) == test_media_subitems_list[i].type);
            subitems_found[i] = true;

            uint64_t value;
            int ret;

            ret = libheimdall_media_get_filestat(m, libheimdall_media_filestat_size, &value);
            assert(ret == 1);
            /* All files size are 0, directory size depends on the FS. */
            if (libheimdall_media_get_type(m) != libheimdall_media_type_directory)
                assert(value == 0);

            ret = libheimdall_media_get_filestat(m, libheimdall_media_filestat_mtime, &value);
            /* Epoch timestamp of the commit that added test/samples/subitems */
            assert(ret == 1 && value >= 1446796477);
        }
    }
    free (mrl);
#undef FILE_SEPARATOR
}

static void on_media_parsed(void *opaque, libheimdall_media_t *media)
{
    (void) media;
    heimdall_sem_t *sem = opaque;
    heimdall_sem_post(sem);
}

static void on_media_subitems_changed(void *opaque, libheimdall_media_t *media)
{
    (void) media;
    heimdall_sem_t *sem = opaque;
    heimdall_sem_post(sem);
}

static void test_media_subitems_media(libheimdall_instance_t *heimdall,
                                      libheimdall_media_t *media, bool play,
                                      bool b_items_expected)
{
    libheimdall_media_add_option(media, ":ignore-filetypes= ");
    libheimdall_media_add_option(media, ":no-sub-autodetect-file");

    bool subitems_found[ARRAY_SIZE(test_media_subitems_list)] = { 0 };
    heimdall_sem_t sem;
    heimdall_sem_init (&sem, 0);

    if (play)
    {
        /* XXX: libheimdall_media_parse won't work with fd, since it
         * won't be preparsed because fd:// is an unknown type, so play the
         * file to force parsing. */

        static const struct libheimdall_media_player_cbs cbs = {
            .version = 0,
            .on_media_parsed = on_media_parsed,
            .on_media_subitems_changed = on_media_subitems_changed,
        };

        libheimdall_media_player_t *mp =
            libheimdall_media_player_new_from_media(heimdall, media, &cbs, &sem);
        assert (mp);
        assert (libheimdall_media_player_play (mp) != -1);

        heimdall_sem_wait(&sem);
        heimdall_sem_wait(&sem);

        libheimdall_media_player_release (mp);
    }
    else
    {
        static const struct libheimdall_parser_cbs cbs = {
            .version = 0,
            .on_parsed = media_parse_ended,
        };

        const struct libheimdall_parser_cfg cfg = {
            .version = 0,
            .max_parser_threads = 1,
            .timeout = -1,
        };

        libheimdall_parser_t *parser = libheimdall_parser_new(heimdall, &cfg);
        assert(parser != NULL);
        libheimdall_parser_request_t req = {
            .version = 0,
            .media = media,
            .parse_flags = 0,
        };
        libheimdall_parser_task *task = libheimdall_parser_task_new_parse(parser, &req, &cbs, &sem);
        assert(task != NULL);
        assert(libheimdall_parser_submit(parser, task) == 0);

        heimdall_sem_wait (&sem);
        libheimdall_parser_destroy(parser);
    }

    libheimdall_media_list_t *subitems = libheimdall_media_subitems(media);
    assert(subitems != NULL);
    for (int i = 0; i < libheimdall_media_list_count(subitems); ++i)
    {
        libheimdall_media_t *child = libheimdall_media_list_item_at_index(subitems, i);
        assert(child != NULL);
        check_subitems(child, subitems_found);
        libheimdall_media_release(child);
    }
    libheimdall_media_list_release(subitems);

    if (!b_items_expected)
        return;

    for (unsigned i = 0; i < ARRAY_SIZE(test_media_subitems_list); ++i)
    {
        test_log ("test if %s was added\n", test_media_subitems_list[i].file);
        assert (subitems_found[i]);
    }
}

static void test_media_subitems(libheimdall_instance_t *heimdall)
{
    const char *subitems_path = SRCDIR"/samples/subitems";

    libheimdall_media_t *media;

    test_log ("Testing media_subitems: path: '%s'\n", subitems_path);
    media = libheimdall_media_new_path(subitems_path);
    assert (media != NULL);
    test_media_subitems_media(heimdall, media, false, true);
    libheimdall_media_release (media);

    char *subitems_realpath = realpath (subitems_path, NULL);
    assert (subitems_realpath != NULL);
    const char *schemes[] = { "file://", "dir://" };
    for (unsigned i = 0; i < ARRAY_SIZE(schemes); ++i)
    {
        char *location;
        assert (asprintf (&location, "%s%s", schemes[i], subitems_realpath) != -1);
        test_log ("Testing media_subitems: location: '%s'\n", location);
        media = libheimdall_media_new_location(location);
        assert (media != NULL);
        test_media_subitems_media(heimdall, media, false, true);
        free (location);
        libheimdall_media_release (media);
    }
    free (subitems_realpath);

#ifdef HAVE_FSTATAT
    /* listing directory via a fd works only if fstatat() exists */
    int fd = open (subitems_path, O_RDONLY);
    test_log ("Testing media_subitems: fd: '%d'\n", fd);
    assert (fd >= 0);
    media = libheimdall_media_new_fd(fd);
    assert (media != NULL);
    test_media_subitems_media(heimdall, media, true, true);
    libheimdall_media_release (media);
    heimdall_close (fd);
#else
#warning not testing subitems list via a fd location
#endif

    test_log ("Testing media_subitems failure\n");
    media = libheimdall_media_new_location("wrongfile://test");
    assert (media != NULL);
    test_media_subitems_media(heimdall, media, false, false);
    libheimdall_media_release (media);
}

int main(int i_argc, char *ppsz_argv[])
{
    test_init();

    libheimdall_instance_t *heimdall = libheimdall_new (test_defaults_nargs,
                                         test_defaults_args);
    assert (heimdall != NULL);

    char *psz_test_arg = i_argc > 1 ? ppsz_argv[1] : NULL;
    if (psz_test_arg != NULL)
    {
        alarm(0);
        const char *psz_test_url;
        const char *psz_test_path;
        if (strstr(psz_test_arg, "://") != NULL)
        {
            psz_test_url = psz_test_arg;
            psz_test_path = NULL;
        }
        else
        {
            psz_test_url = NULL;
            psz_test_path = psz_test_arg;
        }
        test_media_preparsed (heimdall, psz_test_path, psz_test_url,
                              libheimdall_media_parse,
                              true);
        libheimdall_release(heimdall);
        return 0;
    }

    test_media_preparsed (heimdall, SRCDIR"/samples/image.jpg", NULL,
                          libheimdall_media_parse,
                          true);

    test_media_subitems (heimdall);
    test_media_tracks (heimdall);

    /* Testing heimdall_preparser_Submit timeout and heimdall_preparser_Cancel. For
     * that, we need to create a local input_item_t based on a pipe. There is
     * no way to do that with a libheimdall_media_t, that's why we don't use
     * libheimdall_media_parse*() */

    test_input_metadata_timeout (heimdall, 100, 0);
    test_input_metadata_timeout (heimdall, 0, 100);

    libheimdall_release (heimdall);

    return 0;
}
