/*****************************************************************************
 * HeimdallLibraryVideoViewController.m: MacOS X interface module
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

#import "HeimdallLibraryVideoViewController.h"

#import "extensions/NSImage+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

#import "library/HeimdallLibraryConnectableDataSource.h"
#import "library/HeimdallLibraryCollectionView.h"
#import "library/HeimdallLibraryCollectionViewDelegate.h"
#import "library/HeimdallLibraryCollectionViewFlowLayout.h"
#import "library/HeimdallLibraryCollectionViewItem.h"
#import "library/HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.h"
#import "library/HeimdallLibraryCollectionViewSupplementaryElementView.h"
#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibrarySegment.h"
#import "library/HeimdallLibraryTableCellView.h"
#import "library/HeimdallLibraryWindow.h"
#import "library/HeimdallLibraryWindowPersistentPreferences.h"

#import "library/audio-library/HeimdallLibraryAudioViewController.h"

#import "library/home-library/HeimdallLibraryHomeViewVideoContainerViewDataSource.h"

#import "library/playlist-library/HeimdallLibraryPlaylistViewController.h"

#import "library/video-library/HeimdallLibraryMoviesDataSource.h"
#import "library/video-library/HeimdallLibraryShowsDataSource.h"
#import "library/video-library/HeimdallLibraryVideoDataSource.h"
#import "library/video-library/HeimdallLibraryVideoTableViewDelegate.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallUIUnits.h"

#import "windows/video/HeimdallMainVideoViewController.h"
#import "windows/video/HeimdallVoutView.h"

@interface HeimdallLibraryVideoViewController ()
{
    HeimdallLibraryVideoTableViewDelegate *_videoLibraryTableViewDelegate;
    HeimdallLibraryCollectionViewDelegate *_collectionViewDelegate;
    HeimdallLibraryCollectionViewFlowLayout *_collectionViewLayout;

    id<HeimdallMediaLibraryItemProtocol> _awaitingPresentingLibraryItem;

    NSArray<NSLayoutConstraint *> *_internalPlaceholderImageViewSizeConstraints;
}
@end

@implementation HeimdallLibraryVideoViewController

- (instancetype)initWithLibraryWindow:(HeimdallLibraryWindow *)libraryWindow
{
    self = [super initWithLibraryWindow:libraryWindow];

    if(self) {
        _videoLibraryTableViewDelegate = [[HeimdallLibraryVideoTableViewDelegate alloc] init];

        [self setupPropertiesFromLibraryWindow:libraryWindow];
        [self setupTableView];
        [self setupCollectionView];
        [self setupVideoPlaceholderView];
        [self setupVideoLibraryViews];

        NSNotificationCenter *notificationCenter = NSNotificationCenter.defaultCenter;
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelUpdated:)
                                   name:HeimdallLibraryModelVideoMediaListReset
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelUpdated:)
                                   name:HeimdallLibraryModelAllCachesDropped
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelUpdated:)
                                   name:HeimdallLibraryModelVideoMediaItemDeleted
                                 object:nil];

        NSString * const videoMediaResetLongLoadStartNotification = [HeimdallLibraryModelVideoMediaListReset stringByAppendingString:HeimdallLongNotificationNameStartSuffix];
        NSString * const videoMediaResetLongLoadFinishNotification = [HeimdallLibraryModelVideoMediaListReset stringByAppendingString:HeimdallLongNotificationNameFinishSuffix];
        NSString * const videoMediaDeletedLongLoadStartNotification = [HeimdallLibraryModelVideoMediaItemDeleted stringByAppendingString:HeimdallLongNotificationNameStartSuffix];
        NSString * const videoMediaDeletedLongLoadFinishNotification = [HeimdallLibraryModelVideoMediaItemDeleted stringByAppendingString:HeimdallLongNotificationNameFinishSuffix];

        [notificationCenter addObserver:self
                               selector:@selector(libraryModelLongLoadStarted:)
                                   name:videoMediaResetLongLoadStartNotification
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelLongLoadFinished:)
                                   name:videoMediaResetLongLoadFinishNotification
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelLongLoadStarted:)
                                   name:videoMediaDeletedLongLoadStartNotification
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelLongLoadFinished:)
                                   name:videoMediaDeletedLongLoadFinishNotification
                                 object:nil];
    }

    return self;
}

- (void)setupPropertiesFromLibraryWindow:(HeimdallLibraryWindow *)libraryWindow
{
    NSParameterAssert(libraryWindow);
    _videoLibraryView = libraryWindow.videoLibraryView;
    _videoLibraryCollectionViewScrollView = libraryWindow.videoLibraryCollectionViewScrollView;
    _videoLibraryCollectionView = libraryWindow.videoLibraryCollectionView;
    _videoLibrarySplitView = libraryWindow.videoLibrarySplitView;
    _videoLibraryGroupsTableViewScrollView = libraryWindow.videoLibraryGroupsTableViewScrollView;
    _videoLibraryGroupsTableView = libraryWindow.videoLibraryGroupsTableView;
    _videoLibraryGroupSelectionTableViewScrollView = libraryWindow.videoLibraryGroupSelectionTableViewScrollView;
    _videoLibraryGroupSelectionTableView = libraryWindow.videoLibraryGroupSelectionTableView;
}

- (void)setupTableView
{
    NSNib * const tableCellViewNib =
        [[NSNib alloc] initWithNibNamed:@"HeimdallLibraryVideoTableCellView"
                                 bundle:nil];
    [self.videoLibraryGroupSelectionTableView registerNib:tableCellViewNib
                                           forIdentifier:@"HeimdallVideoLibraryTableViewCellIdentifier"];
    [self.videoLibraryGroupsTableView registerNib:tableCellViewNib
                                   forIdentifier:@"HeimdallVideoLibraryTableViewCellIdentifier"];
    
    self.videoLibraryGroupSelectionTableView.floatsGroupRows = NO;
}

- (void)setupVideoDataSource
{
    _libraryVideoDataSource = [[HeimdallLibraryVideoDataSource alloc] init];
    self.libraryVideoDataSource.libraryModel = HeimdallMain.sharedInstance.libraryController.libraryModel;
    self.libraryVideoDataSource.tableView = self.videoLibraryGroupSelectionTableView;
    self.libraryVideoDataSource.collectionView = self.videoLibraryCollectionView;
}

- (void)setupShowsDataSource
{
    _libraryShowsDataSource = [[HeimdallLibraryShowsDataSource alloc] init];
    self.libraryShowsDataSource.libraryModel =
        HeimdallMain.sharedInstance.libraryController.libraryModel;
    self.libraryShowsDataSource.collectionView = self.videoLibraryCollectionView;
    self.libraryShowsDataSource.masterTableView = self.videoLibraryGroupsTableView;
    self.libraryShowsDataSource.detailTableView = self.videoLibraryGroupSelectionTableView;
}

- (void)setupMoviesDataSource
{
    _libraryMoviesDataSource = [[HeimdallLibraryMoviesDataSource alloc] init];
    self.libraryMoviesDataSource.libraryModel =
        HeimdallMain.sharedInstance.libraryController.libraryModel;
    self.libraryMoviesDataSource.tableView = self.videoLibraryGroupSelectionTableView;
    self.libraryMoviesDataSource.collectionView = self.videoLibraryCollectionView;
}

- (void)setupCollectionView
{
    _collectionViewLayout = [[HeimdallLibraryCollectionViewFlowLayout alloc] init];

    const CGFloat collectionItemSpacing = HeimdallUIUnits.collectionViewItemSpacing;
    const NSEdgeInsets collectionViewSectionInset = HeimdallUIUnits.collectionViewSectionInsets;
    _collectionViewLayout.headerReferenceSize = HeimdallLibraryCollectionViewSupplementaryElementView.defaultHeaderSize;
    _collectionViewLayout.minimumLineSpacing = collectionItemSpacing;
    _collectionViewLayout.minimumInteritemSpacing = collectionItemSpacing;
    _collectionViewLayout.sectionInset = collectionViewSectionInset;

    NSCollectionView * const collectionView = self.videoLibraryCollectionView;
    collectionView.collectionViewLayout = _collectionViewLayout;

    _collectionViewDelegate = [[HeimdallLibraryCollectionViewDelegate alloc] init];
    _collectionViewDelegate.itemsAspectRatio = HeimdallLibraryCollectionViewItemAspectRatioVideoItem;
    _collectionViewDelegate.staticItemSize = HeimdallUIUnits.defaultVideoItemCollectionViewItemSize;
    collectionView.delegate = _collectionViewDelegate;

    [collectionView registerClass:HeimdallLibraryCollectionViewItem.class
            forItemWithIdentifier:HeimdallLibraryCollectionViewItemIdentifier];

    [collectionView registerClass:HeimdallLibraryCollectionViewSupplementaryElementView.class
       forSupplementaryViewOfKind:NSCollectionElementKindSectionHeader
                   withIdentifier:HeimdallLibrarySupplementaryElementViewIdentifier];

    NSString * const mediaItemSupplementaryDetailViewString =
        NSStringFromClass(HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.class);
    NSNib * const mediaItemSupplementaryDetailViewNib =
        [[NSNib alloc] initWithNibNamed:mediaItemSupplementaryDetailViewString bundle:nil];
    
    [collectionView registerNib:mediaItemSupplementaryDetailViewNib
     forSupplementaryViewOfKind:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind
                 withIdentifier:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewIdentifier];
}

- (void)setupVideoPlaceholderView
{
    _internalPlaceholderImageViewSizeConstraints = @[
        [NSLayoutConstraint constraintWithItem:self.placeholderImageView
                                     attribute:NSLayoutAttributeWidth
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:nil
                                     attribute:NSLayoutAttributeNotAnAttribute
                                    multiplier:0.f
                                      constant:182.f],
        [NSLayoutConstraint constraintWithItem:self.placeholderImageView
                                     attribute:NSLayoutAttributeHeight
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:nil
                                     attribute:NSLayoutAttributeNotAnAttribute
                                    multiplier:0.f
                                      constant:114.f],
    ];
}

- (void)setupVideoLibraryViews
{
    _videoLibraryGroupSelectionTableView.rowHeight = HeimdallUIUnits.videoLibraryTableViewRowHeight;

    const NSEdgeInsets defaultInsets = HeimdallUIUnits.libraryViewScrollViewContentInsets;
    const NSEdgeInsets scrollerInsets = HeimdallUIUnits.libraryViewScrollViewScrollerInsets;

    _videoLibraryCollectionViewScrollView.automaticallyAdjustsContentInsets = NO;
    _videoLibraryCollectionViewScrollView.contentInsets = defaultInsets;
    _videoLibraryCollectionViewScrollView.scrollerInsets = scrollerInsets;

    _videoLibraryGroupSelectionTableViewScrollView.automaticallyAdjustsContentInsets = NO;
    _videoLibraryGroupSelectionTableViewScrollView.contentInsets = defaultInsets;
    _videoLibraryGroupSelectionTableViewScrollView.scrollerInsets = scrollerInsets;
}

- (void)teardownVideoDataSource
{
    [_libraryVideoDataSource disconnect];
    _libraryVideoDataSource.tableView = nil;
    _libraryVideoDataSource.collectionView = nil;
    _libraryVideoDataSource = nil;
}

- (void)teardownShowsDataSource
{
    [_libraryShowsDataSource disconnect];
    _libraryShowsDataSource.collectionView = nil;
    _libraryShowsDataSource.masterTableView = nil;
    _libraryShowsDataSource.detailTableView = nil;
    _libraryShowsDataSource = nil;
}

- (void)teardownMoviesDataSource
{
    [_libraryMoviesDataSource disconnect];
    _libraryMoviesDataSource.tableView = nil;
    _libraryMoviesDataSource.collectionView = nil;
    _libraryMoviesDataSource = nil;
}

#pragma mark - Show the video library view

- (NSArray<NSLayoutConstraint *> *)placeholderImageViewSizeConstraints
{
    return _internalPlaceholderImageViewSizeConstraints;
}

- (id<HeimdallLibraryDataSource>)currentDataSource
{
    const NSInteger librarySegmentType = self.libraryWindow.librarySegmentType;
    if (librarySegmentType == HeimdallLibraryVideoSegmentType) {
        return self.libraryVideoDataSource;
    } else if (librarySegmentType == HeimdallLibraryShowsVideoSubSegmentType) {
        return self.libraryShowsDataSource;
    } else if (librarySegmentType == HeimdallLibraryMoviesVideoSubSegmentType) {
        return self.libraryMoviesDataSource;
    } else {
        return nil;
    }
}

- (id<HeimdallLibraryConnectableDataSource>)currentConnectableDataSource
{
    return (id<HeimdallLibraryConnectableDataSource>)self.currentDataSource;
}

- (void)updatePresentedVideoLibraryView
{
    [self teardownShowsDataSource];
    [self teardownMoviesDataSource];
    if (_libraryVideoDataSource == nil) {
        [self setupVideoDataSource];
    }
    self.videoLibraryCollectionView.dataSource = self.libraryVideoDataSource;

    self.videoLibraryGroupSelectionTableView.dataSource = self.libraryVideoDataSource;
    self.videoLibraryGroupSelectionTableView.target = self.libraryVideoDataSource;
    self.videoLibraryGroupSelectionTableView.delegate = _videoLibraryTableViewDelegate;

    [self.libraryVideoDataSource reloadData];

    const BOOL anyVideoMedia = self.libraryVideoDataSource.libraryModel.numberOfVideoMedia > 0;
    if (anyVideoMedia) {
        const HeimdallLibraryViewModeSegment viewModeSegment = HeimdallLibraryWindowPersistentPreferences.sharedInstance.videoLibraryViewMode;
        [self presentVideoLibraryView:viewModeSegment];
    } else if (self.libraryVideoDataSource.libraryModel.filterString.length > 0) {
        [self.libraryWindow displayNoResultsMessage];
    } else {
        [self presentPlaceholderVideoLibraryView];
    }
}

- (void)updatePresentedShowsLibraryView
{
    [self teardownVideoDataSource];
    [self teardownMoviesDataSource];
    if (_libraryShowsDataSource == nil) {
        [self setupShowsDataSource];
    }
    self.videoLibraryCollectionView.dataSource = self.libraryShowsDataSource;

    self.videoLibraryGroupsTableView.dataSource = self.libraryShowsDataSource;
    self.videoLibraryGroupsTableView.target = self.libraryShowsDataSource;
    self.videoLibraryGroupsTableView.delegate = _videoLibraryTableViewDelegate;

    self.videoLibraryGroupSelectionTableView.dataSource = self.libraryShowsDataSource;
    self.videoLibraryGroupSelectionTableView.target = self.libraryShowsDataSource;
    self.videoLibraryGroupSelectionTableView.delegate = _videoLibraryTableViewDelegate;

    // Shows uses the master/detail split view, not the single sectioned table.
    // The master and detail table views are wired in setupShowsDataSource.

    [self.libraryShowsDataSource reloadData];

    const BOOL anyShows = self.libraryShowsDataSource.libraryModel.listOfShows.count > 0;
    if (anyShows) {
        const HeimdallLibraryViewModeSegment viewModeSegment = HeimdallLibraryWindowPersistentPreferences.sharedInstance.showsLibraryViewMode;
        [self presentVideoLibraryView:viewModeSegment];
    } else if (self.libraryShowsDataSource.libraryModel.filterString.length > 0) {
        [self.libraryWindow displayNoResultsMessage];
    } else {
        [self presentPlaceholderVideoLibraryView];
    }
}

- (void)presentVideoView
{
    [self updatePresentedVideoLibraryView];
}

- (void)updatePresentedMoviesLibraryView
{
    [self teardownVideoDataSource];
    [self teardownShowsDataSource];
    if (_libraryMoviesDataSource == nil) {
        [self setupMoviesDataSource];
    }
    self.videoLibraryCollectionView.dataSource = self.libraryMoviesDataSource;

    self.videoLibraryGroupSelectionTableView.dataSource = self.libraryMoviesDataSource;
    self.videoLibraryGroupSelectionTableView.target = self.libraryMoviesDataSource;
    self.videoLibraryGroupSelectionTableView.delegate = _videoLibraryTableViewDelegate;

    [self.libraryMoviesDataSource reloadData];

    const BOOL anyMovies = self.libraryMoviesDataSource.libraryModel.numberOfMovies > 0;
    if (anyMovies) {
        const HeimdallLibraryViewModeSegment viewModeSegment = HeimdallLibraryWindowPersistentPreferences.sharedInstance.moviesLibraryViewMode;
        [self presentVideoLibraryView:viewModeSegment];
    } else if (self.libraryMoviesDataSource.libraryModel.filterString.length > 0) {
        [self.libraryWindow displayNoResultsMessage];
    } else {
        [self presentPlaceholderVideoLibraryView];
    }
}

- (void)presentMoviesView
{
    [self updatePresentedMoviesLibraryView];
}

- (void)presentShowsView
{
    [self updatePresentedShowsLibraryView];
}

- (void)presentPlaceholderVideoLibraryView
{
    [self.libraryWindow displayLibraryPlaceholderViewWithImage:NSImage.HeimdallPlaceholderVideoImage
                                              usingConstraints:self.placeholderImageViewSizeConstraints
                                             displayingMessage:_NS("Your videos will appear here.\nGo to the Browse section to add videos you love.")];
}

- (void)presentVideoLibraryView:(HeimdallLibraryViewModeSegment)viewModeSegment
{
    const NSInteger librarySegmentType = self.libraryWindow.librarySegmentType;
    const BOOL isShowsSegment = (librarySegmentType == HeimdallLibraryShowsVideoSubSegmentType);

    [self.libraryWindow displayLibraryView:self.videoLibraryView];

    if (viewModeSegment == HeimdallLibraryGridViewModeSegment) {
        self.videoLibrarySplitView.hidden = YES;
        self.videoLibraryCollectionViewScrollView.hidden = NO;
    } else if (viewModeSegment == HeimdallLibraryListViewModeSegment) {
        self.videoLibraryCollectionViewScrollView.hidden = YES;
        if (isShowsSegment) {
            // Shows uses the master/detail split view
            self.videoLibrarySplitView.hidden = NO;
            self.videoLibraryGroupsTableViewScrollView.hidden = NO;
        } else {
            // Videos and Movies use the single sectioned table
            self.videoLibrarySplitView.hidden = NO;
            self.videoLibraryGroupsTableViewScrollView.hidden = YES;
        }
    } else {
        NSAssert(false, @"View mode must be grid or list mode");
    }
}

- (void)libraryModelUpdated:(NSNotification *)aNotification
{
    NSParameterAssert(aNotification);
    HeimdallLibraryModel *model = HeimdallMain.sharedInstance.libraryController.libraryModel;
    const NSUInteger videoCount = model.numberOfVideoMedia;
    const NSUInteger showsCount = model.numberOfShows;

    if (self.libraryWindow.librarySegmentType == HeimdallLibraryVideoSegmentType &&
        ((videoCount == 0 && ![self.libraryTargetView.subviews containsObject:self.emptyLibraryView]) ||
         (videoCount > 0 && ![self.libraryTargetView.subviews containsObject:_videoLibraryView])) &&
        !self.libraryWindow.embeddedVideoPlaybackActive) {

        [self updatePresentedVideoLibraryView];
    } else if (self.libraryWindow.librarySegmentType == HeimdallLibraryShowsVideoSubSegmentType &&
         ((showsCount == 0 && ![self.libraryTargetView.subviews containsObject:self.emptyLibraryView]) ||
          (showsCount > 0 && ![self.libraryTargetView.subviews containsObject:_videoLibraryView])) &&
         !self.libraryWindow.embeddedVideoPlaybackActive) {

         [self updatePresentedShowsLibraryView];
     } else if (self.libraryWindow.librarySegmentType == HeimdallLibraryMoviesVideoSubSegmentType &&
         ((model.numberOfMovies == 0 && ![self.libraryTargetView.subviews containsObject:self.emptyLibraryView]) ||
          (model.numberOfMovies > 0 && ![self.libraryTargetView.subviews containsObject:_videoLibraryView])) &&
         !self.libraryWindow.embeddedVideoPlaybackActive) {

         [self updatePresentedMoviesLibraryView];
     }
}

- (void)presentLibraryItemWaitForCollectionViewDataSourceFinished:(NSNotification *)notification
{
    [NSNotificationCenter.defaultCenter removeObserver:self
                                                  name:HeimdallLibraryVideoDataSourceDisplayedCollectionChangedNotification
                                                object:self.libraryVideoDataSource];

    NSIndexPath * const indexPath = [self.libraryVideoDataSource indexPathForLibraryItem:_awaitingPresentingLibraryItem];
    if (indexPath != nil) {
        NSSet * const indexPathSet = [NSSet setWithObject:indexPath];
        [self.videoLibraryCollectionView selectItemsAtIndexPaths:indexPathSet scrollPosition:NSCollectionViewScrollPositionTop];
        [self.videoLibraryCollectionView scrollToItemsAtIndexPaths:indexPathSet scrollPosition:NSCollectionViewScrollPositionTop];
    }

    _awaitingPresentingLibraryItem = nil;
}

- (void)presentLibraryItemWaitForTableViewDataSourceFinished:(NSNotification *)notification
{
    [NSNotificationCenter.defaultCenter removeObserver:self
                                                  name:HeimdallLibraryVideoDataSourceDisplayedCollectionChangedNotification
                                             object:self.libraryVideoDataSource];

    const NSInteger rowForLibraryItem = [self.libraryVideoDataSource rowForLibraryItem:_awaitingPresentingLibraryItem];
    if (rowForLibraryItem != NSNotFound) {
        NSIndexSet * const indexSet = [NSIndexSet indexSetWithIndex:rowForLibraryItem];
        [self.videoLibraryGroupSelectionTableView selectRowIndexes:indexSet byExtendingSelection:NO];
        [self.videoLibraryGroupSelectionTableView scrollRowToVisible:rowForLibraryItem];
    }

    _awaitingPresentingLibraryItem = nil;
}

- (void)presentLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if (libraryItem == nil) {
        return;
    }

    _awaitingPresentingLibraryItem = libraryItem;

     const HeimdallLibraryViewModeSegment viewModeSegment = HeimdallLibraryWindowPersistentPreferences.sharedInstance.videoLibraryViewMode;

    if (viewModeSegment == HeimdallLibraryGridViewModeSegment) {
        [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(presentLibraryItemWaitForCollectionViewDataSourceFinished:)
                                               name:HeimdallLibraryVideoDataSourceDisplayedCollectionChangedNotification
                                             object:self.libraryVideoDataSource];

    } else if (viewModeSegment == HeimdallLibraryListViewModeSegment) {
        [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(presentLibraryItemWaitForTableViewDataSourceFinished:)
                                               name:HeimdallLibraryVideoDataSourceDisplayedCollectionChangedNotification
                                             object:self.libraryVideoDataSource];

    } else {
        NSAssert(false, @"View mode must be grid or list mode");
    }
}

- (void)libraryModelLongLoadStarted:(NSNotification *)notification
{
    if (self.connected) {
        const id<HeimdallLibraryConnectableDataSource> dataSource = self.currentConnectableDataSource;
        [dataSource disconnect];
    }
    [self.libraryWindow showLoadingOverlay];
}

- (void)libraryModelLongLoadFinished:(NSNotification *)notification
{
    if (self.connected) {
        const id<HeimdallLibraryConnectableDataSource> dataSource = self.currentConnectableDataSource;
        [dataSource connect];
    }
    [self.libraryWindow hideLoadingOverlay];
}

@end
