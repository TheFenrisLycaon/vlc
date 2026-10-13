// SPDX-License-Identifier: LGPL-2.1-or-later
/*****************************************************************************
 * filesystem.c: filesystem helpers
 *****************************************************************************
 * Copyright © 2024 VLC authors, VideoLAN and Videolabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Gabriel Lafond Thenaille <gabriel@videolabs.io>
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_fs.h>

/**
 * Create all directories in the given path if missing.
 */
int heimdall_mkdir_parent(const char *dirname, mode_t mode)
{
    int ret = heimdall_mkdir(dirname, mode);
    if (ret == 0 || errno == EEXIST) {
        return 0;
    } else if (errno != ENOENT) {
        return -1;
    }

    char *path = strdup(dirname);
    if (path == NULL) {
        return -1;
    }

    char *ptr = path + 1;
    while (*ptr) {
        ptr = strchr(ptr, DIR_SEP_CHAR);
        if (ptr == NULL) {
            break;
        }
        *ptr = '\0';
        if (heimdall_mkdir(path, mode) != 0) {
            if (errno != EEXIST) {
                free(path);
                return -1;
            }
        }
        *ptr = DIR_SEP_CHAR;
        ptr++;
    }
    ret = heimdall_mkdir(path, mode);
    if (errno == EEXIST) {
        ret = 0;
    }
    free(path);
    return ret;
}
