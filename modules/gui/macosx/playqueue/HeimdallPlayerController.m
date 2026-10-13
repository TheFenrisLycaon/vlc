/*****************************************************************************
 * HeimdallPlayerController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne # lycaria -dot- org>
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

#import "HeimdallPlayerController.h"
#include "heimdall_player.h"

#import <heimdall_configuration.h>
#import <heimdall_modules.h>
#import <heimdall_url.h>

#import "extensions/NSString+Helpers.h"

#import "library/HeimdallInputItem.h"

#import "main/HeimdallMain.h"

#import "os-integration/HeimdallRemoteControlService.h"
#import "os-integration/iTunes.h"
#import "os-integration/Spotify.h"

#import "playqueue/HeimdallPlayerTitle.h"

#import "windows/video/HeimdallVoutView.h"
#import "windows/video/HeimdallMainVideoViewController.h"
#import "windows/video/HeimdallVideoWindowCommon.h"

NSString *HeimdallPlayerElementaryStreamID = @"HeimdallPlayerElementaryStreamID";
NSString *HeimdallTick = @"HeimdallTick";
NSString *HeimdallPlayerCurrentMediaItemChanged = @"HeimdallPlayerCurrentMediaItemChanged";
NSString *HeimdallPlayerMetadataChangedForCurrentMedia = @"HeimdallPlayerMetadataChangedForCurrentMedia";
NSString *HeimdallPlayerStateChanged = @"HeimdallPlayerStateChanged";
NSString *HeimdallPlayerErrorChanged = @"HeimdallPlayerErrorChanged";
NSString *HeimdallPlayerBufferFill = @"HeimdallPlayerBufferFill";
NSString *HeimdallPlayerBufferChanged = @"HeimdallPlayerBufferChanged";
NSString *HeimdallPlayerRateChanged = @"HeimdallPlayerRateChanged";
NSString *HeimdallPlayerCapabilitiesChanged = @"HeimdallPlayerCapabilitiesChanged";
NSString *HeimdallPlayerTimeAndPositionChanged = @"HeimdallPlayerTimeAndPositionChanged";
NSString *HeimdallPlayerLengthChanged = @"HeimdallPlayerLengthChanged";
NSString *HeimdallPlayerTitleSelectionChanged = @"HeimdallPlayerTitleSelectionChanged";
NSString *HeimdallPlayerTitleListChanged = @"HeimdallPlayerTitleListChanged";
NSString *HeimdallPlayerChapterSelectionChanged = @"HeimdallPlayerChapterSelectionChanged";
NSString *HeimdallPlayerProgramSelectionChanged = @"HeimdallPlayerProgramSelectionChanged";
NSString *HeimdallPlayerProgramListChanged = @"HeimdallPlayerProgramListChanged";
NSString *HeimdallPlayerABLoopStateChanged = @"HeimdallPlayerABLoopStateChanged";
NSString *HeimdallPlayerTeletextMenuAvailable = @"HeimdallPlayerTeletextMenuAvailable";
NSString *HeimdallPlayerTeletextEnabled = @"HeimdallPlayerTeletextEnabled";
NSString *HeimdallPlayerTeletextPageChanged = @"HeimdallPlayerTeletextPageChanged";
NSString *HeimdallPlayerTeletextTransparencyChanged = @"HeimdallPlayerTeletextTransparencyChanged";
NSString *HeimdallPlayerAudioDelayChanged = @"HeimdallPlayerAudioDelayChanged";
NSString *HeimdallPlayerSubtitlesDelayChanged = @"HeimdallPlayerSubtitlesDelayChanged";
NSString *HeimdallPlayerDelayChangedForSpecificElementaryStream = @"HeimdallPlayerDelayChangedForSpecificElementaryStream";
NSString *HeimdallPlayerSubtitlesFPSChanged = @"HeimdallPlayerSubtitlesFPSChanged";
NSString *HeimdallPlayerSubtitleTextScalingFactorChanged = @"HeimdallPlayerSubtitleTextScalingFactorChanged";
NSString *HeimdallPlayerRecordingChanged = @"HeimdallPlayerRecordingChanged";
NSString *HeimdallPlayerRendererChanged = @"HeimdallPlayerRendererChanged";
NSString *HeimdallPlayerInputStats = @"HeimdallPlayerInputStats";
NSString *HeimdallPlayerStatisticsUpdated = @"HeimdallPlayerStatisticsUpdated";
NSString *HeimdallPlayerTrackListChanged = @"HeimdallPlayerTrackListChanged";
NSString *HeimdallPlayerTrackSelectionChanged = @"HeimdallPlayerTrackSelectionChanged";
NSString *HeimdallPlayerFullscreenChanged = @"HeimdallPlayerFullscreenChanged";
NSString *HeimdallPlayerPictureInPictureChanged = @"HeimdallPlayerPictureInPictureChanged";
NSString *HeimdallPlayerWallpaperModeChanged = @"HeimdallPlayerWallpaperModeChanged";
NSString *HeimdallPlayerListOfVideoOutputThreadsChanged = @"HeimdallPlayerListOfVideoOutputThreadsChanged";
NSString *HeimdallPlayerVolumeChanged = @"HeimdallPlayerVolumeChanged";
NSString *HeimdallPlayerMuteChanged = @"HeimdallPlayerMuteChanged";
NSString * const HeimdallPlayerLyricsAvailableChanged = @"HeimdallPlayerLyricsAvailableChanged";
NSString * const HeimdallPlayerShowLyricsChanged = @"HeimdallPlayerShowLyricsChanged";

NSString * const HeimdallPlayerShowLyricsKey = @"HeimdallPlayerShowLyricsKey";

const CGFloat HeimdallVolumeMaximum = 2.;
const CGFloat HeimdallVolumeDefault = 1.;

@interface HeimdallPlayerController ()
{
    heimdall_player_t *_p_player;
    heimdall_player_listener_id *_playerListenerID;
    heimdall_player_timer_id *_playerTimerID;
    heimdall_player_aout_listener_id *_playerAoutListenerID;
    heimdall_player_vout_listener_id *_playerVoutListenerID;
    heimdall_player_title_list *_currentTitleList;
    NSNotificationCenter *_defaultNotificationCenter;

    /* remote control support */
    HeimdallRemoteControlService *_remoteControlService;

    /* iTunes/Apple Music/Spotify play/pause support */
    BOOL _iTunesPlaybackWasPaused;
    BOOL _appleMusicPlaybackWasPaused;
    BOOL _spotifyPlaybackWasPaused;

    NSTimer *_playbackHasTruelyEndedTimer;
    HeimdallInputItem *_currentMedia;

    struct heimdall_player_timer_point _player_time;
}

@property (readwrite, atomic) iTunesApplication *appleMusicApp;
@property (readwrite, atomic) iTunesApplication *iTunesApp;
@property (readwrite, atomic) SpotifyApplication *spotifyApp;
@property (readwrite, atomic) struct heimdall_player_timer_point playerTime;
@property (readwrite, atomic) NSTimer *positionTimer;
@property (readwrite, atomic) NSTimer *timeTimer;

- (int)interpolateTime:(heimdall_tick_t)system_now;
- (void)updatePositionWithTimer:(NSTimer *)timer;
- (void)updatePosition;
- (void)updateTimeWithTimer:(NSTimer *)timer;
- (void)updateTime:(heimdall_tick_t)time forceUpdate:(BOOL)force;
- (void)currentMediaItemChanged:(input_item_t *)newMediaItem;
- (void)stateChanged:(enum heimdall_player_state)state;
- (void)errorChanged:(enum heimdall_player_error)error;
- (void)newBufferingValue:(float)bufferValue;
- (void)newRateValue:(float)rateValue;
- (void)capabilitiesChanged:(int)newCapabilities;
- (void)lengthChanged:(heimdall_tick_t)length;
- (void)titleListChanged:(heimdall_player_title_list *)p_titles;
- (void)selectedTitleChanged:(size_t)selectedTitle;
- (void)selectedChapterChanged:(size_t)chapterIndex;
- (void)teletextAvailibilityChanged:(BOOL)hasTeletextMenu;
- (void)teletextEnabledChanged:(BOOL)teletextOn;
- (void)teletextPageChanged:(unsigned int)page;
- (void)teletextTransparencyChanged:(BOOL)isTransparent;
- (void)audioDelayChanged:(heimdall_tick_t)audioDelay;
- (void)rendererChanged:(heimdall_renderer_item_t *)newRendererItem;
- (void)subtitlesDelayChanged:(heimdall_tick_t)subtitlesDelay;
- (void)delayChanged:(heimdall_tick_t)trackDelay forTrack:(heimdall_es_id_t *)esID;
- (void)subtitlesFPSChanged:(float)subtitlesFPS;
- (void)recordingChanged:(BOOL)recording;
- (void)inputStatsUpdated:(HeimdallInputStats *)inputStats;
- (void)trackSelectionChanged;
- (void)trackListChanged;
- (void)programListChanged;
- (void)programSelectionChanged:(int)selectedID;
- (void)ABLoopStateChanged:(enum heimdall_player_abloop)abLoopState;
- (void)metaDataChangedForInput:(input_item_t *)inputItem;
- (void)voutListUpdated;

/* video */
- (void)fullscreenChanged:(BOOL)isFullscreen;
- (void)wallpaperModeChanged:(BOOL)wallpaperModeValue;

/* audio */
- (void)volumeChanged:(float)volume;
- (void)muteChanged:(BOOL)mute;
@end

#pragma mark - player callback implementations

static void cb_player_current_media_changed(heimdall_player_t *p_player, input_item_t *p_newMediaItem, void *p_data)
{
    HEIMDALL_UNUSED(p_player);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController currentMediaItemChanged:p_newMediaItem];
    });
}

static void cb_player_state_changed(heimdall_player_t *p_player, enum heimdall_player_state state, void *p_data)
{
    HEIMDALL_UNUSED(p_player);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController stateChanged:state];
    });
}

static void cb_player_error_changed(heimdall_player_t *p_player, enum heimdall_player_error error, void *p_data)
{
    HEIMDALL_UNUSED(p_player);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController errorChanged:error];
    });
}

static void cb_player_buffering(heimdall_player_t *p_player, float newBufferValue, void *p_data)
{
    HEIMDALL_UNUSED(p_player);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController newBufferingValue:newBufferValue];
    });
}

static void cb_player_capabilities_changed(heimdall_player_t *p_player, int oldCapabilities, int newCapabilities, void *p_data)
{
    HEIMDALL_UNUSED(p_player); HEIMDALL_UNUSED(oldCapabilities);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController capabilitiesChanged:newCapabilities];
    });
}

static void cb_player_titles_changed(heimdall_player_t *p_player,
                                     heimdall_player_title_list *p_titles,
                                     void *p_data)
{
    HEIMDALL_UNUSED(p_player);
    if (p_titles)
        heimdall_player_title_list_Hold(p_titles);

    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController titleListChanged:p_titles];
    });
}

static void cb_player_title_selection_changed(heimdall_player_t *p_player,
                                              const struct heimdall_player_title *p_new_title,
                                              size_t selectedIndex,
                                              void *p_data)
{
    HEIMDALL_UNUSED(p_player);
    HEIMDALL_UNUSED(p_new_title);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController selectedTitleChanged:selectedIndex];
    });
}

static void cb_player_chapter_selection_changed(heimdall_player_t *p_player,
                                                const struct heimdall_player_title *p_title, size_t title_idx,
                                                const struct heimdall_player_chapter *p_new_chapter, size_t new_chapter_idx,
                                                void *p_data)
{
    HEIMDALL_UNUSED(p_player);
    HEIMDALL_UNUSED(p_title);
    HEIMDALL_UNUSED(title_idx);
    HEIMDALL_UNUSED(p_new_chapter);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController selectedChapterChanged:new_chapter_idx];
    });
}

static void cb_player_teletext_menu_availability_changed(heimdall_player_t *p_player, bool hasTeletextMenu, void *p_data)
{
    HEIMDALL_UNUSED(p_player);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController teletextAvailibilityChanged:hasTeletextMenu];
    });
}

static void cb_player_teletext_enabled_changed(heimdall_player_t *p_player, bool teletextEnabled, void *p_data)
{
    HEIMDALL_UNUSED(p_player);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController teletextEnabledChanged:teletextEnabled];
    });
}

static void cb_player_teletext_page_changed(heimdall_player_t *p_player, unsigned page, void *p_data)
{
    HEIMDALL_UNUSED(p_player);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController teletextPageChanged:page];
    });
}

static void cb_player_teletext_transparency_changed(heimdall_player_t *p_player, bool isTransparent, void *p_data)
{
    HEIMDALL_UNUSED(p_player);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController teletextTransparencyChanged:isTransparent];
    });
}

static void cb_player_category_delay_changed(heimdall_player_t *p_player, enum es_format_category_e cat,
                                             heimdall_tick_t newDelay, void *p_data)
{
    HEIMDALL_UNUSED(p_player);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        switch (cat)
        {
            case AUDIO_ES:
                [playerController audioDelayChanged:newDelay];
                break;
            case SPU_ES:
                [playerController subtitlesDelayChanged:newDelay];
                break;
            default:
                heimdall_assert_unreachable();
        }
    });
}

static void cb_player_associated_subs_fps_changed(heimdall_player_t *p_player, float subs_fps, void *p_data)
{
    HEIMDALL_UNUSED(p_player);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController subtitlesFPSChanged:subs_fps];
    });
}

static void cb_player_renderer_changed(heimdall_player_t *p_player,
                                       heimdall_renderer_item_t *p_new_renderer,
                                       void *p_data)
{
    HEIMDALL_UNUSED(p_player);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController rendererChanged:p_new_renderer];
    });
}

static void cb_player_record_changed(heimdall_player_t *p_player, bool recording, void *p_data)
{
    HEIMDALL_UNUSED(p_player);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController recordingChanged:recording];
    });
}

static void cb_player_stats_changed(heimdall_player_t *p_player,
                                    const struct input_stats_t *p_stats,
                                    void *p_data)
{
    HEIMDALL_UNUSED(p_player);

    /* the provided structure is valid in this context only, so copy all data to our own */
    HeimdallInputStats *inputStats = [[HeimdallInputStats alloc] initWithStatsStructure:p_stats];

    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController inputStatsUpdated:inputStats];
    });
}

static void cb_player_track_list_changed(heimdall_player_t *p_player,
                                         enum heimdall_player_list_action action,
                                         const struct heimdall_player_track *track,
                                         void *p_data)
{
    HEIMDALL_UNUSED(p_player); HEIMDALL_UNUSED(action); HEIMDALL_UNUSED(track);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController trackListChanged];
    });
}

static void cb_player_track_selection_changed(heimdall_player_t *p_player,
                                              heimdall_es_id_t *unselected_id,
                                              heimdall_es_id_t *selected_id,
                                              void *p_data)
{
    HEIMDALL_UNUSED(p_player); HEIMDALL_UNUSED(unselected_id); HEIMDALL_UNUSED(selected_id);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController trackSelectionChanged];
    });
}

static void cb_player_track_delay_changed(heimdall_player_t *p_player,
                                          heimdall_es_id_t *es_id,
                                          heimdall_tick_t delay,
                                          void *p_data)
{
    HEIMDALL_UNUSED(p_player);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController delayChanged:delay forTrack:es_id];
    });
}

static void cb_player_program_list_changed(heimdall_player_t *p_player,
                                           enum heimdall_player_list_action action,
                                           const struct heimdall_player_program *prgm,
                                           void *p_data)
{
    HEIMDALL_UNUSED(p_player); HEIMDALL_UNUSED(action); HEIMDALL_UNUSED(prgm);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController programListChanged];
    });
}

static void cb_player_program_selection_changed(heimdall_player_t *p_player,
                                                int unselected_id,
                                                int selected_id,
                                                void *p_data)
{
    HEIMDALL_UNUSED(p_player); HEIMDALL_UNUSED(unselected_id);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController programSelectionChanged:selected_id];
    });
}

static void cb_player_atobloop_changed(heimdall_player_t * __unused p_player,
                                       enum heimdall_player_abloop new_state,
                                       heimdall_tick_t __unused time,
                                       double __unused pos,
                                       void *p_data)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController ABLoopStateChanged:new_state];
    });
}

static void cb_player_item_meta_changed(heimdall_player_t * __unused p_player,
                                        input_item_t *p_mediaItem,
                                        void *p_data)
{
    input_item_Hold(p_mediaItem);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController metaDataChangedForInput:p_mediaItem];
    });
}

static void cb_player_vout_changed(heimdall_player_t * __unused p_player,
                                   enum heimdall_player_vout_action __unused action,
                                   vout_thread_t * __unused p_vout,
                                   enum heimdall_vout_order __unused order,
                                   heimdall_es_id_t *es_id,
                                   void *p_data)
{
    if (heimdall_es_id_GetCat(es_id) != VIDEO_ES)
        return;

    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController voutListUpdated];
    });
}

static void cb_player_timer_updated(const struct heimdall_player_timer_point * const p_value, void * const p_data)
{
    HeimdallPlayerController * const playerController = (__bridge HeimdallPlayerController *)p_data;
    const struct heimdall_player_timer_point value_copy = *p_value;

    dispatch_async(dispatch_get_main_queue(), ^{
        playerController.playerTime = value_copy;

        BOOL lengthOrRateChanged = NO;
        if (playerController.length != value_copy.length) {
            [playerController lengthChanged:value_copy.length];
            lengthOrRateChanged = YES;
        }
        if (playerController.playbackRate != value_copy.rate) {
            [playerController newRateValue:value_copy.rate];
            lengthOrRateChanged = YES;
        }
        const heimdall_tick_t system_now = heimdall_tick_now();
        if ([playerController interpolateTime:system_now] == HEIMDALL_SUCCESS) {
            if (lengthOrRateChanged || ![playerController.positionTimer isValid]) {
                [playerController updatePosition];

                if (value_copy.system_date != INT64_MAX) {
                    static const heimdall_tick_t POSITION_MIN_UPDATE_INTERVAL = HEIMDALL_TICK_FROM_MS(15);
                    heimdall_tick_t interval = MAX(value_copy.length / value_copy.rate / HEIMDALL_TICK_FROM_MS(1), POSITION_MIN_UPDATE_INTERVAL);
                    if (interval < POSITION_MIN_UPDATE_INTERVAL)
                        interval = POSITION_MIN_UPDATE_INTERVAL;

                    [playerController.positionTimer invalidate];
                    playerController.positionTimer =
                    [NSTimer scheduledTimerWithTimeInterval:SEC_FROM_HEIMDALL_TICK(interval)
                                                     target:playerController
                                                   selector:@selector(updatePositionWithTimer:)
                                                   userInfo:nil
                                                    repeats:YES];
                }
            }
            [playerController updateTime:system_now forceUpdate:lengthOrRateChanged];
        }
    });
}

static void cb_player_timer_paused(const heimdall_tick_t system_date, void * const p_data)
{
    HeimdallPlayerController * const playerController = (__bridge HeimdallPlayerController *)p_data;
    dispatch_async(dispatch_get_main_queue(), ^{
        if (system_date != HEIMDALL_TICK_INVALID && [playerController interpolateTime:system_date] == HEIMDALL_SUCCESS) {
            // The discontinuity event got a valid system date, update the time properties.
            [playerController updatePosition];
            [playerController updateTime:system_date forceUpdate:NO];
        }

        // And stop the timers.
        [playerController.positionTimer invalidate];
        [playerController.timeTimer invalidate];
    });
}

static void cb_player_timer_seeked(const struct heimdall_player_timer_point * const p_value, void * const p_data)
{
    if (p_value == NULL) {
        return; // NULL means seeking just finished; nothing to update here
    }
    HeimdallPlayerController * const playerController = (__bridge HeimdallPlayerController *)p_data;
    const struct heimdall_player_timer_point value_copy = *p_value;
    dispatch_async(dispatch_get_main_queue(), ^{
        playerController.playerTime = value_copy;
        const heimdall_tick_t system_now = heimdall_tick_now();
        if ([playerController interpolateTime:system_now] == HEIMDALL_SUCCESS) {
            [playerController updatePosition];
            [playerController updateTime:system_now forceUpdate:NO];
        }
    });
}

static const struct heimdall_player_cbs player_callbacks = {
    .on_current_media_changed = cb_player_current_media_changed,
    .on_state_changed = cb_player_state_changed,
    .on_error_changed = cb_player_error_changed,
    .on_buffering_changed = cb_player_buffering,
    .on_rate_changed = NULL, // now handled by timer
    .on_capabilities_changed = cb_player_capabilities_changed,
    .on_position_changed = NULL, // now handled by timer
    .on_length_changed = NULL, // now handled by timer
    .on_track_list_changed = cb_player_track_list_changed,
    .on_track_selection_changed = cb_player_track_selection_changed,
    .on_track_delay_changed = cb_player_track_delay_changed,
    .on_program_list_changed = cb_player_program_list_changed,
    .on_program_selection_changed = cb_player_program_selection_changed,
    .on_titles_changed = cb_player_titles_changed,
    .on_title_selection_changed = cb_player_title_selection_changed,
    .on_chapter_selection_changed = cb_player_chapter_selection_changed,
    .on_teletext_menu_changed = cb_player_teletext_menu_availability_changed,
    .on_teletext_enabled_changed = cb_player_teletext_enabled_changed,
    .on_teletext_page_changed = cb_player_teletext_page_changed,
    .on_teletext_transparency_changed = cb_player_teletext_transparency_changed,
    .on_category_delay_changed = cb_player_category_delay_changed,
    .on_associated_subs_fps_changed = cb_player_associated_subs_fps_changed,
    .on_renderer_changed = cb_player_renderer_changed,
    .on_recording_changed = cb_player_record_changed,
    .on_signal_changed = NULL,
    .on_statistics_changed = cb_player_stats_changed,
    .on_atobloop_changed = cb_player_atobloop_changed,
    .on_media_meta_changed = cb_player_item_meta_changed,
    .on_media_epg_changed = NULL,
    .on_media_subitems_changed = NULL,
    .on_vout_changed = cb_player_vout_changed,
    .on_cork_changed = NULL,
};

static const struct heimdall_player_timer_cbs player_timer_callbacks = {
    .on_update = cb_player_timer_updated,
    .on_paused = cb_player_timer_paused,
    .on_seek = cb_player_timer_seeked
};

#pragma mark - video specific callback implementations

static void cb_player_vout_fullscreen_changed(vout_thread_t *p_vout, bool isFullscreen, void *p_data)
{
    HEIMDALL_UNUSED(p_vout);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController fullscreenChanged:isFullscreen];
    });
}

static void cb_player_vout_wallpaper_mode_changed(vout_thread_t *p_vout,  bool wallpaperModeEnabled, void *p_data)
{
    HEIMDALL_UNUSED(p_vout);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController wallpaperModeChanged:wallpaperModeEnabled];
    });
}

static const struct heimdall_player_vout_cbs player_vout_callbacks = {
    cb_player_vout_fullscreen_changed,
    cb_player_vout_wallpaper_mode_changed,
};

#pragma mark - video specific callback implementations

static void cb_player_aout_volume_changed(audio_output_t *aout, float volume, void *p_data)
{
    HEIMDALL_UNUSED(aout);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController volumeChanged:volume];
    });
}

static void cb_player_aout_mute_changed(audio_output_t *aout, bool muted, void *p_data)
{
    HEIMDALL_UNUSED(aout);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
        [playerController muteChanged:muted];
    });
}

static const struct heimdall_player_aout_cbs player_aout_callbacks = {
    cb_player_aout_volume_changed,
    cb_player_aout_mute_changed,
    NULL,
};

static int BossCallback(heimdall_object_t *p_this,
                        const char *psz_var,
                        heimdall_value_t oldval,
                        heimdall_value_t new_val,
                        void *p_data)
{
    HEIMDALL_UNUSED(p_this); HEIMDALL_UNUSED(psz_var); HEIMDALL_UNUSED(oldval); HEIMDALL_UNUSED(new_val);
    @autoreleasepool {
        dispatch_async(dispatch_get_main_queue(), ^{
            HeimdallPlayerController *playerController = (__bridge HeimdallPlayerController *)p_data;
            [playerController pause];
            [NSApplication.sharedApplication hide:nil];
        });
        return HEIMDALL_SUCCESS;
    }
}

#pragma mark - controller initialization

@implementation HeimdallPlayerController

- (instancetype)initWithPlayer:(heimdall_player_t *)player
{
    self = [super init];
    if (self) {
        _defaultNotificationCenter = NSNotificationCenter.defaultCenter;
        [_defaultNotificationCenter addObserver:self
                                       selector:@selector(applicationWillTerminate:)
                                           name:NSApplicationWillTerminateNotification
                                         object:nil];
        [_defaultNotificationCenter addObserver:self
                                       selector:@selector(applicationDidFinishLaunching:)
                                           name:NSApplicationDidFinishLaunchingNotification
                                         object:nil];
        _position = -1.f;
        _time = HEIMDALL_TICK_INVALID;
        _p_player = player;
        heimdall_player_Lock(_p_player);
        // FIXME: initialize state machine here
        _playerListenerID = heimdall_player_AddListener(_p_player,
                               &player_callbacks,
                               (__bridge void *)self);
        _playerTimerID = heimdall_player_AddTimer(_p_player,
                                             HEIMDALL_TICK_FROM_MS(500),
                                             &player_timer_callbacks,
                                             (__bridge void *)self);
        heimdall_player_Unlock(_p_player);
        _playerAoutListenerID = heimdall_player_aout_AddListener(_p_player,
                                                            &player_aout_callbacks,
                                                            (__bridge void *)self);
        _playerVoutListenerID = heimdall_player_vout_AddListener(_p_player,
                                                            &player_vout_callbacks,
                                                            (__bridge void *)self);

        const float aoutVolume = heimdall_player_aout_GetVolume(_p_player);
        _volume = aoutVolume >= 0.f ? aoutVolume : HeimdallVolumeDefault;

        const int aoutMute = heimdall_player_aout_IsMuted(_p_player);
        _mute = aoutMute == -1 ? NO : aoutMute;

        _aLoopTime = -1;
        _bLoopTime = -1;
        _aLoopPosition = -1;
        _bLoopPosition = -1;

        _playbackRate = 1.0;

        _showLyrics = [NSUserDefaults.standardUserDefaults boolForKey:HeimdallPlayerShowLyricsKey];

        libheimdall_int_t *libheimdall = heimdall_object_instance(getIntf());
        var_AddCallback(libheimdall, "intf-boss", BossCallback, (__bridge void *)self);
    }

    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    _remoteControlService = [[HeimdallRemoteControlService alloc] init];
    [_remoteControlService subscribeToRemoteCommands];

    dispatch_async(dispatch_get_global_queue(QOS_CLASS_BACKGROUND, 0), ^{
        if (@available(macOS 10.15, *)) {
            self.appleMusicApp = (iTunesApplication *) [SBApplication applicationWithBundleIdentifier:@"com.apple.Music"];
        } else {
            self.iTunesApp = (iTunesApplication *) [SBApplication applicationWithBundleIdentifier:@"com.apple.iTunes"];
        }

        self.spotifyApp = (SpotifyApplication *) [SBApplication applicationWithBundleIdentifier:@"com.spotify.client"];

        dispatch_async(dispatch_get_main_queue(), ^{
            if (self->_playerState == HEIMDALL_PLAYER_STATE_PLAYING || self->_playerState == HEIMDALL_PLAYER_STATE_STARTED) {
                [self stopOtherAudioPlaybackApps];
            }
        });
    });
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    libheimdall_int_t *libheimdall = heimdall_object_instance(getIntf());
    var_DelCallback(libheimdall, "intf-boss", BossCallback, (__bridge void *)self);

    [self onPlaybackHasTruelyEnded:nil];
    [_remoteControlService unsubscribeFromRemoteCommands];
    if (_currentTitleList) {
        heimdall_player_title_list_Release(_currentTitleList);
    }
    if (_p_player) {
        if (_playerListenerID) {
            heimdall_player_Lock(_p_player);
            heimdall_player_RemoveListener(_p_player, _playerListenerID);
            heimdall_player_RemoveTimer(_p_player, _playerTimerID);
            heimdall_player_Unlock(_p_player);
        }
        if (_playerAoutListenerID) {
            heimdall_player_aout_RemoveListener(_p_player, _playerAoutListenerID);
        }
        if (_playerVoutListenerID) {
            heimdall_player_vout_RemoveListener(_p_player, _playerVoutListenerID);
        }
    }
}

- (void)dealloc
{
    [_defaultNotificationCenter removeObserver:self];
}

- (void)setShowLyrics:(BOOL)showLyrics
{
    if (_showLyrics == showLyrics) {
        return;
    }

    _showLyrics = showLyrics;
    [NSUserDefaults.standardUserDefaults setBool:showLyrics forKey:HeimdallPlayerShowLyricsKey];

    [_defaultNotificationCenter postNotificationName:HeimdallPlayerShowLyricsChanged
                                              object:self];
}

- (HeimdallInputItem *)currentMedia
{
    if (_currentMedia)
        return _currentMedia;
    heimdall_player_Lock(_p_player);
    input_item_t *heimdallInputItem = heimdall_player_GetCurrentMedia(_p_player);
    if (heimdallInputItem) {
        _currentMedia = [[HeimdallInputItem alloc] initWithInputItem:heimdallInputItem];
    }
    heimdall_player_Unlock(_p_player);
    return _currentMedia;
}

- (nullable HeimdallMediaLibraryMediaItem *)currentMediaLibraryItem
{
    NSURL * const url = [NSURL URLWithString:self.currentMedia.MRL];
    return [HeimdallMediaLibraryMediaItem mediaItemForURL:url];
}

- (int)interpolateTime:(heimdall_tick_t)system_now
{
    heimdall_tick_t new_time;
    if (heimdall_player_timer_point_Interpolate(&_playerTime, system_now, &new_time, &_position) == HEIMDALL_SUCCESS) {
        _time = new_time != HEIMDALL_TICK_INVALID ? new_time - HEIMDALL_TICK_0 : 0;
        return HEIMDALL_SUCCESS;
    }
    return HEIMDALL_EGENERIC;
}

#pragma mark - playback control methods

- (int)start
{
    heimdall_player_Lock(_p_player);
    int ret = heimdall_player_Start(_p_player);
    heimdall_player_Unlock(_p_player);
    return ret;
}

- (void)startInPausedState:(BOOL)startPaused
{
    heimdall_player_Lock(_p_player);
    heimdall_player_SetStartPaused(_p_player, startPaused);
    heimdall_player_Unlock(_p_player);
}

- (void)pause
{
    heimdall_player_Lock(_p_player);
    heimdall_player_Pause(_p_player);
    heimdall_player_Unlock(_p_player);
}

- (void)resume
{
    heimdall_player_Lock(_p_player);
    heimdall_player_Resume(_p_player);
    heimdall_player_Unlock(_p_player);
}

- (void)togglePlayPause
{
    heimdall_player_Lock(_p_player);
    if (_playerState == HEIMDALL_PLAYER_STATE_PLAYING) {
        heimdall_player_Pause(_p_player);
    } else if (_playerState == HEIMDALL_PLAYER_STATE_PAUSED) {
        heimdall_player_Resume(_p_player);
    } else
        heimdall_player_Start(_p_player);
    heimdall_player_Unlock(_p_player);
}

- (void)stop
{
    heimdall_player_Lock(_p_player);
    heimdall_player_Stop(_p_player);
    heimdall_player_Unlock(_p_player);
}

- (void)metaDataChangedForInput:(input_item_t *)inputItem
{
    NSString *syltData = nil;
    if (_currentMedia) {
        syltData = [_currentMedia extraMetaForKey:@"sylt-data"];
    }

    const BOOL lyricsAvailable = (syltData && syltData.length > 0);
    if (_lyricsAvailable != lyricsAvailable) {
        _lyricsAvailable = lyricsAvailable;
        [_defaultNotificationCenter postNotificationName:HeimdallPlayerLyricsAvailableChanged
                                                  object:self];
    }

    [_defaultNotificationCenter postNotificationName:HeimdallPlayerMetadataChangedForCurrentMedia
                                              object:self];
}

- (void)nextVideoFrame
{
    heimdall_player_Lock(_p_player);
    heimdall_player_NextVideoFrame(_p_player);
    heimdall_player_Unlock(_p_player);
}

#pragma mark - player callback delegations

- (void)currentMediaItemChanged:(input_item_t *)newMediaItem
{
    _currentMedia = [[HeimdallInputItem alloc] initWithInputItem:newMediaItem];

    NSString *syltData = nil;
    if (_currentMedia) {
        syltData = [_currentMedia extraMetaForKey:@"sylt-data"];
    }

    const BOOL lyricsAvailable = (syltData && syltData.length > 0);
    if (_lyricsAvailable != lyricsAvailable) {
        _lyricsAvailable = lyricsAvailable;
        [_defaultNotificationCenter postNotificationName:HeimdallPlayerLyricsAvailableChanged
                                                  object:self];
    }

    [_defaultNotificationCenter postNotificationName:HeimdallPlayerCurrentMediaItemChanged object:self];
}

- (heimdall_tick_t)durationOfCurrentMediaItem
{
    if (!self.currentMedia) {
        return -1;
    }

    return self.currentMedia.duration;
}

- (NSURL *)URLOfCurrentMediaItem
{
    if (!self.currentMedia) {
        return nil;
    }

    return [NSURL URLWithString:self.currentMedia.MRL];
}

- (NSString*)nameOfCurrentMediaItem
{
    if (!self.currentMedia) {
        return nil;
    }

    NSString *name = self.currentMedia.name;
    if (!name) {
        NSURL *url = [NSURL URLWithString:self.currentMedia.MRL];
        if ([url isFileURL])
            name = [[NSFileManager defaultManager] displayNameAtPath:[url path]];
        else
            name = [url absoluteString];
    }

    return name;
}

- (BOOL)currentMediaIsAudioOnly
{
    NSURL * const currentItemUrl = self.URLOfCurrentMediaItem;
    if (currentItemUrl == nil) {
        return NO;
    }

    if (self.videoTracks.count > 0) {
        return NO;
    }

    HeimdallMediaLibraryMediaItem * const mediaItem =
        [HeimdallMediaLibraryMediaItem mediaItemForURL:currentItemUrl];
    if (mediaItem != nil) {
        return mediaItem.mediaType == HEIMDALL_ML_MEDIA_TYPE_AUDIO;
    }

    return self.audioTracks.count > 0;
}

- (void)stateChanged:(enum heimdall_player_state)state
{
    /* instead of using heimdall_player_GetState, we cache the state and provide it through a synthesized getter
     * as the direct call might not reflect the actual state due the asynchronous API nature */
    _playerState = state;

    /* we seem to start (over), don't start other players */
    if (_playerState != HEIMDALL_PLAYER_STATE_STOPPED) {
        if (_playbackHasTruelyEndedTimer) {
            [_playbackHasTruelyEndedTimer invalidate];
            _playbackHasTruelyEndedTimer = nil;
        }
    }

    [_defaultNotificationCenter postNotificationName:HeimdallPlayerStateChanged
                                              object:self];

    /* notify third party apps through an informal protocol that our state changed */
    [[NSDistributedNotificationCenter defaultCenter] postNotificationName:@"HeimdallPlayerStateDidChange"
                                                                   object:nil
                                                                 userInfo:nil
                                                       deliverImmediately:YES];

    /* schedule a timer to restart iTunes / Spotify because we are done here */
    if (_playerState == HEIMDALL_PLAYER_STATE_STOPPED) {
        if (_playbackHasTruelyEndedTimer) {
            [_playbackHasTruelyEndedTimer invalidate];
        }
        _playbackHasTruelyEndedTimer = [NSTimer scheduledTimerWithTimeInterval:0.5
                                                                        target:self
                                                                      selector:@selector(onPlaybackHasTruelyEnded:)
                                                                      userInfo:nil
                                                                       repeats:NO];
    }

    /* pause external players */
    if (_playerState == HEIMDALL_PLAYER_STATE_PLAYING || _playerState == HEIMDALL_PLAYER_STATE_STARTED) {
        [self stopOtherAudioPlaybackApps];
    }
}

// Called when playback has truly ended and likely no subsequent media will start playing
- (void)onPlaybackHasTruelyEnded:(id)sender
{
    msg_Dbg(getIntf(), "Playback has been ended");

    [self resumeOtherAudioPlaybackApps];
    _playbackHasTruelyEndedTimer = nil;
    _currentMedia = nil;
}

- (void)stopOtherAudioPlaybackApps
{
    intf_thread_t *p_intf = getIntf();
    int64_t controlOtherPlayers = var_InheritInteger(p_intf, "macosx-control-itunes");
    if (controlOtherPlayers <= 0)
        return;

    // Don't bother looking for Apple Music on macOS below 10.15, and conversely
    // don't bother looking for iTunes on macOS above 10.15
    if (@available(macOS 10.15, *)) {
        // Pause Apple Music playback
        iTunesApplication *appleMusic = self.appleMusicApp;
        if (appleMusic != nil &&
            !_appleMusicPlaybackWasPaused &&
            [appleMusic isRunning] &&
            [appleMusic playerState] == iTunesEPlSPlaying) {

            msg_Dbg(p_intf, "pausing Apple Music");
            [appleMusic pause];
            _appleMusicPlaybackWasPaused = YES;
        }
    } else {
        iTunesApplication *iTunes = self.iTunesApp;
        if (iTunes != nil &&
            !_iTunesPlaybackWasPaused &&
            [iTunes isRunning] &&
            [iTunes playerState] == iTunesEPlSPlaying) {
            // Pause iTunes playback
            msg_Dbg(p_intf, "pausing iTunes");
            [iTunes pause];
            _iTunesPlaybackWasPaused = YES;
        }
    }

    // Pause Spotify
    SpotifyApplication *spotify = self.spotifyApp;
    if (spotify != nil &&
        !_spotifyPlaybackWasPaused &&
        [spotify respondsToSelector:@selector(isRunning)] &&
        [spotify respondsToSelector:@selector(playerState)] &&
        [spotify isRunning] &&
        [spotify playerState] == kSpotifyPlayerStatePlaying) {

        msg_Dbg(p_intf, "pausing Spotify");
        [spotify pause];
        _spotifyPlaybackWasPaused = YES;
    }
}

- (void)resumeOtherAudioPlaybackApps
{
    intf_thread_t *p_intf = getIntf();
    if (var_InheritInteger(p_intf, "macosx-control-itunes") > 1) {
        if (@available(macOS 10.15, *)) {
            iTunesApplication *appleMusic = self.appleMusicApp;
            if (appleMusic != nil &&
                _appleMusicPlaybackWasPaused &&
                [appleMusic isRunning] &&
                [appleMusic playerState] == iTunesEPlSPaused) {

                msg_Dbg(p_intf, "unpausing Apple Music");
                [appleMusic playpause];
            }
        } else {
            iTunesApplication *iTunes = self.iTunesApp;
            if (iTunes != nil &&
                _iTunesPlaybackWasPaused &&
                [iTunes isRunning] &&
                [iTunes playerState] == iTunesEPlSPaused) {

                msg_Dbg(p_intf, "unpausing iTunes");
                [iTunes playpause];
            }
        }

        SpotifyApplication *spotify = self.spotifyApp;
        if (spotify != nil &&
            _spotifyPlaybackWasPaused &&
            [spotify respondsToSelector:@selector(isRunning)] &&
            [spotify respondsToSelector:@selector(playerState)] &&
            [spotify isRunning] &&
            [spotify playerState] == kSpotifyPlayerStatePaused) {

            msg_Dbg(p_intf, "unpausing Spotify");
            [spotify play];
        }
    }

    _iTunesPlaybackWasPaused = NO;
    _appleMusicPlaybackWasPaused = NO;
    _spotifyPlaybackWasPaused = NO;
}

- (void)errorChanged:(enum heimdall_player_error)error
{
    /* instead of using heimdall_player_GetError, we cache the error and provide it through a synthesized getter
     * as the direct call might not reflect the actual error due the asynchronous API nature */
    _error = error;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerErrorChanged
                                              object:self];
}

- (void)newBufferingValue:(float)bufferValue
{
    _bufferFill = bufferValue;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerBufferChanged
                                              object:self
                                            userInfo:@{HeimdallPlayerBufferFill : @(bufferValue)}];
}

- (void)newRateValue:(float)rateValue
{
    _playbackRate = rateValue;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerRateChanged
                                              object:self];
}

- (void)setPlaybackRate:(float)playbackRate
{
    // Set the ivar first and then we will receive whatever has been applied via newRateValue
    [self newRateValue:playbackRate];

    heimdall_player_Lock(_p_player);
    heimdall_player_ChangeRate(_p_player, playbackRate);
    heimdall_player_Unlock(_p_player);
}

- (void)incrementPlaybackRate
{
    heimdall_player_Lock(_p_player);
    heimdall_player_IncrementRate(_p_player);
    heimdall_player_Unlock(_p_player);
}

- (void)decrementPlaybackRate
{
    heimdall_player_Lock(_p_player);
    heimdall_player_DecrementRate(_p_player);
    heimdall_player_Unlock(_p_player);
}

- (void)capabilitiesChanged:(int)newCapabilities
{
    _seekable = newCapabilities & HEIMDALL_PLAYER_CAP_SEEK ? YES : NO;
    _rewindable = newCapabilities & HEIMDALL_PLAYER_CAP_REWIND ? YES : NO;
    _pausable = newCapabilities & HEIMDALL_PLAYER_CAP_PAUSE ? YES : NO;
    _recordable = YES;
    _rateChangable = newCapabilities & HEIMDALL_PLAYER_CAP_CHANGE_RATE ? YES : NO;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerCapabilitiesChanged
                                              object:self];
}

- (void)updatePositionWithTimer:(NSTimer *)timer
{
    heimdall_tick_t system_now = heimdall_tick_now();
    if ([self interpolateTime:system_now] == HEIMDALL_SUCCESS) {
        [self updatePosition];
    }
}

- (void)updatePosition
{
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerTimeAndPositionChanged
                                              object:self];
}

- (void)updateTimeWithTimer:(NSTimer *)timer
{
    const heimdall_tick_t system_now = heimdall_tick_now();
    if ([self interpolateTime:system_now] == HEIMDALL_SUCCESS) {
        [self updateTime:system_now forceUpdate:NO];
    }
}

- (void)updateTime:(heimdall_tick_t)systemNow forceUpdate:(BOOL)forceUpdate
{
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerTimeAndPositionChanged
                                              object:self];

    if (self.playerTime.system_date != INT64_MAX && (forceUpdate || !self.timeTimer.isValid)) {
        const heimdall_tick_t nextUpdateDate =
            heimdall_player_timer_point_GetNextIntervalDate(&_playerTime, systemNow, _time, HEIMDALL_TICK_FROM_SEC(1));
        const heimdall_tick_t nextUpdateInterval = nextUpdateDate - systemNow;

        if (nextUpdateInterval > 0) {
            // The timer can be triggered a little before. In that case, it's
            // likely that we didn't reach the next next second. It's better to
            // add a very small delay in order to be triggered after the next
            // seconds.
            static const double imprecisionDelaySec = 0.03;

            [self.timeTimer invalidate];
            self.timeTimer = [NSTimer timerWithTimeInterval:SEC_FROM_HEIMDALL_TICK(nextUpdateInterval) + imprecisionDelaySec
                                                     target:self
                                                   selector:@selector(updateTimeWithTimer:)
                                                   userInfo:nil
                                                    repeats:NO];
        }
    }
}

- (void)setTimeFast:(heimdall_tick_t)time
{
    heimdall_player_Lock(_p_player);
    heimdall_player_SeekByTime(_p_player, time, HEIMDALL_PLAYER_SEEK_FAST, HEIMDALL_PLAYER_WHENCE_ABSOLUTE);
    heimdall_player_Unlock(_p_player);
}

- (void)setTimePrecise:(heimdall_tick_t)time
{
    heimdall_player_Lock(_p_player);
    heimdall_player_SeekByTime(_p_player, time, HEIMDALL_PLAYER_SEEK_PRECISE, HEIMDALL_PLAYER_WHENCE_ABSOLUTE);
    heimdall_player_Unlock(_p_player);
}

- (void)setPositionFast:(float)position
{
    heimdall_player_Lock(_p_player);
    heimdall_player_SeekByPos(_p_player, position, HEIMDALL_PLAYER_SEEK_FAST, HEIMDALL_PLAYER_WHENCE_ABSOLUTE);
    heimdall_player_Unlock(_p_player);
}

- (void)setPositionPrecise:(float)position
{
    heimdall_player_Lock(_p_player);
    heimdall_player_SeekByPos(_p_player, position, HEIMDALL_PLAYER_SEEK_PRECISE, HEIMDALL_PLAYER_WHENCE_ABSOLUTE);
    heimdall_player_Unlock(_p_player);
}

- (void)displayPosition
{
    heimdall_player_Lock(_p_player);
    heimdall_player_DisplayPosition(_p_player);
    heimdall_player_Unlock(_p_player);
}

- (void)jumpWithValue:(char *)p_userDefinedJumpSize forward:(BOOL)shallJumpForward
{
    int64_t interval = var_InheritInteger(getIntf(), p_userDefinedJumpSize);
    if (interval <= 0)
        return;

    heimdall_tick_t jumptime = heimdall_tick_from_sec(interval);
    if (!shallJumpForward)
        jumptime = jumptime * -1;

    heimdall_player_Lock(_p_player);
    /* No fask seek for jumps. Indeed, jumps can seek to the current position
     * if not precise enough or if the jump value is too small. */
    heimdall_player_SeekByTime(_p_player,
                          jumptime,
                          HEIMDALL_PLAYER_SEEK_PRECISE,
                          HEIMDALL_PLAYER_WHENCE_RELATIVE);
    heimdall_player_Unlock(_p_player);
}

- (void)jumpForwardExtraShort
{
    [self jumpWithValue:"extrashort-jump-size" forward:YES];
}

- (void)jumpBackwardExtraShort
{
    [self jumpWithValue:"extrashort-jump-size" forward:NO];
}

- (void)jumpForwardShort
{
    [self jumpWithValue:"short-jump-size" forward:YES];
}

- (void)jumpBackwardShort
{
    [self jumpWithValue:"short-jump-size" forward:NO];
}

- (void)jumpForwardMedium
{
    [self jumpWithValue:"medium-jump-size" forward:YES];
}

- (void)jumpBackwardMedium
{
    [self jumpWithValue:"medium-jump-size" forward:NO];
}

- (void)jumpForwardLong
{
    [self jumpWithValue:"long-jump-size" forward:YES];
}

- (void)jumpBackwardLong
{
    [self jumpWithValue:"long-jump-size" forward:NO];
}

- (void)lengthChanged:(heimdall_tick_t)length
{
    _length = length;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerLengthChanged
                                              object:self];
}

- (void)titleListChanged:(heimdall_player_title_list *)p_titles
{
    if (_currentTitleList) {
        heimdall_player_title_list_Release(_currentTitleList);
    }
    /* the new list was already hold earlier */
    _currentTitleList = p_titles;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerTitleListChanged
                                              object:self];
}

- (void)selectedTitleChanged:(size_t)selectedTitle
{
    _selectedTitleIndex = selectedTitle;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerTitleSelectionChanged
                                              object:self];
}

- (const struct heimdall_player_title *)selectedTitle
{
    if (_selectedTitleIndex >= 0 && _selectedTitleIndex < [self numberOfTitlesOfCurrentMedia]) {
        return heimdall_player_title_list_GetAt(_currentTitleList, _selectedTitleIndex);
    }
    return NULL;
}

- (void)setSelectedTitleIndex:(size_t)selectedTitleIndex
{
    // Set the ivar first and then we will receive whatever has been selected via selectedTitleChanged
    [self selectedTitleChanged:selectedTitleIndex];

    heimdall_player_Lock(_p_player);
    heimdall_player_SelectTitleIdx(_p_player, selectedTitleIndex);
    heimdall_player_Unlock(_p_player);
}

- (const struct heimdall_player_title *)titleAtIndexForCurrentMedia:(size_t)index
{
    return heimdall_player_title_list_GetAt(_currentTitleList, index);
}

- (size_t)numberOfTitlesOfCurrentMedia
{
    if (!_currentTitleList) {
        return 0;
    }
    return heimdall_player_title_list_GetCount(_currentTitleList);
}

- (NSArray<HeimdallPlayerTitle *> *)titlesOfCurrentMedia
{
    const size_t count = [self numberOfTitlesOfCurrentMedia];
    NSMutableArray<HeimdallPlayerTitle *> * const titles = NSMutableArray.array;
    for (size_t i = 0; i < count; i++) {
        const struct heimdall_player_title * const p_title = [self titleAtIndexForCurrentMedia:i];
        HeimdallPlayerTitle * const title = [[HeimdallPlayerTitle alloc] initWithTitle:p_title atIndex:i];
        [titles addObject:title];
    }
    return titles.copy;
}

- (void)selectedChapterChanged:(size_t)chapterIndex
{
    _selectedChapterIndex = chapterIndex;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerChapterSelectionChanged
                                              object:self];
}

- (void)setSelectedChapterIndex:(size_t)selectedChapterIndex
{
    // Set the ivar first and then we will receive whatever has been selected via selectedChapterChanged
    [self selectedChapterChanged:selectedChapterIndex];

    heimdall_player_Lock(_p_player);
    heimdall_player_SelectChapterIdx(_p_player, selectedChapterIndex);
    heimdall_player_Unlock(_p_player);
}

- (void)selectNextChapter
{
    heimdall_player_Lock(_p_player);
    heimdall_player_SelectNextChapter(_p_player);
    heimdall_player_Unlock(_p_player);
}

- (void)selectPreviousChapter
{
    heimdall_player_Lock(_p_player);
    heimdall_player_SelectPrevChapter(_p_player);
    heimdall_player_Unlock(_p_player);
}

- (size_t)numberOfChaptersForCurrentTitle
{
    const struct heimdall_player_title *p_current_title = [self selectedTitle];
    if (p_current_title == NULL) {
        return 0;
    }

    return p_current_title->chapter_count;
}

- (const struct heimdall_player_chapter *)chapterAtIndexForCurrentTitle:(size_t)index
{
    const struct heimdall_player_title *p_current_title = [self selectedTitle];
    if (p_current_title == NULL || !p_current_title->chapter_count) {
        return NULL;
    }

    return &p_current_title->chapters[index];
}

- (void)teletextAvailibilityChanged:(BOOL)hasTeletextMenu
{
    _teletextMenuAvailable = hasTeletextMenu;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerTeletextTransparencyChanged
                                              object:self];
}

- (void)teletextEnabledChanged:(BOOL)teletextOn
{
    _teletextEnabled = teletextOn;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerTeletextEnabled
                                              object:self];
}

- (void)setTeletextEnabled:(BOOL)teletextEnabled
{
    // Set the ivar first and then we will receive whatever has been enabled via teletextEnabledChanged
    [self teletextEnabledChanged:teletextEnabled];

    heimdall_player_Lock(_p_player);
    heimdall_player_SetTeletextEnabled(_p_player, teletextEnabled);
    heimdall_player_Unlock(_p_player);
}

- (void)teletextPageChanged:(unsigned int)page
{
    _teletextPage = page;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerTeletextPageChanged
                                              object:self];
}

- (void)setTeletextPage:(unsigned int)teletextPage
{
    // Set the ivar first and then we will receive whatever has been selected via teletextPageChanged
    [self teletextPageChanged:teletextPage];

    heimdall_player_Lock(_p_player);
    heimdall_player_SelectTeletextPage(_p_player, teletextPage);
    heimdall_player_Unlock(_p_player);
}

- (void)teletextTransparencyChanged:(BOOL)isTransparent
{
    _teletextTransparent = isTransparent;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerTeletextTransparencyChanged
                                              object:self];
}

- (void)setTeletextTransparent:(BOOL)teletextTransparent
{
    // Set the ivar first and then we will receive whatever has been enabled via teletextTransparencyChanged
    [self teletextTransparencyChanged:teletextTransparent];

    heimdall_player_Lock(_p_player);
    heimdall_player_SetTeletextTransparency(_p_player, teletextTransparent);
    heimdall_player_Unlock(_p_player);
}

- (void)audioDelayChanged:(heimdall_tick_t)audioDelay
{
    _audioDelay = audioDelay;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerAudioDelayChanged
                                              object:self];
}

- (void)setAudioDelay:(heimdall_tick_t)audioDelay
{
    // Set the ivar first and then we will receive whatever has been enabled via audioDelayChanged
    [self audioDelayChanged:audioDelay];

    heimdall_player_Lock(_p_player);
    heimdall_player_SetAudioDelay(_p_player, audioDelay, HEIMDALL_PLAYER_WHENCE_ABSOLUTE);
    heimdall_player_Unlock(_p_player);
}

- (void)subtitlesDelayChanged:(heimdall_tick_t)subtitlesDelay
{
    _subtitlesDelay = subtitlesDelay;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerSubtitlesDelayChanged
                                              object:self];
}

- (void)setSubtitlesDelay:(heimdall_tick_t)subtitlesDelay
{
    // Set the ivar first and then we will receive whatever has been enabled via subtitlesDelayChanged
    [self subtitlesDelayChanged:subtitlesDelay];

    heimdall_player_Lock(_p_player);
    heimdall_player_SetSubtitleDelay(_p_player, subtitlesDelay, HEIMDALL_PLAYER_WHENCE_ABSOLUTE);
    heimdall_player_Unlock(_p_player);
}

- (void)delayChanged:(heimdall_tick_t)trackDelay forTrack:(heimdall_es_id_t *)esID
{
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerDelayChangedForSpecificElementaryStream
                                              object:self
                                            userInfo:@{ HeimdallPlayerElementaryStreamID : [NSValue valueWithPointer:esID],
                                                        HeimdallTick : [NSNumber numberWithLongLong:trackDelay] }];

}

- (heimdall_tick_t)delayForElementaryStreamID:(heimdall_es_id_t *)esID
{
    heimdall_player_Lock(_p_player);
    heimdall_tick_t delay = heimdall_player_GetEsIdDelay(_p_player, esID);
    heimdall_player_Unlock(_p_player);
    return delay;
}

- (int)setDelay:(heimdall_tick_t)delay forElementaryStreamID:(heimdall_es_id_t *)esID relativeWhence:(BOOL)relative
{
    heimdall_player_Lock(_p_player);
    int returnValue = heimdall_player_SetEsIdDelay(_p_player, esID, delay, relative ? HEIMDALL_PLAYER_WHENCE_RELATIVE : HEIMDALL_PLAYER_WHENCE_ABSOLUTE);
    heimdall_player_Unlock(_p_player);
    return returnValue;
}

- (void)subtitlesFPSChanged:(float)subtitlesFPS
{
    _subtitlesFPS = subtitlesFPS;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerSubtitlesFPSChanged
                                              object:self];
}

- (void)setSubtitlesFPS:(float)subtitlesFPS
{
    // Set the ivar first and then we will receive whatever has been enabled via subtitlesFPSChanged
    [self subtitlesFPSChanged:subtitlesFPS];

    heimdall_player_Lock(_p_player);
    heimdall_player_SetAssociatedSubsFPS(_p_player, subtitlesFPS);
    heimdall_player_Unlock(_p_player);
}

- (unsigned int)subtitleTextScalingFactor
{
    unsigned int ret = 100;
    heimdall_player_Lock(_p_player);
    ret = heimdall_player_GetSubtitleTextScale(_p_player);
    heimdall_player_Unlock(_p_player);
    return ret;
}

- (void)setSubtitleTextScalingFactor:(unsigned int)subtitleTextScalingFactor
{
    if (subtitleTextScalingFactor < 10)
        subtitleTextScalingFactor = 10;
    if (subtitleTextScalingFactor > 500)
        subtitleTextScalingFactor = 500;

    heimdall_player_Lock(_p_player);
    heimdall_player_SetSubtitleTextScale(_p_player, subtitleTextScalingFactor);
    heimdall_player_Unlock(_p_player);
}

- (int)addAssociatedMediaToCurrentFromURL:(NSURL *)URL
                               ofCategory:(enum es_format_category_e)category
                         shallSelectTrack:(BOOL)selectTrack
                          shallDisplayOSD:(BOOL)showOSD
                     shallVerifyExtension:(BOOL)verifyExtension
{
    int ret;
    heimdall_player_Lock(_p_player);
    ret = heimdall_player_AddAssociatedMedia(_p_player, category, [[URL absoluteString] UTF8String], selectTrack, showOSD, verifyExtension);
    heimdall_player_Unlock(_p_player);
    return ret;
}

- (void)rendererChanged:(heimdall_renderer_item_t *)newRenderer
{
    _rendererItem = newRenderer;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerRendererChanged
                                              object:self];
}

- (void)setRendererItem:(heimdall_renderer_item_t *)rendererItem
{
    // Set the ivar first and then we will receive whatever has been enabled via rendererChanged
    [self rendererChanged:rendererItem];

    heimdall_player_Lock(_p_player);
    heimdall_player_SetRenderer(_p_player, rendererItem);
    heimdall_player_Unlock(_p_player);
}

- (void)navigateInInteractiveContent:(enum heimdall_player_nav)navigationAction
{
    heimdall_player_Lock(_p_player);
    heimdall_player_Navigate(_p_player, navigationAction);
    heimdall_player_Unlock(_p_player);
}

- (void)recordingChanged:(BOOL)recording
{
    _enableRecording = recording;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerRecordingChanged
                                              object:self];
}

- (void)inputStatsUpdated:(HeimdallInputStats *)inputStats
{
    _statistics = inputStats;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerStatisticsUpdated
                                              object:self
                                            userInfo:@{HeimdallPlayerInputStats : inputStats}];
}

#pragma mark - track selection
- (void)trackSelectionChanged
{
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerTrackSelectionChanged object:self];
}

- (void)trackListChanged
{
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerTrackListChanged object:self];
}

- (void)selectTrack:(HeimdallTrackMetaData *)track exclusively:(BOOL)exclusiveSelection
{
    heimdall_player_Lock(_p_player);
    const enum es_format_category_e formatCategory = heimdall_es_id_GetCat(track.esID);
    heimdall_player_SelectEsId(_p_player, track.esID, (formatCategory == AUDIO_ES || exclusiveSelection) ? HEIMDALL_PLAYER_SELECT_EXCLUSIVE : HEIMDALL_PLAYER_SELECT_SIMULTANEOUS);
    heimdall_player_Unlock(_p_player);
}

- (void)unselectTrack:(HeimdallTrackMetaData *)track
{
    heimdall_player_Lock(_p_player);
    heimdall_player_UnselectEsId(_p_player, track.esID);
    heimdall_player_Unlock(_p_player);
}

- (void)unselectTracksFromCategory:(enum es_format_category_e)category
{
    heimdall_player_Lock(_p_player);
    heimdall_player_UnselectTrackCategory(_p_player, category);
    heimdall_player_Unlock(_p_player);
}

- (void)selectPreviousTrackForCategory:(enum es_format_category_e)category
{
    heimdall_player_Lock(_p_player);
    heimdall_player_SelectPrevTrack(_p_player, category, HEIMDALL_VOUT_ORDER_PRIMARY);
    heimdall_player_Unlock(_p_player);
}

- (void)selectNextTrackForCategory:(enum es_format_category_e)category
{
    heimdall_player_Lock(_p_player);
    heimdall_player_SelectNextTrack(_p_player, category, HEIMDALL_VOUT_ORDER_PRIMARY);
    heimdall_player_Unlock(_p_player);
}

- (NSArray<HeimdallTrackMetaData *> *)tracksForCategory:(enum es_format_category_e)category
{
    size_t numberOfTracks = 0;
    NSMutableArray *tracks;

    heimdall_player_Lock(_p_player);
    numberOfTracks = heimdall_player_GetTrackCount(_p_player, category);
    if (numberOfTracks == 0) {
        heimdall_player_Unlock(_p_player);
        return nil;
    }

    tracks = [[NSMutableArray alloc] initWithCapacity:numberOfTracks];
    for (size_t x = 0; x < numberOfTracks; x++) {
        const struct heimdall_player_track *p_track = heimdall_player_GetTrackAt(_p_player, category, x);
        HeimdallTrackMetaData *trackMetadata = [[HeimdallTrackMetaData alloc] initWithTrackStructure:p_track];
        [tracks addObject:trackMetadata];
    }
    heimdall_player_Unlock(_p_player);

    return [tracks copy];
}

- (NSArray<HeimdallTrackMetaData *> *)audioTracks
{
    return [self tracksForCategory:AUDIO_ES];
}

- (NSArray<HeimdallTrackMetaData *> *)videoTracks
{
    return [self tracksForCategory:VIDEO_ES];
}

- (NSArray<HeimdallTrackMetaData *> *)subtitleTracks
{
    return [self tracksForCategory:SPU_ES];
}

- (HeimdallTrackMetaData *)selectedTrackMetadataOfCategory:(enum es_format_category_e)category
{
    heimdall_player_Lock(_p_player);
    const struct heimdall_player_track * const p_track =
        heimdall_player_GetSelectedTrack(_p_player, category);
    heimdall_player_Unlock(_p_player);
    return [[HeimdallTrackMetaData alloc] initWithTrackStructure:p_track];
}

- (BOOL)videoTracksEnabled
{
    heimdall_player_Lock(_p_player);
    const BOOL enabled = heimdall_player_IsVideoEnabled(_p_player);
    heimdall_player_Unlock(_p_player);
    return enabled;
}

- (HeimdallTrackMetaData *)selectedVideoTrack
{
    return [self selectedTrackMetadataOfCategory:VIDEO_ES];
}

- (BOOL)audioTracksEnabled
{
    heimdall_player_Lock(_p_player);
    const BOOL enabled = heimdall_player_IsAudioEnabled(_p_player);
    heimdall_player_Unlock(_p_player);
    return enabled;
}

- (HeimdallTrackMetaData *)selectedAudioTrack
{
    return [self selectedTrackMetadataOfCategory:AUDIO_ES];
}

- (void)programListChanged
{
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerProgramListChanged
                                              object:self];
}

- (void)programSelectionChanged:(int)selectedID
{
    _selectedProgramID = selectedID;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerProgramSelectionChanged
                                              object:self];
}

- (void)selectProgram:(HeimdallProgramMetaData *)program
{
    heimdall_player_Lock(_p_player);
    heimdall_player_SelectProgram(_p_player, program.group_id);
    heimdall_player_Unlock(_p_player);
}

- (size_t)numberOfPrograms
{
    size_t ret = 0;
    heimdall_player_Lock(_p_player);
    ret = heimdall_player_GetProgramCount(_p_player);
    heimdall_player_Unlock(_p_player);
    return ret;
}

- (nullable HeimdallProgramMetaData *)programAtIndex:(size_t)index
{
    HeimdallProgramMetaData *programMetaData = nil;
    heimdall_player_Lock(_p_player);
    const struct heimdall_player_program *p_program = heimdall_player_GetProgramAt(_p_player, index);
    if (p_program != NULL) {
        programMetaData = [[HeimdallProgramMetaData alloc] initWithProgramStructure:p_program];
    }
    heimdall_player_Unlock(_p_player);
    return programMetaData;
}

- (nullable HeimdallProgramMetaData *)programForID:(int)programID
{
    HeimdallProgramMetaData *programMetaData = nil;
    heimdall_player_Lock(_p_player);
    const struct heimdall_player_program *p_program = heimdall_player_GetProgram(_p_player, programID);
    if (p_program != NULL) {
        programMetaData = [[HeimdallProgramMetaData alloc] initWithProgramStructure:p_program];
    }
    heimdall_player_Unlock(_p_player);
    return programMetaData;
}

- (void)ABLoopStateChanged:(enum heimdall_player_abloop)abLoopState
{
    _abLoopState = abLoopState;

    heimdall_tick_t a_time = -1;
    heimdall_tick_t b_time = -1;
    double a_pos = -1;
    double b_pos = -1;
    
    heimdall_player_Lock(_p_player);
    heimdall_player_GetAtoBLoop(_p_player, &a_time, &a_pos, &b_time, &b_pos);
    heimdall_player_Unlock(_p_player);

    _aLoopTime = a_time;
    _bLoopTime = b_time;
    _aLoopPosition = a_pos;
    _bLoopPosition = b_pos;

    [_defaultNotificationCenter postNotificationName:HeimdallPlayerABLoopStateChanged
                                              object:self];
}

- (int)setABLoop
{
    int ret = 0;

    heimdall_player_Lock(_p_player);
    switch (_abLoopState) {
        case HEIMDALL_PLAYER_ABLOOP_A:
            ret = heimdall_player_SetAtoBLoop(_p_player, HEIMDALL_PLAYER_ABLOOP_B);
            break;

        case HEIMDALL_PLAYER_ABLOOP_B:
            ret = heimdall_player_SetAtoBLoop(_p_player, HEIMDALL_PLAYER_ABLOOP_NONE);
            break;

        default:
            ret = heimdall_player_SetAtoBLoop(_p_player, HEIMDALL_PLAYER_ABLOOP_A);
            break;
    }
    heimdall_player_Unlock(_p_player);

    return ret;
}

- (int)disableABLoop
{
    heimdall_player_Lock(_p_player);
    int ret = heimdall_player_SetAtoBLoop(_p_player, HEIMDALL_PLAYER_ABLOOP_NONE);
    heimdall_player_Unlock(_p_player);
    return ret;
}

- (void)setEnableRecording:(BOOL)enableRecording
{
    // Set the ivar first and then we will receive whatever has been enabled via recordingChanged
    [self recordingChanged:enableRecording];

    heimdall_player_Lock(_p_player);
    heimdall_player_SetRecordingEnabled(_p_player, enableRecording, NULL);
    heimdall_player_Unlock(_p_player);
}

- (void)toggleRecord
{
    heimdall_player_Lock(_p_player);
    heimdall_player_SetRecordingEnabled(_p_player, !_enableRecording, NULL);
    heimdall_player_Unlock(_p_player);
}

#pragma mark - video specific delegation

- (void)fullscreenChanged:(BOOL)isFullscreen
{
    _fullscreen = isFullscreen;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerFullscreenChanged
                                              object:self];
}

- (void)setFullscreen:(BOOL)fullscreen
{
    // Set the ivar first and then we will receive whatever has been enabled via fullscreenChanged
    [self fullscreenChanged:fullscreen];
    heimdall_player_vout_SetFullscreen(_p_player, fullscreen);
}

- (void)toggleFullscreen
{
    heimdall_player_vout_SetFullscreen(_p_player, !_fullscreen);
}

- (void)togglePictureInPicture
{
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerPictureInPictureChanged
                                              object:self];
}

- (void)wallpaperModeChanged:(BOOL)wallpaperModeValue
{
    _wallpaperMode = wallpaperModeValue;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerWallpaperModeChanged
                                              object:self];
}

- (void)setWallpaperMode:(BOOL)wallpaperMode
{
    // Set the ivar first and then we will receive whatever has been enabled via wallpaperModeChanged
    [self wallpaperModeChanged:wallpaperMode];
    heimdall_player_vout_SetWallpaperModeEnabled(_p_player, wallpaperMode);
}

- (void)takeSnapshot
{
    heimdall_player_vout_Snapshot(_p_player);
}

- (void)voutListUpdated
{
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerListOfVideoOutputThreadsChanged
                                              object:self];
}

- (vout_thread_t *)mainVideoOutputThread
{
    return heimdall_player_vout_Hold(_p_player);
}

- (vout_thread_t *)videoOutputThreadForKeyWindow
{
    vout_thread_t *p_vout = nil;

    id currentWindow = [NSApp keyWindow];
    if ([currentWindow respondsToSelector:@selector(videoView)]) {
        HeimdallVideoWindowCommon *videoWindow = (HeimdallVideoWindowCommon *)currentWindow;
        HeimdallVoutView *videoView = videoWindow.videoViewController.voutView;
        if (videoView) {
            p_vout = [videoView voutThread];
        }
    }

    if (!p_vout)
        p_vout = [self mainVideoOutputThread];

    return p_vout;
}

- (NSArray<NSValue *> *)allVideoOutputThreads
{
    size_t numberOfVoutThreads = 0;
    vout_thread_t **pp_vouts = heimdall_player_vout_HoldAll(_p_player, &numberOfVoutThreads);
    if (numberOfVoutThreads == 0) {
        return nil;
    }

    NSMutableArray<NSValue *> *vouts = [NSMutableArray arrayWithCapacity:numberOfVoutThreads];

    for (size_t i = 0; i < numberOfVoutThreads; ++i)
    {
        assert(pp_vouts[i]);
        [vouts addObject:[NSValue valueWithPointer:pp_vouts[i]]];
    }

    free(pp_vouts);
    return vouts;
}

- (void)displayOSDMessage:(NSString *)message
{
    heimdall_player_osd_Message(_p_player, [message UTF8String]);
}

- (void)setAspectRatioIsLocked:(BOOL)b_value
{
    config_PutInt("macosx-lock-aspect-ratio", b_value);
}

- (BOOL)aspectRatioIsLocked
{
    return config_GetInt("macosx-lock-aspect-ratio");
}

#pragma mark - audio specific delegation

- (void)volumeChanged:(float)volume
{
    _volume = volume;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerVolumeChanged
                                              object:self];
}

- (void)setVolume:(float)volume
{
    // Set the ivar first and then we will receive whatever has been enabled via volumeChanged
    [self volumeChanged:volume];
    heimdall_player_aout_SetVolume(_p_player, volume);
}

- (void)incrementVolume
{
    heimdall_player_aout_IncrementVolume(_p_player, 1, NULL);
}

- (void)decrementVolume
{
    heimdall_player_aout_DecrementVolume(_p_player, 1, NULL);
}

- (void)muteChanged:(BOOL)mute
{
    _mute = mute;
    [_defaultNotificationCenter postNotificationName:HeimdallPlayerMuteChanged
                                              object:self];
}

- (void)setMute:(BOOL)mute
{
    // Set the ivar first and then we will receive whatever has been enabled via muteChanged
    [self muteChanged:mute];
    heimdall_player_aout_Mute(_p_player, mute);
}

- (void)toggleMute
{
    heimdall_player_aout_ToggleMute(_p_player);
}

- (audio_output_t *)mainAudioOutput
{
    return heimdall_player_aout_Hold(_p_player);
}

- (int)enableAudioFilterWithName:(NSString *)name state:(BOOL)state
{
    if (name == nil || name.length == 0) {
        return HEIMDALL_EINVAL;
    }

    return heimdall_player_aout_EnableFilter(_p_player, [name UTF8String], state);
}

- (int)enableVideoFilterWithName:(NSString *)name state:(BOOL)state
{
    if (name == nil || name.length == 0) {
        return HEIMDALL_EINVAL;
    }

    vout_thread_t * const vout = [self mainVideoOutputThread];
    if (vout == NULL) {
        return HEIMDALL_EGENERIC;
    }

    const char * const nameUTF8String = name.UTF8String;
    module_t * const module_obj = module_find(nameUTF8String);
    if (module_obj == NULL) {
        vout_Release(vout);
        return HEIMDALL_EGENERIC;
    }

    const char *filter_type = NULL;
    if (module_provides(module_obj, "video splitter")) {
        filter_type = "video-splitter";
    } else if (module_provides(module_obj, "video filter")) {
        filter_type = "video-filter";
    } else if (module_provides(module_obj, "sub source")) {
        filter_type = "sub-source";
    } else if (module_provides(module_obj, "sub filter")) {
        filter_type = "sub-filter";
    }

    if (filter_type == NULL) {
        vout_Release(vout);
        return HEIMDALL_EGENERIC;
    }

    // Get the current filter string
    char *modules_string = var_InheritString(vout, filter_type);

    if (state) { // Enable the filter
        if (modules_string == NULL) { // No current filters
            modules_string = strdup(nameUTF8String);
        } else if (strstr(modules_string, nameUTF8String) == NULL) { // Filter not already enabled
            char *psz_tmp = NULL;
            // Append the filter to the current filter string
            if (asprintf(&psz_tmp, "%s:%s", modules_string, nameUTF8String) == -1) {
                free(modules_string);
                vout_Release(vout);
                return HEIMDALL_ENOMEM;
            }
            free(modules_string);
            modules_string = psz_tmp;
        }
    } else { // Disable the filter
        if (modules_string == NULL) { // No current filters
            vout_Release(vout);
            return HEIMDALL_SUCCESS;
        }

        char * const parser = strstr(modules_string, nameUTF8String);
        if (parser != NULL) { // Enabled filter found
            const size_t name_len = strlen(nameUTF8String);
            // Check the next character to see if it is a colon, if it is...
            if (parser[name_len] == ':') { // ...filter is not the last one in the list
                // Remove the filter from the list by moving the rest of the string after the filter left
                memmove(parser, parser + name_len + 1,
                        strlen(parser + name_len + 1) + 1);
            } else { // Filter is the last one in the list
                // Remove the filter from the list by setting the null terminator to the end of the string
                *parser = '\0';
            }

            if (modules_string[0] != '\0' && modules_string[strlen(modules_string) - 1] == ':') {
                // Remove the trailing colon if it exists
                modules_string[strlen(modules_string) - 1] = '\0';
            }
        } else { // Filter not enabled, nothing needs doing
            free(modules_string);
            vout_Release(vout);
            return HEIMDALL_SUCCESS;
        }
    }

    var_SetString(vout, filter_type, modules_string);
    free(modules_string);
    vout_Release(vout);
    return HEIMDALL_SUCCESS;
}

- (int)setVideoFilterProperty:(NSString *)property forFilter:(NSString *)filterName withValue:(heimdall_value_t)value
{
    if (property == nil || property.length == 0 || filterName == nil || filterName.length == 0) {
        return HEIMDALL_EINVAL;
    }

    vout_thread_t * const vout = [self mainVideoOutputThread];
    if (vout == NULL) {
        return HEIMDALL_EGENERIC;
    }

    const char * const propertyUTF8String = property.UTF8String;
    int i_type = var_Type(vout, propertyUTF8String);
    if (i_type == 0) {
        i_type = config_GetType(propertyUTF8String);
    }

    i_type &= HEIMDALL_VAR_CLASS;
    var_SetChecked(vout, propertyUTF8String, i_type, value);
    vout_Release(vout);
    return HEIMDALL_SUCCESS;
}

@end

@implementation HeimdallInputStats

- (instancetype)initWithStatsStructure:(const struct input_stats_t *)p_stats
{
    self = [super init];
    if (self && p_stats != NULL) {
        _inputReadPackets = p_stats->i_read_packets;
        _inputReadBytes = p_stats->i_read_bytes;
        _inputBitrate = p_stats->f_input_bitrate;

        _demuxReadPackets = p_stats->i_demux_read_packets;
        _demuxReadBytes = p_stats->i_demux_read_bytes;
        _demuxBitrate = p_stats->f_demux_bitrate;
        _demuxCorrupted = p_stats->i_demux_corrupted;
        _demuxDiscontinuity = p_stats->i_demux_discontinuity;

        _decodedAudio = p_stats->i_decoded_audio;
        _decodedVideo = p_stats->i_decoded_video;

        _displayedPictures = p_stats->i_displayed_pictures;
        _latePictures = p_stats->i_late_pictures;
        _lostPictures = p_stats->i_lost_pictures;

        _playedAudioBuffers = p_stats->i_played_abuffers;
        _lostAudioBuffers = p_stats->i_lost_abuffers;
    }
    return self;
}

@end

@implementation HeimdallTrackMetaData

- (instancetype)initWithTrackStructure:(const struct heimdall_player_track *)p_track
{
    self = [super init];
    if (self && p_track != NULL) {
        _esID = p_track->es_id;
        _name = toNSStr(p_track->name);
        _selected = p_track->selected;
    }
    return self;
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"%@: name: %@, selected: %i", [HeimdallTrackMetaData className], self.name, self.selected];
}

@end

@implementation HeimdallProgramMetaData

- (instancetype)initWithProgramStructure:(const struct heimdall_player_program *)p_program
{
    self = [super init];
    if (self && p_program != NULL) {
        _group_id = p_program->group_id;
        _name = toNSStr(p_program->name);
        _selected = p_program->selected;
        _scrambled = p_program->scrambled;
    }
    return self;
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"%@: name: %@", [HeimdallProgramMetaData className], self.name];
}

@end
