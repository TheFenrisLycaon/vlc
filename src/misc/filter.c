/*****************************************************************************
 * filter.c : filter_t helpers.
 *****************************************************************************
 * Copyright (C) 2009 Laurent Aimar
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Author: Laurent Aimar <fenrir _AT_ lycaria _DOT_ org>
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

#include <heimdall_common.h>
#include <heimdall_configuration.h>
#include "../libheimdall.h"
#include <heimdall_filter.h>
#include <heimdall_modules.h>
#include "../misc/variables.h"

/* */

static int TriggerFilterCallback(heimdall_object_t *p_this, char const *psz_var,
                                 heimdall_value_t oldval, heimdall_value_t newval,
                                 void *p_data)
{
    (void) p_this; (void) oldval;
    var_Set((filter_t *)p_data, psz_var, newval);
    return 0;
}

#undef filter_AddProxyCallbacks
void filter_AddProxyCallbacks( heimdall_object_t *obj, filter_t *filter,
                               heimdall_callback_t restart_cb )
{
    char **names = var_GetAllNames(HEIMDALL_OBJECT(filter));
    if (names == NULL)
        return;

    for (char **pname = names; *pname != NULL; pname++)
    {
        char *name = *pname;
        int var_type = var_Type(filter, name);
        if (config_GetType(name) == 0)
        {
            free(name);
            continue;
        }

        /* Create a mapping variable of the filter value if it does not exist.
         * These variables shouldn't be removed on proxy removal as they allow
         * keeping track of the filter state between filter chain resets. */
        const bool not_mapped = (var_Type(obj, name) == 0);
        if (not_mapped)
        {
            var_Create(obj, name,
                       var_type | HEIMDALL_VAR_DOINHERIT | HEIMDALL_VAR_ISCOMMAND);
        }

        if ((var_type & HEIMDALL_VAR_ISCOMMAND))
            var_AddCallback(obj, name, TriggerFilterCallback, filter);
        else
            var_AddCallback(obj, name, restart_cb, obj);
        free(name);
    }
    free(names);
}

#undef filter_DelProxyCallbacks
void filter_DelProxyCallbacks( heimdall_object_t *obj, filter_t *filter,
                               heimdall_callback_t restart_cb )
{
    char **names = var_GetAllNames(HEIMDALL_OBJECT(filter));
    if (names == NULL)
        return;

    for (char **pname = names; *pname != NULL; pname++)
    {
        char *name = *pname;
        if (!(var_Type(obj, name) & HEIMDALL_VAR_ISCOMMAND))
        {
            free(name);
            continue;
        }
        int filter_var_type = var_Type(filter, name);

        if (filter_var_type & HEIMDALL_VAR_ISCOMMAND)
            var_DelCallback(obj, name, TriggerFilterCallback, filter);
        else if (filter_var_type)
            var_DelCallback(obj, name, restart_cb, obj);
        free(name);
    }
    free(names);
}

/* */

heimdall_blender_t *filter_NewBlend( heimdall_object_t *p_this,
                           const video_format_t *p_dst_chroma )
{
    heimdall_blender_t *p_blend = heimdall_custom_create( p_this, sizeof(*p_blend), "blend" );
    if( unlikely( p_blend == NULL ) )
        return NULL;

    es_format_Init( &p_blend->fmt_in, VIDEO_ES, 0 );

    es_format_Init( &p_blend->fmt_out, VIDEO_ES, 0 );

    p_blend->fmt_out.i_codec        =
    p_blend->fmt_out.video.i_chroma = p_dst_chroma->i_chroma;

    /* The blend module will be loaded when needed with the real
    * input format */
    p_blend->p_module = NULL;

    return p_blend;
}

int filter_ConfigureBlend( heimdall_blender_t *p_blend,
                           int i_dst_width, int i_dst_height,
                           const video_format_t *p_src )
{
    /* */
    if( p_blend->p_module &&
        !video_format_IsSameChroma( &p_blend->fmt_in.video, p_src ) )
    {
        /* The chroma is not the same, we need to reload the blend module */
        heimdall_filter_UnloadModule( p_blend );
        p_blend->p_module = NULL;
    }

    /* */

    p_blend->fmt_in.i_codec = p_src->i_chroma;
    p_blend->fmt_in.video   = *p_src;

    /* */
    p_blend->fmt_out.video.i_width          =
    p_blend->fmt_out.video.i_visible_width  = i_dst_width;
    p_blend->fmt_out.video.i_height         =
    p_blend->fmt_out.video.i_visible_height = i_dst_height;

    /* */
    if( !p_blend->p_module )
        p_blend->p_module = heimdall_filter_LoadModule( p_blend, "video blending", NULL, false );
    if( !p_blend->p_module )
        return HEIMDALL_EGENERIC;
    assert( p_blend->ops != NULL );
    return HEIMDALL_SUCCESS;
}

int filter_Blend( heimdall_blender_t *p_blend,
                  picture_t *p_dst, int i_dst_x, int i_dst_y,
                  const picture_t *p_src, int i_alpha )
{
    if( !p_blend->p_module )
        return HEIMDALL_EGENERIC;

    p_blend->ops->blend_video( p_blend, p_dst, p_src, i_dst_x, i_dst_y, i_alpha );
    return HEIMDALL_SUCCESS;
}

void filter_DeleteBlend( heimdall_blender_t *p_blend )
{
    heimdall_filter_Delete( p_blend );
}

/* */
#include <heimdall_video_splitter.h>

video_splitter_t *video_splitter_New( heimdall_object_t *p_this,
                                      const char *psz_name,
                                      const video_format_t *p_fmt )
{
    video_splitter_t *p_splitter = heimdall_custom_create( p_this,
                                       sizeof(*p_splitter), "video splitter" );
    if( !p_splitter )
        return NULL;

    video_format_Copy( &p_splitter->fmt, p_fmt );

    /* */
    p_splitter->p_module = module_need( p_splitter, "video splitter", psz_name, true );
    if( ! p_splitter->p_module )
    {
        video_splitter_Delete( p_splitter );
        return NULL;
    }

    return p_splitter;
}

void video_splitter_Delete( video_splitter_t *p_splitter )
{
    if( p_splitter->p_module )
        module_unneed( p_splitter, p_splitter->p_module );

    video_format_Clean( &p_splitter->fmt );
    heimdall_object_delete(p_splitter);
}
