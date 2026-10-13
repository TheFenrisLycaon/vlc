/*****************************************************************************
 * tdummy.c : dummy text rendering functions
 *****************************************************************************
 * Copyright (C) 2000, 2001 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Sigmund Augdal Helberg <dnumgis@videolan.org>
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
#include <heimdall_plugin.h>
#include <heimdall_filter.h>

static int OpenRenderer( filter_t * );

heimdall_module_begin ()
    set_shortname( N_("Dummy") )
    set_description( N_("Dummy font renderer") )
    set_callback_text_renderer( OpenRenderer, 1 )
heimdall_module_end ()


static subpicture_region_t *RenderText( filter_t *p_filter,
                       const subpicture_region_t *p_region_in,
                       const heimdall_fourcc_t *p_chroma_list )
{
    HEIMDALL_UNUSED(p_filter); HEIMDALL_UNUSED(p_region_in);
    HEIMDALL_UNUSED(p_chroma_list);
    return NULL;
}

static const struct heimdall_filter_operations filter_ops = {
    .render = RenderText,
};

static int OpenRenderer( filter_t *p_filter )
{
    p_filter->ops = &filter_ops;
    return HEIMDALL_SUCCESS;
}
