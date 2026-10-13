/*****************************************************************************
 * Windows.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2012-2014 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne -at- lycaria -dot- org>
 *          David Fuhrmann <david dot fuhrmann at googlemail dot com>
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

#import <Cocoa/Cocoa.h>

#import "windows/video/HeimdallWindow.h"

extern NSString *HeimdallVideoWindowShouldShowFullscreenController;
extern NSString *HeimdallVideoWindowDidEnterFullscreen;
extern NSString *HeimdallWindowShouldShowController;
extern const CGFloat HeimdallVideoWindowCommonMinimalHeight;

@class HeimdallMainVideoViewController;
@class HeimdallPlayerController;

/*****************************************************************************
 * HeimdallVideoWindowCommon
 *
 *  Common code for main window, detached window and extra video window
 *****************************************************************************/

@interface HeimdallVideoWindowCommon : HeimdallWindow <NSWindowDelegate, NSAnimationDelegate>

@property (nonatomic, strong) IBOutlet HeimdallMainVideoViewController* videoViewController;
@property (readonly) BOOL inFullscreenTransition;
@property (readonly) BOOL windowShouldExitFullscreenWhenFinished;
@property (readwrite, assign) NSRect previousSavedFrame;
@property (readonly) HeimdallPlayerController *playerController;

- (void)setup;

- (void)setWindowLevel:(NSInteger)i_state;

/* fullscreen handling */
- (void)enterFullscreenWithAnimation:(BOOL)b_animation;
- (void)leaveFullscreenWithAnimation:(BOOL)b_animation;

- (void)windowWillEnterFullScreen:(NSNotification *)notification;
- (void)windowDidEnterFullScreen:(NSNotification *)notification;
- (void)windowWillExitFullScreen:(NSNotification *)notification;

@end
