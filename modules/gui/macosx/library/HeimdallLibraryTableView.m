/*****************************************************************************
 * HeimdallLibraryTableView.m: MacOS X interface module
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

#import "HeimdallLibraryTableView.h"

#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryMenuController.h"
#import "library/HeimdallLibraryRepresentedItem.h"
#import "library/HeimdallLibrarySectionedTableViewDataSource.h"

#import "library/audio-library/HeimdallLibraryAudioDataSource.h"
#import "library/audio-library/HeimdallLibraryAudioGroupDataSource.h"

#import "media-source/HeimdallMediaSourceDataSource.h"

@interface HeimdallLibraryTableView ()
{
    HeimdallLibraryMenuController *_menuController;
    BOOL _heimdallDataSourceConforming;
}
@end

@implementation HeimdallLibraryTableView

- (void)setupMenu
{
    if(_menuController == nil) {
        _menuController = [[HeimdallLibraryMenuController alloc] init];
        _menuController.libraryMenu.delegate = self;
    }

    self.menu = _menuController.libraryMenu;
}

- (void)setDataSource:(id<NSTableViewDataSource>)dataSource
{
    [super setDataSource:dataSource];

    if([self.dataSource conformsToProtocol:@protocol(HeimdallLibraryTableViewDataSource)] ||
       self.dataSource.class == HeimdallMediaSourceDataSource.class) {
        _heimdallDataSourceConforming = YES;
        [self setupMenu];
    } else {
        _heimdallDataSourceConforming = NO;
        self.menu = nil;
        _menuController = nil;
    }
}

#pragma mark tableview menu delegates

- (NSMenu *)menuForEvent:(NSEvent *)event
{
    if (![self.dataSource conformsToProtocol:@protocol(HeimdallLibrarySectionedTableViewDataSource)]) {
        return [super menuForEvent:event];
    }

    const NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    const NSInteger row = [self rowAtPoint:point];
    NSObject<HeimdallLibrarySectionedTableViewDataSource> * const sectionedDataSource =
        (NSObject<HeimdallLibrarySectionedTableViewDataSource> *)self.dataSource;
    if (row < 0 || row >= self.numberOfRows || [sectionedDataSource isHeaderRow:row]) {
        return nil;
    }
    return [super menuForEvent:event];
}

- (void)menuNeedsUpdate:(NSMenu *)menu
{
    const id<NSTableViewDataSource> dataSource = self.dataSource;
    if (dataSource == nil || !_heimdallDataSourceConforming) {
        return;
    }

    /* right-click on empty space - no represented items, so actions are disabled */
    if (self.clickedRow == -1) {
        _menuController.representedItems = @[];
        return;
    }

    NSMutableIndexSet * const indices = self.selectedRowIndexes.mutableCopy;
    const NSUInteger hasSelectedIndices = indices.count > 0;
    const NSInteger clickedRow = self.clickedRow;

    if (!hasSelectedIndices) {
        if (clickedRow == -1L) {
            return;
        } else {
            [indices addIndex:clickedRow];
        }
    }

    if([dataSource conformsToProtocol:@protocol(HeimdallLibraryTableViewDataSource)]) {
        NSMutableArray<HeimdallLibraryRepresentedItem *> * const representedItems = 
            NSMutableArray.array;
        const id<HeimdallLibraryTableViewDataSource> heimdallLibraryDataSource = 
            (id<HeimdallLibraryTableViewDataSource>)dataSource;
        const HeimdallMediaLibraryParentGroupType parentType =
            heimdallLibraryDataSource.currentParentType;
        const id<HeimdallMediaLibraryItemProtocol> parentItem =
            [heimdallLibraryDataSource respondsToSelector:@selector(parentItemForTableView:)]
                ? [heimdallLibraryDataSource parentItemForTableView:self]
                : nil;

        if ([indices containsIndex:clickedRow]) {
            [indices enumerateIndexesUsingBlock:^(const NSUInteger index, BOOL * const __unused stop) {
                const id<HeimdallMediaLibraryItemProtocol> mediaItem =
                    [heimdallLibraryDataSource libraryItemAtRow:index forTableView:self];
                HeimdallLibraryRepresentedItem * const representedItem =
                    [[HeimdallLibraryRepresentedItem alloc] initWithItem:mediaItem
                                                         parentType:parentType
                                                         parentItem:parentItem
                                                   positionInParent:(NSInteger)index];
                [representedItems addObject:representedItem];
            }];
        } else {
            const id<HeimdallMediaLibraryItemProtocol> mediaItem = 
                [heimdallLibraryDataSource libraryItemAtRow:clickedRow forTableView:self];
            HeimdallLibraryRepresentedItem * const representedItem = 
                [[HeimdallLibraryRepresentedItem alloc] initWithItem:mediaItem
                                                     parentType:parentType
                                                     parentItem:parentItem
                                               positionInParent:clickedRow];
            [representedItems addObject:representedItem];
        }

        _menuController.representedItems = representedItems;

    } else if (dataSource.class == HeimdallMediaSourceDataSource.class) {
        NSMutableArray<HeimdallInputItem *> * const mediaSourceInputItems = NSMutableArray.array;
        HeimdallMediaSourceDataSource * const mediaSourceDataSource = 
            (HeimdallMediaSourceDataSource*)dataSource;
        NSAssert(mediaSourceDataSource != nil, @"This should be a valid pointer");

        [indices enumerateIndexesUsingBlock:^(const NSUInteger index, BOOL * const __unused stop) {
            HeimdallInputItem * const mediaSourceInputItem = 
                [mediaSourceDataSource mediaSourceInputItemAtRow:index];
            [mediaSourceInputItems addObject:mediaSourceInputItem];
        }];
        _menuController.representedInputItems = mediaSourceInputItems;
    }
}

@end
