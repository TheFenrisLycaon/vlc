/*****************************************************************************
 * HeimdallLibraryDataTypes.h: MacOS X interface module
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

#import <Foundation/Foundation.h>
#import <heimdall_media_library.h>

NS_ASSUME_NONNULL_BEGIN

extern NSString * const HeimdallMediaLibraryMediaItemPasteboardType;
extern NSString * const HeimdallMediaLibraryMediaItemUTI;

@class HeimdallMediaLibraryMediaItem;
@class HeimdallMediaLibraryAlbum;
@class HeimdallMediaLibraryArtist;
@class HeimdallMediaLibraryGenre;
@class HeimdallInputItem;

heimdall_medialibrary_t * _Nullable getMediaLibrary(void);

typedef NS_ENUM(NSUInteger, HeimdallMediaLibraryParentGroupType) {
    HeimdallMediaLibraryParentGroupTypeUnknown = HEIMDALL_ML_PARENT_UNKNOWN,
    HeimdallMediaLibraryParentGroupTypeAlbum = HEIMDALL_ML_PARENT_ALBUM,
    HeimdallMediaLibraryParentGroupTypeArtist = HEIMDALL_ML_PARENT_ARTIST,
    HeimdallMediaLibraryParentGroupTypeShow = HEIMDALL_ML_PARENT_SHOW,
    HeimdallMediaLibraryParentGroupTypeGenre = HEIMDALL_ML_PARENT_GENRE,
    HeimdallMediaLibraryParentGroupTypeGroup = HEIMDALL_ML_PARENT_GROUP,
    HeimdallMediaLibraryParentGroupTypeFolder = HEIMDALL_ML_PARENT_FOLDER,
    HeimdallMediaLibraryParentGroupTypePlaylist = HEIMDALL_ML_PARENT_PLAYLIST,
    // Additional types over heimdall_ml_parent_type below
    HeimdallMediaLibraryParentGroupTypeAudioLibrary,
    HeimdallMediaLibraryParentGroupTypeRecentAudios,
    // Video library-specific entries.
    // Please define these in the order the are expected to be presented
    HeimdallMediaLibraryParentGroupTypeRecentVideos,
    HeimdallMediaLibraryParentGroupTypeVideoLibrary,
    HeimdallMediaLibraryParentGroupTypeAllFavorites, // This should be last
};

@interface HeimdallMediaLibraryFile : NSObject

- (instancetype)initWithFile:(struct heimdall_ml_file_t *)p_file;

@property (readonly) NSString *MRL;
@property (readonly) NSURL *fileURL;
@property (readonly) heimdall_ml_file_type_t fileType;
@property (readonly) NSString *readableFileType;
@property (readonly) BOOL external;
@property (readonly) BOOL removable;
@property (readonly) BOOL present;
@property (readonly) time_t lastModificationDate;

@end

@interface HeimdallMediaLibraryTrack : NSObject

- (instancetype)initWithTrack:(struct heimdall_ml_media_track_t *)p_track;

@property (readonly) NSString *codec;
@property (readonly) NSString *readableCodecName;
@property (readonly) NSString *language;
@property (readonly) NSString *trackDescription;
@property (readonly, nullable) NSString *resolutionLabel;
@property (readonly) heimdall_ml_track_type_t trackType;
@property (readonly) NSString *readableTrackType;
@property (readonly) uint32_t bitrate;

@property (readonly) uint32_t numberOfAudioChannels;
@property (readonly) uint32_t audioSampleRate;

@property (readonly) uint32_t videoHeight;
@property (readonly) uint32_t videoWidth;
@property (readonly) uint32_t sourceAspectRatio;
@property (readonly) uint32_t sourceAspectRatioDenominator;
@property (readonly) uint32_t frameRate;
@property (readonly) uint32_t frameRateDenominator;

@end

@interface HeimdallMediaLibraryShowEpisode : NSObject

- (instancetype)initWithShowEpisode:(struct heimdall_ml_show_episode_t *)p_showEpisode;

@property (readonly) NSString *summary;
@property (readonly) NSString *tvdbID;
@property (readonly) uint32_t episodeNumber;
@property (readonly) uint32_t seasonNumber;

@end

@protocol HeimdallLocallyManipulableItemProtocol <NSObject>

- (void)revealInFinder;
- (void)moveToTrash;

@end

// Protocol with common methods and properties expected for media library items
@protocol HeimdallMediaLibraryItemProtocol <HeimdallLocallyManipulableItemProtocol>

@property (readonly) int64_t libraryID;
@property (readonly) BOOL smallArtworkGenerated;
@property (readonly) NSString *smallArtworkMRL;
@property (readonly) NSString *displayString;
@property (readonly) BOOL isFileBacked;
@property (readonly) NSString *primaryDetailString;
@property (readonly) NSString *secondaryDetailString;
@property (readonly) NSString *durationString;
@property (readonly) HeimdallMediaLibraryMediaItem *firstMediaItem;
// Media items should be delivered album-wise. If it is required for derivative
// types to provide media items in a different grouping or order, use methods
// or properties specific to the type (e.g. like in HeimdallMediaLibraryGenre)
@property (readonly) NSArray<HeimdallMediaLibraryMediaItem *> *mediaItems;
// If the info in detailString contains a library object that can be used for nav,
// we lazy load the actionable library item to avoid using the property until we
// actually need to -- resort to `actionableDetail` to know if there is one instead
@property (readonly) BOOL primaryActionableDetail;
@property (readonly) id<HeimdallMediaLibraryItemProtocol> primaryActionableDetailLibraryItem;
@property (readonly) BOOL secondaryActionableDetail;
@property (readonly) id<HeimdallMediaLibraryItemProtocol> secondaryActionableDetailLibraryItem;
@property (readonly) NSArray<NSString *> *labels;
@property (readonly) BOOL favorited;

- (int)setFavorite:(BOOL)favorite;
- (int)toggleFavorite;

- (void)enumerateMediaItemsWithBlock:(void (^)(HeimdallMediaLibraryMediaItem *item, BOOL *stop))mediaItemBlock;
- (void)iterateMediaItemsWithBlock:(void (^)(HeimdallMediaLibraryMediaItem*))mediaItemBlock;

@end

// Base abstract class with common implementations of properties used by media library items.
// Do not use directly -- subclass to create new media library item types.
@interface HeimdallAbstractMediaLibraryItem : NSObject<HeimdallMediaLibraryItemProtocol>

@end

// Extended HeimdallMediaLibraryItemProtocol that includes additional properties for media library item
// audio groups (i.e. artists, genres, etc.)
@protocol HeimdallMediaLibraryAudioGroupProtocol <HeimdallMediaLibraryItemProtocol>

@property (readonly) unsigned int numberOfTracks;
@property (readonly) NSArray<HeimdallMediaLibraryArtist *> *artists;
@property (readonly) NSArray<HeimdallMediaLibraryAlbum *> *albums;
@property (readonly) NSArray<HeimdallMediaLibraryGenre *> *genres;
@property (readonly) HeimdallMediaLibraryParentGroupType matchingParentType;

- (void)iterateMediaItemsWithBlock:(void (^)(HeimdallMediaLibraryMediaItem*))mediaItemBlock;

@end

// Like HeimdallAbstractMediaLibraryItem but with some additional functionality for audio groupings
// such as artists and genres. Do not use directly, subclass instead.
@interface HeimdallAbstractMediaLibraryAudioGroup : HeimdallAbstractMediaLibraryItem<HeimdallMediaLibraryAudioGroupProtocol>

@end

#pragma mark - Media library classes

@interface HeimdallMediaLibraryArtist : HeimdallAbstractMediaLibraryAudioGroup<HeimdallMediaLibraryAudioGroupProtocol>

+ (nullable instancetype)artistWithID:(int64_t)artistID;
- (instancetype)initWithArtist:(struct heimdall_ml_artist_t *)p_artist;

@property (readonly) NSString *name;
@property (readonly) NSString *genreString; // Lazy loaded for performance
@property (readonly) NSString *shortBiography;
@property (readonly) NSString *musicBrainzID;
@property (readonly) unsigned int numberOfAlbums;
@property (readonly) uint32_t numberOfPresentTracks;

@end

@interface HeimdallMediaLibraryAlbum : HeimdallAbstractMediaLibraryAudioGroup<HeimdallMediaLibraryAudioGroupProtocol>

+ (nullable instancetype)albumWithID:(int64_t)albumID;
- (instancetype)initWithAlbum:(struct heimdall_ml_album_t *)p_album;

@property (readonly) NSString *title;
@property (readonly) NSString *summary;
@property (readonly) NSString *artistName;
@property (readonly) NSString *genreString; // Lazy loaded for performance
@property (readonly) int64_t artistID;
@property (readonly) int64_t duration;
@property (readonly) unsigned int year;
@property (readonly) uint32_t numberOfPresentTracks;

@end

@interface HeimdallMediaLibraryGenre : HeimdallAbstractMediaLibraryAudioGroup<HeimdallMediaLibraryAudioGroupProtocol>

+ (nullable instancetype)genreWithID:(int64_t)genreID;
- (instancetype)initWithGenre:(struct heimdall_ml_genre_t *)p_genre;

@property (readonly) NSString *name;

@end

@interface HeimdallMediaLibraryShow : HeimdallAbstractMediaLibraryItem<HeimdallMediaLibraryItemProtocol>

+ (nullable instancetype)showWithLibraryId:(int64_t)libraryId;
- (instancetype)initWithShow:(struct heimdall_ml_show_t *)p_show;

@property (readonly) NSString *name;
@property (readonly) NSString *summary;
@property (readonly) NSString *tvdbId;
@property (readonly) unsigned int releaseYear;
@property (readonly) uint32_t episodeCount;
@property (readonly) uint32_t seasonCount;
@property (readonly) NSArray<HeimdallMediaLibraryMediaItem *> *episodes;

@end

@interface HeimdallMediaLibraryMovie : HeimdallAbstractMediaLibraryItem <HeimdallMediaLibraryItemProtocol>

- (instancetype)initWithMediaItem:(struct heimdall_ml_media_t *)p_media;

@property (readonly) NSString *summary;
@property (readonly) NSString *imdbID;

@end

@interface HeimdallMediaLibraryGroup : HeimdallAbstractMediaLibraryItem

@property (readonly) NSString *name;
@property (readonly) NSUInteger numberOfTotalItems;
@property (readonly) NSUInteger numberOfVideoItems;
@property (readonly) NSUInteger numberOfAudioItems;
@property (readonly) NSUInteger numberOfUnknownItems;
@property (readonly) NSUInteger numberOfPresentTotalItems;
@property (readonly) NSUInteger numberOfPresentVideoItems;
@property (readonly) NSUInteger numberOfPresentAudioItems;
@property (readonly) NSUInteger numberOfPresentUnknownItems;
@property (readonly) NSUInteger numberOfSeenItems;
@property (readonly) NSUInteger numberOfPresentSeenItems;
@property (readonly) NSInteger duration; // milliseconds
@property (readonly) NSDate *creationDate;
@property (readonly) NSDate *lastModificationDate;

+ (nullable instancetype)groupWithID:(int64_t)libraryID;
- (instancetype)initWithGroup:(struct heimdall_ml_group_t *)p_group;

@end

@interface HeimdallMediaLibraryPlaylist : HeimdallAbstractMediaLibraryItem<HeimdallMediaLibraryItemProtocol>

@property (readonly) NSString *MRL;

@property (readonly) NSArray<HeimdallMediaLibraryMediaItem *> *mediaItems;

@property (readonly) unsigned int numberOfMedia;
@property (readonly) uint32_t numberOfVideos;
@property (readonly) uint32_t numberOfAudios;
@property (readonly) uint32_t numberOfUnknowns;

@property (readonly) unsigned int numberOfPresentMedia;
@property (readonly) uint32_t numberOfPresentVideos;
@property (readonly) uint32_t numberOfPresentAudios;
@property (readonly) uint32_t numberOfPresentUnknowns;

@property (readonly) NSDate *creationDate;

@property (readonly) int64_t duration;
@property (readonly) uint32_t numberDurationUnknown;

@property (readonly) BOOL readOnly;

+ (instancetype)playlistForLibraryID:(int64_t)libraryID;
- (instancetype)initWithPlaylist:(const struct heimdall_ml_playlist_t * const)p_playlist;

- (void)removeMediaItemsAtPositions:(NSArray<NSNumber *> *)positions;
- (BOOL)appendMediaItems:(NSArray<HeimdallMediaLibraryMediaItem *> *)mediaItems;
- (BOOL)renameTo:(NSString *)name;

@end

@interface HeimdallMediaLibraryMediaItem : NSObject<HeimdallMediaLibraryItemProtocol, NSSecureCoding>

+ (nullable instancetype)mediaItemForLibraryID:(int64_t)libraryID;
+ (nullable instancetype)mediaItemForURL:(nullable NSURL *)url;
+ (nullable NSArray<HeimdallMediaLibraryMediaItem *> *)mediaItemsFromPasteboardData:(NSData *)data;
- (nullable instancetype)initWithMediaItem:(struct heimdall_ml_media_t *)mediaItem;
- (nullable instancetype)initWithExternalURL:(nullable NSURL *)url;
- (nullable instancetype)initWithStreamURL:(nullable NSURL *)url;

@property (readonly) heimdall_ml_media_type_t mediaType;
@property (readonly) NSString *readableMediaType;
@property (readonly) heimdall_ml_media_subtype_t mediaSubType;
@property (readonly) NSString *readableMediaSubType;
@property (readonly) HeimdallInputItem *inputItem;

@property (readonly) NSArray <HeimdallMediaLibraryFile *> *files;
@property (readonly) NSArray <HeimdallMediaLibraryTrack *> *tracks;
@property (readonly, nullable) HeimdallMediaLibraryTrack *firstVideoTrack;

@property (readonly) int32_t year;
@property (readonly) int64_t duration; /* Duration in milliseconds */
@property (readonly) uint32_t playCount;
@property (readonly) time_t lastPlayedDate;
@property (readonly) float progress;
@property (readonly) NSString *title;

@property (readonly, nullable) HeimdallMediaLibraryShowEpisode *showEpisode;
@property (readonly, nullable) HeimdallMediaLibraryMovie *movie;

@property (readwrite) int rating;
@property (readwrite) float lastPlaybackRate;
@property (readwrite) int lastTitle;
@property (readwrite) int lastChapter;
@property (readwrite) int lastProgram;
@property (readwrite) int lastVideoTrack;
@property (readwrite) NSString *lastAspectRatio;
@property (readwrite) NSString *lastZoom;
@property (readwrite) NSString *lastCrop;
@property (readwrite) NSString *lastDeinterlaceFilter;
@property (readwrite) NSString *lastVideoFilters;
@property (readwrite) int lastAudioTrack;
@property (readwrite) float lastGain;
@property (readwrite) int lastAudioDelay;
@property (readwrite) int lastSubtitleTrack;
@property (readwrite) int lastSubtitleDelay;

@property (readonly) int64_t artistID;
@property (readonly) int64_t albumID;
@property (readonly) int64_t genreID;

@property (readonly) int trackNumber;
@property (readonly) int discNumber;

@end

@interface HeimdallMediaLibraryEntryPoint : NSObject

- (instancetype)initWithEntryPoint:(struct heimdall_ml_folder_t *)p_entryPoint;

@property (readonly) NSString *MRL;
@property (readonly) NSString *decodedMRL;
@property (readonly) BOOL isPresent;
@property (readonly) BOOL isBanned;

@end

@interface HeimdallMediaLibraryDummyItem : NSObject<HeimdallMediaLibraryItemProtocol>

- (instancetype)initWithDisplayString:(NSString *)displayString
              withPrimaryDetailString:(nullable NSString *)primaryDetailString
            withSecondaryDetailString:(nullable NSString *)secondaryDetailString;

- (instancetype)initWithDisplayString:(NSString *)displayString
                       withMediaItems:(NSArray<HeimdallMediaLibraryMediaItem *> *)mediaItems;      

@end

NS_ASSUME_NONNULL_END
