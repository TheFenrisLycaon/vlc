/*****************************************************************************
 * HeimdallLibraryWindowController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
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

#import "HeimdallLibraryWindowController.h"

#import "library/HeimdallLibrarySegment.h"
#import "library/HeimdallLibraryWindow.h"
#import "library/HeimdallLibraryWindowNavigationSidebarViewController.h"
#import "library/HeimdallLibraryWindowSplitViewController.h"

#import "library/audio-library/HeimdallLibraryAudioViewController.h"

#import "main/HeimdallMain.h"

@implementation HeimdallLibraryWindowController

- (instancetype)initWithLibraryWindow
{
    self = [super initWithWindowNibName:@"HeimdallLibraryWindow"];
    return self;
}

- (void)windowDidLoad
{
    HeimdallLibraryWindow *window = (HeimdallLibraryWindow *)self.window;
    [window setRestorationClass:[self class]];
    [window setExcludedFromWindowsMenu:YES];
    [window setAcceptsMouseMovedEvents:YES];
    [window setContentMinSize:NSMakeSize(HeimdallLibraryWindowMinimalWidth, HeimdallLibraryWindowMinimalHeight)];
}

+ (void)restoreWindowWithIdentifier:(NSUserInterfaceItemIdentifier)identifier
                              state:(NSCoder *)state
                  completionHandler:(void (^)(NSWindow *, NSError *))completionHandler
{
    if([identifier isEqualToString:HeimdallLibraryWindowIdentifier] == NO) {
        return;
    }

    if(HeimdallMain.sharedInstance.libraryWindowController == nil) {
        HeimdallMain.sharedInstance.libraryWindowController = [[HeimdallLibraryWindowController alloc] initWithLibraryWindow];
    }

    HeimdallLibraryWindow * const libraryWindow = HeimdallMain.sharedInstance.libraryWindow;

    const NSInteger selectedSegment = [state decodeIntegerForKey:@"macosx-library-selected-segment"];
    [libraryWindow.splitViewController.navSidebarViewController selectSegment:selectedSegment];

    completionHandler(libraryWindow, nil);
}

@end
