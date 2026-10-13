/*****************************************************************************
 * HeimdallBookmarksTableViewDataSource.h: MacOS X interface module bookmarking functionality
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

NS_ASSUME_NONNULL_BEGIN

@class HeimdallBookmark;

extern NSString * const HeimdallBookmarksTableViewCellIdentifier;

extern NSString * const HeimdallBookmarksTableViewNameTableColumnIdentifier;
extern NSString * const HeimdallBookmarksTableViewDescriptionTableColumnIdentifier;
extern NSString * const HeimdallBookmarksTableViewTimeTableColumnIdentifier;

@interface HeimdallBookmarksTableViewDataSource : NSObject<NSTableViewDataSource>

@property (nonatomic, readwrite, assign) int64_t libraryItemId;
@property (readonly) NSArray<HeimdallBookmark *> *bookmarks;
@property (readwrite) NSTableView *tableView;

- (instancetype)initWithTableView:(NSTableView *)tableView;

- (void)addBookmark;
- (void)editBookmark:(HeimdallBookmark *)editedBookmark originalBookmark:(HeimdallBookmark *)originalBookmark;
- (void)removeBookmarkWithTime:(const int64_t)bookmarkTime;
- (void)removeBookmark:(HeimdallBookmark *)bookmark;
- (void)clearBookmarks;

- (void)updateBookmarks;
- (nullable HeimdallBookmark *)bookmarkForRow:(NSInteger)row;

@end

NS_ASSUME_NONNULL_END
