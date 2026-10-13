/*****************************************************************************
 * HeimdallLibraryTableViewDelegate.m: MacOS X interface module
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

#import "HeimdallLibraryTableViewDelegate.h"

#import "extensions/NSColor+HeimdallAdditions.h"
#import "extensions/NSView+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryRepresentedItem.h"
#import "library/HeimdallLibraryTableCellView.h"
#import "library/HeimdallLibraryTableCellViewProtocol.h"
#import "library/HeimdallLibraryTableViewDataSource.h"

#include "main/HeimdallMain.h"

#import "playqueue/HeimdallPlayQueueController.h"

@implementation HeimdallLibraryTableViewDelegate

- (instancetype)init
{
    self = [super init];
    if (self) {
        self.cellViewIdentifier = @"HeimdallLibraryTableViewCellIdentifier";
    }
    return self;
}

- (NSView<HeimdallLibraryTableCellViewProtocol> *)makeCellView
{
    return [HeimdallLibraryTableCellView fromNibWithOwner:self];
}

- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
    if (![tableView.dataSource conformsToProtocol:@protocol(HeimdallLibraryTableViewDataSource)]) {
        return nil;
    }

    NSObject<HeimdallLibraryTableViewDataSource> * const heimdallDataSource = (NSObject<HeimdallLibraryTableViewDataSource>*)tableView.dataSource;
    NSAssert(heimdallDataSource != nil, @"Should be a valid data source");

    NSView<HeimdallLibraryTableCellViewProtocol> * cellView = (NSView<HeimdallLibraryTableCellViewProtocol> *)[tableView makeViewWithIdentifier:self.cellViewIdentifier owner:self];

    if (cellView == nil) {
        cellView = [self makeCellView];
        cellView.identifier = self.cellViewIdentifier;
    }

    NSObject<HeimdallMediaLibraryItemProtocol> * const libraryItem = [heimdallDataSource libraryItemAtRow:row forTableView:tableView];
    if (libraryItem != nil) {
        HeimdallLibraryRepresentedItem *representedItem = nil;
        if ([heimdallDataSource respondsToSelector:@selector(representedItemAtRow:forTableView:)]) {
            representedItem = [heimdallDataSource representedItemAtRow:row forTableView:tableView];
        }

        if (representedItem == nil) {
            representedItem = 
                [[HeimdallLibraryRepresentedItem alloc] initWithItem:libraryItem
                                                     parentType:heimdallDataSource.currentParentType];
        }
        [cellView setRepresentedItem:representedItem];
    }
    return cellView;
}

- (NSArray<NSTableViewRowAction *> *)tableView:(NSTableView *)tableView
                              rowActionsForRow:(NSInteger)row
                                          edge:(NSTableRowActionEdge)edge
{
    if (![tableView.dataSource conformsToProtocol:@protocol(HeimdallLibraryTableViewDataSource)]) {
        return @[];
    }

    NSObject<HeimdallLibraryTableViewDataSource> * const heimdallDataSource =
        (NSObject<HeimdallLibraryTableViewDataSource>*)tableView.dataSource;
    NSAssert(heimdallDataSource != nil, @"Should be a valid data source");

    NSObject<HeimdallMediaLibraryItemProtocol> * const libraryItem =
        [heimdallDataSource libraryItemAtRow:row forTableView:tableView];

    if (edge == NSTableRowActionEdgeLeading) {
        NSTableViewRowAction * const appendToPlayQueueAction =
            [NSTableViewRowAction rowActionWithStyle:NSTableViewRowActionStyleRegular
                                               title:_NS("Append to Play Queue")
                                             handler:^(NSTableViewRowAction * const __unused action, const NSInteger __unused row) {
                HeimdallLibraryController * const libraryController = HeimdallMain.sharedInstance.libraryController;
                for (HeimdallMediaLibraryMediaItem * const mediaItem in libraryItem.mediaItems) {
                    [libraryController appendItemToPlayQueue:mediaItem playImmediately:NO];
                }
        }];
        appendToPlayQueueAction.backgroundColor = NSColor.HeimdallAccentColor;
        return @[appendToPlayQueueAction];
    } else if (edge == NSTableRowActionEdgeTrailing) {
        const BOOL isFavorited = libraryItem.favorited;
        NSString * const displayString =
            isFavorited ? _NS("Remove from Favorites") : _NS("Add to Favorites");

        NSTableViewRowAction * const favoriteAction =
            [NSTableViewRowAction rowActionWithStyle:NSTableViewRowActionStyleRegular
                                               title:displayString
                                             handler:^(NSTableViewRowAction * const __unused action, const NSInteger __unused row) {
            [libraryItem setFavorite:!isFavorited];
        }];
        favoriteAction.backgroundColor =
            isFavorited ? NSColor.systemRedColor : NSColor.systemBlueColor;
        return @[favoriteAction];
    }
    return @[];
}

@end
