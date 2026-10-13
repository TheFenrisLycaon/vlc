/*****************************************************************************
 * HeimdallMainVideoViewController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Claudio Cambra <developer@claudiocambra.com>
 *          Maxime Chapelet <umxprime at videolabs dot io>
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

#import "HeimdallMainVideoViewController.h"

#import <heimdall_common.h>

#import "extensions/NSAnimationContext+HeimdallAdditions.h"
#import "extensions/NSView+HeimdallAdditions.h"
#import "extensions/NSWindow+HeimdallAdditions.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryWindow.h"
#import "library/HeimdallLibraryWindowSidebarRootViewController.h"
#import "library/HeimdallLibraryWindowSplitViewController.h"

#import "main/HeimdallMain.h"

#import "playqueue/HeimdallPlayerController.h"
#import "playqueue/HeimdallPlayQueueController.h"

#import "private/PIPSPI.h"

#import "views/HeimdallBottomBarView.h"
#import "views/HeimdallPlaybackEndViewController.h"
#import "views/HeimdallUIUnits.h"

#import "windows/controlsbar/HeimdallMainVideoViewControlsBar.h"

#import "windows/video/HeimdallMainVideoViewAudioMediaDecorativeView.h"
#import "windows/video/HeimdallMainVideoViewOverlayView.h"
#import "windows/video/HeimdallVideoOutputProvider.h"
#import "windows/video/HeimdallVideoWindowCommon.h"

NSString * const HeimdallUseClassicVideoPlayerLayoutKey = @"HeimdallUseClassicVideoPlayerLayoutKey";

@interface PIPVoutViewController : NSViewController
@property (nonatomic) NSRect previousBounds;
@property (nonatomic, copy) void (^boundsChangeHandler)(void);
@end

@implementation PIPVoutViewController

- (void)setView:(NSView *)view {
    [super setView:view];
    self.previousBounds = NSZeroRect;
}

- (void)viewDidLoad {
    [super viewDidLoad];
}

- (void)viewWillAppear {
    [super viewWillAppear];

    if (self.view.superview) {
        [self.view applyConstraintsToFillSuperview];
    }
}

- (void)viewDidAppear {
    [super viewDidAppear];
}

- (void)viewDidLayout 
{
    [super viewDidLayout];

    NSRect currentBounds = self.view.bounds;
    if (!NSIsEmptyRect(currentBounds) && !NSEqualRects(currentBounds, _previousBounds)) {
            _previousBounds = currentBounds;

            dispatch_async(dispatch_get_main_queue(), ^{
                if (self.boundsChangeHandler) {
                    self.boundsChangeHandler();
                    self.boundsChangeHandler = nil;
                }
            });
    }
}
@end

@interface HeimdallMainVideoViewController() <PIPViewControllerDelegate>
{
    NSTimer *_hideControlsTimer;
    NSLayoutConstraint *_returnButtonBottomConstraint;
    NSLayoutConstraint *_playQueueButtonBottomConstraint;
    PIPViewController *_pipViewController;
    PIPVoutViewController *_voutViewController;

    BOOL _isFadingIn;
}

@property NSWindow *retainedWindow;
@property (readonly) BOOL classic;
@property (readwrite) BOOL voutTaken;

@end

@implementation HeimdallMainVideoViewController

- (instancetype)init
{
    const BOOL classic = [NSUserDefaults.standardUserDefaults boolForKey:HeimdallUseClassicVideoPlayerLayoutKey];
    NSString * const nibName = classic ? @"HeimdallClassicMainVideoView" : @"HeimdallMainVideoView";
    self = [super initWithNibName:nibName bundle:nil];
    if (self) {
        _classic = classic;
        _isFadingIn = NO;
        _voutTaken = NO;

        NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
        [notificationCenter addObserver:self
                               selector:@selector(playerCurrentMediaItemChanged:)
                                   name:HeimdallPlayerCurrentMediaItemChanged
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(playerCurrentItemTrackListChanged:)
                                   name:HeimdallPlayerTrackListChanged
                                 object:nil];
        [notificationCenter addObserver:self
                                   selector:@selector(playerBufferChanged:)
                                   name:HeimdallPlayerBufferChanged
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(floatOnTopChanged:)
                                   name:HeimdallWindowFloatOnTopChangedNotificationName
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(shouldShowControls:)
                                   name:HeimdallVideoWindowShouldShowFullscreenController
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(pictureInPictureChanged:)
                                   name:HeimdallPlayerPictureInPictureChanged
                                 object:nil];

        Class PIPViewControllerClass = NSClassFromString(@"PIPViewController");
        _pipViewController = [[PIPViewControllerClass alloc] init];
        _pipViewController.delegate = self;
        _pipViewController.userCanResize = true;
    }
    return self;
}

- (void)setupAudioDecorativeView
{
    _audioDecorativeView = [HeimdallMainVideoViewAudioMediaDecorativeView fromNibWithOwner:self];
    _audioDecorativeView.translatesAutoresizingMaskIntoConstraints = NO;

    if (!self.classic) {
        // Only set up button constraints for standard layout
        _bottomButtonStackViewConstraint =
            [self.bottomBarView.topAnchor constraintEqualToAnchor:self.centralControlsStackView.bottomAnchor];
        [self updateLyricsInsets];
    }

    HeimdallPlayerController * const controller =
        HeimdallMain.sharedInstance.playQueueController.playerController;
    [self updateDecorativeViewVisibilityOnControllerChange:controller];
}

- (void)viewDidLoad
{
    self.loadingIndicator.hidden = YES;

    BOOL floatOnTopActive = NO;
    HeimdallVideoWindowCommon * const window = (HeimdallVideoWindowCommon *)self.view.window;
    vout_thread_t * const p_vout = window.videoViewController.voutView.voutThread;
    if (p_vout) {
        floatOnTopActive = var_GetBool(p_vout, "video-on-top");
        vout_Release(p_vout);
    }
    self.floatOnTopIndicatorImageView.hidden = !floatOnTopActive;

    _autohideControls = YES;

    [self setDisplayLibraryControls:NO];
    [self updatePlayQueueToggleState];
    [self updateLibraryControls];

    _returnButtonBottomConstraint = [NSLayoutConstraint constraintWithItem:_returnButton
                                                                 attribute:NSLayoutAttributeBottom
                                                                 relatedBy:NSLayoutRelationEqual
                                                                    toItem:_fakeTitleBar
                                                                 attribute:NSLayoutAttributeBottom
                                                                multiplier:1.
                                                                  constant:0];
    _playQueueButtonBottomConstraint = [NSLayoutConstraint constraintWithItem:_playQueueButton
                                                                    attribute:NSLayoutAttributeBottom
                                                                    relatedBy:NSLayoutRelationEqual
                                                                       toItem:_fakeTitleBar
                                                                    attribute:NSLayoutAttributeBottom
                                                                   multiplier:1.
                                                                     constant:0];

    _returnButtonBottomConstraint.active = NO;
    _playQueueButtonBottomConstraint.active = NO;

    [self setupAudioDecorativeView];
    [self.controlsBar update];
    [self updateFloatOnTopIndicator];
    [self updateLyricsInsets];
    
    // For classic layout, create constraint for video view to fill main view when controls are hidden
    if (self.classic) {
        _videoViewBottomToViewConstraint = [self.voutContainingView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor];
        self.videoViewBottomToViewConstraint.active = NO;

        if (@available(macOS 26.0, *)) {
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 260000
            NSGlassEffectContainerView * const glassEffectView = [[NSGlassEffectContainerView alloc] initWithFrame:self.bottomBarView.frame];
            glassEffectView.translatesAutoresizingMaskIntoConstraints = NO;
            [self.classicViewBottomBarContainerView addSubview:glassEffectView];
            [glassEffectView applyConstraintsToFillSuperview];
            [self.bottomBarView removeFromSuperview];
            glassEffectView.contentView = self.bottomBarView;
#endif
        } else {
            NSVisualEffectView * const controlsBackgroundView = [[NSVisualEffectView alloc] initWithFrame:self.bottomBarView.frame];
            controlsBackgroundView.translatesAutoresizingMaskIntoConstraints = NO;
            controlsBackgroundView.blendingMode = NSVisualEffectBlendingModeWithinWindow;
            controlsBackgroundView.material = NSVisualEffectMaterialTitlebar;
            [self.bottomBarView addSubview:controlsBackgroundView positioned:NSWindowBelow relativeTo:self.bottomBarView.subviews.firstObject];
            [controlsBackgroundView applyConstraintsToFillSuperview];
        }
    }
}

- (void)updateLyricsInsets
{
    const CGFloat topInset = self.fakeTitleBar.frame.size.height + HeimdallUIUnits.largeSpacing;
    const CGFloat bottomInset = self.bottomBarView.frame.size.height + HeimdallUIUnits.largeSpacing;

    NSScrollView * const scrollView = self.audioDecorativeView.lyricsScrollView;
    scrollView.automaticallyAdjustsContentInsets = NO;
    scrollView.contentInsets = NSEdgeInsetsMake(topInset, 0, bottomInset, 0);
    scrollView.scrollerInsets = NSEdgeInsetsMake(topInset, 0, 0, 0);
}

- (BOOL)isVisualizationActive
{
    BOOL isVisualActive;
    HeimdallPlayerController * const controller = HeimdallMain.sharedInstance.playQueueController.playerController;
    audio_output_t * const p_aout = controller.mainAudioOutput;
    if (!p_aout) {
        return NO;
    }
    
    char *psz_visual = var_GetString(p_aout, "visual");
    isVisualActive = psz_visual && psz_visual[0] != '\0';
    
    free(psz_visual);
    aout_Release(p_aout);
    
    return isVisualActive;
}

- (void)updateDecorativeViewVisibilityOnControllerChange:(HeimdallPlayerController *)controller
{
    const BOOL isAudioOnly = controller.currentMediaIsAudioOnly;
    const BOOL isVisualActive = [self isVisualizationActive];
    const BOOL decorativeViewVisible = isAudioOnly && !isVisualActive;

    // Keep voutView mounted so the macOS vout module always finds an in-window
    // NSView when WindowEnable runs. Otherwise an audio item that later exposes
    // a video track (e.g. m4b chapter slides) races: WindowEnable can land
    // before HeimdallPlayerTrackListChanged re-attaches voutView, and the vout
    // module then binds its layer to a detached view.
    if (self.voutView.superview != self.voutContainingView) {
        [self.voutContainingView addSubview:self.voutView
                                 positioned:NSWindowBelow
                                 relativeTo:nil];
        [self.voutView applyConstraintsToFillSuperview];
    }

    if (decorativeViewVisible) {
        if (self.audioDecorativeView.superview != self.voutContainingView) {
            [self.voutContainingView addSubview:self.audioDecorativeView];
            [self.audioDecorativeView applyConstraintsToFillSuperview];
        }
    } else {
        [self.audioDecorativeView removeFromSuperview];
    }

    if (decorativeViewVisible) {
        [self setAutohideControls:NO];
        self.centerButtonStackInViewConstraint.active = NO;
        self.bottomButtonStackViewConstraint.active = YES;
        self.prevButtonSizeConstraint.constant = HeimdallUIUnits.smallPlaybackControlButtonSize;
        self.playButtonSizeConstraint.constant = HeimdallUIUnits.smallPlaybackControlButtonSize;
        self.nextButtonSizeConstraint.constant = HeimdallUIUnits.smallPlaybackControlButtonSize;
        self.jumpBackwardButtonSizeConstraint.constant = HeimdallUIUnits.smallPlaybackControlButtonSize;
        self.jumpForwardButtonSizeConstraint.constant = HeimdallUIUnits.smallPlaybackControlButtonSize;
        [self applyAudioDecorativeViewForegroundCoverArtViewConstraints];
    } else {
        [self setAutohideControls:!self.classic];
        self.bottomButtonStackViewConstraint.active = NO;
        self.centerButtonStackInViewConstraint.active = YES;
        self.prevButtonSizeConstraint.constant = HeimdallUIUnits.mediumPlaybackControlButtonSize;
        self.playButtonSizeConstraint.constant = HeimdallUIUnits.largePlaybackControlButtonSize;
        self.nextButtonSizeConstraint.constant = HeimdallUIUnits.mediumPlaybackControlButtonSize;
        self.jumpBackwardButtonSizeConstraint.constant = HeimdallUIUnits.mediumPlaybackControlButtonSize;
        self.jumpForwardButtonSizeConstraint.constant = HeimdallUIUnits.mediumPlaybackControlButtonSize;
    }
}

- (void)applyAudioDecorativeViewForegroundCoverArtViewConstraints
{
    if (self.classic) {
        return;
    }
    if (![self.view.subviews containsObject:self.voutContainingView] ||
        ![self.voutContainingView.subviews containsObject:self.audioDecorativeView]) {
        return;
    }

    NSView * const foregroundCoverArtView = self.audioDecorativeView.foregroundCoverArtView;
    [NSLayoutConstraint activateConstraints:@[
        [self.centralControlsStackView.topAnchor constraintGreaterThanOrEqualToAnchor:foregroundCoverArtView.bottomAnchor constant:HeimdallUIUnits.largeSpacing],
        [self.fakeTitleBar.bottomAnchor constraintLessThanOrEqualToAnchor:foregroundCoverArtView.topAnchor constant:-HeimdallUIUnits.largeSpacing]
    ]];
}

- (void)playerCurrentMediaItemChanged:(NSNotification *)notification
{
    NSParameterAssert(notification);
    HeimdallPlayerController * const controller = notification.object;
    NSAssert(controller != nil, 
             @"Player current media item changed notification should have valid player controller");
    [self updateDecorativeViewVisibilityOnControllerChange:controller];

    // New item playing, dismiss playback end view
    [self.playbackEndViewController.countdownTimer invalidate];
    [self.playbackEndViewController.view removeFromSuperview];
}

- (void)playerCurrentItemTrackListChanged:(NSNotification *)notification
{
    NSParameterAssert(notification);
    HeimdallPlayerController * const controller = notification.object;
    NSAssert(controller != nil, 
             @"Player current item track list changed notification should have valid player controller");
    [self updateDecorativeViewVisibilityOnControllerChange:controller];
}

- (void)playerBufferChanged:(NSNotification *)notification
{
    NSParameterAssert(notification);
    NSParameterAssert(notification.userInfo != nil);

    NSNumber * const bufferFillNumber = notification.userInfo[HeimdallPlayerBufferFill];
    NSAssert(bufferFillNumber != nil, @"Buffer fill number should not be nil");

    const float bufferFill = bufferFillNumber.floatValue;
    self.loadingIndicator.hidden = bufferFill == 1.0;
    if (self.loadingIndicator.hidden) {
        [self.loadingIndicator stopAnimation:self];
    } else {
        [self.loadingIndicator startAnimation:self];
    }
}

- (void)floatOnTopChanged:(NSNotification *)notification
{
    HeimdallVideoWindowCommon * const videoWindow = (HeimdallVideoWindowCommon *)notification.object;
    NSAssert(videoWindow != nil, @"Received video window should not be nil!");
    HeimdallVideoWindowCommon * const selfVideoWindow = (HeimdallVideoWindowCommon *)self.view.window;

    if (videoWindow != selfVideoWindow) {
        return;
    }

    [self updateFloatOnTopIndicator];
}

- (void)updateFloatOnTopIndicator
{
    vout_thread_t * const voutThread = self.voutView.voutThread;
    if (voutThread == NULL) {
        return;
    }

    const bool floatOnTopEnabled = var_GetBool(voutThread, "video-on-top");
    self.floatOnTopIndicatorImageView.hidden = !floatOnTopEnabled;
}

- (BOOL)mouseOnControls
{
    NSPoint mousePos = [self.view.window mouseLocationOutsideOfEventStream];

    return [_centralControlsStackView mouse:mousePos inRect:_centralControlsStackView.frame] ||
        [_controlsBar.bottomBarView mouse:mousePos inRect: _controlsBar.bottomBarView.frame] ||
        [_returnButton mouse:mousePos inRect: _returnButton.frame] ||
        [_playQueueButton mouse:mousePos inRect: _playQueueButton.frame];
}

- (void)stopAutohideTimer
{
    [_hideControlsTimer invalidate];
}

- (void)startAutohideTimer
{
    /* Do nothing if timer is already in place */
    if (_hideControlsTimer.valid) {
        return;
    }

    /* Get timeout and make sure it is not lower than 1 second */
    long long timeToKeepVisibleInSec = MAX(var_CreateGetInteger(getIntf(), "mouse-hide-timeout") / 1000, 1);

    _hideControlsTimer = [NSTimer scheduledTimerWithTimeInterval:timeToKeepVisibleInSec
                                                          target:self
                                                        selector:@selector(shouldHideControls:)
                                                        userInfo:nil
                                                         repeats:NO];
}

- (void)shouldHideControls:(id)sender
{
    [self hideControls];
    [NSCursor setHiddenUntilMouseMoves: YES];
}

- (void)hideControls
{
    if (self.voutTaken) {
        return;
    }

    [self stopAutohideTimer];

    if ([self mouseOnControls]) {
        [self showControls];
        return;
    }

    if (self.classic) {
        self.videoViewBottomConstraint.active = NO;
        self.videoViewBottomToViewConstraint.active = YES;
    }

    [NSAnimationContext runAnimationRespectingPreferencesWithDuration:HeimdallUIUnits.controlsFadeAnimationDuration
                                                              changes:^(NSAnimationContext * const _Nonnull __unused context) {
        [self->_mainControlsView.animator setAlphaValue:0.0f];
        [self->_floatOnTopIndicatorImageView.animator setAlphaValue:0.0f];
    } completionHandler:nil];
}

- (void)setAutohideControls:(BOOL)autohide
{
    if (_autohideControls == autohide) {
        return;
    }

    _autohideControls = autohide;

    if (autohide) {
        [self startAutohideTimer];
    } else {
        [self showControls];
    }
}

- (void)shouldShowControls:(NSNotification *)aNotification
{
    [self showControls];
}

- (void)showControls
{
    if (self.voutTaken) {
        return;
    }

    [self stopAutohideTimer];
    [self updatePlayQueueToggleState];
    [self updateLibraryControls];

    if (self.classic) {
        self.videoViewBottomToViewConstraint.active = NO;
        self.videoViewBottomConstraint.active = YES;
    }

    if (!_autohideControls) {
        _mainControlsView.alphaValue = 1.0f;        
        return;
    }


    [NSAnimationContext runAnimationRespectingPreferencesWithDuration:HeimdallUIUnits.controlsFadeAnimationDuration
                                                              changes:^(NSAnimationContext * const _Nonnull __unused context) {
        self->_isFadingIn = YES;
        [self->_mainControlsView.animator setAlphaValue:1.0f];
        [self->_floatOnTopIndicatorImageView.animator setAlphaValue:1.0f];
    } completionHandler:^{
        self->_isFadingIn = NO;
        [self startAutohideTimer];
    }];
}

- (void)setDisplayLibraryControls:(BOOL)displayLibraryControls
{
    _displayLibraryControls = displayLibraryControls;

    _returnButton.hidden = !displayLibraryControls;
    _playQueueButton.hidden = !displayLibraryControls;
}

- (void)updatePlayQueueToggleState
{
    HeimdallLibraryWindow * const libraryWindow = (HeimdallLibraryWindow*)self.view.window;
    if (libraryWindow != nil && _displayLibraryControls) {
        const BOOL sidebarCollapsed = libraryWindow.splitViewController.multifunctionSidebarItem.isCollapsed;
        self.playQueueButton.state = sidebarCollapsed ? NSControlStateValueOff : NSControlStateValueOn;
    }
}

- (NSRect)windowButtonsRect
{
    NSWindow * const window = self.view.window;
    NSRect buttonBox = NSZeroRect;

    NSButton * const closeButton = [window standardWindowButton:NSWindowCloseButton];
    if (closeButton) {
        buttonBox = NSUnionRect(buttonBox, closeButton.frame);
    }

    NSButton * const minimizeButton = [window standardWindowButton:NSWindowMiniaturizeButton];
    if (minimizeButton) {
        buttonBox = NSUnionRect(buttonBox, minimizeButton.frame);
    }

    NSButton * const zoomButton = [window standardWindowButton:NSWindowZoomButton];
    if (zoomButton) {
        buttonBox = NSUnionRect(buttonBox, zoomButton.frame);
    }

    return buttonBox;
}

- (void)updateLibraryControls:(NSNotification *)notification
{
    [self updateLibraryControls];
}

- (void)updateLibraryControls
{
    const NSWindow * const viewWindow = self.view.window;
    const CGFloat windowTitlebarHeight = viewWindow.titlebarHeight;

    const BOOL windowFullscreen = [(HeimdallWindow*)viewWindow isInNativeFullscreen] ||
                                  [(HeimdallWindow*)viewWindow fullscreen];
    const BOOL placeInFakeToolbar = viewWindow.titlebarAppearsTransparent &&
                                    !windowFullscreen;

    const CGFloat buttonTopSpace = placeInFakeToolbar ? 0 : HeimdallUIUnits.largeSpacing;

    _fakeTitleBarHeightConstraint.constant = windowFullscreen ? 0 : windowTitlebarHeight;

    if (!self.displayLibraryControls) {
        return;
    }

    [NSNotificationCenter.defaultCenter addObserver:self
                                             selector:@selector(updateLibraryControls:)
                                                 name:HeimdallVideoWindowDidEnterFullscreen
                                               object:nil];

    _returnButtonTopConstraint.constant = buttonTopSpace;
    _playQueueButtonTopConstraint.constant = buttonTopSpace;
    _returnButtonBottomConstraint.active = placeInFakeToolbar;
    _playQueueButtonBottomConstraint.active = placeInFakeToolbar;

    NSControlSize buttonSize = NSControlSizeRegular;

    if (@available(macOS 11.0, *)) {
        if (!placeInFakeToolbar) {
            buttonSize = NSControlSizeLarge;
        } else if (viewWindow.toolbarStyle != NSWindowToolbarStyleUnified) {
            buttonSize = NSControlSizeMini;
        }
    } else if (placeInFakeToolbar) {
        buttonSize = NSControlSizeMini;
    }

    NSControlSize previousButtonSize = _playQueueButton.controlSize;
    _returnButton.controlSize = buttonSize;
    _playQueueButton.controlSize = buttonSize;

    // HACK: Upon changing the control size the actual highlight of toggled/hovered buttons doesn't change
    // properly, at least for recessed buttons. This is most obvious on the toggleable playlist button.
    // So reset the state and then retoggle once done.
    if (previousButtonSize != buttonSize) {
        const NSControlStateValue returnButtonControlState = _returnButton.state;
        const NSControlStateValue playQueueButtonControlState = _playQueueButton.state;
        _returnButton.state = NSControlStateValueOff;
        _playQueueButton.state = NSControlStateValueOff;
        _returnButton.state = returnButtonControlState;
        _playQueueButton.state = playQueueButtonControlState;
    }

    const CGFloat realButtonSpace = (windowTitlebarHeight - _playQueueButton.cell.cellSize.height) / 2;
    const NSRect windowButtonBox = [self windowButtonsRect];

    _returnButtonLeadingConstraint.constant = placeInFakeToolbar ? windowButtonBox.size.width + HeimdallUIUnits.mediumSpacing + realButtonSpace : HeimdallUIUnits.largeSpacing;
    _playQueueButtonTrailingConstraint.constant = placeInFakeToolbar ? realButtonSpace: HeimdallUIUnits.largeSpacing;

    _overlayView.drawGradientForTopControls = !placeInFakeToolbar;
    [_overlayView setNeedsDisplay:YES];
}

- (BOOL)pipIsActive
{
    return _voutViewController != nil;
}

- (void)pictureInPictureChanged:(NSNotification *)notification
{
    if (self.pipIsActive) {
        return;
    }

    HeimdallVideoWindowCommon * const videoWindow =
        [HeimdallMain.sharedInstance.voutProvider videoWindowForVoutView:self.voutView];
    if (videoWindow.fullscreen && !videoWindow.isInNativeFullscreen) {
        [videoWindow leaveFullscreenWithAnimation:NO];
    }

    NSWindow * const window = self.view.window;
    [window orderOut:window];
    self.retainedWindow = window;

    _voutViewController = [PIPVoutViewController new];
    _voutViewController.view = [self acquireVideoView];
    NSAssert(_voutViewController.view != nil, @"Vout view should not be nil");
    HeimdallPlayerController * const controller = notification.object;
    _pipViewController.playing = controller.playerState == HEIMDALL_PLAYER_STATE_PLAYING;
    
    HeimdallInputItem * const item = controller.currentMedia;
    input_item_t * const p_input = item.heimdallInputItem;
    heimdall_mutex_lock(&p_input->lock);
    const struct input_item_es *item_es;
    heimdall_vector_foreach_ref(item_es, &p_input->es_vec) {
        if (item_es->es.i_cat != VIDEO_ES) {
            continue;
        }
        const video_format_t * const fmt = &item_es->es.video;
        unsigned int width = fmt->i_visible_width;
        unsigned int height = fmt->i_visible_height;
        if (fmt->i_sar_num && fmt->i_sar_den) {
            height = (height * fmt->i_sar_den) / fmt->i_sar_num;
        }
        _pipViewController.aspectRatio = CGSizeMake(width, height);
        break;
    }
    heimdall_mutex_unlock(&p_input->lock);
    _pipViewController.title = window.title;
    
    __weak typeof(self) weakSelf = self;

    void (^presentVcAsPip)(void) = ^() {
        typeof(self) strongSelf = weakSelf;
        if (strongSelf && strongSelf->_voutViewController.presentingViewController == nil) {
            [strongSelf->_pipViewController presentViewControllerAsPictureInPicture:strongSelf->_voutViewController];
        }
    };

    if (!controller.currentMediaIsAudioOnly) {
        _voutViewController.boundsChangeHandler = presentVcAsPip;
    }

    // Present PiP asynchronously. Previously the video path waited for
    // PIPVoutViewController's viewDidLayout to fire via boundsChangeHandler, but
    // on macOS 26 Tahoe the detached view never receives a layout pass, so the
    // handler never fired. The acquired view already has valid bounds so we can
    // present immediately on the next run-loop cycle for both paths.
    dispatch_async(dispatch_get_main_queue(), ^{
        presentVcAsPip();
    });

    if ([window isKindOfClass:HeimdallLibraryWindow.class]) {
        [self returnToLibrary:self];
    }
}

- (IBAction)togglePlayQueue:(id)sender
{
    HeimdallLibraryWindow * const libraryWindow = (HeimdallLibraryWindow*)self.view.window;
    if (libraryWindow != nil) {
        [libraryWindow.splitViewController toggleMultifunctionSidebar:self];
    }
}

- (IBAction)returnToLibrary:(id)sender
{
    if (self.playbackEndViewController) { // Stop any pending automatic return to library
        [self.playbackEndViewController.countdownTimer invalidate];
        [self.playbackEndViewController.view removeFromSuperview];
    }
    if (self.view.window.class == HeimdallLibraryWindow.class) {
        [(HeimdallLibraryWindow *)self.view.window disableVideoPlaybackAppearance];
    } else {
        [self.view.window close];
    }
}

- (nullable NSView *)acquireVideoView
{
    if (self.voutTaken) {
        return nil;
    }
    self.voutTaken = YES;
    [self.voutContainingView removeFromSuperview];
    return self.voutContainingView;
}

- (void)returnVideoView:(NSView *)videoView
{
    NSParameterAssert(videoView != nil);
    [videoView removeFromSuperview];
    [self.view addSubview:videoView
               positioned:NSWindowBelow
               relativeTo:self.mainControlsView];
    
    if (self.classic) {
        videoView.translatesAutoresizingMaskIntoConstraints = NO;
        [NSLayoutConstraint activateConstraints:@[
            [videoView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
            [videoView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
            [videoView.topAnchor constraintEqualToAnchor:self.view.topAnchor]
        ]];
        
        // Create constraint for video view bottom to main view bottom (for when controls are hidden)
        _videoViewBottomToViewConstraint = [videoView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor];
        _videoViewBottomConstraint = [videoView.bottomAnchor constraintEqualToAnchor:self.bottomBarView.topAnchor];
        
        // The videoViewBottomConstraint outlet connects video bottom to controls top (for when controls are visible)
        // Start with controls visible, so activate the XIB constraint
        self.videoViewBottomConstraint.active = YES;
        self.videoViewBottomToViewConstraint.active = NO;
    } else {
        [videoView applyConstraintsToFillSuperview];
    }
    
    [self applyAudioDecorativeViewForegroundCoverArtViewConstraints];
    self.voutTaken = NO;
}

- (void)displayPlaybackEndView
{
    if (_playbackEndViewController == nil) {
        _playbackEndViewController = [[HeimdallPlaybackEndViewController alloc] init];

        NSNotificationCenter * const defaultCenter = NSNotificationCenter.defaultCenter;
        [defaultCenter addObserver:self
                          selector:@selector(playbackEndViewReturnToLibrary:)
                              name:HeimdallPlaybackEndViewReturnToLibraryNotificationName
                            object:self.playbackEndViewController];
        [defaultCenter addObserver:self
                          selector:@selector(playbackEndViewHide:)
                              name:HeimdallPlaybackEndViewHideNotificationName
                            object:self.playbackEndViewController];
    }

    self.playbackEndViewController.hideLibraryControls = !self.displayLibraryControls;
    [self.view addSubview:self.playbackEndViewController.view];
    [self.playbackEndViewController.view.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor].active = YES;
    [self.playbackEndViewController.view.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor].active = YES;
    [self.playbackEndViewController.view.leadingAnchor constraintGreaterThanOrEqualToAnchor:self.view.leadingAnchor].active = YES;
    [self.playbackEndViewController.view.trailingAnchor constraintLessThanOrEqualToAnchor:self.view.trailingAnchor].active = YES;
    [self.playbackEndViewController startCountdown];
}

- (void)playbackEndViewReturnToLibrary:(NSNotification *)notification
{
    if (self.endViewDismissHandler)
        self.endViewDismissHandler();
    else
        [self returnToLibrary:self];
}

- (void)playbackEndViewHide:(NSNotification *)notification
{
    [self.playbackEndViewController.view removeFromSuperview];
    if (self.endViewDismissHandler)
        self.endViewDismissHandler();
}

#pragma mark - PIPViewControllerDelegate

- (BOOL)pipShouldClose:(PIPViewController *)pip
{
    return YES;
}

- (void)pipWillClose:(PIPViewController *)pip
{
    NSWindow * const window = self.retainedWindow;
    pip.replacementWindow = window;
    pip.replacementRect = self.view.frame;
    if ([window isKindOfClass:HeimdallLibraryWindow.class]) {
        [(HeimdallLibraryWindow *)window enableVideoPlaybackAppearance];
    }
    [window makeKeyAndOrderFront:window];
    self.retainedWindow = nil;
}

- (void)pipDidClose:(PIPViewController *)pip
{
    [self returnVideoView:_voutViewController.view];
    _voutViewController = nil;
}

- (void)pipActionPlay:(PIPViewController *)pip
{
    HeimdallPlayerController * const controller =
        HeimdallMain.sharedInstance.playQueueController.playerController;
    if (controller.playerState == HEIMDALL_PLAYER_STATE_PAUSED) {
        [controller resume];
    } else {
        [controller start];
    }
}

- (void)pipActionStop:(PIPViewController *)pip
{
    HeimdallPlayerController * const controller =
        HeimdallMain.sharedInstance.playQueueController.playerController;
    [controller pause];
}

- (void)pipActionPause:(PIPViewController *)pip
{
    HeimdallPlayerController * const controller =
        HeimdallMain.sharedInstance.playQueueController.playerController;
    [controller pause];
}

@end
