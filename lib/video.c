/*****************************************************************************
 * video.c: libheimdall new API video functions
 *****************************************************************************
 * Copyright (C) 2005-2010 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Filippo Carone <littlejohn@videolan.org>
 *          Jean-Paul Saman <jpsaman _at_ m2x _dot_ nl>
 *          Damien Fouilleul <damienf a_t lycaria dot org>
 *          Alexandre Janniaux <ajanni@videolabs.io>
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

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_renderer_discoverer.h>
#include <heimdall/libheimdall_picture.h>
#include <heimdall/libheimdall_media.h>
#include <heimdall/libheimdall_media_player.h>

#include <heimdall_common.h>
#include <heimdall_modules.h>
#include <heimdall_vout.h>
#include <heimdall_url.h>

#include "libheimdall_internal.h"
#include "media_player_internal.h"
#include <math.h>
#include <assert.h>

/*
 * Remember to release the returned vout_thread_t.
 */
static vout_thread_t **GetVouts( libheimdall_media_player_t *p_mi, size_t *n )
{
    heimdall_player_t *player = p_mi->player;
    return heimdall_player_vout_HoldAll(player, n);
}

static vout_thread_t *GetVout (libheimdall_media_player_t *mp, size_t num)
{
    vout_thread_t *p_vout = NULL;
    size_t n;
    vout_thread_t **pp_vouts = GetVouts (mp, &n);
    if (pp_vouts == NULL)
        goto err;

    if (num < n)
        p_vout = pp_vouts[num];

    for (size_t i = 0; i < n; i++)
        if (i != num)
            vout_Release(pp_vouts[i]);
    free (pp_vouts);

    if (p_vout == NULL)
err:
        libheimdall_printerr ("Video output not active");
    return p_vout;
}

/**********************************************************************
 * Exported functions
 **********************************************************************/

void libheimdall_set_fullscreen(libheimdall_media_player_t *p_mi, bool b_fullscreen)
{
    /* This will work even if the video is not currently active */
    var_SetBool(p_mi, "fullscreen", b_fullscreen);

    /* Apply to current video outputs (if any) */
    size_t n;
    vout_thread_t **pp_vouts = GetVouts (p_mi, &n);
    for (size_t i = 0; i < n; i++)
    {
        var_SetBool (pp_vouts[i], "fullscreen", b_fullscreen);
        vout_Release(pp_vouts[i]);
    }
    free (pp_vouts);
}

bool libheimdall_get_fullscreen( libheimdall_media_player_t *p_mi )
{
    return var_GetBool (p_mi, "fullscreen");
}

void libheimdall_toggle_fullscreen( libheimdall_media_player_t *p_mi )
{
    bool b_fullscreen = var_ToggleBool (p_mi, "fullscreen");

    /* Apply to current video outputs (if any) */
    size_t n;
    vout_thread_t **pp_vouts = GetVouts (p_mi, &n);
    for (size_t i = 0; i < n; i++)
    {
        vout_thread_t *p_vout = pp_vouts[i];

        var_SetBool (p_vout, "fullscreen", b_fullscreen);
        vout_Release(p_vout);
    }
    free (pp_vouts);
}

void libheimdall_video_set_key_input( libheimdall_media_player_t *p_mi, unsigned on )
{
    var_SetBool (p_mi, "keyboard-events", !!on);
}

void libheimdall_video_set_mouse_input( libheimdall_media_player_t *p_mi, unsigned on )
{
    var_SetBool (p_mi, "mouse-events", !!on);
}

int
libheimdall_video_take_snapshot( libheimdall_media_player_t *p_mi, unsigned num,
                            const char *psz_filepath,
                            unsigned int i_width, unsigned int i_height )
{
    assert( psz_filepath );

    vout_thread_t *p_vout = GetVout (p_mi, num);
    if (p_vout == NULL)
        return -1;

    /* FIXME: This is not atomic. All parameters should be passed at once
     * (obviously _not_ with var_*()). Also, the libheimdall object should not be
     * used for the callbacks: that breaks badly if there are concurrent
     * media players in the instance. */
    var_Create( p_vout, "snapshot-width", HEIMDALL_VAR_INTEGER );
    var_SetInteger( p_vout, "snapshot-width", i_width);
    var_Create( p_vout, "snapshot-height", HEIMDALL_VAR_INTEGER );
    var_SetInteger( p_vout, "snapshot-height", i_height );
    var_Create( p_vout, "snapshot-path", HEIMDALL_VAR_STRING );
    var_SetString( p_vout, "snapshot-path", psz_filepath );
    var_Create( p_vout, "snapshot-format", HEIMDALL_VAR_STRING );
    var_SetString( p_vout, "snapshot-format", "png" );
    var_TriggerCallback( p_vout, "video-snapshot" );
    vout_Release(p_vout);
    return 0;
}

int libheimdall_video_get_size( libheimdall_media_player_t *p_mi, unsigned ignored,
                           unsigned *restrict px, unsigned *restrict py )
{
    (void) ignored;

    int ret = -1;
    libheimdall_media_track_t *track =
        libheimdall_media_player_get_selected_track( p_mi, libheimdall_track_video );

    if (track)
    {
        *px = track->u.video->i_width;
        *py = track->u.video->i_height;
        ret = 0;
        libheimdall_media_track_release(track);
    }

    return ret;
}

int libheimdall_video_get_cursor( libheimdall_media_player_t *mp, unsigned num,
                             int *restrict px, int *restrict py )
{
    vout_thread_t *p_vout = GetVout (mp, num);
    if (p_vout == NULL)
        return -1;

    var_GetCoords (p_vout, "mouse-moved", px, py);
    vout_Release(p_vout);
    return 0;
}

unsigned libheimdall_media_player_has_vout( libheimdall_media_player_t *p_mi )
{
    size_t n;
    vout_thread_t **pp_vouts = GetVouts (p_mi, &n);
    for (size_t i = 0; i < n; i++)
        vout_Release(pp_vouts[i]);
    free (pp_vouts);
    return n;
}

float libheimdall_video_get_scale( libheimdall_media_player_t *mp )
{
    float f_scale = var_GetFloat (mp, "zoom");
    if (var_GetBool (mp, "autoscale"))
        f_scale = 0.f;
    return f_scale;
}

void libheimdall_video_set_scale( libheimdall_media_player_t *p_mp, float f_scale )
{
    if (isfinite(f_scale) && f_scale != 0.f)
        var_SetFloat (p_mp, "zoom", f_scale);
    var_SetBool (p_mp, "autoscale", f_scale == 0.f);

    /* Apply to current video outputs (if any) */
    size_t n;
    vout_thread_t **pp_vouts = GetVouts (p_mp, &n);
    for (size_t i = 0; i < n; i++)
    {
        vout_thread_t *p_vout = pp_vouts[i];

        if (isfinite(f_scale) && f_scale != 0.f)
            var_SetFloat (p_vout, "zoom", f_scale);
        var_SetBool (p_vout, "autoscale", f_scale == 0.f);
        vout_Release(p_vout);
    }
    free (pp_vouts);
}

char *libheimdall_video_get_aspect_ratio( libheimdall_media_player_t *p_mi )
{
    return var_GetNonEmptyString (p_mi, "aspect-ratio");
}

void libheimdall_video_set_aspect_ratio( libheimdall_media_player_t *p_mi,
                                    const char *psz_aspect )
{
    if (psz_aspect == NULL)
        psz_aspect = "";
    var_SetString (p_mi, "aspect-ratio", psz_aspect);

    size_t n;
    vout_thread_t **pp_vouts = GetVouts (p_mi, &n);
    for (size_t i = 0; i < n; i++)
    {
        vout_thread_t *p_vout = pp_vouts[i];

        var_SetString (p_vout, "aspect-ratio", psz_aspect);
        vout_Release(p_vout);
    }
    free (pp_vouts);
}

libheimdall_video_fit_mode_t libheimdall_video_get_display_fit( libheimdall_media_player_t *p_mi )
{
    return var_GetInteger (p_mi, "fit");
}

void libheimdall_video_set_display_fit( libheimdall_media_player_t *p_mi, libheimdall_video_fit_mode_t fit )
{
    var_SetInteger (p_mi, "fit", fit);

    size_t n;
    vout_thread_t **pp_vouts = GetVouts (p_mi, &n);
    for (size_t i = 0; i < n; i++)
    {
        vout_thread_t *p_vout = pp_vouts[i];

        var_SetInteger (p_vout, "fit", fit);
        vout_Release(p_vout);
    }
    free (pp_vouts);
}


libheimdall_video_viewpoint_t *libheimdall_video_new_viewpoint(void)
{
    libheimdall_video_viewpoint_t *p_vp = malloc(sizeof *p_vp);
    if (unlikely(p_vp == NULL))
        return NULL;
    p_vp->f_yaw = p_vp->f_pitch = p_vp->f_roll = p_vp->f_field_of_view = 0.0f;
    return p_vp;
}

int libheimdall_video_update_viewpoint( libheimdall_media_player_t *p_mi,
                                   const libheimdall_video_viewpoint_t *p_viewpoint,
                                   bool b_absolute )
{
    heimdall_viewpoint_t update = { .fov = p_viewpoint->f_field_of_view };
    heimdall_viewpoint_from_euler( &update,
                              p_viewpoint->f_yaw,
                              p_viewpoint->f_pitch,
                              p_viewpoint->f_roll);

    enum heimdall_player_whence whence = b_absolute ? HEIMDALL_PLAYER_WHENCE_ABSOLUTE
                                               : HEIMDALL_PLAYER_WHENCE_RELATIVE;

    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_UpdateViewpoint(player, &update, whence);

    heimdall_player_Unlock(player);

    /* may not fail anymore, keep int not to break the API */
    return 0;
}

libheimdall_video_stereo_mode_t libheimdall_video_get_video_stereo_mode(libheimdall_media_player_t *p_mi)
{
    static_assert( libheimdall_VideoStereoAuto       == (int)VIDEO_STEREO_OUTPUT_AUTO &&
                   libheimdall_VideoStereoStereo     == (int)VIDEO_STEREO_OUTPUT_STEREO &&
                   libheimdall_VideoStereoLeftEye    == (int)VIDEO_STEREO_OUTPUT_LEFT_ONLY &&
                   libheimdall_VideoStereoRightEye   == (int)VIDEO_STEREO_OUTPUT_RIGHT_ONLY &&
                   libheimdall_VideoStereoSideBySide == (int)VIDEO_STEREO_OUTPUT_SIDE_BY_SIDE,
                   "stereo mode mismatch" );

    return var_GetInteger(p_mi, "video-stereo-mode");
}

void libheimdall_video_set_video_stereo_mode(libheimdall_media_player_t *p_mi,
                                       const libheimdall_video_stereo_mode_t i_mode)
{
    /* This will work even if the video is not currently active */
    var_SetInteger(p_mi, "video-stereo-mode", i_mode);

    /* Apply to current video outputs (if any) */
    size_t n;
    vout_thread_t **pp_vouts = GetVouts (p_mi, &n);
    for (size_t i = 0; i < n; i++)
    {
        var_SetInteger (pp_vouts[i], "video-stereo-mode", i_mode);
        vout_Release(pp_vouts[i]);
    }
    free (pp_vouts);
}

libheimdall_time_t libheimdall_video_get_spu_delay( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_tick_t delay = heimdall_player_GetSubtitleDelay(player);

    heimdall_player_Unlock(player);

    return US_FROM_HEIMDALL_TICK(delay);
}

int libheimdall_video_set_spu_delay( libheimdall_media_player_t *p_mi,
                                libheimdall_time_t i_delay )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_SetSubtitleDelay(player, HEIMDALL_TICK_FROM_US(i_delay),
                                HEIMDALL_PLAYER_WHENCE_ABSOLUTE);

    heimdall_player_Unlock(player);
    /* may not fail anymore, keep int not to break the API */
    return 0;
}

float libheimdall_video_get_spu_text_scale( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    unsigned scale = heimdall_player_GetSubtitleTextScale(player);

    heimdall_player_Unlock(player);

    return scale / 100.f;
}

void libheimdall_video_set_spu_text_scale( libheimdall_media_player_t *p_mi,
                                      float f_scale )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_SetSubtitleTextScale(player, lroundf(f_scale * 100.f));

    heimdall_player_Unlock(player);
}

static void libheimdall_video_set_crop(libheimdall_media_player_t *mp,
                                  const char *geometry)
{
    var_SetString(mp, "crop", geometry);

    size_t n;
    vout_thread_t **vouts = GetVouts(mp, &n);

    for (size_t i = 0; i < n; i++)
    {
        var_SetString(vouts[i], "crop", geometry);
        vout_Release(vouts[i]);
    }
    free(vouts);
}

void libheimdall_video_set_crop_ratio(libheimdall_media_player_t *mp,
                                 unsigned num, unsigned den)
{
    char geometry[2 * (3 * sizeof (unsigned) + 1)];

    if (den == 0)
        geometry[0] = '\0';
    else
        snprintf(geometry, ARRAY_SIZE(geometry), "%u:%u", num, den);

    libheimdall_video_set_crop(mp, geometry);
}

void libheimdall_video_set_crop_window(libheimdall_media_player_t *mp,
                                  unsigned x, unsigned y,
                                  unsigned width, unsigned height)
{
    char geometry[4 * (3 * sizeof (unsigned) + 1)];

    assert(width != 0 && height != 0);
    snprintf(geometry, ARRAY_SIZE(geometry), "%ux%u+%u+%u", width, height, x, y);
    libheimdall_video_set_crop(mp, geometry);
}

void libheimdall_video_set_crop_border(libheimdall_media_player_t *mp,
                                  unsigned left, unsigned right,
                                  unsigned top, unsigned bottom)
{
    char geometry[4 * (3 * sizeof (unsigned) + 1)];

    snprintf(geometry, ARRAY_SIZE(geometry), "%u+%u+%u+%u", left, top, right, bottom);
    libheimdall_video_set_crop(mp, geometry);
}

int libheimdall_video_get_teletext( libheimdall_media_player_t *p_mi )
{
    return var_GetInteger (p_mi, "vbi-page");
}

void libheimdall_video_set_teletext( libheimdall_media_player_t *p_mi, int i_page )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    if (i_page == 0)
        heimdall_player_SetTeletextEnabled(player, false);
    else if (i_page > 0)
    {
        if (i_page >= 1000)
        {
            bool is_key = i_page == libheimdall_teletext_key_red
                       || i_page == libheimdall_teletext_key_green
                       || i_page == libheimdall_teletext_key_yellow
                       || i_page == libheimdall_teletext_key_blue
                       || i_page == libheimdall_teletext_key_index;
            if (!is_key)
            {
                libheimdall_printerr("Invalid key action");
                heimdall_player_Unlock(player);
                return;
            }
        }
        heimdall_player_SetTeletextEnabled(player, true);
        heimdall_player_SelectTeletextPage(player, i_page);
    }
    else
    {
        libheimdall_printerr("Invalid page number");
        heimdall_player_Unlock(player);
        return;
    }

    heimdall_player_Unlock(player);
}

void libheimdall_video_set_teletext_transparency( libheimdall_media_player_t *p_mi, bool transparent )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_SetTeletextTransparency(player, transparent);

    heimdall_player_Unlock(player);
}

bool libheimdall_video_get_teletext_transparency( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    bool transparent = heimdall_player_IsTeletextTransparent(player);

    heimdall_player_Unlock(player);

    return transparent;
}

/******************************************************************************
 * libheimdall_video_set_deinterlace : enable/disable/auto deinterlace and filter
 *****************************************************************************/
int libheimdall_video_set_deinterlace( libheimdall_media_player_t *p_mi, int deinterlace,
                                  const char *psz_mode )
{
    if (deinterlace != 0 && deinterlace != 1)
        deinterlace = -1;

    if (psz_mode
     && strcmp (psz_mode, "blend")    && strcmp (psz_mode, "bob")
     && strcmp (psz_mode, "discard")  && strcmp (psz_mode, "linear")
     && strcmp (psz_mode, "mean")     && strcmp (psz_mode, "x")
     && strcmp (psz_mode, "yadif")    && strcmp (psz_mode, "yadif2x")
     && strcmp (psz_mode, "phosphor") && strcmp (psz_mode, "ivtc")
     && strcmp (psz_mode, "auto"))
        return -1;

    if (psz_mode && deinterlace != 0)
        var_SetString (p_mi, "deinterlace-mode", psz_mode);

    var_SetInteger (p_mi, "deinterlace", deinterlace);

    size_t n;
    vout_thread_t **pp_vouts = GetVouts (p_mi, &n);
    for (size_t i = 0; i < n; i++)
    {
        vout_thread_t *p_vout = pp_vouts[i];

        if (psz_mode && deinterlace != 0)
            var_SetString (p_vout, "deinterlace-mode", psz_mode);

        var_SetInteger (p_vout, "deinterlace", deinterlace);
        vout_Release(p_vout);
    }
    free (pp_vouts);
    return 0;
}

int libheimdall_video_get_deinterlace(libheimdall_media_player_t *mp, char **modep)
{
    int tristate = var_GetInteger(mp, "deinterlace");

    if (modep != NULL) {
        if (tristate != 0)
            *modep = var_GetString(mp, "deinterlace-mode");
        else
            *modep = NULL;
    }

    return tristate;
}

/* ************** */
/* module helpers */
/* ************** */

static int get_filter_str( heimdall_object_t *p_parent, const char *psz_name,
                           bool b_add, const char **ppsz_filter_type,
                           char **ppsz_filter_value)
{
    char *psz_parser;
    char *psz_string;
    const char *psz_filter_type;

    module_t *p_obj = module_find( psz_name );
    if( !p_obj )
    {
        msg_Err( p_parent, "Unable to find filter module \"%s\".", psz_name );
        return HEIMDALL_EGENERIC;
    }

    if( module_provides( p_obj, "video filter" ) )
    {
        psz_filter_type = "video-filter";
    }
    else if( module_provides( p_obj, "sub source" ) )
    {
        psz_filter_type = "sub-source";
    }
    else if( module_provides( p_obj, "sub filter" ) )
    {
        psz_filter_type = "sub-filter";
    }
    else
    {
        msg_Err( p_parent, "Unknown video filter type." );
        return HEIMDALL_EGENERIC;
    }

    psz_string = var_GetString( p_parent, psz_filter_type );

    /* Todo : Use some generic chain manipulation functions */
    if( !psz_string ) psz_string = strdup("");

    psz_parser = strstr( psz_string, psz_name );
    if( b_add )
    {
        if( !psz_parser )
        {
            psz_parser = psz_string;
            if( asprintf( &psz_string, (*psz_string) ? "%s:%s" : "%s%s",
                          psz_string, psz_name ) == -1 )
            {
                free( psz_parser );
                return HEIMDALL_EGENERIC;
            }
            free( psz_parser );
        }
        else
        {
            free( psz_string );
            return HEIMDALL_EGENERIC;
        }
    }
    else
    {
        if( psz_parser )
        {
            memmove( psz_parser, psz_parser + strlen(psz_name) +
                            (*(psz_parser + strlen(psz_name)) == ':' ? 1 : 0 ),
                            strlen(psz_parser + strlen(psz_name)) + 1 );

            /* Remove trailing : : */
            if( *(psz_string+strlen(psz_string ) -1 ) == ':' )
                *(psz_string+strlen(psz_string ) -1 ) = '\0';
        }
        else
        {
            free( psz_string );
            return HEIMDALL_EGENERIC;
        }
    }

    *ppsz_filter_type = psz_filter_type;
    *ppsz_filter_value = psz_string;
    return HEIMDALL_SUCCESS;
}

static bool find_sub_source_by_name( libheimdall_media_player_t *p_mi, const char *restrict name )
{
    vout_thread_t *vout = GetVout( p_mi, 0 );
    if (!vout)
        return false;

    char *psz_sources = var_GetString( vout, "sub-source" );
    if( !psz_sources )
    {
        libheimdall_printerr( "%s not enabled", name );
        vout_Release(vout);
        return false;
    }

    /* Find 'name'  */
    char *p = strstr( psz_sources, name );
    free( psz_sources );
    vout_Release(vout);
    return (p != NULL);
}

typedef const struct {
    const char name[20];
    unsigned type;
} opt_t;

static void
set_value( libheimdall_media_player_t *p_mi, const char *restrict name,
           const opt_t *restrict opt, unsigned i_expected_type,
           const heimdall_value_t *val, bool b_sub_source )
{
    if( !opt ) return;

    int i_type = opt->type;
    heimdall_value_t new_val = *val;
    const char *psz_opt_name = opt->name;
    switch( i_type )
    {
        case 0: /* the enabler */
        {
            int i_ret = get_filter_str( HEIMDALL_OBJECT( p_mi ), opt->name, val->i_int,
                                        &psz_opt_name, &new_val.psz_string );
            if( i_ret != HEIMDALL_SUCCESS )
                return;
            i_type = HEIMDALL_VAR_STRING;
            break;
        }
        case HEIMDALL_VAR_INTEGER:
        case HEIMDALL_VAR_FLOAT:
        case HEIMDALL_VAR_STRING:
            if( i_expected_type != opt->type )
            {
                libheimdall_printerr( "Invalid argument to %s", name );
                return;
            }
            break;
        default:
            libheimdall_printerr( "Invalid argument to %s", name );
            return;
    }

    /* Set the new value to the media player. Next vouts created from this
     * media player will inherit this new value */
    var_SetChecked( p_mi, psz_opt_name, i_type, new_val );

    /* Set the new value to every loaded vouts */
    size_t i_vout_count;
    vout_thread_t **pp_vouts = GetVouts( p_mi, &i_vout_count );
    for( size_t i = 0; i < i_vout_count; ++i )
    {
        var_SetChecked( pp_vouts[i], psz_opt_name, i_type, new_val );
        if( b_sub_source )
            var_TriggerCallback( pp_vouts[i], "sub-source" );
        vout_Release(pp_vouts[i]);
    }
    free (pp_vouts);

    if( opt->type == 0 )
        free( new_val.psz_string );
}

static int
get_int( libheimdall_media_player_t *p_mi, const char *restrict name,
         const opt_t *restrict opt )
{
    if( !opt ) return 0;

    switch( opt->type )
    {
        case 0: /* the enabler */
        {
            bool b_enabled = find_sub_source_by_name( p_mi, name );
            return b_enabled ? 1 : 0;
        }
    case HEIMDALL_VAR_INTEGER:
        return var_GetInteger(p_mi, opt->name);
    case HEIMDALL_VAR_FLOAT:
        return lroundf(var_GetFloat(p_mi, opt->name));
    default:
        libheimdall_printerr( "Invalid argument to %s in %s", name, "get int" );
        return 0;
    }
}

static float
get_float( libheimdall_media_player_t *p_mi, const char *restrict name,
            const opt_t *restrict opt )
{
    if( !opt ) return 0.0;

    if( opt->type != HEIMDALL_VAR_FLOAT )
    {
        libheimdall_printerr( "Invalid argument to %s in %s", name, "get float" );
        return 0.0;
    }

    return var_GetFloat( p_mi, opt->name );
}

static const opt_t *
marq_option_bynumber(unsigned option)
{
    static const opt_t optlist[] =
    {
        { "marq",          0 },
        { "marq-marquee",  HEIMDALL_VAR_STRING },
        { "marq-color",    HEIMDALL_VAR_INTEGER },
        { "marq-opacity",  HEIMDALL_VAR_INTEGER },
        { "marq-position", HEIMDALL_VAR_INTEGER },
        { "marq-refresh",  HEIMDALL_VAR_INTEGER },
        { "marq-size",     HEIMDALL_VAR_INTEGER },
        { "marq-timeout",  HEIMDALL_VAR_INTEGER },
        { "marq-x",        HEIMDALL_VAR_INTEGER },
        { "marq-y",        HEIMDALL_VAR_INTEGER },
    };
    enum { num_opts = sizeof(optlist) / sizeof(*optlist) };

    const opt_t *r = option < num_opts ? optlist+option : NULL;
    if( !r )
        libheimdall_printerr( "Unknown marquee option" );
    return r;
}

/*****************************************************************************
 * libheimdall_video_get_marquee_int : get a marq option value
 *****************************************************************************/
int libheimdall_video_get_marquee_int( libheimdall_media_player_t *p_mi,
                                  unsigned option )
{
    return get_int( p_mi, "marq", marq_option_bynumber(option) );
}

/*****************************************************************************
 * libheimdall_video_set_marquee_int: enable, disable or set an int option
 *****************************************************************************/
void libheimdall_video_set_marquee_int( libheimdall_media_player_t *p_mi,
                         unsigned option, int value )
{
    set_value( p_mi, "marq", marq_option_bynumber(option), HEIMDALL_VAR_INTEGER,
               &(heimdall_value_t) { .i_int = value }, true );
}

/*****************************************************************************
 * libheimdall_video_set_marquee_string: set a string option
 *****************************************************************************/
void libheimdall_video_set_marquee_string( libheimdall_media_player_t *p_mi,
                unsigned option, const char * value )
{
    set_value( p_mi, "marq", marq_option_bynumber(option), HEIMDALL_VAR_STRING,
               &(heimdall_value_t){ .psz_string = (char *)value }, true );
}


/* logo module support */

static const opt_t *
logo_option_bynumber( unsigned option )
{
    static const opt_t vlogo_optlist[] =
    /* depends on libheimdall_video_logo_option_t */
    {
        { "logo",          0 },
        { "logo-file",     HEIMDALL_VAR_STRING },
        { "logo-x",        HEIMDALL_VAR_INTEGER },
        { "logo-y",        HEIMDALL_VAR_INTEGER },
        { "logo-delay",    HEIMDALL_VAR_INTEGER },
        { "logo-repeat",   HEIMDALL_VAR_INTEGER },
        { "logo-opacity",  HEIMDALL_VAR_INTEGER },
        { "logo-position", HEIMDALL_VAR_INTEGER },
    };
    enum { num_vlogo_opts = sizeof(vlogo_optlist) / sizeof(*vlogo_optlist) };

    const opt_t *r = option < num_vlogo_opts ? vlogo_optlist+option : NULL;
    if( !r )
        libheimdall_printerr( "Unknown logo option" );
    return r;
}

void libheimdall_video_set_logo_string( libheimdall_media_player_t *p_mi,
                                   unsigned option, const char *psz_value )
{
    set_value( p_mi,"logo",logo_option_bynumber(option), HEIMDALL_VAR_STRING,
               &(heimdall_value_t){ .psz_string = (char *)psz_value }, true );
}


void libheimdall_video_set_logo_int( libheimdall_media_player_t *p_mi,
                                unsigned option, int value )
{
    set_value( p_mi, "logo", logo_option_bynumber(option), HEIMDALL_VAR_INTEGER,
               &(heimdall_value_t) { .i_int = value }, true );
}


int libheimdall_video_get_logo_int( libheimdall_media_player_t *p_mi,
                               unsigned option )
{
    return get_int( p_mi, "logo", logo_option_bynumber(option) );
}


/* adjust module support */


static const opt_t *
adjust_option_bynumber( unsigned option )
{
    static const opt_t optlist[] =
    {
        { "adjust",     0 },
        { "contrast",   HEIMDALL_VAR_FLOAT },
        { "brightness", HEIMDALL_VAR_FLOAT },
        { "hue",        HEIMDALL_VAR_FLOAT },
        { "saturation", HEIMDALL_VAR_FLOAT },
        { "gamma",      HEIMDALL_VAR_FLOAT },
    };
    enum { num_opts = sizeof(optlist) / sizeof(*optlist) };

    const opt_t *r = option < num_opts ? optlist+option : NULL;
    if( !r )
        libheimdall_printerr( "Unknown adjust option" );
    return r;
}


void libheimdall_video_set_adjust_int( libheimdall_media_player_t *p_mi,
                                  unsigned option, int value )
{
    set_value( p_mi, "adjust", adjust_option_bynumber(option), HEIMDALL_VAR_INTEGER,
               &(heimdall_value_t) { .i_int = value }, false );
}


int libheimdall_video_get_adjust_int( libheimdall_media_player_t *p_mi,
                                 unsigned option )
{
    return get_int( p_mi, "adjust", adjust_option_bynumber(option) );
}


void libheimdall_video_set_adjust_float( libheimdall_media_player_t *p_mi,
                                    unsigned option, float value )
{
    set_value( p_mi, "adjust", adjust_option_bynumber(option), HEIMDALL_VAR_FLOAT,
               &(heimdall_value_t) { .f_float = value }, false );
}


float libheimdall_video_get_adjust_float( libheimdall_media_player_t *p_mi,
                                     unsigned option )
{
    return get_float( p_mi, "adjust", adjust_option_bynumber(option) );
}

void libheimdall_video_unset_projection_mode(libheimdall_media_player_t *player)
{
    var_SetInteger(player, "projection-mode", -1);

    /* Apply to current video outputs (if any) */
    size_t n;
    vout_thread_t **pp_vouts = GetVouts(player, &n);
    for (size_t i = 0; i < n; i++)
    {
        var_SetInteger(pp_vouts[i], "projection-mode", -1);
        vout_Release(pp_vouts[i]);
    }
    free (pp_vouts);
}

void libheimdall_video_set_projection_mode(libheimdall_media_player_t *player,
                                      libheimdall_video_projection_t projection_mode)
{
    var_SetInteger(player, "projection-mode", projection_mode);

    /* Apply to current video outputs (if any) */
    size_t n;
    vout_thread_t **pp_vouts = GetVouts(player, &n);
    for (size_t i = 0; i < n; i++)
    {
        var_SetInteger(pp_vouts[i], "projection-mode", projection_mode);
        vout_Release(pp_vouts[i]);
    }
    free (pp_vouts);
}
