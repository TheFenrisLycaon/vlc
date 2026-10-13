/*****************************************************************************
 * dirs.c: XDG directories configuration
 *****************************************************************************
 * Copyright (C) 2001-2007 VLC authors and VideoLAN
 * Copyright © 2007-2009 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Gildas Bazin <gbazin@videolan.org>
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

#include <heimdall_common.h>

#include "../libheimdall.h"
#include "../config/configuration.h"

#include <unistd.h>
#include <pwd.h>
#include <assert.h>
#include <limits.h>

/**
 * Determines the architecture-dependent data directory
 *
 * @return a string (always succeeds).
 */
HEIMDALL_WEAK char *config_GetLibDir(void)
{
    return strdup(LIBDIR);
}

char *config_GetSysPath(heimdall_sysdir_t type, const char *filename)
{
    static const char env_vars[][17] = {
        [HEIMDALL_PKG_DATA_DIR] = "HEIMDALL_DATA_PATH",
        [HEIMDALL_PKG_LIB_DIR] = "HEIMDALL_LIB_PATH",
        [HEIMDALL_PKG_LIBEXEC_DIR] = "HEIMDALL_LIBEXEC_PATH",
    };

    if (type < ARRAY_SIZE(env_vars)) {
        const char *name = env_vars[type];
         if (*name != '\0') {
             const char *value = getenv(name);
             if (value != NULL) {
                 const char *fmt = (filename != NULL) ? "%s/%s" : "%s";
                 char *filepath;

                 if (unlikely(asprintf(&filepath, fmt, value, filename) == -1))
                     filepath = NULL;
                 return filepath;
             }
         }
    }

    char *libdir = config_GetLibDir();
    if (libdir == NULL)
        return NULL; /* OOM */

    static const char *const dirs[] = {
        [HEIMDALL_PKG_DATA_DIR] = PKGDATADIR,
        [HEIMDALL_PKG_LIB_DIR] = PKGLIBDIR,
        [HEIMDALL_PKG_LIBEXEC_DIR] = PKGLIBEXECDIR,
        [HEIMDALL_SYSDATA_DIR] = SYSDATADIR,
        [HEIMDALL_LIB_DIR] = LIBDIR,
        [HEIMDALL_LIBEXEC_DIR] = LIBEXECDIR,
        [HEIMDALL_LOCALE_DIR] = LOCALEDIR,
    };
    assert(type < ARRAY_SIZE(dirs));

    const char *dir_static = dirs[type];
    assert(*dir_static != '\0');
    /* Look for common static prefix. */
    size_t prefix_len = 0;
    while (prefix_len < strlen(LIBDIR)
        && LIBDIR[prefix_len] == dir_static[prefix_len])
        prefix_len++;

    /* Check that suffix matches between static and dynamic libdir paths. */
    char *filepath = NULL;
    size_t suffix_len = strlen(LIBDIR) - prefix_len;
    size_t libdir_len = strlen(libdir);

    if (suffix_len > libdir_len
     || memcmp(&LIBDIR[prefix_len], libdir + (libdir_len - suffix_len),
               suffix_len)) {
        suffix_len = libdir_len;
        prefix_len = 0;
    }

    /* Graft non-common static suffix to dynamically computed common prefix. */
    const char *fmt = (filename != NULL) ? "%.*s%s/%s" : "%.*s%s";

    if (unlikely(asprintf(&filepath, fmt, (int)(libdir_len - suffix_len),
                          libdir, dir_static + prefix_len, filename) == -1))
        filepath = NULL;

    free(libdir);
    return filepath;
}

static char *config_GetHomeDir (void)
{
    /* 1/ Try $HOME  */
    const char *home = getenv ("HOME");
    if (home != NULL)
        return strdup (home);
#if defined(HAVE_GETPWUID_R)
    /* 2/ Try /etc/passwd */
    long max = sysconf (_SC_GETPW_R_SIZE_MAX);
    if (max != -1)
    {
        char buf[max];
        struct passwd pwbuf, *pw;

        if (getpwuid_r (getuid (), &pwbuf, buf, sizeof (buf), &pw) == 0
          && pw != NULL)
            return strdup (pw->pw_dir);
    }
#endif
    return NULL;
}

static char *config_GetAppDir (const char *xdg_name, const char *xdg_default)
{
    char *psz_dir;
    char var[sizeof ("XDG__HOME") + strlen (xdg_name)];

    /* XDG Base Directory Specification - Version 0.6 */
    snprintf (var, sizeof (var), "XDG_%s_HOME", xdg_name);

    const char *home = getenv (var);
    if (home != NULL)
    {
        if (asprintf (&psz_dir, "%s/heimdall", home) == -1)
            psz_dir = NULL;
        return psz_dir;
    }

    char *psz_home = config_GetHomeDir ();
    if( psz_home == NULL
     || asprintf( &psz_dir, "%s/%s/heimdall", psz_home, xdg_default ) == -1 )
        psz_dir = NULL;
    free (psz_home);
    return psz_dir;
}

static char *config_GetTypeDir (const char *xdg_name)
{
    const size_t namelen = strlen (xdg_name);
    const char *home = getenv ("HOME");
    const char *dir = getenv ("XDG_CONFIG_HOME");
    const char *file = "user-dirs.dirs";

    if (home == NULL)
        return NULL;
    if (dir == NULL)
    {
        dir = home;
        file = ".config/user-dirs.dirs";
    }

    char *path;
    if (asprintf (&path, "%s/%s", dir, file) == -1)
        return NULL;

    FILE *stream = fopen (path, "rte");
    free (path);
    path = NULL;
    if (stream != NULL)
    {
        char *linebuf = NULL;
        size_t linelen = 0;

        while (getline (&linebuf, &linelen, stream) != -1)
        {
            char *ptr = linebuf;
            ptr += strspn (ptr, " \t"); /* Skip whites */
            if (strncmp (ptr, "XDG_", 4))
                continue;
            ptr += 4; /* Skip XDG_ */
            if (strncmp (ptr, xdg_name, namelen))
                continue;
            ptr += namelen; /* Skip XDG type name */
            if (strncmp (ptr, "_DIR", 4))
                continue;
            ptr += 4; /* Skip _DIR */
            ptr += strspn (ptr, " \t"); /* Skip whites */
            if (*ptr != '=')
                continue;
            ptr++; /* Skip equality sign */
            ptr += strspn (ptr, " \t"); /* Skip whites */
            if (*ptr != '"')
                continue;
            ptr++; /* Skip quote */
            linelen -= ptr - linebuf;

            char *out;
            if (strncmp (ptr, "$HOME", 5))
            {
                path = malloc (linelen);
                if (path == NULL)
                    continue;
                out = path;
            }
            else
            {   /* Prefix with $HOME */
                const size_t homelen = strlen (home);
                ptr += 5;
                path = malloc (homelen + linelen - 5);
                if (path == NULL)
                    continue;
                memcpy (path, home, homelen);
                out = path + homelen;
            }

            while (*ptr != '"')
            {
                if (*ptr == '\\')
                    ptr++;
                if (*ptr == '\0')
                {
                    free (path);
                    path = NULL;
                    break;
                }
                *(out++) = *(ptr++);
            }
            if (path != NULL)
                *out = '\0';
            break;
        }
        free (linebuf);
        fclose (stream);
    }

    /* Default! */
    if (path == NULL)
    {
        if (strcmp (xdg_name, "DESKTOP") == 0)
        {
            if (asprintf (&path, "%s/Desktop", home) == -1)
                return NULL;
        }
        else
            path = strdup (home);
    }

    return path;
}


char *platform_GetUserDir (heimdall_userdir_t type)
{
    switch (type)
    {
        case HEIMDALL_HOME_DIR:
            break;
        case HEIMDALL_CONFIG_DIR:
            return config_GetAppDir ("CONFIG", ".config");
        case HEIMDALL_USERDATA_DIR:
            return config_GetAppDir ("DATA", ".local/share");
        case HEIMDALL_CACHE_DIR:
            return config_GetAppDir ("CACHE", ".cache");

        case HEIMDALL_DESKTOP_DIR:
            return config_GetTypeDir ("DESKTOP");
        case HEIMDALL_DOWNLOAD_DIR:
            return config_GetTypeDir ("DOWNLOAD");
        case HEIMDALL_TEMPLATES_DIR:
            return config_GetTypeDir ("TEMPLATES");
        case HEIMDALL_PUBLICSHARE_DIR:
            return config_GetTypeDir ("PUBLICSHARE");
        case HEIMDALL_DOCUMENTS_DIR:
            return config_GetTypeDir ("DOCUMENTS");
        case HEIMDALL_MUSIC_DIR:
            return config_GetTypeDir ("MUSIC");
        case HEIMDALL_SNAPSHOTS_DIR:
        case HEIMDALL_PICTURES_DIR:
            return config_GetTypeDir ("PICTURES");
        case HEIMDALL_VIDEOS_DIR:
            return config_GetTypeDir ("VIDEOS");
    }
    return config_GetHomeDir ();
}
