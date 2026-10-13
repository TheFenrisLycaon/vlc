/*****************************************************************************
 * HeimdallLibraryVideoDataSource.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne # lycaria -dot- org>
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

#import "HeimdallLibraryVideoDataSource.h"

#import "library/HeimdallLibraryCollectionViewFlowLayout.h"
#import "library/HeimdallLibraryCollectionViewItem.h"
#import "library/HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.h"
#import "library/HeimdallLibraryCollectionViewSupplementaryElementView.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryRepresentedItem.h"
#import "library/HeimdallLibraryTableCellView.h"

#import "library/video-library/HeimdallLibraryVideoGroupDescriptor.h"

#import "main/CompatibilityFixes.h"
#import "main/HeimdallMain.h"

#import "extensions/NSIndexSet+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"
#import "extensions/NSPasteboardItem+HeimdallAdditions.h"

NSString * const HeimdallLibraryVideoDataSourceDisplayedCollectionChangedNotification = @"HeimdallLibraryVideoDataSourceDisplayedCollectionChangedNotification";

/**
 * Represents one row in the flattened table view model.
 * A row is either a section header or a media item within a section.
 */
@interface HeimdallLibraryVideoFlattenedRow : NSObject
@property (readonly) BOOL isHeader;
@property (readonly) HeimdallMediaLibraryParentGroupType parentType;
@property (readonly) NSInteger itemIndex; // -1 for header rows
+ (instancetype)headerForGroup:(HeimdallMediaLibraryParentGroupType)group;
+ (instancetype)itemAtIndex:(NSInteger)index inGroup:(HeimdallMediaLibraryParentGroupType)group;
@end

@implementation HeimdallLibraryVideoFlattenedRow

+ (instancetype)headerForGroup:(HeimdallMediaLibraryParentGroupType)group
{
    HeimdallLibraryVideoFlattenedRow * const row = [HeimdallLibraryVideoFlattenedRow new];
    row->_isHeader = YES;
    row->_parentType = group;
    row->_itemIndex = -1;
    return row;
}

+ (instancetype)itemAtIndex:(NSInteger)index inGroup:(HeimdallMediaLibraryParentGroupType)group
{
    HeimdallLibraryVideoFlattenedRow * const row = [HeimdallLibraryVideoFlattenedRow new];
    row->_isHeader = NO;
    row->_parentType = group;
    row->_itemIndex = index;
    return row;
}

@end

@interface HeimdallLibraryVideoDataSource ()
{
    NSMutableArray *_recentsArray;
    NSMutableArray *_libraryArray;
    HeimdallLibraryCollectionViewFlowLayout *_collectionViewFlowLayout;
    NSUInteger _priorNumVideoSections;
    NSArray<HeimdallLibraryVideoFlattenedRow *> *_flattenedRows;
}

@end

@implementation HeimdallLibraryVideoDataSource

- (instancetype)init
{
    self = [super init];
    if(self) {
        _flattenedRows = @[];
        [self connect];
    }
    return self;
}

- (NSUInteger)indexOfMediaItem:(int64_t)libraryId inArray:(NSArray const *)array
{
    return [array indexOfObjectPassingTest:^BOOL(HeimdallMediaLibraryMediaItem * const findMediaItem, const NSUInteger __unused idx, BOOL * const __unused stop) {
        NSAssert(findMediaItem != nil, @"Collection should not contain nil media items");
        return findMediaItem.libraryID == libraryId;
    }];
}

- (void)libraryModelVideoListReset:(NSNotification * const)aNotification
{
    [self reloadData];
}

- (void)libraryModelVideoItemUpdated:(NSNotification * const)aNotification
{
    NSParameterAssert(aNotification);
    HeimdallMediaLibraryMediaItem * const notificationMediaItem = aNotification.object;
    NSAssert(notificationMediaItem != nil, @"Media item updated notification should carry valid media item");

    [self reloadDataForMediaItem:notificationMediaItem
                    inVideoGroup:HeimdallMediaLibraryParentGroupTypeVideoLibrary];
}

- (void)libraryModelVideoItemDeleted:(NSNotification * const)aNotification
{
    NSParameterAssert(aNotification);
    HeimdallMediaLibraryMediaItem * const notificationMediaItem = aNotification.object;
    NSAssert(notificationMediaItem != nil, @"Media item deleted notification should carry valid media item");

    [self deleteDataForMediaItem:notificationMediaItem
                    inVideoGroup:HeimdallMediaLibraryParentGroupTypeVideoLibrary];
}

- (void)libraryModelRecentsListReset:(NSNotification * const)aNotification
{
    [self reloadData];
}

- (void)libraryModelRecentsItemUpdated:(NSNotification * const)aNotification
{
    NSParameterAssert(aNotification);
    HeimdallMediaLibraryMediaItem * const notificationMediaItem = aNotification.object;
    NSAssert(notificationMediaItem != nil, @"Media item updated notification should carry valid media item");

    [self reloadDataForMediaItem:notificationMediaItem
                    inVideoGroup:HeimdallMediaLibraryParentGroupTypeRecentVideos];
}

- (void)libraryModelRecentsItemDeleted:(NSNotification * const)aNotification
{
    NSParameterAssert(aNotification);
    HeimdallMediaLibraryMediaItem * const notificationMediaItem = aNotification.object;
    NSAssert(notificationMediaItem != nil, @"Media item deleted notification should carry valid media item");

    [self deleteDataForMediaItem:notificationMediaItem
                    inVideoGroup:HeimdallMediaLibraryParentGroupTypeRecentVideos];
}

- (void)connect
{
    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;

    [notificationCenter addObserver:self
                           selector:@selector(libraryModelVideoListReset:)
                               name:HeimdallLibraryModelVideoMediaListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelVideoListReset:)
                               name:HeimdallLibraryModelAllCachesDropped
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelVideoItemUpdated:)
                               name:HeimdallLibraryModelVideoMediaItemUpdated
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelVideoItemDeleted:)
                               name:HeimdallLibraryModelVideoMediaItemDeleted
                             object:nil];

    [notificationCenter addObserver:self
                           selector:@selector(libraryModelRecentsListReset:)
                               name:HeimdallLibraryModelRecentsMediaListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelRecentsItemUpdated:)
                               name:HeimdallLibraryModelRecentsMediaItemUpdated
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelRecentsItemDeleted:)
                               name:HeimdallLibraryModelRecentsMediaItemDeleted
                             object:nil];

    [self reloadData];
}

- (void)disconnect
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark - Flattened row model

- (NSArray *)arrayForGroup:(HeimdallMediaLibraryParentGroupType)group
{
    switch (group) {
        case HeimdallMediaLibraryParentGroupTypeRecentVideos:
            return _recentsArray;
        case HeimdallMediaLibraryParentGroupTypeVideoLibrary:
            return _libraryArray;
        default:
            return @[];
    }
}

- (void)rebuildFlattenedRows
{
    NSMutableArray<HeimdallLibraryVideoFlattenedRow *> * const rows = [NSMutableArray array];

    const NSUInteger recentsCount = _recentsArray.count;
    if (recentsCount > 0) {
        [rows addObject:[HeimdallLibraryVideoFlattenedRow headerForGroup:HeimdallMediaLibraryParentGroupTypeRecentVideos]];
        for (NSUInteger i = 0; i < recentsCount; i++) {
            [rows addObject:[HeimdallLibraryVideoFlattenedRow itemAtIndex:i
                                                             inGroup:HeimdallMediaLibraryParentGroupTypeRecentVideos]];
        }
    }

    const NSUInteger libraryCount = _libraryArray.count;
    if (libraryCount > 0) {
        [rows addObject:[HeimdallLibraryVideoFlattenedRow headerForGroup:HeimdallMediaLibraryParentGroupTypeVideoLibrary]];
        for (NSUInteger i = 0; i < libraryCount; i++) {
            [rows addObject:[HeimdallLibraryVideoFlattenedRow itemAtIndex:i
                                                             inGroup:HeimdallMediaLibraryParentGroupTypeVideoLibrary]];
        }
    }

    _flattenedRows = [rows copy];
}

- (void)reloadData
{
    if(!_libraryModel) {
        return;
    }

    [_collectionViewFlowLayout resetLayout];

    self->_recentsArray = [[self.libraryModel listOfRecentMedia] mutableCopy];
    self->_libraryArray = [[self.libraryModel listOfVideoMedia] mutableCopy];

    [self rebuildFlattenedRows];

    NSAssert(self.tableView == nil || self.tableView.dataSource == self, @"Cannot reload a table view with a different data source");
    [self.tableView reloadData];

    NSAssert(self.collectionView == nil || self.collectionView.dataSource == self, @"Cannot reload a collection view with a different data source");
    [self.collectionView reloadData];

    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallLibraryVideoDataSourceDisplayedCollectionChangedNotification
                                                      object:self
                                                    userInfo:nil];
}

- (NSInteger)flattenedRowIndexForItemIndex:(NSUInteger)itemIndex
                                   inGroup:(HeimdallMediaLibraryParentGroupType)group
{
    const NSUInteger rowCount = _flattenedRows.count;
    for (NSUInteger i = 0; i < rowCount; i++) {
        HeimdallLibraryVideoFlattenedRow * const flatRow = _flattenedRows[i];
        if (!flatRow.isHeader &&
            flatRow.parentType == group &&
            flatRow.itemIndex == (NSInteger)itemIndex) {
            return i;
        }
    }
    return NSNotFound;
}

- (void)changeDataForSpecificMediaItem:(HeimdallMediaLibraryMediaItem * const)mediaItem
                          inVideoGroup:(const HeimdallMediaLibraryParentGroupType)group
                        arrayOperation:(void(^)(const NSMutableArray*, const NSUInteger))arrayOperation
                     completionHandler:(void(^)(const NSIndexSet*))completionHandler
{
    NSMutableArray *groupArray = (NSMutableArray *)[self arrayForGroup:group];
    if (groupArray == nil) {
        return;
    }

    const NSUInteger mediaItemIndex = [self indexOfMediaItem:mediaItem.libraryID inArray:groupArray];
    if (mediaItemIndex == NSNotFound) {
        return;
    }

    // Find the flattened row index BEFORE mutating the arrays
    const NSInteger flatRowIndex = [self flattenedRowIndexForItemIndex:mediaItemIndex
                                                              inGroup:group];

    arrayOperation(groupArray, mediaItemIndex);
    [self rebuildFlattenedRows];

    NSIndexSet * const rowIndexSet = [NSIndexSet indexSetWithIndex:mediaItemIndex];
    completionHandler(rowIndexSet);

    // Targeted table view update using flattened row index
    if (flatRowIndex != NSNotFound) {
        NSAssert(self.tableView.dataSource == self, @"Cannot reload a table view with a different data source");
        [self.tableView reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:flatRowIndex]
                                  columnIndexes:[NSIndexSet indexSetWithIndex:0]];
    }

    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallLibraryVideoDataSourceDisplayedCollectionChangedNotification
                                                      object:self
                                                    userInfo:nil];
}

- (void)reloadDataForMediaItem:(HeimdallMediaLibraryMediaItem * const)mediaItem
                  inVideoGroup:(const HeimdallMediaLibraryParentGroupType)group
{
    [self changeDataForSpecificMediaItem:mediaItem
                            inVideoGroup:group
                          arrayOperation:^(NSMutableArray * const mediaArray, const NSUInteger mediaItemIndex) {

        [mediaArray replaceObjectAtIndex:mediaItemIndex withObject:mediaItem];

    } completionHandler:^(NSIndexSet * const rowIndexSet) {

        NSAssert(self.collectionView.dataSource == self, @"Cannot reload a collection view with a different data source");
        const NSInteger section = [self videoGroupToRow:group];
        NSSet<NSIndexPath *> * const indexPathSet =
            [rowIndexSet indexPathSetWithSection:section];
        [self.collectionView reloadItemsAtIndexPaths:indexPathSet];
    }];
}

- (void)deleteDataForMediaItem:(HeimdallMediaLibraryMediaItem * const)mediaItem
                  inVideoGroup:(const HeimdallMediaLibraryParentGroupType)group
{
    [self changeDataForSpecificMediaItem:mediaItem
                            inVideoGroup:group
                          arrayOperation:^(NSMutableArray * const mediaArray, const NSUInteger mediaItemIndex) {

        [mediaArray removeObjectAtIndex:mediaItemIndex];

    } completionHandler:^(NSIndexSet * const rowIndexSet) {

        NSAssert(self.collectionView.dataSource == self, @"Cannot reload a collection view with a different data source");
        const NSInteger section = [self videoGroupToRow:group];
        NSSet<NSIndexPath *> * const indexPathSet =
            [rowIndexSet indexPathSetWithSection:section];
        [self.collectionView deleteItemsAtIndexPaths:indexPathSet];
    }];
}

#pragma mark - Public query methods

- (BOOL)isHeaderRow:(NSInteger)row
{
    if (row < 0 || (NSUInteger)row >= _flattenedRows.count) {
        return NO;
    }
    return _flattenedRows[row].isHeader;
}

- (HeimdallMediaLibraryParentGroupType)parentTypeForRow:(NSInteger)row
{
    if (row < 0 || (NSUInteger)row >= _flattenedRows.count) {
        return HeimdallMediaLibraryParentGroupTypeUnknown;
    }
    return _flattenedRows[row].parentType;
}

- (NSString *)titleForRow:(NSInteger)row
{
    return [self titleForVideoGroup:[self parentTypeForRow:row]];
}

- (HeimdallLibraryRepresentedItem *)representedItemForHeaderRow:(NSInteger)row
{
    if (![self isHeaderRow:row]) {
        return nil;
    }

    const HeimdallMediaLibraryParentGroupType parentType = [self parentTypeForRow:row];
    NSArray * const groupArray = [self arrayForGroup:parentType];
    if (groupArray.count == 0) {
        return nil;
    }

    NSString * const title = [self titleForVideoGroup:parentType];
    HeimdallMediaLibraryDummyItem * const groupItem =
        [[HeimdallMediaLibraryDummyItem alloc] initWithDisplayString:title
                                                 withMediaItems:groupArray];
    return [[HeimdallLibraryRepresentedItem alloc] initWithItem:groupItem parentType:parentType];
}

#pragma mark - Table view data source (sectioned flat table)

- (BOOL)recentItemsPresent
{
    return _recentsArray.count > 0;
}

- (NSUInteger)rowToVideoGroupAdjustment
{
    static const HeimdallMediaLibraryParentGroupType firstEntry = HeimdallMediaLibraryParentGroupTypeRecentVideos;
    const BOOL anyRecents = [self recentItemsPresent];
    return anyRecents ? firstEntry : firstEntry + 1;
}

- (HeimdallMediaLibraryParentGroupType)rowToVideoGroup:(NSInteger)row
{
    return row + [self rowToVideoGroupAdjustment];
}

- (NSUInteger)videoGroupToRow:(NSInteger)videoGroup
{
    return videoGroup - [self rowToVideoGroupAdjustment];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    return _flattenedRows.count;
}

- (id<NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row
{
    if ([self isHeaderRow:row]) {
        return nil;
    }
    const id<HeimdallMediaLibraryItemProtocol> libraryItem = [self libraryItemAtRow:row forTableView:tableView];
    return [NSPasteboardItem pasteboardItemWithLibraryItem:libraryItem];
}

- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtRow:(NSInteger)row
                                       forTableView:(NSTableView *)tableView
{
    if (row < 0 || (NSUInteger)row >= _flattenedRows.count) {
        return nil;
    }

    HeimdallLibraryVideoFlattenedRow * const flatRow = _flattenedRows[row];

    if (flatRow.isHeader) {
        return nil;
    }

    NSArray * const groupArray = [self arrayForGroup:flatRow.parentType];
    if (flatRow.itemIndex >= 0 && (NSUInteger)flatRow.itemIndex < groupArray.count) {
        return groupArray[flatRow.itemIndex];
    }

    return nil;
}

- (NSInteger)rowForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if (libraryItem == nil) {
        return NSNotFound;
    }

    const NSUInteger rowCount = _flattenedRows.count;
    for (NSUInteger i = 0; i < rowCount; i++) {
        HeimdallLibraryVideoFlattenedRow * const flatRow = _flattenedRows[i];
        if (flatRow.isHeader) {
            continue;
        }

        NSArray * const groupArray = [self arrayForGroup:flatRow.parentType];
        if (flatRow.itemIndex >= 0 && (NSUInteger)flatRow.itemIndex < groupArray.count) {
            id<HeimdallMediaLibraryItemProtocol> const item = groupArray[flatRow.itemIndex];
            if (item.libraryID == libraryItem.libraryID) {
                return i;
            }
        }
    }

    return NSNotFound;
}

- (HeimdallMediaLibraryParentGroupType)currentParentType
{
    return HeimdallMediaLibraryParentGroupTypeVideoLibrary;
}

# pragma mark - collection view data source and delegation

- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtIndexPath:(NSIndexPath *)indexPath
                                        forCollectionView:(NSCollectionView *)collectionView
{
    switch ([self rowToVideoGroup:indexPath.section]) {
        case HeimdallMediaLibraryParentGroupTypeRecentVideos:
            return _recentsArray[indexPath.item];
        case HeimdallMediaLibraryParentGroupTypeVideoLibrary:
            return _libraryArray[indexPath.item];
        default:
            NSAssert(NO, @"Unknown video group received");
            return nil;
    }
}

- (NSIndexPath *)indexPathForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    const NSInteger row = [self rowForLibraryItem:libraryItem];
    if (row == NSNotFound) {
        return nil;
    }
    const NSInteger section = [self videoGroupToRow:HeimdallMediaLibraryParentGroupTypeVideoLibrary];
    return [NSIndexPath indexPathForItem:row inSection:section];
}

- (NSArray<HeimdallLibraryRepresentedItem *> *)representedItemsAtIndexPaths:(NSSet<NSIndexPath *> *const)indexPaths
                                                     forCollectionView:(NSCollectionView *)collectionView
{
    NSMutableArray<HeimdallLibraryRepresentedItem *> * const representedItems =
        [NSMutableArray arrayWithCapacity:indexPaths.count];

    for (NSIndexPath * const indexPath in indexPaths) {
        const HeimdallMediaLibraryParentGroupType parentType = [self rowToVideoGroup:indexPath.section];
        const id<HeimdallMediaLibraryItemProtocol> libraryItem =
            [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
        HeimdallLibraryRepresentedItem * const representedItem =
            [[HeimdallLibraryRepresentedItem alloc] initWithItem:libraryItem parentType:parentType];
        [representedItems addObject:representedItem];
    }

    return representedItems;
}

- (NSString *)titleForVideoGroup:(HeimdallMediaLibraryParentGroupType)videoGroup
{
    switch (videoGroup) {
        case HeimdallMediaLibraryParentGroupTypeRecentVideos:
            return _NS("Recents");
        case HeimdallMediaLibraryParentGroupTypeVideoLibrary:
            return _NS("Library");
        default:
            NSAssert(NO, @"Received unknown video group");
            return @"";
    }
}

- (NSInteger)numberOfSectionsInCollectionView:(NSCollectionView *)collectionView
{
    _priorNumVideoSections = [self recentItemsPresent] ? 2 : 1;
    return _priorNumVideoSections;
}

- (NSInteger)collectionView:(NSCollectionView *)collectionView
     numberOfItemsInSection:(NSInteger)section
{
    switch ([self rowToVideoGroup:section]) {
        case HeimdallMediaLibraryParentGroupTypeRecentVideos:
            return _recentsArray.count;
        case HeimdallMediaLibraryParentGroupTypeVideoLibrary:
            return _libraryArray.count;
        default:
            NSAssert(NO, @"Unknown video group received.");
            return NSNotFound;
    }
}

- (NSCollectionViewItem *)collectionView:(NSCollectionView *)collectionView
     itemForRepresentedObjectAtIndexPath:(NSIndexPath *)indexPath
{
    HeimdallLibraryCollectionViewItem * const viewItem =
        [collectionView makeItemWithIdentifier:HeimdallLibraryCollectionViewItemIdentifier forIndexPath:indexPath];
    const HeimdallMediaLibraryParentGroupType parentType = [self rowToVideoGroup:indexPath.section];
    const id<HeimdallMediaLibraryItemProtocol> item =
        [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
    HeimdallLibraryRepresentedItem * const representedItem =
        [[HeimdallLibraryRepresentedItem alloc] initWithItem:item parentType:parentType];
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
        const HeimdallMediaLibraryParentGroupType videoGroup = [self rowToVideoGroup:indexPath.section];
        sectionHeadingView.stringValue = [self titleForVideoGroup:videoGroup];
        return sectionHeadingView;

    } else if ([kind isEqualToString:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind]) {
        HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView * const mediaItemSupplementaryDetailView = [collectionView makeSupplementaryViewOfKind:kind withIdentifier:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind forIndexPath:indexPath];
        const id<HeimdallMediaLibraryItemProtocol> item = [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
        HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:item parentType:self.currentParentType];

        mediaItemSupplementaryDetailView.representedItem = representedItem;
        mediaItemSupplementaryDetailView.selectedItem = [collectionView itemAtIndexPath:indexPath];
        return mediaItemSupplementaryDetailView;
    }

    return nil;
}

- (NSString *)supplementaryDetailViewKind
{
    return HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind;
}

@end
