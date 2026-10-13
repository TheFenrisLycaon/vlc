/*****************************************************************************
 * HeimdallVideoOutputProvider.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2012-2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne -at- lycaria -dot- org>
 *          David Fuhrmann <david dot fuhrmann at googlemail dot com>
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

#import "HeimdallVideoOutputProvider.h"

#import "extensions/NSAnimationContext+HeimdallAdditions.h"
#import "extensions/NSScreen+HeimdallAdditions.h"

#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryWindow.h"

#import "main/CompatibilityFixes.h"
#import "main/HeimdallMain.h"

#import "os-integration/HeimdallKeyboardBacklightControl.h"

#import "panels/HeimdallVideoEffectsWindowController.h"
#import "panels/HeimdallAudioEffectsWindowController.h"
#import "panels/HeimdallBookmarksWindowController.h"
#import "panels/HeimdallTrackSynchronizationWindowController.h"

#import "playqueue/HeimdallPlayQueueController.h"
#import "playqueue/HeimdallPlayerController.h"

#import "views/HeimdallPlaybackEndViewController.h"

#import "windows/video/HeimdallAspectRatioRetainingVideoWindow.h"
#import "windows/video/HeimdallMainVideoViewController.h"
#import "windows/video/HeimdallVoutView.h"

#include <heimdall_configuration.h>
#include <heimdall_vout_display.h>

NSString * const HeimdallWindowShouldUpdateLevel = @"HeimdallWindowShouldUpdateLevel";
NSString * const HeimdallWindowLevelKey = @"HeimdallWindowLevelKey";
NSString * const HeimdallWindowFloatOnTopChangedNotificationName = @"HeimdallWindowFloatOnTopChanged";
NSString * const HeimdallWindowFloatOnTopEnabledNotificationKey = @"HeimdallWindowFloatOnTopEnabled";

static int WindowEnable(heimdall_window_t *p_wnd, const heimdall_window_cfg_t *cfg)
{
    @autoreleasepool {
        msg_Dbg(p_wnd, "Opening video window");

        NSRect proposedVideoViewPosition = NSMakeRect(cfg->x, cfg->y, cfg->width, cfg->height);

        HeimdallVideoOutputProvider *voutProvider = HeimdallMain.sharedInstance.voutProvider;
        if (!voutProvider) {
            return HEIMDALL_EGENERIC;
        }

        __block HeimdallVoutView *videoView = nil;

        dispatch_sync(dispatch_get_main_queue(), ^{
            videoView = [voutProvider setupVoutForWindow:p_wnd
                             withProposedVideoViewPosition:proposedVideoViewPosition];
        });

        // this method is not supposed to fail
        assert(videoView != nil);

        msg_Dbg(getIntf(), "returning videoview with proposed position x=%i, y=%i, width=%i, height=%i", cfg->x, cfg->y, cfg->width, cfg->height);
        p_wnd->handle.nsobject = (void *)CFBridgingRetain(videoView);
    }
    if (cfg->is_fullscreen)
        heimdall_window_SetFullScreen(p_wnd, NULL);
    return HEIMDALL_SUCCESS;
}

static void WindowDisable(heimdall_window_t *p_wnd)
{
    @autoreleasepool {
        HeimdallVideoOutputProvider *voutProvider = HeimdallMain.sharedInstance.voutProvider;

        dispatch_async(dispatch_get_main_queue(), ^{
            [voutProvider removeVoutForDisplay:[NSValue valueWithPointer:p_wnd]];
        });
    }
}

static void WindowResize(heimdall_window_t *p_wnd,
                         unsigned i_width, unsigned i_height)
{
    @autoreleasepool {
        HeimdallVideoOutputProvider *voutProvider = HeimdallMain.sharedInstance.voutProvider;

        dispatch_async(dispatch_get_main_queue(), ^{
            [voutProvider setNativeVideoSize:NSMakeSize(i_width, i_height)
                          forWindow:p_wnd];
        });
    }
}

static void WindowSetState(heimdall_window_t *p_wnd, unsigned i_state)
{
    if (i_state & HEIMDALL_WINDOW_STATE_BELOW)
        msg_Dbg(p_wnd, "Ignore change to HEIMDALL_WINDOW_STATE_BELOW");

    @autoreleasepool {
        HeimdallVideoOutputProvider * const voutProvider = HeimdallMain.sharedInstance.voutProvider;

        NSInteger i_cocoa_level = NSNormalWindowLevel;
        if (i_state & HEIMDALL_WINDOW_STATE_ABOVE)
            i_cocoa_level = NSStatusWindowLevel;

        dispatch_async(dispatch_get_main_queue(), ^{
            [voutProvider setWindowLevel:i_cocoa_level forWindow:p_wnd];
        });

    }
}

static const char windowed;

static void WindowSetFullscreen(heimdall_window_t *p_wnd, const char *psz_id)
{
    if (var_InheritBool(getIntf(), "video-wallpaper")) {
        msg_Dbg(p_wnd, "Ignore fullscreen event as video-wallpaper is on");
        return;
    }

    int i_full = psz_id != &windowed;
    BOOL b_animation = YES;

    @autoreleasepool {
        HeimdallVideoOutputProvider *voutProvider = HeimdallMain.sharedInstance.voutProvider;

        dispatch_async(dispatch_get_main_queue(), ^{
            [voutProvider setFullscreen:i_full
                          forWindow:p_wnd
                          withAnimation:b_animation];
        });
    }
}

static void WindowUnsetFullscreen(heimdall_window_t *wnd)
{
    WindowSetFullscreen(wnd, &windowed);
}

static atomic_bool b_intf_starting = false;

static const struct heimdall_window_operations ops = {
    .enable = WindowEnable,
    .disable = WindowDisable,
    .resize = WindowResize,
    .set_state = WindowSetState,
    .unset_fullscreen = WindowUnsetFullscreen,
    .set_fullscreen = WindowSetFullscreen,
};

int WindowOpen(heimdall_window_t *p_wnd)
{
    if (!atomic_load(&b_intf_starting)) {
        msg_Err(p_wnd, "Cannot create vout as Mac OS X interface was not found");
        return HEIMDALL_EGENERIC;
    }

    p_wnd->type = HEIMDALL_WINDOW_TYPE_NSOBJECT;
    p_wnd->ops = &ops;
    return HEIMDALL_SUCCESS;
}

static int WindowFloatOnTop(heimdall_object_t *obj,
                     char const *psz_var,
                     heimdall_value_t oldval,
                     heimdall_value_t newval,
                     void *p_data)
{
    HEIMDALL_UNUSED(obj); HEIMDALL_UNUSED(psz_var); HEIMDALL_UNUSED(oldval);
    @autoreleasepool {
        dispatch_async(dispatch_get_main_queue(), ^{
            HeimdallVideoWindowCommon * const videoWindow = (__bridge HeimdallVideoWindowCommon *)p_data;
            NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
            NSDictionary<NSString *, NSNumber *> * const userInfo = @{
                HeimdallWindowFloatOnTopEnabledNotificationKey: @(newval.b_bool)
            };
            [notificationCenter postNotificationName:HeimdallWindowFloatOnTopChangedNotificationName
                                              object:videoWindow
                                            userInfo:userInfo];
        });
        return HEIMDALL_SUCCESS;
    }
}

@interface HeimdallVideoOutputProvider ()
{
    NSMutableDictionary *_voutWindows;
    HeimdallKeyboardBacklightControl *_keyboardBacklight;

    NSPoint _topLeftPoint;

    // save the status level if at least one video window is on status level
    NSUInteger _statusLevelWindowCounter;
    NSInteger _currentWindowLevel;

    BOOL b_mainWindowHasVideo;

    HeimdallPlayerController *_playerController;
}
@end

@implementation HeimdallVideoOutputProvider

- (id)init
{
    self = [super init];
    if (self) {
        atomic_store(&b_intf_starting, true);
        _voutWindows = [[NSMutableDictionary alloc] init];
        _keyboardBacklight = [[HeimdallKeyboardBacklightControl alloc] init];
        _currentWindowLevel = NSNormalWindowLevel;
        _currentStatusWindowLevel = NSFloatingWindowLevel;
    }
    return self;
}

- (void)dealloc
{
    NSArray *keys = [_voutWindows allKeys];
    for (NSValue *key in keys)
        [self removeVoutForDisplay:key];

    if (var_InheritBool(getIntf(), "macosx-dim-keyboard")) {
        [_keyboardBacklight switchLightsInstantly:YES];
    }
}

#pragma mark -
#pragma mark Methods for vout provider

- (HeimdallVideoWindowCommon *)borderlessVideoWindowAsVideoWallpaper:(BOOL)asVideoWallpaper withWindowDecorations:(BOOL)withWindowDecorations
{
    HeimdallMain *mainInstance = HeimdallMain.sharedInstance;

    // videoWallpaper is priorized over !windowDecorations
    msg_Dbg(getIntf(), "Creating background / blank window");
    NSScreen *screen = [NSScreen screenWithDisplayID:(CGDirectDisplayID)var_InheritInteger(getIntf(), "macosx-vdev")];
    if (!screen) {
        screen = mainInstance.libraryWindow.screen;
    }

    NSRect window_rect = asVideoWallpaper ? screen.frame : mainInstance.libraryWindow.frame;
    NSUInteger mask = withWindowDecorations ? NSWindowStyleMaskBorderless | NSWindowStyleMaskResizable : NSWindowStyleMaskBorderless;

    HeimdallVideoWindowCommon *newVideoWindow = [[HeimdallVideoWindowCommon alloc] initWithContentRect:window_rect styleMask:mask backing:NSBackingStoreBuffered defer:YES];
    newVideoWindow.delegate = newVideoWindow;
    newVideoWindow.releasedWhenClosed = NO;

    newVideoWindow.backgroundColor = [NSColor blackColor];
    newVideoWindow.canBecomeKeyWindow = !asVideoWallpaper;
    newVideoWindow.canBecomeMainWindow = !asVideoWallpaper;
    newVideoWindow.acceptsMouseMovedEvents = !asVideoWallpaper;
    newVideoWindow.movableByWindowBackground = !asVideoWallpaper;

    newVideoWindow.videoViewController.displayLibraryControls = NO;
    newVideoWindow.videoViewController.view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    newVideoWindow.videoViewController.view.frame = newVideoWindow.contentView.frame;

    [newVideoWindow.contentView addSubview:newVideoWindow.videoViewController.view positioned:NSWindowAbove relativeTo:nil];

    if (asVideoWallpaper) {
        [newVideoWindow setLevel:CGWindowLevelForKey(kCGDesktopWindowLevelKey) + 1];
        [newVideoWindow orderBack:nil];
    } else {
        BOOL multipleVoutWindows = _voutWindows.count > 0;
        // no frame autosave for additional vout windows
        if (!multipleVoutWindows) {
            // initial window position
            [newVideoWindow center];
            newVideoWindow.frameAutosaveName = @"extra-videowindow";
        }

        newVideoWindow.contentMinSize = NSMakeSize(HeimdallVideoWindowCommonMinimalHeight, HeimdallVideoWindowCommonMinimalHeight);
    }

    return newVideoWindow;
}

- (HeimdallVideoWindowCommon *)setupMainLibraryVideoWindow
{
    HeimdallMain *mainInstance = HeimdallMain.sharedInstance;
    b_mainWindowHasVideo = YES;
    return mainInstance.libraryWindow;
}

- (HeimdallVideoWindowCommon *)setupDetachedVideoWindow
{
    BOOL multipleVoutWindows = _voutWindows.count > 0;
    // setup detached window with controls
    NSWindowStyleMask mask = NSWindowStyleMaskClosable |
        NSWindowStyleMaskMiniaturizable |
        NSWindowStyleMaskResizable |
        NSWindowStyleMaskTitled |
        NSWindowStyleMaskFullSizeContentView;

    HeimdallAspectRatioRetainingVideoWindow *newVideoWindow = [[HeimdallAspectRatioRetainingVideoWindow alloc] initWithContentRect:NSMakeRect(0,0,300,300)
                                                                                                               styleMask:mask
                                                                                                                 backing:NSBackingStoreBuffered
                                                                                                                   defer:YES];

    newVideoWindow.backgroundColor = [NSColor blackColor];
    newVideoWindow.canBecomeKeyWindow = YES;
    newVideoWindow.canBecomeMainWindow = YES;
    newVideoWindow.acceptsMouseMovedEvents = YES;
    newVideoWindow.movableByWindowBackground = YES;
    newVideoWindow.minSize = NSMakeSize(HeimdallVideoWindowCommonMinimalHeight, HeimdallVideoWindowCommonMinimalHeight);
    newVideoWindow.titlebarAppearsTransparent = YES;

    newVideoWindow.videoViewController.displayLibraryControls = NO;
    newVideoWindow.videoViewController.view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    newVideoWindow.videoViewController.view.frame = newVideoWindow.contentView.frame;
    [newVideoWindow enableVideoTitleBarMode];

    [newVideoWindow.contentView addSubview:newVideoWindow.videoViewController.view positioned:NSWindowAbove relativeTo:nil];

    // no frame autosave for additional vout windows
    if (multipleVoutWindows) {
        newVideoWindow.frameAutosaveName = @"";
    }

    newVideoWindow.releasedWhenClosed = NO;
    newVideoWindow.delegate = newVideoWindow;
    newVideoWindow.level = NSNormalWindowLevel;
    [newVideoWindow center];
    return newVideoWindow;
}

- (HeimdallVideoWindowCommon *)setupVideoWindow
{
    BOOL isNativeFullscreen = var_InheritBool(getIntf(), "macosx-nativefullscreenmode");
    BOOL windowDecorations = var_InheritBool(getIntf(), "video-deco");
    BOOL videoWallpaper = var_InheritBool(getIntf(), "video-wallpaper");

    // TODO: make lion fullscreen compatible with video-wallpaper
    if ((videoWallpaper || !windowDecorations) && !isNativeFullscreen) {
        return [self borderlessVideoWindowAsVideoWallpaper:videoWallpaper withWindowDecorations:windowDecorations];
    }

    BOOL isEmbedded = var_InheritBool(getIntf(), "embedded-video") && !b_mainWindowHasVideo;
    if (isEmbedded) {
        return [self setupMainLibraryVideoWindow];
    }

    return [self setupDetachedVideoWindow];
}

- (void)setupWindowOriginForVideoWindow:(HeimdallVideoWindowCommon *)videoWindow
                             atPosition:(NSRect)videoViewPosition
{
    NSRect window_rect = [videoWindow frame];
    if (videoViewPosition.origin.x > 0.)
        window_rect.origin.x = videoViewPosition.origin.x;
    if (videoViewPosition.origin.y > 0.)
        window_rect.origin.y = videoViewPosition.origin.y;

    [videoWindow setFrame:window_rect display:YES];
}

- (void)cascadeVoutWindowsForVideoWindow:(HeimdallVideoWindowCommon *)videoWindow
{
    if (_voutWindows.count == 1) {
        NSWindow * firstWindow = [_voutWindows objectForKey:_voutWindows.allKeys.firstObject];

        NSRect topleftBaseRect = NSMakeRect(0, firstWindow.frame.size.height, 0, 0);
        _topLeftPoint = [firstWindow convertRectToScreen:topleftBaseRect].origin;
    }

    _topLeftPoint = [videoWindow cascadeTopLeftFromPoint:_topLeftPoint];
    [videoWindow setFrameTopLeftPoint:_topLeftPoint];
}

- (void)setupPositionAndSizeForVideoWindow:(HeimdallVideoWindowCommon *)videoWindow
                                atPosition:(NSRect)videoViewPosition
{
    BOOL isEmbedded = [videoWindow isKindOfClass:[HeimdallLibraryWindow class]];
    BOOL multipleVoutWindows = _voutWindows.count > 0;
    NSSize videoViewSize = NSMakeSize(videoViewPosition.size.width, videoViewPosition.size.height);

    // set (only!) window origin if specified
    if (!isEmbedded) {
        if ([videoWindow isKindOfClass:[HeimdallAspectRatioRetainingVideoWindow class]]) {
            [(HeimdallAspectRatioRetainingVideoWindow*)videoWindow setNativeVideoSize:videoViewSize];
        }

        [self setupWindowOriginForVideoWindow:videoWindow
                                   atPosition:videoViewPosition];
    }

    // cascade windows if we have more than one vout
    if (multipleVoutWindows) {
        [self cascadeVoutWindowsForVideoWindow:videoWindow];
    }

    [videoWindow makeKeyAndOrderFront: self];
}

- (void)setupVideoOutputForVideoWindow:(HeimdallVideoWindowCommon *)videoWindow
                         withHeimdallWindow:(heimdall_window_t *)p_wnd
{
    HeimdallVoutView * const voutView = videoWindow.videoViewController.voutView;

    videoWindow.alphaValue = config_GetFloat("macosx-opaqueness");
    [_voutWindows setObject:videoWindow forKey:[NSValue valueWithPointer:p_wnd]];
    vout_thread_t * const p_vout = (vout_thread_t *)heimdall_object_parent(p_wnd);
    var_AddCallback(p_vout, "video-on-top", WindowFloatOnTop, (__bridge void *)videoWindow);
    voutView.voutThread = p_vout;
    voutView.voutWindow = p_wnd;
    videoWindow.hasActiveVideo = YES;
    if (videoWindow.class == HeimdallLibraryWindow.class)
        [(HeimdallLibraryWindow *)videoWindow enableVideoPlaybackAppearance];
    _playerController.activeVideoPlayback = YES;
    HeimdallMain.sharedInstance.libraryWindow.nonembedded = !b_mainWindowHasVideo;
}

- (void)setupFullscreenStartIfNeededForVout:(HeimdallVoutView *)voutView
                              withHeimdallWindow:(heimdall_window_t *)p_wnd
{
    // TODO: find a cleaner way for "start in fullscreen"
    // Start in fs, because either prefs settings, or fullscreen button was pressed before
    /* detect the video-splitter and prevent starts in fullscreen if it is enabled */
    char *psz_splitter = var_GetString(voutView.voutThread, "video-splitter");
    BOOL b_have_splitter = psz_splitter != NULL && strcmp(psz_splitter, "none");
    free(psz_splitter);

    BOOL multipleVoutWindows = _voutWindows.count > 0;
    BOOL videoWallpaper = var_InheritBool(getIntf(), "video-wallpaper") && !multipleVoutWindows;

    if (!videoWallpaper && !b_have_splitter && (var_InheritBool(getIntf(), "fullscreen") || _playerController.fullscreen)) {
        // this is not set when we start in fullscreen because of
        // fullscreen settings in video prefs the second time
        [self setFullscreen:1 forWindow:p_wnd withAnimation:NO];
    }
}

- (HeimdallVoutView *)setupVoutForWindow:(heimdall_window_t *)p_wnd
      withProposedVideoViewPosition:(NSRect)videoViewPosition
{
    _playerController = HeimdallMain.sharedInstance.playQueueController.playerController;
    HeimdallVideoWindowCommon *newVideoWindow = [self setupVideoWindow];
    HeimdallVoutView *voutView = newVideoWindow.videoViewController.voutView;

    BOOL multipleVoutWindows = _voutWindows.count > 0;
    BOOL videoWallpaper = var_InheritBool(getIntf(), "video-wallpaper") && !multipleVoutWindows;

    // Avoid flashes if video will directly start in fullscreen
    [NSAnimationContext runAnimationRespectingPreferencesWithChanges:^(NSAnimationContext * const __unused context) {
        if (!videoWallpaper) {
            [self setupPositionAndSizeForVideoWindow:newVideoWindow atPosition:videoViewPosition];
        }
        [self setupVideoOutputForVideoWindow:newVideoWindow withHeimdallWindow:p_wnd];
        [self setupFullscreenStartIfNeededForVout:voutView withHeimdallWindow:p_wnd];
    } completionHandler:nil];

    return voutView;
}

- (void)removeVoutForDisplay:(NSValue *)key
{
    HeimdallVideoWindowCommon * const videoWindow = [_voutWindows objectForKey:key];
    if (!videoWindow) {
        msg_Err(getIntf(), "Cannot close nonexisting window");
        return;
    }

    heimdall_window_t * const p_wnd = (heimdall_window_t *)key.pointerValue;
    if (p_wnd) {
        vout_thread_t * const p_vout = (vout_thread_t *)heimdall_object_parent(p_wnd);
        if (p_vout) {
            var_DelCallback(p_vout, "video-on-top", WindowFloatOnTop, (__bridge void *)videoWindow);
        } else {
            msg_Warn(getIntf(),
                     "Could not get p_vout to unregister WindowFloatOnTop callback for window %p",
                     p_wnd);
        }
    } else {
        msg_Warn(getIntf(), "Could not get p_wnd from key to unregister WindowFloatOnTop callback");
    }

    [videoWindow.videoViewController.voutView releaseVoutThread];

    // set active video to no BEFORE closing the window and exiting fullscreen
    // (avoid stopping playback due to NSWindowWillCloseNotification, preserving fullscreen state)
    videoWindow.hasActiveVideo = NO;

    // do not close the window if we have the decorative view for audio visible
    // or if we have enabled end-of-playback screens.
    NSURL * const currentMediaUrl = _playerController.URLOfCurrentMediaItem;
    HeimdallMediaLibraryMediaItem * const mediaItem =
        [HeimdallMediaLibraryMediaItem mediaItemForURL:currentMediaUrl];
    const BOOL decorativeViewVisible =
        mediaItem != nil && mediaItem.mediaType == HEIMDALL_ML_MEDIA_TYPE_AUDIO;
    const BOOL endOfPlaybackScreenEnabled =
        [NSUserDefaults.standardUserDefaults boolForKey:HeimdallPlaybackEndViewEnabledKey];

    // we need to check that the player itself is in a stopped state. Removal of the active video
    // can be triggered by more than just end of playback (e.g. disabling the video track).
    if (!decorativeViewVisible
        && endOfPlaybackScreenEnabled
        && _playerController.playerState == HEIMDALL_PLAYER_STATE_STOPPED) {
        [videoWindow.videoViewController displayPlaybackEndView];
        videoWindow.videoViewController.endViewDismissHandler = ^{
            [self handleVideoCloseForDisplay:key];
        };
        if (videoWindow.class == HeimdallLibraryWindow.class) {
            b_mainWindowHasVideo = NO;
        }
    } else if (!decorativeViewVisible) {
        [self handleVideoCloseForDisplay:key];
    }
}

- (void)handleVideoCloseForDisplay:(NSValue *)key
{
    HeimdallVideoWindowCommon * const videoWindow = [_voutWindows objectForKey:key];
    if (!videoWindow) {
        msg_Err(getIntf(), "Cannot handle close for nonexistent window");
        return;
    }

    // prevent visible extra window if in fullscreen
    [NSAnimationContext runAnimationRespectingPreferencesWithChanges:^(NSAnimationContext * const __unused context) {
        const BOOL b_native = var_InheritBool(getIntf(), "macosx-nativefullscreenmode");

        // close fullscreen, without changing fullscreen vars
        if (!b_native && (videoWindow.fullscreen || videoWindow.inFullscreenTransition))
            [videoWindow leaveFullscreenWithAnimation:NO];

        // native fullscreen window will not be closed if
        // fullscreen was triggered without video
        if (b_native
            && videoWindow.class == HeimdallLibraryWindow.class
            && videoWindow.fullscreen
            && videoWindow.windowShouldExitFullscreenWhenFinished) {
            [videoWindow toggleFullScreen:self];
        }

        if (videoWindow.class == HeimdallLibraryWindow.class) {
            // If `embeddedVideoPlaybackActive` is already `NO`, the user navigated back
            // while playback was ongoing - `disableVideoPlaybackAppearance` was already
            // called at that point, so nothing more to do here.
        
            if (((HeimdallLibraryWindow *)videoWindow).embeddedVideoPlaybackActive) {
                [(HeimdallLibraryWindow *)videoWindow disableVideoPlaybackAppearance];
            }
        } else {
            [videoWindow close];
        }
    } completionHandler:nil];

    [_voutWindows removeObjectForKey:key];
    if (_voutWindows.count == 0) {
        _playerController.activeVideoPlayback = NO;
        _statusLevelWindowCounter = 0;
    }

    if ([videoWindow class] == [HeimdallLibraryWindow class]) {
        b_mainWindowHasVideo = NO;

        // video in main window might get stopped while another vout is open
        if (_voutWindows.count > 0)
            HeimdallMain.sharedInstance.libraryWindow.nonembedded = YES;
    }
}

- (void)setNativeVideoSize:(NSSize)size forWindow:(heimdall_window_t *)p_wnd
{
    HeimdallVideoWindowCommon *o_window = [_voutWindows objectForKey:[NSValue valueWithPointer:p_wnd]];
    if (!o_window) {
        msg_Err(getIntf(), "Cannot set size for nonexisting window");
        return;
    } else if (![o_window isKindOfClass:[HeimdallAspectRatioRetainingVideoWindow class]]) {
        return;
    }

    [(HeimdallAspectRatioRetainingVideoWindow*)o_window setNativeVideoSize:size];
}

- (void)setWindowLevel:(NSInteger)i_level forWindow:(heimdall_window_t *)p_wnd
{
    NSValue * const windowKey = [NSValue valueWithPointer:p_wnd];
    HeimdallVideoWindowCommon * const o_window = [_voutWindows objectForKey:windowKey];
    if (!o_window) {
        msg_Err(getIntf(), "Cannot set level for nonexisting window");
        return;
    }

    // only set level for helper windows to normal if no status vout window exist anymore
    if(i_level == NSStatusWindowLevel) {
        _statusLevelWindowCounter++;
        // window level need to stay on normal in fullscreen mode
        if (!o_window.fullscreen && !o_window.inFullscreenTransition) {
            [self updateWindowLevelForHelperWindows:i_level];

            // make sure float on top can join all spaces, including full-screen ones
            NSWindowCollectionBehavior behavior =
                NSWindowCollectionBehaviorCanJoinAllSpaces |
                NSWindowCollectionBehaviorIgnoresCycle |
                NSWindowCollectionBehaviorTransient |
                NSWindowCollectionBehaviorFullScreenAuxiliary;

            if (@available(macOS 13.0, *)) {
                behavior |= NSWindowCollectionBehaviorCanJoinAllApplications;
            } else {
                // Pre-Ventura: accessory policy is the only way to float over
                // other apps' full-screen Spaces. Dock tile disappears as a
                // side effect. #29250
                NSApp.activationPolicy = NSApplicationActivationPolicyAccessory;
            }
            o_window.collectionBehavior = behavior;
        }
    } else {
        if (_statusLevelWindowCounter > 0) {
            _statusLevelWindowCounter--;
        }
        if (_statusLevelWindowCounter == 0) {
            if (@available(macOS 13.0, *)) {
                // NO-OP. Intentionally empty as negating @available is unsupported in ObjC
            } else {
                NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
            }
            [self updateWindowLevelForHelperWindows:i_level];
        }
        o_window.collectionBehavior = NSWindowCollectionBehaviorDefault;
    }

    o_window.level = i_level;
}

- (void)setFullscreen:(int)i_full forWindow:(heimdall_window_t *)p_wnd withAnimation:(BOOL)b_animation
{
    intf_thread_t *p_intf = getIntf();
    BOOL b_nativeFullscreenMode = var_InheritBool(getIntf(), "macosx-nativefullscreenmode");
    BOOL b_fullscreen = i_full != 0;

    if (var_InheritBool(p_intf, "macosx-dim-keyboard")) {
        [_keyboardBacklight switchLightsAsync:!b_fullscreen];
    }

    if (!p_intf || (!b_nativeFullscreenMode && !p_wnd))
        return;

    if (!_playerController.fullscreen != !b_fullscreen) {
        _playerController.fullscreen = b_fullscreen;
    }

    HeimdallVideoWindowCommon *o_current_window = nil;
    if (p_wnd) {
        o_current_window = [_voutWindows objectForKey:[NSValue valueWithPointer:p_wnd]];
    }

    if (b_nativeFullscreenMode) {
        if(!o_current_window)
            o_current_window = HeimdallMain.sharedInstance.libraryWindow ;
        assert(o_current_window);

        // fullscreen might be triggered twice (vout event)
        // so ignore duplicate events here
        if((b_fullscreen && !([o_current_window fullscreen] || [o_current_window inFullscreenTransition])) ||
           (!b_fullscreen && [o_current_window fullscreen])) {

            dispatch_async(dispatch_get_main_queue(), ^{
                [o_current_window toggleFullScreen:self];
            });
        }
    } else {
        assert(o_current_window);

        if (b_fullscreen) {
            if (_playerController.playerState != HEIMDALL_PLAYER_STATE_STOPPED && [_playerController activeVideoPlayback]) {
                // activate app, as method can also be triggered from outside the app (prevents nasty window layout)
                [NSApp activateIgnoringOtherApps:YES];
                dispatch_async(dispatch_get_main_queue(), ^{
                    [o_current_window enterFullscreenWithAnimation:b_animation];
                });
            }
        } else {
            // leaving fullscreen is always allowed
            dispatch_async(dispatch_get_main_queue(), ^{
                [o_current_window leaveFullscreenWithAnimation:YES];
            });
        }
    }
}

#pragma mark -
#pragma mark Misc methods

- (void)updateWindowLevelForHelperWindows:(NSInteger)i_level
{
    if (var_InheritBool(getIntf(), "video-wallpaper"))
        return;

    _currentWindowLevel = i_level;
    if (i_level == NSNormalWindowLevel) {
        _currentStatusWindowLevel = NSFloatingWindowLevel;
    } else {
        _currentStatusWindowLevel = i_level + 1;
    }

    HeimdallMain *main = HeimdallMain.sharedInstance;
    [[main libraryWindow] setWindowLevel:i_level];

    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallWindowShouldUpdateLevel object:self userInfo:@{HeimdallWindowLevelKey : @(_currentWindowLevel)}];
}

- (HeimdallVideoWindowCommon *)videoWindowForVoutView:(HeimdallVoutView *)voutView
{
    return [self.voutWindows objectForKey:[NSValue valueWithPointer:voutView.voutWindow]];
}

#pragma mark -
#pragma mark Property methods

- (NSDictionary *)voutWindows
{
    return _voutWindows.copy;
}

@end
