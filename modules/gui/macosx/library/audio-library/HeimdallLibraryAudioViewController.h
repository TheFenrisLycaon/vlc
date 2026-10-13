/*****************************************************************************
 * HeimdallLibraryAudioViewController.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Claudio Cambra <claudio.cambra@gmail.com>
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
#import "library/HeimdallLibraryAbstractMediaLibrarySegmentViewController.h"
#import "library/HeimdallLibraryItemPresentingCapable.h"
#import "library/HeimdallLibraryGroupHeaderDelegate.h"

@class HeimdallLibraryAudioDataSource;
@class HeimdallLibraryAudioGroupDataSource;
@class HeimdallLibraryHeaderView;
@class HeimdallLibraryCollectionView;
@class HeimdallLibraryWindow;

@protocol HeimdallMediaLibraryItemProtocol;

NS_ASSUME_NONNULL_BEGIN

@interface HeimdallLibraryAudioViewController : HeimdallLibraryAbstractMediaLibrarySegmentViewController<HeimdallLibraryItemPresentingCapable, HeimdallLibraryGroupHeaderDelegate>

@property (readonly, weak) NSView *audioLibraryView;
@property (readonly, weak) NSSplitView *audioLibrarySplitView;
@property (readonly, weak) NSScrollView *audioCollectionSelectionTableViewScrollView;
@property (readonly, weak) NSTableView *audioCollectionSelectionTableView;
@property (readonly, strong) HeimdallLibraryHeaderView *audioCollectionHeaderView;
@property (readonly, weak) NSScrollView *audioGroupSelectionTableViewScrollView;
@property (readonly, weak) NSTableView *audioGroupSelectionTableView;
@property (readonly, weak) NSScrollView *audioSongTableViewScrollView;
@property (readonly, weak) NSTableView *audioSongTableView;
@property (readonly, weak) NSScrollView *audioCollectionViewScrollView;
@property (readonly, weak) HeimdallLibraryCollectionView *audioLibraryCollectionView;
@property (readonly, weak) NSSplitView *audioLibraryGridModeSplitView;
@property (readonly, weak) NSScrollView *audioLibraryGridModeSplitViewListTableViewScrollView;
@property (readonly, weak) NSTableView *audioLibraryGridModeSplitViewListTableView;
@property (readonly, weak) NSScrollView *audioLibraryGridModeSplitViewListSelectionCollectionViewScrollView;
@property (readonly, weak) HeimdallLibraryCollectionView *audioLibraryGridModeSplitViewListSelectionCollectionView;

@property (readonly) HeimdallLibraryAudioDataSource *audioDataSource;
@property (readonly) HeimdallLibraryAudioGroupDataSource *audioGroupDataSource;

- (instancetype)initWithLibraryWindow:(HeimdallLibraryWindow *)libraryWindow;

- (void)presentAudioView;
- (void)reloadData;

@end

NS_ASSUME_NONNULL_END
