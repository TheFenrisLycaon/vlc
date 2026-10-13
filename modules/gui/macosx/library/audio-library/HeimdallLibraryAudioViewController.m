/*****************************************************************************
 * HeimdallLibraryAudioViewController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Claudio Cambra <claudio.cambra@gmail.com>
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

#import "HeimdallLibraryAudioViewController.h"

#import "extensions/NSImage+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"
#import "extensions/NSWindow+HeimdallAdditions.h"

#import "library/HeimdallLibraryCollectionView.h"
#import "library/HeimdallLibraryCollectionViewDelegate.h"
#import "library/HeimdallLibraryCollectionViewFlowLayout.h"
#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibrarySegment.h"
#import "library/HeimdallLibraryWindowNavigationSidebarViewController.h"
#import "library/HeimdallLibraryWindowSplitViewController.h"
#import "library/HeimdallLibraryTwoPaneSplitViewDelegate.h"
#import "library/HeimdallLibraryWindow.h"
#import "library/HeimdallLibraryWindowPersistentPreferences.h"

#import "library/audio-library/HeimdallLibraryAlbumTableCellView.h"
#import "library/audio-library/HeimdallLibraryAudioDataSource.h"
#import "library/audio-library/HeimdallLibraryAudioGroupDataSource.h"
#import "library/HeimdallLibraryHeaderView.h"
#import "library/audio-library/HeimdallLibraryAudioGroupTableViewDelegate.h"
#import "library/HeimdallLibraryHeaderCell.h"
#import "library/audio-library/HeimdallLibraryAudioTableViewDelegate.h"

#import "library/playlist-library/HeimdallLibraryPlaylistViewController.h"

#import "library/video-library/HeimdallLibraryVideoViewController.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallUIUnits.h"

#import "windows/video/HeimdallVoutView.h"
#import "windows/video/HeimdallMainVideoViewController.h"

NSString *HeimdallLibraryPlaceholderAudioViewIdentifier = @"HeimdallLibraryPlaceholderAudioViewIdentifier";

@interface HeimdallLibraryAudioViewController()
{
    id<HeimdallMediaLibraryItemProtocol> _awaitingPresentingLibraryItem;

    NSArray<NSLayoutConstraint *> *_internalPlaceholderImageViewSizeConstraints;
    NSArray<NSImage *> *_placeholderImages;
    NSArray<NSString *> *_placeholderLabelStrings;

    HeimdallLibraryCollectionViewDelegate *_audioLibraryCollectionViewDelegate;
    HeimdallLibraryAudioTableViewDelegate *_audioLibraryTableViewDelegate;
    HeimdallLibraryAudioGroupTableViewDelegate *_audioGroupLibraryTableViewDelegate;
    HeimdallLibraryTwoPaneSplitViewDelegate *_splitViewDelegate;
}
@end

@implementation HeimdallLibraryAudioViewController

#pragma mark - Set up the view controller

- (instancetype)initWithLibraryWindow:(HeimdallLibraryWindow *)libraryWindow
{
    self = [super initWithLibraryWindow:libraryWindow];

    if (self) {
        [self setupPropertiesFromLibraryWindow:libraryWindow];
        [self setupAudioDataSource];

        _audioLibraryCollectionViewDelegate = [[HeimdallLibraryCollectionViewDelegate alloc] init];
        _audioLibraryTableViewDelegate = [[HeimdallLibraryAudioTableViewDelegate alloc] init];
        _audioGroupLibraryTableViewDelegate = [[HeimdallLibraryAudioGroupTableViewDelegate alloc] init];
        _splitViewDelegate = [[HeimdallLibraryTwoPaneSplitViewDelegate alloc] init];

        [self setupAudioPlaceholderView];
        [self setupAudioCollectionView];
        [self setupGridModeSplitView];
        [self setupAudioTableViews];
        [self setupAudioLibraryViews];

        NSNotificationCenter *notificationCenter = NSNotificationCenter.defaultCenter;
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelUpdated:)
                                   name:HeimdallLibraryModelAudioMediaListReset
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelUpdated:)
                                   name:HeimdallLibraryModelAudioMediaItemDeleted
                                 object:nil];

        NSString * const audioMediaResetLongLoadStartNotification = [HeimdallLibraryModelAudioMediaListReset stringByAppendingString:HeimdallLongNotificationNameStartSuffix];
        NSString * const audioMediaResetLongLoadFinishNotification = [HeimdallLibraryModelAudioMediaListReset stringByAppendingString:HeimdallLongNotificationNameFinishSuffix];
        NSString * const audioMediaDeletedLongLoadStartNotification = [HeimdallLibraryModelAudioMediaItemDeleted stringByAppendingString:HeimdallLongNotificationNameStartSuffix];
        NSString * const audioMediaDeletedLongLoadFinishNotification = [HeimdallLibraryModelAudioMediaItemDeleted stringByAppendingString:HeimdallLongNotificationNameFinishSuffix];

        [notificationCenter addObserver:self
                               selector:@selector(libraryModelLongLoadStarted:)
                                   name:audioMediaResetLongLoadStartNotification
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelLongLoadFinished:)
                                   name:audioMediaResetLongLoadFinishNotification
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelLongLoadStarted:)
                                   name:audioMediaDeletedLongLoadStartNotification
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(libraryModelLongLoadFinished:)
                                   name:audioMediaDeletedLongLoadFinishNotification
                                 object:nil];
    }

    return self;
}

- (void)setupPropertiesFromLibraryWindow:(HeimdallLibraryWindow*)libraryWindow
{
    NSParameterAssert(libraryWindow);

    _audioLibraryView = libraryWindow.audioLibraryView;
    _audioLibrarySplitView = libraryWindow.audioLibrarySplitView;
    _audioCollectionSelectionTableViewScrollView = libraryWindow.audioCollectionSelectionTableViewScrollView;
    _audioCollectionSelectionTableView = libraryWindow.audioCollectionSelectionTableView;
    _audioGroupSelectionTableViewScrollView = libraryWindow.audioGroupSelectionTableViewScrollView;
    _audioGroupSelectionTableView = libraryWindow.audioGroupSelectionTableView;
    _audioSongTableViewScrollView = libraryWindow.audioLibrarySongsTableViewScrollView;
    _audioSongTableView = libraryWindow.audioLibrarySongsTableView;
    _audioCollectionViewScrollView = libraryWindow.audioCollectionViewScrollView;
    _audioLibraryCollectionView = libraryWindow.audioLibraryCollectionView;
    _audioLibraryGridModeSplitView = libraryWindow.audioLibraryGridModeSplitView;
    _audioLibraryGridModeSplitViewListTableViewScrollView = libraryWindow.audioLibraryGridModeSplitViewListTableViewScrollView;
    _audioLibraryGridModeSplitViewListTableView = libraryWindow.audioLibraryGridModeSplitViewListTableView;
    _audioLibraryGridModeSplitViewListSelectionCollectionViewScrollView = libraryWindow.audioLibraryGridModeSplitViewListSelectionCollectionViewScrollView;
    _audioLibraryGridModeSplitViewListSelectionCollectionView = libraryWindow.audioLibraryGridModeSplitViewListSelectionCollectionView;
}

- (void)setupAudioDataSource
{
    _audioDataSource = [[HeimdallLibraryAudioDataSource alloc] init];
    _audioDataSource.libraryModel = HeimdallMain.sharedInstance.libraryController.libraryModel;
    _audioDataSource.headerDelegate = self;
    [_audioDataSource setup];

    _audioGroupDataSource = [[HeimdallLibraryAudioGroupDataSource alloc] init];
    _audioDataSource.audioGroupDataSource = _audioGroupDataSource;
}

- (void)setupAudioCollectionView
{
    _audioDataSource.collectionView = _audioLibraryCollectionView;
    _audioLibraryCollectionView.dataSource = _audioDataSource;
    _audioLibraryCollectionView.delegate = _audioLibraryCollectionViewDelegate;

    _audioLibraryCollectionView.selectable = YES;
    _audioLibraryCollectionView.allowsMultipleSelection = YES;
    _audioLibraryCollectionView.allowsEmptySelection = YES;
    _audioLibraryCollectionView.collectionViewLayout = HeimdallLibraryCollectionViewFlowLayout.standardLayout;
}

- (void)setupAudioTableViews
{
    _audioLibrarySplitView.delegate = _splitViewDelegate;

    _audioDataSource.collectionSelectionTableView = _audioCollectionSelectionTableView;
    _audioCollectionSelectionTableView.dataSource = _audioDataSource;
    _audioCollectionSelectionTableView.delegate = _audioLibraryTableViewDelegate;

    _audioCollectionHeaderView = [HeimdallLibraryHeaderView paddedHeaderView];
    _audioGroupSelectionTableView.headerView = self.audioCollectionHeaderView;
    _audioGroupSelectionTableView.tableColumns.firstObject.headerCell = [HeimdallLibraryHeaderCell new];

    _audioGroupSelectionTableView.dataSource = _audioGroupDataSource;
    _audioGroupDataSource.tableViews = @[_audioGroupSelectionTableView];
    _audioGroupSelectionTableView.delegate = _audioGroupLibraryTableViewDelegate;

    if(@available(macOS 11.0, *)) {
        _audioGroupSelectionTableView.style = NSTableViewStyleFullWidth;
    }

    _audioDataSource.songsTableView = _audioSongTableView;
    _audioSongTableView.dataSource = _audioDataSource;

    _audioDataSource.gridModeListTableView = _audioLibraryGridModeSplitViewListTableView;
    _audioLibraryGridModeSplitViewListTableView.dataSource = _audioDataSource;

    _audioLibraryGridModeSplitViewListSelectionCollectionView.dataSource = _audioGroupDataSource;
    _audioGroupDataSource.collectionView = _audioLibraryGridModeSplitViewListSelectionCollectionView;

    _audioSongTableView.delegate = _audioLibraryTableViewDelegate;

    [_audioDataSource applySelectionForTableView:_audioCollectionSelectionTableView];
}

- (void)setupGridModeSplitView
{
    _audioLibraryGridModeSplitView.delegate = _splitViewDelegate;

    _audioLibraryGridModeSplitViewListTableView.dataSource = _audioDataSource;
    _audioLibraryGridModeSplitViewListTableView.delegate = _audioLibraryTableViewDelegate;

    _audioLibraryGridModeSplitViewListSelectionCollectionView.dataSource = _audioGroupDataSource;
    _audioLibraryGridModeSplitViewListSelectionCollectionView.delegate = _audioLibraryCollectionViewDelegate;

    _audioLibraryGridModeSplitViewListSelectionCollectionView.selectable = YES;
    _audioLibraryGridModeSplitViewListSelectionCollectionView.allowsMultipleSelection = YES;
    _audioLibraryGridModeSplitViewListSelectionCollectionView.allowsEmptySelection = YES;

    HeimdallLibraryCollectionViewFlowLayout * const audioLibraryGridModeListSelectionCollectionViewLayout =
        HeimdallLibraryCollectionViewFlowLayout.standardLayout;
    _audioLibraryGridModeSplitViewListSelectionCollectionView.collectionViewLayout = audioLibraryGridModeListSelectionCollectionViewLayout;
    audioLibraryGridModeListSelectionCollectionViewLayout.headerReferenceSize =
        NSMakeSize(self.audioCollectionViewScrollView.documentView.frame.size.width, HeimdallLibraryHeaderViewHeight);

    audioLibraryGridModeListSelectionCollectionViewLayout.sectionHeadersPinToVisibleBounds = YES;

    [HeimdallLibraryAudioDataSource setupCollectionView:_audioLibraryGridModeSplitViewListSelectionCollectionView];
    [HeimdallLibraryAudioGroupDataSource setupCollectionView:_audioLibraryGridModeSplitViewListSelectionCollectionView];
}

- (void)setupAudioPlaceholderView
{
    _internalPlaceholderImageViewSizeConstraints = @[
        [NSLayoutConstraint constraintWithItem:self.placeholderImageView
                                     attribute:NSLayoutAttributeWidth
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:nil
                                     attribute:NSLayoutAttributeNotAnAttribute
                                    multiplier:0.f
                                      constant:149.f],
        [NSLayoutConstraint constraintWithItem:self.placeholderImageView
                                     attribute:NSLayoutAttributeHeight
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:nil
                                     attribute:NSLayoutAttributeNotAnAttribute
                                    multiplier:0.f
                                      constant:149.f],
    ];

    _placeholderImages = @[
        NSImage.HeimdallPlaceholderGroupImage,
        NSImage.HeimdallPlaceholderMusicImage,
        NSImage.HeimdallPlaceholderMusicImage,
        NSImage.HeimdallPlaceholderMusicImage,
    ];
    _placeholderLabelStrings = @[
        _NS("Your music artists will appear here.\nGo to the Browse section to add artists you love."),
        _NS("Your music albums will appear here.\nGo to the Browse section to add albums you love."),
        _NS("Your music tracks will appear here.\nGo to the Browse section to add tracks you love."),
        _NS("Your music genres will appear here.\nGo to the Browse section to add genres you love."),
    ];
}

- (void)setupAudioLibraryViews
{
    _audioCollectionSelectionTableView.rowHeight = HeimdallUIUnits.mediumTableViewRowHeight;
    _audioLibraryGridModeSplitViewListTableView.rowHeight = HeimdallUIUnits.mediumTableViewRowHeight;
    _audioGroupSelectionTableView.rowHeight = HeimdallLibraryAlbumTableCellView.defaultHeight;

    const NSEdgeInsets audioScrollViewContentInsets = HeimdallUIUnits.libraryViewScrollViewContentInsets;
    const NSEdgeInsets audioScrollViewScrollerInsets = HeimdallUIUnits.libraryViewScrollViewScrollerInsets;

    _audioCollectionViewScrollView.automaticallyAdjustsContentInsets = NO;
    _audioCollectionViewScrollView.contentInsets = audioScrollViewContentInsets;
    _audioCollectionViewScrollView.scrollerInsets = audioScrollViewScrollerInsets;

    _audioCollectionSelectionTableViewScrollView.automaticallyAdjustsContentInsets = NO;
    _audioCollectionSelectionTableViewScrollView.contentInsets = audioScrollViewContentInsets;
    _audioCollectionSelectionTableViewScrollView.scrollerInsets = audioScrollViewScrollerInsets;
    _audioGroupSelectionTableViewScrollView.automaticallyAdjustsContentInsets = NO;
    _audioGroupSelectionTableViewScrollView.contentInsets = HeimdallUIUnits.libraryViewScrollViewDetailListContentInsets;
    _audioGroupSelectionTableViewScrollView.scrollerInsets = audioScrollViewScrollerInsets;

    _audioLibraryGridModeSplitViewListTableViewScrollView.automaticallyAdjustsContentInsets = NO;
    _audioLibraryGridModeSplitViewListTableViewScrollView.contentInsets = audioScrollViewContentInsets;
    _audioLibraryGridModeSplitViewListTableViewScrollView.scrollerInsets = audioScrollViewScrollerInsets;

    _audioLibraryGridModeSplitViewListSelectionCollectionViewScrollView.automaticallyAdjustsContentInsets = NO;
    _audioLibraryGridModeSplitViewListSelectionCollectionViewScrollView.contentInsets = HeimdallUIUnits.libraryViewScrollViewDetailGridContentInsets;
    _audioLibraryGridModeSplitViewListSelectionCollectionViewScrollView.scrollerInsets = audioScrollViewScrollerInsets;

    // Songs table view needs bottom padding for controls bar
    const CGFloat controlsBarHeight = HeimdallUIUnits.libraryWindowControlsBarHeight;
    const CGFloat controlsBarPadding = HeimdallUIUnits.largeSpacing * 2;
    NSClipView * const clipView = _audioSongTableViewScrollView.contentView;
    const CGFloat topInset = HeimdallUIUnits.libraryWindowContentSafeTopInset + self.audioSongTableView.headerView.frame.size.height;
    clipView.automaticallyAdjustsContentInsets = NO;
    clipView.contentInsets = NSEdgeInsetsMake(topInset, 0, controlsBarHeight + controlsBarPadding, 0);
}

#pragma mark - Superclass property overrides

- (NSArray<NSLayoutConstraint *> *)placeholderImageViewSizeConstraints
{
    return _internalPlaceholderImageViewSizeConstraints;
}

- (id<HeimdallLibraryDataSource>)currentDataSource
{
    return self.audioDataSource;
}

#pragma mark - Show the audio view

- (void)presentAudioView
{
    [self updatePresentedView];
}

- (void)presentPlaceholderAudioView
{
    const NSInteger selectedLibrarySegment = self.audioDataSource.audioLibrarySegment;
    NSAssert(selectedLibrarySegment != HeimdallAudioLibraryRecentsSegment &&
             selectedLibrarySegment != HeimdallAudioLibraryUnknownSegment,
             @"Received invalid audio library segment from audio data source!");
    [self.libraryWindow displayLibraryPlaceholderViewWithImage:_placeholderImages[selectedLibrarySegment]
                                              usingConstraints:self.placeholderImageViewSizeConstraints
                                             displayingMessage:_placeholderLabelStrings[selectedLibrarySegment]];
}

- (void)hideAllViews
{
    _audioLibrarySplitView.hidden = YES;
    _audioLibraryGridModeSplitView.hidden = YES;
    _audioSongTableViewScrollView.hidden = YES;
    _audioCollectionViewScrollView.hidden = YES;
}

- (void)presentAudioGridModeView
{
    const HeimdallLibrarySegmentType selectedSegment = self.libraryWindow.librarySegmentType;
    if (selectedSegment == HeimdallLibrarySongsMusicSubSegmentType ||
        selectedSegment == HeimdallLibraryAlbumsMusicSubSegmentType) {

        [_audioLibraryCollectionView deselectAll:self];
        [(HeimdallLibraryCollectionViewFlowLayout *)_audioLibraryCollectionView.collectionViewLayout resetLayout];

        _audioCollectionViewScrollView.hidden = NO;
    } else {
        _audioLibraryGridModeSplitView.hidden = NO;
        [_splitViewDelegate resetDefaultSplitForSplitView:self.audioLibraryGridModeSplitView];
    }
}

- (void)presentAudioTableView
{
    if (self.libraryWindow.librarySegmentType == HeimdallLibrarySongsMusicSubSegmentType) {
        _audioSongTableViewScrollView.hidden = NO;
    } else {
        _audioLibrarySplitView.hidden = NO;
        [_splitViewDelegate resetDefaultSplitForSplitView:self.audioLibrarySplitView];
    }
}

- (HeimdallLibraryViewModeSegment)viewModeSegmentForCurrentLibrarySegment
{
    HeimdallLibraryWindowPersistentPreferences * const libraryWindowPrefs = HeimdallLibraryWindowPersistentPreferences.sharedInstance;

    switch (self.libraryWindow.librarySegmentType) {
        case HeimdallLibraryArtistsMusicSubSegmentType:
            return libraryWindowPrefs.artistLibraryViewMode;
        case HeimdallLibraryGenresMusicSubSegmentType:
            return libraryWindowPrefs.genreLibraryViewMode;
        case HeimdallLibrarySongsMusicSubSegmentType:
            return libraryWindowPrefs.songsLibraryViewMode;
        case HeimdallLibraryAlbumsMusicSubSegmentType:
            return libraryWindowPrefs.albumLibraryViewMode;
        default:
            return HeimdallLibraryGridViewModeSegment;
    }
}

- (HeimdallAudioLibrarySegment)currentLibrarySegmentToAudioLibrarySegment
{
    switch (self.libraryWindow.librarySegmentType) {
        case HeimdallLibraryMusicSegmentType:
        case HeimdallLibraryArtistsMusicSubSegmentType:
            return HeimdallAudioLibraryArtistsSegment;
        case HeimdallLibraryAlbumsMusicSubSegmentType:
            return HeimdallAudioLibraryAlbumsSegment;
        case HeimdallLibrarySongsMusicSubSegmentType:
            return HeimdallAudioLibrarySongsSegment;
        case HeimdallLibraryGenresMusicSubSegmentType:
            return HeimdallAudioLibraryGenresSegment;
        default:
            NSAssert(false, @"Non-audio or unknown library subsegment received");
            return HeimdallAudioLibraryUnknownSegment;
    }
}

- (void)updatePresentedView
{
    self.audioDataSource.audioLibrarySegment = [self currentLibrarySegmentToAudioLibrarySegment];
    const BOOL hasResultsForCurrentSegment = self.audioDataSource.collectionToDisplayCount > 0;

    if (hasResultsForCurrentSegment) {
        [self.libraryWindow displayLibraryView:self.audioLibraryView];
        [self hideAllViews];

        const HeimdallLibraryViewModeSegment viewModeSegment = [self viewModeSegmentForCurrentLibrarySegment];

        if (viewModeSegment == HeimdallLibraryListViewModeSegment) {
            [self presentAudioTableView];
        } else if (viewModeSegment == HeimdallLibraryGridViewModeSegment) {
            [self presentAudioGridModeView];
        } else {
            NSAssert(false, @"View mode must be grid or list mode");
        }

        [HeimdallMain.sharedInstance.libraryWindow updateGridVsListViewModeSegmentedControl];
    } else if (self.audioDataSource.libraryModel.filterString.length > 0) {
        [self.libraryWindow displayNoResultsMessage];
    } else {
        [self presentPlaceholderAudioView];
    }
}

- (void)reloadData
{
    [_audioDataSource reloadData];
}

- (void)presentLibraryItemInTableView:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if (libraryItem == nil) {
        return;
    }

    NSTableView *targetMainTableView;
    if ([libraryItem isKindOfClass:HeimdallMediaLibraryMediaItem.class]) {
        targetMainTableView = self.audioSongTableView;
    } else if ([self viewModeSegmentForCurrentLibrarySegment] == HeimdallLibraryGridViewModeSegment) {
        targetMainTableView = self.audioLibraryGridModeSplitViewListTableView;
    } else {
        targetMainTableView = self.audioCollectionSelectionTableView;
    }
    NSAssert(targetMainTableView != nil, @"Target tableview for presenting audio library view is nil");
    NSAssert(targetMainTableView.dataSource == self.audioDataSource, @"Target tableview data source is unexpected");

    const NSInteger rowForLibraryItem = [self.audioDataSource rowForLibraryItem:libraryItem];
    if (rowForLibraryItem != NSNotFound) {
        NSIndexSet * const indexSet = [NSIndexSet indexSetWithIndex:rowForLibraryItem];
        [self.audioDataSource tableView:targetMainTableView selectRowIndices:indexSet];
    }
}

- (void)presentLibraryItemInCollectionView:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if (libraryItem == nil) {
        return;
    }

    // If we are handling a library item that is shown in one of the split view modes,
    // with a table view on the left and the collection view on the right, defer back
    // to presentLibraryItemInTabelView
    if ([libraryItem isKindOfClass:HeimdallMediaLibraryGenre.class] ||
        [libraryItem isKindOfClass:HeimdallMediaLibraryArtist.class]) {
        [self presentLibraryItemInTableView:libraryItem];
    }

    NSIndexPath * const indexPathForLibraryItem = [self.audioDataSource indexPathForLibraryItem:libraryItem];
    if (indexPathForLibraryItem) {
        NSSet<NSIndexPath *> * const indexPathSet = [NSSet setWithObject:indexPathForLibraryItem];
        NSCollectionView * const collectionView = self.audioLibraryCollectionView;
        HeimdallLibraryCollectionViewFlowLayout * const expandableFlowLayout = (HeimdallLibraryCollectionViewFlowLayout *)collectionView.collectionViewLayout;

        [collectionView selectItemsAtIndexPaths:indexPathSet
                                 scrollPosition:NSCollectionViewScrollPositionTop];
        [expandableFlowLayout expandDetailSectionAtIndex:indexPathForLibraryItem];
    }
}

- (void)presentLibraryItemWaitForDataSourceFinished:(nullable NSNotification *)aNotification
{
    if ((NSUInteger)self.audioDataSource.displayedCollectionCount < self.audioDataSource.collectionToDisplayCount) {
        return;
    }
    
    [NSNotificationCenter.defaultCenter removeObserver:self
                                                  name:HeimdallLibraryAudioDataSourceDisplayedCollectionChangedNotification
                                                object:self.audioDataSource];

    const HeimdallLibraryViewModeSegment viewModeSegment = [self viewModeSegmentForCurrentLibrarySegment];
    if (viewModeSegment == HeimdallLibraryListViewModeSegment) {
        [self presentLibraryItemInTableView:_awaitingPresentingLibraryItem];
    } else if (viewModeSegment == HeimdallLibraryGridViewModeSegment) {
        [self presentLibraryItemInCollectionView:_awaitingPresentingLibraryItem];
    } else {
        NSAssert(NO, @"No valid view mode segment acquired, cannot present item!");
    }

    _awaitingPresentingLibraryItem = nil;
}

- (void)presentLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if (libraryItem == nil) {
        return;
    }

    _awaitingPresentingLibraryItem = libraryItem;

    // If the library item is a media item, we need to select the corresponding segment
    // in the segmented control. We then need to update the presented view.
    HeimdallLibrarySegmentType segmentType;
    if ([libraryItem isKindOfClass:HeimdallMediaLibraryAlbum.class]) {
        segmentType = HeimdallLibraryAlbumsMusicSubSegmentType;
    } else if ([libraryItem isKindOfClass:HeimdallMediaLibraryArtist.class]) {
        segmentType = HeimdallLibraryArtistsMusicSubSegmentType;
    } else if ([libraryItem isKindOfClass:HeimdallMediaLibraryGenre.class]) {
        segmentType = HeimdallLibraryGenresMusicSubSegmentType;
    } else {
        segmentType = HeimdallLibrarySongsMusicSubSegmentType;
    }

    HeimdallLibraryWindow * const libraryWindow = self.libraryWindow;
    const BOOL segmentAlreadySelected = segmentType == libraryWindow.librarySegmentType;
    if (segmentAlreadySelected &&
        !self.audioDataSource.displayedCollectionUpdating &&
        (NSUInteger)self.audioDataSource.displayedCollectionCount >= self.audioDataSource.collectionToDisplayCount) {
        [self presentLibraryItemWaitForDataSourceFinished:nil];
        return;
    }

    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(presentLibraryItemWaitForDataSourceFinished:)
                                               name:HeimdallLibraryAudioDataSourceDisplayedCollectionChangedNotification
                                             object:self.audioDataSource];

    if (!segmentAlreadySelected) {
        libraryWindow.librarySegmentType = segmentType;
        [libraryWindow.splitViewController.navSidebarViewController selectSegment:segmentType];
    }
}

- (void)libraryModelUpdated:(NSNotification *)aNotification
{
    HeimdallLibraryModel * const model = HeimdallMain.sharedInstance.libraryController.libraryModel;
    const NSUInteger audioCount = model.numberOfAudioMedia;

    if ((self.libraryWindow.librarySegmentType == HeimdallLibraryMusicSegmentType ||
         self.libraryWindow.librarySegmentType == HeimdallLibrarySongsMusicSubSegmentType ||
         self.libraryWindow.librarySegmentType == HeimdallLibraryArtistsMusicSubSegmentType ||
         self.libraryWindow.librarySegmentType == HeimdallLibraryAlbumsMusicSubSegmentType ||
         self.libraryWindow.librarySegmentType == HeimdallLibraryGenresMusicSubSegmentType) &&
        ((audioCount == 0 && ![self.libraryTargetView.subviews containsObject:self.emptyLibraryView]) ||
         (audioCount > 0 && ![self.libraryTargetView.subviews containsObject:_audioLibraryView])) &&
        !self.libraryWindow.embeddedVideoPlaybackActive) {

        [self updatePresentedView];
    }
}

- (void)libraryModelLongLoadStarted:(NSNotification *)notification
{
    if (self.connected) {
        [self.audioDataSource disconnect];
        [self.audioGroupDataSource disconnect];
    }

    [self.libraryWindow showLoadingOverlay];
}

- (void)libraryModelLongLoadFinished:(NSNotification *)notification
{
    if (self.connected) {
        [self.audioDataSource connect];
        [self.audioGroupDataSource connect];
    }

    [self.libraryWindow hideLoadingOverlay];
}

- (void)updateHeaderForTableView:(NSTableView *)tableView
             withRepresentedItem:(nullable HeimdallLibraryRepresentedItem *)representedItem
                   fallbackTitle:(NSString *)fallbackTitle
                  fallbackDetail:(NSString *)fallbackDetail
{
    if (tableView != self.audioCollectionSelectionTableView &&
        tableView != self.audioLibraryGridModeSplitViewListTableView &&
        tableView != self.audioGroupSelectionTableView &&
        ![self.audioGroupDataSource.tableViews containsObject:tableView])
        return;

    if (representedItem != nil) {
        self.audioCollectionHeaderView.representedItem = representedItem;
    } else {
        [self.audioCollectionHeaderView updateWithRepresentedItem:nil
                                                    fallbackTitle:fallbackTitle
                                                   fallbackDetail:fallbackDetail];
    }
}

@end
