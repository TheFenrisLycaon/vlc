/*****************************************************************************
 * HeimdallLibrarySearchDataSource.m: MacOS X interface module
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

#import "HeimdallLibrarySearchDataSource.h"

#import "HeimdallLibrarySearchProvider.h"

#import "library/HeimdallLibraryCollectionViewFlowLayout.h"
#import "library/HeimdallLibraryCollectionViewItem.h"
#import "library/HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView.h"
#import "library/HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.h"
#import "library/HeimdallLibraryCollectionViewSupplementaryDetailView.h"
#import "library/HeimdallLibraryCollectionViewSupplementaryElementView.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryRepresentedItem.h"
#import "library/HeimdallLibraryWindow.h"

#import "library/audio-library/HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailView.h"

#import "extensions/NSPasteboardItem+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

NSString * const HeimdallLibrarySearchDataSourceDidReloadNotification = @"HeimdallLibrarySearchDataSourceDidReloadNotification";

#pragma mark - Flattened row model

/**
 * Represents one row in the flattened table view model.
 * A row is either a section header or a media item within a provider's results.
 */
@interface HeimdallLibrarySearchFlattenedRow : NSObject
@property (readonly) BOOL isHeader;
@property (readonly) NSUInteger providerIndex;
@property (readonly) NSInteger itemIndex; // -1 for header rows
+ (instancetype)headerForProvider:(NSUInteger)providerIndex;
+ (instancetype)itemAtIndex:(NSInteger)index forProvider:(NSUInteger)providerIndex;
@end

@implementation HeimdallLibrarySearchFlattenedRow

+ (instancetype)headerForProvider:(NSUInteger)providerIndex
{
    HeimdallLibrarySearchFlattenedRow * const row = [HeimdallLibrarySearchFlattenedRow new];
    row->_isHeader = YES;
    row->_providerIndex = providerIndex;
    row->_itemIndex = -1;
    return row;
}

+ (instancetype)itemAtIndex:(NSInteger)index forProvider:(NSUInteger)providerIndex
{
    HeimdallLibrarySearchFlattenedRow * const row = [HeimdallLibrarySearchFlattenedRow new];
    row->_isHeader = NO;
    row->_providerIndex = providerIndex;
    row->_itemIndex = index;
    return row;
}

@end

#pragma mark - Data source

@interface HeimdallLibrarySearchDataSource ()
{
    NSArray<HeimdallLibrarySearchProvider *> *_providers;
    NSArray<NSNumber *> *_visibleProviderIndices;
    NSArray<HeimdallLibrarySearchFlattenedRow *> *_flattenedRows;
    NSMutableDictionary<NSString *, HeimdallMediaLibraryDummyItem *> *_cachedProviderParentItems;
    NSUInteger _pendingProviderCount;
    dispatch_queue_t _rebuildQueue;
}
@end

@implementation HeimdallLibrarySearchDataSource

- (instancetype)init
{
    self = [super init];
    if (self) {
        _providers = [HeimdallLibrarySearchProvider defaultProviders];
        _visibleProviderIndices = @[];
        _flattenedRows = @[];
        _cachedProviderParentItems = [NSMutableDictionary dictionary];
        _rebuildQueue = dispatch_queue_create("searchDataSourceQueue", DISPATCH_QUEUE_SERIAL);
        [self connect];
    }
    return self;
}

- (void)connect
{
    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(providerResultsUpdated:)
                                               name:HeimdallLibrarySearchProviderResultsUpdated
                                             object:nil];
}

- (void)disconnect
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)providerResultsUpdated:(NSNotification *)notification
{
    if (_pendingProviderCount > 0) {
        _pendingProviderCount--;
    }
    if (_pendingProviderCount == 0) {
        _searching = NO;
    }
    if (self.viewMode == HeimdallLibraryGridViewModeSegment) {
        [self scheduleRebuildVisibleIndices];
    } else {
        [self scheduleRebuildFlattenedRows];
    }
}

- (void)scheduleRebuildVisibleIndices
{
    __weak typeof(self) weakSelf = self;
    dispatch_async(self->_rebuildQueue, ^{
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        NSArray<NSNumber *> * const newVisibleIndices = [strongSelf visibleProviderIndices];
        dispatch_async(dispatch_get_main_queue(), ^{
            if (strongSelf == nil) {
                return;
            }
            strongSelf->_visibleProviderIndices = newVisibleIndices;
            [strongSelf.collectionView reloadData];
            [strongSelf postDidReloadNotification];
        });
    });
}

- (void)scheduleRebuildFlattenedRows
{
    __weak typeof(self) weakSelf = self;
    dispatch_async(self->_rebuildQueue, ^{
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        NSArray<NSNumber *> * const newVisibleIndices = [strongSelf visibleProviderIndices];
        NSArray<HeimdallLibrarySearchFlattenedRow *> * const newFlattenedRows =
            [strongSelf flattenedRowsForVisibleIndices:newVisibleIndices];

        dispatch_async(dispatch_get_main_queue(), ^{
            if (strongSelf == nil) {
                return;
            }
            strongSelf->_visibleProviderIndices = newVisibleIndices;
            strongSelf->_flattenedRows = newFlattenedRows;
            [strongSelf->_cachedProviderParentItems removeAllObjects];
            [strongSelf.tableView reloadData];
            [strongSelf postDidReloadNotification];
        });
    });
}

- (void)postDidReloadNotification
{
    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallLibrarySearchDataSourceDidReloadNotification
                                                      object:self];
}

- (NSArray<NSNumber *> *)visibleProviderIndices
{
    NSMutableArray<NSNumber *> * const visible = [NSMutableArray array];
    for (NSUInteger i = 0; i < _providers.count; i++) {
        if (_providers[i].results.count > 0) {
            [visible addObject:@(i)];
        }
    }
    return [visible copy];
}

- (NSArray<HeimdallLibrarySearchFlattenedRow *> *)flattenedRowsForVisibleIndices:(NSArray<NSNumber *> *)visibleIndices
{
    NSMutableArray<HeimdallLibrarySearchFlattenedRow *> * const rows = [NSMutableArray array];
    for (NSNumber * const indexNumber in visibleIndices) {
        const NSUInteger providerIndex = indexNumber.unsignedIntegerValue;
        HeimdallLibrarySearchProvider * const provider = _providers[providerIndex];
        const NSUInteger resultCount = provider.results.count;
        [rows addObject:[HeimdallLibrarySearchFlattenedRow headerForProvider:providerIndex]];
        for (NSUInteger i = 0; i < resultCount; i++) {
            [rows addObject:[HeimdallLibrarySearchFlattenedRow itemAtIndex:i forProvider:providerIndex]];
        }
    }
    return [rows copy];
}

- (void)reloadData
{
    [_cachedProviderParentItems removeAllObjects];
    if (self.viewMode != HeimdallLibraryGridViewModeSegment) {
        [self scheduleRebuildFlattenedRows];
    } else {
        [self scheduleRebuildVisibleIndices];
    }
}

#pragma mark - Search

- (void)searchForString:(NSString *)string
{
    _pendingProviderCount = _providers.count;
    _searching = YES;
    for (HeimdallLibrarySearchProvider * const provider in _providers) {
        [provider searchForString:string];
    }
}

- (void)clearSearch
{
    _pendingProviderCount = 0;
    _searching = NO;
    for (HeimdallLibrarySearchProvider * const provider in _providers) {
        [provider clearSearch];
    }
    [self reloadData];
}

- (void)setViewMode:(HeimdallLibraryViewModeSegment)viewMode
{
    if (_viewMode == viewMode) {
        return;
    }
    _viewMode = viewMode;
    if (viewMode == HeimdallLibraryGridViewModeSegment) {
        [self scheduleRebuildVisibleIndices];
    } else {
        [self scheduleRebuildFlattenedRows];
    }
}

#pragma mark - Data management

- (HeimdallLibrarySearchProvider *)providerForVisibleSection:(NSInteger)visibleSection
{
    NSParameterAssert(visibleSection >= 0 && (NSUInteger)visibleSection < _visibleProviderIndices.count);
    const NSUInteger providerIndex = _visibleProviderIndices[visibleSection].unsignedIntegerValue;
    return _providers[providerIndex];
}

- (HeimdallMediaLibraryDummyItem *)dummyParentItemForProvider:(HeimdallLibrarySearchProvider *)provider
{
    HeimdallMediaLibraryDummyItem *cached = _cachedProviderParentItems[provider.displayTitle];
    if (cached != nil) {
        return cached;
    }

    NSMutableArray<HeimdallMediaLibraryMediaItem *> * const mediaItems = [NSMutableArray array];
    for (id<HeimdallMediaLibraryItemProtocol> item in provider.results) {
        [item iterateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem *mediaItem) {
            [mediaItems addObject:mediaItem];
        }];
    }
    cached = [[HeimdallMediaLibraryDummyItem alloc] initWithDisplayString:provider.displayTitle
                                                      withMediaItems:mediaItems];
    _cachedProviderParentItems[provider.displayTitle] = cached;
    return cached;
}

- (NSInteger)flattenedMediaItemOffsetForResultIndex:(NSInteger)resultIndex
                                         inProvider:(HeimdallLibrarySearchProvider *)provider
{
    NSInteger offset = 0;
    NSArray<id<HeimdallMediaLibraryItemProtocol>> * const results = provider.results;
    for (NSInteger i = 0; i < resultIndex && (NSUInteger)i < results.count; i++) {
        offset += results[i].mediaItems.count;
    }
    return offset;
}

- (HeimdallLibraryRepresentedItem *)representedItemForItem:(id<HeimdallMediaLibraryItemProtocol>)item
                                           atPosition:(NSInteger)position
                                         fromProvider:(HeimdallLibrarySearchProvider *)provider
{
    HeimdallMediaLibraryDummyItem * const parentItem = [self dummyParentItemForProvider:provider];
    const NSInteger flattenedPosition =
        [self flattenedMediaItemOffsetForResultIndex:position inProvider:provider];
    return [[HeimdallLibraryRepresentedItem alloc] initWithItem:item
                                               parentType:HeimdallMediaLibraryParentGroupTypeUnknown
                                               parentItem:parentItem
                                         positionInParent:flattenedPosition];
}

#pragma mark - HeimdallLibraryTableViewDataSource

- (HeimdallMediaLibraryParentGroupType)currentParentType
{
    return HeimdallMediaLibraryParentGroupTypeUnknown;
}

- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtRow:(NSInteger)row
                                       forTableView:(NSTableView *)tableView
{
    NSParameterAssert(row >= 0 && (NSUInteger)row < _flattenedRows.count);

    HeimdallLibrarySearchFlattenedRow * const flatRow = _flattenedRows[row];
    NSParameterAssert(!flatRow.isHeader || (flatRow.providerIndex < _providers.count));

    HeimdallLibrarySearchProvider * const provider = _providers[flatRow.providerIndex];
    return provider.results[flatRow.itemIndex];
}

- (HeimdallLibraryRepresentedItem *)representedItemAtRow:(NSInteger)row
                                       forTableView:(NSTableView *)tableView
{
    if ([self isHeaderRow:row]) {
        return [self representedItemForHeaderRow:row];
    }

    HeimdallLibrarySearchFlattenedRow * const flatRow = _flattenedRows[row];
    HeimdallLibrarySearchProvider * const provider = _providers[flatRow.providerIndex];
    id<HeimdallMediaLibraryItemProtocol> const item = provider.results[flatRow.itemIndex];

    return [self representedItemForItem:item atPosition:flatRow.itemIndex fromProvider:provider];
}

- (NSInteger)rowForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    NSParameterAssert(libraryItem != nil);

    const NSUInteger rowCount = _flattenedRows.count;
    for (NSUInteger i = 0; i < rowCount; i++) {
        HeimdallLibrarySearchFlattenedRow * const flatRow = _flattenedRows[i];
        if (flatRow.isHeader) {
            continue;
        }

        HeimdallLibrarySearchProvider * const provider = _providers[flatRow.providerIndex];
        if (flatRow.itemIndex >= 0 && (NSUInteger)flatRow.itemIndex < provider.results.count) {
            const id<HeimdallMediaLibraryItemProtocol> item = provider.results[flatRow.itemIndex];
            if (item.libraryID == libraryItem.libraryID) {
                return i;
            }
        }
    }

    return NSNotFound;
}

#pragma mark - HeimdallLibrarySectionedTableViewDataSource

- (BOOL)isHeaderRow:(NSInteger)row
{
    NSParameterAssert(row >= 0 && (NSUInteger)row < _flattenedRows.count);
    return _flattenedRows[row].isHeader;
}

- (NSString *)titleForRow:(NSInteger)row
{
    NSParameterAssert(row >= 0 && (NSUInteger)row < _flattenedRows.count);
    HeimdallLibrarySearchFlattenedRow * const flatRow = _flattenedRows[row];
    return _providers[flatRow.providerIndex].displayTitle;
}

- (HeimdallLibraryRepresentedItem *)representedItemForHeaderRow:(NSInteger)row
{
    NSParameterAssert([self isHeaderRow:row]);

    HeimdallLibrarySearchFlattenedRow * const flatRow = _flattenedRows[row];
    HeimdallLibrarySearchProvider * const provider = _providers[flatRow.providerIndex];
    NSParameterAssert(provider.results.count > 0);

    HeimdallMediaLibraryDummyItem * const groupItem = [self dummyParentItemForProvider:provider];
    return [[HeimdallLibraryRepresentedItem alloc] initWithItem:groupItem
                                                parentType:HeimdallMediaLibraryParentGroupTypeUnknown];
}

#pragma mark - NSTableViewDataSource

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

#pragma mark - NSCollectionViewDataSource

- (NSInteger)numberOfSectionsInCollectionView:(NSCollectionView *)collectionView
{
    return _visibleProviderIndices.count;
}

- (NSInteger)collectionView:(NSCollectionView *)collectionView
     numberOfItemsInSection:(NSInteger)section
{
    HeimdallLibrarySearchProvider * const provider = [self providerForVisibleSection:section];
    return provider ? provider.results.count : 0;
}

- (NSCollectionViewItem *)collectionView:(NSCollectionView *)collectionView
     itemForRepresentedObjectAtIndexPath:(NSIndexPath *)indexPath
{
    HeimdallLibraryCollectionViewItem * const viewItem =
        [collectionView makeItemWithIdentifier:HeimdallLibraryCollectionViewItemIdentifier forIndexPath:indexPath];
    const id<HeimdallMediaLibraryItemProtocol> item =
        [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
    HeimdallLibrarySearchProvider * const provider = [self providerForVisibleSection:indexPath.section];
    HeimdallLibraryRepresentedItem * const representedItem =
        [self representedItemForItem:item atPosition:indexPath.item fromProvider:provider];
    viewItem.representedItem = representedItem;
    return viewItem;
}

- (NSView *)collectionView:(NSCollectionView *)collectionView
viewForSupplementaryElementOfKind:(NSCollectionViewSupplementaryElementKind)kind
               atIndexPath:(NSIndexPath *)indexPath
{
    if ([kind isEqualToString:NSCollectionElementKindSectionHeader]) {
        HeimdallLibraryCollectionViewSupplementaryElementView * const sectionHeadingView =
            [collectionView makeSupplementaryViewOfKind:kind
                                         withIdentifier:HeimdallLibrarySupplementaryElementViewIdentifier
                                           forIndexPath:indexPath];
        HeimdallLibrarySearchProvider * const provider = [self providerForVisibleSection:indexPath.section];
        sectionHeadingView.stringValue = provider.displayTitle;
        return sectionHeadingView;

    } else if ([kind isEqualToString:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind] ||
               [kind isEqualToString:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind] ||
               [kind isEqualToString:HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailViewKind]) {
        HeimdallLibraryCollectionViewSupplementaryDetailView * const detailView =
            (HeimdallLibraryCollectionViewSupplementaryDetailView *)
            [collectionView makeSupplementaryViewOfKind:kind
                                         withIdentifier:kind
                                           forIndexPath:indexPath];
        const id<HeimdallMediaLibraryItemProtocol> item =
            [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
        HeimdallLibrarySearchProvider * const provider = [self providerForVisibleSection:indexPath.section];
        HeimdallLibraryRepresentedItem * const representedItem =
            [self representedItemForItem:item atPosition:indexPath.item fromProvider:provider];
        detailView.representedItem = representedItem;
        detailView.selectedItem = [collectionView itemAtIndexPath:indexPath];
        return detailView;
    }

    return nil;
}

#pragma mark - HeimdallLibraryCollectionViewDataSource

- (NSString *)supplementaryDetailViewKind
{
    if (self.collectionView == nil) {
        return HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind;
    }

    NSIndexPath * const selectedIndexPath = self.collectionView.selectionIndexPaths.anyObject;
    if (selectedIndexPath == nil) {
        return HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind;
    }

    const id<HeimdallMediaLibraryItemProtocol> item =
        [self libraryItemAtIndexPath:selectedIndexPath forCollectionView:self.collectionView];
    if ([item isKindOfClass:HeimdallMediaLibraryAlbum.class] ||
        [item isKindOfClass:HeimdallMediaLibraryGroup.class]) {
        return HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind;
    } else if ([item conformsToProtocol:@protocol(HeimdallMediaLibraryAudioGroupProtocol)]) {
        return HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailViewKind;
    }
    return HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind;
}

- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtIndexPath:(NSIndexPath *)indexPath
                                        forCollectionView:(NSCollectionView *)collectionView
{
    HeimdallLibrarySearchProvider * const provider = [self providerForVisibleSection:indexPath.section];
    NSParameterAssert(provider != nil && indexPath.item >= 0 && (NSUInteger)indexPath.item < provider.results.count);
    return provider.results[indexPath.item];
}

- (NSIndexPath *)indexPathForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    NSParameterAssert(libraryItem != nil);

    for (NSUInteger visibleIndex = 0; visibleIndex < _visibleProviderIndices.count; visibleIndex++) {
        const NSUInteger providerIndex = _visibleProviderIndices[visibleIndex].unsignedIntegerValue;
        HeimdallLibrarySearchProvider * const provider = _providers[providerIndex];
        for (NSUInteger i = 0; i < provider.results.count; i++) {
            if (provider.results[i].libraryID == libraryItem.libraryID) {
                return [NSIndexPath indexPathForItem:i inSection:visibleIndex];
            }
        }
    }

    return nil;
}

- (NSArray<HeimdallLibraryRepresentedItem *> *)representedItemsAtIndexPaths:(NSSet<NSIndexPath *> *)indexPaths
                                                     forCollectionView:(NSCollectionView *)collectionView
{
    NSMutableArray<HeimdallLibraryRepresentedItem *> * const representedItems =
        [NSMutableArray arrayWithCapacity:indexPaths.count];

    for (NSIndexPath * const indexPath in indexPaths) {
        const id<HeimdallMediaLibraryItemProtocol> libraryItem =
            [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
        if (libraryItem) {
            HeimdallLibrarySearchProvider * const provider = [self providerForVisibleSection:indexPath.section];
            HeimdallLibraryRepresentedItem * const representedItem =
                [self representedItemForItem:libraryItem atPosition:indexPath.item fromProvider:provider];
            [representedItems addObject:representedItem];
        }
    }

    return representedItems;
}

@end
