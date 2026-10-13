/*****************************************************************************
 * HeimdallLibraryVideoViewController.h: MacOS X interface module
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

#import <Cocoa/Cocoa.h>

#import "library/HeimdallLibraryAbstractMediaLibrarySegmentViewController.h"
#import "library/HeimdallLibraryItemPresentingCapable.h"

@class HeimdallLibraryCollectionView;
@class HeimdallLibraryWindow;
@class HeimdallLibraryVideoDataSource;
@class HeimdallLibraryShowsDataSource;
@class HeimdallLibraryMoviesDataSource;

@protocol HeimdallMediaLibraryItemProtocol;

NS_ASSUME_NONNULL_BEGIN

// Controller for the video library views
@interface HeimdallLibraryVideoViewController : HeimdallLibraryAbstractMediaLibrarySegmentViewController<HeimdallLibraryItemPresentingCapable>

@property (readonly, weak) NSView *videoLibraryView;
@property (readonly, weak) NSScrollView *videoLibraryCollectionViewScrollView;
@property (readonly, weak) HeimdallLibraryCollectionView *videoLibraryCollectionView;
@property (readonly, weak) NSSplitView *videoLibrarySplitView;
@property (readonly, weak) NSScrollView *videoLibraryGroupsTableViewScrollView;
@property (readonly, weak) NSTableView *videoLibraryGroupsTableView;
@property (readonly, weak) NSScrollView *videoLibraryGroupSelectionTableViewScrollView;
@property (readonly, weak) NSTableView *videoLibraryGroupSelectionTableView;

@property (readonly, nullable) HeimdallLibraryVideoDataSource *libraryVideoDataSource;
@property (readonly, nullable) HeimdallLibraryShowsDataSource *libraryShowsDataSource;
@property (readonly, nullable) HeimdallLibraryMoviesDataSource *libraryMoviesDataSource;

- (instancetype)initWithLibraryWindow:(HeimdallLibraryWindow *)libraryWindow;
- (void)presentVideoView;
- (void)presentShowsView;
- (void)presentMoviesView;
- (void)presentLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem;

@end

NS_ASSUME_NONNULL_END
