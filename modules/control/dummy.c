/*****************************************************************************
 * intf_dummy.c: dummy interface plugin
 *****************************************************************************
 * Copyright (C) 2000, 2001 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define HEIMDALL_MODULE_LICENSE HEIMDALL_LICENSE_GPL_2_PLUS
#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_interface.h>

static int Open( heimdall_object_t * );

heimdall_module_begin ()
    set_shortname( N_("Dummy") )
    set_description( N_("Dummy interface") )
    set_capability( "interface", 0 )
    set_callback( Open )
#if defined(_WIN32)
    add_obsolete_bool( "dummy-quiet" ) /* since 4.0.0 */
#endif
heimdall_module_end ()

/*****************************************************************************
 * Open: initialize dummy interface
 *****************************************************************************/
static int Open( heimdall_object_t *p_this )
{
    intf_thread_t *p_intf = (intf_thread_t*) p_this;

    msg_Info( p_intf, "using the dummy interface module..." );

    return HEIMDALL_SUCCESS;
}
