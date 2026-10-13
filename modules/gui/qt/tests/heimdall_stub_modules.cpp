/*****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Define builtin modules for mocked parts */
#define MODULE_NAME module_faker
#undef HEIMDALL_DYNAMIC_PLUGIN

#include <heimdall/heimdall.h>

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_playlist.h>
#include <heimdall_services_discovery.h>
#include <heimdall_renderer_discovery.h>
#include <heimdall_probe.h>
#include <heimdall_interface.h>
#include <heimdall_player.h>
#include <heimdall_media_library.h>

#include "../../../../lib/libheimdall_internal.h"

#include "qt.hpp"


#include "heimdall_stub_modules.hpp"

static HeimdallTestingEnv* testenv = nullptr;

///RenderDiscovery module

static int OpenRD( heimdall_object_t* p_this )
{
    testenv->renderDiscovery = (heimdall_renderer_discovery_t *)p_this;
    return HEIMDALL_SUCCESS;
}

static void CloseRD( heimdall_object_t* )
{
    testenv->renderDiscovery = nullptr;
}

static int heimdall_rd_probe_open(heimdall_object_t *obj) {
    auto probe = (struct heimdall_probe_t *)obj;

    if (testenv->renderDiscoveryProbeEnabled)
        heimdall_rd_probe_add(probe, "rd", "a fake renderer for testing purpose");
    //only probe ourself
    return HEIMDALL_PROBE_STOP;
}

///Interface module

namespace heimdall {
class Compositor {};
}

static int OpenIntf(heimdall_object_t* p_this)
{
    auto intfThread = reinterpret_cast<intf_thread_t*>(p_this);
    libheimdall_int_t* libheimdall = heimdall_object_instance( p_this );

    /* Ensure initialization of objects in qt_intf_t. */
    qt_intf_t* qt_intf = heimdall_object_create<qt_intf_t>( libheimdall );
    if (!qt_intf)
        return HEIMDALL_ENOMEM;

    qt_intf->obj.logger = heimdall_LogHeaderCreate(libheimdall->obj.logger, "qt");
    if (!qt_intf->obj.logger)
    {
        heimdall_object_delete(qt_intf);
        return HEIMDALL_EGENERIC;
    }

    qt_intf->p_playlist = heimdall_intf_GetMainPlaylist(intfThread);
    qt_intf->p_player = heimdall_playlist_GetPlayer(qt_intf->p_playlist);
    qt_intf->intf = intfThread;

    intfThread->p_sys = reinterpret_cast<intf_sys_t*>(qt_intf);

    testenv->intf = qt_intf;

    return HEIMDALL_SUCCESS;
}

static void CloseIntf( heimdall_object_t *p_this )
{
    intf_thread_t* intfThread = (intf_thread_t*)(p_this);
    auto qt_intf = reinterpret_cast<qt_intf_t*>(intfThread->p_sys);
    if (!qt_intf)
        return;
    heimdall_LogDestroy(qt_intf->obj.logger);
    heimdall_object_delete(qt_intf);

    testenv->intf = nullptr;
}

//Medialib module

static void* MLGet( heimdall_medialibrary_module_t*, int, va_list )
{
    return nullptr;
}

static int MLList( heimdall_medialibrary_module_t*, int,
                const heimdall_ml_query_params_t*, va_list )
{
    return HEIMDALL_EGENERIC;
}

static int MLControl( heimdall_medialibrary_module_t*, int, va_list )
{
    return HEIMDALL_EGENERIC;
}

static int MLOpen( heimdall_object_t* obj )
{
    auto* p_ml = reinterpret_cast<heimdall_medialibrary_module_t*>( obj );
    p_ml->pf_control = MLControl;
    p_ml->pf_get = MLGet;
    p_ml->pf_list = MLList;
    return HEIMDALL_SUCCESS;
}

static void MLClose( heimdall_object_t* obj )
{
    HEIMDALL_UNUSED(obj);
}

//module declaration

heimdall_module_begin()
    set_callbacks(OpenIntf, CloseIntf)
    set_capability("interface", 0)
add_submodule()
    set_capability("renderer_discovery", 0)
    add_shortcut("rd")
    set_callbacks(OpenRD, CloseRD)
add_submodule()
    set_capability("renderer probe", 10000)
    set_callback(heimdall_rd_probe_open)
add_submodule()
    set_capability("medialibrary", 10000)
    set_callbacks(MLOpen, MLClose)
heimdall_module_end()

extern "C" {

const char heimdall_module_name[] = MODULE_STRING;
HEIMDALL_EXPORT heimdall_plugin_cb heimdall_static_modules[] = {
    HEIMDALL_SYMBOL(heimdall_entry),
    NULL
};
}

HeimdallTestingEnv::HeimdallTestingEnv()
{
    assert(testenv == nullptr);
    testenv = this;

}

HeimdallTestingEnv::~HeimdallTestingEnv()
{
    libheimdall_release(libheimdall);
    testenv = nullptr;
}

static const char * test_defaults_args[] = {
    "-v", "--vout=vdummy", "--aout=adummy", "--text-renderer=tdummy", "--media-library"
};

static const int test_defaults_nargs = ARRAY_SIZE(test_defaults_args);

bool HeimdallTestingEnv::init()
{
    //see test/libheimdall/test.h
    QByteArray alarm_timeout = qgetenv("HEIMDALL_TEST_TIMEOUT");
    bool has_timeout;
    auto heimdall_timeout = alarm_timeout.toLongLong(&has_timeout);
    if (!has_timeout)
        qputenv("QTEST_FUNCTION_TIMEOUT", "5000");
    else
    {
        alarm_timeout.setNum(heimdall_timeout * 1000);
        qputenv("QTEST_FUNCTION_TIMEOUT", alarm_timeout);
    }

    setenv("HEIMDALL_PLUGIN_PATH", TOP_BUILDDIR"/modules", 1);
    setenv("HEIMDALL_LIB_PATH", TOP_BUILDDIR"/modules", 1);

    libheimdall = libheimdall_new(test_defaults_nargs, test_defaults_args);
    if (!libheimdall)
        return false;
    libheimdall_InternalAddIntf(libheimdall->p_libheimdall_int, MODULE_STRING);
    libheimdall_InternalPlay(libheimdall->p_libheimdall_int);

    if (!intf)
        return false;

    return true;
}
