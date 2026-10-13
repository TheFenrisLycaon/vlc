/*****************************************************************************
 * HeimdallLibraryWindowToolbarDelegate.h: MacOS X interface module
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

#import <Cocoa/Cocoa.h>

#import "library/HeimdallLibrarySegment.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_OPTIONS(NSUInteger, HeimdallLibraryWindowToolbarDisplayFlags) {
    HeimdallLibraryWindowToolbarDisplayFlagNavigationButtons = 1 << 0,
    HeimdallLibraryWindowToolbarDisplayFlagToggleViewModeSegmentButton = 1 << 1,
    HeimdallLibraryWindowToolbarDisplayFlagSortOrderButton = 1 << 2,
    HeimdallLibraryWindowToolbarDisplayFlagLibrarySearchBar = 1 << 3
};

extern NSString * const HeimdallLibraryWindowTrackingSeparatorToolbarItemIdentifier;

@class HeimdallLibraryWindow;

@interface HeimdallLibraryWindowToolbarDelegate : NSObject<NSToolbarDelegate>

@property (readwrite, weak) IBOutlet HeimdallLibraryWindow *libraryWindow;
@property (readwrite, weak) IBOutlet NSToolbar *toolbar;

@property (readonly, strong) NSToolbarItem *trackingSeparatorToolbarItem;

@property (readwrite, weak) IBOutlet NSToolbarItem *heimdallIconToolbarItem;
@property (readwrite, weak) IBOutlet NSToolbarItem *toggleNavSidebarToolbarItem;
@property (readwrite, weak) IBOutlet NSToolbarItem *backwardsToolbarItem;
@property (readwrite, weak) IBOutlet NSToolbarItem *forwardsToolbarItem;
@property (readwrite, weak) IBOutlet NSToolbarItem *libraryViewModeToolbarItem;
@property (readwrite, weak) IBOutlet NSToolbarItem *sortOrderToolbarItem;
@property (readwrite, weak) IBOutlet NSToolbarItem *flexibleSpaceToolbarItem;
@property (readwrite, weak) IBOutlet NSToolbarItem *librarySearchToolbarItem;
@property (readwrite, weak) IBOutlet NSToolbarItem *togglePlayQueueToolbarItem;
@property (readwrite, weak) IBOutlet NSToolbarItem *renderersToolbarItem;

- (IBAction)rendererControlAction:(id)sender;
- (void)applyVisiblityFlags:(HeimdallLibraryWindowToolbarDisplayFlags)flags;

@end

NS_ASSUME_NONNULL_END
