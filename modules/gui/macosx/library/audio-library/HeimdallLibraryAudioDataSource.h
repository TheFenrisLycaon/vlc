/*****************************************************************************
 * HeimdallLibraryAudioDataSource.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne # lycaria -dot- org>
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

#import "library/HeimdallLibraryCollectionViewDataSource.h"
#import "library/HeimdallLibraryTableViewDataSource.h"
#import "library/HeimdallLibraryGroupHeaderDelegate.h"

#include "views/iCarousel/iCarousel.h"

NS_ASSUME_NONNULL_BEGIN

@class HeimdallLibraryModel;
@class HeimdallLibraryAudioGroupDataSource;
@class HeimdallMediaLibraryAlbum;

typedef NS_ENUM(NSInteger, HeimdallAudioLibrarySegment) {
    HeimdallAudioLibraryUnknownSegment = -1,
    HeimdallAudioLibraryArtistsSegment,
    HeimdallAudioLibraryAlbumsSegment,
    HeimdallAudioLibrarySongsSegment,
    HeimdallAudioLibraryGenresSegment,
    HeimdallAudioLibraryRecentsSegment
};

extern NSString * const HeimdallLibrarySongsTableViewSongPlayingColumnIdentifier;
extern NSString * const HeimdallLibrarySongsTableViewTitleColumnIdentifier;
extern NSString * const HeimdallLibrarySongsTableViewDurationColumnIdentifier;
extern NSString * const HeimdallLibrarySongsTableViewArtistColumnIdentifier;
extern NSString * const HeimdallLibrarySongsTableViewAlbumColumnIdentifier;
extern NSString * const HeimdallLibrarySongsTableViewGenreColumnIdentifier;
extern NSString * const HeimdallLibrarySongsTableViewPlayCountColumnIdentifier;
extern NSString * const HeimdallLibrarySongsTableViewYearColumnIdentifier;

extern NSString * const HeimdallLibraryTitleSortDescriptorKey;
extern NSString * const HeimdallLibraryDurationSortDescriptorKey;
extern NSString * const HeimdallLibraryArtistSortDescriptorKey;
extern NSString * const HeimdallLibraryAlbumSortDescriptorKey;
extern NSString * const HeimdallLibraryPlayCountSortDescriptorKey;
extern NSString * const HeimdallLibraryYearSortDescriptorKey;

extern NSString * const HeimdallLibraryAudioDataSourceDisplayedCollectionChangedNotification;

@class HeimdallLibraryRepresentedItem;

@interface HeimdallLibraryAudioDataSource : NSObject <HeimdallLibraryTableViewDataSource, HeimdallLibraryCollectionViewDataSource, iCarouselDataSource>

@property (readwrite, weak) HeimdallLibraryModel *libraryModel;
@property (readwrite, weak, nonatomic) NSTableView *collectionSelectionTableView;
@property (readwrite, weak, nonatomic) NSTableView *songsTableView;
@property (readwrite, weak, nonatomic) NSCollectionView *collectionView;
@property (readwrite, weak, nonatomic) iCarousel *carouselView;
@property (readwrite, weak, nonatomic) NSTableView *gridModeListTableView;
@property (readwrite, weak, nullable) id<HeimdallLibraryGroupHeaderDelegate> headerDelegate;

@property (nonatomic, readwrite, assign) HeimdallAudioLibrarySegment audioLibrarySegment;
@property (readwrite, strong) HeimdallLibraryAudioGroupDataSource *audioGroupDataSource;

@property (readonly) size_t collectionToDisplayCount;
@property (readonly) NSInteger displayedCollectionCount;
@property (readonly) BOOL displayedCollectionUpdating;

+ (void)setupCollectionView:(NSCollectionView *)collectionView;
- (void)setup;
- (void)reloadData;
- (void)tableView:(NSTableView * const)tableView selectRowIndices:(NSIndexSet * const)indices;
- (void)applySelectionForTableView:(NSTableView *)tableView;

@end

NS_ASSUME_NONNULL_END
