/*****************************************************************************
 * HeimdallPlayerTitle.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
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

#import <Foundation/Foundation.h>

#include <heimdall_player.h>

@class HeimdallPlayerChapter;

NS_ASSUME_NONNULL_BEGIN

@interface HeimdallPlayerTitle : NSObject

@property (readonly) size_t index;
@property (readonly) NSString *name;
@property (readonly) heimdall_tick_t length;
@property (readonly) NSString *lengthString;
@property (readonly) NSUInteger flags;
@property (readonly) NSUInteger chapterCount;
@property (readonly) NSArray<HeimdallPlayerChapter *> *chapters;

- (instancetype)initWithTitle:(const struct heimdall_player_title *)p_title atIndex:(size_t)index;

@end

NS_ASSUME_NONNULL_END
