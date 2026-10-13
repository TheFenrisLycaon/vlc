/*****************************************************************************
 * libheimdall_audio.c: New libheimdall audio control API
 *****************************************************************************
 * Copyright (C) 2006 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Filippo Carone <filippo@carone.org>
 *          Jean-Paul Saman <jpsaman _at_ m2x _dot_ nl>
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
#include <math.h>

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_renderer_discoverer.h>
#include <heimdall/libheimdall_picture.h>
#include <heimdall/libheimdall_media.h>
#include <heimdall/libheimdall_media_player.h>

#include <heimdall_common.h>
#include <heimdall_aout.h>
#include <heimdall_modules.h>

#include "libheimdall_internal.h"
#include "media_player_internal.h"

/*
 * Remember to release the returned audio_output_t since it is locked at
 * the end of this function.
 */
static audio_output_t *GetAOut( libheimdall_media_player_t *mp )
{
    assert( mp != NULL );

    audio_output_t *p_aout = heimdall_player_aout_Hold(mp->player);
    if( p_aout == NULL )
        libheimdall_printerr( "No active audio output" );
    return p_aout;
}

/*****************************************
 * Get the list of available audio outputs
 *****************************************/
libheimdall_audio_output_t *
        libheimdall_audio_output_list_get( libheimdall_instance_t *p_instance )
{
    size_t count;
    module_t **module_list = module_list_get( &count );
    libheimdall_audio_output_t *list = NULL;

    for (size_t i = 0; i < count; i++)
    {
        module_t *module = module_list[i];

        if( !module_provides( module, "audio output" ) )
            continue;

        libheimdall_audio_output_t *item = malloc( sizeof( *item ) );
        if( unlikely(item == NULL) )
        {
    error:
            libheimdall_printerr( "Not enough memory" );
            libheimdall_audio_output_list_release( list );
            list = NULL;
            break;
        }

        item->psz_name = strdup( module_get_object( module ) );
        item->psz_description = strdup( module_GetLongName( module ) );
        if( unlikely(item->psz_name == NULL || item->psz_description == NULL) )
        {
            free( item->psz_name );
            free( item->psz_description );
            free( item );
            goto error;
        }
        item->p_next = list;
        list = item;
    }
    module_list_free( module_list );

    HEIMDALL_UNUSED( p_instance );
    return list;
}

/********************************************
 * Free the list of available audio outputs
 ***********************************************/
void libheimdall_audio_output_list_release( libheimdall_audio_output_t *list )
{
    while( list != NULL )
    {
        libheimdall_audio_output_t *next = list->p_next;

        free( list->psz_name );
        free( list->psz_description );
        free( list );
        list = next;
    }
}


/***********************
 * Set the audio output.
 ***********************/
int libheimdall_audio_output_set( libheimdall_media_player_t *mp, const char *psz_name )
{
    char *value;

    if( !module_exists( psz_name )
     || asprintf( &value, "%s,none", psz_name ) == -1 )
        return -1;
    var_SetString( mp, "aout", value );
    free( value );

    heimdall_player_aout_Reset(mp->player);
    return 0;
}

libheimdall_audio_output_device_t *
libheimdall_audio_output_device_enum( libheimdall_media_player_t *mp )
{
    audio_output_t *aout = GetAOut( mp );
    if( aout == NULL )
        return NULL;

    libheimdall_audio_output_device_t *list, **pp = &list;
    char **values, **texts;

    int n = aout_DevicesList( aout, &values, &texts );
    aout_Release(aout);
    if( n < 0 )
        goto err;

    for (int i = 0; i < n; i++)
    {
        libheimdall_audio_output_device_t *item = malloc( sizeof(*item) );
        if( unlikely(item == NULL) )
        {
            free( texts[i] );
            free( values[i] );
            continue;
        }

        *pp = item;
        pp = &item->p_next;
        item->psz_device = values[i];
        item->psz_description = texts[i];
    }

    free( texts );
    free( values );
err:
    *pp = NULL;
    return list;
}

void libheimdall_audio_output_device_list_release( libheimdall_audio_output_device_t *l )
{
    while( l != NULL )
    {
        libheimdall_audio_output_device_t *next = l->p_next;

        free( l->psz_description );
        free( l->psz_device );
        free( l );
        l = next;
    }
}

/*****************************
 * Set device for using
 *****************************/
int libheimdall_audio_output_device_set( libheimdall_media_player_t *mp,
                                    const char *devid )
{
    if( devid == NULL )
        return -1;

    audio_output_t *aout = GetAOut( mp );
    if( aout == NULL )
        return -1;

    int ret = aout_DeviceSet( aout, devid );
    aout_Release(aout);
    return ret;
}

char *libheimdall_audio_output_device_get( libheimdall_media_player_t *mp )
{
    audio_output_t *aout = GetAOut( mp );
    if( aout == NULL )
        return NULL;

    char *devid = aout_DeviceGet( aout );

    aout_Release(aout);

    return devid;
}

void libheimdall_audio_toggle_mute( libheimdall_media_player_t *mp )
{
    int mute = libheimdall_audio_get_mute( mp );
    if( mute != -1 )
        libheimdall_audio_set_mute( mp, !mute );
}

int libheimdall_audio_get_mute( libheimdall_media_player_t *mp )
{
    int mute = -1;

    audio_output_t *aout = GetAOut( mp );
    if( aout != NULL )
    {
        mute = aout_MuteGet( aout );
        aout_Release(aout);
    }
    return mute;
}

void libheimdall_audio_set_mute( libheimdall_media_player_t *mp, int mute )
{
    audio_output_t *aout = GetAOut( mp );
    if( aout != NULL )
    {
        mute = aout_MuteSet( aout, mute );
        aout_Release(aout);
    }
}

int libheimdall_audio_get_volume( libheimdall_media_player_t *mp )
{
    int volume = -1;

    audio_output_t *aout = GetAOut( mp );
    if( aout != NULL )
    {
        float vol = aout_VolumeGet( aout );
        aout_Release(aout);
        volume = lroundf( vol * 100.f );
    }
    return volume;
}

int libheimdall_audio_set_volume( libheimdall_media_player_t *mp, int volume )
{
    float vol = volume / 100.f;
    if (!isgreaterequal(vol, 0.f))
    {
        libheimdall_printerr( "Volume out of range" );
        return -1;
    }

    int ret = -1;
    audio_output_t *aout = GetAOut( mp );
    if( aout != NULL )
    {
        ret = aout_VolumeSet( aout, vol );
        aout_Release(aout);
    }
    return ret;
}

/*****************************************************************************
 * libheimdall_audio_get_stereomode : Get the current audio stereo-mode
 *****************************************************************************/
libheimdall_audio_output_stereomode_t libheimdall_audio_get_stereomode( libheimdall_media_player_t *mp )
{
    audio_output_t *p_aout = GetAOut( mp );
    if( !p_aout )
        return libheimdall_AudioStereoMode_Unset;

    int val = var_GetInteger( p_aout, "stereo-mode" );
    aout_Release(p_aout);
    return val;
}

/*****************************************************************************
 * libheimdall_audio_set_stereomode : Set the current audio stereo-mode
 *****************************************************************************/
int libheimdall_audio_set_stereomode( libheimdall_media_player_t *mp, libheimdall_audio_output_stereomode_t mode )
{
    static_assert(libheimdall_AudioStereoMode_Unset == AOUT_VAR_CHAN_UNSET &&
                  libheimdall_AudioStereoMode_Stereo == AOUT_VAR_CHAN_STEREO &&
                  libheimdall_AudioStereoMode_RStereo == AOUT_VAR_CHAN_RSTEREO &&
                  libheimdall_AudioStereoMode_Left == AOUT_VAR_CHAN_LEFT &&
                  libheimdall_AudioStereoMode_Right == AOUT_VAR_CHAN_RIGHT &&
                  libheimdall_AudioStereoMode_Dolbys == AOUT_VAR_CHAN_DOLBYS &&
                  libheimdall_AudioStereoMode_Mono == AOUT_VAR_CHAN_MONO,
                  "Mismatch with stereo-mode LibHeimdall/Heimdall enums");

    audio_output_t *p_aout = GetAOut( mp );
    int ret = 0;

    if( !p_aout )
        return -1;

    if( var_SetInteger( p_aout, "stereo-mode", mode ) < 0 )
    {
        libheimdall_printerr( "Audio stereo-mode out of range" );
        ret = -1;
    }
    aout_Release(p_aout);
    return ret;
}

/*****************************************************************************
 * libheimdall_audio_get_mixmode : Get the current audio mix-mode
 *****************************************************************************/
libheimdall_audio_output_mixmode_t libheimdall_audio_get_mixmode( libheimdall_media_player_t *mp )
{
    audio_output_t *p_aout = GetAOut( mp );
    if( !p_aout )
        return libheimdall_AudioMixMode_Unset;

    int val = var_GetInteger( p_aout, "mix-mode" );
    aout_Release(p_aout);
    return val;
}

/*****************************************************************************
 * libheimdall_audio_set_mixmode : Set the current audio mix-mode
 *****************************************************************************/
int libheimdall_audio_set_mixmode( libheimdall_media_player_t *mp, libheimdall_audio_output_mixmode_t mode )
{
    static_assert(libheimdall_AudioMixMode_Unset == AOUT_VAR_CHAN_UNSET &&
                  libheimdall_AudioMixMode_Stereo == AOUT_MIX_MODE_STEREO &&
                  libheimdall_AudioMixMode_Binaural == AOUT_MIX_MODE_BINAURAL &&
                  libheimdall_AudioMixMode_4_0 == AOUT_MIX_MODE_4_0 &&
                  libheimdall_AudioMixMode_5_1 == AOUT_MIX_MODE_5_1 &&
                  libheimdall_AudioMixMode_7_1 == AOUT_MIX_MODE_7_1,
                  "Mismatch with mix-mode LibHeimdall/Heimdall enums");

    audio_output_t *p_aout = GetAOut( mp );
    int ret = 0;

    if( !p_aout )
        return -1;

    if( var_SetInteger( p_aout, "mix-mode", mode ) < 0 )
    {
        libheimdall_printerr( "Audio mix-mode out of range" );
        ret = -1;
    }
    aout_Release(p_aout);
    return ret;
}

/*****************************************************************************
 * libheimdall_audio_get_delay : Get the current audio delay
 *****************************************************************************/
libheimdall_time_t libheimdall_audio_get_delay( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_tick_t delay = heimdall_player_GetAudioDelay(player);

    heimdall_player_Unlock(player);

    return US_FROM_HEIMDALL_TICK(delay);
}

/*****************************************************************************
 * libheimdall_audio_set_delay : Set the current audio delay
 *****************************************************************************/
int libheimdall_audio_set_delay( libheimdall_media_player_t *p_mi, libheimdall_time_t i_delay )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_SetAudioDelay(player, HEIMDALL_TICK_FROM_US(i_delay),
                             HEIMDALL_PLAYER_WHENCE_ABSOLUTE);

    heimdall_player_Unlock(player);
    /* may not fail anymore, keep int not to break the API */
    return 0;
}

/*****************************************************************************
 * libheimdall_audio_equalizer_get_preset_count : Get the number of equalizer presets
 *****************************************************************************/
unsigned libheimdall_audio_equalizer_get_preset_count( void )
{
    return NB_PRESETS;
}

/*****************************************************************************
 * libheimdall_audio_equalizer_get_preset_name : Get the name for a preset
 *****************************************************************************/
const char *libheimdall_audio_equalizer_get_preset_name( unsigned u_index )
{
    if ( u_index >= NB_PRESETS )
        return NULL;

    return preset_list_text[ u_index ];
}

/*****************************************************************************
 * libheimdall_audio_equalizer_get_band_count : Get the number of equalizer frequency bands
 *****************************************************************************/
unsigned libheimdall_audio_equalizer_get_band_count( void )
{
    return EQZ_BANDS_MAX;
}

/*****************************************************************************
 * libheimdall_audio_equalizer_get_band_frequency : Get the frequency for a band
 *****************************************************************************/
float libheimdall_audio_equalizer_get_band_frequency( unsigned u_index )
{
    if ( u_index >= EQZ_BANDS_MAX )
        return -1.f;

    return f_iso_frequency_table_10b[ u_index ];
}

/*****************************************************************************
 * libheimdall_audio_equalizer_new : Create a new audio equalizer with zeroed values
 *****************************************************************************/
libheimdall_equalizer_t *libheimdall_audio_equalizer_new( void )
{
    libheimdall_equalizer_t *p_equalizer;
    p_equalizer = malloc( sizeof( *p_equalizer ) );
    if ( unlikely( p_equalizer == NULL ) )
        return NULL;

    p_equalizer->f_preamp = 0.f;
    for ( unsigned i = 0; i < EQZ_BANDS_MAX; i++ )
        p_equalizer->f_amp[ i ] = 0.f;

    return p_equalizer;
}

/*****************************************************************************
 * libheimdall_audio_equalizer_new_from_preset : Create a new audio equalizer based on a preset
 *****************************************************************************/
libheimdall_equalizer_t *libheimdall_audio_equalizer_new_from_preset( unsigned u_index )
{
    libheimdall_equalizer_t *p_equalizer;

    if ( u_index >= NB_PRESETS )
        return NULL;

    p_equalizer = malloc( sizeof( *p_equalizer ) );
    if ( unlikely( p_equalizer == NULL ) )
        return NULL;

    p_equalizer->f_preamp = eqz_preset_10b[ u_index ].f_preamp;

    for ( unsigned i = 0; i < EQZ_BANDS_MAX; i++ )
        p_equalizer->f_amp[ i ] = eqz_preset_10b[ u_index ].f_amp[ i ];

    return p_equalizer;
}

/*****************************************************************************
 * libheimdall_audio_equalizer_release : Release a previously created equalizer
 *****************************************************************************/
void libheimdall_audio_equalizer_release( libheimdall_equalizer_t *p_equalizer )
{
    free( p_equalizer );
}

/*****************************************************************************
 * libheimdall_audio_equalizer_set_preamp : Set the preamp value for an equalizer
 *****************************************************************************/
int libheimdall_audio_equalizer_set_preamp( libheimdall_equalizer_t *p_equalizer, float f_preamp )
{
    if( isnan(f_preamp) )
        return -1;
    if( f_preamp < -20.f )
        f_preamp = -20.f;
    else if( f_preamp > 20.f )
        f_preamp = 20.f;

    p_equalizer->f_preamp = f_preamp;
    return 0;
}

/*****************************************************************************
 * libheimdall_audio_equalizer_get_preamp : Get the preamp value for an equalizer
 *****************************************************************************/
float libheimdall_audio_equalizer_get_preamp( libheimdall_equalizer_t *p_equalizer )
{
    return p_equalizer->f_preamp;
}

/*****************************************************************************
 * libheimdall_audio_equalizer_set_amp_at_index : Set the amplification value for an equalizer band
 *****************************************************************************/
int libheimdall_audio_equalizer_set_amp_at_index( libheimdall_equalizer_t *p_equalizer, float f_amp, unsigned u_band )
{
    if( u_band >= EQZ_BANDS_MAX || isnan(f_amp) )
        return -1;


    if( f_amp < -20.f )
        f_amp = -20.f;
    else if( f_amp > 20.f )
        f_amp = 20.f;

    p_equalizer->f_amp[ u_band ] = f_amp;
    return 0;
}

/*****************************************************************************
 * libheimdall_audio_equalizer_get_amp_at_index : Get the amplification value for an equalizer band
 *****************************************************************************/
float libheimdall_audio_equalizer_get_amp_at_index( libheimdall_equalizer_t *p_equalizer, unsigned u_band )
{
    if ( u_band >= EQZ_BANDS_MAX )
        return nanf("");

    return p_equalizer->f_amp[ u_band ];
}
