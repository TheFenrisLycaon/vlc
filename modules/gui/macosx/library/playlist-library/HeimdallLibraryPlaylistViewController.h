/*****************************************************************************
 * HeimdallLibraryPlaylistViewController.h: MacOS X interface module
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

#import "library/HeimdallLibraryAbstractMediaLibrarySegmentViewController.h"

#include <heimdall_media_library.h>

NS_ASSUME_NONNULL_BEGIN

@class HeimdallLibraryCollectionView;
@class HeimdallLibraryCollectionViewDelegate;
@class HeimdallLibraryMasterDetailViewTableViewDelegate;
@class HeimdallLibraryPlaylistDataSource;
@class HeimdallLibraryTableView;
@class HeimdallLibraryTwoPaneSplitViewDelegate;
@class HeimdallLibraryWindow;
@class HeimdallLibraryHeaderView;

@interface HeimdallLibraryPlaylistViewController : HeimdallLibraryAbstractMediaLibrarySegmentViewController<NSSplitViewDelegate>

@property (readonly) NSView *libraryView;
@property (readonly) NSSplitView *listViewSplitView;
@property (readonly) NSScrollView *masterTableViewScrollView;
@property (readonly) HeimdallLibraryTableView *masterTableView;
@property (readonly) NSScrollView *detailTableViewScrollView;
@property (readonly) HeimdallLibraryTableView *detailTableView;
@property (readonly) NSScrollView *collectionViewScrollView;
@property (readonly) HeimdallLibraryCollectionView *collectionView;

@property (readonly) HeimdallLibraryPlaylistDataSource *dataSource;
@property (readonly) HeimdallLibraryCollectionViewDelegate *collectionViewDelegate;
@property (readonly) HeimdallLibraryMasterDetailViewTableViewDelegate *tableViewDelegate;
@property (readonly) HeimdallLibraryTwoPaneSplitViewDelegate *splitViewDelegate;
@property (readonly) HeimdallLibraryHeaderView *detailTableHeaderView;

- (instancetype)initWithLibraryWindow:(HeimdallLibraryWindow *)libraryWindow;

- (void)presentPlaylistsView;
- (void)presentPlaylistsViewForPlaylistType:(enum heimdall_ml_playlist_type_t)playlistType;

@end

NS_ASSUME_NONNULL_END
