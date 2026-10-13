/*****************************************************************************
 * HeimdallLibraryHomeViewController.m: MacOS X interface module
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

#import "HeimdallLibraryHomeViewController.h"

#import "extensions/NSImage+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibraryTableCellView.h"
#import "library/HeimdallLibraryWindow.h"
#import "library/HeimdallLibraryWindowPersistentPreferences.h"

#import "library/audio-library/HeimdallLibraryAudioViewController.h"

#import "library/home-library/HeimdallLibraryHomeViewStackViewController.h"
#import "library/home-library/HeimdallLibraryHomeViewVideoContainerViewDataSource.h"

#import "library/search-library/HeimdallLibrarySearchViewController.h"

#import "library/video-library/HeimdallLibraryVideoDataSource.h"
#import "library/video-library/HeimdallLibraryVideoTableViewDelegate.h"
#import "library/video-library/HeimdallLibraryVideoViewController.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallUIUnits.h"

#import "windows/video/HeimdallMainVideoViewController.h"

@interface HeimdallLibraryHomeViewController ()
{
    id<HeimdallMediaLibraryItemProtocol> _awaitingPresentingLibraryItem;
    NSMutableSet<NSString *> *_ongoingLongLoadingNotifications;
    NSArray<NSLayoutConstraint *> *_internalPlaceholderImageViewSizeConstraints;
}
@property (readwrite, nullable, nonatomic) HeimdallLibrarySearchViewController *searchViewController;
@property (readwrite) BOOL isShowingSearchOverlay;
@end

@implementation HeimdallLibraryHomeViewController

- (instancetype)initWithLibraryWindow:(HeimdallLibraryWindow *)libraryWindow
{
    self = [super initWithLibraryWindow:libraryWindow];

    if(self) {
        [self setupPropertiesFromLibraryWindow:libraryWindow];
        [self setupGridViewController];
        [self setupHomePlaceholderView];
        [self setupHomeLibraryViews];

        NSNotificationCenter *notificationCenter = NSNotificationCenter.defaultCenter;
        NSString *notificationNames[] =
        {
            HeimdallLibraryModelVideoMediaListReset,
            HeimdallLibraryModelAudioMediaListReset,
            HeimdallLibraryModelVideoMediaItemDeleted,
            HeimdallLibraryModelAudioMediaItemDeleted,
            HeimdallLibraryModelAllCachesDropped,
        };

        for (size_t i = 0; i < ARRAY_SIZE(notificationNames); ++i) @autoreleasepool
        {
            [notificationCenter addObserver:self
                                   selector:@selector(libraryModelUpdated:)
                                       name:notificationNames[i]
                                     object:nil];

            NSString *startedNotification = [notificationNames[i] stringByAppendingString:HeimdallLongNotificationNameStartSuffix];
            [notificationCenter addObserver:self
                                   selector:@selector(libraryModelLongLoadStarted:)
                                       name:startedNotification
                                     object:nil];

            NSString *finishedNotification = [notificationNames[i] stringByAppendingString:HeimdallLongNotificationNameFinishSuffix];
            [notificationCenter addObserver:self
                                   selector:@selector(libraryModelLongLoadFinished:)
                                       name:finishedNotification
                                     object:nil];
        }

        [notificationCenter addObserver:self
                               selector:@selector(librarySearchFieldDidChange:)
                                   name:NSControlTextDidChangeNotification
                                 object:nil];
    }

    return self;
}

- (void)dealloc
{
    [_searchViewController dismissFromContainer];
    _searchViewController = nil;
}

- (void)setupPropertiesFromLibraryWindow:(HeimdallLibraryWindow *)libraryWindow
{
    NSParameterAssert(libraryWindow);
    _homeLibraryView = libraryWindow.homeLibraryView;
    _homeLibraryStackViewScrollView = libraryWindow.homeLibraryStackViewScrollView;
    _homeLibraryStackView = libraryWindow.homeLibraryStackView;
}

- (void)setupGridViewController
{
    _stackViewController = [[HeimdallLibraryHomeViewStackViewController alloc] init];
    self.stackViewController.collectionsStackView = _homeLibraryStackView;
}

- (void)setupHomePlaceholderView
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

- (void)setupHomeLibraryViews
{
    const NSEdgeInsets defaultInsets = HeimdallUIUnits.libraryViewScrollViewContentInsets;
    const NSEdgeInsets scrollerInsets = HeimdallUIUnits.libraryViewScrollViewScrollerInsets;

    self.homeLibraryStackViewScrollView.automaticallyAdjustsContentInsets = NO;
    self.homeLibraryStackViewScrollView.contentInsets = defaultInsets;
    self.homeLibraryStackViewScrollView.scrollerInsets = scrollerInsets;
}

#pragma mark - Show the video library view

- (NSArray<NSLayoutConstraint *> *)placeholderImageViewSizeConstraints
{
    return _internalPlaceholderImageViewSizeConstraints;
}

- (void)updatePresentedView
{
    if (HeimdallMain.sharedInstance.libraryController.libraryModel.numberOfVideoMedia == 0) { // empty library
        [self presentPlaceholderHomeLibraryView];
    } else if (self.isShowingSearchOverlay) {
        [self presentSearchOverlay];
    } else {
        [self presentHomeLibraryView];
    }
}

- (void)presentHomeView
{
    // If the search field has text from a previous session, present search overlay
    NSString * const searchText = self.libraryWindow.librarySearchField.stringValue;
    if (searchText.length > 0) {
        [self showSearchOverlayForQuery:searchText];
        return;
    }
    [self updatePresentedView];
}

- (void)presentPlaceholderHomeLibraryView
{
    [self.libraryWindow displayLibraryPlaceholderViewWithImage:NSImage.HeimdallPlaceholderVideoImage
                                              usingConstraints:self.placeholderImageViewSizeConstraints
                                             displayingMessage:_NS("Your media will appear here.\nGo to the Browse section to add media you love.")];
}

- (void)presentHomeLibraryView
{
    [self.libraryWindow displayLibraryView:self.homeLibraryView];
    self.homeLibraryStackViewScrollView.hidden = NO;
    [self.stackViewController reloadData];
}

- (void)presentSearchOverlay
{
    [self.searchViewController presentInContainer:self.libraryTargetView];
    self.isShowingSearchOverlay = YES;
    [self.libraryWindow updateToolbarDisplayFlags];
}

- (void)libraryModelUpdated:(NSNotification *)aNotification
{
    NSParameterAssert(aNotification);
    HeimdallLibraryModel * const model = HeimdallMain.sharedInstance.libraryController.libraryModel;
    const NSUInteger videoCount = model.numberOfVideoMedia;
    const NSUInteger audioCount = model.numberOfAudioMedia;

    NSArray<NSView *> * const targetViewSubViews = self.libraryTargetView.subviews;
    const BOOL emptyLibraryViewPresent = [targetViewSubViews containsObject:self.emptyLibraryView];
    const BOOL homeLibraryViewPresent = [targetViewSubViews containsObject:self.homeLibraryView];
    if (self.libraryWindow.librarySegmentType == HeimdallLibraryHomeSegmentType &&
        !self.isShowingSearchOverlay &&
        ((videoCount == 0 && !emptyLibraryViewPresent) ||
         (videoCount > 0 && !homeLibraryViewPresent) ||
         (audioCount == 0 && !emptyLibraryViewPresent) ||
         (audioCount > 0 && !homeLibraryViewPresent)) &&
        !self.libraryWindow.embeddedVideoPlaybackActive) {

        [self updatePresentedView];
    }
}

#pragma mark - Search overlay

- (HeimdallLibrarySearchViewController *)searchViewController
{
    if (_searchViewController == nil) {
        _searchViewController = [[HeimdallLibrarySearchViewController alloc] initWithLibraryWindow:self.libraryWindow];
    }
    return _searchViewController;
}

- (void)librarySearchFieldDidChange:(NSNotification *)aNotification
{
    NSParameterAssert(aNotification);

    NSSearchField * const searchField = self.libraryWindow.librarySearchField;
    if (aNotification.object != searchField && aNotification.object != searchField.currentEditor) {
        return;
    }

    NSString * const searchString = searchField.stringValue ?: @"";

    if (searchString.length == 0) {
        [self hideSearchOverlay];
    } else {
        [self showSearchOverlayForQuery:searchString];
    }
}

- (void)showSearchOverlayForQuery:(NSString *)query
{
    NSParameterAssert(query && query.length > 0);
    if (!self.isShowingSearchOverlay) {
        [self presentSearchOverlay];
    }
    [self.searchViewController searchForString:query];
}

- (void)hideSearchOverlay
{
    if (!self.isShowingSearchOverlay) {
        return;
    }
    [self.searchViewController clearSearch];
    [self.searchViewController dismissFromContainer];
    self.isShowingSearchOverlay = NO;
    [HeimdallMain.sharedInstance.libraryController filterByString:@""];
    [self.libraryWindow updateToolbarDisplayFlags];

    // Re-present the home content (or placeholder) in the target view.
    [self updatePresentedView];
}

- (void)libraryModelLongLoadStarted:(NSNotification *)notification
{
    if ([_ongoingLongLoadingNotifications containsObject:notification.name]) {
        return;
    }

    [_ongoingLongLoadingNotifications addObject:notification.name];
    if (self.connected) {
        [self.stackViewController disconnectContainers];
    }
    [self.libraryWindow showLoadingOverlay];
}

- (void)libraryModelLongLoadFinished:(NSNotification *)notification
{
    [_ongoingLongLoadingNotifications removeObject:notification.name];
    if (_ongoingLongLoadingNotifications.count > 0) {
        return;
    }
    if (self.connected) {
        [self.stackViewController connectContainers];
    }
    [self.libraryWindow hideLoadingOverlay];
}

- (void)connect
{
    [self.stackViewController connectContainers];
}

- (void)disconnect
{
    [self.stackViewController disconnectContainers];
}

#pragma mark - HeimdallLibraryDynamicToolbarFlagsCapable

- (HeimdallLibraryWindowToolbarDisplayFlags)toolbarDisplayFlags
{
    if (self.isShowingSearchOverlay) {
        return HeimdallLibraryWindowToolbarDisplayFlagLibrarySearchBar |
               HeimdallLibraryWindowToolbarDisplayFlagToggleViewModeSegmentButton;
    }
    return HeimdallLibraryWindowToolbarDisplayFlagLibrarySearchBar;
}

#pragma mark - HeimdallLibraryDynamicViewModeCapable

- (HeimdallLibraryViewModeSegment)viewMode
{
    HeimdallLibraryWindowPersistentPreferences * const prefs =
        HeimdallLibraryWindowPersistentPreferences.sharedInstance;
    return self.isShowingSearchOverlay ? prefs.searchLibraryViewMode : prefs.homeLibraryViewMode;
}

- (void)setViewMode:(HeimdallLibraryViewModeSegment)viewMode
{
    HeimdallLibraryWindowPersistentPreferences * const prefs =
        HeimdallLibraryWindowPersistentPreferences.sharedInstance;
    if (self.isShowingSearchOverlay) {
        prefs.searchLibraryViewMode = viewMode;
    } else {
        prefs.homeLibraryViewMode = viewMode;
    }
}

@end
