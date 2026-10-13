/*****************************************************************************
 * HeimdallLibrarySearchViewController.m: MacOS X interface module
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

#import "HeimdallLibrarySearchViewController.h"

#import "HeimdallLibrarySearchDataSource.h"
#import "HeimdallLibrarySearchProvider.h"

#import "extensions/NSFont+HeimdallAdditions.h"
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
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibrarySectionedTableViewDelegate.h"
#import "library/HeimdallLibraryTableCellView.h"
#import "library/HeimdallLibraryTableView.h"
#import "library/HeimdallLibraryWindow.h"
#import "library/HeimdallLibraryWindowPersistentPreferences.h"

#import "library/audio-library/HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailView.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallUIUnits.h"

@interface HeimdallLibrarySearchViewController ()

@property (readwrite) HeimdallLibraryCollectionViewDelegate *collectionViewDelegate;
@property (readwrite) HeimdallLibraryCollectionViewFlowLayout *collectionViewLayout;
@property (readwrite) HeimdallLibrarySectionedTableViewDelegate *tableViewDelegate;
@property (readwrite) NSArray<NSLayoutConstraint *> *internalPlaceholderImageViewSizeConstraints;
@property (readwrite, nullable) NSView *presentationContainer;
@property (readwrite, copy, nullable) NSString *currentQuery;
@property (readwrite) NSView *backgroundView;
@property (readwrite, nullable) NSTimer *searchDebounceTimer;

@end

@implementation HeimdallLibrarySearchViewController

- (instancetype)initWithLibraryWindow:(HeimdallLibraryWindow *)libraryWindow
{
    self = [super initWithLibraryWindow:libraryWindow];
    if (self) {
        _dataSource = [[HeimdallLibrarySearchDataSource alloc] init];
        self.dataSource.viewMode = HeimdallLibrarySmallestSentinelViewModeSegment;

        [self setupBackgroundView];
        [self setupStatusLabel];
        [self setupCollectionView];
        [self setupTableView];
        [self setupPlaceholderView];

        [NSNotificationCenter.defaultCenter addObserver:self
                                               selector:@selector(dataSourceDidReload:)
                                                   name:HeimdallLibrarySearchDataSourceDidReloadNotification
                                                 object:self.dataSource];
    }
    return self;
}

- (void)dealloc
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)dataSourceDidReload:(NSNotification *)notification
{
    [self updatePresentationForQuery:self.currentQuery ?: @""];
}

#pragma mark - Setup

- (void)setupBackgroundView
{
    _backgroundView = [[NSView alloc] init];
    self.backgroundView.translatesAutoresizingMaskIntoConstraints = NO;
    self.backgroundView.wantsLayer = YES;
    self.backgroundView.layer.backgroundColor = NSColor.windowBackgroundColor.CGColor;
}

- (void)setupStatusLabel
{
    _statusLabel = [NSTextField labelWithString:_NS("Search your library")];
    self.statusLabel.translatesAutoresizingMaskIntoConstraints = NO;
    self.statusLabel.font = NSFont.HeimdalllibrarySectionHeaderFont;
    self.statusLabel.alignment = NSTextAlignmentCenter;
}

- (void)setupCollectionView
{
    _collectionViewLayout = [[HeimdallLibraryCollectionViewFlowLayout alloc] init];
    const CGFloat collectionItemSpacing = HeimdallUIUnits.collectionViewItemSpacing;
    const NSEdgeInsets collectionViewSectionInset = HeimdallUIUnits.collectionViewSectionInsets;
    self.collectionViewLayout.headerReferenceSize = HeimdallLibraryCollectionViewSupplementaryElementView.defaultHeaderSize;
    self.collectionViewLayout.minimumLineSpacing = collectionItemSpacing;
    self.collectionViewLayout.minimumInteritemSpacing = collectionItemSpacing;
    self.collectionViewLayout.sectionInset = collectionViewSectionInset;

    _collectionView = [[HeimdallLibraryCollectionView alloc] initWithFrame:NSZeroRect];
    self.collectionView.collectionViewLayout = self.collectionViewLayout;
    self.collectionView.selectable = YES;
    self.collectionView.allowsEmptySelection = YES;
    self.collectionView.allowsMultipleSelection = YES;

    _collectionViewDelegate = [[HeimdallLibraryCollectionViewDelegate alloc] init];
    self.collectionViewDelegate.itemsAspectRatio = HeimdallLibraryCollectionViewItemAspectRatioDefaultItem;
    self.collectionViewDelegate.staticItemSize = HeimdallUIUnits.defaultMediaItemCollectionViewItemSize;
    self.collectionView.delegate = self.collectionViewDelegate;

    [self.collectionView registerClass:HeimdallLibraryCollectionViewItem.class
                 forItemWithIdentifier:HeimdallLibraryCollectionViewItemIdentifier];

    [self.collectionView registerClass:HeimdallLibraryCollectionViewSupplementaryElementView.class
            forSupplementaryViewOfKind:NSCollectionElementKindSectionHeader
                        withIdentifier:HeimdallLibrarySupplementaryElementViewIdentifier];

    NSString * const mediaItemDetailViewString =
        NSStringFromClass(HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.class);
    NSNib * const mediaItemDetailViewNib =
        [[NSNib alloc] initWithNibNamed:mediaItemDetailViewString bundle:nil];
    [self.collectionView registerNib:mediaItemDetailViewNib
         forSupplementaryViewOfKind:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind
                     withIdentifier:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewIdentifier];

    NSString * const albumDetailViewString =
        NSStringFromClass(HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView.class);
    NSNib * const albumDetailViewNib =
        [[NSNib alloc] initWithNibNamed:albumDetailViewString bundle:nil];
    [self.collectionView registerNib:albumDetailViewNib
         forSupplementaryViewOfKind:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind
                     withIdentifier:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind];

    NSString * const audioGroupDetailViewString =
        NSStringFromClass(HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailView.class);
    NSNib * const audioGroupDetailViewNib =
        [[NSNib alloc] initWithNibNamed:audioGroupDetailViewString bundle:nil];
    [self.collectionView registerNib:audioGroupDetailViewNib
         forSupplementaryViewOfKind:HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailViewKind
                     withIdentifier:HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailViewKind];

    _collectionViewScrollView = [NSScrollView libraryScrollViewWithDocumentView:self.collectionView
                                                                  contentInsets:HeimdallUIUnits.libraryViewScrollViewContentInsets];
    
    self.dataSource.collectionView = self.collectionView;
    self.collectionView.dataSource = self.dataSource;
}

- (void)setupTableView
{
    _tableView = [[HeimdallLibraryTableView alloc] init];
    self.tableView.headerView = nil;
    self.tableView.rowHeight = HeimdallUIUnits.mediumTableViewRowHeight;
    self.tableView.allowsMultipleSelection = YES;
    self.tableView.floatsGroupRows = NO;

    NSTableColumn * const column =
        [[NSTableColumn alloc] initWithIdentifier:@"HeimdallSearchLibraryTableViewColumnIdentifier"];
    column.resizingMask = NSTableColumnAutoresizingMask;
    [self.tableView addTableColumn:column];

    NSNib * const tableCellViewNib =
        [[NSNib alloc] initWithNibNamed:NSStringFromClass(HeimdallLibraryTableCellView.class) bundle:nil];
    [self.tableView registerNib:tableCellViewNib
                  forIdentifier:HeimdallLibraryTableCellViewIdentifier];

    _tableViewDelegate = [[HeimdallLibrarySectionedTableViewDelegate alloc] init];
    self.tableView.delegate = self.tableViewDelegate;

    _tableViewScrollView = [NSScrollView libraryScrollViewWithDocumentView:self.tableView
                                                             contentInsets:HeimdallUIUnits.libraryViewScrollViewContentInsets];

    self.tableView.dataSource = self.dataSource;
    self.tableView.delegate = self.tableViewDelegate;
    self.dataSource.tableView = self.tableView;
}

- (void)setupPlaceholderView
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
                                      constant:182.f],
    ];
}

#pragma mark - Abstract overrides

- (NSArray<NSLayoutConstraint *> *)placeholderImageViewSizeConstraints
{
    return self.internalPlaceholderImageViewSizeConstraints;
}

- (id<HeimdallLibraryDataSource>)currentDataSource
{
    return self.dataSource;
}

#pragma mark - Presentation

- (void)presentInContainer:(NSView *)container
{
    NSParameterAssert(container);
    const HeimdallLibraryViewModeSegment viewMode =
        HeimdallLibraryWindowPersistentPreferences.sharedInstance.searchLibraryViewMode;
    if (self.presentationContainer == container && self.dataSource.viewMode == viewMode) {
        return;
    }

    if (self.presentationContainer != container) {
        [self.backgroundView removeFromSuperview];
        [self.collectionViewScrollView removeFromSuperview];
        [self.tableViewScrollView removeFromSuperview];
        [self.statusLabel removeFromSuperview];

        [container addSubview:self.backgroundView];
        [self.backgroundView applyConstraintsToFillSuperview];

        [container addSubview:self.statusLabel];
        [NSLayoutConstraint activateConstraints:@[
            [self.statusLabel.centerXAnchor constraintEqualToAnchor:container.centerXAnchor],
            [self.statusLabel.centerYAnchor constraintEqualToAnchor:container.centerYAnchor],
        ]];
        self.statusLabel.hidden = YES;
    }

    if (self.dataSource.viewMode != viewMode || self.presentationContainer != container) {
        if (viewMode == HeimdallLibraryGridViewModeSegment) {
            [self.tableViewScrollView removeFromSuperview];
            [container addSubview:self.collectionViewScrollView positioned:NSWindowBelow relativeTo:self.statusLabel];
            [self.collectionViewScrollView applyConstraintsToFillSuperview];
        } else if (viewMode == HeimdallLibraryListViewModeSegment) {
            [self.collectionViewScrollView removeFromSuperview];
            [container addSubview:self.tableViewScrollView positioned:NSWindowBelow relativeTo:self.statusLabel];
            [self.tableViewScrollView applyConstraintsToFillSuperview];
        }
        self.dataSource.viewMode = viewMode;
        [self updatePresentationForQuery:self.currentQuery ?: @""];
    }

    self.presentationContainer = container;
}

- (void)dismissFromContainer
{
    if (self.presentationContainer == nil) {
        return;
    }
    [self.backgroundView removeFromSuperview];
    [self.collectionViewScrollView removeFromSuperview];
    [self.tableViewScrollView removeFromSuperview];
    [self.statusLabel removeFromSuperview];
    [self.searchDebounceTimer invalidate];
    self.searchDebounceTimer = nil;
    self.presentationContainer = nil;
}

- (void)updatePresentationForQuery:(NSString *)query
{
    NSParameterAssert(query);

    if (self.presentationContainer == nil) {
        return;
    }

    const BOOL hasSearchText = query.length > 0;
    const BOOL hasResults = hasSearchText && [self hasAnyResults];

    self.statusLabel.hidden = hasResults;
    if (!self.statusLabel.hidden) {
        self.statusLabel.stringValue = hasSearchText
            ? _NS("No results")
            : _NS("Search your library");
    }
}

- (BOOL)hasAnyResults
{
    const NSInteger sectionCount =
        [self.dataSource numberOfSectionsInCollectionView:self.collectionView];
    for (NSInteger i = 0; i < sectionCount; i++) {
        if ([self.dataSource collectionView:self.collectionView numberOfItemsInSection:i] > 0) {
            return YES;
        }
    }
    return NO;
}

#pragma mark - Search driving

- (void)searchForString:(NSString *)searchString
{
    NSParameterAssert(searchString);
    if (searchString.length == 0) {
        [self clearSearch];
        return;
    }

    self.currentQuery = searchString;
    [self updatePresentationForQuery:searchString];

    [self.searchDebounceTimer invalidate];
    self.searchDebounceTimer = [NSTimer scheduledTimerWithTimeInterval:0.15
                                                               target:self
                                                             selector:@selector(performPendingSearch)
                                                             userInfo:nil
                                                              repeats:NO];
}

- (void)performPendingSearch
{
    self.searchDebounceTimer = nil;
    if (self.currentQuery.length == 0) {
        return;
    }
    [self.dataSource searchForString:self.currentQuery];
}

- (void)clearSearch
{
    [self.searchDebounceTimer invalidate];
    self.searchDebounceTimer = nil;
    self.currentQuery = nil;
    [self.dataSource clearSearch];
    [self updatePresentationForQuery:@""];
}

@end
