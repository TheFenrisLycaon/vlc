/*****************************************************************************
 * HeimdallLibraryPlaylistViewController.m: MacOS X interface module
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

#import "HeimdallLibraryPlaylistViewController.h"

#import "extensions/NSImage+HeimdallAdditions.h"
#import "extensions/NSScrollView+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"
#import "extensions/NSView+HeimdallAdditions.h"

#import "library/HeimdallLibraryCollectionView.h"
#import "library/HeimdallLibraryCollectionViewDelegate.h"
#import "library/HeimdallLibraryCollectionViewFlowLayout.h"
#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryHeaderCell.h"
#import "library/HeimdallLibraryHeaderView.h"
#import "library/HeimdallLibraryMasterDetailViewTableViewDelegate.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibraryTableCellView.h"
#import "library/HeimdallLibraryTableView.h"
#import "library/HeimdallLibraryTwoPaneSplitViewDelegate.h"
#import "library/HeimdallLibraryWindow.h"
#import "library/HeimdallLibraryWindowPersistentPreferences.h"

#import "library/audio-library/HeimdallLibraryAudioViewController.h"

#import "library/playlist-library/HeimdallLibraryPlaylistDataSource.h"

#import "library/video-library/HeimdallLibraryVideoViewController.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallUIUnits.h"

#import "windows/video/HeimdallMainVideoViewController.h"

@interface HeimdallLibraryPlaylistViewController ()
{
    NSArray<NSLayoutConstraint *> *_internalPlaceholderImageViewSizeConstraints;
}
@end

@implementation HeimdallLibraryPlaylistViewController

- (instancetype)initWithLibraryWindow:(HeimdallLibraryWindow *)libraryWindow
{
    self = [super initWithLibraryWindow:libraryWindow];

    if (self) {
        _dataSource = [[HeimdallLibraryPlaylistDataSource alloc] init];
        _splitViewDelegate = [[HeimdallLibraryTwoPaneSplitViewDelegate alloc] init];

        [self setupPlaylistCollectionView];
        [self setupPlaylistTableView];
        [self setupPlaylistLibraryContainerView];
        [self setupPlaylistPlaceholderView];

        NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelUpdated:)
                                   name:HeimdallLibraryModelPlaylistAdded
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelUpdated:)
                                   name:HeimdallLibraryModelAllCachesDropped
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelPlaylistDeleted:)
                                   name:HeimdallLibraryModelPlaylistDeleted
                                 object:nil];

        NSString * const playlistListResetLongLoadStartNotification = [HeimdallLibraryModelPlaylistAdded stringByAppendingString:HeimdallLongNotificationNameStartSuffix];
        NSString * const playlistListResetLongLoadFinishNotification = [HeimdallLibraryModelPlaylistAdded stringByAppendingString:HeimdallLongNotificationNameFinishSuffix];
        NSString * const playlistDeletedLongLoadStartNotification = [HeimdallLibraryModelPlaylistDeleted stringByAppendingString:HeimdallLongNotificationNameStartSuffix];
        NSString * const playlistDeletedLongLoadFinishNotification = [HeimdallLibraryModelPlaylistDeleted stringByAppendingString:HeimdallLongNotificationNameFinishSuffix];

        [notificationCenter addObserver:self
                               selector:@selector(libraryModelLongLoadStarted:)
                                   name:playlistListResetLongLoadStartNotification
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelLongLoadFinished:)
                                   name:playlistListResetLongLoadFinishNotification
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelLongLoadStarted:)
                                   name:playlistDeletedLongLoadStartNotification
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelLongLoadFinished:)
                                   name:playlistDeletedLongLoadFinishNotification
                                 object:nil];
    }

    return self;
}

- (void)setupPlaylistCollectionView
{
    _libraryView = [[NSView alloc] init];
    _collectionViewDelegate = [[HeimdallLibraryCollectionViewDelegate alloc] init];
    _collectionView = [[HeimdallLibraryCollectionView alloc] init];

    _collectionViewScrollView = [NSScrollView libraryScrollViewWithDocumentView:_collectionView];

    _collectionView.delegate = _collectionViewDelegate;
    _collectionView.collectionViewLayout = HeimdallLibraryCollectionViewFlowLayout.standardLayout;
    _collectionView.selectable = YES;
    _collectionView.allowsMultipleSelection = YES;
    _collectionView.allowsEmptySelection = YES;

    self.dataSource.collectionViews = @[self.collectionView];
}

- (void)setupPlaylistTableView
{
    _tableViewDelegate = [[HeimdallLibraryMasterDetailViewTableViewDelegate alloc] init];
    _masterTableView = [[HeimdallLibraryTableView alloc] init];
    _detailTableView = [[HeimdallLibraryTableView alloc] init];
    _listViewSplitView = [[NSSplitView alloc] init];

    self.dataSource.headerDelegate = _tableViewDelegate;

    self.listViewSplitView.translatesAutoresizingMaskIntoConstraints = NO;
    self.masterTableView.translatesAutoresizingMaskIntoConstraints = NO;
    self.detailTableView.translatesAutoresizingMaskIntoConstraints = NO;

    _masterTableViewScrollView = [NSScrollView libraryScrollViewWithDocumentView:self.masterTableView];
    _detailTableViewScrollView = [NSScrollView libraryDetailScrollViewWithDocumentView:self.detailTableView];

    self.listViewSplitView.vertical = YES;
    self.listViewSplitView.dividerStyle = NSSplitViewDividerStyleThin;
    self.listViewSplitView.delegate = self.splitViewDelegate;
    [self.listViewSplitView addArrangedSubview:self.masterTableViewScrollView];
    [self.listViewSplitView addArrangedSubview:self.detailTableViewScrollView];

    NSTableColumn * const masterColumn = [[NSTableColumn alloc] initWithIdentifier:@"playlists"];
    NSTableColumn * const detailColumn =
        [[NSTableColumn alloc] initWithIdentifier:@"selectedPlaylist"];

    [self.masterTableView addTableColumn:masterColumn];
    [self.detailTableView addTableColumn:detailColumn];

    NSNib * const tableCellViewNib =
        [[NSNib alloc] initWithNibNamed:NSStringFromClass(HeimdallLibraryTableCellView.class)
                                 bundle:nil];
    [self.masterTableView registerNib:tableCellViewNib
                        forIdentifier:@"HeimdallLibraryTableViewCellIdentifier"];
    [self.detailTableView registerNib:tableCellViewNib
                        forIdentifier:@"HeimdallLibraryTableViewCellIdentifier"];

    _detailTableHeaderView = [HeimdallLibraryHeaderView paddedHeaderView];

    self.tableViewDelegate.detailTableHeaderView = self.detailTableHeaderView;

    self.masterTableView.headerView = nil;
    self.masterTableView.allowsEmptySelection = NO;
    self.detailTableView.headerView = self.detailTableHeaderView;
    self.detailTableView.allowsMultipleSelection = YES;
    [self.detailTableView registerForDraggedTypes:@[
        HeimdallMediaLibraryMediaItemPasteboardType,
        HeimdallMediaLibraryMediaItemUTI
    ]];

    detailColumn.headerCell = [HeimdallLibraryHeaderCell new];

    self.masterTableView.rowHeight = HeimdallUIUnits.mediumTableViewRowHeight;
    self.detailTableView.rowHeight = HeimdallUIUnits.mediumTableViewRowHeight;

    self.masterTableView.delegate = self.tableViewDelegate;
    self.detailTableView.delegate = self.tableViewDelegate;

    self.dataSource.masterTableView = self.masterTableView;
    self.dataSource.detailTableView = self.detailTableView;

    self.masterTableView.dataSource = self.dataSource;
    self.detailTableView.dataSource = self.dataSource;
}

- (void)setupPlaylistPlaceholderView
{
    _internalPlaceholderImageViewSizeConstraints = @[
        [NSLayoutConstraint constraintWithItem:self.libraryWindow.placeholderImageView
                                     attribute:NSLayoutAttributeWidth
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:nil
                                     attribute:NSLayoutAttributeNotAnAttribute
                                    multiplier:0.f
                                      constant:149.f],
        [NSLayoutConstraint constraintWithItem:self.libraryWindow.placeholderImageView
                                     attribute:NSLayoutAttributeHeight
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:nil
                                     attribute:NSLayoutAttributeNotAnAttribute
                                    multiplier:0.f
                                      constant:149.f],
    ];
}

- (NSArray<NSLayoutConstraint *> *)placeholderImageViewSizeConstraints
{
    return _internalPlaceholderImageViewSizeConstraints;
}

- (id<HeimdallLibraryDataSource>)currentDataSource
{
    return self.dataSource;
}

- (void)presentPlaceholderPlaylistLibraryView
{
    const heimdall_ml_playlist_type_t playlistType = self.dataSource.playlistType;
    NSString *placeholderPlaylistsString = nil;
    switch (playlistType) {
        case HEIMDALL_ML_PLAYLIST_TYPE_ALL:
            placeholderPlaylistsString =
                _NS("Your playlists will appear here.\n"
                    "Go to the Browse section to add playlists you love.");
            break;
        case HEIMDALL_ML_PLAYLIST_TYPE_AUDIO:
        case HEIMDALL_ML_PLAYLIST_TYPE_AUDIO_ONLY:
            placeholderPlaylistsString =
                _NS("Your music playlists will appear here.\n"
                    "Go to the Browse section to add playlists you love.");
            break;
        case HEIMDALL_ML_PLAYLIST_TYPE_VIDEO:
        case HEIMDALL_ML_PLAYLIST_TYPE_VIDEO_ONLY:
            placeholderPlaylistsString =
                _NS("Your video playlists will appear here.\n"
                    "Go to the Browse section to add playlists you love.");
            break;
    }

    [self.libraryWindow displayLibraryPlaceholderViewWithImage:NSImage.HeimdallPlaceholderGroupImage
                                              usingConstraints:self.placeholderImageViewSizeConstraints
                                             displayingMessage:placeholderPlaylistsString];
}

- (void)presentPlaylistLibraryView
{
    [self.libraryWindow displayLibraryView:self.libraryView];
    const HeimdallLibraryViewModeSegment viewModeSegment =
        HeimdallLibraryWindowPersistentPreferences.sharedInstance.playlistLibraryViewMode;

    if (viewModeSegment == HeimdallLibraryGridViewModeSegment) {
        self.listViewSplitView.hidden = YES;
        self.collectionViewScrollView.hidden = NO;
    } else {
        self.listViewSplitView.hidden = NO;
        self.collectionViewScrollView.hidden = YES;
        [self.splitViewDelegate resetDefaultSplitForSplitView:self.listViewSplitView];
        [self.dataSource updateHeaderInTableView:self.detailTableView
                              forMasterSelection:self.masterTableView];
    }
}

- (void)setupPlaylistLibraryContainerView
{
    self.collectionViewScrollView.translatesAutoresizingMaskIntoConstraints = NO;
    self.listViewSplitView.translatesAutoresizingMaskIntoConstraints = NO;

    [self.libraryView addSubview:self.collectionViewScrollView];
    [self.libraryView addSubview:self.listViewSplitView];

    [self.collectionViewScrollView applyConstraintsToFillSuperview];
    [self.listViewSplitView applyConstraintsToFillSuperview];
}

- (void)updatePresentedView
{
    const heimdall_ml_playlist_type_t playlistType = self.dataSource.playlistType;
    HeimdallLibraryModel * const libraryModel = HeimdallMain.sharedInstance.libraryController.libraryModel;
    [self.dataSource reloadData];
    if ([libraryModel numberOfPlaylistsOfType:playlistType] > 0) {
        [self presentPlaylistLibraryView];
    } else if (self.dataSource.libraryModel.filterString.length > 0) {
        [self.libraryWindow displayNoResultsMessage];
    } else {
        [self presentPlaceholderPlaylistLibraryView];
    }
}

- (void)presentPlaylistsView
{
    [self updatePresentedView];
}

- (void)presentPlaylistsViewForPlaylistType:(enum heimdall_ml_playlist_type_t)playlistType
{
    self.dataSource.playlistType = playlistType;
    [self presentPlaylistsView];
}

- (void)libraryModelUpdated:(NSNotification *)notification
{
    HeimdallLibraryModel * const model = HeimdallMain.sharedInstance.libraryController.libraryModel;
    const heimdall_ml_playlist_type_t playlistType = self.dataSource.playlistType;
    const size_t numberOfPlaylists = [model numberOfPlaylistsOfType:playlistType];
    const BOOL isPlaylistSegmentActive =
        self.libraryWindow.librarySegmentType == HeimdallLibraryPlaylistsSegmentType ||
        self.libraryWindow.librarySegmentType == HeimdallLibraryPlaylistsMusicOnlyPlaylistsSubSegmentType ||
        self.libraryWindow.librarySegmentType == HeimdallLibraryPlaylistsVideoOnlyPlaylistsSubSegmentType;
    const BOOL shouldRefreshForFilterChange =
        [notification.name isEqualToString:HeimdallLibraryModelAllCachesDropped];

    if (isPlaylistSegmentActive &&
        (shouldRefreshForFilterChange ||
         (numberOfPlaylists == 0 && ![self.libraryWindow.libraryTargetView.subviews containsObject:self.libraryWindow.emptyLibraryView]) ||
         (numberOfPlaylists > 0 && ![self.libraryWindow.libraryTargetView.subviews containsObject:_collectionViewScrollView])) &&
        !self.libraryWindow.embeddedVideoPlaybackActive) {

        [self updatePresentedView];
    }
}

- (void)libraryModelPlaylistDeleted:(NSNotification *)notification
{
    NSParameterAssert(notification);
    if (self.libraryWindow.librarySegmentType == HeimdallLibraryPlaylistsSegmentType &&
        !self.libraryWindow.embeddedVideoPlaybackActive) {
        [self updatePresentedView];
    }
}

- (void)libraryModelLongLoadStarted:(NSNotification *)notification
{
    if (self.connected) {
        [self.dataSource disconnect];
    }
    [self.libraryWindow showLoadingOverlay];
}

- (void)libraryModelLongLoadFinished:(NSNotification *)notification
{
    if (self.connected) {
        [self.dataSource connect];
    }
    [self.libraryWindow hideLoadingOverlay];
}

@end
