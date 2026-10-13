/*****************************************************************************
 * HeimdallLibraryModel.m: MacOS X interface module
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

#import "HeimdallLibraryModel.h"
#include <sys/qos.h>

#import "HeimdallMediaLibraryFolderObserver.h"

#import "extensions/NSArray+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

#import "main/HeimdallMain.h"

NSString * const HeimdallLibraryModelArtistListReset = @"HeimdallLibraryModelArtistListReset";
NSString * const HeimdallLibraryModelAlbumListReset = @"HeimdallLibraryModelAlbumListReset";
NSString * const HeimdallLibraryModelGenreListReset = @"HeimdallLibraryModelGenreListReset";
NSString * const HeimdallLibraryModelListOfMonitoredFoldersUpdated = @"HeimdallLibraryModelListOfMonitoredFoldersUpdated";
NSString * const HeimdallLibraryModelMediaItemThumbnailGenerated = @"HeimdallLibraryModelMediaItemThumbnailGenerated";

NSString * const HeimdallLibraryModelAllCachesDropped = @"HeimdallLibraryModelAllCachesDropped";
NSString * const HeimdallLibraryModelAudioMediaListReset = @"HeimdallLibraryModelAudioMediaListReset";
NSString * const HeimdallLibraryModelVideoMediaListReset = @"HeimdallLibraryModelVideoMediaListReset";
NSString * const HeimdallLibraryModelFavoriteAudioMediaListReset = @"HeimdallLibraryModelFavoriteAudioMediaListReset";
NSString * const HeimdallLibraryModelFavoriteVideoMediaListReset = @"HeimdallLibraryModelFavoriteVideoMediaListReset";
NSString * const HeimdallLibraryModelFavoriteAlbumsListReset = @"HeimdallLibraryModelFavoriteAlbumsListReset";
NSString * const HeimdallLibraryModelFavoriteArtistsListReset = @"HeimdallLibraryModelFavoriteArtistsListReset";
NSString * const HeimdallLibraryModelFavoriteGenresListReset = @"HeimdallLibraryModelFavoriteGenresListReset";
NSString * const HeimdallLibraryModelRecentsMediaListReset = @"HeimdallLibraryModelRecentsMediaListReset";
NSString * const HeimdallLibraryModelRecentAudioMediaListReset = @"HeimdallLibraryModelRecentAudioMediaListReset";
NSString * const HeimdallLibraryModelListOfShowsReset = @"HeimdallLibraryModelListOfShowsReset";
NSString * const HeimdallLibraryModelListOfMoviesReset = @"HeimdallLibraryModelListOfMoviesReset";
NSString * const HeimdallLibraryModelListOfGroupsReset = @"HeimdallLibraryModelListOfGroupsReset";

NSString * const HeimdallLibraryModelPlaylistAdded = @"HeimdallLibraryModelPlaylistAdded";

NSString * const HeimdallLibraryModelAudioMediaItemDeleted = @"HeimdallLibraryModelAudioMediaItemDeleted";
NSString * const HeimdallLibraryModelVideoMediaItemDeleted = @"HeimdallLibraryModelVideoMediaItemDeleted";
NSString * const HeimdallLibraryModelRecentsMediaItemDeleted = @"HeimdallLibraryModelRecentsMediaItemDeleted";
NSString * const HeimdallLibraryModelRecentAudioMediaItemDeleted = @"HeimdallLibraryModelRecentAudioMediaItemDeleted";
NSString * const HeimdallLibraryModelAlbumDeleted = @"HeimdallLibraryModelAlbumDeleted";
NSString * const HeimdallLibraryModelArtistDeleted = @"HeimdallLibraryModelArtistDeleted";
NSString * const HeimdallLibraryModelGenreDeleted = @"HeimdallLibraryModelGenreDeleted";
NSString * const HeimdallLibraryModelGroupDeleted = @"HeimdallLibraryModelGroupDeleted";
NSString * const HeimdallLibraryModelPlaylistDeleted = @"HeimdallLibraryModelPlaylistDeleted";
NSString * const HeimdallLibraryModelShowDeleted = @"HeimdallLibraryModelShowDeleted";

NSString * const HeimdallLibraryModelAudioMediaItemUpdated = @"HeimdallLibraryModelAudioMediaItemUpdated";
NSString * const HeimdallLibraryModelVideoMediaItemUpdated = @"HeimdallLibraryModelVideoMediaItemUpdated";
NSString * const HeimdallLibraryModelRecentsMediaItemUpdated = @"HeimdallLibraryModelRecentsMediaItemUpdated";
NSString * const HeimdallLibraryModelRecentAudioMediaItemUpdated = @"HeimdallLibraryModelRecentAudioMediaItemUpdated";
NSString * const HeimdallLibraryModelAlbumUpdated = @"HeimdallLibraryModelAlbumUpdated";
NSString * const HeimdallLibraryModelArtistUpdated = @"HeimdallLibraryModelArtistUpdated";
NSString * const HeimdallLibraryModelGenreUpdated = @"HeimdallLibraryModelGenreUpdated";
NSString * const HeimdallLibraryModelGroupUpdated = @"HeimdallLibraryModelGroupUpdated";
NSString * const HeimdallLibraryModelPlaylistUpdated = @"HeimdallLibraryModelPlaylistUpdated";
NSString * const HeimdallLibraryModelShowUpdated = @"HeimdallLibraryModelShowUpdated";

NSString * const HeimdallLibraryModelDiscoveryStarted = @"HeimdallLibraryModelDiscoveryStarted";
NSString * const HeimdallLibraryModelDiscoveryProgress = @"HeimdallLibraryModelDiscoveryProgress";
NSString * const HeimdallLibraryModelDiscoveryCompleted = @"HeimdallLibraryModelDiscoveryCompleted";
NSString * const HeimdallLibraryModelDiscoveryFailed = @"HeimdallLibraryModelDiscoveryFailed";

@interface HeimdallLibraryModel ()
{
    heimdall_medialibrary_t *_p_mediaLibrary;
    heimdall_ml_event_callback_t *_p_eventCallback;

    NSNotificationCenter *_defaultNotificationCenter;

    enum heimdall_ml_sorting_criteria_t _sortCriteria;
    bool _sortDescending;

    size_t _initialVideoCount;
    size_t _initialAudioCount;
    size_t _initialAlbumCount;
    size_t _initialArtistCount;
    size_t _initialGenreCount;
    size_t _initialShowCount;
    size_t _initialMovieCount;
    size_t _initialGroupCount;
    size_t _initialRecentsCount;
    size_t _initialRecentAudioCount;

    dispatch_queue_t _mediaItemCacheModificationQueue;
    dispatch_queue_t _albumCacheModificationQueue;
    dispatch_queue_t _artistCacheModificationQueue;
    dispatch_queue_t _genreCacheModificationQueue;
    dispatch_queue_t _groupCacheModificationQueue;
    dispatch_queue_t _mediaTitlesCacheModificationQueue;
    NSCountedSet<NSString *> *_mediaTitleCounts;
    NSMutableDictionary<NSValue *, NSObject *> *_cacheUpdateTokens;
    NSMutableSet<NSValue *> *_pendingCacheUpdates;
    // Owned by the media-cache queue. Preserve membership after the ML removes
    // a relationship, even if an earlier media update has cleared the subtype.
    NSMutableDictionary<NSNumber *, NSNumber *> *_showIDsByEpisodeID;
    NSMutableDictionary<NSNumber *, NSObject *> *_showRefreshTokens;
    // Owned by each cache's modification queue. Later updates must preserve a
    // pending membership/order refresh even when they change unrelated fields.
    BOOL _audioCacheNeedsReload;
    BOOL _videoCacheNeedsReload;
    BOOL _artistCacheNeedsReload;
    BOOL _albumCacheNeedsReload;
    BOOL _genreCacheNeedsReload;
}

@property (readwrite) NSArray<HeimdallMediaLibraryFolderObserver *> *folderObservers;
@property (atomic) NSUInteger cacheGeneration;

@property (readwrite, nonatomic) NSArray *cachedAudioMedia;
@property (readwrite, nonatomic) NSArray *cachedArtists;
@property (readwrite, nonatomic) NSArray *cachedAlbums;
@property (readwrite, nonatomic) NSArray *cachedGenres;
@property (readwrite, nonatomic) NSArray *cachedVideoMedia;
@property (readwrite, nonatomic) NSArray *cachedListOfShows;
@property (readwrite, nonatomic) NSArray *cachedListOfMovies;
@property (readwrite, nonatomic) NSArray *cachedListOfGroups;
@property (readwrite, nonatomic) NSArray *cachedRecentMedia;
@property (readwrite, nonatomic) NSArray *cachedRecentAudioMedia;
@property (readwrite, nonatomic) NSArray *cachedListOfMonitoredFolders;
@property (readwrite, nonatomic) NSArray *cachedMediaTitles;

- (void)resetCachedListOfRecentMedia;
- (void)resetCachedListOfRecentAudioMedia;
- (void)resetCachedListOfArtists;
- (void)resetCachedListOfAlbums;
- (void)resetCachedListOfGenres;
- (void)resetCachedListOfShows;
- (void)resetCachedListOfGroups;
- (void)resetCachedListOfMonitoredFolders;
- (void)mediaItemThumbnailGenerated:(HeimdallMediaLibraryMediaItem *)mediaItem;
- (void)handleMediaItemAddedEvent:(const heimdall_ml_event_t * const)p_event;
- (void)handlePlaylistAddedEvent:(const heimdall_ml_event_t * const)p_event;
- (void)handleMediaItemDeletionEvent:(const heimdall_ml_event_t * const)p_event;
- (void)handleAlbumDeletionEvent:(const heimdall_ml_event_t * const)p_event;
- (void)handleArtistDeletionEvent:(const heimdall_ml_event_t * const)p_event;
- (void)handleGenreDeletionEvent:(const heimdall_ml_event_t * const)p_event;
- (void)handleGroupDeletionEvent:(const heimdall_ml_event_t * const)p_event;
- (void)handlePlaylistDeletionEvent:(const heimdall_ml_event_t * const)p_event;
- (void)handleMediaItemUpdateEvent:(const heimdall_ml_event_t * const)p_event;
- (void)handleAlbumUpdateEvent:(const heimdall_ml_event_t * const)p_event;
- (void)handleArtistUpdateEvent:(const heimdall_ml_event_t * const)p_event;
- (void)handleGenreUpdateEvent:(const heimdall_ml_event_t * const)p_event;
- (void)handleGroupUpdateEvent:(const heimdall_ml_event_t * const)p_event;
- (void)handlePlaylistUpdateEvent:(const heimdall_ml_event_t * const)p_event;
- (void)performAfterCacheWritesOnQueue:(dispatch_queue_t)queue
                                 block:(dispatch_block_t)block;

@end

static void libraryCallback(void *p_data, const heimdall_ml_event_t *p_event)
{
    HeimdallLibraryModel * const libraryModel = (__bridge HeimdallLibraryModel *)p_data;
    if (libraryModel == nil) {
        return;
    }

    @autoreleasepool {
        switch(p_event->i_type)
        {
            case HEIMDALL_ML_EVENT_MEDIA_ADDED:
                [libraryModel handleMediaItemAddedEvent:p_event];
                break;
            case HEIMDALL_ML_EVENT_MEDIA_UPDATED:
                [libraryModel handleMediaItemUpdateEvent:p_event];
                break;
            case HEIMDALL_ML_EVENT_MEDIA_DELETED:
                [libraryModel handleMediaItemDeletionEvent:p_event];
                break;
            case HEIMDALL_ML_EVENT_MEDIA_THUMBNAIL_GENERATED:
                if (p_event->media_thumbnail_generated.b_success) {
                    HeimdallMediaLibraryMediaItem *mediaItem = [[HeimdallMediaLibraryMediaItem alloc] initWithMediaItem:(struct heimdall_ml_media_t *)p_event->media_thumbnail_generated.p_media];
                    if (mediaItem == nil) {
                        return;
                    }
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [libraryModel mediaItemThumbnailGenerated:mediaItem];
                    });
                }
                break;
            case HEIMDALL_ML_EVENT_ARTIST_ADDED:
                [libraryModel resetCachedListOfArtists];
                break;
            case HEIMDALL_ML_EVENT_ARTIST_UPDATED:
                [libraryModel handleArtistUpdateEvent:p_event];
                break;
            case HEIMDALL_ML_EVENT_ARTIST_DELETED:
                [libraryModel handleArtistDeletionEvent:p_event];
                break;
            case HEIMDALL_ML_EVENT_ALBUM_ADDED:
                [libraryModel resetCachedListOfAlbums];
                break;
            case HEIMDALL_ML_EVENT_ALBUM_UPDATED:
                [libraryModel handleAlbumUpdateEvent:p_event];
                break;
            case HEIMDALL_ML_EVENT_ALBUM_DELETED:
                [libraryModel handleAlbumDeletionEvent:p_event];
                break;
            case HEIMDALL_ML_EVENT_GENRE_ADDED:
                [libraryModel resetCachedListOfGenres];
                break;
            case HEIMDALL_ML_EVENT_GENRE_UPDATED:
                [libraryModel handleGenreUpdateEvent:p_event];
                break;
            case HEIMDALL_ML_EVENT_GENRE_DELETED:
                [libraryModel handleGenreDeletionEvent:p_event];
                break;
            case HEIMDALL_ML_EVENT_GROUP_ADDED:
                [libraryModel resetCachedListOfGroups];
                break;
            case HEIMDALL_ML_EVENT_GROUP_UPDATED:
                [libraryModel handleGroupUpdateEvent:p_event];
                break;
            case HEIMDALL_ML_EVENT_GROUP_DELETED:
                [libraryModel handleGroupDeletionEvent:p_event];
                break;
            case HEIMDALL_ML_EVENT_PLAYLIST_ADDED:
                [libraryModel handlePlaylistAddedEvent:p_event];
                break;
            case HEIMDALL_ML_EVENT_PLAYLIST_UPDATED:
                [libraryModel handlePlaylistUpdateEvent:p_event];
                break;
            case HEIMDALL_ML_EVENT_PLAYLIST_DELETED:
                [libraryModel handlePlaylistDeletionEvent:p_event];
                break;
            case HEIMDALL_ML_EVENT_FOLDER_ADDED:
            case HEIMDALL_ML_EVENT_FOLDER_UPDATED:
            case HEIMDALL_ML_EVENT_FOLDER_DELETED:
                [libraryModel resetCachedListOfMonitoredFolders];
                break;
            case HEIMDALL_ML_EVENT_HISTORY_CHANGED:
                [libraryModel resetCachedListOfRecentMedia];
                [libraryModel resetCachedListOfRecentAudioMedia];
                break;
            case HEIMDALL_ML_EVENT_FAVORITES_CHANGED:
            {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallLibraryModelFavoriteAudioMediaListReset object:libraryModel];
                    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallLibraryModelFavoriteVideoMediaListReset object:libraryModel];
                    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallLibraryModelFavoriteAlbumsListReset object:libraryModel];
                    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallLibraryModelFavoriteArtistsListReset object:libraryModel];
                    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallLibraryModelFavoriteGenresListReset object:libraryModel];
                });
                break;
            }
            case HEIMDALL_ML_EVENT_DISCOVERY_STARTED:
                dispatch_async(dispatch_get_main_queue(), ^{
                    NSNotificationCenter * const defaultCenter = NSNotificationCenter.defaultCenter;
                    [defaultCenter postNotificationName:HeimdallLibraryModelDiscoveryStarted object:nil];
                });
                break;
            case HEIMDALL_ML_EVENT_DISCOVERY_PROGRESS:
            {
                NSString * const entryPoint = toNSStr(p_event->discovery_progress.psz_entry_point);
                dispatch_async(dispatch_get_main_queue(), ^{
                    NSDictionary<NSString *, NSString *> * const info = entryPoint == nil
                        ? nil
                        : @{@"entryPoint": entryPoint};
                    NSNotificationCenter * const defaultCenter = NSNotificationCenter.defaultCenter;
                    [defaultCenter postNotificationName:HeimdallLibraryModelDiscoveryProgress
                                                 object:nil
                                               userInfo:info];
                });
                break;
            }
            case HEIMDALL_ML_EVENT_DISCOVERY_COMPLETED:
                dispatch_async(dispatch_get_main_queue(), ^{
                    NSNotificationCenter * const defaultCenter = NSNotificationCenter.defaultCenter;
                    [defaultCenter postNotificationName:HeimdallLibraryModelDiscoveryCompleted object:nil];
                });
                break;
            case HEIMDALL_ML_EVENT_DISCOVERY_FAILED:
                dispatch_async(dispatch_get_main_queue(), ^{
                    NSNotificationCenter * const defaultCenter = NSNotificationCenter.defaultCenter;
                    [defaultCenter postNotificationName:HeimdallLibraryModelDiscoveryFailed object:nil];
                });
                break;
            default:
                break;
        }
    }
}

@implementation HeimdallLibraryModel

@synthesize cachedAudioMedia = _cachedAudioMedia;
@synthesize cachedVideoMedia = _cachedVideoMedia;
@synthesize cachedRecentMedia = _cachedRecentMedia;
@synthesize cachedRecentAudioMedia = _cachedRecentAudioMedia;
@synthesize cachedListOfShows = _cachedListOfShows;
@synthesize cachedListOfMovies = _cachedListOfMovies;
@synthesize cachedListOfMonitoredFolders = _cachedListOfMonitoredFolders;
@synthesize cachedAlbums = _cachedAlbums;
@synthesize cachedArtists = _cachedArtists;
@synthesize cachedGenres = _cachedGenres;
@synthesize cachedListOfGroups = _cachedListOfGroups;
@synthesize cachedMediaTitles = _cachedMediaTitles;

+ (NSUInteger)modelIndexFromModelItemNotification:(NSNotification * const)aNotification
{
    NSParameterAssert(aNotification);
    NSDictionary * const notificationUserInfo = aNotification.userInfo;
    NSAssert(notificationUserInfo != nil, @"Video item-related notification should carry valid user info");

    NSNumber * const modelIndexNumber = (NSNumber * const)[notificationUserInfo objectForKey:@"index"];
    NSAssert(modelIndexNumber != nil, @"Video item notification user info should carry index for updated item");

    return modelIndexNumber.longLongValue;
}

- (instancetype)initWithLibrary:(heimdall_medialibrary_t *)library
{
    self = [super init];
    if (self) {
        _changeDelegate = [[HeimdallLibraryModelChangeDelegate alloc] initWithLibraryModel:self];
        _sortCriteria = HEIMDALL_ML_SORTING_DEFAULT;
        _sortDescending = NO;
        _filterString = @"";
        _recentMediaLimit = 20;
        _p_mediaLibrary = library;
        _p_eventCallback = heimdall_ml_event_register_callback(_p_mediaLibrary, libraryCallback, (__bridge void *)self);

        // Concurrent queues allow multiple readers while using barrier flags for exclusive writes,
        // providing better performance than serial queues while maintaining thread safety
        _mediaItemCacheModificationQueue = dispatch_queue_create("mediaItemCacheModificationQueue", DISPATCH_QUEUE_CONCURRENT);
        _albumCacheModificationQueue = dispatch_queue_create("albumCacheModificationQueue", DISPATCH_QUEUE_CONCURRENT);
        _artistCacheModificationQueue = dispatch_queue_create("artistCacheModificationQueue", DISPATCH_QUEUE_CONCURRENT);
        _genreCacheModificationQueue = dispatch_queue_create("genreCacheModificationQueue", DISPATCH_QUEUE_CONCURRENT);
        _groupCacheModificationQueue = dispatch_queue_create("groupCacheModificationQueue", DISPATCH_QUEUE_CONCURRENT);
        _mediaTitlesCacheModificationQueue = dispatch_queue_create("mediaTitlesCacheModificationQueue", DISPATCH_QUEUE_CONCURRENT);
        _mediaTitleCounts = [NSCountedSet new];
        _cacheUpdateTokens = NSMutableDictionary.dictionary;
        _pendingCacheUpdates = NSMutableSet.set;
        _showRefreshTokens = NSMutableDictionary.dictionary;

        _defaultNotificationCenter = NSNotificationCenter.defaultCenter;
        [_defaultNotificationCenter addObserver:self
                                       selector:@selector(applicationWillTerminate:)
                                           name:NSApplicationWillTerminateNotification
                                         object:nil];

        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
            heimdall_ml_query_params_t queryParameters = heimdall_ml_query_params_create();

            // Preload video and audio count for gui
            self->_initialVideoCount = heimdall_ml_count_video_media(self->_p_mediaLibrary, &queryParameters);
            self->_initialAudioCount = heimdall_ml_count_audio_media(self->_p_mediaLibrary, &queryParameters);
            self->_initialAlbumCount = heimdall_ml_count_albums(self->_p_mediaLibrary, &queryParameters);
            self->_initialArtistCount = heimdall_ml_count_artists(self->_p_mediaLibrary, &queryParameters, true);
            self->_initialGenreCount = heimdall_ml_count_genres(self->_p_mediaLibrary, &queryParameters);
            self->_initialShowCount = heimdall_ml_count_shows(self->_p_mediaLibrary, &queryParameters);
            self->_initialMovieCount = heimdall_ml_count_movies(self->_p_mediaLibrary, &queryParameters);
            self->_initialGroupCount = heimdall_ml_count_groups(self->_p_mediaLibrary, &queryParameters);

            queryParameters.i_nbResults = self->_recentMediaLimit;
            self->_initialRecentsCount = heimdall_ml_count_video_history(self->_p_mediaLibrary, &queryParameters);
            self->_initialRecentAudioCount = heimdall_ml_count_audio_history(self->_p_mediaLibrary, &queryParameters);
        });

        [self resetCachedListOfMonitoredFolders];
    }
    return self;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    if (_p_eventCallback) {
        heimdall_ml_event_unregister_callback(_p_mediaLibrary, _p_eventCallback);
    }
}

- (void)dealloc
{
    [_defaultNotificationCenter removeObserver:self];
}

- (void)mediaItemThumbnailGenerated:(HeimdallMediaLibraryMediaItem *)mediaItem
{
    [self.changeDelegate notifyChange:HeimdallLibraryModelMediaItemThumbnailGenerated withObject:mediaItem];
}

#pragma mark - Thread-Safe Cache Getters

- (NSArray *)readCacheOnQueue:(dispatch_queue_t)queue withBlock:(NSArray *(^)(void))block
{
    __block NSArray *cache;
    dispatch_sync(queue, ^{
        cache = block();
    });
    return cache;
}

- (NSArray<HeimdallMediaLibraryMediaItem *> *)cachedAudioMedia
{
    return [self readCacheOnQueue:_mediaItemCacheModificationQueue withBlock:^NSArray *{
        return self->_cachedAudioMedia;
    }];
}

- (NSArray<HeimdallMediaLibraryMediaItem *> *)cachedVideoMedia
{
    return [self readCacheOnQueue:_mediaItemCacheModificationQueue withBlock:^NSArray *{
        return self->_cachedVideoMedia;
    }];
}

- (NSArray<HeimdallMediaLibraryMediaItem *> *)cachedRecentMedia
{
    return [self readCacheOnQueue:_mediaItemCacheModificationQueue withBlock:^NSArray *{
        return self->_cachedRecentMedia;
    }];
}

- (NSArray<HeimdallMediaLibraryMediaItem *> *)cachedRecentAudioMedia
{
    return [self readCacheOnQueue:_mediaItemCacheModificationQueue withBlock:^NSArray *{
        return self->_cachedRecentAudioMedia;
    }];
}

- (NSArray<HeimdallMediaLibraryShow *> *)cachedListOfShows
{
    return [self readCacheOnQueue:_mediaItemCacheModificationQueue withBlock:^NSArray *{
        return self->_cachedListOfShows;
    }];
}

- (NSArray<HeimdallMediaLibraryMovie *> *)cachedListOfMovies
{
    return [self readCacheOnQueue:_mediaItemCacheModificationQueue withBlock:^NSArray *{
        return self->_cachedListOfMovies;
    }];
}

- (NSArray<HeimdallMediaLibraryEntryPoint *> *)cachedListOfMonitoredFolders
{
    return [self readCacheOnQueue:_mediaItemCacheModificationQueue withBlock:^NSArray *{
        return self->_cachedListOfMonitoredFolders;
    }];
}

- (NSArray<HeimdallMediaLibraryAlbum *> *)cachedAlbums
{
    return [self readCacheOnQueue:_albumCacheModificationQueue withBlock:^NSArray *{
        return self->_cachedAlbums;
    }];
}

- (NSArray<HeimdallMediaLibraryArtist *> *)cachedArtists
{
    return [self readCacheOnQueue:_artistCacheModificationQueue withBlock:^NSArray *{
        return self->_cachedArtists;
    }];
}

- (NSArray<HeimdallMediaLibraryGenre *> *)cachedGenres
{
    return [self readCacheOnQueue:_genreCacheModificationQueue withBlock:^NSArray *{
        return self->_cachedGenres;
    }];
}

- (NSArray<HeimdallMediaLibraryGroup *> *)cachedListOfGroups
{
    return [self readCacheOnQueue:_groupCacheModificationQueue withBlock:^NSArray *{
        return self->_cachedListOfGroups;
    }];
}

- (NSArray<NSString *> *)cachedMediaTitles
{
    return [self readCacheOnQueue:_mediaTitlesCacheModificationQueue withBlock:^NSArray *{
        return self->_cachedMediaTitles;
    }];
}

#pragma mark - Custom Thread-Safe Cache Setters

- (void)setCachedAudioMedia:(NSArray *)cachedAudioMedia
{
    [self invalidateCacheUpdateForGetter:@selector(cachedAudioMedia)];
    NSArray * const media = [cachedAudioMedia copy];
    dispatch_barrier_async(_mediaItemCacheModificationQueue, ^{
        [self replaceMediaTitlesFromMedia:self->_cachedAudioMedia withMedia:media];
        self->_cachedAudioMedia = media;
        self->_audioCacheNeedsReload = NO;
    });
}

- (void)setCachedVideoMedia:(NSArray *)cachedVideoMedia
{
    [self invalidateCacheUpdateForGetter:@selector(cachedVideoMedia)];
    NSArray * const media = [cachedVideoMedia copy];
    dispatch_barrier_async(_mediaItemCacheModificationQueue, ^{
        [self replaceMediaTitlesFromMedia:self->_cachedVideoMedia withMedia:media];
        self->_cachedVideoMedia = media;
        self->_videoCacheNeedsReload = NO;
    });
}

- (void)setCachedRecentMedia:(NSArray *)cachedRecentMedia
{
    [self invalidateCacheUpdateForGetter:@selector(cachedRecentMedia)];
    dispatch_barrier_async(_mediaItemCacheModificationQueue, ^{
        self->_cachedRecentMedia = [cachedRecentMedia copy];
    });
}

- (void)setCachedRecentAudioMedia:(NSArray *)cachedRecentAudioMedia
{
    [self invalidateCacheUpdateForGetter:@selector(cachedRecentAudioMedia)];
    dispatch_barrier_async(_mediaItemCacheModificationQueue, ^{
        self->_cachedRecentAudioMedia = [cachedRecentAudioMedia copy];
    });
}

- (NSDictionary<NSNumber *, NSNumber *> *)showIDsByEpisodeIDForShows:(NSArray<HeimdallMediaLibraryShow *> *)shows
{
    NSMutableDictionary<NSNumber *, NSNumber *> * const showIDs = NSMutableDictionary.dictionary;
    for (HeimdallMediaLibraryShow * const show in shows) {
        if (show.episodeCount == 0) {
            continue;
        }
        heimdall_ml_media_list_t * const episodes = heimdall_ml_list_show_episodes(_p_mediaLibrary, NULL, show.libraryID);
        if (episodes == NULL) {
            return nil;
        }
        for (size_t index = 0; index < episodes->i_nb_items; index++) {
            showIDs[@(episodes->p_items[index].i_id)] = @(show.libraryID);
        }
        heimdall_ml_media_list_release(episodes);
    }
    return showIDs;
}

- (void)replaceCachedShows:(NSArray<HeimdallMediaLibraryShow *> *)shows
        showIDsByEpisodeID:(NSDictionary<NSNumber *, NSNumber *> *)showIDsByEpisodeID
{
    _cachedListOfShows = shows.copy;
    _showIDsByEpisodeID = showIDsByEpisodeID.mutableCopy;
    [_showRefreshTokens removeAllObjects];
}

- (void)setCachedListOfShows:(NSArray<HeimdallMediaLibraryShow *> *)cachedListOfShows
{
    NSArray<HeimdallMediaLibraryShow *> * const shows = cachedListOfShows.copy;
    if (shows == nil) {
        [self invalidateCacheUpdateForGetter:@selector(cachedListOfShows)];
        dispatch_barrier_async(_mediaItemCacheModificationQueue, ^{
            [self replaceCachedShows:nil showIDsByEpisodeID:nil];
        });
        return;
    }
    const NSUInteger generation = self.cacheGeneration;
    NSObject * const token = [self beginCacheUpdateForGetter:@selector(cachedListOfShows)];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
        NSDictionary<NSNumber *, NSNumber *> * const showIDs = [self showIDsByEpisodeIDForShows:shows];
        if (showIDs == nil) {
            return;
        }
        [self performCacheWriteForGeneration:generation
                                       token:token
                                      getter:@selector(cachedListOfShows)
                                     onQueue:self->_mediaItemCacheModificationQueue
                                       block:^{
            [self replaceCachedShows:shows showIDsByEpisodeID:showIDs];
        }];
    });
}

- (void)setCachedListOfMovies:(NSArray *)cachedListOfMovies
{
    [self invalidateCacheUpdateForGetter:@selector(cachedListOfMovies)];
    dispatch_barrier_async(_mediaItemCacheModificationQueue, ^{
        self->_cachedListOfMovies = [cachedListOfMovies copy];
    });
}

- (void)setCachedListOfMonitoredFolders:(NSArray *)cachedListOfMonitoredFolders
{
    [self invalidateCacheUpdateForGetter:@selector(cachedListOfMonitoredFolders)];
    dispatch_barrier_async(_mediaItemCacheModificationQueue, ^{
        self->_cachedListOfMonitoredFolders = [cachedListOfMonitoredFolders copy];
    });
}

- (void)setCachedAlbums:(NSArray *)cachedAlbums
{
    [self invalidateCacheUpdateForGetter:@selector(cachedAlbums)];
    dispatch_barrier_async(_albumCacheModificationQueue, ^{
        self->_cachedAlbums = [cachedAlbums copy];
        self->_albumCacheNeedsReload = NO;
    });
}

- (void)setCachedArtists:(NSArray *)cachedArtists
{
    [self invalidateCacheUpdateForGetter:@selector(cachedArtists)];
    dispatch_barrier_async(_artistCacheModificationQueue, ^{
        self->_cachedArtists = [cachedArtists copy];
        self->_artistCacheNeedsReload = NO;
    });
}

- (void)setCachedGenres:(NSArray *)cachedGenres
{
    [self invalidateCacheUpdateForGetter:@selector(cachedGenres)];
    dispatch_barrier_async(_genreCacheModificationQueue, ^{
        self->_cachedGenres = [cachedGenres copy];
        self->_genreCacheNeedsReload = NO;
    });
}

- (void)setCachedListOfGroups:(NSArray *)cachedListOfGroups
{
    [self invalidateCacheUpdateForGetter:@selector(cachedListOfGroups)];
    dispatch_barrier_async(_groupCacheModificationQueue, ^{
        self->_cachedListOfGroups = [cachedListOfGroups copy];
    });
}

- (void)setCachedMediaTitles:(NSArray *)cachedMediaTitles
{
    [self invalidateCacheUpdateForGetter:@selector(cachedMediaTitles)];
    dispatch_barrier_async(_mediaTitlesCacheModificationQueue, ^{
        self->_cachedMediaTitles = [cachedMediaTitles copy];
    });
}

- (size_t)numberOfAudioMedia
{
    NSArray<HeimdallMediaLibraryMediaItem *> * const cache = self.cachedAudioMedia;
    if (cache == nil) {
        [self resetCachedListOfAudioMedia];

        // Return initial count here, otherwise it will return 0 on the first time
        return _initialAudioCount;
    }
    return cache.count;
}

- (heimdall_ml_query_params_t)queryParams
{
    const heimdall_ml_query_params_t queryParams = { .psz_pattern = self->_filterString.length > 0 ? [self->_filterString UTF8String] : NULL, 
                                                .i_sort = self->_sortCriteria, 
                                                .b_desc = self->_sortDescending };
    return queryParams;
}

- (void)performAfterCacheWritesOnQueue:(dispatch_queue_t)queue
                                 block:(dispatch_block_t)block
{
    const NSUInteger generation = self.cacheGeneration;
    // The sentinel runs after all cache writes already queued on this queue.
    dispatch_barrier_async(queue, ^{
        dispatch_async(dispatch_get_main_queue(), ^{
            if (generation == self.cacheGeneration) {
                block();
            }
        });
    });
}

- (NSObject *)beginCacheUpdateForGetter:(SEL)getter
{
    NSObject * const token = NSObject.new;
    @synchronized (_cacheUpdateTokens) {
        _cacheUpdateTokens[[NSValue valueWithPointer:getter]] = token;
        [_pendingCacheUpdates addObject:[NSValue valueWithPointer:getter]];
    }
    return token;
}

- (BOOL)isCurrentCacheUpdate:(NSObject *)token forGetter:(SEL)getter
{
    @synchronized (_cacheUpdateTokens) {
        return _cacheUpdateTokens[[NSValue valueWithPointer:getter]] == token;
    }
}

- (BOOL)invalidateCacheUpdateForGetter:(SEL)getter
{
    @synchronized (_cacheUpdateTokens) {
        NSValue * const key = [NSValue valueWithPointer:getter];
        const BOOL pending = [_pendingCacheUpdates containsObject:key];
        [_cacheUpdateTokens removeObjectForKey:key];
        [_pendingCacheUpdates removeObject:key];
        return pending;
    }
}

- (void)performCacheMutationForGetters:(const SEL *)getters
                                count:(NSUInteger)count
                                block:(dispatch_block_t)block
{
    @synchronized (_cacheUpdateTokens) {
        BOOL pending[count];
        for (NSUInteger index = 0; index < count; index++) {
            pending[index] = [self invalidateCacheUpdateForGetter:getters[index]];
        }
        block();

        // Incremental changes cannot replace a pending membership/order query.
        // Reconcile again unless the mutation already started a newer query.
        for (NSUInteger index = 0; index < count; index++) {
            const SEL getter = getters[index];
            if (!pending[index] || [_pendingCacheUpdates containsObject:[NSValue valueWithPointer:getter]]) {
                continue;
            }
            if (getter == @selector(cachedAudioMedia))
                [self resetCachedListOfAudioMedia];
            else if (getter == @selector(cachedVideoMedia))
                [self resetCachedListOfVideoMedia];
            else if (getter == @selector(cachedRecentAudioMedia))
                [self resetCachedListOfRecentAudioMedia];
            else if (getter == @selector(cachedRecentMedia))
                [self resetCachedListOfRecentMedia];
            else if (getter == @selector(cachedArtists))
                [self resetCachedListOfArtists];
            else if (getter == @selector(cachedAlbums))
                [self resetCachedListOfAlbums];
            else if (getter == @selector(cachedGenres))
                [self resetCachedListOfGenres];
            else if (getter == @selector(cachedListOfGroups))
                [self resetCachedListOfGroups];
            else if (getter == @selector(cachedListOfShows))
                [self resetCachedListOfShows];
        }
    }
}

- (void)performCacheMutationForGetter:(SEL)getter block:(dispatch_block_t)block
{
    SEL const getters[] = { getter };
    [self performCacheMutationForGetters:getters count:1 block:block];
}

- (void)performCacheWriteForGeneration:(NSUInteger)generation
                                 token:(NSObject *)token
                                getter:(SEL)getter
                               onQueue:(dispatch_queue_t)queue
                                 block:(dispatch_block_t)block
{
    dispatch_barrier_async(queue, ^{
        // Validate at the write, not when the background query finishes.
        @synchronized (self->_cacheUpdateTokens) {
            if (generation == self.cacheGeneration && [self isCurrentCacheUpdate:token forGetter:getter]) {
                [self->_pendingCacheUpdates removeObject:[NSValue valueWithPointer:getter]];
                block();
            }
        }
    });
}

- (void)resetCachedListOfAudioMedia
{
    const NSUInteger generation = self.cacheGeneration;
    NSObject * const token = [self beginCacheUpdateForGetter:@selector(cachedAudioMedia)];
    NSString * const filterString = [self.filterString copy];
    const heimdall_ml_query_params_t capturedParams = [self queryParams];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
        heimdall_ml_query_params_t queryParams = capturedParams;
        queryParams.psz_pattern = filterString.length > 0 ? filterString.UTF8String : NULL;
        heimdall_ml_media_list_t * const p_media_list = heimdall_ml_list_audio_media(self->_p_mediaLibrary, &queryParams);
        NSArray * const mediaArray = [NSArray arrayFromHeimdallMediaList:p_media_list];
        if (mediaArray == nil) {
            return;
        }
        heimdall_ml_media_list_release(p_media_list);
        [self performCacheWriteForGeneration:generation
                                       token:token
                                      getter:@selector(cachedAudioMedia)
                                     onQueue:self->_mediaItemCacheModificationQueue
                                       block:^{
            [self replaceMediaTitlesFromMedia:self->_cachedAudioMedia withMedia:mediaArray];
            self->_cachedAudioMedia = mediaArray;
            self->_audioCacheNeedsReload = NO;
            [self performAfterCacheWritesOnQueue:self->_mediaItemCacheModificationQueue block:^{
                [self.changeDelegate notifyChange:HeimdallLibraryModelAudioMediaListReset withObject:self];
            }];
        }];
    });
}

- (NSArray<HeimdallMediaLibraryMediaItem *> *)listOfAudioMedia
{
    NSArray<HeimdallMediaLibraryMediaItem *> * const cache = self.cachedAudioMedia;
    if (cache == nil) {
        [self resetCachedListOfAudioMedia];
    }
    return cache;
}

- (size_t)numberOfArtists
{
    NSArray<HeimdallMediaLibraryArtist *> * const cache = self.cachedArtists;
    if (cache == nil) {
        [self resetCachedListOfArtists];
        // Return initial count here, otherwise it will return 0 on the first time
        return _initialArtistCount;
    }
    return cache.count;
}

- (void)resetCachedListOfArtists
{
    const NSUInteger generation = self.cacheGeneration;
    NSObject * const token = [self beginCacheUpdateForGetter:@selector(cachedArtists)];
    NSString * const filterString = [self.filterString copy];
    const heimdall_ml_query_params_t capturedParams = [self queryParams];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
        heimdall_ml_query_params_t queryParams = capturedParams;
        queryParams.psz_pattern = filterString.length > 0 ? filterString.UTF8String : NULL;
        heimdall_ml_artist_list_t * const p_artist_list = heimdall_ml_list_artists(self->_p_mediaLibrary, &queryParams, YES);
        const size_t numberOfArtists = p_artist_list->i_nb_items;
        NSMutableArray * const mutableArtistArray = [[NSMutableArray alloc] initWithCapacity:numberOfArtists];

        for (size_t x = 0; x < numberOfArtists; x++) {
            HeimdallMediaLibraryArtist * const artist = [[HeimdallMediaLibraryArtist alloc] initWithArtist:&p_artist_list->p_items[x]];

            if (artist != nil) {
                [mutableArtistArray addObject:artist];
            }
        }

        heimdall_ml_artist_list_release(p_artist_list);

        [self performCacheWriteForGeneration:generation
                                       token:token
                                      getter:@selector(cachedArtists)
                                     onQueue:self->_artistCacheModificationQueue
                                       block:^{
            self->_cachedArtists = mutableArtistArray.copy;
            self->_artistCacheNeedsReload = NO;
            [self performAfterCacheWritesOnQueue:self->_artistCacheModificationQueue block:^{
                if (![self isCurrentCacheUpdate:token forGetter:@selector(cachedArtists)]) {
                    return;
                }
                [self.changeDelegate notifyChange:HeimdallLibraryModelArtistListReset withObject:self];
            }];
        }];
    });
}

- (NSArray<HeimdallMediaLibraryArtist *> *)listOfArtists
{
    NSArray<HeimdallMediaLibraryArtist *> * const cache = self.cachedArtists;
    if (cache == nil) {
        [self resetCachedListOfArtists];
    }
    return cache;
}

- (size_t)numberOfAlbums
{
    NSArray<HeimdallMediaLibraryAlbum *> * const cache = self.cachedAlbums;
    if (cache == nil) {
        [self resetCachedListOfAlbums];
        // Return initial count here, otherwise it will return 0 on the first time
        return _initialAlbumCount;
    }
    return cache.count;
}

- (void)resetCachedListOfAlbums
{
    const NSUInteger generation = self.cacheGeneration;
    NSObject * const token = [self beginCacheUpdateForGetter:@selector(cachedAlbums)];
    NSString * const filterString = [self.filterString copy];
    const heimdall_ml_query_params_t capturedParams = [self queryParams];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
        heimdall_ml_query_params_t queryParams = capturedParams;
        queryParams.psz_pattern = filterString.length > 0 ? filterString.UTF8String : NULL;
        heimdall_ml_album_list_t * const p_album_list = heimdall_ml_list_albums(self->_p_mediaLibrary, &queryParams);
        const size_t numberOfAlbums = p_album_list->i_nb_items;
        NSMutableArray * const mutableAlbumArray = [[NSMutableArray alloc] initWithCapacity:numberOfAlbums];

        for (size_t x = 0; x < numberOfAlbums; x++) {
            HeimdallMediaLibraryAlbum * const album = [[HeimdallMediaLibraryAlbum alloc] initWithAlbum:&p_album_list->p_items[x]];
            [mutableAlbumArray addObject:album];
        }

        heimdall_ml_album_list_release(p_album_list);

        [self performCacheWriteForGeneration:generation
                                       token:token
                                      getter:@selector(cachedAlbums)
                                     onQueue:self->_albumCacheModificationQueue
                                       block:^{
            self->_cachedAlbums = mutableAlbumArray.copy;
            self->_albumCacheNeedsReload = NO;
            [self performAfterCacheWritesOnQueue:self->_albumCacheModificationQueue block:^{
                if (![self isCurrentCacheUpdate:token forGetter:@selector(cachedAlbums)]) {
                    return;
                }
                [self.changeDelegate notifyChange:HeimdallLibraryModelAlbumListReset withObject:self];
            }];
        }];
    });
}

- (NSArray<HeimdallMediaLibraryAlbum *> *)listOfAlbums
{
    NSArray<HeimdallMediaLibraryAlbum *> * const cache = self.cachedAlbums;
    if (cache == nil) {
        [self resetCachedListOfAlbums];
    }
    return cache;
}

- (size_t)numberOfGenres
{
    NSArray<HeimdallMediaLibraryGenre *> * const cache = self.cachedGenres;
    if (cache == nil) {
        [self resetCachedListOfGenres];
        // Return initial count here, otherwise it will return 0 on the first time
        return _initialGenreCount;
    }
    return cache.count;
}

- (void)resetCachedListOfGenres
{
    const NSUInteger generation = self.cacheGeneration;
    NSObject * const token = [self beginCacheUpdateForGetter:@selector(cachedGenres)];
    NSString * const filterString = [self.filterString copy];
    const heimdall_ml_query_params_t capturedParams = [self queryParams];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
        heimdall_ml_query_params_t queryParams = capturedParams;
        queryParams.psz_pattern = filterString.length > 0 ? filterString.UTF8String : NULL;
        heimdall_ml_genre_list_t * const p_genre_list = heimdall_ml_list_genres(self->_p_mediaLibrary, &queryParams);
        const size_t numberOfGenres = p_genre_list->i_nb_items;
        NSMutableArray * const mutableGenreArray = [[NSMutableArray alloc] initWithCapacity:numberOfGenres];

        for (size_t x = 0; x < numberOfGenres; x++) {
            HeimdallMediaLibraryGenre * const genre = [[HeimdallMediaLibraryGenre alloc] initWithGenre:&p_genre_list->p_items[x]];
            [mutableGenreArray addObject:genre];
        }

        heimdall_ml_genre_list_release(p_genre_list);

        [self performCacheWriteForGeneration:generation
                                       token:token
                                      getter:@selector(cachedGenres)
                                     onQueue:self->_genreCacheModificationQueue
                                       block:^{
            self->_cachedGenres = mutableGenreArray.copy;
            self->_genreCacheNeedsReload = NO;
            [self performAfterCacheWritesOnQueue:self->_genreCacheModificationQueue block:^{
                if (![self isCurrentCacheUpdate:token forGetter:@selector(cachedGenres)]) {
                    return;
                }
                [self.changeDelegate notifyChange:HeimdallLibraryModelGenreListReset withObject:self];
            }];
        }];
    });
}

- (NSArray<HeimdallMediaLibraryGenre *> *)listOfGenres
{
    NSArray<HeimdallMediaLibraryGenre *> * const cache = self.cachedGenres;
    if (cache == nil) {
        [self resetCachedListOfGenres];
    }
    return cache;
}

- (size_t)numberOfVideoMedia
{
    NSArray<HeimdallMediaLibraryMediaItem *> * const cache = self.cachedVideoMedia;
    if (cache == nil) {
        [self resetCachedListOfVideoMedia];

        // Return initial count here, otherwise it will return 0 on the first time
        return _initialVideoCount;
    }
    return cache.count;
}

- (void)resetCachedListOfVideoMedia
{
    const NSUInteger generation = self.cacheGeneration;
    NSObject * const token = [self beginCacheUpdateForGetter:@selector(cachedVideoMedia)];
    NSString * const filterString = [self.filterString copy];
    const heimdall_ml_query_params_t capturedParams = [self queryParams];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
        heimdall_ml_query_params_t queryParameters = capturedParams;
        queryParameters.psz_pattern = filterString.length > 0 ? filterString.UTF8String : NULL;
        heimdall_ml_media_list_t *p_media_list = heimdall_ml_list_video_media(self->_p_mediaLibrary, &queryParameters);
        if (p_media_list == NULL) {
            return;
        }
        NSMutableArray *mutableArray = [[NSMutableArray alloc] initWithCapacity:p_media_list->i_nb_items];
        for (size_t x = 0; x < p_media_list->i_nb_items; x++) {
            HeimdallMediaLibraryMediaItem *mediaItem = [[HeimdallMediaLibraryMediaItem alloc] initWithMediaItem:&p_media_list->p_items[x]];
            if (mediaItem != nil) {
                [mutableArray addObject:mediaItem];
            }
        }
        heimdall_ml_media_list_release(p_media_list);
        [self performCacheWriteForGeneration:generation
                                       token:token
                                      getter:@selector(cachedVideoMedia)
                                     onQueue:self->_mediaItemCacheModificationQueue
                                       block:^{
            NSArray<HeimdallMediaLibraryMediaItem *> * const media = mutableArray.copy;
            [self replaceMediaTitlesFromMedia:self->_cachedVideoMedia withMedia:media];
            self->_cachedVideoMedia = media;
            self->_videoCacheNeedsReload = NO;
            [self performAfterCacheWritesOnQueue:self->_mediaItemCacheModificationQueue block:^{
                [self.changeDelegate notifyChange:HeimdallLibraryModelVideoMediaListReset withObject:self];
            }];
        }];
    });
}

- (NSArray<HeimdallMediaLibraryMediaItem *> *)listOfVideoMedia
{
    NSArray<HeimdallMediaLibraryMediaItem *> * const cache = self.cachedVideoMedia;
    if (cache == nil) {
        [self resetCachedListOfVideoMedia];
    }
    return cache;
}

- (void)replaceMediaTitlesFromMedia:(NSArray<HeimdallMediaLibraryMediaItem *> *)oldMedia
                          withMedia:(NSArray<HeimdallMediaLibraryMediaItem *> *)newMedia
{
    dispatch_barrier_async(_mediaTitlesCacheModificationQueue, ^{
        for (HeimdallMediaLibraryMediaItem * const item in oldMedia) {
            NSString * const title = item.displayString;
            if (title) {
                [self->_mediaTitleCounts removeObject:title];
            }
        }
        for (HeimdallMediaLibraryMediaItem * const item in newMedia) {
            NSString * const title = item.displayString;
            if (title) {
                [self->_mediaTitleCounts addObject:title];
            }
        }
        if (newMedia == nil && self->_mediaTitleCounts.count == 0) {
            self->_cachedMediaTitles = nil;
        } else if (newMedia != nil || self->_cachedMediaTitles != nil) {
            self->_cachedMediaTitles = [self->_mediaTitleCounts.allObjects
                sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
        }
    });
}

- (void)replaceMediaTitle:(NSString *)oldTitle withTitle:(NSString *)newTitle
{
    if (oldTitle == newTitle || [oldTitle isEqualToString:newTitle]) {
        return;
    }

    dispatch_barrier_async(_mediaTitlesCacheModificationQueue, ^{
        BOOL removeTitle = NO;
        BOOL insertTitle = NO;
        if (oldTitle && [self->_mediaTitleCounts countForObject:oldTitle] > 0) {
            [self->_mediaTitleCounts removeObject:oldTitle];
            removeTitle = [self->_mediaTitleCounts countForObject:oldTitle] == 0;
        }
        if (newTitle) {
            insertTitle = [self->_mediaTitleCounts countForObject:newTitle] == 0;
            [self->_mediaTitleCounts addObject:newTitle];
        }
        if (self->_cachedMediaTitles == nil || (!removeTitle && !insertTitle)) {
            return;
        }

        NSMutableArray<NSString *> * const titles = self->_cachedMediaTitles.mutableCopy;
        if (removeTitle) {
            [titles removeObject:oldTitle];
        }
        if (insertTitle) {
            const NSUInteger index =
                [titles indexOfObject:newTitle
                        inSortedRange:NSMakeRange(0, titles.count)
                              options:NSBinarySearchingInsertionIndex
                      usingComparator:^NSComparisonResult(NSString *left, NSString *right) {
                return [left localizedCaseInsensitiveCompare:right];
            }];
            [titles insertObject:newTitle atIndex:index];
        }
        self->_cachedMediaTitles = titles.copy;
    });
}

- (NSArray<NSString *> *)listOfMediaTitles
{
    // Source setters maintain the counts; only a title lookup loads missing sources.
    if (self.cachedAudioMedia == nil) {
        [self resetCachedListOfAudioMedia];
    }
    if (self.cachedVideoMedia == nil) {
        [self resetCachedListOfVideoMedia];
    }
    return self.cachedMediaTitles;
}

- (void)getListOfRecentMediaOfType:(heimdall_ml_media_type_t)type
                    withCountLimit:(size_t)countLimit
                    withCompletion:(void (^)(NSArray *recentMediaArray))completionHandler
{
    NSString * const filterString = [self.filterString copy];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
        heimdall_ml_query_params_t queryParameters = heimdall_ml_query_params_create();
        queryParameters.i_nbResults = countLimit;
        queryParameters.psz_pattern = filterString.length > 0 ? filterString.UTF8String : NULL;
        // we don't set the sorting criteria here as they are not applicable to history
        heimdall_ml_media_list_t *p_media_list = NULL;
        if (type == HEIMDALL_ML_MEDIA_TYPE_VIDEO)
            p_media_list = heimdall_ml_list_video_history(self->_p_mediaLibrary, &queryParameters);
        else if (type == HEIMDALL_ML_MEDIA_TYPE_AUDIO)
            p_media_list = heimdall_ml_list_audio_history(self->_p_mediaLibrary, &queryParameters);

        NSArray * const mediaArray = [NSArray arrayFromHeimdallMediaList:p_media_list];
        if (mediaArray == nil) {
            return;
        }
        heimdall_ml_media_list_release(p_media_list);
        dispatch_async(dispatch_get_main_queue(), ^{
            completionHandler(mediaArray);
        });
    });
}

- (size_t)countOfRecentVideoMediaWithCountLimit:(size_t)countLimit
{
    heimdall_ml_query_params_t queryParameters = heimdall_ml_query_params_create();
    queryParameters.i_nbResults = countLimit;
    queryParameters.psz_pattern = self->_filterString.length > 0 ? self->_filterString.UTF8String : NULL;
    return heimdall_ml_count_video_history(self->_p_mediaLibrary, &queryParameters);
}

- (void)resetCachedListOfRecentMedia
{
    const NSUInteger generation = self.cacheGeneration;
    NSObject * const token = [self beginCacheUpdateForGetter:@selector(cachedRecentMedia)];
    [self getListOfRecentMediaOfType:HEIMDALL_ML_MEDIA_TYPE_VIDEO
                      withCountLimit:_recentMediaLimit
                      withCompletion:^(NSArray * const mediaArray) {
        [self performCacheWriteForGeneration:generation
                                       token:token
                                      getter:@selector(cachedRecentMedia)
                                     onQueue:self->_mediaItemCacheModificationQueue
                                       block:^{
            self->_cachedRecentMedia = mediaArray;
            [self performAfterCacheWritesOnQueue:self->_mediaItemCacheModificationQueue block:^{
                [self.changeDelegate notifyChange:HeimdallLibraryModelRecentsMediaListReset withObject:self];
            }];
        }];
    }];
}

- (size_t)numberOfRecentMedia
{
    NSArray<HeimdallMediaLibraryMediaItem *> * const cache = self.cachedRecentMedia;
    if (cache == nil) {
        [self resetCachedListOfRecentMedia];
        // Return the filtered count immediately during search, otherwise keep the startup fast path.
        if (_filterString.length > 0) {
            return [self countOfRecentVideoMediaWithCountLimit:_recentMediaLimit];
        }
        return _initialRecentsCount;
    }
    return cache.count;
}

- (NSArray<HeimdallMediaLibraryMediaItem *> *)listOfRecentMedia
{
    NSArray<HeimdallMediaLibraryMediaItem *> * const cache = self.cachedRecentMedia;
    if (cache == nil) {
        [self resetCachedListOfRecentMedia];
    }
    return cache;
}

- (void)resetCachedListOfRecentAudioMedia
{
    const NSUInteger generation = self.cacheGeneration;
    NSObject * const token = [self beginCacheUpdateForGetter:@selector(cachedRecentAudioMedia)];
    [self getListOfRecentMediaOfType:HEIMDALL_ML_MEDIA_TYPE_AUDIO
                      withCountLimit:_recentAudioMediaLimit
                      withCompletion:^(NSArray * const mediaArray) {
        [self performCacheWriteForGeneration:generation
                                       token:token
                                      getter:@selector(cachedRecentAudioMedia)
                                     onQueue:self->_mediaItemCacheModificationQueue
                                       block:^{
            self->_cachedRecentAudioMedia = mediaArray;
            [self performAfterCacheWritesOnQueue:self->_mediaItemCacheModificationQueue block:^{
                [self.changeDelegate notifyChange:HeimdallLibraryModelRecentAudioMediaListReset withObject:self];
            }];
        }];
    }];
}

- (size_t)numberOfRecentAudioMedia
{
    NSArray<HeimdallMediaLibraryMediaItem *> * const cache = self.cachedRecentAudioMedia;
    if (cache == nil) {
        [self resetCachedListOfRecentAudioMedia];
        // Return initial count here, otherwise it will return 0 on the first time
        return _initialRecentAudioCount;
    }
    return cache.count;
}

- (NSArray<HeimdallMediaLibraryMediaItem *> *)listOfRecentAudioMedia
{
    NSArray<HeimdallMediaLibraryMediaItem *> * const cache = self.cachedRecentAudioMedia;
    if (cache == nil) {
        [self resetCachedListOfRecentAudioMedia];
    }
    return cache;
}

- (void)resetCachedListOfShows
{
    const NSUInteger generation = self.cacheGeneration;
    NSObject * const token = [self beginCacheUpdateForGetter:@selector(cachedListOfShows)];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
        heimdall_ml_show_list_t * const p_show_list = heimdall_ml_list_shows(self->_p_mediaLibrary, NULL);
        if (p_show_list == NULL) {
            return;
        }
        const size_t itemCount = p_show_list->i_nb_items;
        NSMutableArray * const mutableArray = [[NSMutableArray alloc] initWithCapacity:itemCount];
        for (size_t x = 0; x < p_show_list->i_nb_items; x++) {
            heimdall_ml_show_t * const p_heimdall_show = &p_show_list->p_items[x];
            HeimdallMediaLibraryShow * const show = [[HeimdallMediaLibraryShow alloc] initWithShow:p_heimdall_show];
            if (show) {
                [mutableArray addObject:show];
            }
        }
        heimdall_ml_show_list_release(p_show_list);
        NSDictionary<NSNumber *, NSNumber *> * const showIDs = [self showIDsByEpisodeIDForShows:mutableArray];
        if (showIDs == nil) {
            return;
        }
        [self performCacheWriteForGeneration:generation
                                       token:token
                                      getter:@selector(cachedListOfShows)
                                     onQueue:self->_mediaItemCacheModificationQueue
                                       block:^{
            [self replaceCachedShows:mutableArray showIDsByEpisodeID:showIDs];
            [self performAfterCacheWritesOnQueue:self->_mediaItemCacheModificationQueue block:^{
                [self.changeDelegate notifyChange:HeimdallLibraryModelListOfShowsReset withObject:self];
            }];
        }];
    });
}

- (void)refreshCachedShowForEpisodeID:(int64_t)episodeID
{
    NSNumber * const showID = _showIDsByEpisodeID[@(episodeID)];
    if (showID == nil) {
        return;
    }
    if ([self invalidateCacheUpdateForGetter:@selector(cachedListOfShows)]) {
        // A parent-only lookup would lose other shows in the pending full list.
        [self resetCachedListOfShows];
        return;
    }
    const BOOL refreshPending = _showRefreshTokens[showID] != nil;
    _showRefreshTokens[showID] = NSObject.new;
    if (!refreshPending) {
        [self refreshCachedShowForLibraryID:showID.longLongValue];
    }
}

- (void)refreshCachedShowForLibraryID:(int64_t)libraryID
{
    const NSUInteger generation = self.cacheGeneration;
    NSNumber * const showID = @(libraryID);
    NSObject * const token = _showRefreshTokens[showID];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
        heimdall_ml_show_t * const p_show = heimdall_ml_get_show(self->_p_mediaLibrary, libraryID);
        HeimdallMediaLibraryShow * const updatedShow =
            p_show == NULL ? nil : [[HeimdallMediaLibraryShow alloc] initWithShow:p_show];
        if (p_show != NULL) {
            heimdall_ml_show_release(p_show);
        }
        [self updateCachedShow:updatedShow
                     libraryID:libraryID
                    generation:generation
                         token:token];
    });
}

- (void)updateCachedShow:(HeimdallMediaLibraryShow *)updatedShow
               libraryID:(int64_t)libraryID
              generation:(NSUInteger)generation
                   token:(NSObject *)token
{
    NSNumber * const showID = @(libraryID);
    dispatch_barrier_async(_mediaItemCacheModificationQueue, ^{
        NSObject * const currentToken = self->_showRefreshTokens[showID];
        if (generation != self.cacheGeneration || currentToken == nil) {
            return;
        }
        if (currentToken != token) {
            // Coalesce changes that arrived during this lookup
            [self refreshCachedShowForLibraryID:libraryID];
            return;
        }
        [self->_showRefreshTokens removeObjectForKey:showID];
        const NSUInteger index = [self->_cachedListOfShows indexOfObjectPassingTest:^BOOL(HeimdallMediaLibraryShow * const show, NSUInteger __unused idx, BOOL * __unused stop) {
            return show.libraryID == libraryID;
        }];
        if (self->_cachedListOfShows == nil || index == NSNotFound) {
            return;
        }
        NSMutableArray<HeimdallMediaLibraryShow *> * const shows = self->_cachedListOfShows.mutableCopy;
        const BOOL deleted = updatedShow == nil || updatedShow.episodeCount == 0;
        if (deleted) {
            [shows removeObjectAtIndex:index];
            NSSet<NSNumber *> * const episodeIDs = [self->_showIDsByEpisodeID keysOfEntriesPassingTest:^BOOL(NSNumber * __unused key, NSNumber * const parentID, BOOL * __unused stop) {
                return [parentID isEqualToNumber:showID];
            }];
            [self->_showIDsByEpisodeID removeObjectsForKeys:episodeIDs.allObjects];
        } else {
            [shows replaceObjectAtIndex:index withObject:updatedShow];
        }
        self->_cachedListOfShows = shows.copy;
        [self performAfterCacheWritesOnQueue:self->_mediaItemCacheModificationQueue block:^{
            [self.changeDelegate notifyChange:deleted ? HeimdallLibraryModelShowDeleted : HeimdallLibraryModelShowUpdated
                                   withObject:deleted ? showID : updatedShow];
        }];
    });
}

- (void)resetCachedListOfMovies
{
    const NSUInteger generation = self.cacheGeneration;
    NSObject * const token = [self beginCacheUpdateForGetter:@selector(cachedListOfMovies)];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
        heimdall_ml_media_list_t * const p_movie_list = heimdall_ml_list_movies(self->_p_mediaLibrary, NULL);
        if (p_movie_list == NULL) {
            return;
        }
        const size_t itemCount = p_movie_list->i_nb_items;
        NSMutableArray * const mutableArray = [[NSMutableArray alloc] initWithCapacity:itemCount];
        for (size_t x = 0; x < p_movie_list->i_nb_items; x++) {
            heimdall_ml_media_t * const p_heimdall_media = &p_movie_list->p_items[x];
            // Defensive: only add if subtype is MOVIE
            if (p_heimdall_media->i_subtype == HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE) {
                HeimdallMediaLibraryMovie * const movie = [[HeimdallMediaLibraryMovie alloc] initWithMediaItem:p_heimdall_media];
                if (movie) {
                    [mutableArray addObject:movie];
                }
            }
        }
        heimdall_ml_media_list_release(p_movie_list);
        [self performCacheWriteForGeneration:generation
                                       token:token
                                      getter:@selector(cachedListOfMovies)
                                     onQueue:self->_mediaItemCacheModificationQueue
                                       block:^{
            self->_cachedListOfMovies = mutableArray.copy;
            [self performAfterCacheWritesOnQueue:self->_mediaItemCacheModificationQueue block:^{
                [self.changeDelegate notifyChange:HeimdallLibraryModelListOfMoviesReset withObject:self];
                [NSNotificationCenter.defaultCenter postNotificationName:HeimdallLibraryModelListOfMoviesReset object:self];
            }];
        }];
    });
}

- (size_t)numberOfShows
{
    NSArray<HeimdallMediaLibraryShow *> * const cache = self.cachedListOfShows;
    if (cache == nil) {
        [self resetCachedListOfShows];
        // Return initial count here, otherwise it will return 0 on the first time
        return _initialShowCount;
    }
    return cache.count;
}

- (size_t)numberOfMovies
{
    NSArray<HeimdallMediaLibraryMovie *> * const cache = self.cachedListOfMovies;
    if (cache == nil) {
        [self resetCachedListOfMovies];
        // Return initial count here, otherwise it will return 0 on the first time
        return _initialMovieCount;
    }
    return cache.count;
}

- (NSArray<HeimdallMediaLibraryShow *> *)listOfShows
{
    NSArray<HeimdallMediaLibraryShow *> * const cache = self.cachedListOfShows;
    if (cache == nil) {
        [self resetCachedListOfShows];
    }
    return cache;
}

- (NSArray<HeimdallMediaLibraryMovie *> *)listOfMovies
{
    NSArray<HeimdallMediaLibraryMovie *> * const cache = self.cachedListOfMovies;
    if (cache == nil) {
        [self resetCachedListOfMovies];
    }
    return cache;
}

- (size_t)numberOfGroups
{
    NSArray<HeimdallMediaLibraryGroup *> * const cache = self.cachedListOfGroups;
    if (cache == nil) {
        [self resetCachedListOfGroups];
        // Return initial count here, otherwise it will return 0 on the first time
        return _initialGroupCount;
    }
    return cache.count;
}

- (NSArray<HeimdallMediaLibraryGroup *> *)listOfGroups
{
    NSArray<HeimdallMediaLibraryGroup *> * const cache = self.cachedListOfGroups;
    if (cache == nil) {
        [self resetCachedListOfGroups];
    }
    return cache;
}

- (void)resetCachedListOfGroups
{
    const NSUInteger generation = self.cacheGeneration;
    NSObject * const token = [self beginCacheUpdateForGetter:@selector(cachedListOfGroups)];
    NSString * const filterString = [self.filterString copy];
    const heimdall_ml_query_params_t capturedParams = [self queryParams];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
        heimdall_ml_query_params_t queryParams = capturedParams;
        queryParams.psz_pattern = filterString.length > 0 ? filterString.UTF8String : NULL;
        heimdall_ml_group_list_t * const p_group_list =
            heimdall_ml_list_groups(self->_p_mediaLibrary, &queryParams);
        if (p_group_list == NULL) {
            return;
        }
        const size_t itemCount = p_group_list->i_nb_items;
        NSMutableArray * const mutableArray = [[NSMutableArray alloc] initWithCapacity:itemCount];
        for (size_t x = 0; x < p_group_list->i_nb_items; x++) {
            heimdall_ml_group_t * const p_heimdall_group = &p_group_list->p_items[x];
            HeimdallMediaLibraryGroup * const group = [[HeimdallMediaLibraryGroup alloc] initWithGroup:p_heimdall_group];
            if (group) {
                [mutableArray addObject:group];
            }
        }
        heimdall_ml_group_list_release(p_group_list);
        [self performCacheWriteForGeneration:generation
                                       token:token
                                      getter:@selector(cachedListOfGroups)
                                     onQueue:self->_groupCacheModificationQueue
                                       block:^{
            self->_cachedListOfGroups = mutableArray.copy;
            [self performAfterCacheWritesOnQueue:self->_groupCacheModificationQueue block:^{
                [self.changeDelegate notifyChange:HeimdallLibraryModelListOfGroupsReset withObject:self];
            }];
        }];
    });
}

- (void)handleMediaItemAddedEvent:(const heimdall_ml_event_t * const)p_event
{
    NSParameterAssert(p_event);
    const heimdall_ml_media_t * const p_media = p_event->creation.p_media;
    NSParameterAssert(p_media);

    if (p_media->i_type == HEIMDALL_ML_MEDIA_TYPE_AUDIO || p_media->i_type == HEIMDALL_ML_MEDIA_TYPE_UNKNOWN) {
        [self resetCachedListOfAudioMedia];
    }

    if (p_media->i_type == HEIMDALL_ML_MEDIA_TYPE_VIDEO || p_media->i_type == HEIMDALL_ML_MEDIA_TYPE_UNKNOWN) {
        [self resetCachedListOfVideoMedia];

        if (p_media->i_subtype == HEIMDALL_ML_MEDIA_SUBTYPE_SHOW_EPISODE) {
            [self resetCachedListOfShows];
        }
    }
}

- (size_t)numberOfPlaylistsOfType:(const enum heimdall_ml_playlist_type_t)playlistType
{
    const heimdall_ml_query_params_t queryParams = self.queryParams;
    return heimdall_ml_count_playlists(_p_mediaLibrary, &queryParams, playlistType);
}

- (nullable NSArray<HeimdallMediaLibraryPlaylist *> *)listOfPlaylistsOfType:(const enum heimdall_ml_playlist_type_t)playlistType
{
    const heimdall_ml_query_params_t queryParams = self.queryParams;
    heimdall_ml_playlist_list_t * const p_playlistList =
        heimdall_ml_list_playlists(_p_mediaLibrary, &queryParams, playlistType);
    if (p_playlistList == NULL) {
        return nil;
    }

    NSMutableArray * const mutableArray =
        [[NSMutableArray alloc] initWithCapacity:p_playlistList->i_nb_items];
    for (size_t x = 0; x < p_playlistList->i_nb_items; x++) {
        HeimdallMediaLibraryPlaylist * const playlist =
            [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&p_playlistList->p_items[x]];
        if (playlist) {
            [mutableArray addObject:playlist];
        }
    }

    heimdall_ml_playlist_list_release(p_playlistList);
    return mutableArray.copy;
}

- (void)resetCachedListOfMonitoredFolders
{
    const NSUInteger generation = self.cacheGeneration;
    NSObject * const token = [self beginCacheUpdateForGetter:@selector(cachedListOfMonitoredFolders)];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
        heimdall_ml_folder_list_t *pp_entrypoints = heimdall_ml_list_entry_points(self->_p_mediaLibrary, NULL);
        if (pp_entrypoints == NULL) {
            msg_Err(getIntf(), "failed to retrieve list of monitored library folders");
            return;
        }

        NSMutableArray * const mutableArray =
            [[NSMutableArray alloc] initWithCapacity:pp_entrypoints->i_nb_items];

        for (size_t x = 0; x < pp_entrypoints->i_nb_items; x++) {
            HeimdallMediaLibraryEntryPoint * const entryPoint =
                [[HeimdallMediaLibraryEntryPoint alloc] initWithEntryPoint:&pp_entrypoints->p_items[x]];
            if (entryPoint) {
                [mutableArray addObject:entryPoint];
            }
        }

        heimdall_ml_folder_list_release(pp_entrypoints);

        [self performCacheWriteForGeneration:generation
                                       token:token
                                      getter:@selector(cachedListOfMonitoredFolders)
                                     onQueue:self->_mediaItemCacheModificationQueue
                                       block:^{
            // Stop old observers before creating new ones to avoid duplicate
            // FSEvent streams for the same paths
            self.folderObservers = nil;

            NSMutableArray * const mutableObservers =
                [[NSMutableArray alloc] initWithCapacity:mutableArray.count];
            for (HeimdallMediaLibraryEntryPoint * const entryPoint in mutableArray) {
                NSURL * const url = [NSURL URLWithString:entryPoint.MRL];
                HeimdallMediaLibraryFolderObserver * const observer =
                    [[HeimdallMediaLibraryFolderObserver alloc] initWithURL:url];
                if (observer) {
                    [mutableObservers addObject:observer];
                }
            }

            self->_cachedListOfMonitoredFolders = mutableArray.copy;
            self.folderObservers = mutableObservers.copy;
            [self performAfterCacheWritesOnQueue:self->_mediaItemCacheModificationQueue block:^{
                [self.changeDelegate notifyChange:HeimdallLibraryModelListOfMonitoredFoldersUpdated withObject:self];
            }];
        }];
    });
}

- (NSArray<HeimdallMediaLibraryEntryPoint *> *)listOfMonitoredFolders
{
    NSArray<HeimdallMediaLibraryEntryPoint *> * const cache = self.cachedListOfMonitoredFolders;
    if (cache == nil) {
        [self resetCachedListOfMonitoredFolders];
    }
    return cache;
}

- (nullable NSArray <HeimdallMediaLibraryAlbum *>*)listAlbumsOfParentType:(const enum heimdall_ml_parent_type)parentType forID:(int64_t)ID
{
    const heimdall_ml_query_params_t queryParams = [self queryParams];
    heimdall_ml_album_list_t *p_albumList = heimdall_ml_list_albums_of(_p_mediaLibrary, &queryParams, parentType, ID);
    if (p_albumList == NULL) {
        return nil;
    }
    NSMutableArray *mutableArray = [[NSMutableArray alloc] initWithCapacity:p_albumList->i_nb_items];
    for (size_t x = 0; x < p_albumList->i_nb_items; x++) {
        HeimdallMediaLibraryAlbum *album = [[HeimdallMediaLibraryAlbum alloc] initWithAlbum:&p_albumList->p_items[x]];
        [mutableArray addObject:album];
    }
    heimdall_ml_album_list_release(p_albumList);
    return [mutableArray copy];
}

- (NSArray<id<HeimdallMediaLibraryItemProtocol>> *)listOfLibraryItemsOfParentType:(const HeimdallMediaLibraryParentGroupType)parentType
{
    switch(parentType) {
    case HeimdallMediaLibraryParentGroupTypeAllFavorites:
    {
        const heimdall_ml_query_params_t queryParams = [self queryParams];
        heimdall_ml_media_list_t * const p_media_list = heimdall_ml_list_favorite_media(_p_mediaLibrary, &queryParams);
        NSArray * const mediaArray = [NSArray arrayFromHeimdallMediaList:p_media_list];
        if (p_media_list) {
            heimdall_ml_media_list_release(p_media_list);
        }
        return mediaArray ?: @[];
    }
    case HeimdallMediaLibraryParentGroupTypeArtist:
        return self.listOfArtists;
    case HeimdallMediaLibraryParentGroupTypeAlbum:
        return self.listOfAlbums;
    case HeimdallMediaLibraryParentGroupTypeGenre:
        return self.listOfGenres;
    case HeimdallMediaLibraryParentGroupTypeAudioLibrary:
        return self.listOfAudioMedia;
    case HeimdallMediaLibraryParentGroupTypeVideoLibrary:
        return self.listOfVideoMedia;
    case HeimdallMediaLibraryParentGroupTypeRecentVideos:
        return self.listOfRecentMedia;
    case HeimdallMediaLibraryParentGroupTypeUnknown:
    default:
        return nil;
    }
}

- (NSArray<HeimdallMediaLibraryMediaItem *> *)listOfMediaItemsForParentType:(const HeimdallMediaLibraryParentGroupType)parentType
{
    NSArray<id<HeimdallMediaLibraryItemProtocol>> * const libraryItems = [self listOfLibraryItemsOfParentType:parentType];
    NSMutableArray<HeimdallMediaLibraryMediaItem *> * const mediaItems = [[NSMutableArray alloc] initWithCapacity:self.numberOfAudioMedia];

    for (id<HeimdallMediaLibraryItemProtocol> libraryItem in libraryItems) {
        [mediaItems addObjectsFromArray:libraryItem.mediaItems];
    }

    return mediaItems.copy;
}


- (void)sortByCriteria:(enum heimdall_ml_sorting_criteria_t)sortCriteria andDescending:(bool)descending
{
    if(sortCriteria == _sortCriteria && descending == _sortDescending) {
        return;
    }

    _sortCriteria = sortCriteria;
    _sortDescending = descending;
    [self dropCaches];
}

- (void)setFilterString:(NSString *)filterString
{
    if([filterString isEqualToString:_filterString]) {
        return;
    }

    _filterString = filterString;
    [self dropCaches];
}

- (void)dropCaches
{
    @synchronized (_cacheUpdateTokens) {
        self.cacheGeneration++;
        [_cacheUpdateTokens removeAllObjects];
        [_pendingCacheUpdates removeAllObjects];
    }
    dispatch_queue_t const cacheQueues[] = {
        _mediaItemCacheModificationQueue,
        _albumCacheModificationQueue,
        _artistCacheModificationQueue,
        _genreCacheModificationQueue,
        _groupCacheModificationQueue,
        _mediaTitlesCacheModificationQueue,
    };
    const NSUInteger cacheQueueCount = sizeof(cacheQueues) / sizeof(cacheQueues[0]);
    dispatch_group_t const cacheDropGroup = dispatch_group_create();

    for (NSUInteger index = 0; index < cacheQueueCount; index++) {
        dispatch_group_enter(cacheDropGroup);
    }

    self.cachedVideoMedia = nil;
    self.cachedAudioMedia = nil;
    self.cachedRecentMedia = nil;
    self.cachedRecentAudioMedia = nil;
    self.cachedListOfShows = nil;
    self.cachedListOfMovies = nil;
    self.cachedListOfMonitoredFolders = nil;
    self.cachedAlbums = nil;
    self.cachedArtists = nil;
    self.cachedGenres = nil;
    self.cachedListOfGroups = nil;

    // Barrier sentinels complete after the setter writes on each queue.
    for (NSUInteger index = 0; index < cacheQueueCount; index++) {
        dispatch_barrier_async(cacheQueues[index], ^{
            dispatch_group_leave(cacheDropGroup);
        });
    }

    dispatch_group_notify(cacheDropGroup, _mediaTitlesCacheModificationQueue, ^{
        [self performAfterCacheWritesOnQueue:self->_mediaTitlesCacheModificationQueue block:^{
            [self.changeDelegate notifyChange:HeimdallLibraryModelAllCachesDropped withObject:self];
        }];
    });
}

- (void)performActionOnMediaItemInCache:(const int64_t)libraryId 
                                 action:(void (^)(
                                    NSMutableArray * const,
                                    const NSUInteger,
                                    NSMutableArray * const,
                                    const NSUInteger
                                 ))action
{
    const NSUInteger generation = self.cacheGeneration;
    dispatch_barrier_async(_mediaItemCacheModificationQueue, ^{
        if (generation != self.cacheGeneration) {
            return;
        }
        BOOL (^idCheckBlock)(HeimdallMediaLibraryMediaItem * const, const NSUInteger, BOOL * const) = ^BOOL(HeimdallMediaLibraryMediaItem * const mediaItem, const NSUInteger __unused idx, BOOL * const __unused stop) {
            NSAssert(mediaItem != nil, @"Cache list should not contain nil media items");
            return mediaItem.libraryID == libraryId;
       };

        NSArray<HeimdallMediaLibraryMediaItem *> * const cachedRecents = self->_cachedRecentMedia;
        NSArray<HeimdallMediaLibraryMediaItem *> * const cachedVideos = self->_cachedVideoMedia;

        const NSUInteger recentsIndex = cachedRecents ? [cachedRecents indexOfObjectPassingTest:idCheckBlock] : NSNotFound;
        const NSUInteger videoIndex = cachedVideos ? [cachedVideos indexOfObjectPassingTest:idCheckBlock] : NSNotFound;

        if (videoIndex != NSNotFound || recentsIndex != NSNotFound) {
            // Create mutable copies for modification
            NSMutableArray<HeimdallMediaLibraryMediaItem *> * const recentsMutable = cachedRecents.mutableCopy;
            NSMutableArray<HeimdallMediaLibraryMediaItem *> * const videoMutable = cachedVideos.mutableCopy;

            SEL const getters[] = { @selector(cachedVideoMedia), @selector(cachedRecentMedia) };
            [self performCacheMutationForGetters:getters count:2 block:^{
                action(videoMutable, videoIndex, recentsMutable, recentsIndex);
                // Item handlers adjust title counts individually inside this barrier.
                self->_cachedVideoMedia = videoMutable.copy;
                self->_cachedRecentMedia = recentsMutable.copy;
            }];
            return;
        }

        // Not in either video cache, check the audio caches.
        NSArray<HeimdallMediaLibraryMediaItem *> * const cachedRecentAudios = self->_cachedRecentAudioMedia;
        NSArray<HeimdallMediaLibraryMediaItem *> * const cachedAudios = self->_cachedAudioMedia;

        const NSUInteger recentAudiosIndex = cachedRecentAudios ? [cachedRecentAudios indexOfObjectPassingTest:idCheckBlock] : NSNotFound;
        const NSUInteger audioIndex = cachedAudios ? [cachedAudios indexOfObjectPassingTest:idCheckBlock] : NSNotFound;

        if (audioIndex != NSNotFound || recentAudiosIndex != NSNotFound) {
            // Found in an audio cache - create mutable copies for modification.
            NSMutableArray<HeimdallMediaLibraryMediaItem *> * const recentAudiosMutable = cachedRecentAudios.mutableCopy;
            NSMutableArray<HeimdallMediaLibraryMediaItem *> * const audioMutable = cachedAudios.mutableCopy;

            SEL const getters[] = { @selector(cachedAudioMedia), @selector(cachedRecentAudioMedia) };
            [self performCacheMutationForGetters:getters count:2 block:^{
                action(audioMutable, audioIndex, recentAudiosMutable, recentAudiosIndex);
                self->_cachedAudioMedia = audioMutable.copy;
                self->_cachedRecentAudioMedia = recentAudiosMutable.copy;
            }];
            return;
        }

        // Not found in any cache
        action(nil, NSNotFound, nil, NSNotFound);
    });
}

- (BOOL)shouldReloadCacheForUpdatedItem:(id<HeimdallMediaLibraryItemProtocol>)item
                           previousItem:(id<HeimdallMediaLibraryItemProtocol>)previousItem
{
    if (previousItem == nil || item.class != previousItem.class ||
        item.libraryID != previousItem.libraryID)
        return YES;

    if ([item isKindOfClass:HeimdallMediaLibraryGenre.class]) {
        // Genre queries search and sort by name, regardless of sort criterion.
        return ![((HeimdallMediaLibraryGenre *)item).name isEqualToString:((HeimdallMediaLibraryGenre *)previousItem).name];
    }

    if ([item isKindOfClass:HeimdallMediaLibraryArtist.class]) {
        HeimdallMediaLibraryArtist * const artist = (id)item;
        HeimdallMediaLibraryArtist * const previousArtist = (id)previousItem;
        if (![artist.name isEqualToString:previousArtist.name] ||
            (artist.numberOfPresentTracks > 0) != (previousArtist.numberOfPresentTracks > 0) ||
            (artist.numberOfTracks > 0) != (previousArtist.numberOfTracks > 0))
            return YES;
        switch (_sortCriteria) {
            case HEIMDALL_ML_SORTING_DEFAULT:
            case HEIMDALL_ML_SORTING_ALPHA:
                return NO;
            case HEIMDALL_ML_SORTING_TRACKNUMBER:
                return artist.numberOfTracks != previousArtist.numberOfTracks;
            default:
                return YES;
        }
    }

    if ([item isKindOfClass:HeimdallMediaLibraryAlbum.class]) {
        HeimdallMediaLibraryAlbum * const album = (id)item;
        HeimdallMediaLibraryAlbum * const previousAlbum = (id)previousItem;
        if (![album.title isEqualToString:previousAlbum.title] ||
            (album.numberOfPresentTracks > 0) != (previousAlbum.numberOfPresentTracks > 0) ||
            (_filterString.length > 0 && ![album.artistName isEqualToString:previousAlbum.artistName]))
            return YES;
        switch (_sortCriteria) {
            case HEIMDALL_ML_SORTING_DEFAULT:
            case HEIMDALL_ML_SORTING_ALPHA:
                return ![album.artistName isEqualToString:previousAlbum.artistName];
            case HEIMDALL_ML_SORTING_DURATION:
                return album.duration != previousAlbum.duration;
            case HEIMDALL_ML_SORTING_RELEASEDATE:
                return album.year != previousAlbum.year;
            case HEIMDALL_ML_SORTING_ARTIST:
                return ![album.artistName isEqualToString:previousAlbum.artistName];
            case HEIMDALL_ML_SORTING_TRACKNUMBER:
                return album.numberOfTracks != previousAlbum.numberOfTracks;
            default:
                // Aggregate play counts and insertion dates are not in the snapshot.
                return YES;
        }
    }

    if ([item isKindOfClass:HeimdallMediaLibraryGroup.class]) {
        HeimdallMediaLibraryGroup * const group = (id)item;
        HeimdallMediaLibraryGroup * const previousGroup = (id)previousItem;
        if (![group.name isEqualToString:previousGroup.name] ||
            (group.numberOfPresentTotalItems > 0) != (previousGroup.numberOfPresentTotalItems > 0))
            return YES;
        switch (_sortCriteria) {
            case HEIMDALL_ML_SORTING_DEFAULT:
            case HEIMDALL_ML_SORTING_ALPHA:
                return NO;
            case HEIMDALL_ML_SORTING_DURATION:
                return group.duration != previousGroup.duration;
            case HEIMDALL_ML_SORTING_INSERTIONDATE:
                return ![group.creationDate isEqualToDate:previousGroup.creationDate];
            case HEIMDALL_ML_SORTING_LASTMODIFICATIONDATE:
                return ![group.lastModificationDate isEqualToDate:previousGroup.lastModificationDate];
            default:
                return YES;
        }
    }

    // Media labels are lazy lookups, not snapshots of the previous search data.
    // Retain full queries where membership or sort keys cannot be compared safely.
    return _filterString.length > 0 ||
           (_sortCriteria != HEIMDALL_ML_SORTING_DEFAULT && _sortCriteria != HEIMDALL_ML_SORTING_ALPHA) ||
           ![previousItem.displayString isEqualToString:item.displayString];
}

- (void)handleMediaItemUpdateEvent:(const heimdall_ml_event_t * const)p_event
{
    NSParameterAssert(p_event != NULL);

    const int64_t itemId = p_event->modification.i_entity_id;

    HeimdallMediaLibraryMediaItem * const mediaItem = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:itemId];
    if (mediaItem == nil) {
        NSLog(@"Could not find a library media item with this ID. Can't handle update.");
        return;
    }

    [self performActionOnMediaItemInCache:itemId action:^(
        NSMutableArray * const cachedMediaArray,
        const NSUInteger cachedMediaIndex,
        NSMutableArray * const recentMediaArray,
        const NSUInteger recentMediaIndex
    ) {
        [self refreshCachedShowForEpisodeID:itemId];
        id<HeimdallMediaLibraryItemProtocol> const previousItem =
            cachedMediaIndex == NSNotFound ? nil : cachedMediaArray[cachedMediaIndex];
        const BOOL audio = mediaItem.mediaType == HEIMDALL_ML_MEDIA_TYPE_AUDIO;
        NSArray * const mainCache = audio ? self->_cachedAudioMedia : self->_cachedVideoMedia;
        NSArray * const recentCache = audio ? self->_cachedRecentAudioMedia : self->_cachedRecentMedia;
        const BOOL reloadMain = mainCache != nil &&
            ((audio ? self->_audioCacheNeedsReload : self->_videoCacheNeedsReload) ||
             [self shouldReloadCacheForUpdatedItem:mediaItem previousItem:previousItem]);
        const BOOL reloadRecent = recentCache != nil && self->_filterString.length > 0;
        if (reloadMain) {
            if (audio) {
                self->_audioCacheNeedsReload = YES;
                [self resetCachedListOfAudioMedia];
            } else {
                self->_videoCacheNeedsReload = YES;
                [self resetCachedListOfVideoMedia];
            }
        }
        if (reloadRecent) {
            if (audio)
                [self resetCachedListOfRecentAudioMedia];
            else
                [self resetCachedListOfRecentMedia];
        }
        if (cachedMediaIndex == NSNotFound && recentMediaIndex == NSNotFound) {
            if (!reloadMain && !reloadRecent)
                NSLog(@"Could not handle update for media library item with id %lld in model", itemId);
            return;
        }

        // Notify what happened
        if (cachedMediaIndex != NSNotFound && !reloadMain) {
            [self replaceMediaTitle:((HeimdallMediaLibraryMediaItem *)cachedMediaArray[cachedMediaIndex]).displayString
                          withTitle:mediaItem.displayString];
            [cachedMediaArray replaceObjectAtIndex:cachedMediaIndex withObject:mediaItem];
        }

        if (recentMediaArray != nil && recentMediaIndex != NSNotFound && !reloadRecent) {
            [recentMediaArray replaceObjectAtIndex:recentMediaIndex withObject:mediaItem];
            [self performAfterCacheWritesOnQueue:self->_mediaItemCacheModificationQueue block:^{
                switch (mediaItem.mediaType) {
                    case HEIMDALL_ML_MEDIA_TYPE_VIDEO:
                        [self.changeDelegate notifyChange:HeimdallLibraryModelRecentsMediaItemUpdated 
                                            withObject:mediaItem];
                        break;
                    case HEIMDALL_ML_MEDIA_TYPE_AUDIO:
                        [self.changeDelegate notifyChange:HeimdallLibraryModelRecentAudioMediaItemUpdated 
                                            withObject:mediaItem];
                        break;
                    case HEIMDALL_ML_MEDIA_TYPE_UNKNOWN:
                        NSLog(@"Unknown type of media type encountered, don't know what to do in deletion");
                        break;
                }
            }];
        }

        if (reloadMain) {
            return;
        }
        [self performAfterCacheWritesOnQueue:self->_mediaItemCacheModificationQueue block:^{
            switch (mediaItem.mediaType) {
                case HEIMDALL_ML_MEDIA_TYPE_VIDEO:
                    [self.changeDelegate notifyChange:HeimdallLibraryModelVideoMediaItemUpdated 
                                        withObject:mediaItem];
                    break;
                case HEIMDALL_ML_MEDIA_TYPE_AUDIO:
                    [self.changeDelegate notifyChange:HeimdallLibraryModelAudioMediaItemUpdated 
                                        withObject:mediaItem];
                    break;
                case HEIMDALL_ML_MEDIA_TYPE_UNKNOWN:
                    NSLog(@"Unknown type of media type encountered, don't know what to do in update");
                    break;
            }
        }];
    }];
}

- (void)handleMediaItemDeletionEvent:(const heimdall_ml_event_t * const)p_event
{
    NSParameterAssert(p_event != NULL);

    const int64_t itemId = p_event->modification.i_entity_id;

    [self performActionOnMediaItemInCache:itemId action:^(
        NSMutableArray * const cachedMediaArray,
        const NSUInteger cachedMediaIndex,
        NSMutableArray * const recentMediaArray,
        const NSUInteger recentMediaIndex
    ) {
        [self refreshCachedShowForEpisodeID:itemId];
        [self->_showIDsByEpisodeID removeObjectForKey:@(itemId)];

        // A deletion also supersedes any snapshot fetched before it. Preserve
        // pending membership/order changes by querying the latest state again.
        if (self->_audioCacheNeedsReload && self->_cachedAudioMedia != nil)
            [self resetCachedListOfAudioMedia];
        if (self->_videoCacheNeedsReload && self->_cachedVideoMedia != nil)
            [self resetCachedListOfVideoMedia];

        if (cachedMediaIndex == NSNotFound && recentMediaIndex == NSNotFound) {
            NSLog(@"Could not handle deletion for media library item with id %lld in model", itemId);
            return;
        }

        HeimdallMediaLibraryMediaItem * const mediaItem = cachedMediaIndex != NSNotFound
            ? cachedMediaArray[cachedMediaIndex] : recentMediaArray[recentMediaIndex];
        // Notify what happened
        if (cachedMediaIndex != NSNotFound) {
            [self replaceMediaTitle:mediaItem.displayString withTitle:nil];
            [cachedMediaArray removeObjectAtIndex:cachedMediaIndex];
        }

        if (recentMediaArray != nil && recentMediaIndex != NSNotFound) {
            [recentMediaArray removeObjectAtIndex:recentMediaIndex];
            [self performAfterCacheWritesOnQueue:self->_mediaItemCacheModificationQueue block:^{
                switch (mediaItem.mediaType) {
                    case HEIMDALL_ML_MEDIA_TYPE_VIDEO:
                        [self.changeDelegate notifyChange:HeimdallLibraryModelRecentsMediaItemDeleted
                                               withObject:mediaItem];
                        break;
                    case HEIMDALL_ML_MEDIA_TYPE_AUDIO:
                        [self.changeDelegate notifyChange:HeimdallLibraryModelRecentAudioMediaItemDeleted 
                                               withObject:mediaItem];
                        break;
                    case HEIMDALL_ML_MEDIA_TYPE_UNKNOWN:
                        NSLog(@"Unknown type of media type encountered, don't know what to do in deletion");
                        break;
                }
            }];
        }

        [self performAfterCacheWritesOnQueue:self->_mediaItemCacheModificationQueue block:^{
            switch (mediaItem.mediaType) {
                case HEIMDALL_ML_MEDIA_TYPE_VIDEO:
                    [self.changeDelegate notifyChange:HeimdallLibraryModelVideoMediaItemDeleted
                                           withObject:mediaItem];
                    break;
                case HEIMDALL_ML_MEDIA_TYPE_AUDIO:
                    [self.changeDelegate notifyChange:HeimdallLibraryModelAudioMediaItemDeleted
                                           withObject:mediaItem];
                    break;
                case HEIMDALL_ML_MEDIA_TYPE_UNKNOWN:
                    NSLog(@"Unknown type of media type encountered, don't know what to do in deletion");
                    break;
            }
        }];
    }];
}

- (NSInteger)indexForAudioGroupInCache:(NSArray * const)cache withItemId:(const int64_t)itemId
{
    if (cache == nil) {
        return NSNotFound;
    }
    return [cache indexOfObjectPassingTest:^BOOL(id<HeimdallMediaLibraryAudioGroupProtocol> audioGroupItem, const NSUInteger __unused idx, BOOL * const __unused stop) {
        NSAssert(audioGroupItem != nil, @"Cache list should not contain nil audio group items");
        return audioGroupItem.libraryID == itemId;
    }];
}

- (void)updateAudioGroupItem:(const id<HeimdallMediaLibraryAudioGroupProtocol>)audioGroupItem
                 usingGetter:(NSArray *(^)(void))cacheGetter
                    forCache:(SEL)getter
                 usingSetter:(void (^)(NSArray *))cacheSetter
                usingRefresh:(dispatch_block_t)cacheRefresh
               reloadPending:(BOOL *)reloadPending
                  usingQueue:(const dispatch_queue_t)queue
        withNotificationName:(const NSNotificationName)notificationName
{
    NSParameterAssert(cacheGetter != nil);
    NSParameterAssert(cacheSetter != nil);
    NSParameterAssert(cacheRefresh != nil);
    NSParameterAssert(reloadPending != NULL);
    const int64_t itemId = audioGroupItem.libraryID;

    dispatch_barrier_async(queue, ^{
        NSArray * const cache = cacheGetter();

        if (cache == nil) {
            return;
        }

        const NSUInteger audioGroupIndex = [self indexForAudioGroupInCache:cache
                                                                withItemId:itemId];
        id<HeimdallMediaLibraryItemProtocol> const previousItem =
            audioGroupIndex == NSNotFound ? nil : cache[audioGroupIndex];
        if (*reloadPending || [self shouldReloadCacheForUpdatedItem:audioGroupItem previousItem:previousItem]) {
            *reloadPending = YES;
            cacheRefresh();
            return;
        }

        // Create mutable copy for modification
        NSMutableArray * const mutableAudioGroupCache = [cache mutableCopy];
        [mutableAudioGroupCache replaceObjectAtIndex:audioGroupIndex withObject:audioGroupItem];

        // Complete the read-modify-write within this barrier, before another
        // event can take a snapshot of the same cache.
        [self performCacheMutationForGetter:getter block:^{
            cacheSetter(mutableAudioGroupCache.copy);
        }];

        [self performAfterCacheWritesOnQueue:queue block:^{
            [self.changeDelegate notifyChange:notificationName withObject:audioGroupItem];
        }];
    });
}

- (void)deleteAudioGroupItemWithId:(const int64_t)itemId
                       usingGetter:(NSArray *(^)(void))cacheGetter
                          forCache:(SEL)getter
                       usingSetter:(void (^)(NSArray *))cacheSetter
                      usingRefresh:(dispatch_block_t)cacheRefresh
                     reloadPending:(BOOL *)reloadPending
                        usingQueue:(const dispatch_queue_t)queue
              withNotificationName:(const NSNotificationName)notificationName
{
    NSParameterAssert(cacheGetter != nil);
    NSParameterAssert(cacheSetter != nil);
    NSParameterAssert(cacheRefresh != nil);
    NSParameterAssert(reloadPending != NULL);

    dispatch_barrier_async(queue, ^{
        NSArray * const cache = cacheGetter();

        if (cache == nil) {
            return;
        }

        if (*reloadPending) {
            cacheRefresh();
            return;
        }

        const NSUInteger audioGroupIndex = [self indexForAudioGroupInCache:cache
                                                                withItemId:itemId];
        if (audioGroupIndex == NSNotFound) {
            NSLog(@"Did not find audio group item with id %lli in cache.", itemId);
            return;
        }

        const id<HeimdallMediaLibraryAudioGroupProtocol> audioGroupItem = cache[audioGroupIndex];

        // Create mutable copy for modification
        NSMutableArray * const mutableAudioGroupCache = [cache mutableCopy];
        [mutableAudioGroupCache removeObjectAtIndex:audioGroupIndex];

        [self performCacheMutationForGetter:getter block:^{
            cacheSetter(mutableAudioGroupCache.copy);
        }];

        [self performAfterCacheWritesOnQueue:queue block:^{
            [self.changeDelegate notifyChange:notificationName withObject:audioGroupItem];
        }];
    });
}

- (void)handleAlbumUpdateEvent:(const heimdall_ml_event_t * const)p_event
{
    NSParameterAssert(p_event != NULL);

    const int64_t itemId = p_event->modification.i_entity_id;

    HeimdallMediaLibraryAlbum * const album = [HeimdallMediaLibraryAlbum albumWithID:itemId];
    if (album == nil) {
        NSLog(@"Could not find a library album with this ID. Can't handle update.");
        return;
    }

    [self updateAudioGroupItem:album
                   usingGetter:^NSArray *{ return self->_cachedAlbums; }
                      forCache:@selector(cachedAlbums)
                   usingSetter:^(NSArray *cache) {
        self->_cachedAlbums = cache;
    }
                  usingRefresh:^{ [self resetCachedListOfAlbums]; }
                 reloadPending:&_albumCacheNeedsReload
                    usingQueue:_albumCacheModificationQueue
          withNotificationName:HeimdallLibraryModelAlbumUpdated];
}

- (void)handleAlbumDeletionEvent:(const heimdall_ml_event_t * const)p_event
{
    NSParameterAssert(p_event != NULL);

    const int64_t itemId = p_event->modification.i_entity_id;

    [self deleteAudioGroupItemWithId:itemId
                         usingGetter:^NSArray *{ return self->_cachedAlbums; }
                            forCache:@selector(cachedAlbums)
                         usingSetter:^(NSArray *cache) {
        self->_cachedAlbums = cache;
    }
                        usingRefresh:^{ [self resetCachedListOfAlbums]; }
                       reloadPending:&_albumCacheNeedsReload
                          usingQueue:_albumCacheModificationQueue
                withNotificationName:HeimdallLibraryModelAlbumDeleted];
}

- (void)handleArtistUpdateEvent:(const heimdall_ml_event_t * const)p_event
{
    NSParameterAssert(p_event != NULL);

    const int64_t itemId = p_event->modification.i_entity_id;

    HeimdallMediaLibraryArtist * const artist = [HeimdallMediaLibraryArtist artistWithID:itemId];
    if (artist == nil) {
        NSLog(@"Could not find a library artist with this ID. Can't handle update.");
        return;
    }

    [self updateAudioGroupItem:artist
                   usingGetter:^NSArray *{ return self->_cachedArtists; }
                      forCache:@selector(cachedArtists)
                   usingSetter:^(NSArray *cache) {
        self->_cachedArtists = cache;
    }
                  usingRefresh:^{ [self resetCachedListOfArtists]; }
                 reloadPending:&_artistCacheNeedsReload
                    usingQueue:_artistCacheModificationQueue
          withNotificationName:HeimdallLibraryModelArtistUpdated];
}

- (void)handleArtistDeletionEvent:(const heimdall_ml_event_t * const)p_event
{
    NSParameterAssert(p_event != NULL);

    const int64_t itemId = p_event->modification.i_entity_id;

    [self deleteAudioGroupItemWithId:itemId
                         usingGetter:^NSArray *{ return self->_cachedArtists; }
                            forCache:@selector(cachedArtists)
                         usingSetter:^(NSArray *cache) {
        self->_cachedArtists = cache;
    }
                        usingRefresh:^{ [self resetCachedListOfArtists]; }
                       reloadPending:&_artistCacheNeedsReload
                          usingQueue:_artistCacheModificationQueue
                withNotificationName:HeimdallLibraryModelArtistDeleted];
}

- (void)handleGenreUpdateEvent:(const heimdall_ml_event_t * const)p_event
{
    NSParameterAssert(p_event != NULL);

    const int64_t itemId = p_event->modification.i_entity_id;

    HeimdallMediaLibraryGenre * const genre = [HeimdallMediaLibraryGenre genreWithID:itemId];
    if (genre == nil) {
        NSLog(@"Could not find a library genre with this ID. Can't handle update.");
        return;
    }

    [self updateAudioGroupItem:genre
                   usingGetter:^NSArray *{ return self->_cachedGenres; }
                      forCache:@selector(cachedGenres)
                   usingSetter:^(NSArray *cache) {
        self->_cachedGenres = cache;
    }
                  usingRefresh:^{ [self resetCachedListOfGenres]; }
                 reloadPending:&_genreCacheNeedsReload
                    usingQueue:_genreCacheModificationQueue
          withNotificationName:HeimdallLibraryModelGenreUpdated];
}

- (void)handleGenreDeletionEvent:(const heimdall_ml_event_t * const)p_event
{
    NSParameterAssert(p_event != NULL);

    const int64_t itemId = p_event->modification.i_entity_id;

    [self deleteAudioGroupItemWithId:itemId
                         usingGetter:^NSArray *{ return self->_cachedGenres; }
                            forCache:@selector(cachedGenres)
                         usingSetter:^(NSArray *cache) {
        self->_cachedGenres = cache;
    }
                        usingRefresh:^{ [self resetCachedListOfGenres]; }
                       reloadPending:&_genreCacheNeedsReload
                          usingQueue:_genreCacheModificationQueue
                withNotificationName:HeimdallLibraryModelGenreDeleted];
}

- (void)handleGroupDeletionEvent:(const heimdall_ml_event_t *const)p_event
{
    NSParameterAssert(p_event != NULL);

    const int64_t itemId = p_event->modification.i_entity_id;

    dispatch_barrier_async(_groupCacheModificationQueue, ^{
        NSArray<HeimdallMediaLibraryGroup *> * const cachedGroups = self->_cachedListOfGroups;
        if (cachedGroups == nil) {
            return;
        }

        const NSUInteger groupIdx =
            [cachedGroups indexOfObjectPassingTest:^BOOL(HeimdallMediaLibraryGroup * const group,
                                                         const NSUInteger __unused idx,
                                                         BOOL * const __unused stop) {
            return group.libraryID == itemId;
        }];

        if (groupIdx == NSNotFound) {
            NSLog(@"Could not handle deletion of group with id %lld in model", itemId);
            return;
        }

        HeimdallMediaLibraryGroup * const groupToDelete = cachedGroups[groupIdx];

        NSMutableArray * const mutableGroups = cachedGroups.mutableCopy;
        [mutableGroups removeObjectAtIndex:groupIdx];
        [self performCacheMutationForGetter:@selector(cachedListOfGroups) block:^{
            self->_cachedListOfGroups = mutableGroups.copy;
        }];

        [self performAfterCacheWritesOnQueue:self->_groupCacheModificationQueue block:^{
            [self->_defaultNotificationCenter postNotificationName:HeimdallLibraryModelGroupDeleted
                                                            object:groupToDelete];
        }];
    });
}

- (void)handleGroupUpdateEvent:(const heimdall_ml_event_t *const)p_event
{
    NSParameterAssert(p_event != NULL);

    const int64_t itemId = p_event->modification.i_entity_id;
    HeimdallMediaLibraryGroup * const group = [HeimdallMediaLibraryGroup groupWithID:itemId];
    if (group == nil) {
        NSLog(@"Could not find a library group with this ID. Can't handle update.");
        return;
    }

    dispatch_barrier_async(_groupCacheModificationQueue, ^{
        [self performCacheMutationForGetter:@selector(cachedListOfGroups) block:^{
            NSArray<HeimdallMediaLibraryGroup *> * const cachedGroups = self->_cachedListOfGroups;
            if (cachedGroups == nil) {
                return;
            }

            const NSUInteger groupIdx =
                [cachedGroups indexOfObjectPassingTest:^BOOL(HeimdallMediaLibraryGroup * const group,
                                                             const NSUInteger __unused idx,
                                                             BOOL * const __unused stop) {
                NSAssert(group != nil, @"Cache list should not contain nil groups");
                return group.libraryID == itemId;
            }];
            HeimdallMediaLibraryGroup * const previousGroup = groupIdx == NSNotFound ? nil : cachedGroups[groupIdx];
            if ([self shouldReloadCacheForUpdatedItem:group previousItem:previousGroup]) {
                [self resetCachedListOfGroups];
                return;
            }

            NSMutableArray * const mutableGroups = cachedGroups.mutableCopy;
            [mutableGroups replaceObjectAtIndex:groupIdx withObject:group];
            self->_cachedListOfGroups = mutableGroups.copy;

            [self performAfterCacheWritesOnQueue:self->_groupCacheModificationQueue block:^{
                [self->_defaultNotificationCenter postNotificationName:HeimdallLibraryModelGroupUpdated
                                                                object:group];
            }];
        }];
    });
}

- (void)handlePlaylistAddedEvent:(const heimdall_ml_event_t * const)p_event
{
    NSParameterAssert(p_event != NULL);

    const heimdall_ml_playlist_t * const p_playlist = p_event->creation.p_playlist;
    HeimdallMediaLibraryPlaylist * const playlist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:p_playlist];
    if (playlist == nil) {
        NSLog(@"Could not find a library playlist with this ID. Can't handle update.");
        return;
    }

    dispatch_async(dispatch_get_main_queue(), ^{
        [self->_defaultNotificationCenter postNotificationName:HeimdallLibraryModelPlaylistAdded
                                                        object:playlist];
    });
}

- (void)handlePlaylistUpdateEvent:(const heimdall_ml_event_t * const)p_event
{
    NSParameterAssert(p_event != NULL);

    const int64_t itemId = p_event->modification.i_entity_id;
    HeimdallMediaLibraryPlaylist * const playlist = [HeimdallMediaLibraryPlaylist playlistForLibraryID:itemId];
    if (playlist == nil) {
        NSLog(@"Could not find a library playlist with this ID. Can't handle update.");
        return;
    }

    dispatch_async(dispatch_get_main_queue(), ^{
        [self->_defaultNotificationCenter postNotificationName:HeimdallLibraryModelPlaylistUpdated
                                                        object:playlist];
    });
}

- (void)handlePlaylistDeletionEvent:(const heimdall_ml_event_t * const)p_event
{
    NSParameterAssert(p_event != NULL);

    const int64_t itemId = p_event->modification.i_entity_id;
    dispatch_async(dispatch_get_main_queue(), ^{
        [self->_defaultNotificationCenter postNotificationName:HeimdallLibraryModelPlaylistDeleted 
                                                        object:@(itemId)];
    });
}

#pragma mark - Favorites Support

- (NSArray<HeimdallMediaLibraryMediaItem *> *)listOfFavoriteAudioMedia
{
    const heimdall_ml_query_params_t queryParams = [self queryParams];
    heimdall_ml_media_list_t * const p_media_list = heimdall_ml_list_favorite_audios(_p_mediaLibrary, &queryParams);
    NSArray * const mediaArray = [NSArray arrayFromHeimdallMediaList:p_media_list];
    if (mediaArray == nil) {
        return @[];
    }
    heimdall_ml_media_list_release(p_media_list);
    return mediaArray;
}

- (NSArray<HeimdallMediaLibraryMediaItem *> *)listOfFavoriteVideoMedia
{
    const heimdall_ml_query_params_t queryParams = [self queryParams];
    heimdall_ml_media_list_t * const p_media_list = heimdall_ml_list_favorite_videos(_p_mediaLibrary, &queryParams);
    NSArray * const mediaArray = [NSArray arrayFromHeimdallMediaList:p_media_list];
    if (mediaArray == nil) {
        return @[];
    }
    heimdall_ml_media_list_release(p_media_list);
    return mediaArray;
}

- (NSArray<HeimdallMediaLibraryAlbum *> *)listOfFavoriteAlbums
{
    const heimdall_ml_query_params_t queryParams = [self queryParams];
    heimdall_ml_album_list_t * const p_album_list = heimdall_ml_list_favorite_albums(_p_mediaLibrary, &queryParams);
    if (p_album_list == NULL) {
        return @[];
    }
    
    NSMutableArray * const mutableArray = [[NSMutableArray alloc] initWithCapacity:p_album_list->i_nb_items];
    for (size_t x = 0; x < p_album_list->i_nb_items; x++) {
        HeimdallMediaLibraryAlbum * const album = [[HeimdallMediaLibraryAlbum alloc] initWithAlbum:&p_album_list->p_items[x]];
        if (album != nil) {
            [mutableArray addObject:album];
        }
    }
    heimdall_ml_album_list_release(p_album_list);
    return mutableArray.copy;
}

- (NSArray<HeimdallMediaLibraryArtist *> *)listOfFavoriteArtists
{
    const heimdall_ml_query_params_t queryParams = [self queryParams];
    heimdall_ml_artist_list_t * const p_artist_list = heimdall_ml_list_favorite_artists(_p_mediaLibrary, &queryParams);
    if (p_artist_list == NULL) {
        return @[];
    }
    
    NSMutableArray * const mutableArray = [[NSMutableArray alloc] initWithCapacity:p_artist_list->i_nb_items];
    for (size_t x = 0; x < p_artist_list->i_nb_items; x++) {
        HeimdallMediaLibraryArtist * const artist = [[HeimdallMediaLibraryArtist alloc] initWithArtist:&p_artist_list->p_items[x]];
        if (artist != nil) {
            [mutableArray addObject:artist];
        }
    }
    heimdall_ml_artist_list_release(p_artist_list);
    return mutableArray.copy;
}

- (NSArray<HeimdallMediaLibraryGenre *> *)listOfFavoriteGenres
{
    const heimdall_ml_query_params_t queryParams = [self queryParams];
    heimdall_ml_genre_list_t * const p_genre_list = heimdall_ml_list_favorite_genres(_p_mediaLibrary, &queryParams);
    if (p_genre_list == NULL) {
        return @[];
    }
    
    NSMutableArray * const mutableArray = [[NSMutableArray alloc] initWithCapacity:p_genre_list->i_nb_items];
    for (size_t x = 0; x < p_genre_list->i_nb_items; x++) {
        HeimdallMediaLibraryGenre * const genre = [[HeimdallMediaLibraryGenre alloc] initWithGenre:&p_genre_list->p_items[x]];
        if (genre != nil) {
            [mutableArray addObject:genre];
        }
    }
    heimdall_ml_genre_list_release(p_genre_list);
    return mutableArray.copy;
}

- (size_t)numberOfFavoriteAudioMedia
{
    const heimdall_ml_query_params_t queryParams = [self queryParams];
    return heimdall_ml_count_favorite_audios(_p_mediaLibrary, &queryParams);
}

- (size_t)numberOfFavoriteVideoMedia
{
    const heimdall_ml_query_params_t queryParams = [self queryParams];
    return heimdall_ml_count_favorite_videos(_p_mediaLibrary, &queryParams);
}

- (size_t)numberOfFavoriteAlbums
{
    const heimdall_ml_query_params_t queryParams = [self queryParams];
    return heimdall_ml_count_favorite_albums(_p_mediaLibrary, &queryParams);
}

- (size_t)numberOfFavoriteArtists
{
    const heimdall_ml_query_params_t queryParams = [self queryParams];
    return heimdall_ml_count_favorite_artists(_p_mediaLibrary, &queryParams);
}

- (size_t)numberOfFavoriteGenres
{
    const heimdall_ml_query_params_t queryParams = [self queryParams];
    return heimdall_ml_count_favorite_genres(_p_mediaLibrary, &queryParams);
}

@end
