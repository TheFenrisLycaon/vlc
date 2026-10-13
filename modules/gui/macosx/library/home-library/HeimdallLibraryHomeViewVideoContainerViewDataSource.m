/*****************************************************************************
 * HeimdallLibraryHomeViewVideoContainerViewDataSource.m: MacOS X interface module
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

#import "HeimdallLibraryHomeViewVideoContainerViewDataSource.h"

#import "extensions/NSView+HeimdallAdditions.h"

#import "library/HeimdallLibraryCarouselViewItemView.h"
#import "library/HeimdallLibraryCollectionViewFlowLayout.h"
#import "library/HeimdallLibraryCollectionViewItem.h"
#import "library/HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.h"
#import "library/HeimdallLibraryCollectionViewSupplementaryElementView.h"
#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryImageCache.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibraryRepresentedItem.h"

#import "library/home-library/HeimdallLibraryHomeViewVideoCarouselContainerView.h"
#import "library/home-library/HeimdallLibraryHomeViewVideoGridContainerView.h"

#import "library/video-library/HeimdallLibraryVideoGroupDescriptor.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallUIUnits.h"

NSString * const HeimdallLibraryVideoCollectionViewDataSourceDisplayedCollectionChangedNotification = @"HeimdallLibraryVideoCollectionViewDataSourceDisplayedCollectionChangedNotification";

@interface HeimdallLibraryHomeViewVideoContainerViewDataSource ()
{
    HeimdallLibraryCollectionViewFlowLayout *_collectionViewFlowLayout;
    HeimdallLibraryModel *_libraryModel;
}

@property (readwrite, atomic) NSMutableArray *collectionArray;

@end

@implementation HeimdallLibraryHomeViewVideoContainerViewDataSource

- (instancetype)init
{
    self = [super init];
    if(self) {
        _libraryModel = HeimdallMain.sharedInstance.libraryController.libraryModel;
        self.collectionArray = [NSMutableArray array];
        [self connect];
    }
    return self;
}

- (NSUInteger)indexOfMediaItemInCollection:(const int64_t)libraryId
{
    return [self.collectionArray indexOfObjectPassingTest:^BOOL(HeimdallMediaLibraryMediaItem * const findMediaItem, const NSUInteger __unused idx, BOOL * const __unused stop) {
        NSAssert(findMediaItem != nil, @"Collection should not contain nil media items");
        return findMediaItem.libraryID == libraryId;
    }];
}

- (void)libraryModelVideoListReset:(NSNotification * const)aNotification
{
    if (_groupDescriptor.group != HeimdallMediaLibraryParentGroupTypeVideoLibrary) {
        return;
    }

    [self reloadData];
}

- (void)libraryModelVideoItemUpdated:(NSNotification * const)aNotification
{
    if (_groupDescriptor.group != HeimdallMediaLibraryParentGroupTypeVideoLibrary) {
        return;
    }

    NSParameterAssert(aNotification);
    HeimdallMediaLibraryMediaItem *notificationMediaItem = aNotification.object;
    NSAssert(notificationMediaItem != nil, @"Media item updated notification should carry valid media item");

    [self reloadDataForMediaItem:notificationMediaItem];
}

- (void)libraryModelVideoItemDeleted:(NSNotification * const)aNotification
{
    if (_groupDescriptor.group != HeimdallMediaLibraryParentGroupTypeVideoLibrary) {
        return;
    }

    NSParameterAssert(aNotification);
    HeimdallMediaLibraryMediaItem *notificationMediaItem = aNotification.object;
    NSAssert(notificationMediaItem != nil, @"Media item deleted notification should carry valid media item");

    [self deleteDataForMediaItem:notificationMediaItem];
}

- (void)libraryModelRecentsListReset:(NSNotification * const)aNotification
{
    if (_groupDescriptor.group != HeimdallMediaLibraryParentGroupTypeRecentVideos) {
        return;
    }

    [self reloadData];
}

- (void)libraryModelRecentsItemUpdated:(NSNotification * const)aNotification
{
    if (_groupDescriptor.group != HeimdallMediaLibraryParentGroupTypeRecentVideos) {
        return;
    }

    NSParameterAssert(aNotification);
    HeimdallMediaLibraryMediaItem *notificationMediaItem = aNotification.object;
    NSAssert(notificationMediaItem != nil, @"Media item updated notification should carry valid media item");

    [self reloadDataForMediaItem:notificationMediaItem];
}

- (void)libraryModelRecentsItemDeleted:(NSNotification * const)aNotification
{
    if (_groupDescriptor.group != HeimdallMediaLibraryParentGroupTypeRecentVideos) {
        return;
    }

    NSParameterAssert(aNotification);
    HeimdallMediaLibraryMediaItem *notificationMediaItem = aNotification.object;
    NSAssert(notificationMediaItem != nil, @"Media item deleted notification should carry valid media item");

    [self deleteDataForMediaItem:notificationMediaItem];
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

- (void)reloadData
{
    if((self.collectionView == nil && self.carouselView == nil) || !_groupDescriptor) {
        NSLog(@"Null collection view/carousel view or video group descriptor");
        return;
    }

    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        NSArray *newCollectionArray = nil;
        switch(self->_groupDescriptor.group) {
            case HeimdallMediaLibraryParentGroupTypeVideoLibrary:
                newCollectionArray = self->_libraryModel.listOfVideoMedia;
                break;
            case HeimdallMediaLibraryParentGroupTypeRecentVideos:
                newCollectionArray = self->_libraryModel.listOfRecentMedia;
                break;
            default:
                return;
        }

        dispatch_async(dispatch_get_main_queue(), ^{
            self.collectionArray = [newCollectionArray mutableCopy];
            [self.collectionView reloadData];
            [self.carouselView reloadData];
            [NSNotificationCenter.defaultCenter postNotificationName:HeimdallLibraryVideoCollectionViewDataSourceDisplayedCollectionChangedNotification
                                                              object:self];
        });
    });
}

- (void)reloadDataForMediaItem:(HeimdallMediaLibraryMediaItem * const)mediaItem
{
    const NSUInteger mediaItemIndex = [self indexOfMediaItemInCollection:mediaItem.libraryID];
    if (mediaItemIndex == NSNotFound) {
        return;
    }

    [self.collectionArray replaceObjectAtIndex:mediaItemIndex withObject:mediaItem];

    NSIndexPath * const indexPath = [NSIndexPath indexPathForItem:mediaItemIndex inSection:0];
    [self.collectionView reloadItemsAtIndexPaths:[NSSet setWithObject:indexPath]];
    [self.carouselView reloadData];
    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallLibraryVideoCollectionViewDataSourceDisplayedCollectionChangedNotification
                                                      object:self];
}

- (void)deleteDataForMediaItem:(HeimdallMediaLibraryMediaItem * const)mediaItem
{
    const NSUInteger mediaItemIndex = [self indexOfMediaItemInCollection:mediaItem.libraryID];
    if (mediaItemIndex == NSNotFound) {
        return;
    }

    [self.collectionArray removeObjectAtIndex:mediaItemIndex];

    NSIndexPath * const indexPath = [NSIndexPath indexPathForItem:mediaItemIndex inSection:0];
    [self.collectionView deleteItemsAtIndexPaths:[NSSet setWithObject:indexPath]];
    [self.carouselView reloadData];
    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallLibraryVideoCollectionViewDataSourceDisplayedCollectionChangedNotification
                                                      object:self];
}

- (void)setGroupDescriptor:(HeimdallLibraryVideoCollectionViewGroupDescriptor *)groupDescriptor
{
    if(!groupDescriptor) {
        NSLog(@"Invalid group descriptor");
        return;
    }

    _groupDescriptor = groupDescriptor;
    [self reloadData];
}

- (void)setup
{
    _collectionViewFlowLayout = _collectionView.collectionViewLayout;
    self.collectionView.dataSource = self;
    self.carouselView.dataSource = self;

    [self.collectionView registerClass:HeimdallLibraryCollectionViewItem.class
                 forItemWithIdentifier:HeimdallLibraryCollectionViewItemIdentifier];

    [self.collectionView registerClass:[HeimdallLibraryCollectionViewSupplementaryElementView class]
        forSupplementaryViewOfKind:NSCollectionElementKindSectionHeader
                    withIdentifier:HeimdallLibrarySupplementaryElementViewIdentifier];

    NSNib * const mediaItemSupplementaryDetailView = [[NSNib alloc] initWithNibNamed:@"HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView" bundle:nil];
    [self.collectionView registerNib:mediaItemSupplementaryDetailView
          forSupplementaryViewOfKind:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind
                      withIdentifier:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewIdentifier];
}

- (NSInteger)numberOfSectionsInCollectionView:(NSCollectionView *)collectionView
{
    return 1;
}

- (NSInteger)collectionView:(NSCollectionView *)collectionView
     numberOfItemsInSection:(NSInteger)section
{
    if (!_libraryModel) {
        return 0;
    }

    return self.collectionArray.count;
}

- (NSCollectionViewItem *)collectionView:(NSCollectionView *)collectionView
     itemForRepresentedObjectAtIndexPath:(NSIndexPath *)indexPath
{
    HeimdallLibraryCollectionViewItem * const viewItem =
        [collectionView makeItemWithIdentifier:HeimdallLibraryCollectionViewItemIdentifier
                                  forIndexPath:indexPath];
    const id<HeimdallMediaLibraryItemProtocol> item = self.collectionArray[indexPath.item];
    HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:item parentType:self.groupDescriptor.group];

    viewItem.representedItem = representedItem;
    return viewItem;
}

- (NSView *)collectionView:(NSCollectionView *)collectionView
viewForSupplementaryElementOfKind:(NSCollectionViewSupplementaryElementKind)kind
               atIndexPath:(NSIndexPath *)indexPath
{
    if([kind isEqualToString:NSCollectionElementKindSectionHeader]) {
        HeimdallLibraryCollectionViewSupplementaryElementView *sectionHeadingView = [collectionView makeSupplementaryViewOfKind:kind
                                                                                                            withIdentifier:HeimdallLibrarySupplementaryElementViewIdentifier
                                                                                                              forIndexPath:indexPath];

        sectionHeadingView.stringValue = _groupDescriptor.name;
        return sectionHeadingView;

    } else if ([kind isEqualToString:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind]) {
        HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView * const mediaItemSupplementaryDetailView = [collectionView makeSupplementaryViewOfKind:kind withIdentifier:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind forIndexPath:indexPath];

        const id<HeimdallMediaLibraryItemProtocol> item = self.collectionArray[indexPath.item];
        HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:item parentType:self.groupDescriptor.group];

        mediaItemSupplementaryDetailView.representedItem = representedItem;
        mediaItemSupplementaryDetailView.selectedItem = [collectionView itemAtIndexPath:indexPath];

        HeimdallLibraryCollectionViewFlowLayout *flowLayout = (HeimdallLibraryCollectionViewFlowLayout*)collectionView.collectionViewLayout;
        if (flowLayout != nil) {
            mediaItemSupplementaryDetailView.layoutScrollDirection = flowLayout.scrollDirection;
        }
        
        return mediaItemSupplementaryDetailView;
    }

    return nil;
}

- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtIndexPath:(NSIndexPath *)indexPath
                                        forCollectionView:(NSCollectionView *)collectionView
{
    const NSUInteger indexPathItem = indexPath.item;

    if (indexPathItem < 0 || indexPathItem >= self.collectionArray.count) {
        return nil;
    }

    return self.collectionArray[indexPath.item];
}

- (NSIndexPath *)indexPathForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if (libraryItem == nil) {
        return nil;
    }

    const NSUInteger libraryItemIndex = [self indexOfMediaItemInCollection:libraryItem.libraryID];
    if (libraryItemIndex == NSNotFound) {
        return nil;
    }

    return [NSIndexPath indexPathForItem:libraryItemIndex inSection:0];
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
                                                 parentType:self.groupDescriptor.group];
        [representedItems addObject:representedItem];
    }

    return representedItems;
}

// pragma mark: iCarouselDataSource methods

- (NSInteger)numberOfItemsInCarousel:(iCarousel *)carousel
{
    if (self.collectionArray == nil) {
        return 0;
    }
    
    return self.collectionArray.count;
}

- (NSView *)carousel:(iCarousel *)carousel viewForItemAtIndex:(NSInteger)index reusingView:(NSView *)view
{
    HeimdallLibraryCarouselViewItemView *carouselItemView = (HeimdallLibraryCarouselViewItemView *)view;
    if (carouselItemView == nil) {
        const NSRect itemFrame = NSMakeRect(0,
                                            0,
                                            HeimdallUIUnits.carouselViewVideoItemViewWidth,
                                            HeimdallUIUnits.carouselViewItemViewHeight);
        carouselItemView = [HeimdallLibraryCarouselViewItemView fromNibWithOwner:self];
        carouselItemView.frame = itemFrame;
    }

    const id<HeimdallMediaLibraryItemProtocol> libraryItem = self.collectionArray[index];
    HeimdallLibraryRepresentedItem * const representedItem =
        [[HeimdallLibraryRepresentedItem alloc] initWithItem:libraryItem
                                             parentType:self.groupDescriptor.group];
    carouselItemView.representedItem = representedItem;
    return carouselItemView;
}

- (NSString *)supplementaryDetailViewKind
{
    return HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind;
}

@end
