/*****************************************************************************
 * HeimdallLibraryWindowPersistentPreferences.h: MacOS X interface module
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

#import <Cocoa/Cocoa.h>

#import "HeimdallLibraryWindow.h"

NS_ASSUME_NONNULL_BEGIN

@interface HeimdallLibraryWindowPersistentPreferences : NSObject

@property (class, readonly) HeimdallLibraryWindowPersistentPreferences *sharedInstance;

@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment homeLibraryViewMode;
@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment favoritesLibraryViewMode;
@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment videoLibraryViewMode;
@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment showsLibraryViewMode;
@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment moviesLibraryViewMode;
@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment albumLibraryViewMode;
@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment genreLibraryViewMode;
@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment songsLibraryViewMode;
@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment artistLibraryViewMode;
@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment playlistLibraryViewMode;
@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment musicOnlyPlaylistLibraryViewMode;
@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment videoOnlyPlaylistLibraryViewMode;
@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment browseLibraryViewMode;
@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment streamLibraryViewMode;
@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment groupsLibraryViewMode;
@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment searchLibraryViewMode;

@end

NS_ASSUME_NONNULL_END
