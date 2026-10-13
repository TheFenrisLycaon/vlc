/*****************************************************************************
 * extension.c: test for the lua extension module
 *****************************************************************************
 * Copyright (C) 2023 Videolabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Alexandre Janniaux <ajanni@videolabs.io>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
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
# include <config.h>
#endif

/* Define a builtin module for mocked parts */
#define MODULE_NAME test_lua_extension
#undef HEIMDALL_DYNAMIC_PLUGIN

#include "../../libheimdall/test.h"

#include <heimdall/heimdall.h>

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_modules.h>
#include <heimdall_interface.h>
#include <heimdall_playlist.h>
#include <heimdall_player.h>
#include <heimdall_extensions.h>
#include <heimdall_input_item.h>

#include <limits.h>

#include "../lib/libheimdall_internal.h"

const char heimdall_module_name[] = MODULE_STRING;

static int exitcode = 0;

static int OnLuaEventTriggered(heimdall_object_t *obj, const char *name,
        heimdall_value_t oldv, heimdall_value_t newv, void *opaque)
{
    (void)obj; (void)name; (void)oldv; (void)newv;
    heimdall_sem_t *sem = opaque;
    heimdall_sem_post(sem);
    return HEIMDALL_SUCCESS;
}

static int OpenIntf(heimdall_object_t *root)
{
    heimdall_object_t *libheimdall = (heimdall_object_t*)heimdall_object_instance(root);
    var_Create(libheimdall, "test-lua-activate", HEIMDALL_VAR_STRING | HEIMDALL_VAR_ISCOMMAND);
    var_Create(libheimdall, "test-lua-deactivate", HEIMDALL_VAR_STRING | HEIMDALL_VAR_ISCOMMAND);
    var_Create(libheimdall, "test-lua-input-changed", HEIMDALL_VAR_STRING | HEIMDALL_VAR_ISCOMMAND);

    intf_thread_t *intf = (intf_thread_t*)root;
    extensions_manager_t *mgr =
        heimdall_object_create(root, sizeof *mgr);
    assert(mgr);

    setenv("HEIMDALL_USERDATA_PATH", TOP_SRCDIR "/test/modules/", 1);

    heimdall_playlist_t *playlist = heimdall_intf_GetMainPlaylist(intf);
    heimdall_player_t *player = heimdall_playlist_GetPlayer(playlist);
    mgr->player = player;
    mgr->p_module = module_need(mgr, "extension", "lua", true);

    if (mgr->p_module == NULL)
    {
        exitcode = 77;
        goto end;
    }

    heimdall_sem_t sem_input, sem_activate, sem_deactivate;
    heimdall_sem_init(&sem_input, 0);
    heimdall_sem_init(&sem_activate, 0);
    heimdall_sem_init(&sem_deactivate, 0);

    var_AddCallback(libheimdall, "test-lua-activate", OnLuaEventTriggered, &sem_activate);
    var_AddCallback(libheimdall, "test-lua-deactivate", OnLuaEventTriggered, &sem_deactivate);
    var_AddCallback(libheimdall, "test-lua-input-changed", OnLuaEventTriggered, &sem_input);

    /* Check that the extension from the test is correctly probed. */
    assert(mgr->extensions.i_size == 1);
    extension_Activate(mgr, mgr->extensions.p_elems[0]);
    heimdall_sem_wait(&sem_activate);

    heimdall_player_Lock(player);
    input_item_t *item = input_item_New(
            "mock://length=100000000000000000", // TODO: make it infinite
            "lua_test_sample");
    heimdall_player_SetCurrentMedia(player, item);
    heimdall_player_Start(player);
    heimdall_player_Unlock(player);

    heimdall_sem_wait(&sem_input);

    extension_Deactivate(mgr, mgr->extensions.p_elems[0]);
    heimdall_sem_wait(&sem_deactivate);

    var_DelCallback(libheimdall, "test-lua-activate", OnLuaEventTriggered, &sem_activate);
    var_DelCallback(libheimdall, "test-lua-deactivate", OnLuaEventTriggered, &sem_deactivate);
    var_DelCallback(libheimdall, "test-lua-input-changed", OnLuaEventTriggered, &sem_input);

    module_unneed(mgr, mgr->p_module);
    input_item_Release(item);
end:
    heimdall_object_delete(mgr);
    return HEIMDALL_SUCCESS;
}

/** Inject the mocked modules as a static plugin: **/
heimdall_module_begin()
    set_callback(OpenIntf)
    set_capability("interface", 0)
heimdall_module_end()

HEIMDALL_EXPORT const heimdall_plugin_cb heimdall_static_modules[] = {
    HEIMDALL_SYMBOL(heimdall_entry),
    NULL
};


int main(void)
{
    test_init();

    const char * const args[] = {
        "-vvv", "--vout=dummy", "--aout=dummy", "--text-renderer=dummy",
        "--no-auto-preparse",
    };

    libheimdall_instance_t *heimdall = libheimdall_new(ARRAY_SIZE(args), args);

    libheimdall_InternalAddIntf(heimdall->p_libheimdall_int, MODULE_STRING);
    libheimdall_InternalPlay(heimdall->p_libheimdall_int);

    libheimdall_release(heimdall);
    return 0;
}
