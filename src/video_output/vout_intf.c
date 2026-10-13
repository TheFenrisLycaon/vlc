/*****************************************************************************
 * vout_intf.c : video output interface
 *****************************************************************************
 * Copyright (C) 2000-2007 VLC authors and VideoLAN
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>

#include <stdio.h>
#include <stdlib.h>                                                /* free() */
#include <assert.h>

#include <heimdall_block.h>
#include <heimdall_modules.h>

#include <heimdall_vout.h>
#include <heimdall_vout_osd.h>
#include <heimdall_strings.h>
#include <heimdall_charset.h>
#include <heimdall_spu.h>
#include <heimdall_subpicture.h>
#include "vout_internal.h"
#include "snapshot.h"

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
/* Object variables callbacks */
static int CropCallback( heimdall_object_t *, char const *,
                         heimdall_value_t, heimdall_value_t, void * );
static int CropBorderCallback( heimdall_object_t *, char const *,
                               heimdall_value_t, heimdall_value_t, void * );
static int AspectCallback( heimdall_object_t *, char const *,
                           heimdall_value_t, heimdall_value_t, void * );
static int AutoScaleCallback( heimdall_object_t *, char const *,
                              heimdall_value_t, heimdall_value_t, void * );
static int FitCallback( heimdall_object_t *, char const *,
                        heimdall_value_t, heimdall_value_t, void * );
static int ZoomCallback( heimdall_object_t *, char const *,
                         heimdall_value_t, heimdall_value_t, void * );
static int AboveCallback( heimdall_object_t *, char const *,
                          heimdall_value_t, heimdall_value_t, void * );
static int WallPaperCallback( heimdall_object_t *, char const *,
                              heimdall_value_t, heimdall_value_t, void * );
static int FullscreenCallback( heimdall_object_t *, char const *,
                               heimdall_value_t, heimdall_value_t, void * );
static int SnapshotCallback( heimdall_object_t *, char const *,
                             heimdall_value_t, heimdall_value_t, void * );
static int VideoFilterCallback( heimdall_object_t *, char const *,
                                heimdall_value_t, heimdall_value_t, void * );
static int SubSourceCallback( heimdall_object_t *, char const *,
                              heimdall_value_t, heimdall_value_t, void * );
static int SubFilterCallback( heimdall_object_t *, char const *,
                              heimdall_value_t, heimdall_value_t, void * );
static int SubMarginCallback( heimdall_object_t *, char const *,
                              heimdall_value_t, heimdall_value_t, void * );
static int SecondarySubMarginCallback( heimdall_object_t *, char const *,
                                       heimdall_value_t, heimdall_value_t, void * );
static int ViewpointCallback( heimdall_object_t *, char const *,
                              heimdall_value_t, heimdall_value_t, void * );
static int Stereo3DCallback( heimdall_object_t *, char const *,
                             heimdall_value_t, heimdall_value_t, void * );

static int OverrideProjectionCallback( heimdall_object_t *, char const *,
                              heimdall_value_t, heimdall_value_t, void * );

static int ChangeProjectionCallback( heimdall_object_t *, char const *,
                              heimdall_value_t, heimdall_value_t, void * );

/*****************************************************************************
 * vout_IntfInit: called during the vout creation to initialise misc things.
 *****************************************************************************/
static const struct
{
    double f_value;
    char psz_label[13];
} p_zoom_values[] = {
    { 0.25, N_("1:4 Quarter") },
    { 0.5, N_("1:2 Half") },
    { 1, N_("1:1 Original") },
    { 2, N_("2:1 Double") },
};

static const struct
{
    char psz_value[8];
    char psz_label[8];
} p_crop_values[] = {
    { "", N_("Default") },
    { "16:10", "16:10" },
    { "16:9", "16:9" },
    { "4:3", "4:3" },
    { "185:100", "1.85:1" },
    { "221:100", "2.21:1" },
    { "235:100", "2.35:1" },
    { "239:100", "2.39:1" },
    { "5:3", "5:3" },
    { "5:4", "5:4" },
    { "1:1", "1:1" },
};

static const struct
{
    char psz_value[8];
    char psz_label[12];
} p_aspect_ratio_values[] = {
    { "", N_("Default") },
    { "16:9", "16:9" },
    { "4:3", "4:3" },
    { "1:1", "1:1" },
    { "16:10", "16:10" },
    { "221:100", "2.21:1" },
    { "235:100", "2.35:1" },
    { "239:100", "2.39:1" },
    { "5:4", "5:4" },
    { "fill", N_("Fill Window") },
};

static const struct
{
    enum heimdall_video_fitting fit;
    char psz_label[15];
} p_fit_values[] = {
    { HEIMDALL_VIDEO_FIT_NONE,    N_("None") },
    { HEIMDALL_VIDEO_FIT_SMALLER, N_("Inside Window") },
    { HEIMDALL_VIDEO_FIT_LARGER,  N_("Outside Window") },
    { HEIMDALL_VIDEO_FIT_WIDTH,   N_("Window Width") },
    { HEIMDALL_VIDEO_FIT_HEIGHT,  N_("Window Height") },
};

static const struct
{
    int i_value;
    char psz_label[13];
} p_3D_output_format_values[] = {
    { VIDEO_STEREO_OUTPUT_AUTO, N_("Auto") },
    { VIDEO_STEREO_OUTPUT_STEREO, N_("Stereo") },
    { VIDEO_STEREO_OUTPUT_LEFT_ONLY, N_("Left Only") },
    { VIDEO_STEREO_OUTPUT_RIGHT_ONLY, N_("Right Only") },
    { VIDEO_STEREO_OUTPUT_SIDE_BY_SIDE, N_("Side-by-Side") },
};

static void AddCustomRatios( vout_thread_t *p_vout, const char *psz_var,
                             char *psz_list )
{
    assert( psz_list );

    char *psz_cur = psz_list;
    char *psz_next;
    while( psz_cur && *psz_cur )
    {
        heimdall_value_t val;
        psz_next = strchr( psz_cur, ',' );
        if( psz_next )
        {
            *psz_next = '\0';
            psz_next++;
        }
        val.psz_string = psz_cur;
        var_Change( p_vout, psz_var, HEIMDALL_VAR_ADDCHOICE, val,
                    (const char *)psz_cur );
        psz_cur = psz_next;
    }
}

enum heimdall_video_fitting var_InheritFit(heimdall_object_t *obj)
{
    int64_t v = var_InheritInteger(obj, "fit");
    /* Safe variable => paranoid checks */
    switch (v) {
        case HEIMDALL_VIDEO_FIT_SMALLER:
        case HEIMDALL_VIDEO_FIT_LARGER:
        case HEIMDALL_VIDEO_FIT_WIDTH:
        case HEIMDALL_VIDEO_FIT_HEIGHT:
            return v;
        default:
            return HEIMDALL_VIDEO_FIT_SMALLER;
    }
}

void vout_CreateVars( vout_thread_t *p_vout )
{
    heimdall_value_t val;
    char *psz_buf;

    /* Create a few object variables we'll need later on */
    var_Create( p_vout, "snapshot-num", HEIMDALL_VAR_INTEGER );
    var_SetInteger( p_vout, "snapshot-num", 1 );

    var_Create( p_vout, "width", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT );
    var_Create( p_vout, "height", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT );
    var_Create( p_vout, "align", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT );

    var_Create( p_vout, "mouse-hide-timeout",
                HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT );

    /* Add variables to manage scaling video */
    var_Create( p_vout, "autoscale", HEIMDALL_VAR_BOOL | HEIMDALL_VAR_DOINHERIT
                | HEIMDALL_VAR_ISCOMMAND );

    var_Create( p_vout, "zoom", HEIMDALL_VAR_FLOAT | HEIMDALL_VAR_ISCOMMAND |
                HEIMDALL_VAR_DOINHERIT );
    var_Change( p_vout, "zoom", HEIMDALL_VAR_SETTEXT, _("Zoom") );
    for( size_t i = 0; i < ARRAY_SIZE(p_zoom_values); i++ )
    {
        val.f_float = p_zoom_values[i].f_value;
        var_Change( p_vout, "zoom", HEIMDALL_VAR_ADDCHOICE, val,
                    heimdall_gettext( p_zoom_values[i].psz_label ) );
    }

    var_Create( p_vout, "video-stereo-mode", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT );
    var_Change( p_vout, "video-stereo-mode", HEIMDALL_VAR_SETTEXT, _("video-stereo-output"), NULL );

    for( size_t i = 0; i < ARRAY_SIZE(p_3D_output_format_values); i++ )
    {
        val.i_int = p_3D_output_format_values[i].i_value;
        var_Change( p_vout, "video-stereo-mode", HEIMDALL_VAR_ADDCHOICE, val,
                    heimdall_gettext( p_3D_output_format_values[i].psz_label ) );
    }

    /* Crop offset vars */
    var_Create( p_vout, "crop-left", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_ISCOMMAND );
    var_Create( p_vout, "crop-top", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_ISCOMMAND );
    var_Create( p_vout, "crop-right", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_ISCOMMAND );
    var_Create( p_vout, "crop-bottom", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_ISCOMMAND );

    /* Crop object var */
    var_Create( p_vout, "crop", HEIMDALL_VAR_STRING | HEIMDALL_VAR_ISCOMMAND |
                HEIMDALL_VAR_DOINHERIT );
    var_Change( p_vout, "crop", HEIMDALL_VAR_SETTEXT, _("Crop") );

    for( size_t i = 0; i < ARRAY_SIZE(p_crop_values); i++ )
    {
        val.psz_string = (char*)p_crop_values[i].psz_value;
        var_Change( p_vout, "crop", HEIMDALL_VAR_ADDCHOICE, val,
                    p_crop_values[i].psz_label );
    }

    /* Add custom crop ratios */
    psz_buf = var_CreateGetNonEmptyString( p_vout, "custom-crop-ratios" );
    if( psz_buf )
    {
        AddCustomRatios( p_vout, "crop", psz_buf );
        free( psz_buf );
    }

    /* Monitor pixel aspect-ratio */
    var_Create( p_vout, "monitor-par", HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT );

    /* Aspect-ratio object var */
    var_Create( p_vout, "aspect-ratio", HEIMDALL_VAR_STRING | HEIMDALL_VAR_ISCOMMAND |
                HEIMDALL_VAR_DOINHERIT );
    var_Change( p_vout, "aspect-ratio", HEIMDALL_VAR_SETTEXT, _("Aspect ratio") );

    for( size_t i = 0; i < ARRAY_SIZE(p_aspect_ratio_values); i++ )
    {
        val.psz_string = (char*)p_aspect_ratio_values[i].psz_value;
        var_Change( p_vout, "aspect-ratio", HEIMDALL_VAR_ADDCHOICE, val,
                    heimdall_gettext(p_aspect_ratio_values[i].psz_label) );
    }

    /* Add custom aspect ratios */
    psz_buf = var_CreateGetNonEmptyString( p_vout, "custom-aspect-ratios" );
    if( psz_buf )
    {
        AddCustomRatios( p_vout, "aspect-ratio", psz_buf );
        free( psz_buf );
    }

    /* display fit */
    var_Create( p_vout, "fit", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_ISCOMMAND |
                HEIMDALL_VAR_DOINHERIT );
    var_Change( p_vout, "fit", HEIMDALL_VAR_SETTEXT, _("Fit Mode") );

    for( size_t i = 0; i < ARRAY_SIZE(p_fit_values); i++ )
    {
        val.i_int = p_fit_values[i].fit;
        var_Change( p_vout, "fit", HEIMDALL_VAR_ADDCHOICE, val,
                    p_fit_values[i].psz_label );
    }

    /* Add a variable to indicate if the window should be on top of others */
    var_Create( p_vout, "video-on-top", HEIMDALL_VAR_BOOL | HEIMDALL_VAR_DOINHERIT
                | HEIMDALL_VAR_ISCOMMAND );
    var_Change( p_vout, "video-on-top", HEIMDALL_VAR_SETTEXT,
                _("Always on top") );

    /* Add a variable to indicate if the window should be below all others */
    var_Create( p_vout, "video-wallpaper", HEIMDALL_VAR_BOOL | HEIMDALL_VAR_DOINHERIT );

    /* Add a variable to indicate whether we want window decoration or not */
    var_Create( p_vout, "video-deco", HEIMDALL_VAR_BOOL | HEIMDALL_VAR_DOINHERIT );

    /* Add a fullscreen variable */
    var_Create( p_vout, "fullscreen",
                HEIMDALL_VAR_BOOL | HEIMDALL_VAR_DOINHERIT | HEIMDALL_VAR_ISCOMMAND );
    var_Change( p_vout, "fullscreen", HEIMDALL_VAR_SETTEXT, _("Fullscreen") );

    /* Add a snapshot variable */
    var_Create( p_vout, "video-snapshot", HEIMDALL_VAR_VOID | HEIMDALL_VAR_ISCOMMAND );
    var_Change( p_vout, "video-snapshot", HEIMDALL_VAR_SETTEXT, _("Snapshot") );

    /* Add a video-filter variable */
    var_Create( p_vout, "video-filter",
                HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT | HEIMDALL_VAR_ISCOMMAND );

    /* Add a sub-source variable */
    var_Create( p_vout, "sub-source",
                HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT | HEIMDALL_VAR_ISCOMMAND );

    /* Add a sub-filter variable */
    var_Create( p_vout, "sub-filter",
                HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT | HEIMDALL_VAR_ISCOMMAND );

    /* Add sub-margin variable */
    var_Create( p_vout, "sub-margin",
                HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT | HEIMDALL_VAR_ISCOMMAND );

    /* Add secondary-sub-margin variable (dual subtitles) */
    var_Create( p_vout, "secondary-sub-margin",
                HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT | HEIMDALL_VAR_ISCOMMAND );
    var_AddCallback( p_vout, "secondary-sub-margin", SecondarySubMarginCallback, NULL );

    /* Mouse coordinates */
    var_Create( p_vout, "mouse-button-down", HEIMDALL_VAR_INTEGER );
    var_Create( p_vout, "mouse-moved", HEIMDALL_VAR_COORDS );

    /* Device orientation */
    var_Create( p_vout, "viewpoint-moved", HEIMDALL_VAR_ADDRESS );

    /* Viewpoint */
    var_Create( p_vout, "viewpoint", HEIMDALL_VAR_ADDRESS  );
    var_Create( p_vout, "viewpoint-changeable", HEIMDALL_VAR_BOOL );

    /* SPU in full window */
    var_Create( p_vout, "spu-fill", HEIMDALL_VAR_BOOL | HEIMDALL_VAR_DOINHERIT
                | HEIMDALL_VAR_ISCOMMAND );

    var_Create(p_vout, "override-projection", HEIMDALL_VAR_BOOL);
    var_SetBool(p_vout, "override-projection", false);

    var_Create(p_vout, "projection-mode", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT);
}

void vout_IntfInit( vout_thread_t *p_vout )
{
    var_AddCallback( p_vout, "autoscale", AutoScaleCallback, NULL );
    var_AddCallback( p_vout, "fit", FitCallback, NULL );
    var_AddCallback( p_vout, "zoom", ZoomCallback, NULL );
    var_AddCallback( p_vout, "crop-left", CropBorderCallback, NULL );
    var_AddCallback( p_vout, "crop-top", CropBorderCallback, NULL );
    var_AddCallback( p_vout, "crop-right", CropBorderCallback, NULL );
    var_AddCallback( p_vout, "crop-bottom", CropBorderCallback, NULL );
    var_AddCallback( p_vout, "crop", CropCallback, NULL );
    var_AddCallback( p_vout, "aspect-ratio", AspectCallback, NULL );
    var_AddCallback( p_vout, "video-on-top", AboveCallback, NULL );
    var_AddCallback( p_vout, "video-wallpaper", WallPaperCallback, NULL );
    var_AddCallback( p_vout, "fullscreen", FullscreenCallback, NULL );
    var_AddCallback( p_vout, "video-snapshot", SnapshotCallback, NULL );
    var_AddCallback( p_vout, "video-filter", VideoFilterCallback, NULL );
    var_AddCallback( p_vout, "sub-source", SubSourceCallback, NULL );
    var_AddCallback( p_vout, "sub-filter", SubFilterCallback, NULL );
    var_AddCallback( p_vout, "sub-margin", SubMarginCallback, NULL );
    var_AddCallback( p_vout, "viewpoint", ViewpointCallback, NULL );
    var_AddCallback( p_vout, "override-projection", OverrideProjectionCallback, NULL );
    var_AddCallback( p_vout, "projection-mode", ChangeProjectionCallback, NULL );
    var_AddCallback(p_vout, "video-stereo-mode", Stereo3DCallback, NULL);
}

void vout_IntfReinit( vout_thread_t *p_vout )
{
    var_TriggerCallback( p_vout, "video-on-top" );
    var_TriggerCallback( p_vout, "video-wallpaper" );

    var_TriggerCallback( p_vout, "video-filter" );
    var_TriggerCallback( p_vout, "sub-source" );
    var_TriggerCallback( p_vout, "sub-filter" );
    /* !Warn those will trigger also heimdall_player_vout_OSDCallback and
        cause unwanted OSD on vout start. Filter out it there. */
    var_TriggerCallback( p_vout, "sub-margin" );
    var_TriggerCallback( p_vout, "secondary-sub-margin" );

    var_TriggerCallback( p_vout, "projection-mode" );
    var_TriggerCallback(p_vout, "video-stereo-mode");
}

void vout_IntfDeinit(heimdall_object_t *obj)
{
    var_DelCallback(obj, "viewpoint", ViewpointCallback, NULL);
    var_DelCallback(obj, "sub-margin", SubMarginCallback, NULL);
    var_DelCallback(obj, "secondary-sub-margin", SecondarySubMarginCallback, NULL);
    var_DelCallback(obj, "sub-filter", SubFilterCallback, NULL);
    var_DelCallback(obj, "sub-source", SubSourceCallback, NULL);
    var_DelCallback(obj, "video-filter", VideoFilterCallback, NULL);
    var_DelCallback(obj, "video-snapshot", SnapshotCallback, NULL);
    var_DelCallback(obj, "fullscreen", FullscreenCallback, NULL);
    var_DelCallback(obj, "video-wallpaper", WallPaperCallback, NULL);
    var_DelCallback(obj, "video-on-top", AboveCallback, NULL);
    var_DelCallback(obj, "aspect-ratio", AspectCallback, NULL);
    var_DelCallback(obj, "crop", CropCallback, NULL);
    var_DelCallback(obj, "crop-bottom", CropBorderCallback, NULL);
    var_DelCallback(obj, "crop-right", CropBorderCallback, NULL);
    var_DelCallback(obj, "crop-top", CropBorderCallback, NULL);
    var_DelCallback(obj, "crop-left", CropBorderCallback, NULL);
    var_DelCallback(obj, "zoom", ZoomCallback, NULL);
    var_DelCallback(obj, "fit", FitCallback, NULL);
    var_DelCallback(obj, "autoscale", AutoScaleCallback, NULL);
    var_DelCallback(obj, "override-projection", OverrideProjectionCallback, NULL);
    var_DelCallback(obj, "projection-mode", ChangeProjectionCallback, NULL);
    var_DelCallback(obj, "video-stereo-mode", Stereo3DCallback, NULL);
}

/*****************************************************************************
 * vout_Snapshot: generates a snapshot.
 *****************************************************************************/
/**
 * This function will inject a subpicture into the vout with the provided
 * picture
 */
static int VoutSnapshotPip( vout_thread_t *p_vout, picture_t *p_pic )
{
    subpicture_t *p_subpic = subpicture_NewFromPicture( HEIMDALL_OBJECT(p_vout),
                                                        p_pic, HEIMDALL_CODEC_YUVA );
    if( !p_subpic )
        return HEIMDALL_EGENERIC;

    /* FIXME SPU_DEFAULT_CHANNEL is not good (used by the text) but
     * hardcoded 0 doesn't seem right */
    p_subpic->i_channel = 0;
    p_subpic->i_start = heimdall_tick_now();
    p_subpic->i_stop  = p_subpic->i_start + HEIMDALL_TICK_FROM_SEC(4);
    p_subpic->b_ephemer = true;
    p_subpic->b_fade = true;

    /* Reduce the picture to 1/4^2 of the screen */
    p_subpic->i_original_picture_width  *= 4;
    p_subpic->i_original_picture_height *= 4;

    vout_PutSubpicture( p_vout, p_subpic );
    return HEIMDALL_SUCCESS;
}

/**
 * This function will display the name and a PIP of the provided snapshot
 */
static void VoutOsdSnapshot( vout_thread_t *p_vout, picture_t *p_pic, const char *psz_filename )
{
    msg_Dbg( p_vout, "snapshot taken (%s)", psz_filename );
    vout_OSDMessage( p_vout, VOUT_SPU_CHANNEL_OSD, "%s", psz_filename );

    if( var_InheritBool( p_vout, "snapshot-preview" ) )
    {
        if( VoutSnapshotPip( p_vout, p_pic ) )
            msg_Warn( p_vout, "Failed to display snapshot" );
    }
}

/**
 * This function will handle a snapshot request
 */
static void VoutSaveSnapshot( vout_thread_t *p_vout )
{
    char *psz_path = var_InheritString( p_vout, "snapshot-path" );
    char *psz_format = var_InheritString( p_vout, "snapshot-format" );
    char *psz_prefix = var_InheritString( p_vout, "snapshot-prefix" );

    /* */
    picture_t *p_picture;
    block_t *p_image;

    /* 500ms timeout
     * XXX it will cause trouble with low fps video (< 2fps) */
    if( vout_GetSnapshot( p_vout, &p_image, &p_picture, NULL, psz_format, HEIMDALL_TICK_FROM_MS(500) ) )
    {
        p_picture = NULL;
        p_image = NULL;
        goto exit;
    }

    if( !psz_path )
    {
        psz_path = vout_snapshot_GetDirectory();
        if( !psz_path )
        {
            msg_Err( p_vout, "no path specified for snapshots" );
            goto exit;
        }
    }

    vout_snapshot_save_cfg_t cfg;
    memset( &cfg, 0, sizeof(cfg) );
    cfg.is_sequential = var_InheritBool( p_vout, "snapshot-sequential" );
    cfg.sequence = var_GetInteger( p_vout, "snapshot-num" );
    cfg.path = psz_path;
    cfg.format = psz_format;
    cfg.prefix_fmt = psz_prefix;

    char *psz_filename;
    int  i_sequence;
    if (vout_snapshot_SaveImage( &psz_filename, &i_sequence,
                                 p_image, p_vout, &cfg ) )
        goto exit;
    if( cfg.is_sequential )
        var_SetInteger( p_vout, "snapshot-num", i_sequence + 1 );

    VoutOsdSnapshot( p_vout, p_picture, psz_filename );

    /* signal creation of a new snapshot file */
    var_SetString( heimdall_object_instance(p_vout), "snapshot-file", psz_filename );

    free( psz_filename );

exit:
    if( p_image )
        block_Release( p_image );
    if( p_picture )
        picture_Release( p_picture );
    free( psz_prefix );
    free( psz_format );
    free( psz_path );
}

bool vout_ParseCrop(struct vout_crop *restrict cfg, const char *crop_str)
{
    float fnum, fden;

    if (sscanf(crop_str, "%u:%u", &cfg->ratio.num, &cfg->ratio.den) == 2) {
        if (cfg->ratio.num != 0 && cfg->ratio.den != 0)
            cfg->mode = VOUT_CROP_RATIO;
        else
            cfg->mode = VOUT_CROP_NONE;
    } else if (sscanf(crop_str, "%ux%u+%u+%u",
                      &cfg->window.width, &cfg->window.height,
                      &cfg->window.x, &cfg->window.y) == 4) {
        cfg->mode = VOUT_CROP_WINDOW;
    } else if (sscanf(crop_str, "%u+%u+%u+%u",
                      &cfg->border.left, &cfg->border.top,
                      &cfg->border.right, &cfg->border.bottom) == 4) {
        cfg->mode = VOUT_CROP_BORDER;
    } else if (heimdall_sscanf_c(crop_str, "%f:%f", &fnum, &fden) == 2) {
        long num = lroundf(ldexp(fnum, 24/*-bit mantissa */));
        long den = lroundf(ldexp(fden, 24));
        long gcd = GCD(num, den);

        cfg->mode = VOUT_CROP_RATIO;
        cfg->ratio.num = num / gcd;
        cfg->ratio.den = den / gcd;
    } else if (*crop_str == '\0') {
        cfg->mode = VOUT_CROP_NONE;
    } else {
        return false;
    }
    return true;
}

/*****************************************************************************
 * Object variables callbacks
 *****************************************************************************/
static int CropCallback( heimdall_object_t *object, char const *cmd,
                         heimdall_value_t oldval, heimdall_value_t newval, void *data )
{
    vout_thread_t *vout = (vout_thread_t *)object;
    HEIMDALL_UNUSED(cmd); HEIMDALL_UNUSED(oldval); HEIMDALL_UNUSED(data);
    struct vout_crop crop;

    if (vout_ParseCrop(&crop, newval.psz_string))
        vout_ChangeCrop(vout, &crop);
    else
        msg_Err(object, "Unknown crop format (%s)", newval.psz_string);

    return HEIMDALL_SUCCESS;
}

static int CropBorderCallback(heimdall_object_t *object, char const *cmd,
                              heimdall_value_t oldval, heimdall_value_t newval, void *data)
{
    char buf[4 * 21];

    snprintf(buf, sizeof (buf), "%"PRIu64"+%"PRIu64"+%"PRIu64"+%"PRIu64,
             var_GetInteger(object, "crop-left"),
             var_GetInteger(object, "crop-top"),
             var_GetInteger(object, "crop-right"),
             var_GetInteger(object, "crop-bottom"));
    var_SetString(object, "crop", buf);

    HEIMDALL_UNUSED(cmd); HEIMDALL_UNUSED(oldval); HEIMDALL_UNUSED(data); HEIMDALL_UNUSED(newval);
    return HEIMDALL_SUCCESS;
}

bool vout_ParseDisplayAspectRatio(heimdall_rational_t *ar, const char *ar_str)
{
    if (*ar_str == '\0') {
        *ar = HEIMDALL_DAR_FROM_SOURCE;
        return true;
    }
    if (strcmp(ar_str,"fill")==0) {
        *ar = HEIMDALL_DAR_FILL_DISPLAY;
        return true;
    }
    if (sscanf(ar_str, "%u:%u", &ar->num, &ar->den) == 2 &&
        (ar->num != 0) == (ar->den != 0))
        return true;
    return false;
}

static int AspectCallback( heimdall_object_t *object, char const *cmd,
                         heimdall_value_t oldval, heimdall_value_t newval, void *data )
{
    vout_thread_t *vout = (vout_thread_t *)object;
    HEIMDALL_UNUSED(cmd); HEIMDALL_UNUSED(oldval); HEIMDALL_UNUSED(data);
    heimdall_rational_t ar;

    if (vout_ParseDisplayAspectRatio(&ar, newval.psz_string))
        vout_ChangeDisplayAspectRatio(vout, ar);
    return HEIMDALL_SUCCESS;
}

static int AutoScaleCallback( heimdall_object_t *obj, char const *name,
                              heimdall_value_t prev, heimdall_value_t cur, void *data )
{
    vout_thread_t *p_vout = (vout_thread_t *)obj;
    enum heimdall_video_fitting fit = cur.b_bool ? var_InheritFit(obj)
                                            : HEIMDALL_VIDEO_FIT_NONE;

    (void) name; (void) prev; (void) data;
    vout_ChangeDisplayFitting(p_vout, fit);
    return HEIMDALL_SUCCESS;
}

static int FitCallback( heimdall_object_t *obj, char const *name,
                        heimdall_value_t prev, heimdall_value_t cur, void *data )
{
    vout_thread_t *p_vout = (vout_thread_t *)obj;
    enum heimdall_video_fitting fit = cur.i_int;

    (void) name; (void) prev; (void) data;
    vout_ChangeDisplayFitting(p_vout, fit);
    return HEIMDALL_SUCCESS;
}

static int OverrideProjectionCallback( heimdall_object_t *obj, char const *name,
                              heimdall_value_t prev, heimdall_value_t cur, void *data )
{
    HEIMDALL_UNUSED(name); HEIMDALL_UNUSED(prev); HEIMDALL_UNUSED(data);
    vout_thread_t *vout = (vout_thread_t *)obj;

    vout_OSDMessage(vout, VOUT_SPU_CHANNEL_OSD, "Projection: %s", cur.b_bool ? "disabled" : "enabled");
    vout_ToggleProjection(vout, !cur.b_bool);
    return HEIMDALL_SUCCESS;
}

static int ChangeProjectionCallback( heimdall_object_t *obj, char const *name,
                              heimdall_value_t prev, heimdall_value_t cur, void *data )
{
    HEIMDALL_UNUSED(name); HEIMDALL_UNUSED(prev); HEIMDALL_UNUSED(data);
    vout_thread_t *vout = (vout_thread_t *)obj;

    const char *projection;
    switch(cur.i_int)
    {
        case -1:                                        projection = "source"; break;
        case PROJECTION_MODE_RECTANGULAR:               projection = "rectangular"; break;
        case PROJECTION_MODE_EQUIRECTANGULAR:           projection = "equirectangular"; break;
        case PROJECTION_MODE_CUBEMAP_LAYOUT_STANDARD:   projection = "cubemap (standard)"; break;
        default: projection = "unknown"; break;
    }

    if (cur.i_int != prev.i_int)
        vout_OSDMessage(vout, VOUT_SPU_CHANNEL_OSD,
                        "Projection: %s", projection);
    if (cur.i_int == -1)
    {
        vout_ResetProjection(vout);
        return HEIMDALL_SUCCESS;
    }
    else if (cur.i_int < 0 || cur.i_int > (int)PROJECTION_MODE_CUBEMAP_LAYOUT_STANDARD)
    {
        return HEIMDALL_EGENERIC;
    }

    vout_ChangeProjection(vout, cur.i_int);
    return HEIMDALL_SUCCESS;
}

static int ZoomCallback( heimdall_object_t *obj, char const *name,
                         heimdall_value_t prev, heimdall_value_t cur, void *data )
{
    vout_thread_t *p_vout = (vout_thread_t *)obj;

    (void) name; (void) prev; (void) data;
    vout_ChangeZoom(p_vout, 1000 * cur.f_float, 1000);
    return HEIMDALL_SUCCESS;
}

static int Stereo3DCallback( heimdall_object_t *obj, char const *name,
                         heimdall_value_t prev, heimdall_value_t cur, void *data )
{
    vout_thread_t *p_vout = (vout_thread_t *)obj;
    (void) name; (void) prev; (void) data;
    vout_ControlChangeStereo( p_vout, cur.i_int );
    return HEIMDALL_SUCCESS;
}

static int AboveCallback( heimdall_object_t *obj, char const *name,
                          heimdall_value_t prev, heimdall_value_t cur, void *data )
{
    vout_ChangeWindowState((vout_thread_t *)obj,
        cur.b_bool ? HEIMDALL_WINDOW_STATE_ABOVE : HEIMDALL_WINDOW_STATE_NORMAL);
    (void) name; (void) prev; (void) data;
    return HEIMDALL_SUCCESS;
}

static int WallPaperCallback( heimdall_object_t *obj, char const *name,
                              heimdall_value_t prev, heimdall_value_t cur, void *data )
{
    vout_thread_t *vout = (vout_thread_t *)obj;

    if( cur.b_bool )
    {
        vout_ChangeWindowState(vout, HEIMDALL_WINDOW_STATE_BELOW);
        vout_ChangeFullscreen(vout, NULL);
    }
    else
    {
        var_TriggerCallback( obj, "fullscreen" );
        var_TriggerCallback( obj, "video-on-top" );
    }
    (void) name; (void) prev; (void) data;
    return HEIMDALL_SUCCESS;
}

static int FullscreenCallback( heimdall_object_t *p_this, char const *psz_cmd,
                       heimdall_value_t oldval, heimdall_value_t newval, void *p_data )
{
    vout_thread_t *p_vout = (vout_thread_t *)p_this;
    (void)psz_cmd; (void) oldval; (void)p_data;

    if( newval.b_bool )
        vout_ChangeFullscreen(p_vout, NULL);
    else
        vout_ChangeWindowed(p_vout);
    return HEIMDALL_SUCCESS;
}

static int SnapshotCallback( heimdall_object_t *p_this, char const *psz_cmd,
                       heimdall_value_t oldval, heimdall_value_t newval, void *p_data )
{
    vout_thread_t *p_vout = (vout_thread_t *)p_this;
    HEIMDALL_UNUSED(psz_cmd); HEIMDALL_UNUSED(oldval);
    HEIMDALL_UNUSED(newval); HEIMDALL_UNUSED(p_data);

    VoutSaveSnapshot( p_vout );
    return HEIMDALL_SUCCESS;
}

static int VideoFilterCallback( heimdall_object_t *p_this, char const *psz_cmd,
                                heimdall_value_t oldval, heimdall_value_t newval, void *p_data)
{
    vout_thread_t *p_vout = (vout_thread_t *)p_this;
    HEIMDALL_UNUSED(psz_cmd); HEIMDALL_UNUSED(oldval); HEIMDALL_UNUSED(p_data);

    vout_ControlChangeFilters( p_vout, newval.psz_string );
    return HEIMDALL_SUCCESS;
}

static int SubSourceCallback( heimdall_object_t *p_this, char const *psz_cmd,
                              heimdall_value_t oldval, heimdall_value_t newval, void *p_data)
{
    vout_thread_t *p_vout = (vout_thread_t *)p_this;
    HEIMDALL_UNUSED(psz_cmd); HEIMDALL_UNUSED(oldval); HEIMDALL_UNUSED(p_data);

    vout_ControlChangeSubSources( p_vout, newval.psz_string );
    return HEIMDALL_SUCCESS;
}

static int SubFilterCallback( heimdall_object_t *p_this, char const *psz_cmd,
                              heimdall_value_t oldval, heimdall_value_t newval, void *p_data)
{
    vout_thread_t *p_vout = (vout_thread_t *)p_this;
    HEIMDALL_UNUSED(psz_cmd); HEIMDALL_UNUSED(oldval); HEIMDALL_UNUSED(p_data);

    vout_ControlChangeSubFilters( p_vout, newval.psz_string );
    return HEIMDALL_SUCCESS;
}

static int SubMarginCallback( heimdall_object_t *p_this, char const *psz_cmd,
                              heimdall_value_t oldval, heimdall_value_t newval, void *p_data)
{
    vout_thread_t *p_vout = (vout_thread_t *)p_this;
    HEIMDALL_UNUSED(psz_cmd); HEIMDALL_UNUSED(oldval); HEIMDALL_UNUSED(p_data);

    vout_ChangeSpuChannelMargin(p_vout, HEIMDALL_VOUT_ORDER_PRIMARY, newval.i_int);
    return HEIMDALL_SUCCESS;
}

static int SecondarySubMarginCallback( heimdall_object_t *p_this, char const *psz_cmd,
                              heimdall_value_t oldval, heimdall_value_t newval, void *p_data)
{
    vout_thread_t *p_vout = (vout_thread_t *)p_this;
    HEIMDALL_UNUSED(psz_cmd); HEIMDALL_UNUSED(oldval); HEIMDALL_UNUSED(p_data);

    vout_ChangeSpuChannelMargin(p_vout, HEIMDALL_VOUT_ORDER_SECONDARY, newval.i_int);
    return HEIMDALL_SUCCESS;
}

static int ViewpointCallback( heimdall_object_t *p_this, char const *psz_cmd,
                              heimdall_value_t oldval, heimdall_value_t newval, void *p_data)
{
    vout_thread_t *p_vout = (vout_thread_t *)p_this;
    HEIMDALL_UNUSED(psz_cmd); HEIMDALL_UNUSED(oldval); HEIMDALL_UNUSED(p_data);

    if( newval.p_address != NULL )
        vout_ChangeViewpoint(p_vout, newval.p_address);
    return HEIMDALL_SUCCESS;
}
