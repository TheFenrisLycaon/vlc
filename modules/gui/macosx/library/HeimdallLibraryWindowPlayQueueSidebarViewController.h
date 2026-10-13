/*****************************************************************************
 * HeimdallLibraryWindowPlayQueueSidebarViewController.h: MacOS X interface module
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

#include <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>

#import "HeimdallLibraryWindowAbstractSidebarViewController.h"

NS_ASSUME_NONNULL_BEGIN

@class HeimdallDragDropView;
@class HeimdallPlayQueueController;
@class HeimdallPlayQueueDataSource;
@class HeimdallPlayQueueSortingMenuController;
@class HeimdallRoundedCornerTextField;

@interface HeimdallLibraryWindowPlayQueueSidebarViewController : HeimdallLibraryWindowAbstractSidebarViewController

@property (readwrite, weak) IBOutlet NSTableView *tableView;
@property (readwrite, weak) IBOutlet NSView *footerContainerView;
@property (readwrite, weak) IBOutlet HeimdallDragDropView *dragDropView;
@property (readwrite, weak) IBOutlet NSButton *openMediaButton;
@property (readwrite, weak) IBOutlet NSBox *bottomButtonsSeparator;
@property (readwrite, weak) IBOutlet NSButton *repeatButton;
@property (readwrite, weak) IBOutlet NSButton *shuffleButton;
@property (readwrite, weak) IBOutlet NSButton *sortButton;
@property (readwrite, weak) IBOutlet NSButton *clearButton;
@property (readwrite, weak) IBOutlet NSStackView *buttonStack;
@property (readwrite, weak) IBOutlet NSScrollView *scrollView;
@property (readwrite, weak) IBOutlet NSLayoutConstraint *footerContainerViewDefaultBottomConstraint;
@property (readwrite, weak) IBOutlet NSLayoutConstraint *footerContainerViewLeadingConstraint;
@property (readwrite, weak) IBOutlet NSLayoutConstraint *footerContainerViewTrailingConstraint;
@property (readwrite, weak) IBOutlet NSLayoutConstraint *scrollViewDefaultBottomConstraint;

@property (readwrite, nonatomic) NSTextField *counterLabel;

@property (readonly) HeimdallPlayQueueController *playQueueController;
@property (readonly) HeimdallPlayQueueDataSource *dataSource;
@property (readonly) HeimdallPlayQueueSortingMenuController *sortingMenuController;

- (instancetype)initWithLibraryWindow:(HeimdallLibraryWindow *)libraryWindow;

- (IBAction)tableDoubleClickAction:(id)sender;
- (IBAction)openMedia:(id)sender;
- (IBAction)shuffleAction:(id)sender;
- (IBAction)repeatAction:(id)sender;
- (IBAction)sortPlayQueue:(id)sender;
- (IBAction)clearPlayQueue:(id)sender;

@end

NS_ASSUME_NONNULL_END
