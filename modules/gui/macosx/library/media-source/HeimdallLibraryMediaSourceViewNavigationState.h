/*****************************************************************************
 * HeimdallLibraryMediaSourceViewNavigationState.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Claudio Cambra <developer@claudiocambra.com>
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

NS_ASSUME_NONNULL_BEGIN

@class HeimdallMediaSourceDataSource;
@class HeimdallInputNode;
@class HeimdallInputNodePathControlItem;

@interface HeimdallLibraryMediaSourceViewNavigationState : NSObject

@property (readonly) HeimdallMediaSourceDataSource *currentMediaSource;
@property (readwrite) HeimdallInputNode *currentNodeDisplayed;
// Snapshot of the full breadcrumb shown while this state was current, so the
// path control can be rebuilt exactly when navigating back or forward to it.
@property (readwrite, nullable) NSArray<HeimdallInputNodePathControlItem *> *pathControlItems;

- (instancetype)initFromMediaSourceDataSource:(HeimdallMediaSourceDataSource *)mediaSourceDataSource;

@end

NS_ASSUME_NONNULL_END
