/*****************************************************************************
 * HeimdallBookmark.h: MacOS X interface module bookmarking functionality
 *****************************************************************************
 * Copyright (C) 2023 VLC authors and VideoLAN
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

#import <heimdall_media_library.h>

NS_ASSUME_NONNULL_BEGIN

@interface HeimdallBookmark : NSObject<NSCopying>

+ (instancetype)bookmarkWithHeimdallBookmark:(heimdall_ml_bookmark_t)heimdallBookmark;

@property (readonly) int64_t mediaLibraryItemId;
@property (readwrite, assign) int64_t bookmarkTime;
@property (readwrite) NSString *bookmarkName;
@property (readwrite) NSString *bookmarkDescription;


@end

NS_ASSUME_NONNULL_END
