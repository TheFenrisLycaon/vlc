/*****************************************************************************
 * HeimdallVideoOutputProvider.h: MacOS X interface module
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

#import <heimdall_window.h>

@class HeimdallControlsBarCommon;
@class HeimdallVideoWindowCommon;
@class HeimdallVoutView;

extern NSString * const HeimdallWindowShouldUpdateLevel;
extern NSString * const HeimdallWindowLevelKey;
extern NSString * const HeimdallWindowFloatOnTopChangedNotificationName;
extern NSString * const HeimdallWindowFloatOnTopEnabledNotificationKey;

@interface HeimdallVideoOutputProvider : NSObject

@property (readonly, nonatomic) NSInteger currentStatusWindowLevel;
@property (readonly) NSDictionary *voutWindows;

- (HeimdallVoutView *)setupVoutForWindow:(heimdall_window_t *)p_wnd withProposedVideoViewPosition:(NSRect)videoViewPosition;
- (HeimdallVideoWindowCommon *)setupVideoWindow;

- (void)removeVoutForDisplay:(NSValue *)o_key;
- (void)setNativeVideoSize:(NSSize)size forWindow:(heimdall_window_t *)p_wnd;
- (void)setWindowLevel:(NSInteger)i_level forWindow:(heimdall_window_t *)p_wnd;
- (void)setFullscreen:(int)i_full forWindow:(heimdall_window_t *)p_wnd withAnimation:(BOOL)b_animation;

- (void)updateWindowLevelForHelperWindows:(NSInteger)i_level;

- (HeimdallVideoWindowCommon *)videoWindowForVoutView:(HeimdallVoutView *)voutView;

@end
