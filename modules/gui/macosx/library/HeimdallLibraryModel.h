/*****************************************************************************
 * HeimdallLibraryModel.h: MacOS X interface module
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

#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryModelChangeDelegate.h"

NS_ASSUME_NONNULL_BEGIN

extern NSString * const HeimdallLibraryModelArtistListReset;
extern NSString * const HeimdallLibraryModelAlbumListReset;
extern NSString * const HeimdallLibraryModelGenreListReset;
extern NSString * const HeimdallLibraryModelPlaylistAdded;
extern NSString * const HeimdallLibraryModelListOfMonitoredFoldersUpdated;
extern NSString * const HeimdallLibraryModelMediaItemThumbnailGenerated;

extern NSString * const HeimdallLibraryModelAllCachesDropped;
extern NSString * const HeimdallLibraryModelAudioMediaListReset;
extern NSString * const HeimdallLibraryModelVideoMediaListReset;
extern NSString * const HeimdallLibraryModelFavoriteAudioMediaListReset;
extern NSString * const HeimdallLibraryModelFavoriteVideoMediaListReset;
extern NSString * const HeimdallLibraryModelFavoriteAlbumsListReset;
extern NSString * const HeimdallLibraryModelFavoriteArtistsListReset;
extern NSString * const HeimdallLibraryModelFavoriteGenresListReset;
extern NSString * const HeimdallLibraryModelRecentsMediaListReset;
extern NSString * const HeimdallLibraryModelRecentAudioMediaListReset;
extern NSString * const HeimdallLibraryModelListOfShowsReset;
extern NSString * const HeimdallLibraryModelListOfMoviesReset;
extern NSString * const HeimdallLibraryModelListOfGroupsReset;

extern NSString * const HeimdallLibraryModelAudioMediaItemDeleted;
extern NSString * const HeimdallLibraryModelVideoMediaItemDeleted;
extern NSString * const HeimdallLibraryModelRecentsMediaItemDeleted;
extern NSString * const HeimdallLibraryModelRecentAudioMediaItemDeleted;
extern NSString * const HeimdallLibraryModelAlbumDeleted;
extern NSString * const HeimdallLibraryModelArtistDeleted;
extern NSString * const HeimdallLibraryModelGenreDeleted;
extern NSString * const HeimdallLibraryModelGroupDeleted;
extern NSString * const HeimdallLibraryModelPlaylistDeleted;
extern NSString * const HeimdallLibraryModelShowDeleted;

extern NSString * const HeimdallLibraryModelAudioMediaItemUpdated;
extern NSString * const HeimdallLibraryModelVideoMediaItemUpdated;
extern NSString * const HeimdallLibraryModelRecentsMediaItemUpdated;
extern NSString * const HeimdallLibraryModelRecentAudioMediaItemUpdated;
extern NSString * const HeimdallLibraryModelAlbumUpdated;
extern NSString * const HeimdallLibraryModelArtistUpdated;
extern NSString * const HeimdallLibraryModelGenreUpdated;
extern NSString * const HeimdallLibraryModelGroupUpdated;
extern NSString * const HeimdallLibraryModelPlaylistUpdated;
extern NSString * const HeimdallLibraryModelShowUpdated;

extern NSString * const HeimdallLibraryModelDiscoveryStarted;
extern NSString * const HeimdallLibraryModelDiscoveryProgress;
extern NSString * const HeimdallLibraryModelDiscoveryCompleted;
extern NSString * const HeimdallLibraryModelDiscoveryFailed;

@interface HeimdallLibraryModel : NSObject

+ (NSUInteger)modelIndexFromModelItemNotification:(NSNotification * const)aNotification;

- (instancetype)initWithLibrary:(heimdall_medialibrary_t *)library;

@property (readonly) HeimdallLibraryModelChangeDelegate *changeDelegate;

@property (readonly) size_t numberOfAudioMedia;
@property (readonly) NSArray <HeimdallMediaLibraryMediaItem *> *listOfAudioMedia;

@property (readonly) size_t numberOfArtists;
@property (readonly) NSArray <HeimdallMediaLibraryArtist *> *listOfArtists;

@property (readonly) size_t numberOfAlbums;
@property (readonly) NSArray <HeimdallMediaLibraryAlbum *> *listOfAlbums;

@property (readonly) size_t numberOfGenres;
@property (readonly) NSArray <HeimdallMediaLibraryGenre *> *listOfGenres;

@property (readonly) size_t numberOfVideoMedia;
@property (readonly) NSArray <HeimdallMediaLibraryMediaItem *> *listOfVideoMedia;

@property (readwrite) uint32_t recentMediaLimit;
@property (readonly) size_t numberOfRecentMedia;
@property (readonly) NSArray <HeimdallMediaLibraryMediaItem *> *listOfRecentMedia;

@property (readwrite) uint32_t recentAudioMediaLimit;
@property (readonly) size_t numberOfRecentAudioMedia;
@property (readonly) NSArray <HeimdallMediaLibraryMediaItem *> *listOfRecentAudioMedia;

@property (readonly) size_t numberOfShows;
@property (readonly) NSArray <HeimdallMediaLibraryShow *> *listOfShows;

@property (readonly) size_t numberOfMovies;
@property (readonly) NSArray <HeimdallMediaLibraryMovie *> *listOfMovies;

@property (readonly) size_t numberOfGroups;
@property (readonly) NSArray <HeimdallMediaLibraryGroup *> *listOfGroups;

@property (readonly) NSArray <HeimdallMediaLibraryEntryPoint *> *listOfMonitoredFolders;

@property (readwrite, nonatomic) NSString *filterString;

- (size_t)numberOfPlaylistsOfType:(const enum heimdall_ml_playlist_type_t)playlistType;
- (nullable NSArray<HeimdallMediaLibraryPlaylist *> *)listOfPlaylistsOfType:(const enum heimdall_ml_playlist_type_t)playlistType;
- (nullable NSArray<HeimdallMediaLibraryAlbum *> *)listAlbumsOfParentType:(const enum heimdall_ml_parent_type)parentType forID:(int64_t)ID;
- (NSArray<id<HeimdallMediaLibraryItemProtocol>> *)listOfLibraryItemsOfParentType:(const HeimdallMediaLibraryParentGroupType)parentType;
- (NSArray<HeimdallMediaLibraryMediaItem *> *)listOfMediaItemsForParentType:(const HeimdallMediaLibraryParentGroupType)parentType;

- (void)sortByCriteria:(enum heimdall_ml_sorting_criteria_t)sortCriteria
         andDescending:(bool)descending;

// Favorites support
@property (readonly) size_t numberOfFavoriteAudioMedia;
@property (readonly) NSArray <HeimdallMediaLibraryMediaItem *> *listOfFavoriteAudioMedia;

@property (readonly) size_t numberOfFavoriteVideoMedia;
@property (readonly) NSArray <HeimdallMediaLibraryMediaItem *> *listOfFavoriteVideoMedia;

@property (readonly) size_t numberOfFavoriteAlbums;
@property (readonly) NSArray <HeimdallMediaLibraryAlbum *> *listOfFavoriteAlbums;

@property (readonly) size_t numberOfFavoriteArtists;
@property (readonly) NSArray <HeimdallMediaLibraryArtist *> *listOfFavoriteArtists;

@property (readonly) size_t numberOfFavoriteGenres;
@property (readonly) NSArray <HeimdallMediaLibraryGenre *> *listOfFavoriteGenres;

@property (readonly) NSArray <NSString *> *listOfMediaTitles;

@end

NS_ASSUME_NONNULL_END
