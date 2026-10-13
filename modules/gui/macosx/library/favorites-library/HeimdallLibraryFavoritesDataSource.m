/*****************************************************************************
 * HeimdallLibraryFavoritesDataSource.m: MacOS X interface module
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

#import "HeimdallLibraryFavoritesDataSource.h"

#import "library/HeimdallLibraryCollectionViewFlowLayout.h"
#import "library/HeimdallLibraryCollectionViewItem.h"
#import "library/HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.h"
#import "library/HeimdallLibraryCollectionViewSupplementaryElementView.h"
#import "library/HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryRepresentedItem.h"
#import "library/HeimdallLibraryTableCellView.h"
#import "library/HeimdallLibraryGroupHeaderDelegate.h"
#import "library/audio-library/HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailView.h"

#import "views/HeimdallImageView.h"

#import "main/CompatibilityFixes.h"
#import "main/HeimdallMain.h"

#import "extensions/NSIndexSet+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"
#import "extensions/NSPasteboardItem+HeimdallAdditions.h"

NSString * const HeimdallLibraryFavoritesDataSourceDisplayedCollectionChangedNotification = @"HeimdallLibraryFavoritesDataSourceDisplayedCollectionChangedNotification";

@interface HeimdallLibraryFavoritesDataSource ()
{
    NSArray<HeimdallMediaLibraryMediaItem *> *_allFavoritesArray;
    NSArray<HeimdallMediaLibraryMediaItem *> *_favoriteVideoMediaArray;
    NSArray<HeimdallMediaLibraryMediaItem *> *_favoriteAudioMediaArray;
    NSArray<HeimdallMediaLibraryAlbum *> *_favoriteAlbumsArray;
    NSArray<HeimdallMediaLibraryArtist *> *_favoriteArtistsArray;
    NSArray<HeimdallMediaLibraryGenre *> *_favoriteGenresArray;
    HeimdallLibraryCollectionViewFlowLayout *_collectionViewFlowLayout;
    NSArray<NSNumber *> *_visibleSectionMapping; // Maps visible sections to HeimdallLibraryFavoritesSection values
    NSMutableDictionary<NSNumber *, NSArray<id<HeimdallMediaLibraryItemProtocol>> *> *_flattenedRowMappings; // Maps row indices to items for flattened artist/genre views
}

@end

@implementation HeimdallLibraryFavoritesDataSource

- (instancetype)init
{
    self = [super init];
    if (self) {
        _flattenedRowMappings = [NSMutableDictionary dictionary];
        [self connect];
    }
    return self;
}

- (NSArray<id<HeimdallMediaLibraryItemProtocol>> *)arrayForSection:(HeimdallLibraryFavoritesSection)section
{
    switch (section) {
        case HeimdallLibraryFavoritesSectionAllFavorites:
            return _allFavoritesArray;
        case HeimdallLibraryFavoritesSectionVideoMedia:
            return _favoriteVideoMediaArray;
        case HeimdallLibraryFavoritesSectionAudioMedia:
            return _favoriteAudioMediaArray;
        case HeimdallLibraryFavoritesSectionAlbums:
            return _favoriteAlbumsArray;
        case HeimdallLibraryFavoritesSectionArtists:
            return _favoriteArtistsArray;
        case HeimdallLibraryFavoritesSectionGenres:
            return _favoriteGenresArray;
        default:
            return @[];
    }
}

- (NSString *)titleForSection:(HeimdallLibraryFavoritesSection)section
{
    switch (section) {
        case HeimdallLibraryFavoritesSectionAllFavorites:
            return _NS("All Favorites");
        case HeimdallLibraryFavoritesSectionVideoMedia:
            return _NS("Favorite Videos");
        case HeimdallLibraryFavoritesSectionAudioMedia:
            return _NS("Favorite Music");
        case HeimdallLibraryFavoritesSectionAlbums:
            return _NS("Favorite Albums");
        case HeimdallLibraryFavoritesSectionArtists:
            return _NS("Favorite Artists");
        case HeimdallLibraryFavoritesSectionGenres:
            return _NS("Favorite Genres");
        default:
            return @"";
    }
}

- (HeimdallMediaLibraryParentGroupType)parentTypeForSection:(HeimdallLibraryFavoritesSection)section
{
    switch (section) {
        case HeimdallLibraryFavoritesSectionAllFavorites:
            return HeimdallMediaLibraryParentGroupTypeAllFavorites;
        case HeimdallLibraryFavoritesSectionVideoMedia:
            return HeimdallMediaLibraryParentGroupTypeVideoLibrary;
        case HeimdallLibraryFavoritesSectionAudioMedia:
            return HeimdallMediaLibraryParentGroupTypeAudioLibrary;
        case HeimdallLibraryFavoritesSectionAlbums:
            return HeimdallMediaLibraryParentGroupTypeAlbum;
        case HeimdallLibraryFavoritesSectionArtists:
            return HeimdallMediaLibraryParentGroupTypeArtist;
        case HeimdallLibraryFavoritesSectionGenres:
            return HeimdallMediaLibraryParentGroupTypeGenre;
        default:
            return HeimdallMediaLibraryParentGroupTypeUnknown;
    }
}

- (HeimdallLibraryFavoritesSection)sectionForVisibleIndex:(NSInteger)visibleIndex
{
    if (visibleIndex < 0 || (NSUInteger)visibleIndex >= _visibleSectionMapping.count) {
        return HeimdallLibraryFavoritesSectionCount; // Invalid
    }
    return (HeimdallLibraryFavoritesSection)[_visibleSectionMapping[visibleIndex] integerValue];
}

- (BOOL)isAudioGroupSection:(HeimdallLibraryFavoritesSection)section
{
    // Only artists and genres show the audio group view (list of albums)
    return section == HeimdallLibraryFavoritesSectionArtists ||
           section == HeimdallLibraryFavoritesSectionGenres;
}

- (BOOL)isMediaListSection:(HeimdallLibraryFavoritesSection)section
{
    // Albums show the media list view (list of tracks)
    return section == HeimdallLibraryFavoritesSectionAlbums;
}

- (NSInteger)visibleIndexForSection:(HeimdallLibraryFavoritesSection)section
{
    NSNumber * const sectionNumber = @(section);
    NSUInteger index = [_visibleSectionMapping indexOfObject:sectionNumber];
    return index == NSNotFound ? -1 : (NSInteger)index;
}

- (void)updateVisibleSectionMapping
{
    NSMutableArray<NSNumber *> * const visibleSections = [NSMutableArray arrayWithCapacity:HeimdallLibraryFavoritesSectionCount];
    for (NSUInteger i = 1; i < HeimdallLibraryFavoritesSectionCount; i++) {
        NSArray * const sectionArray = [self arrayForSection:i];
        if (sectionArray.count > 0) {
            [visibleSections addObject:@(i)];
        }
    }
    if (visibleSections.count > 1) {
        [visibleSections insertObject:@(HeimdallLibraryFavoritesSectionAllFavorites) atIndex:0];
    }

    _visibleSectionMapping = [visibleSections copy];
}

- (NSUInteger)indexOfMediaItem:(const int64_t)libraryId inArray:(NSArray const *)array
{
    return [array indexOfObjectPassingTest:^BOOL(id<HeimdallMediaLibraryItemProtocol> const findMediaItem, const NSUInteger __unused idx, BOOL * const __unused stop) {
        NSAssert(findMediaItem != nil, @"Collection should not contain nil media items");
        return findMediaItem.libraryID == libraryId;
    }];
}

- (id<HeimdallMediaLibraryItemProtocol>)createGroupDescriptorForSection:(HeimdallLibraryFavoritesSection)section
{
    NSArray * const sectionArray = [self arrayForSection:section];
    
    // For video and audio media sections, the array already contains HeimdallMediaLibraryMediaItem objects
    if (section == HeimdallLibraryFavoritesSectionVideoMedia || 
        section == HeimdallLibraryFavoritesSectionAudioMedia) {
        return [[HeimdallMediaLibraryDummyItem alloc] initWithDisplayString:[self titleForSection:section]
                                                        withMediaItems:sectionArray];
    }
    
    // For albums, artists, and genres, we need to extract media items from each item
    NSMutableArray<HeimdallMediaLibraryMediaItem *> * const mediaItems = [NSMutableArray array];
    for (id<HeimdallMediaLibraryItemProtocol> item in sectionArray) {
        NSArray<HeimdallMediaLibraryMediaItem *> * const itemMediaItems = item.mediaItems;
        if (itemMediaItems) {
            [mediaItems addObjectsFromArray:itemMediaItems];
        }
    }
    
    return [[HeimdallMediaLibraryDummyItem alloc] initWithDisplayString:[self titleForSection:section]
                                                    withMediaItems:[mediaItems copy]];
}

- (NSArray *)buildFlattenedArrayForAudioGroupSection:(HeimdallLibraryFavoritesSection)section
{
    NSParameterAssert([self isAudioGroupSection:section]);
    NSArray<id<HeimdallMediaLibraryAudioGroupProtocol>> * const sectionArray = 
        (NSArray<id<HeimdallMediaLibraryAudioGroupProtocol>> *)[self arrayForSection:section];
    NSMutableArray * const flattenedArray = [NSMutableArray array];
    
    for (id<HeimdallMediaLibraryAudioGroupProtocol> audioGroup in sectionArray) {
        [flattenedArray addObject:audioGroup];
        [flattenedArray addObjectsFromArray:audioGroup.albums];
    }
    
    return [flattenedArray copy];
}

#pragma mark - Notification handlers

- (void)libraryModelFavoriteListReset:(NSNotification * const)notification
{
    [self reloadData];
}

#pragma mark - HeimdallLibraryDataSource

- (void)connect
{
    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;

    [notificationCenter addObserver:self
                           selector:@selector(libraryModelFavoriteListReset:)
                               name:HeimdallLibraryModelFavoriteVideoMediaListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelFavoriteListReset:)
                               name:HeimdallLibraryModelAllCachesDropped
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelFavoriteListReset:)
                               name:HeimdallLibraryModelFavoriteAudioMediaListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelFavoriteListReset:)
                               name:HeimdallLibraryModelFavoriteAlbumsListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelFavoriteListReset:)
                               name:HeimdallLibraryModelFavoriteArtistsListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelFavoriteListReset:)
                               name:HeimdallLibraryModelFavoriteGenresListReset
                             object:nil];

    [self reloadData];
}

- (void)disconnect
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)reloadData
{
    if (!_libraryModel) {
        return;
    }

    [_collectionViewFlowLayout resetLayout];
    NSSet<NSIndexPath *> * const selectedIndexPaths = self.collectionView.selectionIndexPaths;
    if (selectedIndexPaths.count > 0) {
        [self.collectionView deselectItemsAtIndexPaths:selectedIndexPaths];
    }

    _favoriteVideoMediaArray = [self.libraryModel listOfFavoriteVideoMedia];
    _favoriteAudioMediaArray = [self.libraryModel listOfFavoriteAudioMedia];
    _favoriteAlbumsArray = [self.libraryModel listOfFavoriteAlbums];
    _favoriteArtistsArray = [self.libraryModel listOfFavoriteArtists];
    _favoriteGenresArray = [self.libraryModel listOfFavoriteGenres];
    _allFavoritesArray = [self.libraryModel listOfMediaItemsForParentType:HeimdallMediaLibraryParentGroupTypeAllFavorites];

    [self updateVisibleSectionMapping];

    [_flattenedRowMappings removeAllObjects];

    [self updateHeaderForMasterSelection];

    const NSInteger selectedRow = self.masterTableView.selectedRow;
    NSAssert(self.masterTableView == nil || self.masterTableView.dataSource == self, @"Cannot reload a table view with a different data source");
    [self.masterTableView reloadData];
    if (selectedRow != -1 && selectedRow < [self.masterTableView numberOfRows]) {
        [self.masterTableView selectRowIndexes:[NSIndexSet indexSetWithIndex:selectedRow] byExtendingSelection:NO];
    }

    NSAssert(self.detailTableView == nil || self.detailTableView.dataSource == self, @"Cannot reload a table view with a different data source");
    [self.detailTableView reloadData];

    NSAssert(self.collectionView == nil || self.collectionView.dataSource == self, @"Cannot reload a collection view with a different data source");
    [self.collectionView reloadData];
    
    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallLibraryFavoritesDataSourceDisplayedCollectionChangedNotification
                                                      object:self
                                                    userInfo:nil];
}

#pragma mark - NSTableViewDataSource (Master-Detail View)

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    if (tableView == self.masterTableView) {
        return _visibleSectionMapping.count;
    } else if (tableView == self.detailTableView && self.masterTableView.selectedRow > -1) {
        const HeimdallLibraryFavoritesSection section = [self sectionForVisibleIndex:self.masterTableView.selectedRow];
        
        // For artist and genre sections, return the flattened count
        if ([self isAudioGroupSection:section]) {
            NSArray * const flattenedArray = _flattenedRowMappings[@(section)];
            if (flattenedArray) {
                return flattenedArray.count;
            }
            NSArray * const newFlattenedArray = [self buildFlattenedArrayForAudioGroupSection:section];
            _flattenedRowMappings[@(section)] = newFlattenedArray;
            return newFlattenedArray.count;
        }
        
        return [self arrayForSection:section].count;
    }
    
    return 0;
}

- (id<NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row
{
    const id<HeimdallMediaLibraryItemProtocol> libraryItem = [self libraryItemAtRow:row forTableView:tableView];
    return [NSPasteboardItem pasteboardItemWithLibraryItem:libraryItem];
}

- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtRow:(NSInteger)row
                                       forTableView:(NSTableView *)tableView
{
    if (tableView == self.masterTableView) {
        // For master table, return a group descriptor object
        NSParameterAssert(row >= 0 && (NSUInteger)row < _visibleSectionMapping.count);
        return [self createGroupDescriptorForSection:[self sectionForVisibleIndex:row]];
    } else if (tableView == self.detailTableView && self.masterTableView.selectedRow > -1) {
        const HeimdallLibraryFavoritesSection section = [self sectionForVisibleIndex:self.masterTableView.selectedRow];
        
        // For artist and genre sections, use the flattened array
        if ([self isAudioGroupSection:section]) {
            NSArray<id<HeimdallMediaLibraryItemProtocol>> * const flattenedArray = _flattenedRowMappings[@(section)];
            NSParameterAssert(flattenedArray && (NSUInteger)row < flattenedArray.count);
            return flattenedArray[row];
        } else {
            // For other sections, use the regular array
            NSArray<id<HeimdallMediaLibraryItemProtocol>> * const sectionArray = [self arrayForSection:section];
            NSParameterAssert(sectionArray && row >= 0 && (NSUInteger)row < sectionArray.count);
            return sectionArray[row];
        }
    }
    
    return nil;
}

- (NSInteger)rowForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if (libraryItem == nil) {
        return NSNotFound;
    }
    
    // Search through all visible sections
    for (NSNumber * const sectionNumber in _visibleSectionMapping) {
        const HeimdallLibraryFavoritesSection section = (HeimdallLibraryFavoritesSection)[sectionNumber integerValue];
        NSArray * const sectionArray = [self arrayForSection:section];
        const NSInteger index = [self indexOfMediaItem:libraryItem.libraryID inArray:sectionArray];
        if (index != NSNotFound) {
            return index;
        }
    }
    
    return NSNotFound;
}

- (void)updateHeaderForMasterSelection
{
    HeimdallLibraryRepresentedItem *representedItem = nil;
    NSString *fallbackTitle = nil;
    NSString *fallbackDetail = nil;

    if (self.masterTableView.selectedRow >= 0) {
        const HeimdallLibraryFavoritesSection section = [self sectionForVisibleIndex:self.masterTableView.selectedRow];
        id<HeimdallMediaLibraryItemProtocol> const groupDescriptor = [self createGroupDescriptorForSection:section];
        fallbackDetail = groupDescriptor.primaryDetailString;
        const HeimdallMediaLibraryParentGroupType parentType = [self parentTypeForSection:section];
        representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:groupDescriptor parentType:parentType];

        switch (section) {
            case HeimdallLibraryFavoritesSectionVideoMedia:
                fallbackTitle = _NS("Favorite Videos");
                break;
            case HeimdallLibraryFavoritesSectionAudioMedia:
                fallbackTitle = _NS("Favorite Audio");
                break;
            case HeimdallLibraryFavoritesSectionAlbums:
                fallbackTitle = _NS("Favorite Albums");
                break;
            case HeimdallLibraryFavoritesSectionArtists:
                fallbackTitle = _NS("Favorite Artists");
                break;
            case HeimdallLibraryFavoritesSectionGenres:
                fallbackTitle = _NS("Favorite Genres");
                break;
            default:
                break;
        }
    } else {
        fallbackTitle = _NS("Favorites");
        NSUInteger totalCount = 0;
        totalCount += _favoriteVideoMediaArray.count;
        totalCount += _favoriteAudioMediaArray.count;
        totalCount += _favoriteAlbumsArray.count;
        totalCount += _favoriteArtistsArray.count;
        totalCount += _favoriteGenresArray.count;
        fallbackDetail = [NSString stringWithFormat:@"%lu items", (unsigned long)totalCount];
    }

    [self.headerDelegate updateHeaderForTableView:self.detailTableView
                              withRepresentedItem:representedItem
                                    fallbackTitle:fallbackTitle
                                   fallbackDetail:fallbackDetail];
}

- (void)updateHeaderInTableView:(NSTableView *)detailTableView forMasterSelection:(NSTableView *)masterTableView
{
    [self updateHeaderForMasterSelection];
}

- (HeimdallMediaLibraryParentGroupType)currentParentType
{
    if (self.masterTableView.selectedRow > -1) {
        const HeimdallLibraryFavoritesSection section = [self sectionForVisibleIndex:self.masterTableView.selectedRow];
        return [self parentTypeForSection:section];
    }
    return HeimdallMediaLibraryParentGroupTypeVideoLibrary; // Default fallback
}

#pragma mark - NSCollectionViewDataSource

- (NSInteger)numberOfSectionsInCollectionView:(NSCollectionView *)collectionView
{
    return _visibleSectionMapping.count;
}

- (NSInteger)collectionView:(NSCollectionView *)collectionView
     numberOfItemsInSection:(NSInteger)section
{
    const HeimdallLibraryFavoritesSection favoritesSection = [self sectionForVisibleIndex:section];
    return [self arrayForSection:favoritesSection].count;
}

- (NSCollectionViewItem *)collectionView:(NSCollectionView *)collectionView
     itemForRepresentedObjectAtIndexPath:(NSIndexPath *)indexPath
{
    HeimdallLibraryCollectionViewItem * const viewItem =
        [collectionView makeItemWithIdentifier:HeimdallLibraryCollectionViewItemIdentifier
                                  forIndexPath:indexPath];
    
    const HeimdallLibraryFavoritesSection section = [self sectionForVisibleIndex:indexPath.section];
    const HeimdallMediaLibraryParentGroupType parentType = [self parentTypeForSection:section];
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
        
        const HeimdallLibraryFavoritesSection section = [self sectionForVisibleIndex:indexPath.section];
        sectionHeadingView.stringValue = [self titleForSection:section];
        return sectionHeadingView;

    } else if ([kind isEqualToString:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind]) {
        const HeimdallLibraryFavoritesSection section = [self sectionForVisibleIndex:indexPath.section];
        if ([self isAudioGroupSection:section]) {
            // Redirect to audio group supplementary view
            return [self collectionView:collectionView 
                viewForSupplementaryElementOfKind:HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailViewKind 
                               atIndexPath:indexPath];
        } else if ([self isMediaListSection:section]) {
            // Redirect to media list supplementary view
            return [self collectionView:collectionView 
                viewForSupplementaryElementOfKind:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind 
                               atIndexPath:indexPath];
        }
        
        HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView * const mediaItemSupplementaryDetailView = 
            [collectionView makeSupplementaryViewOfKind:kind 
                                         withIdentifier:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind 
                                           forIndexPath:indexPath];
        
        const id<HeimdallMediaLibraryItemProtocol> item = [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
        if (item == nil || item.firstMediaItem == nil) {
            return nil;
        }

        const HeimdallMediaLibraryParentGroupType parentType = [self parentTypeForSection:section];
        HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:item parentType:parentType];

        mediaItemSupplementaryDetailView.representedItem = representedItem;
        mediaItemSupplementaryDetailView.selectedItem = [collectionView itemAtIndexPath:indexPath];
        return mediaItemSupplementaryDetailView;
    } else if ([kind isEqualToString:HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailViewKind]) {
        HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailView * const audioGroupSupplementaryDetailView = 
            [collectionView makeSupplementaryViewOfKind:kind 
                                         withIdentifier:HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailViewIdentifier 
                                           forIndexPath:indexPath];
        
        const id<HeimdallMediaLibraryItemProtocol> item = [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
        const HeimdallLibraryFavoritesSection section = [self sectionForVisibleIndex:indexPath.section];
        const HeimdallMediaLibraryParentGroupType parentType = [self parentTypeForSection:section];
        HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:item parentType:parentType];

        audioGroupSupplementaryDetailView.representedItem = representedItem;
        audioGroupSupplementaryDetailView.selectedItem = [collectionView itemAtIndexPath:indexPath];
        return audioGroupSupplementaryDetailView;
    } else if ([kind isEqualToString:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind]) {
        HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView * const mediaListSupplementaryDetailView = 
            [collectionView makeSupplementaryViewOfKind:kind 
                                         withIdentifier:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewIdentifier 
                                           forIndexPath:indexPath];
        
        const id<HeimdallMediaLibraryItemProtocol> item = [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
        const HeimdallLibraryFavoritesSection section = [self sectionForVisibleIndex:indexPath.section];
        const HeimdallMediaLibraryParentGroupType parentType = [self parentTypeForSection:section];
        HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:item parentType:parentType];

        mediaListSupplementaryDetailView.representedItem = representedItem;
        mediaListSupplementaryDetailView.selectedItem = [collectionView itemAtIndexPath:indexPath];
        return mediaListSupplementaryDetailView;
    }

    return nil;
}

#pragma mark - HeimdallLibraryCollectionViewDataSource

- (HeimdallLibraryCollectionViewItemAspectRatio)collectionView:(NSCollectionView *)collectionView
                                    aspectRatioForSection:(NSInteger)section
{    
    if ([self sectionForVisibleIndex:section] == HeimdallLibraryFavoritesSectionVideoMedia) {
        return HeimdallLibraryCollectionViewItemAspectRatioVideoItem;
    }
    return HeimdallLibraryCollectionViewItemAspectRatioDefaultItem;
}

- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtIndexPath:(NSIndexPath *)indexPath
                                        forCollectionView:(NSCollectionView *)collectionView
{
    const HeimdallLibraryFavoritesSection section = [self sectionForVisibleIndex:indexPath.section];
    NSArray * const sectionArray = [self arrayForSection:section];
    
    if (indexPath.item >= 0 && (NSUInteger)indexPath.item < sectionArray.count) {
        return sectionArray[indexPath.item];
    }
    
    return nil;
}

- (NSIndexPath *)indexPathForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if (libraryItem == nil) {
        return nil;
    }
    
    // Search through all visible sections
    for (NSUInteger visibleIndex = 0; visibleIndex < _visibleSectionMapping.count; visibleIndex++) {
        const HeimdallLibraryFavoritesSection section = (HeimdallLibraryFavoritesSection)[_visibleSectionMapping[visibleIndex] integerValue];
        NSArray * const sectionArray = [self arrayForSection:section];
        const NSInteger itemIndex = [self indexOfMediaItem:libraryItem.libraryID inArray:sectionArray];
        if (itemIndex != NSNotFound) {
            return [NSIndexPath indexPathForItem:itemIndex inSection:visibleIndex];
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
        const HeimdallLibraryFavoritesSection section = [self sectionForVisibleIndex:indexPath.section];
        const HeimdallMediaLibraryParentGroupType parentType = [self parentTypeForSection:section];
        const id<HeimdallMediaLibraryItemProtocol> libraryItem =
            [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
        
        if (libraryItem) {
            HeimdallLibraryRepresentedItem * const representedItem =
                [[HeimdallLibraryRepresentedItem alloc] initWithItem:libraryItem parentType:parentType];
            [representedItems addObject:representedItem];
        }
    }

    return representedItems;
}

- (NSString *)supplementaryDetailViewKind
{
    if (self.collectionView.selectionIndexPaths.count > 0) {
        NSIndexPath * const firstIndexPath = self.collectionView.selectionIndexPaths.anyObject;
        const HeimdallLibraryFavoritesSection section = [self sectionForVisibleIndex:firstIndexPath.section];
        
        if ([self isAudioGroupSection:section]) {
            return HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailViewKind;
        } else if ([self isMediaListSection:section]) {
            return HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind;
        }
    }
    
    return HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind;
}

- (NSArray<HeimdallMediaLibraryMediaItem *> *)allFavoritesArray
{
    return _allFavoritesArray;
}

@end
