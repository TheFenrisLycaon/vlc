/*****************************************************************************
 * HeimdallLibraryWindow.m: MacOS X interface module
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

#import "HeimdallLibraryWindow.h"

#import <heimdall_common.h>
#import <heimdall_configuration.h>
#import <heimdall_media_library.h>
#import <heimdall_url.h>

#import "HeimdallLibraryDataTypes.h"

#import "extensions/NSAnimationContext+HeimdallAdditions.h"
#import "extensions/NSColor+HeimdallAdditions.h"
#import "extensions/NSFont+HeimdallAdditions.h"
#import "extensions/NSImage+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"
#import "extensions/NSView+HeimdallAdditions.h"
#import "extensions/NSWindow+HeimdallAdditions.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryAbstractMediaLibrarySegmentViewController.h"
#import "library/HeimdallLibraryCollectionViewSupplementaryElementView.h"
#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryDynamicToolbarFlagsCapable.h"
#import "library/HeimdallLibraryDynamicViewModeCapable.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibrarySegment.h"
#import "library/HeimdallLibrarySortingMenuController.h"
#import "library/HeimdallLibraryWindowChaptersSidebarViewController.h"
#import "library/HeimdallLibraryWindowNavigationSidebarViewController.h"
#import "library/HeimdallLibraryWindowPersistentPreferences.h"
#import "library/HeimdallLibraryWindowSidebarRootViewController.h"
#import "library/HeimdallLibraryWindowSplitViewController.h"
#import "library/HeimdallLibraryWindowToolbarDelegate.h"

#import "library/audio-library/HeimdallLibraryAlbumTableCellView.h"
#import "library/audio-library/HeimdallLibraryAudioDataSource.h"
#import "library/audio-library/HeimdallLibraryAudioViewController.h"

#import "library/groups-library/HeimdallLibraryGroupsViewController.h"

#import "library/home-library/HeimdallLibraryHomeViewController.h"

#import "library/playlist-library/HeimdallLibraryPlaylistViewController.h"

#import "library/video-library/HeimdallLibraryVideoDataSource.h"
#import "library/video-library/HeimdallLibraryVideoViewController.h"

#import "main/HeimdallApplication.h"
#import "main/HeimdallMain.h"

#import "media-source/HeimdallLibraryMediaSourceViewController.h"
#import "media-source/HeimdallLibraryMediaSourceViewNavigationStack.h"
#import "media-source/HeimdallMediaSourceBaseDataSource.h"

#import "menus/HeimdallMainMenu.h"

#import "playqueue/HeimdallPlayerController.h"
#import "playqueue/HeimdallPlayQueueController.h"
#import "playqueue/HeimdallPlayQueueModel.h"

#import "views/HeimdallBottomBarView.h"
#import "views/HeimdallDragDropView.h"
#import "views/HeimdallImageView.h"
#import "views/HeimdallLoadingOverlayView.h"
#import "views/HeimdallNoResultsLabel.h"
#import "views/HeimdallPlaybackEndViewController.h"
#import "views/HeimdallRoundedCornerTextField.h"
#import "views/HeimdallTrackingView.h"
#import "views/HeimdallUIUnits.h"

#import "windows/HeimdallDetachedAudioWindow.h"
#import "windows/HeimdallOpenInputMetadata.h"
#import "windows/HeimdallOpenWindowController.h"

#ifdef ENABLE_MACOSX_UI_SHADERS
# import "views/HeimdallSnowEffectView.h"
#endif

#import "windows/controlsbar/HeimdallMainWindowControlsBar.h"

#import "windows/video/HeimdallMainVideoViewController.h"
#import "windows/video/HeimdallVideoOutputProvider.h"
#import "windows/video/HeimdallVoutView.h"

const NSUInteger HeimdallLibrarySearchMaxMatchingTitles = 5;
const CGFloat HeimdallLibraryWindowMinimalWidth = 604.;
const CGFloat HeimdallLibraryWindowMinimalHeight = 307.;
const NSUserInterfaceItemIdentifier HeimdallLibraryWindowIdentifier = @"HeimdallLibraryWindow";
NSString * const HeimdallLibraryWindowEmbeddedVideoPlaybackActiveKey = @"embeddedVideoPlaybackActive";

@interface HeimdallLibraryWindow () <NSControlTextEditingDelegate>
{
    NSInteger _currentSelectedViewModeSegment;
    HeimdallVideoWindowCommon *_temporaryAudioDecorativeWindow;
    NSView *_acquiredVideoView;
}

@property NSTimer *searchInputTimer;

@end

static int ShowFullscreenController(heimdall_object_t * __unused p_this,
                                    const char * __unused psz_variable,
                                    heimdall_value_t __unused old_val,
                                    heimdall_value_t __unused new_val,
                                    void * __unused param)
{
    @autoreleasepool {
        dispatch_async(dispatch_get_main_queue(), ^{
            [NSNotificationCenter.defaultCenter postNotificationName:HeimdallVideoWindowShouldShowFullscreenController
                                                                object:nil];
        });

        return HEIMDALL_SUCCESS;
    }
}

static int ShowController(heimdall_object_t * __unused p_this,
                          const char * __unused psz_variable,
                          heimdall_value_t __unused old_val,
                          heimdall_value_t __unused new_val,
                          void * __unused param)
{
    @autoreleasepool {
        dispatch_async(dispatch_get_main_queue(), ^{
            [NSNotificationCenter.defaultCenter postNotificationName:HeimdallWindowShouldShowController
                                                                object:nil];
        });

        return HEIMDALL_SUCCESS;
    }
}

@interface HeimdallLibraryWindow ()

@property BOOL presentLoadingOverlayOnVideoPlaybackHide;

@end

@implementation HeimdallLibraryWindow

- (void)awakeFromNib
{
    [super awakeFromNib];
    self.identifier = HeimdallLibraryWindowIdentifier;
    self.minSize = NSMakeSize(HeimdallLibraryWindowMinimalWidth, HeimdallLibraryWindowMinimalHeight);

    self.tabbingMode = NSWindowTabbingModeDisallowed;

    HeimdallMain *mainInstance = HeimdallMain.sharedInstance;
    _playQueueController = [mainInstance playQueueController];

    libheimdall_int_t *libheimdall = heimdall_object_instance(getIntf());
    var_AddCallback(libheimdall, "intf-toggle-fscontrol", ShowFullscreenController, (__bridge void *)self);
    var_AddCallback(libheimdall, "intf-show", ShowController, (__bridge void *)self);

    _libraryTargetView = [[NSView alloc] init];

    self.videoViewController.view.frame = self.mainSplitView.frame;
    self.videoViewController.view.hidden = YES;
    self.videoViewController.displayLibraryControls = YES;
    [self hideControlsBarImmediately];

    NSNotificationCenter *notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(shouldShowController:)
                               name:HeimdallWindowShouldShowController
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(playerStateChanged:)
                               name:HeimdallPlayerCurrentMediaItemChanged
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(playerCurrentMediaItemChanged:)
                               name:HeimdallPlayerCurrentMediaItemChanged
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(playerStateChanged:)
                               name:HeimdallPlayerStateChanged
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(playerTrackSelectionChanged:)
                               name:HeimdallPlayerTrackSelectionChanged
                             object:nil];

    [self setViewForSelectedSegment];
    [self setupLoadingOverlayView];

    self.librarySearchField.delegate = self;
}

- (void)dealloc
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
    libheimdall_int_t *libheimdall = heimdall_object_instance(getIntf());
    var_DelCallback(libheimdall, "intf-toggle-fscontrol", ShowFullscreenController, (__bridge void *)self);
    var_DelCallback(libheimdall, "intf-show", ShowController, (__bridge void *)self);
}

- (void)encodeRestorableStateWithCoder:(NSCoder *)coder
{
    [super encodeRestorableStateWithCoder:coder];
    [coder encodeInteger:_librarySegmentType forKey:@"macosx-library-selected-segment"];
}

- (void)setupLoadingOverlayView
{
    _loadingOverlayView = [[HeimdallLoadingOverlayView alloc] init];
    self.loadingOverlayView.translatesAutoresizingMaskIntoConstraints = NO;
}

#pragma mark - misc. user interactions

- (void)updateGridVsListViewModeSegmentedControl
{
    _currentSelectedViewModeSegment = [self currentViewMode];
    _gridVsListSegmentedControl.selectedSegment = _currentSelectedViewModeSegment;
}

- (NSInteger)currentViewMode
{
    if ([self.librarySegmentViewController conformsToProtocol:@protocol(HeimdallLibraryDynamicViewModeCapable)]) {
        id<HeimdallLibraryDynamicViewModeCapable> const capableViewController =
            (id<HeimdallLibraryDynamicViewModeCapable>)self.librarySegmentViewController;
        return capableViewController.viewMode;
    }
    return [HeimdallLibrarySegment segmentWithSegmentType:self.librarySegmentType].viewMode;
}

- (void)setCurrentViewMode:(NSInteger)viewMode
{
    if ([self.librarySegmentViewController conformsToProtocol:@protocol(HeimdallLibraryDynamicViewModeCapable)]) {
        id<HeimdallLibraryDynamicViewModeCapable> const capableViewController =
            (id<HeimdallLibraryDynamicViewModeCapable>)self.librarySegmentViewController;
        capableViewController.viewMode = (HeimdallLibraryViewModeSegment)viewMode;
    } else {
        [HeimdallLibrarySegment segmentWithSegmentType:self.librarySegmentType].viewMode = viewMode;
    }
}

- (void)updateToolbarDisplayFlags
{
    NSParameterAssert([self.librarySegmentViewController conformsToProtocol:@protocol(HeimdallLibraryDynamicToolbarFlagsCapable)]);
    id<HeimdallLibraryDynamicToolbarFlagsCapable> const capableViewController =
        (id<HeimdallLibraryDynamicToolbarFlagsCapable>)self.librarySegmentViewController;
    [self.toolbarDelegate applyVisiblityFlags:capableViewController.toolbarDisplayFlags];
}

- (void)applyToolbarDisplayFlagsForSegment:(HeimdallLibrarySegment *)segment
{
    HeimdallLibraryWindowToolbarDisplayFlags flags;
    if ([self.librarySegmentViewController conformsToProtocol:@protocol(HeimdallLibraryDynamicToolbarFlagsCapable)]) {
        id<HeimdallLibraryDynamicToolbarFlagsCapable> const capableViewController =
            (id<HeimdallLibraryDynamicToolbarFlagsCapable>)self.librarySegmentViewController;
        flags = capableViewController.toolbarDisplayFlags;
    } else {
        flags = segment.toolbarDisplayFlags;
    }
    [self.toolbarDelegate applyVisiblityFlags:flags];
}

- (void)setViewForSelectedSegment
{
    const HeimdallLibrarySegmentType segmentType = self.librarySegmentType;
    HeimdallLibrarySegment * const segment = [HeimdallLibrarySegment segmentWithSegmentType:segmentType];
    [self applySegmentView:segment];
}

- (void)applySegmentView:(HeimdallLibrarySegment *)segment
{
    self.librarySearchField.placeholderString = segment.searchFieldPlaceholder;
    if (![self.librarySegmentViewController isKindOfClass:segment.libraryViewControllerClass]) {
        _librarySegmentViewController = [segment newLibraryViewController];
    }
    [self applyToolbarDisplayFlagsForSegment:segment];
    [segment presentLibraryViewUsingController:self.librarySegmentViewController];
    [self invalidateRestorableState];
}

- (void)setLibrarySegmentType:(NSInteger)segmentType
{
    if (segmentType == _librarySegmentType) {
        return;
    }

    _librarySegmentType = segmentType;
    [self setViewForSelectedSegment];
    [self updateGridVsListViewModeSegmentedControl];
}

- (IBAction)gridVsListSegmentedControlAction:(id)sender
{
    if (_gridVsListSegmentedControl.selectedSegment == _currentSelectedViewModeSegment) {
        return;
    }

    _currentSelectedViewModeSegment = _gridVsListSegmentedControl.selectedSegment;
    [self setCurrentViewMode:_currentSelectedViewModeSegment];
    [self setViewForSelectedSegment];
}

- (void)displayLibraryView:(NSView *)view
{
    view.translatesAutoresizingMaskIntoConstraints = NO;
    if ([self.libraryTargetView.subviews containsObject:self.loadingOverlayView]) {
        self.libraryTargetView.subviews = @[view, self.loadingOverlayView];
    } else {
        self.libraryTargetView.subviews = @[view];
    }

    [NSLayoutConstraint activateConstraints:@[
        [view.topAnchor constraintEqualToAnchor:self.libraryTargetView.topAnchor],
        [view.bottomAnchor constraintEqualToAnchor:self.libraryTargetView.bottomAnchor],
        [view.leftAnchor constraintEqualToAnchor:self.libraryTargetView.leftAnchor],
        [view.rightAnchor constraintEqualToAnchor:self.libraryTargetView.rightAnchor]
    ]];

#ifdef ENABLE_MACOSX_UI_SHADERS
    if (HeimdallMain.sharedInstance.metalLibrary && ((HeimdallApplication *)NSApplication.sharedApplication).winterHolidaysTheming) {
        HeimdallSnowEffectView * const snowView =
            [[HeimdallSnowEffectView alloc] initWithFrame:self.contentView.bounds];
        [self.libraryTargetView addSubview:snowView];
        snowView.translatesAutoresizingMaskIntoConstraints = NO;
        [snowView applyConstraintsToFillSuperview];
    }
#endif
}

- (void)displayLibraryPlaceholderViewWithImage:(NSImage *)image
                              usingConstraints:(NSArray<NSLayoutConstraint *> *)constraints
                             displayingMessage:(NSString *)message
{
    for (NSLayoutConstraint * const constraint in self.placeholderImageViewConstraints) {
        constraint.active = NO;
    }
    _placeholderImageViewConstraints = constraints;
    for (NSLayoutConstraint * const constraint in constraints) {
        constraint.active = YES;
    }

    [self displayLibraryView:self.emptyLibraryView];
    self.placeholderImageView.image = image;
    self.placeholderLabel.stringValue = message;
}

- (void)displayNoResultsMessage
{
    if (self.noResultsLabel == nil) {
        _noResultsLabel = [[HeimdallNoResultsLabel alloc] init];
        _noResultsLabel.translatesAutoresizingMaskIntoConstraints = NO;
    }

    if ([self.libraryTargetView.subviews containsObject:self.loadingOverlayView]) {
        self.libraryTargetView.subviews = @[self.noResultsLabel, self.loadingOverlayView];
    } else {
        self.libraryTargetView.subviews = @[_noResultsLabel];
    }

    [NSLayoutConstraint activateConstraints:@[
        [self.noResultsLabel.centerXAnchor constraintEqualToAnchor:self.libraryTargetView.centerXAnchor],
        [self.noResultsLabel.centerYAnchor constraintEqualToAnchor:self.libraryTargetView.centerYAnchor]
    ]];
}

- (void)presentLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    HeimdallLibrarySegment * const segment = [HeimdallLibrarySegment segmentForLibraryItem:libraryItem];
    [self.splitViewController.navSidebarViewController selectSegment:segment.segmentType];
    if ([self.librarySegmentViewController conformsToProtocol:@protocol(HeimdallLibraryItemPresentingCapable)]) {
        [(HeimdallLibraryAbstractSegmentViewController<HeimdallLibraryItemPresentingCapable> *)self.librarySegmentViewController presentLibraryItem:libraryItem];
    }
}

- (void)browseFolderByMrl:(NSString *)mrl
{
    [self goToBrowseSection:self];
    [(HeimdallLibraryMediaSourceViewController *)self.librarySegmentViewController browseFolderByMrl:mrl];
}

- (IBAction)sortLibrary:(id)sender
{
    if (!_librarySortingMenuController) {
        _librarySortingMenuController = [[HeimdallLibrarySortingMenuController alloc] init];
    }
    [NSMenu popUpContextMenu:_librarySortingMenuController.librarySortingMenu withEvent:[NSApp currentEvent] forView:sender];
}

- (void)stopSearchTimer
{
    [self.searchInputTimer invalidate];
    self.searchInputTimer = nil;
}

- (IBAction)filterLibrary:(id)sender
{
    [self stopSearchTimer];
    self.searchInputTimer = [NSTimer scheduledTimerWithTimeInterval:0.3
                                                            target:self
                                                           selector:@selector(updateFilterString)
                                                           userInfo:nil
                                                            repeats:NO];

    if (self.librarySegmentType == HeimdallLibraryHomeSegmentType) {
        return;
    }

    // Trigger completions
    NSText * const fieldEditor = self.librarySearchField.currentEditor;
    if (fieldEditor) {
        [fieldEditor complete:sender];
    }
}

- (void)updateFilterString
{
    if (self.librarySegmentType == HeimdallLibraryHomeSegmentType) {
        return;
    }

    [HeimdallMain.sharedInstance.libraryController filterByString:_librarySearchField.stringValue];
}

- (void)clearFilterString
{
    [self stopSearchTimer];
    _librarySearchField.stringValue = @"";
    [self updateFilterString];
}

- (BOOL)handlePasteBoardFromDragSession:(NSPasteboard *)paste
{
    return [HeimdallFileDragRecognisingView handlePasteboardFromDragSessionAsPlayQueueItems:paste];
}

- (IBAction)goToBrowseSection:(id)sender
{
    [self.splitViewController.navSidebarViewController selectSegment:HeimdallLibraryBrowseSegmentType];
}

- (IBAction)backwardsNavigationAction:(id)sender
{
    !self.embeddedVideoPlaybackActive
        ? [((HeimdallLibraryMediaSourceViewController *)self.librarySegmentViewController).navigationStack backwards]
        : [self disableVideoPlaybackAppearance];
}

- (IBAction)forwardsNavigationAction:(id)sender
{
    [((HeimdallLibraryMediaSourceViewController *)self.librarySegmentViewController).navigationStack forwards];
}

#pragma mark - video output controlling

- (void)playerStateChanged:(NSNotification *)notification
{
    if (_playQueueController.playerController.playerState == HEIMDALL_PLAYER_STATE_STOPPED) {
        [self hideControlsBar];
        return;
    }

    if (!self.embeddedVideoPlaybackActive) {
        [self showControlsBar];
    }
}

- (void)playerCurrentMediaItemChanged:(NSNotification *)notification
{
    NSParameterAssert(notification);
    HeimdallPlayerController * const controller = notification.object;
    NSAssert(controller != nil,
             @"Player current media item changed notification should have valid player controller");

    // Live video playback in controls bar artwork button handling
    if (self.splitViewController.mainVideoModeEnabled) {
        return;
    }

    if (controller.currentMediaIsAudioOnly && _acquiredVideoView) {
        [self.videoViewController returnVideoView:_acquiredVideoView];
        _acquiredVideoView = nil;
    } else if (!controller.currentMediaIsAudioOnly && _acquiredVideoView == nil) {
        [self configureArtworkButtonLiveVideoView];
    }
}

- (void)playerTrackSelectionChanged:(NSNotification *)notification
{
    [self updateArtworkButtonEnabledState];
}

- (void)updateArtworkButtonEnabledState
{
    HeimdallPlayerController * const playerController = self.playerController;
    const BOOL videoTrackDisabled =
        !playerController.videoTracksEnabled || !playerController.selectedVideoTrack.selected;
    const BOOL audioTrackDisabled =
        !playerController.audioTracksEnabled || !playerController.selectedAudioTrack.selected;
    const BOOL currentItemIsAudio =
        playerController.videoTracks.count == 0 && playerController.audioTracks.count > 0;
    const BOOL pipOpen = self.videoViewController.pipIsActive;
    const BOOL artworkButtonDisabled =
        (videoTrackDisabled && audioTrackDisabled) ||
        (videoTrackDisabled && !currentItemIsAudio) ||
        pipOpen;
    self.artworkButton.enabled = !artworkButtonDisabled;
    self.artworkButton.hidden = artworkButtonDisabled;
    self.controlsBar.thumbnailTrackingView.enabled = !artworkButtonDisabled;
    for (NSView * const view in self.controlsBar.thumbnailTrackingView.viewsToHide) {
        view.hidden = artworkButtonDisabled;
    }
}

- (void)configureArtworkButtonLiveVideoView
{
    HeimdallPlayerController * const playerController = self.playerController;
    const BOOL videoTrackDisabled =
        !playerController.videoTracksEnabled || !playerController.selectedVideoTrack.selected;

    if (videoTrackDisabled || !var_InheritBool(getIntf(), "embedded-video"))
        return;

    _acquiredVideoView = [self.videoViewController acquireVideoView];
    if (_acquiredVideoView) {
        [self.controlsBar.artworkImageView addSubview:_acquiredVideoView
                                           positioned:NSWindowBelow
                                           relativeTo:self.artworkButton];
        [_acquiredVideoView applyConstraintsToFillSuperview];
    }
}

- (void)hideControlsBarImmediately
{
    self.controlsBar.bottomBarView.hidden = YES;
    self.controlsBar.bottomBarView.alphaValue = 0;
}

- (void)hideControlsBar
{
     [NSAnimationContext runAnimationRespectingPreferencesWithDuration:HeimdallUIUnits.controlsFadeAnimationDuration
                                                               changes:^(NSAnimationContext * const context) {
        context.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];
        self.controlsBar.bottomBarView.animator.alphaValue = 0;
    } completionHandler:^{
        self.controlsBar.bottomBarView.hidden = self.controlsBar.bottomBarView.alphaValue == 0;
    }];
}

- (void)showControlsBarImmediately
{
    self.controlsBar.bottomBarView.hidden = NO;
    self.controlsBar.bottomBarView.alphaValue = 1;
}

- (void)showControlsBar
{
    self.controlsBar.bottomBarView.hidden = NO;

    [NSAnimationContext runAnimationRespectingPreferencesWithDuration:HeimdallUIUnits.controlsFadeAnimationDuration
                                                              changes:^(NSAnimationContext * const __unused context) {
        context.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseIn];
        self.controlsBar.bottomBarView.animator.alphaValue = 1;
    } completionHandler:nil];
}

- (BOOL)shouldShowControlsBarForPlaybackContext
{
    HeimdallPlayerController * const playerController = self.playQueueController.playerController;
    return playerController.playerState != HEIMDALL_PLAYER_STATE_STOPPED &&
           self.playQueueController.playQueueModel.numberOfPlayQueueItems > 0;
}

- (void)presentExternalWindows
{
    HeimdallVideoOutputProvider * const voutProvider = HeimdallMain.sharedInstance.voutProvider;
    NSArray<NSWindow *> * const voutWindows = voutProvider.voutWindows.allValues;

    if (voutWindows.count == 0 && self.playerController.videoTracks.count == 0) {
        // If we have no video windows in the video provider but are being asked to present a window
        // then we are dealing with an audio item and the user wants to see the decorative artwork
        // window for said audio
        [HeimdallMain.sharedInstance.detachedAudioWindow makeKeyAndOrderFront:self];
        return;
    }

    for (NSWindow * const window in voutWindows) {
        [window makeKeyAndOrderFront:self];
    }
}

- (void)presentVideoView
{
    for (NSView *subview in _libraryTargetView.subviews) {
        [subview removeFromSuperview];
    }

    NSLog(@"Presenting video view in main library window.");

    NSView * const videoView = self.videoViewController.view;
    videoView.translatesAutoresizingMaskIntoConstraints = NO;
    videoView.hidden = NO;

    [_libraryTargetView addSubview:videoView];
    [videoView applyConstraintsToFillSuperview];
}

- (void)enableVideoPlaybackAppearance
{
    HeimdallPlayerController * const playerController = self.playerController;
    const BOOL videoTrackDisabled =
        !playerController.videoTracksEnabled || !playerController.selectedVideoTrack.selected;
    const BOOL audioTrackDisabled =
        !playerController.audioTracksEnabled || !playerController.selectedAudioTrack.selected;
    const BOOL currentItemIsAudio =
        playerController.videoTracks.count == 0 && playerController.audioTracks.count > 0;
    if ((videoTrackDisabled && audioTrackDisabled) || (videoTrackDisabled && !currentItemIsAudio)) {
        return;
    }

    const BOOL isEmbedded = var_InheritBool(getIntf(), "embedded-video");
    if (!isEmbedded) {
        [self presentExternalWindows];
        return;
    }

    NSAppearance *darkAppearance = nil;
    if (@available(macOS 10.14, *)) {
        darkAppearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    } else {
        darkAppearance = [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark];
    }

    if (darkAppearance) {
        self.appearance = darkAppearance;
    }

    if (_acquiredVideoView) {
        [self.videoViewController returnVideoView:_acquiredVideoView];
        _acquiredVideoView = nil;
    }

    [self presentVideoView];
    [self enableVideoTitleBarMode];
    [self hideControlsBarImmediately];
    [self.videoViewController showControls];

    self.splitViewController.mainVideoModeEnabled = YES;

    [self willChangeValueForKey:HeimdallLibraryWindowEmbeddedVideoPlaybackActiveKey];
    _embeddedVideoPlaybackActive = YES;
    [self didChangeValueForKey:HeimdallLibraryWindowEmbeddedVideoPlaybackActiveKey];

    if ([self.librarySegmentViewController isKindOfClass:HeimdallLibraryAbstractMediaLibrarySegmentViewController.class]) {
        [(HeimdallLibraryAbstractMediaLibrarySegmentViewController *)self.librarySegmentViewController disconnect];
    }
}

- (void)disableVideoPlaybackAppearance
{
    self.appearance = nil;

    [self makeFirstResponder:self.splitViewController.multifunctionSidebarViewController.view];
    [HeimdallMain.sharedInstance.voutProvider updateWindowLevelForHelperWindows:NSNormalWindowLevel];

    // restore alpha value to 1 for the case that macosx-opaqueness is set to < 1
    self.alphaValue = 1.0;
    [self disableVideoTitleBarMode];
    [self setViewForSelectedSegment];
    if ([self shouldShowControlsBarForPlaybackContext]) {
        [self showControlsBarImmediately];
    } else {
        [self hideControlsBarImmediately];
    }
    [self updateArtworkButtonEnabledState];

    if (!self.playQueueController.playerController.currentMediaIsAudioOnly) {
        [self configureArtworkButtonLiveVideoView];
    }

    self.splitViewController.mainVideoModeEnabled = NO;

    [self willChangeValueForKey:HeimdallLibraryWindowEmbeddedVideoPlaybackActiveKey];
    _embeddedVideoPlaybackActive = NO;
    [self didChangeValueForKey:HeimdallLibraryWindowEmbeddedVideoPlaybackActiveKey];

    if (self.presentLoadingOverlayOnVideoPlaybackHide) {
        [self showLoadingOverlay];
    }

    if ([self.librarySegmentViewController isKindOfClass:HeimdallLibraryAbstractMediaLibrarySegmentViewController.class]) {
        [(HeimdallLibraryAbstractMediaLibrarySegmentViewController *)self.librarySegmentViewController connect];
    }
}

- (void)showLoadingOverlay
{
    if ([self.libraryTargetView.subviews containsObject:self.loadingOverlayView]) {
        return;
    } else if ([self.libraryTargetView.subviews containsObject:self.videoViewController.view]) {
        self.presentLoadingOverlayOnVideoPlaybackHide = YES;
        return;
    }

    self.loadingOverlayView.wantsLayer = YES;
    self.loadingOverlayView.alphaValue = 0.0;

    NSArray * const views = [self.libraryTargetView.subviews arrayByAddingObject:self.loadingOverlayView];
    self.libraryTargetView.subviews = views;
    [self.loadingOverlayView applyConstraintsToFillSuperview];

    [NSAnimationContext runAnimationRespectingPreferencesWithDuration:0.5
                                                              changes:^(NSAnimationContext * const __unused context) {
        self.loadingOverlayView.animator.alphaValue = 1.0;
    } completionHandler:nil];

    [self.loadingOverlayView.indicator startAnimation:self];

}

- (void)hideLoadingOverlay
{
    self.presentLoadingOverlayOnVideoPlaybackHide = NO;

    if (![self.libraryTargetView.subviews containsObject:self.loadingOverlayView]) {
        return;
    }

    self.loadingOverlayView.wantsLayer = YES;
    self.loadingOverlayView.alphaValue = 1.0;

    [NSAnimationContext runAnimationRespectingPreferencesWithDuration:1.0
                                                              changes:^(NSAnimationContext * const __unused context) {
        self.loadingOverlayView.animator.alphaValue = 0.0;
    } completionHandler:^{
        NSMutableArray * const mutableSubviews = self.libraryTargetView.subviews.mutableCopy;
        [mutableSubviews removeObject:self.loadingOverlayView];
        self.libraryTargetView.subviews = mutableSubviews;
        [self.loadingOverlayView.indicator stopAnimation:self];
    }];
}

- (void)mouseMoved:(NSEvent *)o_event
{
    if (self.embeddedVideoPlaybackActive) {
        NSPoint mouseLocation = [o_event locationInWindow];
        NSView *videoView = self.videoViewController.view;
        NSRect videoViewRect = [videoView convertRect:videoView.frame toView:self.contentView];

        if ([self.contentView mouse:mouseLocation inRect:videoViewRect]) {
            [NSNotificationCenter.defaultCenter postNotificationName:HeimdallVideoWindowShouldShowFullscreenController
                                                                object:self];
        }
    }

    [super mouseMoved:o_event];
}

#pragma mark - NSControlTextEditingDelegate

- (NSArray<NSString *> *)control:(NSControl *)control
                         textView:(NSTextView *)textView
                      completions:(NSArray<NSString *> *)words
              forPartialWordRange:(NSRange)charRange
              indexOfSelectedItem:(NSInteger *)index
{
    // Get the partial word being completed
    NSString * const currentText = textView.string;
    NSString * const partialWord = [currentText substringWithRange:charRange];

    if (partialWord.length == 0) {
        return @[];
    }

    // Get cached titles from library model
    HeimdallLibraryModel * const libraryModel = HeimdallMain.sharedInstance.libraryController.libraryModel;
    if (!libraryModel) {
        return @[];
    }

    NSArray<NSString *> * const allTitles = libraryModel.listOfMediaTitles;
    NSMutableArray<NSString *> * const matchingTitles = [NSMutableArray arrayWithCapacity:HeimdallLibrarySearchMaxMatchingTitles];

    // Filter titles that match the partial word
    for (NSString * const title in allTitles) {
        if (![title.lowercaseString hasPrefix:partialWord.lowercaseString]) {
            continue;
        }
        [matchingTitles addObject:title];
        if (matchingTitles.count >= HeimdallLibrarySearchMaxMatchingTitles) {
            break;
        }
    }

    *index = -1;
    return matchingTitles;
}

#pragma mark -
#pragma mark respond to core events

- (void)shouldShowController:(NSNotification *)aNotification
{
    [self makeKeyAndOrderFront:nil];

    if (self.videoViewController.view.isHidden) {
        [self showControlsBar];
        NSView *standardWindowButtonsSuperView = [self standardWindowButton:NSWindowCloseButton].superview;
        standardWindowButtonsSuperView.hidden = NO;
    }
}

- (void)windowWillEnterFullScreen:(NSNotification *)notification
{
    [super windowWillEnterFullScreen:notification];

    if (self.splitViewController.mainVideoModeEnabled) {
        [self hideControlsBar];
    }

    if (self.splitViewController.mainVideoModeEnabled) {
        self.splitViewController.multifunctionSidebarItem.animator.collapsed = YES;
    }
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification
{
    [super windowDidEnterFullScreen:notification];
    if (!self.splitViewController.mainVideoModeEnabled) {
        [self showControlsBar];
    }
}

@end
