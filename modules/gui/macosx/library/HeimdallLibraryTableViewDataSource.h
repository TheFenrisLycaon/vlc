/*****************************************************************************
 * HeimdallLibraryTableViewDataSource.h: MacOS X interface module
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

#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryDataSource.h"

NS_ASSUME_NONNULL_BEGIN

@class HeimdallLibraryRepresentedItem;

@protocol HeimdallLibraryTableViewDataSource <NSTableViewDataSource, HeimdallLibraryDataSource>

@property (readonly) HeimdallMediaLibraryParentGroupType currentParentType;

- (NSInteger)rowForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem;
- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtRow:(NSInteger)row
                                       forTableView:(nullable NSTableView *)tableView;

@optional
- (nullable id<HeimdallMediaLibraryItemProtocol>)parentItemForTableView:(NSTableView *)tableView;
- (HeimdallLibraryRepresentedItem *)representedItemAtRow:(NSInteger)row
                                       forTableView:(NSTableView *)tableView;


@end

NS_ASSUME_NONNULL_END
