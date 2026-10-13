/*****************************************************************************
 * ytdl.c:
 *****************************************************************************
 * Copyright (C) 2019-2020 Rémi Denis-Courmont
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
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>         /* STDERR_FILENO */
#ifdef _WIN32
# include <processthreadsapi.h>
#endif

#include "json/json.h"
#include <heimdall_common.h>
#include <heimdall_configuration.h>
#include <heimdall_demux.h>
#include <heimdall_stream.h>
#include <heimdall_fs.h>
#include <heimdall_input_item.h>
#include <heimdall_plugin.h>
#include <heimdall_spawn.h>
#include <heimdall_strings.h>
#include <heimdall_interrupt.h>

struct ytdl_json {
    struct heimdall_logger *logger;
    int fd;
};

void json_parse_error(void *data, const char *msg)
{
    struct ytdl_json *sys = data;

    heimdall_error(sys->logger, "%s", msg);
}

size_t json_read(void *data, void *buf, size_t size)
{
    struct ytdl_json *sys = data;

    while (!heimdall_killed()) {
        ssize_t val = heimdall_read_i11e(sys->fd, buf, size);

        if (val >= 0)
            return val;
    }

    return 0;
}

static int ytdl_popen(pid_t *restrict pid, const char *argv[])
{
    int fds[2];

    if (heimdall_pipe(fds))
        return -1;

    int fdv[] = { -1, fds[1], STDERR_FILENO, -1 };
    int val = heimdall_spawnp(pid, argv[0], fdv, argv);

    heimdall_close(fds[1]);

    if (val) {
        heimdall_close(fds[0]);
        errno = val;
        return -1;
    }

    return fds[0];
}

#ifdef _WIN32
static int heimdall_kill(pid_t pid, int signum)
{
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    BOOL res;

    if (h == NULL) {
        errno = ESRCH;
        return -1;
    }

    if (signum > 0)
        res = TerminateProcess(h, signum);
    else
        res = TRUE;

    CloseHandle(h);

    if (!res) {
        errno = EINVAL;
        return -1;
    }

    return 0;
}
#define kill(p, s) heimdall_kill(p, s)
#endif

struct ytdl_playlist {
    struct json_object json;
    stream_t *source;
};

static int CompareFormats(const struct json_object *f_a,
                          const struct json_object *f_b, double pref_height)
{
    double h_a = json_get_num(f_a, "height");
    double abr_a = json_get_num(f_a, "abr");
    double h_b = json_get_num(f_b, "height");
    double abr_b = json_get_num(f_b, "abr");

    /* Prefer non-mute formats */
    if (!isnan(abr_a) != !isnan(abr_b))
        return isnan(abr_a) ? -1 : +1;

    /* Prefer non-blind formats */
    if (!isnan(h_a) != !isnan(h_b))
        return isnan(h_a) ? -1 : +1;

    if (islessequal(h_a, pref_height)) {
        if (!islessequal(h_b, pref_height))
            return +1;
        if (h_a > h_b)
            return -1;
        if (h_a < h_b)
            return +1;
    } else {
        if (islessequal(h_b, pref_height))
            return -1;
        if (isless(h_a, h_b))
            return -1;
        if (isgreater(h_a, h_b))
            return +1;
    }

    if (isgreater(abr_a, abr_b))
        return +1;
    if (isgreater(abr_b, abr_a))
        return -1;

    return 0;
}

static const struct json_object *PickFormat(stream_t *s,
                                            const struct json_object *entry)
{
     const struct json_value *fmts = json_get(entry, "formats");

     if (fmts == NULL)
         return entry; /* only one format */
     if (fmts->type != JSON_ARRAY)
         return NULL;

     const struct json_object *best_fmt = NULL;
     double pref_height = var_InheritInteger(s, "preferred-resolution");

     if (isless(pref_height, 0.))
         pref_height = NAN;

     for (size_t i = 0; i < fmts->array.size; i++) {
         const struct json_value *v = &fmts->array.entries[i];

         if (v->type != JSON_OBJECT)
             continue;

         const struct json_object *fmt = &v->object;

         if (best_fmt == NULL) {
             best_fmt = fmt;
             continue;
         }

         if (CompareFormats(fmt, best_fmt, pref_height) > 0)
              best_fmt = fmt;
     }

     return best_fmt;
}

static const char *PickArt(const struct json_object *entry)
{
    const struct json_value *v = json_get(entry, "thumbnails");

    if (v == NULL || v->type != JSON_ARRAY || v->array.size == 0)
        return NULL;

    v = &v->array.entries[0];

    if (v->type != JSON_OBJECT)
        return NULL;

    return json_get_str(&v->object, "url");
}

static void GetMeta(heimdall_meta_t *meta, const struct json_object *json)
{
    const char *title = json_get_str(json, "title");
    if (title != NULL)
        heimdall_meta_Set(meta, heimdall_meta_Title, title);

    const char *desc = json_get_str(json, "description");
    if (desc != NULL)
        heimdall_meta_Set(meta, heimdall_meta_Description, desc);

    const char *author = json_get_str(json, "uploader");
    if (author != NULL)
        heimdall_meta_Set(meta, heimdall_meta_Artist, author);

    const char *arturl = PickArt(json);
    if (arturl != NULL)
        heimdall_meta_Set(meta, heimdall_meta_ArtworkURL, arturl);
}

static int ReadItem(stream_t *s, input_item_node_t *node,
                    const struct json_object *json)
{
    const struct json_object *fmt = PickFormat(s, json);

    if (fmt == NULL)
        return HEIMDALL_EGENERIC;

    const char *url = json_get_str(fmt, "url");

    if (url == NULL)
        return HEIMDALL_EGENERIC;

    const char *title = json_get_str(json, "title");
    double duration = json_get_num(json, "duration");
    heimdall_tick_t ticks = isnan(duration) ? INPUT_DURATION_UNSET
                                       : lround(duration * CLOCK_FREQ);
    char *wrapurl;

    if (title == NULL)
        title = url;
    if (unlikely(asprintf(&wrapurl, "ytdl://%s", url) == -1))
        return HEIMDALL_ENOMEM;

    input_item_t *item = input_item_NewStream(wrapurl, title, ticks);

    free(wrapurl);

    if (unlikely(item == NULL))
        return HEIMDALL_ENOMEM;

    /* Don't care to lock, the item is still private. */
    GetMeta(item->p_meta, json);
    input_item_node_AppendItem(node, item);
    input_item_Release(item);

    return HEIMDALL_SUCCESS;
}

static int ReadDir(stream_t *s, input_item_node_t *node)
{
    struct ytdl_playlist *sys = s->p_sys;
    const struct json_value *v = json_get(&sys->json, "entries");

    if (v == NULL) /* Single item */
        return ReadItem(s, node, &sys->json);

    /* Playlist: parse each entry */
    if (v->type != JSON_ARRAY)
        return HEIMDALL_EGENERIC;

    for (size_t i = 0; i < v->array.size; i++) {
         const struct json_value *e = &v->array.entries[i];

         if (e->type == JSON_OBJECT)
             ReadItem(s, node, &e->object);
    }

    return HEIMDALL_SUCCESS;
}

static int Control(stream_t *s, int query, va_list args)
{
    switch (query)
    {
        case STREAM_CAN_SEEK:
        case STREAM_CAN_FASTSEEK:
        case STREAM_CAN_PAUSE:
        case STREAM_CAN_CONTROL_PACE:
            *va_arg(args, bool *) = false;
            break;

        case STREAM_GET_TYPE:
            *va_arg(args, int *) = ITEM_TYPE_PLAYLIST;
            break;

        case STREAM_GET_PTS_DELAY:
            *va_arg(args, heimdall_tick_t *) =
                 HEIMDALL_TICK_FROM_MS(var_InheritInteger(s, "network-caching"));
            break;

        default:
            return HEIMDALL_EGENERIC;

    }

    return HEIMDALL_SUCCESS;
}

static int DemuxNested(stream_t *s)
{
    struct ytdl_playlist *sys = s->p_sys;

    return demux_Demux(sys->source);
}

static int ControlNested(stream_t *s, int query, va_list args)
{
    struct ytdl_playlist *sys = s->p_sys;

    switch (query) {
        case DEMUX_GET_META: {
            heimdall_meta_t *meta = va_arg(args, heimdall_meta_t *);

            GetMeta(meta, &sys->json);
            return demux_Control(sys->source, query, meta);
        }

        default:
            return demux_vaControl(sys->source, query, args);
    }
}

static stream_t *heimdall_demux_NewURL(heimdall_object_t *obj, const char *url,
                                  es_out_t *out)
{
    stream_t *stream = heimdall_stream_NewURL(obj, url);

    if (stream != NULL) {
        demux_t *demux = demux_New(obj, "any", url, stream, out);

        if (demux != NULL)
            return demux;

        heimdall_stream_Delete(stream);
    }

    return NULL;
}

static void Close(heimdall_object_t *obj)
{
    stream_t *s = (stream_t *)obj;
    struct ytdl_playlist *sys = s->p_sys;

    if (sys->source != NULL)
        heimdall_stream_Delete(sys->source);

    json_free(&sys->json);
}

static int OpenCommon(heimdall_object_t *obj, const char *src_url)
{
    stream_t *s = (stream_t *)obj;

    struct ytdl_playlist *sys = heimdall_obj_malloc(obj, sizeof (*sys));
    if (unlikely(sys == NULL))
        return HEIMDALL_EGENERIC;

    char *path = var_InheritString(obj, "ytdl-path");
    if (unlikely(path == NULL))
        return HEIMDALL_EINVAL;

    struct ytdl_json jsdata;
    pid_t pid;
    const char *argv[] = {
        path, "--flat-playlist", "--dump-single-json", "--", src_url, NULL
    };

    jsdata.logger = s->obj.logger;
    jsdata.fd = ytdl_popen(&pid, argv);

    if (jsdata.fd == -1) {
        msg_Dbg(obj, "cannot start %s: %s", path, heimdall_strerror_c(errno));
        free(path);
        return HEIMDALL_EGENERIC;
    }

    free(path);

    int val = json_parse(&jsdata, &sys->json);

    kill(pid, SIGTERM);
    heimdall_close(jsdata.fd);
    heimdall_waitpid(pid);

    if (val) {
        /* Location not handled */
        msg_Dbg(s, "cannot extract infos");
        return HEIMDALL_EGENERIC;
    }

    s->p_sys = sys;
    sys->source = NULL;

    if (json_get(&sys->json, "entries") != NULL) {
        /* Playlist */
        s->pf_readdir = ReadDir;
        s->pf_control = Control;
        return HEIMDALL_SUCCESS;
    }

    /* Redirect if there is a single URL, so that we can refresh it every
     * time it is opened.
     */
    const struct json_object *fmt = PickFormat(s, &sys->json);
    stream_t *demux = NULL;

    if (fmt != NULL) {
        const char *url = json_get_str(fmt, "url");

        if (url != NULL) {
            var_Create(obj, "ytdl", HEIMDALL_VAR_BOOL);
            demux = heimdall_demux_NewURL(obj, url, s->out);

            if (demux == NULL)
                msg_Err(s, "cannot open URL: %s", url);
            else
                msg_Dbg(s, "redirecting to: %s", url);
         }
    }

    if (demux == NULL) {
        json_free(&sys->json);
        return HEIMDALL_EGENERIC;
    }

    s->pf_demux = DemuxNested;
    s->pf_control = ControlNested;
    sys->source = demux;
    return HEIMDALL_SUCCESS;
}

static int OpenFilter(heimdall_object_t *obj)
{
    stream_t *s = (stream_t *)obj;

    if (s->psz_url == NULL)
        return HEIMDALL_EGENERIC;
    if (strncasecmp(s->psz_url, "http:", 5)
     && strncasecmp(s->psz_url, "https:", 6))
        return HEIMDALL_EGENERIC;
    if (!var_InheritBool(obj, "ytdl"))
        return HEIMDALL_EGENERIC;

    return OpenCommon(obj, s->psz_url);
}

static int OpenAccess(heimdall_object_t *obj)
{
    stream_t *s = (stream_t *)obj;
    const char *url = s->psz_url;

    assert(url != NULL);

    if (heimdall_ascii_strncasecmp(url, "ytdl://", 7) == 0)
        url += 7;

    return OpenCommon(obj, url);
}

heimdall_module_begin()
    set_shortname("YT-DL")
    set_description("YT-DL extractor")
    set_subcategory(SUBCAT_INPUT_STREAM_FILTER)
    set_capability("demux", 7)
    set_callbacks(OpenFilter, Close)
    add_bool("ytdl", true, N_("Enable YT-DL"), NULL)
        change_safe()
    add_loadfile("ytdl-path", "yt-dlp" EXEEXT,
                 N_("Path to YT-DLP"), N_("Path to YT-DLP"))

    add_submodule()
    set_capability("access", 0)
    add_shortcut("ytdl")
    set_callbacks(OpenAccess, Close)
heimdall_module_end()
