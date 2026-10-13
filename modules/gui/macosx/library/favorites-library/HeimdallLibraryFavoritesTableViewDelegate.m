/*****************************************************************************
 * HeimdallLibraryFavoritesTableViewDelegate.m: MacOS X interface module
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

#import "HeimdallLibraryFavoritesTableViewDelegate.h"
#include <Foundation/Foundation.h>

#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryRepresentedItem.h"
#import "library/HeimdallLibraryTableCellView.h"
#import "library/HeimdallLibraryTableCellViewProtocol.h"
#import "library/HeimdallLibraryTableViewDataSource.h"
#import "library/audio-library/HeimdallLibraryAlbumTableCellView.h"
#import "library/favorites-library/HeimdallLibraryFavoritesDataSource.h"

@implementation HeimdallLibraryFavoritesTableViewDelegate

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
    [super tableViewSelectionDidChange:notification];

    NSTableView * const tableView = (NSTableView *)notification.object;
    if (![tableView.dataSource isKindOfClass:HeimdallLibraryFavoritesDataSource.class]) {
        return;
    }

    HeimdallLibraryFavoritesDataSource * const favoritesDataSource = (HeimdallLibraryFavoritesDataSource *)tableView.dataSource;
    if (tableView == favoritesDataSource.masterTableView) {
        [favoritesDataSource updateHeaderForMasterSelection];
    }
}

- (NSView *)tableView:(NSTableView *)tableView
   viewForTableColumn:(NSTableColumn *)tableColumn
                  row:(NSInteger)row
{
    if (![tableView.dataSource conformsToProtocol:@protocol(HeimdallLibraryTableViewDataSource)]) {
        return [super tableView:tableView viewForTableColumn:tableColumn row:row];
    }

    NSObject<HeimdallLibraryTableViewDataSource> * const heimdallDataSource = 
        (NSObject<HeimdallLibraryTableViewDataSource>*)tableView.dataSource;
    NSAssert(heimdallDataSource != nil, @"Should be a valid data source");

    id<HeimdallMediaLibraryItemProtocol> const libraryItem =
        [heimdallDataSource libraryItemAtRow:row forTableView:tableView];
    if (libraryItem == nil)
        return nil;

    HeimdallLibraryRepresentedItem * const representedItem = 
        [[HeimdallLibraryRepresentedItem alloc] initWithItem:libraryItem
                                             parentType:heimdallDataSource.currentParentType];

    const BOOL isAlbum = [libraryItem isKindOfClass:HeimdallMediaLibraryAlbum.class];

    NSString * const cellIdentifier =
        isAlbum ? HeimdallAudioLibraryCellIdentifier : HeimdallLibraryTableCellViewIdentifier;
    NSView<HeimdallLibraryTableCellViewProtocol> * const cellView =
        (NSView<HeimdallLibraryTableCellViewProtocol> *)[tableView makeViewWithIdentifier:cellIdentifier 
                                                                               owner:self];
    if (cellView == nil)
        return [super tableView:tableView viewForTableColumn:tableColumn row:row];

    [cellView setRepresentedItem:representedItem];
    return cellView;
}

- (CGFloat)tableView:(NSTableView *)tableView heightOfRow:(NSInteger)row
{
    if (![tableView.dataSource conformsToProtocol:@protocol(HeimdallLibraryTableViewDataSource)]) {
        return tableView.rowHeight;
    }

    NSObject<HeimdallLibraryTableViewDataSource> * const heimdallDataSource = 
        (NSObject<HeimdallLibraryTableViewDataSource>*)tableView.dataSource;
    id<HeimdallMediaLibraryItemProtocol> const libraryItem = [heimdallDataSource libraryItemAtRow:row 
                                                                           forTableView:tableView];

    if ([libraryItem isKindOfClass:HeimdallMediaLibraryAlbum.class]) {
        return [HeimdallLibraryAlbumTableCellView heightForAlbum:(HeimdallMediaLibraryAlbum *)libraryItem];
    }
    return tableView.rowHeight;
}

@end
