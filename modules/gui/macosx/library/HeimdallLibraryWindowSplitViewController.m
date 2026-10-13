/*****************************************************************************
 * HeimdallLibraryWindowSplitViewManager.h: MacOS X interface module
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

#import "HeimdallLibraryWindowSplitViewController.h"

#import "library/HeimdallLibraryWindow.h"
#import "library/HeimdallLibraryWindowNavigationSidebarViewController.h"
#import "library/HeimdallLibraryWindowSidebarRootViewController.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallBottomBarView.h"
#import "views/HeimdallUIUnits.h"

#import "windows/controlsbar/HeimdallMainWindowControlsBar.h"

#import "windows/video/HeimdallMainVideoViewController.h"

@interface HeimdallLibraryWindowSplitViewController ()

@property (readwrite) BOOL priorNavSidebarCollapsedState;

@end

@implementation HeimdallLibraryWindowSplitViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

    HeimdallLibraryWindow * const libraryWindow = HeimdallMain.sharedInstance.libraryWindow;
    [libraryWindow addObserver:self
                    forKeyPath:HeimdallLibraryWindowEmbeddedVideoPlaybackActiveKey
                       options:0
                       context:nil];

    self.splitView.wantsLayer = YES;

    _navSidebarViewController =
        [[HeimdallLibraryWindowNavigationSidebarViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
    _libraryTargetViewController = [[NSViewController alloc] init];
    _multifunctionSidebarViewController =
        [[HeimdallLibraryWindowSidebarRootViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];

    self.libraryTargetViewController.view = self.libraryWindow.libraryTargetView;

    _navSidebarItem = [NSSplitViewItem sidebarWithViewController:self.navSidebarViewController];
    _libraryTargetViewItem = [NSSplitViewItem splitViewItemWithViewController:self.libraryTargetViewController];

    _multifunctionSidebarItem = [NSSplitViewItem splitViewItemWithViewController:self.multifunctionSidebarViewController];

    if (@available(macOS 11.0, *)) {
        _navSidebarItem.allowsFullHeightLayout = YES;
    }

    _navSidebarItem.preferredThicknessFraction = 0.2;
    _navSidebarItem.minimumThickness = HeimdallUIUnits.libraryWindowNavSidebarMinWidth;
    _navSidebarItem.maximumThickness = HeimdallUIUnits.libraryWindowNavSidebarMaxWidth;

    self.multifunctionSidebarItem.preferredThicknessFraction = 0.2;
    self.multifunctionSidebarItem.maximumThickness =
        HeimdallUIUnits.libraryWindowPlayQueueSidebarMaxWidth;
    self.multifunctionSidebarItem.canCollapse = YES;
    self.multifunctionSidebarItem.collapseBehavior =
        NSSplitViewItemCollapseBehaviorPreferResizingSiblingsWithFixedSplitView;

    self.splitViewItems = @[_navSidebarItem, _libraryTargetViewItem, self.multifunctionSidebarItem];

    HeimdallMainWindowControlsBar * const controlsBar = libraryWindow.controlsBar;
    HeimdallBottomBarView * const bottomBarView = controlsBar.bottomBarView;
    bottomBarView.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
        [bottomBarView.leadingAnchor constraintEqualToAnchor:self.libraryTargetViewController.view.leadingAnchor
                                                    constant:HeimdallUIUnits.largeSpacing * 2],
        [bottomBarView.trailingAnchor constraintEqualToAnchor:self.libraryTargetViewController.view.trailingAnchor
                                                     constant:-(HeimdallUIUnits.largeSpacing * 2)],
    ]];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
    if([keyPath isEqualToString:HeimdallLibraryWindowEmbeddedVideoPlaybackActiveKey]) {
        HeimdallLibraryWindow * const libraryWindow = HeimdallMain.sharedInstance.libraryWindow;
        const BOOL videoPlaybackActive = libraryWindow.embeddedVideoPlaybackActive;
        _navSidebarItem.collapsed = videoPlaybackActive;
    }
}

- (IBAction)toggleNavigationSidebar:(id)sender
{
    if (self.mainVideoModeEnabled) {
        return;
    }
    const BOOL navigationSidebarCollapsed = self.navSidebarItem.isCollapsed;
    self.navSidebarItem.animator.collapsed = !navigationSidebarCollapsed;
}

- (IBAction)toggleMultifunctionSidebar:(id)sender
{
    const BOOL sidebarCollapsed = self.multifunctionSidebarItem.isCollapsed;
    self.multifunctionSidebarItem.animator.collapsed = !sidebarCollapsed;

    const NSControlStateValue controlState =
        self.multifunctionSidebarItem.isCollapsed ? NSControlStateValueOff : NSControlStateValueOn;
    self.libraryWindow.playQueueToggle.state = controlState;
    self.libraryWindow.videoViewController.playQueueButton.state = controlState;
}

- (void)setMainVideoModeEnabled:(BOOL)mainVideoModeEnabled
{
    if (self.mainVideoModeEnabled == mainVideoModeEnabled) {
        return;
    } else if (mainVideoModeEnabled) {
        self.priorNavSidebarCollapsedState = self.navSidebarItem.isCollapsed;
        self.navSidebarItem.collapsed = YES;
    } else {
        self.navSidebarItem.collapsed = self.priorNavSidebarCollapsedState;
    }

    _mainVideoModeEnabled = mainVideoModeEnabled;
    self.multifunctionSidebarViewController.mainVideoModeEnabled = mainVideoModeEnabled;
}

- (BOOL)splitView:(NSSplitView *)splitView canCollapseSubview:(NSView *)subview
{
    return subview != self.navSidebarViewController.view
           || !self.mainVideoModeEnabled
           || [super splitView:splitView canCollapseSubview:subview];
}

@end
