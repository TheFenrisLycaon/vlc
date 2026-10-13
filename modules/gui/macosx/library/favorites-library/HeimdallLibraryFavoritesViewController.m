/*****************************************************************************
 * HeimdallLibraryFavoritesViewController.m MacOS X interface module
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

#import "HeimdallLibraryFavoritesViewController.h"

#import "extensions/NSImage+HeimdallAdditions.h"
#import "extensions/NSScrollView+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"
#import "extensions/NSView+HeimdallAdditions.h"

#import "library/HeimdallLibraryCollectionView.h"
#import "library/HeimdallLibraryCollectionViewDelegate.h"
#import "library/HeimdallLibraryCollectionViewFlowLayout.h"
#import "library/HeimdallLibraryCollectionViewItem.h"
#import "library/HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView.h"
#import "library/HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.h"
#import "library/HeimdallLibraryCollectionViewSupplementaryElementView.h"
#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryHeaderCell.h"
#import "library/HeimdallLibraryHeaderView.h"
#import "library/HeimdallLibraryMasterDetailViewTableViewDelegate.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibrarySegment.h"
#import "library/HeimdallLibraryTableCellView.h"
#import "library/HeimdallLibraryTableView.h"
#import "library/HeimdallLibraryTwoPaneSplitViewDelegate.h"
#import "library/HeimdallLibraryWindow.h"
#import "library/HeimdallLibraryWindowPersistentPreferences.h"

#import "library/audio-library/HeimdallLibraryAlbumTableCellView.h"
#import "library/audio-library/HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailView.h"

#import "library/favorites-library/HeimdallLibraryFavoritesDataSource.h"
#import "library/favorites-library/HeimdallLibraryFavoritesTableViewDelegate.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallUIUnits.h"

@interface HeimdallLibraryFavoritesViewController ()
{
    HeimdallLibraryFavoritesTableViewDelegate *_favoritesLibraryTableViewDelegate;
    HeimdallLibraryTwoPaneSplitViewDelegate *_splitViewDelegate;
    HeimdallLibraryCollectionViewDelegate *_collectionViewDelegate;
    HeimdallLibraryCollectionViewFlowLayout *_collectionViewLayout;
    NSArray<NSLayoutConstraint *> *_internalPlaceholderImageViewSizeConstraints;
    
    id<HeimdallMediaLibraryItemProtocol> _awaitingPresentingLibraryItem;
}

@end

@implementation HeimdallLibraryFavoritesViewController

- (instancetype)initWithLibraryWindow:(HeimdallLibraryWindow *)libraryWindow
{
    self = [super initWithLibraryWindow:libraryWindow];
    
    if (self) {
        _favoritesLibraryTableViewDelegate = [[HeimdallLibraryFavoritesTableViewDelegate alloc] init];
        _splitViewDelegate = [[HeimdallLibraryTwoPaneSplitViewDelegate alloc] init];
        
        [self setupProperties];
        [self setupTableViews];
        [self setupCollectionView];
        [self setupFavoritesDataSource];
        [self setupFavoritesPlaceholderView];
        [self setupFavoritesLibraryViews];
        [self setupFavoritesLibraryContainerView];
        [self setupNotifications];
    }
    
    return self;
}

- (void)presentFavoritesView
{
    [self updatePresentedFavoritesView];
}

- (void)setupProperties
{
    _favoritesLibraryView = [[NSView alloc] init];
    _favoritesLibrarySplitView = [[NSSplitView alloc] init];
    _favoritesLibraryCollectionView = [[HeimdallLibraryCollectionView alloc] init];
    _favoritesLibraryGroupSelectionTableView = [[HeimdallLibraryTableView alloc] init];
    _favoritesLibraryGroupsTableView = [[HeimdallLibraryTableView alloc] init];
}

- (void)setupTableViews
{
    self.favoritesLibrarySplitView.delegate = _splitViewDelegate;

    _favoritesHeaderView = [HeimdallLibraryHeaderView paddedHeaderView];

    _favoritesLibraryTableViewDelegate.detailTableHeaderView = self.favoritesHeaderView;
    self.favoritesLibraryGroupSelectionTableView.headerView = self.favoritesHeaderView;

    NSTableColumn * const groupsColumn = [[NSTableColumn alloc] initWithIdentifier:@"groups"];
    NSTableColumn * const selectedGroupColumn = [[NSTableColumn alloc] initWithIdentifier:@"selectedGroup"];
    
    selectedGroupColumn.headerCell = [HeimdallLibraryHeaderCell new];

    [self.favoritesLibraryGroupsTableView addTableColumn:groupsColumn];
    [self.favoritesLibraryGroupSelectionTableView addTableColumn:selectedGroupColumn];

    NSNib * const tableCellViewNib =
        [[NSNib alloc] initWithNibNamed:NSStringFromClass(HeimdallLibraryTableCellView.class)
                                 bundle:nil];
    [self.favoritesLibraryGroupsTableView registerNib:tableCellViewNib
                                        forIdentifier:HeimdallLibraryTableCellViewIdentifier];
    [self.favoritesLibraryGroupSelectionTableView registerNib:tableCellViewNib 
                                                forIdentifier:HeimdallLibraryTableCellViewIdentifier];
    
    // Register album cell view for artist/genre sections
    NSNib * const albumCellViewNib =
        [[NSNib alloc] initWithNibNamed:NSStringFromClass(HeimdallLibraryAlbumTableCellView.class)
                                 bundle:nil];
    [self.favoritesLibraryGroupSelectionTableView registerNib:albumCellViewNib
                                                forIdentifier:HeimdallAudioLibraryCellIdentifier];
    
    self.favoritesLibraryGroupsTableView.headerView = nil;
    self.favoritesLibraryGroupsTableView.allowsEmptySelection = NO;
    
    self.favoritesLibraryGroupsTableView.rowHeight = HeimdallUIUnits.mediumTableViewRowHeight;
    self.favoritesLibraryGroupSelectionTableView.rowHeight = HeimdallUIUnits.mediumTableViewRowHeight;

    self.favoritesLibraryGroupsTableView.allowsMultipleSelection = YES;
    self.favoritesLibraryGroupSelectionTableView.allowsMultipleSelection = YES;
}

- (void)setupCollectionView
{
    _favoritesLibraryCollectionViewScrollView =
        [NSScrollView libraryScrollViewWithDocumentView:self.favoritesLibraryCollectionView];
    self.favoritesLibraryCollectionView.translatesAutoresizingMaskIntoConstraints = NO;

    _collectionViewLayout = [[HeimdallLibraryCollectionViewFlowLayout alloc] init];
    
    const CGFloat collectionItemSpacing = HeimdallUIUnits.collectionViewItemSpacing;
    const NSEdgeInsets collectionViewSectionInset = HeimdallUIUnits.collectionViewSectionInsets;
    _collectionViewLayout.headerReferenceSize = HeimdallLibraryCollectionViewSupplementaryElementView.defaultHeaderSize;
    _collectionViewLayout.minimumLineSpacing = collectionItemSpacing;
    _collectionViewLayout.minimumInteritemSpacing = collectionItemSpacing;
    _collectionViewLayout.sectionInset = collectionViewSectionInset;
    
    NSCollectionView * const collectionView = self.favoritesLibraryCollectionView;
    collectionView.collectionViewLayout = _collectionViewLayout;
    
    _collectionViewDelegate = [[HeimdallLibraryCollectionViewDelegate alloc] init];
    collectionView.delegate = _collectionViewDelegate;
    
    collectionView.selectable = YES;
    collectionView.allowsEmptySelection = YES;
    collectionView.allowsMultipleSelection = YES;
    
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
    
    NSString * const audioGroupSupplementaryDetailViewString =
        NSStringFromClass(HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailView.class);
    NSNib * const audioGroupSupplementaryDetailViewNib =
        [[NSNib alloc] initWithNibNamed:audioGroupSupplementaryDetailViewString bundle:nil];
    
    [collectionView registerNib:audioGroupSupplementaryDetailViewNib
     forSupplementaryViewOfKind:HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailViewKind
                 withIdentifier:HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailViewIdentifier];
    
    NSString * const mediaListSupplementaryDetailViewString =
        NSStringFromClass(HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView.class);
    NSNib * const mediaListSupplementaryDetailViewNib =
        [[NSNib alloc] initWithNibNamed:mediaListSupplementaryDetailViewString bundle:nil];
    
    [collectionView registerNib:mediaListSupplementaryDetailViewNib
     forSupplementaryViewOfKind:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind
                 withIdentifier:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewIdentifier];
}

- (void)setupFavoritesDataSource
{
    _libraryFavoritesDataSource = [[HeimdallLibraryFavoritesDataSource alloc] init];
    self.libraryFavoritesDataSource.libraryModel = HeimdallMain.sharedInstance.libraryController.libraryModel;
    self.libraryFavoritesDataSource.collectionView = self.favoritesLibraryCollectionView;
    self.libraryFavoritesDataSource.masterTableView = self.favoritesLibraryGroupsTableView;
    self.libraryFavoritesDataSource.detailTableView = self.favoritesLibraryGroupSelectionTableView;
    self.libraryFavoritesDataSource.headerDelegate = _favoritesLibraryTableViewDelegate;
    
    self.favoritesLibraryCollectionView.dataSource = self.libraryFavoritesDataSource;
    
    self.favoritesLibraryGroupsTableView.dataSource = self.libraryFavoritesDataSource;
    self.favoritesLibraryGroupsTableView.target = self.libraryFavoritesDataSource;
    self.favoritesLibraryGroupsTableView.delegate = _favoritesLibraryTableViewDelegate;

    self.favoritesLibraryGroupSelectionTableView.dataSource = self.libraryFavoritesDataSource;
    self.favoritesLibraryGroupSelectionTableView.target = self.libraryFavoritesDataSource;
    self.favoritesLibraryGroupSelectionTableView.delegate = _favoritesLibraryTableViewDelegate;
}

- (void)setupFavoritesLibraryViews
{
    self.favoritesLibraryView.translatesAutoresizingMaskIntoConstraints = NO;
    self.favoritesLibrarySplitView.translatesAutoresizingMaskIntoConstraints = NO;

    _favoritesLibraryGroupsTableViewScrollView =
        [NSScrollView libraryScrollViewWithDocumentView:self.favoritesLibraryGroupsTableView];
    _favoritesLibraryGroupSelectionTableViewScrollView =
        [NSScrollView libraryDetailScrollViewWithDocumentView:self.favoritesLibraryGroupSelectionTableView];

    self.favoritesLibrarySplitView.vertical = YES;
    self.favoritesLibrarySplitView.dividerStyle = NSSplitViewDividerStyleThin;
    self.favoritesLibrarySplitView.delegate = _splitViewDelegate;
    [self.favoritesLibrarySplitView addArrangedSubview:self.favoritesLibraryGroupsTableViewScrollView];
    [self.favoritesLibrarySplitView addArrangedSubview:self.favoritesLibraryGroupSelectionTableViewScrollView];
}

- (void)setupFavoritesLibraryContainerView
{
    self.favoritesLibrarySplitView.translatesAutoresizingMaskIntoConstraints = NO;

    [self.favoritesLibraryView addSubview:self.favoritesLibraryCollectionViewScrollView];
    [self.favoritesLibraryView addSubview:self.favoritesLibrarySplitView];

    [self.favoritesLibraryCollectionViewScrollView applyConstraintsToFillSuperview];
    [self.favoritesLibrarySplitView applyConstraintsToFillSuperview];
}

- (void)setupFavoritesPlaceholderView
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

- (void)setupNotifications
{
    NSNotificationCenter *notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelUpdated:)
                               name:HeimdallLibraryModelFavoriteVideoMediaListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelUpdated:)
                               name:HeimdallLibraryModelAllCachesDropped
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelUpdated:)
                               name:HeimdallLibraryModelFavoriteAudioMediaListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelUpdated:)
                               name:HeimdallLibraryModelFavoriteAlbumsListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelUpdated:)
                               name:HeimdallLibraryModelFavoriteArtistsListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelUpdated:)
                               name:HeimdallLibraryModelFavoriteGenresListReset
                             object:nil];
}

#pragma mark - HeimdallLibraryAbstractMediaLibrarySegmentViewController

- (NSArray<NSLayoutConstraint *> *)placeholderImageViewSizeConstraints
{
    return _internalPlaceholderImageViewSizeConstraints;
}

- (id<HeimdallLibraryDataSource>)currentDataSource
{
    return self.libraryFavoritesDataSource;
}

#pragma mark - Public methods

- (void)updatePresentedFavoritesView
{
    [self.libraryFavoritesDataSource reloadData];
    
    if ([self hasFavoriteItems]) {
        const HeimdallLibraryViewModeSegment viewModeSegment = HeimdallLibraryWindowPersistentPreferences.sharedInstance.favoritesLibraryViewMode;
        [self presentFavoritesLibraryView:viewModeSegment];
    } else if (self.libraryFavoritesDataSource.libraryModel.filterString.length > 0) {
        [self.libraryWindow displayNoResultsMessage];
    } else {
        [self presentPlaceholderFavoritesView];
    }
}

- (BOOL)hasFavoriteItems
{
    HeimdallLibraryModel * const libraryModel = self.libraryFavoritesDataSource.libraryModel;
    return libraryModel.numberOfFavoriteVideoMedia > 0 ||
           libraryModel.numberOfFavoriteAudioMedia > 0 ||
           libraryModel.numberOfFavoriteAlbums > 0 ||
           libraryModel.numberOfFavoriteArtists > 0 ||
           libraryModel.numberOfFavoriteGenres > 0;
}

- (void)presentFavoritesCollectionView
{
    [self.libraryWindow displayLibraryView:self.favoritesLibraryCollectionViewScrollView];
}

- (void)presentPlaceholderFavoritesView
{
    [self.libraryWindow displayLibraryPlaceholderViewWithImage:NSImage.HeimdallPlaceholderVideoImage
                                              usingConstraints:self.placeholderImageViewSizeConstraints
                                             displayingMessage:_NS("Your favorite media will appear here.\nMark media items as favorites to see them in this view.")];
}

- (void)presentFavoritesLibraryView:(HeimdallLibraryViewModeSegment)viewModeSegment
{
    [self.libraryWindow displayLibraryView:self.favoritesLibraryView];
    if (viewModeSegment == HeimdallLibraryGridViewModeSegment) {
        self.favoritesLibrarySplitView.hidden = YES;
        self.favoritesLibraryCollectionViewScrollView.hidden = NO;
    } else if (viewModeSegment == HeimdallLibraryListViewModeSegment) {
        self.favoritesLibrarySplitView.hidden = NO;
        self.favoritesLibraryCollectionViewScrollView.hidden = YES;
        [_splitViewDelegate resetDefaultSplitForSplitView:self.favoritesLibrarySplitView];
    } else {
        NSAssert(false, @"View mode must be grid or list mode");
    }
}

- (void)presentLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if (libraryItem == nil) {
        return;
    }

    _awaitingPresentingLibraryItem = libraryItem;

    const HeimdallLibraryViewModeSegment viewModeSegment = HeimdallLibraryWindowPersistentPreferences.sharedInstance.favoritesLibraryViewMode;

    if (viewModeSegment == HeimdallLibraryGridViewModeSegment) {
        [NSNotificationCenter.defaultCenter addObserver:self
                                               selector:@selector(presentLibraryItemWaitForCollectionViewDataSourceFinished:)
                                                   name:HeimdallLibraryFavoritesDataSourceDisplayedCollectionChangedNotification
                                                 object:self.libraryFavoritesDataSource];
    } else if (viewModeSegment == HeimdallLibraryListViewModeSegment) {
        [NSNotificationCenter.defaultCenter addObserver:self
                                               selector:@selector(presentLibraryItemWaitForTableViewDataSourceFinished:)
                                                   name:HeimdallLibraryFavoritesDataSourceDisplayedCollectionChangedNotification
                                                 object:self.libraryFavoritesDataSource];
    } else {
        NSAssert(false, @"View mode must be grid or list mode");
    }
}

- (void)presentLibraryItemWaitForCollectionViewDataSourceFinished:(NSNotification *)notification
{
    [NSNotificationCenter.defaultCenter removeObserver:self
                                                  name:HeimdallLibraryFavoritesDataSourceDisplayedCollectionChangedNotification
                                                object:self.libraryFavoritesDataSource];

    _awaitingPresentingLibraryItem = nil;
}

- (void)presentLibraryItemWaitForTableViewDataSourceFinished:(NSNotification *)notification
{
    [NSNotificationCenter.defaultCenter removeObserver:self
                                                  name:HeimdallLibraryFavoritesDataSourceDisplayedCollectionChangedNotification
                                                object:self.libraryFavoritesDataSource];

    const NSInteger rowForLibraryItem = [self.libraryFavoritesDataSource rowForLibraryItem:_awaitingPresentingLibraryItem];
    if (rowForLibraryItem != NSNotFound) {
        NSIndexSet * const indexSet = [NSIndexSet indexSetWithIndex:rowForLibraryItem];
        [self.favoritesLibraryGroupsTableView selectRowIndexes:indexSet byExtendingSelection:NO];
        [self.favoritesLibraryGroupsTableView scrollRowToVisible:rowForLibraryItem];
    }

    _awaitingPresentingLibraryItem = nil;
}

#pragma mark - Notification handlers

- (void)libraryModelUpdated:(NSNotification *)notification
{
    NSParameterAssert(notification);
    if (self.libraryWindow.librarySegmentType == HeimdallLibraryFavoritesSegmentType) {
        [self updatePresentedFavoritesView];
    }
}

@end
