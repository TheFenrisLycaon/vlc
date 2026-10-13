/*****************************************************************************
 * applescript.m: MacOS X AppleScript support
 *****************************************************************************
 * Copyright (C) 2002-2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Derk-Jan Hartman <thedj@users.sourceforge.net>
 *          Felix Paul Kühne <fkuehne at lycaria dot org>
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

#import "applescript.h"

#import <heimdall_common.h>
#import <heimdall_url.h>

#import "main/HeimdallMain.h"
#import "playqueue/HeimdallPlayQueueController.h"
#import "playqueue/HeimdallPlayerController.h"
#import "windows/HeimdallOpenInputMetadata.h"

/*****************************************************************************
 * VLGetURLScriptCommand implementation
 *****************************************************************************/
@implementation VLGetURLScriptCommand

- (id)performDefaultImplementation
{
    NSString *commandString = [[self commandDescription] commandName];
    NSString *parameterString = [self directParameter];

    if ([commandString isEqualToString:@"GetURL"] || [commandString isEqualToString:@"OpenURL"]) {
        if (parameterString) {
            HeimdallOpenInputMetadata *inputMetadata = [[HeimdallOpenInputMetadata alloc] init];
            inputMetadata.MRLString = parameterString;

            [HeimdallMain.sharedInstance.playQueueController addPlayQueueItems:@[inputMetadata]];
        }
    }
    return nil;
}

@end

/*****************************************************************************
 * HeimdallontrolScriptCommand implementation
 *****************************************************************************/
/*
 * This entire control command needs a better design. more object oriented.
 * Applescript developers would be very welcome (hartman)
 */

@implementation HeimdallontrolScriptCommand

- (id)performDefaultImplementation
{
    HeimdallPlayQueueController * const playQueueController = HeimdallMain.sharedInstance.playQueueController;
    HeimdallPlayerController * const playerController = [playQueueController playerController];

    NSString * const commandString = [[self commandDescription] commandName];
    NSString * const parameterString = [self directParameter];

    if (commandString == nil || commandString.length == 0) {
        return nil;
    }

    if ([commandString isEqualToString:@"play"]) {
        [playerController togglePlayPause];
    } else if ([commandString isEqualToString:@"stop"]) {
        [playerController stop];
    } else if ([commandString isEqualToString:@"previous"]) {
        [playQueueController playPreviousItem];
    } else if ([commandString isEqualToString:@"next"]) {
        [playQueueController playNextItem];
    } else if ([commandString isEqualToString:@"fullscreen"]) {
        [playerController toggleFullscreen];
    } else if ([commandString isEqualToString:@"mute"]) {
        [playerController toggleMute];
    } else if ([commandString isEqualToString:@"volumeUp"]) {
        [playerController incrementVolume];
    } else if ([commandString isEqualToString:@"volumeDown"]) {
        [playerController decrementVolume];
    } else if ([commandString isEqualToString:@"moveMenuFocusUp"]) {
        [playerController navigateInInteractiveContent:HEIMDALL_PLAYER_NAV_UP];
    } else if ([commandString isEqualToString:@"moveMenuFocusDown"]) {
        [playerController navigateInInteractiveContent:HEIMDALL_PLAYER_NAV_DOWN];
    } else if ([commandString isEqualToString:@"moveMenuFocusLeft"]) {
        [playerController navigateInInteractiveContent:HEIMDALL_PLAYER_NAV_LEFT];
    } else if ([commandString isEqualToString:@"moveMenuFocusRight"]) {
        [playerController navigateInInteractiveContent:HEIMDALL_PLAYER_NAV_RIGHT];
    } else if ([commandString isEqualToString:@"menuFocusActivate"]) {
        [playerController navigateInInteractiveContent:HEIMDALL_PLAYER_NAV_ACTIVATE];
    } else if ([commandString isEqualToString:@"menuActivatePopupMenu"]) {
        [playerController navigateInInteractiveContent:HEIMDALL_PLAYER_NAV_POPUP];
    } else if ([commandString isEqualToString:@"menuActivateDiscRootMenu"]) {
        [playerController navigateInInteractiveContent:HEIMDALL_PLAYER_NAV_MENU];
    } else if ([commandString isEqualToString:@"stepForward"]) {
        if (parameterString) {
            int parameterInt = [parameterString intValue];
            switch (parameterInt) {
                case 1:
                    [playerController jumpForwardExtraShort];
                    break;
                case 3:
                    [playerController jumpForwardMedium];
                    break;
                case 4:
                    [playerController jumpForwardLong];
                    break;
                case 2:
                default:
                    [playerController jumpForwardShort];
                    break;
            }
        } else
            [playerController jumpForwardShort];
    } else if ([commandString isEqualToString:@"stepBackward"]) {
        //default: backwardShort
        if (parameterString) {
            int parameterInt = [parameterString intValue];
            switch (parameterInt) {
                case 1:
                    [playerController jumpBackwardExtraShort];
                    break;
                case 3:
                    [playerController jumpBackwardMedium];
                    break;
                case 4:
                    [playerController jumpBackwardLong];
                    break;
                case 2:
                default:
                    [playerController jumpBackwardShort];
                    break;
            }
        } else
            [playerController jumpBackwardShort];
    } else if ([commandString isEqualToString:@"incrementPlaybackRate"]) {
        [HeimdallMain.sharedInstance.playQueueController.playerController incrementPlaybackRate];
    } else if ([commandString isEqualToString:@"decrementPlaybackRate"]) {
        [HeimdallMain.sharedInstance.playQueueController.playerController decrementPlaybackRate];
    } else {
        msg_Err(getIntf(), "Unhandled AppleScript command '%s'", [commandString UTF8String]);
    }

    return nil;
}

@end

/*****************************************************************************
 * Category that adds AppleScript support to NSApplication
 *****************************************************************************/
@implementation NSApplication(ScriptSupport)

- (BOOL)scriptFullscreenMode
{
    return [HeimdallMain.sharedInstance.playQueueController.playerController fullscreen];
}

- (void)setScriptFullscreenMode:(BOOL)mode
{
    [HeimdallMain.sharedInstance.playQueueController.playerController setFullscreen:mode];
}

- (BOOL)muted
{
    return [HeimdallMain.sharedInstance.playQueueController.playerController mute];
}

- (BOOL)playing
{
    enum heimdall_player_state playerState = [HeimdallMain.sharedInstance.playQueueController.playerController playerState];

    if (playerState == HEIMDALL_PLAYER_STATE_STARTED || playerState == HEIMDALL_PLAYER_STATE_PLAYING) {
        return YES;
    }

    return NO;
}

- (float)audioVolume
{
    return [HeimdallMain.sharedInstance.playQueueController.playerController volume];
}

- (void)setAudioVolume:(float)volume
{
    [HeimdallMain.sharedInstance.playQueueController.playerController setVolume:volume];
}

- (long long)audioDesync
{
    return MS_FROM_HEIMDALL_TICK([HeimdallMain.sharedInstance.playQueueController.playerController audioDelay]);
}

- (void)setAudioDesync:(long long)audioDelay
{
    [HeimdallMain.sharedInstance.playQueueController.playerController setAudioDelay: HEIMDALL_TICK_FROM_MS(audioDelay)];
}

- (int)currentTime
{
    return (int)SEC_FROM_HEIMDALL_TICK([HeimdallMain.sharedInstance.playQueueController.playerController time]);
}

- (void)setCurrentTime:(int)currentTime
{
    [HeimdallMain.sharedInstance.playQueueController.playerController setTimeFast: HEIMDALL_TICK_FROM_SEC(currentTime)];
}

- (float)playbackRate
{
    return HeimdallMain.sharedInstance.playQueueController.playerController.playbackRate;
}

- (void)setPlaybackRate:(float)playbackRate
{
    [HeimdallMain.sharedInstance.playQueueController.playerController setPlaybackRate:playbackRate];
}

- (NSInteger)durationOfCurrentItem
{
    return SEC_FROM_HEIMDALL_TICK(HeimdallMain.sharedInstance.playQueueController.playerController.durationOfCurrentMediaItem);
}

- (NSString *)pathOfCurrentItem
{
    return HeimdallMain.sharedInstance.playQueueController.playerController.URLOfCurrentMediaItem.path;
}

- (NSString *)nameOfCurrentItem
{
    return HeimdallMain.sharedInstance.playQueueController.playerController.nameOfCurrentMediaItem;
}

- (BOOL)playbackShowsMenu
{
    const struct heimdall_player_title * const currentTitle = HeimdallMain.sharedInstance.playQueueController.playerController.selectedTitle;
    if (currentTitle == NULL) {
        return NO;
    }

    if (currentTitle->flags & HEIMDALL_PLAYER_TITLE_MENU) {
        return YES;
    }

    return NO;
}

- (BOOL)recordable
{
    return HeimdallMain.sharedInstance.playQueueController.playerController.recordable;
}

- (BOOL)recordingEnabled
{
    return HeimdallMain.sharedInstance.playQueueController.playerController.enableRecording;
}

- (void)setRecordingEnabled:(BOOL)recordingEnabled
{
    [HeimdallMain.sharedInstance.playQueueController.playerController setEnableRecording:recordingEnabled];
}

- (BOOL)shuffledPlayback
{
    const enum heimdall_playlist_playback_order playbackOrder = HeimdallMain.sharedInstance.playQueueController.playbackOrder;
    return playbackOrder == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM ? YES : NO;
}

- (void)setShuffledPlayback:(BOOL)shuffledPlayback
{
    [HeimdallMain.sharedInstance.playQueueController setPlaybackOrder: shuffledPlayback ? HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM : HEIMDALL_PLAYLIST_PLAYBACK_ORDER_NORMAL];
}

- (BOOL)repeatOne
{
    const enum heimdall_playlist_playback_repeat repeatMode = HeimdallMain.sharedInstance.playQueueController.playbackRepeat;
    return repeatMode == HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_CURRENT ? YES : NO;
}

- (void)setRepeatOne:(BOOL)repeatOne
{
    [HeimdallMain.sharedInstance.playQueueController setPlaybackRepeat: repeatOne == YES ? HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_CURRENT : HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_NONE];
}

- (BOOL)repeatAll
{
    const enum heimdall_playlist_playback_repeat repeatMode = HeimdallMain.sharedInstance.playQueueController.playbackRepeat;
    return repeatMode == HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL ? YES : NO;
}

- (void)setRepeatAll:(BOOL)repeatAll
{
    [HeimdallMain.sharedInstance.playQueueController setPlaybackRepeat: repeatAll == YES ? HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL : HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_NONE];
}

@end
