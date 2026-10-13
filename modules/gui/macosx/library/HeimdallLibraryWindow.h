/*****************************************************************************
 * HeimdallLibraryWindow.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne # lycaria -dot- org>
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

#import "windows/video/HeimdallFullVideoViewWindow.h"

#import "views/HeimdallDragDropView.h"

#import "library/HeimdallLibraryItemPresentingCapable.h"

NS_ASSUME_NONNULL_BEGIN

@class HeimdallMainWindowControlsBar;
@class HeimdallDragDropView;
@class HeimdallRoundedCornerTextField;
@class HeimdallInputNodePathControl;
@class HeimdallLibraryAbstractSegmentViewController;
@class HeimdallLibraryCollectionView;
@class HeimdallLibrarySortingMenuController;
@class HeimdallMediaLibraryGroup;
@class HeimdallPlayQueueController;
@class HeimdallCustomEmptyLibraryBrowseButton;
@class HeimdallLibraryWindowSplitViewController;
@class HeimdallLibraryWindowToolbarDelegate;
@class HeimdallLoadingOverlayView;
@class HeimdallNoResultsLabel;

@protocol HeimdallMediaLibraryItemProtocol;

typedef NS_ENUM(NSInteger, HeimdallLibraryViewModeSegment) {
    HeimdallLibrarySmallestSentinelViewModeSegment = -1,
    HeimdallLibraryGridViewModeSegment,
    HeimdallLibraryListViewModeSegment,
    HeimdallLibraryLargestSentinelViewModeSegment
};

@interface HeimdallLibraryWindow : HeimdallFullVideoViewWindow<NSUserInterfaceItemIdentification, HeimdallDragDropTarget, HeimdallLibraryItemPresentingCapable, NSSearchFieldDelegate>

extern const CGFloat HeimdallLibraryWindowMinimalWidth;
extern const CGFloat HeimdallLibraryWindowMinimalHeight;
extern const NSUserInterfaceItemIdentifier HeimdallLibraryWindowIdentifier;
extern NSString * const HeimdallLibraryWindowEmbeddedVideoPlaybackActiveKey;

@property (readonly) NSView *libraryTargetView;

@property (nonatomic, weak) IBOutlet HeimdallMainWindowControlsBar *controlsBar;
@property (readwrite, weak) IBOutlet NSSegmentedControl *gridVsListSegmentedControl;
@property (readwrite, weak) IBOutlet NSSplitView *mainSplitView;
@property (readwrite, weak) IBOutlet NSView *homeLibraryView;
@property (readwrite, weak) IBOutlet NSScrollView *homeLibraryStackViewScrollView;
@property (readwrite, weak) IBOutlet NSStackView *homeLibraryStackView;
@property (readwrite, weak) IBOutlet NSView *videoLibraryView;
@property (readwrite, weak) IBOutlet NSSplitView *videoLibrarySplitView;
@property (readwrite, weak) IBOutlet NSScrollView *videoLibraryCollectionViewScrollView;
@property (readwrite, weak) IBOutlet HeimdallLibraryCollectionView *videoLibraryCollectionView;
@property (readwrite, weak) IBOutlet NSScrollView *videoLibraryGroupSelectionTableViewScrollView;
@property (readwrite, weak) IBOutlet NSTableView *videoLibraryGroupSelectionTableView;
@property (readwrite, weak) IBOutlet NSScrollView *videoLibraryGroupsTableViewScrollView;
@property (readwrite, weak) IBOutlet NSTableView *videoLibraryGroupsTableView;
@property (readwrite, weak) IBOutlet HeimdallLibraryCollectionView *mediaSourceCollectionView;
@property (readwrite, weak) IBOutlet NSView *audioLibraryView;
@property (readwrite, weak) IBOutlet NSSplitView *audioLibrarySplitView;
@property (readwrite, weak) IBOutlet NSScrollView *audioCollectionSelectionTableViewScrollView;
@property (readwrite, weak) IBOutlet NSTableView *audioCollectionSelectionTableView;
@property (readwrite, weak) IBOutlet NSScrollView *audioGroupSelectionTableViewScrollView;
@property (readwrite, weak) IBOutlet NSTableView *audioGroupSelectionTableView;
@property (readwrite, weak) IBOutlet NSScrollView *audioLibrarySongsTableViewScrollView;
@property (readwrite, weak) IBOutlet NSTableView *audioLibrarySongsTableView;
@property (readwrite, weak) IBOutlet NSScrollView *audioCollectionViewScrollView;
@property (readwrite, weak) IBOutlet HeimdallLibraryCollectionView *audioLibraryCollectionView;
@property (readwrite, weak) IBOutlet NSSplitView *audioLibraryGridModeSplitView;
@property (readwrite, weak) IBOutlet NSScrollView *audioLibraryGridModeSplitViewListTableViewScrollView;
@property (readwrite, weak) IBOutlet NSTableView *audioLibraryGridModeSplitViewListTableView;
@property (readwrite, weak) IBOutlet NSScrollView *audioLibraryGridModeSplitViewListSelectionCollectionViewScrollView;
@property (readwrite, weak) IBOutlet HeimdallLibraryCollectionView *audioLibraryGridModeSplitViewListSelectionCollectionView;
@property (readwrite, weak) IBOutlet NSView *mediaSourceView;
// These two media source controls are held strongly due to their handling within the media source view controller.
// Specifically, we do some custom handling to give them the glass background on macOS 26.0 and later; to avoid deallocation
// when removing the glass container view from the media source view, we must hold them strongly.
@property (readwrite, strong) IBOutlet NSButton *mediaSourceHomeButton;
@property (readwrite, strong) IBOutlet HeimdallInputNodePathControl *mediaSourcePathControl;
@property (readwrite, weak) IBOutlet NSVisualEffectView *mediaSourcePathControlVisualEffectView;
@property (readwrite, weak) IBOutlet NSScrollView *mediaSourceTableViewScrollView;
@property (readwrite, weak) IBOutlet NSTableView *mediaSourceTableView;
@property (readwrite, weak) IBOutlet NSScrollView *mediaSourceCollectionViewScrollView;
@property (readwrite, weak) IBOutlet NSToolbar *mediaToolBar;
@property (readwrite, weak) IBOutlet NSButton *librarySortButton;
@property (readwrite, weak) IBOutlet NSSearchField *librarySearchField;
@property (readwrite, weak) IBOutlet NSButton *playQueueToggle;
@property (readwrite, weak) IBOutlet NSButton *backwardsNavigationButton;
@property (readwrite, weak) IBOutlet NSButton *forwardsNavigationButton;
@property (readwrite, weak) IBOutlet NSButton *artworkButton;
@property (readwrite, weak) IBOutlet HeimdallLibraryWindowToolbarDelegate *toolbarDelegate;

@property (nonatomic, readwrite, strong) IBOutlet NSView *emptyLibraryView;
@property (nonatomic, readwrite, weak) IBOutlet NSImageView *placeholderImageView;
@property (nonatomic, readwrite, weak) IBOutlet NSTextField *placeholderLabel;
@property (nonatomic, readwrite, weak) IBOutlet HeimdallCustomEmptyLibraryBrowseButton *placeholderGoToBrowseButton;

@property (readonly) HeimdallLoadingOverlayView *loadingOverlayView;
@property (readonly) NSArray<NSLayoutConstraint *> *placeholderImageViewConstraints;
@property (readonly) HeimdallNoResultsLabel *noResultsLabel;

@property (readwrite, nonatomic) NSInteger librarySegmentType;
@property (readwrite) BOOL nonembedded;
@property (readonly) BOOL embeddedVideoPlaybackActive;

@property (readwrite, weak) IBOutlet HeimdallLibraryWindowSplitViewController *splitViewController;
@property (readonly) HeimdallLibraryAbstractSegmentViewController *librarySegmentViewController;
@property (readonly) HeimdallLibrarySortingMenuController *librarySortingMenuController;
@property (readonly) HeimdallPlayQueueController *playQueueController;

- (void)enableVideoPlaybackAppearance;
- (void)disableVideoPlaybackAppearance;
- (void)hideControlsBar;
- (void)showControlsBar;
- (void)updateGridVsListViewModeSegmentedControl;
- (void)updateFilterString;
- (void)clearFilterString;
- (void)updateToolbarDisplayFlags;
- (void)showLoadingOverlay;
- (void)hideLoadingOverlay;

- (void)displayLibraryView:(NSView *)view;
- (void)displayLibraryPlaceholderViewWithImage:(NSImage *)image
                              usingConstraints:(NSArray<NSLayoutConstraint *> *)constraints
                             displayingMessage:(NSString *)message;
- (void)displayNoResultsMessage;
- (void)browseFolderByMrl:(NSString *)mrl;

- (IBAction)goToBrowseSection:(id)sender;
- (IBAction)sortLibrary:(id)sender;
- (IBAction)filterLibrary:(id)sender;
- (IBAction)backwardsNavigationAction:(id)sender;
- (IBAction)forwardsNavigationAction:(id)sender;
- (IBAction)gridVsListSegmentedControlAction:(id)sender;

@end

NS_ASSUME_NONNULL_END
