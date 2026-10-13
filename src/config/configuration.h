/*****************************************************************************
 * configuration.h management of the modules configuration
 *****************************************************************************
 * Copyright (C) 2007 VLC authors and VideoLAN
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

#ifndef LIBHEIMDALL_CONFIGURATION_H
# define LIBHEIMDALL_CONFIGURATION_H 1

#include <heimdall_configuration.h>

/* Internal configuration prototypes and structures */

struct heimdall_plugin_t;

struct heimdall_param {
    union {
        _Atomic int64_t i; /**< Current value (if integer or boolean) */
        _Atomic float f; /**< Current value (if floating point) */
        char *_Atomic str; /**< Current value (if character string) */
    } value;

    struct heimdall_plugin_t *owner;
    unsigned char shortname; /**< Optional short option name */
    unsigned internal:1; /**< Hidden from preferences and help */
    unsigned unsaved:1; /**< Not stored in persistent configuration */
    unsigned safe:1; /**< Safe for untrusted provisioning (playlists) */
    unsigned obsolete:1; /**< Ignored for backward compatibility */
    struct module_config_t item;
};

/**
 * Looks up a configuration parameter by name.
 *
 * \return the configuration parameter, or NULL if not found
 */
struct heimdall_param *heimdall_param_Find(const char *name);

int heimdall_param_SetString(struct heimdall_param *param, const char *value);

int  config_AutoSaveConfigFile( libheimdall_int_t * );

void config_Free(struct heimdall_param *, size_t);

void config_CmdLineEarlyScan( libheimdall_int_t *, int, const char *[] );

int config_LoadCmdLine   ( libheimdall_int_t *, int, const char *[], int * );
int config_LoadConfigFile( libheimdall_int_t * );
bool config_PrintHelp (libheimdall_int_t *);
void config_Lock(void);
void config_Unlock(void);

int config_SortConfig (void);
void config_UnsortConfig (void);

bool config_IsSafe (const char *);

/**
 * Gets the arch-specific installation directory.
 *
 * This function determines the directory containing the architecture-specific
 * installed asset files (such as executable plugins and compiled byte code).
 *
 * @return a heap-allocated string (use free() to release it), or NULL on error
 */
char *config_GetLibDir(void) HEIMDALL_USED HEIMDALL_MALLOC;

/* The configuration file */
#define CONFIG_FILE                     "heimdallrc"

char *platform_GetUserDir( heimdall_userdir_t ) HEIMDALL_USED HEIMDALL_MALLOC;

#endif
