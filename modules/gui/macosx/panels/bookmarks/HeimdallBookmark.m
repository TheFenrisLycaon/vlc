/*****************************************************************************
 * HeimdallBookmark.m: MacOS X interface module bookmarking functionality
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

#import "HeimdallBookmark.h"

#import "extensions/NSString+Helpers.h"

@implementation HeimdallBookmark

+ (instancetype)bookmarkWithHeimdallBookmark:(heimdall_ml_bookmark_t)heimdallBookmark
{
    HeimdallBookmark * const bookmark = [[HeimdallBookmark alloc] init];
    
    bookmark->_mediaLibraryItemId = heimdallBookmark.i_media_id;
    bookmark.bookmarkTime = heimdallBookmark.i_time;
    bookmark.bookmarkName = toNSStr(heimdallBookmark.psz_name);
    bookmark.bookmarkDescription = toNSStr(heimdallBookmark.psz_description);

    return bookmark;
}

- (nonnull id)copyWithZone:(nullable NSZone *)zone
{
    HeimdallBookmark * const bookmarkCopy = [[HeimdallBookmark alloc] init];

    bookmarkCopy->_mediaLibraryItemId = self.mediaLibraryItemId;
    bookmarkCopy.bookmarkTime = self.bookmarkTime;
    bookmarkCopy.bookmarkName = self.bookmarkName;
    bookmarkCopy.bookmarkDescription = self.bookmarkDescription;

    return bookmarkCopy;
}

@end
