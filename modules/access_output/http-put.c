/*****************************************************************************
 * http-put.c
 *****************************************************************************
 * Copyright (C) 2020 Rémi Denis-Courmont
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

#include <heimdall_common.h>
#include <heimdall_configuration.h>
#include <heimdall_plugin.h>
#include <heimdall_sout.h>
#include <heimdall_block.h>
#include <heimdall_strings.h>
#include "../access/http/connmgr.h"
#include "../access/http/outfile.h"

#define SOUT_CFG_PREFIX "sout-http-put-"

struct sout_http_put {
    struct heimdall_http_mgr *manager;
    struct heimdall_http_outfile *file;
};

static ssize_t Write(sout_access_out_t *access, block_t *block)
{
    struct sout_http_put *sys = access->p_sys;

    return heimdall_http_outfile_write(sys->file, block);
}

static int Control(sout_access_out_t *access, int query, va_list args)
{
    (void) access;

    switch (query)
    {
        case ACCESS_OUT_CONTROLS_PACE:
            *va_arg(args, bool *) = true;
            break;

        case ACCESS_OUT_CAN_SEEK:
            *va_arg(args, bool *) = false;
            break;

        default:
            return HEIMDALL_EGENERIC;
    }

    return HEIMDALL_SUCCESS;
}

static const char *const sout_options[] = {
    "user", "pwd", NULL
};

static int Open(heimdall_object_t *obj)
{
    sout_access_out_t *access = (sout_access_out_t *)obj;

    struct sout_http_put *sys = heimdall_obj_malloc(obj, sizeof (*sys));
    if (unlikely(sys == NULL))
        return HEIMDALL_ENOMEM;

    sys->manager = heimdall_http_mgr_create(obj, NULL);
    if (sys->manager == NULL)
        return HEIMDALL_ENOMEM;

    config_ChainParse(obj, SOUT_CFG_PREFIX, sout_options, access->p_cfg);

    char *ua = var_InheritString(obj, "http-user-agent");
    char *user = var_GetString(obj, SOUT_CFG_PREFIX"user");
    char *pwd = var_GetString(obj, SOUT_CFG_PREFIX"pwd");

    /* XXX: Empty user / password strings are not the same as NULL. No ways to
     * distinguish with the Heimdall APIs.
     */

    sys->file = heimdall_http_outfile_create(sys->manager, access->psz_path, ua,
                                        user, pwd);
    free(pwd);
    free(user);
    free(ua);

    if (sys->file == NULL) {
        msg_Err(obj, "cannot create HTTP resource %s", access->psz_path);
        heimdall_http_mgr_destroy(sys->manager);
        return HEIMDALL_EGENERIC;
    }

    access->p_sys = sys;
    access->pf_write = Write;
    access->pf_control = Control;

    return HEIMDALL_SUCCESS;
}

static void Close(heimdall_object_t *obj)
{
    sout_access_out_t *access = (sout_access_out_t *)obj;
    struct sout_http_put *sys = access->p_sys;

    if (heimdall_http_outfile_close(sys->file))
        msg_Err(obj, "server error while writing file");

    heimdall_http_mgr_destroy(sys->manager);
}

heimdall_module_begin()
    set_description(N_("HTTP PUT stream output"))
    set_shortname(N_("HTTP PUT"))
    set_capability("sout access", 0)
    set_subcategory(SUBCAT_SOUT_ACO)
    add_shortcut("http-put")
    add_string(SOUT_CFG_PREFIX"user", NULL, N_("Username"), NULL)
    add_password(SOUT_CFG_PREFIX"pwd", NULL, N_("Password"), NULL)
    set_callbacks(Open, Close)
heimdall_module_end()
