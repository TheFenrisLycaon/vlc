/*****************************************************************************
 * HeimdallLibraryPlaylistDataSource.m: MacOS X interface module
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

#import "HeimdallLibraryPlaylistDataSource.h"

#import "extensions/NSPasteboardItem+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

#import "library/HeimdallLibraryCollectionViewFlowLayout.h"
#import "library/HeimdallLibraryCollectionViewItem.h"
#import "library/HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView.h"
#import "library/HeimdallLibraryCollectionViewSupplementaryElementView.h"
#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryMasterDetailViewTableViewDelegate.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibraryRepresentedItem.h"

#import "main/HeimdallMain.h"

typedef NS_ENUM(NSInteger, HeimdallLibraryDataSourceCacheAction) {
    HeimdallLibraryDataSourceCacheUpdateAction,
    HeimdallLibraryDataSourceCacheDeleteAction,
};

@interface HeimdallLibraryPlaylistDataSource () {
    dispatch_queue_t _playlistQueue;
    NSArray<HeimdallMediaLibraryPlaylist *> *_playlists;
}

@end

@implementation HeimdallLibraryPlaylistDataSource

@synthesize headerDelegate;

- (instancetype)init
{
    self = [super init];
    if (self) {
        [self setup];
    }
    return self;
}

- (void)setup
{
    _playlistQueue = dispatch_queue_create("com.lycaria.heimdall.libraryplaylistdatasource.queue", DISPATCH_QUEUE_CONCURRENT);
    _libraryModel = HeimdallMain.sharedInstance.libraryController.libraryModel;
    [self connect];
    [self reloadData];
}

- (NSArray<HeimdallMediaLibraryPlaylist *> *)playlists
{
    __block NSArray<HeimdallMediaLibraryPlaylist *> *playlists;
    dispatch_sync(_playlistQueue, ^{
        playlists = self->_playlists;
    });
    return playlists;
}

- (void)setPlaylists:(NSArray<HeimdallMediaLibraryPlaylist *> *)playlists
{
    dispatch_barrier_async(_playlistQueue, ^{
        self->_playlists = [playlists copy];
    });
}

- (void)connect
{
    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(playlistsReset:)
                               name:HeimdallLibraryModelPlaylistAdded
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(playlistUpdated:)
                               name:HeimdallLibraryModelPlaylistUpdated
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(playlistDeleted:)
                               name:HeimdallLibraryModelPlaylistDeleted
                             object:nil];
}

- (void)disconnect
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)playlistsReset:(NSNotification *)notification
{
    NSParameterAssert(notification);
    [self reloadData];
}

- (void)playlistUpdated:(NSNotification *)notification
{
    NSParameterAssert(notification);
    HeimdallMediaLibraryPlaylist * const playlist = (HeimdallMediaLibraryPlaylist *)notification.object;
    [self applyCacheAction:HeimdallLibraryDataSourceCacheUpdateAction
            withPlaylistID:playlist.libraryID
                  playlist:playlist];
}

- (void)playlistDeleted:(NSNotification *)notification
{
    NSParameterAssert(notification);
    NSParameterAssert((NSNumber *)notification.object != nil);

    const int64_t playlistId = [(NSNumber *)notification.object longLongValue];
    [self applyCacheAction:HeimdallLibraryDataSourceCacheDeleteAction
            withPlaylistID:playlistId
                  playlist:nil];
}

- (void)reloadData
{
    NSArray<HeimdallMediaLibraryPlaylist *> * const listOfPlaylists =
        [self.libraryModel listOfPlaylistsOfType:self.playlistType];
    self.playlists = listOfPlaylists ?: @[];
    [self reloadViews];
    [self updateHeaderInTableView:self.detailTableView forMasterSelection:self.masterTableView];
}

- (void)reloadViews
{
    [self.masterTableView reloadData];
    [self.detailTableView reloadData];
    
    for (NSCollectionView * const collectionView in self.collectionViews) {
        [(HeimdallLibraryCollectionViewFlowLayout *)collectionView.collectionViewLayout resetLayout];
        [collectionView reloadData];
    }
}

- (void)reloadViewsAtIndex:(NSUInteger)index
          dueToCacheAction:(HeimdallLibraryDataSourceCacheAction)action
{
    NSTableView * const masterTableView = self.masterTableView;
    NSTableView * const detailTableView = self.detailTableView;
    NSIndexSet * const indexSet = [NSIndexSet indexSetWithIndex:index];
    const NSInteger selectedMasterRow = masterTableView.selectedRow;
    const BOOL affectsSelectedDetail =
        detailTableView != nil && (NSInteger)index == selectedMasterRow;

    switch (action) {
        case HeimdallLibraryDataSourceCacheUpdateAction: {
            NSIndexSet * const columnSet =
                [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, masterTableView.numberOfColumns)];
            [masterTableView reloadDataForRowIndexes:indexSet columnIndexes:columnSet];
            break;
        }
        case HeimdallLibraryDataSourceCacheDeleteAction:
            [masterTableView removeRowsAtIndexes:indexSet
                                   withAnimation:NSTableViewAnimationEffectNone];
            break;
        default:
            NSAssert(false, @"Invalid playlist cache action");
    }

    if (affectsSelectedDetail) {
        [detailTableView reloadData];
        [self updateHeaderInTableView:detailTableView forMasterSelection:masterTableView];
    }

    NSIndexPath * const indexPath = [NSIndexPath indexPathForItem:index inSection:0];
    NSSet<NSIndexPath *> * const indexPathSet = [NSSet setWithObject:indexPath];

    for (NSCollectionView * const collectionView in self.collectionViews) {
        switch (action) {
            case HeimdallLibraryDataSourceCacheUpdateAction:
                [collectionView reloadItemsAtIndexPaths:indexPathSet];
                break;
            case HeimdallLibraryDataSourceCacheDeleteAction:
                [(HeimdallLibraryCollectionViewFlowLayout *)collectionView.collectionViewLayout resetLayout];
                [collectionView deleteItemsAtIndexPaths:indexPathSet];
                break;
            default:
                NSAssert(false, @"Invalid playlist cache action");
        }
    }
}

- (void)applyCacheAction:(HeimdallLibraryDataSourceCacheAction)action
          withPlaylistID:(const int64_t)playlistID
                playlist:(HeimdallMediaLibraryPlaylist * _Nullable const)playlist
{
    dispatch_barrier_async(_playlistQueue, ^{
        NSMutableArray *mutablePlaylists = [self->_playlists mutableCopy];
        if (mutablePlaylists == nil) {
            return;
        }

        const NSUInteger idx =
            [mutablePlaylists indexOfObjectPassingTest:^BOOL(const HeimdallMediaLibraryPlaylist *item, 
                                                             const NSUInteger __unused index,
                                                             BOOL * const __unused stop) {
            return item.libraryID == playlistID;
        }];

        if (idx == NSNotFound) {
            return;
        }

        switch (action) {
            case HeimdallLibraryDataSourceCacheUpdateAction:
                NSAssert(playlist != nil, @"Playlist must not be nil for update action");
                if (playlist != nil) {
                    [mutablePlaylists replaceObjectAtIndex:idx withObject:playlist];
                }
                break;
            case HeimdallLibraryDataSourceCacheDeleteAction:
                [mutablePlaylists removeObjectAtIndex:idx];
                break;
            default:
                NSAssert(false, @"Invalid playlist cache action");
        }
        self->_playlists = [mutablePlaylists copy];

        dispatch_async(dispatch_get_main_queue(), ^{
            [self reloadViewsAtIndex:idx dueToCacheAction:action];
        });
    });
}

#pragma mark - table view data source

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    if (tableView == self.masterTableView) {
        return self.playlists.count;
    }

    const NSInteger selectedMasterRow = self.masterTableView.selectedRow;
    if (selectedMasterRow > -1) {
        const id<HeimdallMediaLibraryItemProtocol> item = self.playlists[selectedMasterRow];
        return item.mediaItems.count;
    }

    return 0;
}

- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtRow:(NSInteger)row
                                       forTableView:(NSTableView *)tableView
{
    if (tableView == self.masterTableView) {
        return self.playlists[row];
    }

    const NSInteger selectedMasterRow = self.masterTableView.selectedRow;
    if (tableView == self.detailTableView && selectedMasterRow > -1) {
        const id<HeimdallMediaLibraryItemProtocol> item = self.playlists[selectedMasterRow];
        return item.mediaItems[row];
    }

    return nil;
}

- (id<NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row
{
    const id<HeimdallMediaLibraryItemProtocol> libraryItem = [self libraryItemAtRow:row forTableView:tableView];
    return [NSPasteboardItem pasteboardItemWithLibraryItem:libraryItem];
}

- (NSInteger)rowForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if (libraryItem == nil) {
        return NSNotFound;
    }
    return [self.playlists indexOfObjectPassingTest:^BOOL(const HeimdallMediaLibraryPlaylist *playlist, const NSUInteger __unused idx, BOOL * const __unused stop) {
        return playlist.libraryID == libraryItem.libraryID;
    }];
}

#pragma mark - collection view data source

- (void)setCollectionViews:(NSArray<NSCollectionView *> *)collectionViews
{
    _collectionViews = collectionViews;
    for (NSCollectionView * const collectionView in self.collectionViews) {
        [self setupCollectionView:collectionView];
    }
}

- (void)setupCollectionView:(NSCollectionView *)collectionView
{
    [collectionView registerClass:HeimdallLibraryCollectionViewItem.class
            forItemWithIdentifier:HeimdallLibraryCollectionViewItemIdentifier];
    [collectionView registerClass:HeimdallLibraryCollectionViewSupplementaryElementView.class
       forSupplementaryViewOfKind:NSCollectionElementKindSectionHeader
                   withIdentifier:HeimdallLibrarySupplementaryElementViewIdentifier];

    NSNib * const supplementaryDetailView =
        [[NSNib alloc] initWithNibNamed:@"HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView" bundle:nil];
    [collectionView registerNib:supplementaryDetailView
      forSupplementaryViewOfKind:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind
                  withIdentifier:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewIdentifier];

    NSCollectionViewFlowLayout * const layout = collectionView.collectionViewLayout;
    layout.headerReferenceSize = HeimdallLibraryCollectionViewSupplementaryElementView.defaultHeaderSize;

    collectionView.dataSource = self;
    [collectionView reloadData];
}

- (NSInteger)collectionView:(NSCollectionView *)collectionView
     numberOfItemsInSection:(NSInteger)section
{
    return self.playlists.count;
}

- (NSInteger)numberOfSectionsInCollectionView:(NSCollectionView *)collectionView
{
    return 1;
}

- (NSCollectionViewItem *)collectionView:(NSCollectionView *)collectionView
     itemForRepresentedObjectAtIndexPath:(NSIndexPath *)indexPath
{
    HeimdallLibraryCollectionViewItem * const viewItem =
        [collectionView makeItemWithIdentifier:HeimdallLibraryCollectionViewItemIdentifier
                                  forIndexPath:indexPath];
    const id<HeimdallMediaLibraryItemProtocol> libraryItem = self.playlists[indexPath.item];
    // NOTE: Unknown parent type represented items default to playing the represented item only.
    // We want this behaviour as it feels unnatural to handle any parent types for playlists
    HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:libraryItem
                                                                                             parentType:HeimdallMediaLibraryParentGroupTypeUnknown];
    viewItem.representedItem = representedItem;
    return viewItem;
}

- (NSView *)collectionView:(NSCollectionView *)collectionView
viewForSupplementaryElementOfKind:(NSCollectionViewSupplementaryElementKind)kind
               atIndexPath:(NSIndexPath *)indexPath
{
    if([kind isEqualToString:NSCollectionElementKindSectionHeader]) {
        HeimdallLibraryCollectionViewSupplementaryElementView * const sectionHeadingView = [collectionView makeSupplementaryViewOfKind:kind withIdentifier:HeimdallLibrarySupplementaryElementViewIdentifier forIndexPath:indexPath];

        sectionHeadingView.stringValue = _NS("Playlists");
        return sectionHeadingView;

    } else if ([kind isEqualToString:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind]) {
        NSString * const supplementaryDetailViewIdentifier =
            HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewIdentifier;
        HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView * const supplementaryDetailView =
            [collectionView makeSupplementaryViewOfKind:kind
                                         withIdentifier:supplementaryDetailViewIdentifier
                                           forIndexPath:indexPath];
        const id<HeimdallMediaLibraryItemProtocol> item =
            [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
        HeimdallLibraryRepresentedItem * const representedItem =
            [[HeimdallLibraryRepresentedItem alloc] initWithItem:item parentType:self.currentParentType];
        supplementaryDetailView.representedItem = representedItem;
        supplementaryDetailView.selectedItem = [collectionView itemAtIndexPath:indexPath];
        return supplementaryDetailView;
    }

    return nil;
}

- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtIndexPath:(NSIndexPath *)indexPath
                                        forCollectionView:(NSCollectionView *)collectionView
{
    const NSUInteger indexPathItem = indexPath.item;

    if (indexPathItem < 0 || indexPathItem >= self.playlists.count) {
        return nil;
    }

    return self.playlists[indexPathItem];
}

- (NSIndexPath *)indexPathForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    const NSUInteger idx = [self.playlists indexOfObject:libraryItem];
    if (idx == NSNotFound) {
        return nil;
    }

    return [NSIndexPath indexPathForItem:idx inSection:0];
}

- (NSArray<HeimdallLibraryRepresentedItem *> *)representedItemsAtIndexPaths:(NSSet<NSIndexPath *> *const)indexPaths
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

- (HeimdallMediaLibraryParentGroupType)currentParentType
{
    return HeimdallMediaLibraryParentGroupTypePlaylist;
}

- (id<HeimdallMediaLibraryItemProtocol>)parentItemForTableView:(NSTableView *)tableView
{
    if (tableView != self.detailTableView) {
        return nil;
    }

    const NSInteger selectedRow = self.masterTableView.selectedRow;
    if (selectedRow < 0 || (NSUInteger)selectedRow >= self.playlists.count) {
        return nil;
    }

    return self.playlists[selectedRow];
}

- (NSString *)supplementaryDetailViewKind
{
    return HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind;
}

- (void)setPlaylistType:(heimdall_ml_playlist_type_t)playlistType
{
    if (self.playlistType == playlistType) {
        return;
    }

    _playlistType = playlistType;
    [self reloadData];
}

- (void)updateHeaderInTableView:(NSTableView *)detailTableView forMasterSelection:(NSTableView *)masterTableView
{
    if (self.headerDelegate == nil) {
        return;
    }

    const NSInteger selectedRow = masterTableView.selectedRow;
    if (selectedRow < 0 || (NSUInteger)selectedRow >= self.playlists.count) {
        [self.headerDelegate updateHeaderForTableView:detailTableView
                                  withRepresentedItem:nil
                                        fallbackTitle:_NS("Playlists")
                                       fallbackDetail:_NS("Select a playlist")];
        return;
    }

    const HeimdallMediaLibraryPlaylist * const playlist = self.playlists[selectedRow];
    HeimdallLibraryRepresentedItem * const representedItem =
        [[HeimdallLibraryRepresentedItem alloc] initWithItem:playlist
                                             parentType:self.currentParentType];

    [self.headerDelegate updateHeaderForTableView:detailTableView
                              withRepresentedItem:representedItem
                                    fallbackTitle:playlist.primaryDetailString
                                   fallbackDetail:playlist.secondaryDetailString];
}

@end
