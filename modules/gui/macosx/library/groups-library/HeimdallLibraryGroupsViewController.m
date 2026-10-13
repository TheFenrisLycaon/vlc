/*****************************************************************************
 * HeimdallLibraryGroupsViewController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
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

#import "HeimdallLibraryGroupsViewController.h"

#import "extensions/NSImage+HeimdallAdditions.h"
#import "extensions/NSScrollView+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

#import "library/HeimdallLibraryCollectionView.h"
#import "library/HeimdallLibraryCollectionViewDelegate.h"
#import "library/HeimdallLibraryCollectionViewFlowLayout.h"
#import "library/HeimdallLibraryCollectionViewItem.h"
#import "library/HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.h"
#import "library/HeimdallLibraryCollectionViewSupplementaryElementView.h"
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

#import "library/groups-library/HeimdallLibraryGroupsDataSource.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallUIUnits.h"

@interface HeimdallLibraryGroupsViewController ()
{
    NSArray<NSLayoutConstraint *> *_internalPlaceholderImageViewSizeConstraints;
}
@end

@implementation HeimdallLibraryGroupsViewController

- (instancetype)initWithLibraryWindow:(HeimdallLibraryWindow *)libraryWindow
{
    self = [super initWithLibraryWindow:libraryWindow];
    if (self) {
        [self setupDataSource];
        [self setupGridViewModeViews];
        [self setupListViewModeViews];
        [self setupPlaceholderView];
    }
    return self;
}

- (void)setupDataSource
{
    _dataSource = [[HeimdallLibraryGroupsDataSource alloc] init];
    self.dataSource.libraryModel = HeimdallMain.sharedInstance.libraryController.libraryModel;
}

- (void)setupGridViewModeViews
{
    _collectionView = [[HeimdallLibraryCollectionView alloc] init];
    self.collectionView.translatesAutoresizingMaskIntoConstraints = NO;

    _collectionViewScrollView = [NSScrollView libraryScrollViewWithDocumentView:self.collectionView];

    const CGFloat collectionItemSpacing = HeimdallUIUnits.collectionViewItemSpacing;
    const NSEdgeInsets collectionViewSectionInset = HeimdallUIUnits.collectionViewSectionInsets;

    HeimdallLibraryCollectionViewFlowLayout * const collectionViewLayout =
        [[HeimdallLibraryCollectionViewFlowLayout alloc] init];
    collectionViewLayout.headerReferenceSize =
        HeimdallLibraryCollectionViewSupplementaryElementView.defaultHeaderSize;
    collectionViewLayout.minimumLineSpacing = collectionItemSpacing;
    collectionViewLayout.minimumInteritemSpacing = collectionItemSpacing;
    collectionViewLayout.sectionInset = collectionViewSectionInset;
    self.collectionView.collectionViewLayout = collectionViewLayout;

    _collectionViewDelegate = [[HeimdallLibraryCollectionViewDelegate alloc] init];
    self.collectionViewDelegate.itemsAspectRatio = HeimdallLibraryCollectionViewItemAspectRatioVideoItem;
    self.collectionViewDelegate.staticItemSize = HeimdallUIUnits.defaultVideoItemCollectionViewItemSize;
    self.collectionView.delegate = self.collectionViewDelegate;

    self.collectionView.selectable = YES;
    self.collectionView.allowsEmptySelection = YES;
    self.collectionView.allowsMultipleSelection = YES;

    [self.collectionView registerClass:HeimdallLibraryCollectionViewItem.class
                 forItemWithIdentifier:HeimdallLibraryCollectionViewItemIdentifier];

    [self.collectionView registerClass:HeimdallLibraryCollectionViewSupplementaryElementView.class
            forSupplementaryViewOfKind:NSCollectionElementKindSectionHeader
                        withIdentifier:HeimdallLibrarySupplementaryElementViewIdentifier];

    NSString * const mediaItemSupplementaryDetailViewString =
        NSStringFromClass(HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.class);
    NSNib * const mediaItemSupplementaryDetailViewNib =
        [[NSNib alloc] initWithNibNamed:mediaItemSupplementaryDetailViewString bundle:nil];

    [self.collectionView registerNib:mediaItemSupplementaryDetailViewNib
          forSupplementaryViewOfKind:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind
                      withIdentifier:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewIdentifier];

    self.dataSource.collectionView = self.collectionView;

    self.collectionView.dataSource = self.dataSource;
    self.collectionView.delegate = self.collectionViewDelegate;
}

- (void)setupListViewModeViews
{
    _tableViewDelegate = [[HeimdallLibraryMasterDetailViewTableViewDelegate alloc] init];
    _splitViewDelegate = [[HeimdallLibraryTwoPaneSplitViewDelegate alloc] init];
    _groupsTableView = [[HeimdallLibraryTableView alloc] init];
    _selectedGroupTableView = [[HeimdallLibraryTableView alloc] init];
    _listViewSplitView = [[NSSplitView alloc] init];

    self.dataSource.headerDelegate = self.tableViewDelegate;

    self.listViewSplitView.translatesAutoresizingMaskIntoConstraints = NO;
    self.groupsTableView.translatesAutoresizingMaskIntoConstraints = NO;
    self.selectedGroupTableView.translatesAutoresizingMaskIntoConstraints = NO;

    _groupsTableViewScrollView = [NSScrollView libraryScrollViewWithDocumentView:self.groupsTableView];
    _selectedGroupTableViewScrollView = [NSScrollView libraryDetailScrollViewWithDocumentView:self.selectedGroupTableView];

    self.listViewSplitView.vertical = YES;
    self.listViewSplitView.dividerStyle = NSSplitViewDividerStyleThin;
    self.listViewSplitView.delegate = self.splitViewDelegate;
    [self.listViewSplitView addArrangedSubview:self.groupsTableViewScrollView];
    [self.listViewSplitView addArrangedSubview:self.selectedGroupTableViewScrollView];

    NSTableColumn * const groupsColumn = [[NSTableColumn alloc] initWithIdentifier:@"groups"];
    NSTableColumn * const selectedGroupColumn =
        [[NSTableColumn alloc] initWithIdentifier:@"selectedGroup"];

    [self.groupsTableView addTableColumn:groupsColumn];
    [self.selectedGroupTableView addTableColumn:selectedGroupColumn];

    NSNib * const tableCellViewNib = 
        [[NSNib alloc] initWithNibNamed:NSStringFromClass(HeimdallLibraryTableCellView.class)
                                 bundle:nil];
    [self.groupsTableView registerNib:tableCellViewNib
                        forIdentifier:@"HeimdallLibraryTableViewCellIdentifier"];
    [self.selectedGroupTableView registerNib:tableCellViewNib
                               forIdentifier:@"HeimdallLibraryTableViewCellIdentifier"];

    _selectedGroupTableHeaderView = [HeimdallLibraryHeaderView paddedHeaderView];

    self.tableViewDelegate.detailTableHeaderView = self.selectedGroupTableHeaderView;

    self.groupsTableView.headerView = nil;
    self.groupsTableView.allowsEmptySelection = NO;
    self.groupsTableView.allowsMultipleSelection = YES;
    self.selectedGroupTableView.headerView = self.selectedGroupTableHeaderView;

    selectedGroupColumn.headerCell = [HeimdallLibraryHeaderCell new];

    self.groupsTableView.rowHeight = HeimdallUIUnits.mediumTableViewRowHeight;
    self.selectedGroupTableView.rowHeight = HeimdallUIUnits.mediumTableViewRowHeight;
    self.selectedGroupTableView.allowsMultipleSelection = YES;

    self.groupsTableView.delegate = self.tableViewDelegate;
    self.selectedGroupTableView.delegate = self.tableViewDelegate;

    self.dataSource.masterTableView = self.groupsTableView;
    self.dataSource.detailTableView = self.selectedGroupTableView;

    self.groupsTableView.dataSource = self.dataSource;
    self.selectedGroupTableView.dataSource = self.dataSource;
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
                                      constant:114.f],
    ];
}

- (id<HeimdallLibraryDataSource>)currentDataSource
{
    return self.dataSource;
}

- (NSArray<NSLayoutConstraint *> *)placeholderImageViewSizeConstraints
{
    return _internalPlaceholderImageViewSizeConstraints;
}

- (void)presentPlaceholderGroupsView
{
    [self.libraryWindow displayLibraryPlaceholderViewWithImage:NSImage.HeimdallPlaceholderVideoImage
                                              usingConstraints:self.placeholderImageViewSizeConstraints
                                             displayingMessage:_NS("Your media groups will appear here.")];
}

- (void)presentGroupsView
{
    const HeimdallLibraryViewModeSegment viewModeSegment =
        HeimdallLibraryWindowPersistentPreferences.sharedInstance.groupsLibraryViewMode;

    if (self.dataSource.libraryModel.numberOfGroups > 0) {
        if (viewModeSegment == HeimdallLibraryGridViewModeSegment) {
            [self.libraryWindow displayLibraryView:self.collectionViewScrollView];
        } else {
            [self.libraryWindow displayLibraryView:self.listViewSplitView];
            [self.splitViewDelegate resetDefaultSplitForSplitView:self.listViewSplitView];
            [self.dataSource updateHeaderInTableView:self.selectedGroupTableView
                                  forMasterSelection:self.groupsTableView];
        }
    } else if (self.dataSource.libraryModel.filterString.length > 0) {
        [self.libraryWindow displayNoResultsMessage];
    } else {
        [self presentPlaceholderGroupsView];
    }
}

- (void)presentLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    [self presentGroupsView];
    
    const HeimdallLibraryViewModeSegment viewModeSegment =
        HeimdallLibraryWindowPersistentPreferences.sharedInstance.groupsLibraryViewMode;
    if (viewModeSegment == HeimdallLibraryGridViewModeSegment) {
        NSIndexPath * const groupIndexPath = [self.dataSource indexPathForLibraryItem:libraryItem];
        NSSet<NSIndexPath *> * const groupIndexPathSet = [NSSet setWithObject:groupIndexPath];
        [self.collectionView scrollToItemsAtIndexPaths:groupIndexPathSet
                                        scrollPosition:NSCollectionViewScrollPositionTop];
    } else {
        const NSInteger groupRow = [self.dataSource rowForLibraryItem:libraryItem];
        [self.groupsTableView selectRowIndexes:[NSIndexSet indexSetWithIndex:groupRow]
                          byExtendingSelection:NO];
        [self.groupsTableView scrollRowToVisible:groupRow];
    }
}

@end
