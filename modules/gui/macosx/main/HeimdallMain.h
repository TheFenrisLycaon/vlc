/*****************************************************************************
 * HeimdallMain.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2002-2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Derk-Jan Hartman <hartman at lycaria dot org>
 *          Felix Paul Kühne <fkuehne at lycaria dot org>
 *          David Fuhrmann <david dot fuhrmann at googlemail dot com>
 *          Pierre d'Herbemont <pdherbemont # lycaria org>
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

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>

#import <heimdall_common.h>
#import <heimdall_interface.h>

/*****************************************************************************
 * Local prototypes.
 *****************************************************************************/

typedef struct heimdall_preparser_t heimdall_preparser_t;

intf_thread_t *getIntf(void);
heimdall_preparser_t *getNetworkPreparser();
heimdall_preparser_t *getThumbnailer();

extern NSString *HeimdallConfigurationChangedNotification;
extern NSString * const kHeimdallPreferencesVersion;

/*****************************************************************************
 * HeimdallMain interface
 *****************************************************************************/
@class HeimdallExtensionsManager;
@class HeimdallMainMenu;
@class HeimdallSimplePrefsController;
@class HeimdallPrefs;
@class HeimdallCoreDialogProvider;
@class HeimdallBookmarksWindowController;
@class HeimdallOpenWindowController;
@class HeimdallLogWindowController;
@class HeimdallLibraryWindow;
@class HeimdallLibraryWindowController;
@class HeimdallTrackSynchronizationWindowController;
@class HeimdallAudioEffectsWindowController;
@class HeimdallVideoEffectsWindowController;
@class HeimdallConvertAndSaveWindowController;
@class HeimdallLibraryController;
@class HeimdallPlayQueueController;
@class HeimdallVideoOutputProvider;
@class HeimdallDetachedAudioWindow;
#ifdef HAVE_SPARKLE
@class SPUStandardUpdaterController;
#endif

@interface HeimdallMain : NSObject

@property (class, readonly) HeimdallMain *sharedInstance;
+ (void)relaunchApplication;

@property (readonly) BOOL isTerminating;

@property (readonly) HeimdallAudioEffectsWindowController *audioEffectsPanel;
@property (readonly) HeimdallBookmarksWindowController *bookmarks;
@property (readonly) HeimdallConvertAndSaveWindowController *convertAndSaveWindow;
@property (readonly) HeimdallCoreDialogProvider *coreDialogProvider;
@property (readonly) HeimdallExtensionsManager *extensionsManager;
@property (readonly) HeimdallLogWindowController *debugMsgPanel;
@property (readonly) HeimdallLibraryController *libraryController;
@property (readonly) HeimdallLibraryWindow *libraryWindow;
@property (readwrite) HeimdallLibraryWindowController *libraryWindowController;
@property (readonly) HeimdallMainMenu *mainMenu;
@property (readonly) HeimdallOpenWindowController *open;
@property (readonly) HeimdallPlayQueueController *playQueueController;
@property (readonly) HeimdallPrefs *preferences;
@property (readonly) HeimdallSimplePrefsController *simplePreferences;
@property (readonly) HeimdallTrackSynchronizationWindowController *trackSyncPanel;
@property (readonly) HeimdallVideoEffectsWindowController *videoEffectsPanel;
@property (readonly) HeimdallVideoOutputProvider *voutProvider;
@property (readonly) HeimdallDetachedAudioWindow *detachedAudioWindow;
@property (readonly) id<MTLDevice> metalDevice;
@property (readonly) id<MTLLibrary> metalLibrary;
#ifdef HAVE_SPARKLE
@property (readonly) SPUStandardUpdaterController *sparkleUpdaterController;
#endif

- (int)processIsTranslated;

@end
