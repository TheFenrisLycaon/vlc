/*****************************************************************************
 * HeimdallLibrarySearchDataSource.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2025 VLC authors and VideoLAN
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

#import "library/HeimdallLibraryCollectionViewDataSource.h"
#import "library/HeimdallLibrarySectionedTableViewDataSource.h"
#import "library/HeimdallLibraryWindow.h"

NS_ASSUME_NONNULL_BEGIN

extern NSString * const HeimdallLibrarySearchDataSourceDidReloadNotification;

@interface HeimdallLibrarySearchDataSource : NSObject <HeimdallLibraryCollectionViewDataSource, HeimdallLibrarySectionedTableViewDataSource>

@property (readwrite, weak) NSCollectionView *collectionView;
@property (readwrite, weak) NSTableView *tableView;
@property (readwrite, nonatomic) HeimdallLibraryViewModeSegment viewMode;
@property (readonly) BOOL searching;

- (void)searchForString:(NSString *)string;
- (void)clearSearch;
- (void)reloadData;

@end

NS_ASSUME_NONNULL_END
