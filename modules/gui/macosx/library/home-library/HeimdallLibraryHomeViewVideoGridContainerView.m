/*****************************************************************************
 * HeimdallLibraryHomeViewVideoGridContainerView.m: MacOS X interface module
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

#import "HeimdallLibraryHomeViewVideoGridContainerView.h"

#import "extensions/NSView+HeimdallAdditions.h"

#import "library/HeimdallLibraryCollectionView.h"
#import "library/HeimdallLibraryCollectionViewDelegate.h"
#import "library/HeimdallLibraryCollectionViewFlowLayout.h"
#import "library/HeimdallLibraryCollectionViewItem.h"
#import "library/HeimdallLibraryCollectionViewSupplementaryElementView.h"
#import "library/HeimdallLibraryDataTypes.h"

#import "library/home-library/HeimdallLibraryHomeViewVideoContainerViewDataSource.h"

#import "library/video-library/HeimdallLibraryVideoGroupDescriptor.h"

#import "views/HeimdallSubScrollView.h"
#import "views/HeimdallUIUnits.h"

@implementation HeimdallLibraryHomeViewVideoGridContainerView

@synthesize groupDescriptor = _groupDescriptor;
@synthesize videoGroup = _videoGroup;
@synthesize constraintsWithSuperview = _constraintsWithSuperview;
@synthesize dataSource = _dataSource;

- (instancetype)init
{
    self = [super init];

    if(self) {
        [self setupView];
        [self setupDataSource];
        [self setupCollectionViewSizeChangeListener];
        [self connect];
    }

    return self;
}

- (void)setupView
{
    [self setupCollectionView];
    [self setupScrollView];

    [self addSubview:_scrollView];
    [self.scrollView applyConstraintsToFillSuperview];

    [self setContentHuggingPriority:NSLayoutPriorityDefaultLow
                     forOrientation:NSLayoutConstraintOrientationHorizontal];
    [self setContentCompressionResistancePriority:NSLayoutPriorityDefaultLow
                                   forOrientation:NSLayoutConstraintOrientationHorizontal];

    [self setContentHuggingPriority:NSLayoutPriorityDefaultHigh
                     forOrientation:NSLayoutConstraintOrientationVertical];
    [self setContentCompressionResistancePriority:NSLayoutPriorityDefaultHigh
                     forOrientation:NSLayoutConstraintOrientationVertical];
}

- (void)setupCollectionView
{
    _collectionViewLayout = HeimdallLibraryCollectionViewFlowLayout.standardLayout;
    _collectionViewLayout.headerReferenceSize = HeimdallLibraryCollectionViewSupplementaryElementView.defaultHeaderSize;

    _collectionView = [[HeimdallLibraryCollectionView alloc] initWithFrame:NSZeroRect];
    _collectionView.postsFrameChangedNotifications = YES;
    _collectionView.collectionViewLayout = _collectionViewLayout;
    _collectionView.selectable = YES;
    _collectionView.allowsEmptySelection = YES;
    _collectionView.allowsMultipleSelection = YES;

    _collectionViewDelegate = [[HeimdallLibraryCollectionViewDelegate alloc] init];
    _collectionViewDelegate.itemsAspectRatio = HeimdallLibraryCollectionViewItemAspectRatioVideoItem;
    _collectionViewDelegate.staticItemSize = HeimdallUIUnits.defaultVideoItemCollectionViewItemSize;
    _collectionView.delegate = _collectionViewDelegate;
}

- (void)setupScrollView
{
    _scrollView = [[HeimdallSubScrollView alloc] init];
    _scrollView.scrollParentY = YES;
    _scrollView.forceHideVerticalScroller = YES;

    _scrollView.translatesAutoresizingMaskIntoConstraints = NO;
    _scrollView.documentView = _collectionView;
}

- (void)setupDataSource
{
    _dataSource = [[HeimdallLibraryHomeViewVideoContainerViewDataSource alloc] init];
    _dataSource.collectionView = _collectionView;
    [_dataSource setup];
}

- (void)setupCollectionViewSizeChangeListener
{
    NSNotificationCenter *notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(collectionViewFrameChanged:)
                               name:NSViewFrameDidChangeNotification
                             object:nil];
}

- (void)connect
{
    [self.dataSource connect];
}

- (void)disconnect
{
    [self.dataSource disconnect];
}

- (void)setGroupDescriptor:(HeimdallLibraryVideoCollectionViewGroupDescriptor *)groupDescriptor
{
    _groupDescriptor = groupDescriptor;
    _videoGroup = groupDescriptor.group;
    _dataSource.groupDescriptor = groupDescriptor;

    _collectionViewLayout.scrollDirection = _groupDescriptor.isHorizontalBarCollectionView ?
                                            NSCollectionViewScrollDirectionHorizontal :
                                            NSCollectionViewScrollDirectionVertical;
    _scrollView.scrollSelf = _groupDescriptor.isHorizontalBarCollectionView;
    _collectionViewDelegate.dynamicItemSizing = !_groupDescriptor.isHorizontalBarCollectionView;
}

- (void)setVideoGroup:(const HeimdallMediaLibraryParentGroupType)group
{
    if (_groupDescriptor.group == group) {
        return;
    }

    HeimdallLibraryVideoCollectionViewGroupDescriptor * const descriptor = [[HeimdallLibraryVideoCollectionViewGroupDescriptor alloc] initWithHeimdallVideoLibraryGroup:group];
    [self setGroupDescriptor:descriptor];
}

- (void)collectionViewFrameChanged:(NSNotification *)notification
{
    if ((NSCollectionView *)notification.object != _collectionView) {
        return;
    }

    // HACK: On app init the vertical collection views will not get their heights updated properly.
    // So let's schedule a check a bit later to correct this issue...
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
        [self invalidateIntrinsicContentSize];
    });
}

- (NSSize)intrinsicContentSize
{
    const NSSize collectionViewContentSize = _collectionViewLayout.collectionViewContentSize;
    const NSEdgeInsets scrollViewInsets = _collectionView.enclosingScrollView.contentInsets;
    const NSEdgeInsets collectionViewLayoutInset = _collectionViewLayout.sectionInset;
    const CGFloat insetsHeight = scrollViewInsets.top +
                                 scrollViewInsets.bottom +
                                 collectionViewLayoutInset.top +
                                 collectionViewLayoutInset.bottom;
    const CGFloat itemHeight = _collectionViewDelegate.staticItemSize.height;
    const CGFloat width = scrollViewInsets.left +
                          scrollViewInsets.right +
                          collectionViewLayoutInset.left +
                          collectionViewLayoutInset.right +
                          self.frame.size.width;

    if (collectionViewContentSize.height == 0) {
        // If we don't return a size larger than 0 then we run into issues with the collection
        // view layout not trying to properly calculate its size. So let's return something
        NSLog(@"Unable to provide accurate height for container -- providing rough size");
        const CGFloat roughValue = itemHeight + insetsHeight;
        return NSMakeSize(width, roughValue);
    }

    if (_groupDescriptor.isHorizontalBarCollectionView) {
        const CGFloat viewHeight = itemHeight + insetsHeight + HeimdallUIUnits.smallSpacing;
        return NSMakeSize(width, viewHeight);
    }

    // HACK: At very specific widths of the container, the full height containers
    // can have a bug where the top rows of the collection view are not displayed
    // at all. By reducing the height of the container to below the height of the
    // collection view contents we can eliminate this, so we reduce the height
    // just enough to not be noticeable but enough for the bug to not manifest
    return NSMakeSize(width, collectionViewContentSize.height - 15);
}

- (void)presentLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if (libraryItem == nil) {
        return;
    }

    NSIndexPath * const indexPath = [self.dataSource indexPathForLibraryItem:libraryItem];
    if (indexPath == nil) {
        return;
    }

    NSSet<NSIndexPath *> * const indexPathSet = [NSSet setWithObject:indexPath];
    NSCollectionView * const collectionView = self.collectionView;
    HeimdallLibraryCollectionViewFlowLayout * const flowLayout = (HeimdallLibraryCollectionViewFlowLayout *)collectionView.collectionViewLayout;

    [collectionView selectItemsAtIndexPaths:indexPathSet
                             scrollPosition:NSCollectionViewScrollPositionCenteredVertically];
    [flowLayout expandDetailSectionAtIndex:indexPath];
}

@end
