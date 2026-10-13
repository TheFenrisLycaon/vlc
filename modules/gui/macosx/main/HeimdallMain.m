/*****************************************************************************
 * HeimdallMain.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2002-2020 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Derk-Jan Hartman <hartman at videolan.org>
 *          Felix Paul Kühne <fkuehne at lycaria dot org>
 *          Pierre d'Herbemont <pdherbemont # lycaria org>
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#import "main/HeimdallMain.h"

#include <stdlib.h>                                      /* malloc(), free() */
#include <string.h>
#include <stdatomic.h>
#include <sys/sysctl.h>

#include <heimdall_common.h>
#include <heimdall_actions.h>
#include <heimdall_dialog.h>
#include <heimdall_url.h>
#include <heimdall_variables.h>
#include <heimdall_preparser.h>

#include <Metal/Metal.h>

#import "extensions/NSString+Helpers.h"

#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryWindow.h"
#import "library/HeimdallLibraryWindowController.h"

#import "main/CompatibilityFixes.h"
#import "main/HeimdallMain+OldPrefs.h"
#import "main/HeimdallApplication.h"

#import "menus/HeimdallMainMenu.h"
#import "menus/HeimdallStatusBarIcon.h"

#import "os-integration/HeimdallClickerManager.h"

#import "panels/dialogs/HeimdallCoreDialogProvider.h"
#import "panels/HeimdallAudioEffectsWindowController.h"
#import "panels/HeimdallBookmarksWindowController.h"
#import "panels/HeimdallVideoEffectsWindowController.h"
#import "panels/HeimdallTrackSynchronizationWindowController.h"

#import "playqueue/HeimdallPlayQueueController.h"
#import "playqueue/HeimdallPlayerController.h"
#import "playqueue/HeimdallPlayQueueModel.h"
#import "playqueue/HeimdallPlayQueueTableCellView.h"
#import "playqueue/HeimdallPlaybackContinuityController.h"

#import "preferences/prefs.h"
#import "preferences/HeimdallSimplePrefsController.h"

#import "views/HeimdallPlaybackEndViewController.h"

#import "windows/HeimdallDetachedAudioWindow.h"
#import "windows/HeimdallOpenWindowController.h"
#import "windows/HeimdallOpenInputMetadata.h"
#import "windows/convertandsave/HeimdallConvertAndSaveWindowController.h"
#import "windows/extensions/HeimdallExtensionsManager.h"
#import "windows/logging/HeimdallLogWindowController.h"
#import "windows/video/HeimdallVoutView.h"
#import "windows/video/HeimdallVideoOutputProvider.h"
#import "windows/video/HeimdallMainVideoViewController.h"

#ifdef HAVE_SPARKLE
#import "main/HeimdallMain+Sparkle.h"
#endif

NSString *HeimdallConfigurationChangedNotification = @"HeimdallConfigurationChangedNotification";

NSString * const kHeimdallPreferencesVersion = @"HeimdallPreferencesVersion";

#pragma mark -
#pragma mark Private extension

@interface HeimdallMain ()
<NSApplicationDelegate>
{
    intf_thread_t *_p_intf;
    BOOL _launched;

    HeimdallMainMenu *_mainmenu;
    HeimdallPrefs *_prefs;
    HeimdallSimplePrefsController *_sprefs;
    HeimdallOpenWindowController *_open;
    HeimdallCoreDialogProvider *_coredialogs;
    HeimdallBookmarksWindowController *_bookmarks;
    HeimdallPlaybackContinuityController *_continuityController;
    HeimdallLogWindowController *_messagePanelController;
    HeimdallStatusBarIcon *_statusBarIcon;
    HeimdallTrackSynchronizationWindowController *_trackSyncPanel;
    HeimdallAudioEffectsWindowController *_audioEffectsPanel;
    HeimdallVideoEffectsWindowController *_videoEffectsPanel;
    HeimdallConvertAndSaveWindowController *_convertAndSaveWindow;
    HeimdallClickerManager *_clickerManager;
    HeimdallDetachedAudioWindow *_detachedAudioWindow;
#ifdef HAVE_SPARKLE
    SPUStandardUpdaterController *_sparkleUpdaterController;
#endif

    bool _interfaceIsTerminating; /* Makes sure applicationWillTerminate will be called only once */
}
+ (void)killInstance;
- (void)applicationWillTerminate:(NSNotification *)notification;
#ifdef HAVE_SPARKLE
@property (readwrite) SPUStandardUpdaterController *sparkleUpdaterController;
#endif

@end

#pragma mark -
#pragma mark Heimdall Interface Object Callbacks

/*****************************************************************************
 * OpenIntf: initialize interface
 *****************************************************************************/

static intf_thread_t *p_interface_thread;
static heimdall_preparser_t *p_network_preparser;
static heimdall_preparser_t *p_thumbnailer;
heimdall_sem_t g_wait_quit;

intf_thread_t *getIntf()
{
    return p_interface_thread;
}

heimdall_preparser_t *getNetworkPreparser()
{
    return p_network_preparser;
}

heimdall_preparser_t *getThumbnailer()
{
    return p_thumbnailer;
}

int OpenIntf (heimdall_object_t *p_this)
{
    __block int retcode = HEIMDALL_SUCCESS;
    __block dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopDefaultMode, ^{
        @autoreleasepool {
            intf_thread_t *p_intf = (intf_thread_t*) p_this;
            p_interface_thread = p_intf;

            const struct heimdall_preparser_cfg cfg = {
                .types = HEIMDALL_PREPARSER_TYPE_PARSE,
                .max_parser_threads = 1,
                .timeout = 0,
                .external_process = false,
            };
            p_network_preparser = heimdall_preparser_New(p_this, &cfg);
            if (p_network_preparser == nil)
            {
                retcode = HEIMDALL_ENOMEM;
                dispatch_semaphore_signal(sem);
                return;
            }

            const struct heimdall_preparser_cfg thumbnailer_cfg = {
                .types = HEIMDALL_PREPARSER_TYPE_THUMBNAIL_TO_FILES,
                .max_thumbnailer_threads = 1,
                .timeout = HEIMDALL_TICK_FROM_SEC(3),
                .external_process = false,
            };
            p_thumbnailer = heimdall_preparser_New(p_this, &thumbnailer_cfg);
            if (p_thumbnailer == NULL) {
                msg_Warn(p_intf, "Could not initialize the thumbnailer");
            }
            msg_Dbg(p_intf, "Starting macosx interface");

            @try {
                HeimdallApplication * const application = HeimdallApplication.sharedApplication;
                NSCAssert(application != nil, @"HeimdallApplication must not be nil");
                [application setIntf:p_intf];

                HeimdallMain * const main = HeimdallMain.sharedInstance;
                NSCAssert(main != nil, @"HeimdallMain must not be nil");

                msg_Dbg(p_intf, "Finished loading macosx interface");
            } @catch (NSException *exception) {
                msg_Err(p_intf, "Loading the macosx interface failed. Do you have a valid window server?");
                retcode = HEIMDALL_EGENERIC;
                dispatch_semaphore_signal(sem);
                return;
            }
            dispatch_semaphore_signal(sem);
            [NSApp run];
        }
        heimdall_sem_post(&g_wait_quit);
    });
    CFRunLoopWakeUp(CFRunLoopGetMain());

    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);

    return retcode;
}

void CloseIntf (heimdall_object_t *p_this)
{
    void (^release_intf)() = ^{
        @autoreleasepool {
            msg_Dbg(p_this, "Closing macosx interface");
            [HeimdallMain.sharedInstance applicationWillTerminate:nil];
            if (p_thumbnailer != NULL) {
                heimdall_preparser_Cancel(p_thumbnailer, NULL);
                heimdall_preparser_Delete(p_thumbnailer);
                p_thumbnailer = NULL;
            }
            [HeimdallMain killInstance];
        }
        heimdall_preparser_Delete(p_network_preparser);
        [NSApp stop:nil];
        NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
            location:NSMakePoint(0,0)
            modifierFlags:0
            timestamp:0.0
            windowNumber:0
            context:nil
            subtype:0
            data1:0
            data2:0];
        [NSApp postEvent:event atStart:YES];
    };
    if (CFRunLoopGetCurrent() == CFRunLoopGetMain())
        release_intf();
    else
        dispatch_sync(dispatch_get_main_queue(), release_intf);
    heimdall_sem_wait(&g_wait_quit);
}

/*****************************************************************************
 * HeimdallMain implementation
 *****************************************************************************/
@implementation HeimdallMain

#pragma mark -
#pragma mark Initialization

static HeimdallMain *sharedInstance = nil;

+ (HeimdallMain *)sharedInstance
{
    static dispatch_once_t pred;
    dispatch_once(&pred, ^{
        sharedInstance = [[HeimdallMain alloc] init];
    });

    return sharedInstance;
}

+ (void)killInstance
{
    sharedInstance = nil;
}

+ (void)relaunchApplication
{
    const char *path = [[[NSBundle mainBundle] executablePath] UTF8String];

    /* For some reason we need to fork(), not just execl(), which reports a ENOTSUP then. */
    if (fork() != 0) {
        exit(0);
    }
    execl(path, path, (char *)NULL);
}

- (id)init
{
    self = [super init];
    if (self) {
        _p_intf = getIntf();

        HeimdallApplication.sharedApplication.delegate = self;

        _playQueueController = [[HeimdallPlayQueueController alloc] initWithPlaylist:heimdall_intf_GetMainPlaylist(_p_intf)];
        _libraryController = [[HeimdallLibraryController alloc] init];
        _continuityController = [[HeimdallPlaybackContinuityController alloc] init];

        // first initialize extensions dialog provider, then core dialog
        // provider which will register both at the core
        _extensionsManager = [[HeimdallExtensionsManager alloc] init];
        _coredialogs = [[HeimdallCoreDialogProvider alloc] init];

        _mainmenu = [[HeimdallMainMenu alloc] init];
        _voutProvider = [[HeimdallVideoOutputProvider alloc] init];

        // Load them here already to apply stored profiles
        _videoEffectsPanel = [[HeimdallVideoEffectsWindowController alloc] init];
        _audioEffectsPanel = [[HeimdallAudioEffectsWindowController alloc] init];

        if ([NSApp currentSystemPresentationOptions] & NSApplicationPresentationFullScreen)
            [_playQueueController.playerController setFullscreen:YES];
    }

    return self;
}

- (void)applicationWillFinishLaunching:(NSNotification *)aNotification
{
    // Only Metal shader in use at the moment is for holiday theming, so only load during this time.
    // Change this if you are going to add new shaders!
#ifdef ENABLE_MACOSX_UI_SHADERS
    if (((HeimdallApplication *)NSApplication.sharedApplication).winterHolidaysTheming) {
        _metalDevice = MTLCreateSystemDefaultDevice();
        NSString * const libraryPath =
            [NSBundle.mainBundle pathForResource:@"Shaders" ofType:@"metallib"];
        if (libraryPath) {
            NSError *error = nil;
            _metalLibrary = [_metalDevice newLibraryWithFile:libraryPath error:&error];
            if (!_metalLibrary) {
                NSLog(@"Error creating Metal library: %@", error);
            }
        } else {
            NSLog(@"Error: Could not find Shaders.metallib in the bundle.");
        }
    }
#endif

    _clickerManager = [[HeimdallClickerManager alloc] init];

#ifdef HAVE_SPARKLE
    [self setupSparkle];
#endif

    [[NSBundle mainBundle] loadNibNamed:@"MainMenu" owner:_mainmenu topLevelObjects:nil];

    NSImage *appIconImage = [HeimdallApplication.sharedApplication heimdallAppIconImage];
    [HeimdallApplication.sharedApplication
        setApplicationIconImage:appIconImage];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    _launched = YES;

    if (_libraryWindowController == nil) {
        _libraryWindowController = [[HeimdallLibraryWindowController alloc] initWithLibraryWindow];
    }

    [_libraryWindowController.window makeKeyAndOrderFront:nil];

    if (!_p_intf)
        return;

    NSUserDefaults * const defaults = NSUserDefaults.standardUserDefaults;
    if ([defaults integerForKey:kHeimdallPreferencesVersion] != 4) {
        [defaults setBool:YES forKey:HeimdallPlaybackEndViewEnabledKey];
        [defaults setBool:NO forKey:HeimdallDisplayTrackNumberPlayQueueKey];
        [defaults setBool:YES forKey:HeimdallUseClassicVideoPlayerLayoutKey];
        [self migrateOldPreferences];
    }

    _statusBarIcon = [[HeimdallStatusBarIcon alloc] init:_p_intf];

    /* on macOS 11 and later, check whether the user attempts to deploy
     * the x86_64 binary on ARM-64 - if yes, log it */
    if (OSX_BIGSUR_AND_HIGHER) {
        if ([self processIsTranslated] > 0) {
            msg_Warn(getIntf(), "Process is translated!");
        }
    }
}

- (int)processIsTranslated
{
   int ret = 0;
   size_t size = sizeof(ret);
   if (sysctlbyname("sysctl.proc_translated", &ret, &size, NULL, 0) == -1) {
      if (errno == ENOENT)
         return 0;
      return -1;
   }
   return ret;
}

#pragma mark -
#pragma mark Termination

- (BOOL)isTerminating
{
    return _interfaceIsTerminating;
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
    if (_interfaceIsTerminating)
        return;
    _interfaceIsTerminating = true;

    NSNotificationCenter *notiticationCenter = NSNotificationCenter.defaultCenter;
    if (notification == nil) {
        [notiticationCenter postNotificationName: NSApplicationWillTerminateNotification object: nil];
    }
    [notiticationCenter removeObserver: self];

    // closes all open vouts
    _voutProvider = nil;
    _continuityController = nil;

    /* write cached user defaults to disk */
    CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
}

#pragma mark -
#pragma mark File opening over dock icon

- (void)application:(NSApplication *)o_app openFiles:(NSArray *)o_names
{
    // Only add items here which are getting dropped to to the application icon
    // or are given at startup. If a file is passed via command line, libheimdallcore
    // will add the item, but cocoa also calls this function. In this case, the
    // invocation is ignored here.
    NSArray *resultItems = o_names;
    if (_launched == NO) {
        NSArray *launchArgs = [[NSProcessInfo processInfo] arguments];

        if (launchArgs) {
            NSSet *launchArgsSet = [NSSet setWithArray:launchArgs];
            NSMutableSet *itemSet = [NSMutableSet setWithArray:o_names];
            [itemSet minusSet:launchArgsSet];
            resultItems = [itemSet allObjects];
        }
    }

    NSArray *o_sorted_names = [resultItems sortedArrayUsingSelector: @selector(caseInsensitiveCompare:)];
    NSMutableArray *o_result = [NSMutableArray arrayWithCapacity: [o_sorted_names count]];
    for (NSString *filepath in o_sorted_names) {
        HeimdallOpenInputMetadata *inputMetadata;

        inputMetadata = [HeimdallOpenInputMetadata inputMetaWithPath:filepath];
        if (!inputMetadata)
            continue;

        [o_result addObject:inputMetadata];
    }

    [_playQueueController addPlayQueueItems:o_result];
}

/* When user click in the Dock icon our double click in the finder */
- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)hasVisibleWindows
{
    if (!hasVisibleWindows)
        [[self libraryWindow] makeKeyAndOrderFront:self];

    return YES;
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app
{
    return YES;
}

#pragma mark -
#pragma mark Other objects getters

- (HeimdallMainMenu *)mainMenu
{
    return _mainmenu;
}

- (HeimdallLibraryWindow *)libraryWindow
{
    return (HeimdallLibraryWindow *)_libraryWindowController.window;
}

- (HeimdallLogWindowController *)debugMsgPanel
{
    if (!_messagePanelController)
        _messagePanelController = [[HeimdallLogWindowController alloc] init];

    return _messagePanelController;
}

- (HeimdallTrackSynchronizationWindowController *)trackSyncPanel
{
    if (!_trackSyncPanel)
        _trackSyncPanel = [[HeimdallTrackSynchronizationWindowController alloc] init];

    return _trackSyncPanel;
}

- (HeimdallAudioEffectsWindowController *)audioEffectsPanel
{
    return _audioEffectsPanel;
}

- (HeimdallVideoEffectsWindowController *)videoEffectsPanel
{
    return _videoEffectsPanel;
}

- (HeimdallBookmarksWindowController *)bookmarks
{
    if (!self.libraryController.libraryModel) {
        return nil;
    }

    if (!_bookmarks)
        _bookmarks = [[HeimdallBookmarksWindowController alloc] init];

    return _bookmarks;
}

- (HeimdallOpenWindowController *)open
{
    if (!_open)
        _open = [[HeimdallOpenWindowController alloc] init];

    return _open;
}

- (HeimdallConvertAndSaveWindowController *)convertAndSaveWindow
{
    if (_convertAndSaveWindow == nil)
        _convertAndSaveWindow = [[HeimdallConvertAndSaveWindowController alloc] init];

    return _convertAndSaveWindow;
}

- (HeimdallSimplePrefsController *)simplePreferences
{
    if (!_sprefs)
        _sprefs = [[HeimdallSimplePrefsController alloc] init];

    return _sprefs;
}

- (HeimdallPrefs *)preferences
{
    if (!_prefs)
        _prefs = [[HeimdallPrefs alloc] init];

    return _prefs;
}

- (HeimdallCoreDialogProvider *)coreDialogProvider
{
    return _coredialogs;
}

- (HeimdallDetachedAudioWindow *)detachedAudioWindow
{
    if (_detachedAudioWindow == nil) {
        NSWindowController * const windowController = [[NSWindowController alloc] initWithWindowNibName:NSStringFromClass(HeimdallDetachedAudioWindow.class)];
        [windowController loadWindow];
        _detachedAudioWindow = (HeimdallDetachedAudioWindow *)windowController.window;
    }

    return _detachedAudioWindow;
}

@end
