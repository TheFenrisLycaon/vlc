/*****************************************************************************
 * HeimdallLibraryAbstractGroupingDataSource.m: MacOS X interface module
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

#import "HeimdallLibraryAbstractGroupingDataSource.h"

#import "extensions/NSPasteboardItem+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

#import "library/HeimdallLibraryCollectionViewFlowLayout.h"
#import "library/HeimdallLibraryCollectionViewItem.h"
#import "library/HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.h"
#import "library/HeimdallLibraryCollectionViewSupplementaryElementView.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryRepresentedItem.h"

@interface HeimdallLibraryAbstractGroupingDataSource ()

@property (readwrite, copy) NSArray<id<HeimdallMediaLibraryItemProtocol>> *backingArraySnapshot;

@end

@implementation HeimdallLibraryAbstractGroupingDataSource

#pragma mark - Override points

- (NSArray<id<HeimdallMediaLibraryItemProtocol>> *)backingArray
{
    return self.backingArraySnapshot ?: @[];
}

- (NSArray<id<HeimdallMediaLibraryItemProtocol>> *)sourceBackingArray
{
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

- (HeimdallMediaLibraryParentGroupType)currentParentType
{
    [self doesNotRecognizeSelector:_cmd];
    return HeimdallMediaLibraryParentGroupTypeUnknown;
}

#pragma mark - Shared entry points

- (void)reloadData
{
    self.backingArraySnapshot = [self sourceBackingArray] ?: @[];
    [(HeimdallLibraryCollectionViewFlowLayout *)self.collectionView.collectionViewLayout resetLayout];
    [self.masterTableView reloadData];
    [self.detailTableView reloadData];
    [self.collectionView reloadData];
    [self updateHeaderInTableView:self.detailTableView forMasterSelection:self.masterTableView];
}

- (void)setLibraryModel:(HeimdallLibraryModel *)libraryModel
{
    if (_libraryModel == libraryModel) {
        return;
    }

    _libraryModel = libraryModel;
    [self reloadData];
}

- (void)applySnapshotForChangedItemID:(const int64_t)libraryID isDeletion:(BOOL)isDeletion
{
    NSArray<id<HeimdallMediaLibraryItemProtocol>> * const oldSnapshot = self.backingArraySnapshot ?: @[];
    NSArray<id<HeimdallMediaLibraryItemProtocol>> * const newSnapshot = [self sourceBackingArray] ?: @[];
    const NSInteger selectedRow = self.masterTableView.selectedRow;
    const int64_t selectedItemID =
        selectedRow > -1 && selectedRow < (NSInteger)oldSnapshot.count
            ? oldSnapshot[selectedRow].libraryID
            : -1;

    NSArray<NSNumber *> * const oldIDs = [self backingArrayIDsForSnapshot:oldSnapshot];
    NSArray<NSNumber *> * const newIDs = [self backingArrayIDsForSnapshot:newSnapshot];
    const NSUInteger oldIndex = [oldIDs indexOfObject:@(libraryID)];
    const NSUInteger newIndex = [newIDs indexOfObject:@(libraryID)];

    if (isDeletion) {
        const BOOL canDeleteSection =
            oldIndex != NSNotFound &&
            newIndex == NSNotFound &&
            oldSnapshot.count == newSnapshot.count + 1 &&
            [self newIDs:newIDs equalOldIDs:oldIDs withoutID:@(libraryID)];

        if (!canDeleteSection) {
            [self reloadViewsWithSnapshot:newSnapshot
                   preservingSelectionRow:selectedRow
                           selectedItemID:selectedItemID];
            return;
        }

        self.backingArraySnapshot = newSnapshot;
        [(HeimdallLibraryCollectionViewFlowLayout *)self.collectionView.collectionViewLayout resetLayout];
        [self.collectionView performBatchUpdates:^{
            [self.collectionView deleteSections:[NSIndexSet indexSetWithIndex:oldIndex]];
        } completionHandler:nil];
        [self reloadTableViewsPreservingSelectionItemID:selectedItemID
                                            fallbackRow:selectedRow];
        return;
    }

    const BOOL canReloadSection =
        oldIndex != NSNotFound &&
        newIndex != NSNotFound &&
        oldIndex == newIndex &&
        [oldIDs isEqualToArray:newIDs];

    if (!canReloadSection) {
        [self reloadViewsWithSnapshot:newSnapshot
               preservingSelectionRow:selectedRow
                       selectedItemID:selectedItemID];
        return;
    }

    self.backingArraySnapshot = newSnapshot;
    [self.collectionView performBatchUpdates:^{
        [self.collectionView reloadSections:[NSIndexSet indexSetWithIndex:newIndex]];
    } completionHandler:nil];
    [self reloadTableViewsPreservingSelectionItemID:selectedItemID
                                        fallbackRow:selectedRow];
}

#pragma mark - Utilities

- (NSUInteger)indexOfMediaItem:(const int64_t)libraryId inArray:(NSArray const *)array
{
    return [array indexOfObjectPassingTest:^BOOL(const id<HeimdallMediaLibraryItemProtocol> findItem,
                                                 const NSUInteger __unused idx,
                                                 BOOL * const __unused stop) {
        NSAssert(findItem != nil, @"Collection should not contain nil items");
        return findItem.libraryID == libraryId;
    }];
}

- (NSInteger)rowForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if (libraryItem == nil) {
        return NSNotFound;
    }
    return [self indexOfMediaItem:libraryItem.libraryID inArray:self.backingArray];
}

- (NSInteger)rowForLibraryItemID:(const int64_t)libraryID
{
    return [self indexOfMediaItem:libraryID inArray:self.backingArray];
}

#pragma mark - Table view data source and delegation

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    if (tableView == self.masterTableView) {
        return self.backingArray.count;
    }

    const NSInteger selectedMasterRow = self.masterTableView.selectedRow;
    const NSInteger backingItemsCount = self.backingArray.count;
    const BOOL selectedRowIsValid = selectedMasterRow > -1 && selectedMasterRow < backingItemsCount;

    if (tableView == self.detailTableView && backingItemsCount > 0 && selectedRowIsValid) {
        return self.backingArray[selectedMasterRow].mediaItems.count;
    }

    return 0;
}

- (id<NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row
{
    const id<HeimdallMediaLibraryItemProtocol> libraryItem = [self libraryItemAtRow:row
                                                                  forTableView:tableView];
    return [NSPasteboardItem pasteboardItemWithLibraryItem:libraryItem];
}

- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtRow:(NSInteger)row
                                       forTableView:(NSTableView *)tableView
{
    if (tableView == self.masterTableView) {
        return self.backingArray[row];
    }

    const NSInteger selectedMasterRow = self.masterTableView.selectedRow;
    if (tableView == self.detailTableView && selectedMasterRow > -1) {
        const id<HeimdallMediaLibraryItemProtocol> item = self.backingArray[selectedMasterRow];
        return item.mediaItems[row];
    }

    return nil;
}

- (void)updateHeaderInTableView:(NSTableView *)detailTableView forMasterSelection:(NSTableView *)masterTableView
{
    if (self.headerDelegate == nil) {
        return;
    }

    HeimdallLibraryRepresentedItem *representedItem = nil;
    NSString *fallbackTitle = nil;
    NSString *fallbackDetail = nil;

    const NSInteger selectedRow = masterTableView.selectedRow;
    if (selectedRow != -1) {
        const id<HeimdallMediaLibraryItemProtocol> selectedItem = [self libraryItemAtRow:selectedRow forTableView:masterTableView];
        representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:selectedItem parentType:self.currentParentType];
        fallbackTitle = selectedItem.displayString;
        fallbackDetail = selectedItem.primaryDetailString;
    }

    [self.headerDelegate updateHeaderForTableView:detailTableView
                              withRepresentedItem:representedItem
                                    fallbackTitle:fallbackTitle
                                   fallbackDetail:fallbackDetail];
}

#pragma mark - Collection view data source and delegation

- (NSInteger)numberOfSectionsInCollectionView:(NSCollectionView *)collectionView
{
    return self.backingArray.count;
}

- (NSInteger)collectionView:(NSCollectionView *)collectionView
     numberOfItemsInSection:(NSInteger)section
{
    return self.backingArray[section].mediaItems.count;
}

- (NSCollectionViewItem *)collectionView:(NSCollectionView *)collectionView
     itemForRepresentedObjectAtIndexPath:(NSIndexPath *)indexPath
{
    HeimdallLibraryCollectionViewItem * const viewItem =
        [collectionView makeItemWithIdentifier:HeimdallLibraryCollectionViewItemIdentifier
                                  forIndexPath:indexPath];
    const id<HeimdallMediaLibraryItemProtocol> item =
        [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
    HeimdallLibraryRepresentedItem * const representedItem =
        [[HeimdallLibraryRepresentedItem alloc] initWithItem:item parentType:self.currentParentType];
    viewItem.representedItem = representedItem;
    return viewItem;
}

- (NSView *)collectionView:(NSCollectionView *)collectionView
viewForSupplementaryElementOfKind:(NSCollectionViewSupplementaryElementKind)kind
               atIndexPath:(NSIndexPath *)indexPath
{
    if([kind isEqualToString:NSCollectionElementKindSectionHeader]) {
        HeimdallLibraryCollectionViewSupplementaryElementView * const sectionHeadingView =
            [collectionView makeSupplementaryViewOfKind:kind
                                         withIdentifier:HeimdallLibrarySupplementaryElementViewIdentifier
                                           forIndexPath:indexPath];
        const id<HeimdallMediaLibraryItemProtocol> item = self.backingArray[indexPath.section];
        NSString *displayString = item.displayString;
        if (displayString.length == 0) {
            displayString = [NSString stringWithFormat:@"%@ %@", _NS("Unknown"), self.dataSourceTypeDisplayString];
        }
        sectionHeadingView.stringValue = displayString;
        return sectionHeadingView;

    } else if ([kind isEqualToString:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind]) {
        NSString * const viewIdentifier =
            HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewIdentifier;
        HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView * const mediaItemDetailView =
            [collectionView makeSupplementaryViewOfKind:kind
                                         withIdentifier:viewIdentifier
                                           forIndexPath:indexPath];
        const id<HeimdallMediaLibraryItemProtocol> item = [self libraryItemAtIndexPath:indexPath
                                                                forCollectionView:collectionView];
        HeimdallLibraryRepresentedItem * const representedItem =
            [[HeimdallLibraryRepresentedItem alloc] initWithItem:item parentType:self.currentParentType];

        mediaItemDetailView.representedItem = representedItem;
        mediaItemDetailView.selectedItem = [collectionView itemAtIndexPath:indexPath];
        return mediaItemDetailView;
    }

    return nil;
}

- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtIndexPath:(NSIndexPath *)indexPath
                                        forCollectionView:(NSCollectionView *)collectionView
{
    const id<HeimdallMediaLibraryItemProtocol> item = self.backingArray[indexPath.section];
    return item.mediaItems[indexPath.item];
}

- (NSIndexPath *)indexPathForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if ([libraryItem isKindOfClass:self.backingArray.firstObject.class]) {
        const NSInteger itemIndex = [self indexOfMediaItem:libraryItem.libraryID
                                                   inArray:self.backingArray];
        return itemIndex != NSNotFound ? [NSIndexPath indexPathForItem:0 inSection:itemIndex] : nil;
    }

    __block NSInteger itemInternalMediaItemIndex = NSNotFound;
    const NSInteger itemIndex =
        [self.backingArray indexOfObjectPassingTest:^BOOL(const id<HeimdallMediaLibraryItemProtocol> item,
                                                          const NSUInteger __unused idx,
                                                          BOOL * const __unused stop) {
            itemInternalMediaItemIndex =
                [self indexOfMediaItem:libraryItem.libraryID inArray:item.mediaItems];
            return itemInternalMediaItemIndex != NSNotFound;
        }];

    return itemIndex != NSNotFound
        ? [NSIndexPath indexPathForItem:itemInternalMediaItemIndex inSection:itemIndex]
        : nil;
}

- (NSArray<HeimdallLibraryRepresentedItem *> *)representedItemsAtIndexPaths:(NSSet<NSIndexPath *> *)indexPaths
                                                     forCollectionView:(NSCollectionView *)collectionView
{
    NSMutableArray<HeimdallLibraryRepresentedItem *> * const representedItems =
        [NSMutableArray arrayWithCapacity:indexPaths.count];

    for (NSIndexPath * const indexPath in indexPaths) {
        const id<HeimdallMediaLibraryItemProtocol> libraryItem =
            [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
        HeimdallLibraryRepresentedItem * const representedItem =
            [[HeimdallLibraryRepresentedItem alloc] initWithItem:libraryItem
                                                 parentType:self.currentParentType];
        [representedItems addObject:representedItem];
    }

    return representedItems;
}

- (NSString *)supplementaryDetailViewKind
{
    return HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind;
}

#pragma mark - Snapshot helpers

- (NSArray<NSNumber *> *)backingArrayIDsForSnapshot:(NSArray<id<HeimdallMediaLibraryItemProtocol>> *)snapshot
{
    NSMutableArray<NSNumber *> * const snapshotIDs = [NSMutableArray arrayWithCapacity:snapshot.count];

    for (id<HeimdallMediaLibraryItemProtocol> const item in snapshot) {
        [snapshotIDs addObject:@(item.libraryID)];
    }

    return snapshotIDs.copy;
}

- (BOOL)newIDs:(NSArray<NSNumber *> *)newIDs
   equalOldIDs:(NSArray<NSNumber *> *)oldIDs
     withoutID:(NSNumber * _Nullable)removedID
{
    NSMutableArray<NSNumber *> * const filteredOldIDs =
        [NSMutableArray arrayWithCapacity:oldIDs.count];

    for (NSNumber * const itemID in oldIDs) {
        if (removedID != nil && [itemID isEqualToNumber:removedID]) {
            continue;
        }
        [filteredOldIDs addObject:itemID];
    }

    return [filteredOldIDs isEqualToArray:newIDs];
}

- (void)reloadViewsWithSnapshot:(NSArray<id<HeimdallMediaLibraryItemProtocol>> *)snapshot
         preservingSelectionRow:(const NSInteger)selectedRow
                 selectedItemID:(const int64_t)selectedItemID
{
    self.backingArraySnapshot = snapshot;
    [(HeimdallLibraryCollectionViewFlowLayout *)self.collectionView.collectionViewLayout resetLayout];
    [self.collectionView reloadData];
    [self reloadTableViewsPreservingSelectionItemID:selectedItemID
                                        fallbackRow:selectedRow];
}

- (void)reloadTableViewsPreservingSelectionItemID:(const int64_t)selectedItemID
                                      fallbackRow:(const NSInteger)selectedRow
{
    [self.masterTableView reloadData];
    [self restoreMasterSelectionUsingItemID:selectedItemID fallbackRow:selectedRow];
    [self.detailTableView reloadData];
    [self updateHeaderInTableView:self.detailTableView forMasterSelection:self.masterTableView];
}

- (void)restoreMasterSelectionUsingItemID:(const int64_t)itemID
                              fallbackRow:(const NSInteger)fallbackRow
{
    NSInteger rowToSelect = NSNotFound;

    if (itemID != -1) {
        rowToSelect = [self rowForLibraryItemID:itemID];
    }

    if (rowToSelect == NSNotFound && fallbackRow != NSNotFound) {
        if (self.backingArray.count == 0) {
            rowToSelect = NSNotFound;
        } else {
            rowToSelect = MIN(fallbackRow, (NSInteger)self.backingArray.count - 1);
        }
    }

    if (rowToSelect != NSNotFound) {
        [self.masterTableView selectRowIndexes:[NSIndexSet indexSetWithIndex:rowToSelect]
                          byExtendingSelection:NO];
    }
}

@end
