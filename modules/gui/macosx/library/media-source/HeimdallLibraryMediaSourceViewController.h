/*****************************************************************************
 * HeimdallLibraryMediaSourceViewController.h: MacOS X interface module
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

#import "library/HeimdallLibraryAbstractSegmentViewController.h"

NS_ASSUME_NONNULL_BEGIN

@class HeimdallInputNodePathControl;
@class HeimdallLibraryCollectionView;
@class HeimdallLibraryMediaSourceViewNavigationStack;
@class HeimdallLibraryWindow;
@class HeimdallMediaSourceBaseDataSource;

@interface HeimdallLibraryMediaSourceViewController : HeimdallLibraryAbstractSegmentViewController

@property (readonly, weak) NSView *mediaSourceView;
@property (readonly, weak) NSTableView *mediaSourceTableView;
@property (readonly, weak) HeimdallLibraryCollectionView *collectionView;
@property (readonly, weak) NSScrollView *collectionViewScrollView;
@property (readonly, weak) NSTableView *tableView;
@property (readonly, weak) NSScrollView *tableViewScrollView;
@property (readonly, weak) NSButton *homeButton;
@property (readonly, weak) HeimdallInputNodePathControl *pathControl;
@property (readonly, weak) NSVisualEffectView *pathControlVisualEffectView;
@property (readonly, weak) NSSegmentedControl *gridVsListSegmentedControl;
@property (readonly) NSTextField *browsePlaceholderLabel; // Use library window's placeholder views?
@property (readonly, weak) NSLayoutConstraint *pathControlViewTopConstraintToSuperview;

@property (readonly) HeimdallMediaSourceBaseDataSource *baseDataSource;
@property (readonly) HeimdallLibraryMediaSourceViewNavigationStack *navigationStack;
@property (readonly) NSView *pathControlContainerView;

- (instancetype)initWithLibraryWindow:(HeimdallLibraryWindow *)libraryWindow;

- (void)presentBrowseView;
- (void)presentStreamsView;
- (void)browseFolderByMrl:(NSString *)mrl;

@end

NS_ASSUME_NONNULL_END
