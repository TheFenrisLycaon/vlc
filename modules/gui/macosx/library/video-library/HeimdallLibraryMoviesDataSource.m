/*****************************************************************************
 * HeimdallLibraryMoviesDataSource.m: MacOS X interface module
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

#import "HeimdallLibraryMoviesDataSource.h"
#import "library/HeimdallLibraryCollectionViewItem.h"
#import "library/HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibraryRepresentedItem.h"

NSString * const HeimdallLibraryMoviesDataSourceDisplayedCollectionChangedNotification = @"HeimdallLibraryMoviesDataSourceDisplayedCollectionChangedNotification";

@implementation HeimdallLibraryMoviesDataSource

- (instancetype)init
{
    self = [super init];
    if (self) {
        [self connect];
    }
    return self;
}

- (void)connect
{
    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelMoviesListReset:)
                               name:HeimdallLibraryModelListOfMoviesReset
                             object:nil];
    [self reloadData];
}

- (void)disconnect
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)libraryModelMoviesListReset:(NSNotification *)notification
{
    [self reloadData];
}

#pragma mark - NSCollectionViewDataSource

- (NSInteger)numberOfSectionsInCollectionView:(NSCollectionView *)collectionView
{
    return 1;
}

- (NSInteger)collectionView:(NSCollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
    return self.libraryModel.numberOfMovies;
}

- (NSCollectionViewItem *)collectionView:(NSCollectionView *)collectionView itemForRepresentedObjectAtIndexPath:(NSIndexPath *)indexPath
{
    HeimdallLibraryCollectionViewItem * const item = [collectionView makeItemWithIdentifier:HeimdallLibraryCollectionViewItemIdentifier forIndexPath:indexPath];
    const id<HeimdallMediaLibraryItemProtocol> movie = [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
    HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:movie parentType:self.currentParentType];
    item.representedItem = representedItem;
    return item;
}

- (NSView *)collectionView:(NSCollectionView *)collectionView viewForSupplementaryElementOfKind:(NSString *)kind atIndexPath:(NSIndexPath *)indexPath
{
    if ([kind isEqualToString:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind]) {
        HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView * const detailView =
            [collectionView makeSupplementaryViewOfKind:kind
                                         withIdentifier:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind
                                           forIndexPath:indexPath];
        const id<HeimdallMediaLibraryItemProtocol> movie = [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
        HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:movie parentType:self.currentParentType];
        detailView.representedItem = representedItem;
        detailView.selectedItem = [collectionView itemAtIndexPath:indexPath];
        return detailView;
    }
    return nil;
}

#pragma mark - HeimdallLibraryCollectionViewDataSource

- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtIndexPath:(NSIndexPath *)indexPath forCollectionView:(NSCollectionView *)collectionView
{
    return self.libraryModel.listOfMovies[indexPath.item];
}

- (NSIndexPath *)indexPathForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    const NSUInteger idx = [self.libraryModel.listOfMovies indexOfObject:libraryItem];
    if (idx == NSNotFound) return nil;
    return [NSIndexPath indexPathForItem:idx inSection:0];
}

- (NSArray<HeimdallLibraryRepresentedItem *> *)representedItemsAtIndexPaths:(NSSet<NSIndexPath *> *const)indexPaths forCollectionView:(NSCollectionView *)collectionView
{
    NSMutableArray * const items = [NSMutableArray arrayWithCapacity:indexPaths.count];
    for (NSIndexPath * const indexPath in indexPaths) {
        const id<HeimdallMediaLibraryItemProtocol> item = [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
        if (item) {
            [items addObject:[[HeimdallLibraryRepresentedItem alloc] initWithItem:item parentType:self.currentParentType]];
        }
    }
    return items;
}

- (NSString *)supplementaryDetailViewKind
{
    return HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind;
}

- (void)reloadData
{
    // Notify all attached views to reload their data
    if (self.collectionView) {
        [self.collectionView reloadData];
    }
    if (self.tableView) {
        [self.tableView reloadData];
    }
    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallLibraryMoviesDataSourceDisplayedCollectionChangedNotification
                                                      object:self
                                                    userInfo:nil];
}

#pragma mark - HeimdallLibraryTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    return self.libraryModel.numberOfMovies;
}

- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtRow:(NSInteger)row forTableView:(nullable NSTableView *)tableView
{
    return self.libraryModel.listOfMovies[row];
}

- (NSInteger)rowForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    return [self.libraryModel.listOfMovies indexOfObject:libraryItem];
}

- (HeimdallMediaLibraryParentGroupType)currentParentType
{
    return HeimdallMediaLibraryParentGroupTypeVideoLibrary;
}

@end
