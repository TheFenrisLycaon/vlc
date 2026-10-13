/*****************************************************************************
 * directory.c: expands a directory (directory: access_browser plug-in)
 *****************************************************************************
 * Copyright (C) 2002-2015 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Derk-Jan Hartman <hartman at lycaria dot org>
 *          Rémi Denis-Courmont
 *          Julien 'Lta' BALLET <contact # lta.io>
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

#include <limits.h>
#include <sys/stat.h>

#include <heimdall_common.h>
#include "fs.h"
#include <heimdall_access.h>
#include <heimdall_input_item.h>

#include <heimdall_fs.h>
#include <heimdall_url.h>

typedef struct
{
    char *base_uri;
    bool need_separator;
    heimdall_DIR *dir;
} access_sys_t;

static int DirRead (stream_t *access, input_item_node_t *node);

static int DirControl( stream_t *p_access, int i_query, va_list args )
{
    switch ( i_query )
    {
        case STREAM_GET_TYPE:
        {
            *va_arg( args, int* ) = ITEM_TYPE_DIRECTORY;
            return HEIMDALL_SUCCESS;
        }
        default:
            return access_vaDirectoryControlHelper( p_access, i_query, args );
    }
}

/*****************************************************************************
 * DirInit: Init the directory access with a directory stream
 *****************************************************************************/
int DirInit (stream_t *access, heimdall_DIR *dir)
{
    access_sys_t *sys = heimdall_obj_malloc(HEIMDALL_OBJECT(access), sizeof (*sys));
    if (unlikely(sys == NULL))
        goto error;

    if (!strcmp(access->psz_name, "fd"))
    {
        if (unlikely(asprintf(&sys->base_uri, "fd://%s",
                              access->psz_location) == -1))
            sys->base_uri = NULL;
    }
    else
        sys->base_uri = heimdall_path2uri(access->psz_filepath, "file");
    if (unlikely(sys->base_uri == NULL))
        goto error;

    char last_char = sys->base_uri[strlen(sys->base_uri) - 1];
    sys->need_separator =
#ifdef _WIN32
            last_char != '\\' &&
#endif
            last_char != '/';
    sys->dir = dir;

    access->p_sys = sys;
    access->pf_readdir = DirRead;
    access->pf_control = DirControl;
    return HEIMDALL_SUCCESS;

error:
    heimdall_closedir(dir);
    return HEIMDALL_ENOMEM;
}

/*****************************************************************************
 * DirOpen: Open the directory access
 *****************************************************************************/
int DirOpen (heimdall_object_t *obj)
{
    stream_t *access = (stream_t *)obj;

    if (access->psz_filepath == NULL)
        return HEIMDALL_EGENERIC;

    heimdall_DIR *dir = heimdall_opendir(access->psz_filepath);
    if (dir == NULL)
        return HEIMDALL_EGENERIC;

    return DirInit(access, dir);
}

/*****************************************************************************
 * Close: close the target
 *****************************************************************************/
void DirClose(heimdall_object_t *obj)
{
    stream_t *access = (stream_t *)obj;
    access_sys_t *sys = access->p_sys;

    free(sys->base_uri);
    heimdall_closedir(sys->dir);
}

static int DirRead (stream_t *access, input_item_node_t *node)
{
    access_sys_t *sys = access->p_sys;
    const char *entry;
    int ret = HEIMDALL_SUCCESS;

    bool special_files = var_InheritBool(access, "list-special-files");

    struct heimdall_readdir_helper rdh;
    heimdall_readdir_helper_init(&rdh, access, node);

    while (ret == HEIMDALL_SUCCESS && (entry = heimdall_readdir(sys->dir)) != NULL)
    {
        struct stat st;
        int type;

#ifdef HAVE_FSTATAT
        if (fstatat(dirfd(sys->dir), entry, &st, 0))
            continue;
#else
        char *path;

        if (asprintf(&path, "%s"DIR_SEP"%s", access->psz_filepath, entry) == -1
         || (type = heimdall_stat(path, &st), free(path), type))
            continue;
#endif
        switch (st.st_mode & S_IFMT)
        {
#ifdef S_IFBLK
            case S_IFBLK:
                if (!special_files)
                    continue;
                type = ITEM_TYPE_DISC;
                break;
#endif
            case S_IFCHR:
                if (!special_files)
                    continue;
                type = ITEM_TYPE_CARD;
                break;
            case S_IFIFO:
                if (!special_files)
                    continue;
                type = ITEM_TYPE_STREAM;
                break;
            case S_IFREG:
                type = ITEM_TYPE_FILE;
                break;
            case S_IFDIR:
                type = ITEM_TYPE_DIRECTORY;
                break;
            /* S_IFLNK cannot occur while following symbolic links */
            /* S_IFSOCK cannot be opened with open()/openat() */
            default:
                continue; /* ignore */
        }

        /* Create an input item for the current entry */
        char *encoded = heimdall_uri_encode(entry);
        if (unlikely(encoded == NULL))
        {
            ret = HEIMDALL_ENOMEM;
            break;
        }

        char *uri;
        if (unlikely(asprintf(&uri, "%s%s%s", sys->base_uri,
                              sys->need_separator ? "/" : "",
                              encoded) == -1))
            uri = NULL;
        free(encoded);
        if (unlikely(uri == NULL))
        {
            ret = HEIMDALL_ENOMEM;
            break;
        }
        input_item_t *p_item;
        ret = heimdall_readdir_helper_additem(&rdh, uri, NULL, entry, type,
                                         ITEM_NET_UNKNOWN, &p_item);

        if (ret == HEIMDALL_SUCCESS && p_item && st.st_mtime >= 0 && st.st_size >= 0)
        {
            input_item_AddStat( p_item, "mtime", st.st_mtime );
            input_item_AddStat( p_item, "size", st.st_size );
        }
        free(uri);
    }

    heimdall_readdir_helper_finish(&rdh, ret == HEIMDALL_SUCCESS);

    return ret;
}
