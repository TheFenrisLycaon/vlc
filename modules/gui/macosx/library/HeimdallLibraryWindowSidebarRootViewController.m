/*****************************************************************************
 * HeimdallLibraryWindowSidebarRootViewController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
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

#import "HeimdallLibraryWindowSidebarRootViewController.h"

#import "extensions/NSFont+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"
#import "extensions/NSTextField+HeimdallAdditions.h"
#import "extensions/NSView+HeimdallAdditions.h"
#import "extensions/NSWindow+HeimdallAdditions.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryWindow.h"
#import "library/HeimdallLibraryWindowChaptersSidebarViewController.h"
#import "library/HeimdallLibraryWindowPlayQueueSidebarViewController.h"
#import "library/HeimdallLibraryWindowSidebarChildViewController.h"
#import "library/HeimdallLibraryWindowTitlesSidebarViewController.h"

#import "main/HeimdallMain.h"

#import "playqueue/HeimdallPlayerController.h"
#import "playqueue/HeimdallPlayQueueController.h"

#import "views/HeimdallRoundedCornerTextField.h"
#import "views/HeimdallUIUnits.h"

@interface HeimdallLibraryWindowSidebarRootViewController ()

@property (readwrite) NSViewController<HeimdallLibraryWindowSidebarChildViewController> *currentChildVc;

@end

@implementation HeimdallLibraryWindowSidebarRootViewController

- (instancetype)initWithLibraryWindow:(HeimdallLibraryWindow *)libraryWindow
{
    self = [super initWithNibName:@"HeimdallLibraryWindowSidebarRootView" bundle:nil];
    if (self) {
        _libraryWindow = libraryWindow;
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self setupPlayQueueTitle];
    [self setupCounterLabel];

    self.topInternalConstraint.constant = [self topConstraintConstant];

    self.mainVideoModeEnabled = NO;

    _playQueueSidebarViewController =
        [[HeimdallLibraryWindowPlayQueueSidebarViewController alloc] initWithLibraryWindow:self.libraryWindow];
    _chaptersSidebarViewController =
        [[HeimdallLibraryWindowChaptersSidebarViewController alloc] initWithLibraryWindow:self.libraryWindow];
    _titlesSidebarViewController =
        [[HeimdallLibraryWindowTitlesSidebarViewController alloc] initWithLibraryWindow:self.libraryWindow];

    self.targetView.translatesAutoresizingMaskIntoConstraints = NO;

    self.viewSelector.segmentDistribution = NSSegmentDistributionFillEqually;

    [self setupViewSelectorSegments];
    [self updateViewSelectorState];
    [self viewSelectorAction:self.viewSelector];

    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(titleListChanged:)
                               name:HeimdallPlayerTitleListChanged
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(mediaItemChanged:)
                               name:HeimdallPlayerCurrentMediaItemChanged
                             object:nil];
}

- (void)setupPlayQueueTitle
{
    _playQueueHeaderLabel = [NSTextField defaultLabelWithString:@""];
    self.playQueueHeaderLabel.font = NSFont.HeimdallLibrarySubsectionHeaderFont;
    self.playQueueHeaderLabel.textColor = NSColor.headerTextColor;

    [self.view addSubview:self.playQueueHeaderLabel];

    _playQueueHeaderTopConstraint = 
        [self.playQueueHeaderLabel.topAnchor constraintEqualToAnchor:self.view.topAnchor
                                                            constant:HeimdallUIUnits.libraryWindowContentSafeTopInset + HeimdallUIUnits.smallSpacing];
    [NSLayoutConstraint activateConstraints:@[
        self.playQueueHeaderTopConstraint,
        [self.playQueueHeaderLabel.bottomAnchor constraintEqualToAnchor:self.targetView.topAnchor
                                                               constant:-HeimdallUIUnits.smallSpacing],
        [self.playQueueHeaderLabel.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor
                                                                constant:HeimdallUIUnits.largeSpacing] 
    ]];
}

- (void)setupViewSelectorSegments
{
    self.viewSelector.segmentCount = 1;
    [self.viewSelector setLabel:self.playQueueSidebarViewController.title
                     forSegment:0];

    HeimdallPlayQueueController * const playQueueController = HeimdallMain.sharedInstance.playQueueController;
    HeimdallPlayerController * const playerController = playQueueController.playerController;

    if (playerController.numberOfTitlesOfCurrentMedia > 0) {
        self.viewSelector.segmentCount++; 
        [self.viewSelector setLabel:self.titlesSidebarViewController.title
                         forSegment:self.viewSelector.segmentCount - 1];
    }
    if (playerController.numberOfChaptersForCurrentTitle > 0) {
        self.viewSelector.segmentCount++;
        [self.viewSelector setLabel:self.chaptersSidebarViewController.title
                         forSegment:self.viewSelector.segmentCount - 1];
    }
}

- (void)setupCounterLabel
{
    _counterLabel = [[HeimdallRoundedCornerTextField alloc] init];
    self.counterLabel.useStrongRounding = YES;
    self.counterLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [self.counterLabel setContentCompressionResistancePriority:NSLayoutPriorityRequired
                                                 forOrientation:NSLayoutConstraintOrientationHorizontal];
    [self.counterLabel setContentCompressionResistancePriority:NSLayoutPriorityRequired
                                                 forOrientation:NSLayoutConstraintOrientationVertical];

    [self.view addSubview:self.counterLabel];

    _counterLabelInHeaderConstraint = 
        [self.counterLabel.centerYAnchor constraintEqualToAnchor:self.playQueueHeaderLabel.centerYAnchor];
    _counterLabelInChildViewConstraint =
        [self.counterLabel.topAnchor constraintEqualToAnchor:self.targetView.topAnchor
                                                    constant:HeimdallUIUnits.smallSpacing];
    
    [NSLayoutConstraint activateConstraints:@[
        self.counterLabelInHeaderConstraint,
        [self.counterLabel.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor
                                                         constant:-HeimdallUIUnits.largeSpacing]
    ]];
}

- (void)titleListChanged:(NSNotification *)notification
{
    [self updateViewSelectorState];
}

- (void)mediaItemChanged:(NSNotification *)notification
{
    [self updateViewSelectorState];
}

- (void)updateViewSelectorState
{
    [self setupViewSelectorSegments];

    const BOOL showSelector = self.viewSelector.segmentCount > 1;
    self.viewSelector.hidden = !showSelector;
    self.topInternalConstraint.active = showSelector;

    self.playQueueHeaderLabel.hidden = showSelector;
    const NSLayoutPriority playQueueCompressionPriority =
        showSelector ? NSLayoutPriorityRequired : NSLayoutPriorityDefaultHigh;
    [self.playQueueHeaderLabel setContentCompressionResistancePriority:playQueueCompressionPriority
                                                        forOrientation:NSLayoutConstraintOrientationVertical];
    self.playQueueHeaderTopConstraint.active = !showSelector;

    if (showSelector) {
        self.counterLabelInHeaderConstraint.active = NO;
        self.counterLabelInChildViewConstraint.active = YES;
    } else {
        self.counterLabelInHeaderConstraint.active = YES;
        self.counterLabelInChildViewConstraint.active = NO;
    }

    NSString * const selectedSegmentLabel = [self.viewSelector labelForSegment:self.viewSelector.selectedSegment];
    BOOL selectedSegmentValid = NO;
    for (NSInteger i = 0; i < self.viewSelector.segmentCount; i++) {
        if ([[self.viewSelector labelForSegment:i] isEqualToString:selectedSegmentLabel]) {
            selectedSegmentValid = YES;
            break;
        }
    }

    if (!selectedSegmentValid) {
        self.viewSelector.selectedSegment = 0;
        [self viewSelectorAction:self.viewSelector];
    }
}

- (IBAction)viewSelectorAction:(id)sender
{
    NSParameterAssert(sender == self.viewSelector);
    const NSInteger selectedSegment = self.viewSelector.selectedSegment;
    NSString * const selectedSegmentLabel = [self.viewSelector labelForSegment:selectedSegment];
    if ([selectedSegmentLabel isEqualToString:self.playQueueSidebarViewController.title]) {
        [self setChildViewController:self.playQueueSidebarViewController];
    } else if ([selectedSegmentLabel isEqualToString:self.titlesSidebarViewController.title]) {
        [self setChildViewController:self.titlesSidebarViewController];
    } else if ([selectedSegmentLabel isEqualToString:self.chaptersSidebarViewController.title]) {
        [self setChildViewController:self.chaptersSidebarViewController];
    } else {
        NSAssert(NO, @"Invalid or unknown segment selected for sidebar!");
    }
}

- (void)setChildViewController:(NSViewController<HeimdallLibraryWindowSidebarChildViewController> *)viewController
{
    self.currentChildVc.counterLabel = nil; // Stop old vc manipulating the counter label
    self.currentChildVc = viewController;
    
    self.counterLabel.hidden = !viewController.supportsItemCount;
    if (viewController.supportsItemCount) {
        viewController.counterLabel = self.counterLabel;
    }

    self.playQueueHeaderLabel.stringValue = viewController.title;

    NSView * const view = viewController.view;
    self.targetView.subviews = @[view];
    view.translatesAutoresizingMaskIntoConstraints = NO;
    [view applyConstraintsToFillSuperview];
}

- (void)setMainVideoModeEnabled:(BOOL)mainVideoModeEnabled
{
    _mainVideoModeEnabled = mainVideoModeEnabled;
    [self updateTopConstraints];
}

- (CGFloat)topConstraintConstant
{
    CGFloat constant = HeimdallUIUnits.smallSpacing;
    if (@available(macOS 11.0, *)) {
        // Safe area layout guide already accounts for titlebar height
        if (!self.mainVideoModeEnabled) {
            constant += HeimdallUIUnits.libraryWindowContentSafeTopInset;
        }
    } else if (!self.mainVideoModeEnabled && self.libraryWindow.styleMask & NSWindowStyleMaskFullSizeContentView) {
        // Compensate for full content view window's titlebar height, prevent top being cut off
        constant += self.libraryWindow.titlebarHeight;
    }
    return constant;
}

- (void)updateTopConstraints
{
    const CGFloat internalTopConstraintConstant = [self topConstraintConstant];
    self.topInternalConstraint.constant = internalTopConstraintConstant;
    self.playQueueHeaderTopConstraint.constant = internalTopConstraintConstant;
}

@end
