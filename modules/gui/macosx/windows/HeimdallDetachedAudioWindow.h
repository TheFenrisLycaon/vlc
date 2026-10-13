/*****************************************************************************
* HeimdallDetachedAudioWindow.h: macOS user interface
*****************************************************************************
* Copyright (C) 2019 VLC authors and VideoLAN
* Copyright (C) 2026 TheFenrisLycaon
*
* Author: Felix Paul Kühne <fkuehne at lycaria dot org>
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

@class HeimdallControlsBarCommon;
@class HeimdallImageView;
@class HeimdallBottomBarView;
@class HeimdallMainVideoViewOverlayView;
@class HeimdallMainVideoViewAudioMediaDecorativeView;

NS_ASSUME_NONNULL_BEGIN

@interface HeimdallDetachedAudioWindow : HeimdallWindow

@property (readonly) HeimdallMainVideoViewAudioMediaDecorativeView *decorativeView;
@property (nonatomic, weak) IBOutlet HeimdallMainVideoViewOverlayView *overlayView;
@property (nonatomic, weak) IBOutlet HeimdallControlsBarCommon *controlsBar;
@property (nonatomic, weak) IBOutlet HeimdallBottomBarView *bottomBarView;

@end

NS_ASSUME_NONNULL_END
