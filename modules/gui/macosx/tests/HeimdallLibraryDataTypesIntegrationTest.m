/****************************************************************************
 * HeimdallLibraryDataTypesIntegrationTest.m: real medialibrary wrapper tests
 ****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Claudio Cambra <developer@claudiocambra.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any later
 * version.
 *****************************************************************************/

#import <XCTest/XCTest.h>

#include <stdint.h>

#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "tests/HeimdallLibraryDataTypesIntegrationTestSupport.h"
#import "tests/HeimdallLibraryDataTypesTestSupport.h"
#import "tests/HeimdallInputItemTestSupport.h"

@interface HeimdallLibraryDataTypesIntegrationTest : XCTestCase
@end

@implementation HeimdallLibraryDataTypesIntegrationTest

- (NSArray<NSURL *> *)fileURLsForMediaItems:(NSArray<HeimdallMediaLibraryMediaItem *> *)mediaItems
{
    NSMutableArray<NSURL *> * const URLs = NSMutableArray.array;
    for (HeimdallMediaLibraryMediaItem * const item in mediaItems) {
        for (HeimdallMediaLibraryFile * const file in item.files) {
            [URLs addObject:file.fileURL];
        }
    }
    return URLs;
}

- (void)assertMediaItemsAreTrashed:(NSArray<HeimdallMediaLibraryMediaItem *> *)mediaItems
{
    NSArray<NSURL *> * const expectedURLs = [self fileURLsForMediaItems:mediaItems];
    XCTAssertGreaterThan(expectedURLs.count, (NSUInteger)0);
    XCTAssertEqualObjects([NSSet setWithArray:HeimdallLibraryDataTypesTestTrashedSourceURLs()],
                          [NSSet setWithArray:expectedURLs]);
    for (NSURL * const URL in expectedURLs) {
        XCTAssertFalse([[NSFileManager defaultManager] fileExistsAtPath:URL.path]);
    }
    NSArray<NSURL *> * const destinationURLs =
        HeimdallLibraryDataTypesTestTrashDestinationURLs();
    XCTAssertEqual(destinationURLs.count, expectedURLs.count);
    for (NSURL * const URL in destinationURLs) {
        XCTAssertTrue([[NSFileManager defaultManager] fileExistsAtPath:URL.path]);
    }
}

- (HeimdallMediaLibraryMediaItem *)integrationMediaItem
{
    HeimdallMediaLibraryMediaItem * const item =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertNotNil(item);
    return item;
}

- (HeimdallMediaLibraryPlaylist *)integrationPlaylist
{
    const int64_t playlistID = HeimdallLibraryDataTypesIntegrationCreatePlaylist();
    XCTAssertNotEqual(playlistID, (int64_t)0);

    HeimdallMediaLibraryPlaylist * const playlist =
        [HeimdallMediaLibraryPlaylist playlistForLibraryID:playlistID];
    XCTAssertNotNil(playlist);
    return playlist;
}

- (HeimdallMediaLibraryPlaylist *)playlistWithLibraryID:(int64_t)libraryID
                                                MRL:(NSString *)MRL
{
    heimdall_ml_playlist_list_t * const playlists =
        heimdall_ml_list_playlists(HeimdallLibraryDataTypesIntegrationMediaLibrary(),
                              NULL,
                              HEIMDALL_ML_PLAYLIST_TYPE_ALL);
    XCTAssertTrue(playlists != NULL);

    HeimdallMediaLibraryPlaylist *playlist = nil;
    for (size_t index = 0; playlists != NULL && index < playlists->i_nb_items; ++index) {
        if (playlists->p_items[index].i_id != libraryID) {
            continue;
        }

        struct heimdall_ml_playlist_t playlistData = playlists->p_items[index];
        playlistData.psz_mrl = (char *)MRL.UTF8String;
        playlist = [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];
        break;
    }
    if (playlists != NULL) {
        heimdall_ml_playlist_list_release(playlists);
    }
    return playlist;
}

- (HeimdallMediaLibraryMediaItem *)factoryAudioTrack
{
    XCTAssertTrue(HeimdallLibraryDataTypesIntegrationPrepareFactoryFixtures());
    HeimdallMediaLibraryMediaItem * const track =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:
            HeimdallLibraryDataTypesIntegrationFactoryAudioMediaID()];
    XCTAssertNotNil(track);
    return track;
}

- (HeimdallMediaLibraryMediaItem *)factoryVideoEpisode
{
    XCTAssertTrue(HeimdallLibraryDataTypesIntegrationPrepareFactoryFixtures());
    HeimdallMediaLibraryMediaItem * const episode =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:
            HeimdallLibraryDataTypesIntegrationFactoryVideoMediaID()];
    XCTAssertNotNil(episode);
    return episode;
}

- (HeimdallMediaLibraryMovie *)factoryMovie
{
    XCTAssertTrue(HeimdallLibraryDataTypesIntegrationPrepareMovieFixture());
    heimdall_ml_media_t * const media =
        heimdall_ml_get_media(HeimdallLibraryDataTypesIntegrationMediaLibrary(),
                         HeimdallLibraryDataTypesIntegrationFactoryMovieMediaID());
    XCTAssertNotEqual(media, (heimdall_ml_media_t *)NULL);
    if (media == NULL) {
        return nil;
    }

    HeimdallMediaLibraryMovie * const movie =
        [[HeimdallMediaLibraryMovie alloc] initWithMediaItem:media];
    heimdall_ml_media_release(media);
    XCTAssertNotNil(movie);
    return movie;
}

- (HeimdallMediaLibraryAlbum *)factoryAlbum
{
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];
    HeimdallMediaLibraryAlbum * const album =
        [HeimdallMediaLibraryAlbum albumWithID:track.albumID];
    XCTAssertNotNil(album);
    return album;
}

- (HeimdallMediaLibraryGenre *)factoryGenre
{
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];
    HeimdallMediaLibraryGenre * const genre =
        [HeimdallMediaLibraryGenre genreWithID:track.genreID];
    XCTAssertNotNil(genre);
    return genre;
}

- (HeimdallMediaLibraryArtist *)factoryArtist
{
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];
    HeimdallMediaLibraryArtist * const artist = [HeimdallMediaLibraryArtist artistWithID:track.artistID];
    XCTAssertNotNil(artist);
    return artist;
}

- (HeimdallMediaLibraryShow *)factoryShow
{
    XCTAssertTrue(HeimdallLibraryDataTypesIntegrationPrepareFactoryFixtures());
    HeimdallMediaLibraryShow * const show =
        [HeimdallMediaLibraryShow showWithLibraryId:HeimdallLibraryDataTypesIntegrationShowID()];
    XCTAssertNotNil(show);
    return show;
}

- (HeimdallMediaLibraryGroup *)factoryGroup
{
    XCTAssertTrue(HeimdallLibraryDataTypesIntegrationPrepareFactoryFixtures());
    HeimdallMediaLibraryGroup * const group =
        [HeimdallMediaLibraryGroup groupWithID:HeimdallLibraryDataTypesIntegrationGroupID()];
    XCTAssertNotNil(group);
    return group;
}

+ (void)setUp
{
    [super setUp];
    XCTAssertTrue(HeimdallLibraryDataTypesIntegrationStart());
}

+ (void)tearDown
{
    HeimdallLibraryDataTypesIntegrationStop();
    [super tearDown];
}

- (void)tearDown
{
    HeimdallLibraryDataTypesTestResetTrashState();
    [super tearDown];
}

- (void)testMediaItemFactoryResolvesPersistedExternalMedia
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    XCTAssertEqual(item.libraryID, HeimdallLibraryDataTypesIntegrationMediaID());
}

- (void)testMediaItemsFromPasteboardDataRoundTripsPersistedItem
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];
    NSError *archiveError = nil;
    NSData * const data = [NSKeyedArchiver archivedDataWithRootObject:@[ item ]
                                                  requiringSecureCoding:YES
                                                                  error:&archiveError];
    XCTAssertNil(archiveError);
    XCTAssertNotNil(data);

    NSArray<HeimdallMediaLibraryMediaItem *> * const decodedItems =
        [HeimdallMediaLibraryMediaItem mediaItemsFromPasteboardData:data];
    XCTAssertEqual(decodedItems.count, (NSUInteger)1);

    HeimdallMediaLibraryMediaItem * const decodedItem = decodedItems.firstObject;
    XCTAssertNotNil(decodedItem);
    XCTAssertNotEqual(decodedItem, item);
    XCTAssertEqual(decodedItem.libraryID, item.libraryID);
}

- (void)testMediaItemFactoryRejectsUnknownLibraryID
{
    XCTAssertNil([HeimdallMediaLibraryMediaItem mediaItemForLibraryID:INT64_MAX]);
}

- (void)testMediaItemFactoryResolvesPersistedExternalMediaByURL
{
    NSURL * const url = [NSURL URLWithString:@"mock://macosx-datatypes-integration"];
    HeimdallMediaLibraryMediaItem * const item = [HeimdallMediaLibraryMediaItem mediaItemForURL:url];

    XCTAssertEqual(item.libraryID, HeimdallLibraryDataTypesIntegrationMediaID());
}

- (void)testMediaItemFactoryRejectsUnknownURL
{
    NSURL * const url = [NSURL URLWithString:@"mock://macosx-datatypes-unknown"];
    XCTAssertNil([HeimdallMediaLibraryMediaItem mediaItemForURL:url]);
    XCTAssertNil([HeimdallMediaLibraryMediaItem mediaItemForURL:nil]);
}

- (void)testArtistFactoryRejectsUnknownLibraryID
{
    XCTAssertNil([HeimdallMediaLibraryArtist artistWithID:INT64_MAX]);
}

- (void)testArtistFactoryResolvesUnknownArtist
{
    HeimdallMediaLibraryArtist * const artist =
        [HeimdallMediaLibraryArtist artistWithID:1];

    XCTAssertNotNil(artist);
    XCTAssertEqual(artist.libraryID, (int64_t)1);
    XCTAssertEqualObjects(artist.name, @"Unknown Artist");
}

- (void)testArtistFactoryResolvesVariousArtists
{
    HeimdallMediaLibraryArtist * const artist =
        [HeimdallMediaLibraryArtist artistWithID:2];

    XCTAssertNotNil(artist);
    XCTAssertEqual(artist.libraryID, (int64_t)2);
    XCTAssertEqualObjects(artist.name, @"Various Artist");
}

- (void)testAlbumFactoryRejectsUnknownLibraryID
{
    XCTAssertNil([HeimdallMediaLibraryAlbum albumWithID:INT64_MAX]);
}

- (void)testAlbumFactoryResolvesPersistedAlbum
{
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];
    HeimdallMediaLibraryAlbum * const album =
        [HeimdallMediaLibraryAlbum albumWithID:track.albumID];

    XCTAssertEqual(album.libraryID, track.albumID);
    XCTAssertEqualObjects(album.title, @"Factory Album");
}

- (void)testGenreFactoryRejectsUnknownLibraryID
{
    XCTAssertNil([HeimdallMediaLibraryGenre genreWithID:INT64_MAX]);
}

- (void)testGenreFactoryResolvesPersistedGenre
{
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];
    HeimdallMediaLibraryGenre * const genre =
        [HeimdallMediaLibraryGenre genreWithID:track.genreID];

    XCTAssertEqual(genre.libraryID, track.genreID);
    XCTAssertEqualObjects(genre.name, @"Rock");
}

- (void)testShowFactoryRejectsUnknownLibraryID
{
    XCTAssertNil([HeimdallMediaLibraryShow showWithLibraryId:INT64_MAX]);
}

- (void)testShowFactoryResolvesUnknownShow
{
    HeimdallMediaLibraryShow * const show =
        [HeimdallMediaLibraryShow showWithLibraryId:1];

    XCTAssertNotNil(show);
    XCTAssertEqual(show.libraryID, (int64_t)1);
    XCTAssertEqualObjects(show.name, @"");
}

- (void)testShowFactoryResolvesPersistedShow
{
    HeimdallMediaLibraryShow * const show = [self factoryShow];

    XCTAssertEqual(show.libraryID, HeimdallLibraryDataTypesIntegrationShowID());
    XCTAssertEqualObjects(show.name, @"Factory Show");
}

- (void)testGroupFactoryRejectsUnknownLibraryID
{
    XCTAssertNil([HeimdallMediaLibraryGroup groupWithID:INT64_MAX]);
}

- (void)testGroupFactoryResolvesPersistedGroup
{
    HeimdallMediaLibraryGroup * const group = [self factoryGroup];

    XCTAssertEqual(group.libraryID, HeimdallLibraryDataTypesIntegrationGroupID());
    XCTAssertEqualObjects(group.displayString, @"Factory Show S01E01");
}

- (void)testArtistMoveToTrashMovesFetchedMediaItems
{
    HeimdallMediaLibraryArtist * const artist = [self factoryArtist];
    NSArray<HeimdallMediaLibraryMediaItem *> * const mediaItems = artist.mediaItems;

    HeimdallInputItemTestResetAppKitState();
    HeimdallLibraryDataTypesTestResetTrashState();
    [artist moveToTrash];

    [self assertMediaItemsAreTrashed:mediaItems];
    XCTAssertTrue(HeimdallInputItemTestDidReload());
}

- (void)testAlbumMoveToTrashMovesFetchedMediaItems
{
    HeimdallMediaLibraryAlbum * const album = [self factoryAlbum];
    NSArray<HeimdallMediaLibraryMediaItem *> * const mediaItems = album.mediaItems;

    HeimdallInputItemTestResetAppKitState();
    HeimdallLibraryDataTypesTestResetTrashState();
    [album moveToTrash];

    [self assertMediaItemsAreTrashed:mediaItems];
    XCTAssertTrue(HeimdallInputItemTestDidReload());
}

- (void)testGenreMoveToTrashMovesFetchedMediaItems
{
    HeimdallMediaLibraryGenre * const genre = [self factoryGenre];
    NSArray<HeimdallMediaLibraryMediaItem *> * const mediaItems = genre.mediaItems;

    HeimdallInputItemTestResetAppKitState();
    HeimdallLibraryDataTypesTestResetTrashState();
    [genre moveToTrash];

    [self assertMediaItemsAreTrashed:mediaItems];
    XCTAssertTrue(HeimdallInputItemTestDidReload());
}

- (void)testGroupMoveToTrashMovesFetchedMediaItems
{
    HeimdallMediaLibraryGroup * const group = [self factoryGroup];
    NSArray<HeimdallMediaLibraryMediaItem *> * const mediaItems = group.mediaItems;

    HeimdallInputItemTestResetAppKitState();
    HeimdallLibraryDataTypesTestResetTrashState();
    [group moveToTrash];

    [self assertMediaItemsAreTrashed:mediaItems];
    XCTAssertTrue(HeimdallInputItemTestDidReload());
}

- (void)testShowMoveToTrashMovesFetchedEpisodes
{
    HeimdallMediaLibraryShow * const show = [self factoryShow];
    NSArray<HeimdallMediaLibraryMediaItem *> * const mediaItems = show.episodes;

    HeimdallInputItemTestResetAppKitState();
    HeimdallLibraryDataTypesTestResetTrashState();
    [show moveToTrash];

    [self assertMediaItemsAreTrashed:mediaItems];
    XCTAssertTrue(HeimdallInputItemTestDidReload());
}

- (void)testArtistArtistsRelationshipContainsTheArtist
{
    HeimdallMediaLibraryArtist * const artist = [self factoryArtist];

    XCTAssertEqual(artist.artists.count, (NSUInteger)1);
    XCTAssertEqual(artist.artists.firstObject.libraryID, artist.libraryID);
}

- (void)testArtistAlbumsRelationshipContainsTheAlbum
{
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];
    HeimdallMediaLibraryArtist * const artist = [self factoryArtist];

    NSArray<HeimdallMediaLibraryAlbum *> * const albums = artist.albums;
    XCTAssertEqual(albums.count, (NSUInteger)1);
    XCTAssertEqual(albums.firstObject.libraryID, track.albumID);
}

- (void)testArtistGenresRelationshipContainsTheGenre
{
    HeimdallMediaLibraryArtist * const artist = [self factoryArtist];
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];

    NSArray<HeimdallMediaLibraryGenre *> * const genres = artist.genres;
    XCTAssertEqual(genres.count, (NSUInteger)1);
    XCTAssertEqual(genres.firstObject.libraryID, track.genreID);
}

- (void)testArtistMediaItemsRelationshipContainsTheAlbumTrack
{
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];
    HeimdallMediaLibraryArtist * const artist = [self factoryArtist];

    NSArray<HeimdallMediaLibraryMediaItem *> * const mediaItems = artist.mediaItems;
    XCTAssertEqual(mediaItems.count, (NSUInteger)1);
    XCTAssertEqual(mediaItems.firstObject.libraryID, track.libraryID);
    XCTAssertEqual(mediaItems.firstObject.artistID, artist.libraryID);
}

- (void)testArtistSecondaryActionableDetailResolvesItsGenre
{
    HeimdallMediaLibraryArtist * const artist = [self factoryArtist];
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];

    XCTAssertEqual(artist.secondaryActionableDetailLibraryItem.libraryID,
                   track.genreID);
}

- (void)testArtistEnumerationTraversesAlbumTracks
{
    HeimdallMediaLibraryArtist * const artist = [self factoryArtist];
    __block NSUInteger enumeratedCount = 0;
    __block int64_t enumeratedID = 0;

    [artist enumerateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem * const item,
                                           BOOL * const __unused stop) {
        enumeratedCount++;
        enumeratedID = item.libraryID;
    }];

    XCTAssertEqual(enumeratedCount, (NSUInteger)1);
    XCTAssertNotEqual(enumeratedID, (int64_t)0);
}

- (void)testArtistIterationTraversesAlbumTracks
{
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];
    HeimdallMediaLibraryArtist * const artist = [self factoryArtist];
    __block NSUInteger iteratedCount = 0;

    [artist iterateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem * const item) {
        XCTAssertEqual(item.libraryID, track.libraryID);
        iteratedCount++;
    }];

    XCTAssertEqual(iteratedCount, (NSUInteger)1);
}

- (void)testAlbumArtistsRelationshipResolvesThePersistedArtist
{
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];
    HeimdallMediaLibraryAlbum * const album = [self factoryAlbum];

    NSArray<HeimdallMediaLibraryArtist *> * const artists = album.artists;
    XCTAssertEqual(artists.count, (NSUInteger)1);
    XCTAssertEqual(artists.firstObject.libraryID, track.artistID);
}

- (void)testAlbumAlbumsRelationshipContainsTheAlbum
{
    HeimdallMediaLibraryAlbum * const album = [self factoryAlbum];

    XCTAssertEqual(album.albums.count, (NSUInteger)1);
    XCTAssertEqual(album.albums.firstObject.libraryID, album.libraryID);
}

- (void)testAlbumGenresRelationshipContainsThePersistedGenre
{
    HeimdallMediaLibraryAlbum * const album = [self factoryAlbum];
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];

    NSArray<HeimdallMediaLibraryGenre *> * const genres = album.genres;
    XCTAssertEqual(genres.count, (NSUInteger)1);
    XCTAssertEqual(genres.firstObject.libraryID, track.genreID);
}

- (void)testAlbumMediaItemsRelationshipContainsTheAlbumTrack
{
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];
    HeimdallMediaLibraryAlbum * const album = [self factoryAlbum];

    NSArray<HeimdallMediaLibraryMediaItem *> * const mediaItems = album.mediaItems;
    XCTAssertEqual(mediaItems.count, (NSUInteger)1);
    XCTAssertEqual(mediaItems.firstObject.libraryID, track.libraryID);
    XCTAssertEqual(mediaItems.firstObject.albumID, album.libraryID);
}

- (void)testAlbumPrimaryActionableDetailResolvesItsArtist
{
    HeimdallMediaLibraryAlbum * const album = [self factoryAlbum];

    XCTAssertEqual(album.primaryActionableDetailLibraryItem.libraryID, album.artistID);
}

- (void)testAlbumSecondaryActionableDetailResolvesItsGenre
{
    HeimdallMediaLibraryAlbum * const album = [self factoryAlbum];
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];

    XCTAssertEqual(album.secondaryActionableDetailLibraryItem.libraryID,
                   track.genreID);
}

- (void)testGenreArtistsRelationshipContainsThePersistedArtist
{
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];
    HeimdallMediaLibraryGenre * const genre = [self factoryGenre];

    NSArray<HeimdallMediaLibraryArtist *> * const artists = genre.artists;
    XCTAssertEqual(artists.count, (NSUInteger)1);
    XCTAssertEqual(artists.firstObject.libraryID, track.artistID);
}

- (void)testGenreAlbumsRelationshipContainsThePersistedAlbum
{
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];
    HeimdallMediaLibraryGenre * const genre = [self factoryGenre];

    NSArray<HeimdallMediaLibraryAlbum *> * const albums = genre.albums;
    XCTAssertEqual(albums.count, (NSUInteger)1);
    XCTAssertEqual(albums.firstObject.libraryID, track.albumID);
}

- (void)testGenreGenresRelationshipContainsTheGenre
{
    HeimdallMediaLibraryGenre * const genre = [self factoryGenre];

    XCTAssertEqual(genre.genres.count, (NSUInteger)1);
    XCTAssertEqual(genre.genres.firstObject.libraryID, genre.libraryID);
}

- (void)testGenreMediaItemsRelationshipContainsTheAlbumTrack
{
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];
    HeimdallMediaLibraryGenre * const genre = [self factoryGenre];

    NSArray<HeimdallMediaLibraryMediaItem *> * const mediaItems = genre.mediaItems;
    XCTAssertEqual(mediaItems.count, (NSUInteger)1);
    XCTAssertEqual(mediaItems.firstObject.libraryID, track.libraryID);
    XCTAssertEqual(mediaItems.firstObject.genreID, genre.libraryID);
}

- (void)testShowEpisodesRelationshipContainsThePersistedEpisode
{
    HeimdallMediaLibraryMediaItem * const episode = [self factoryVideoEpisode];
    HeimdallMediaLibraryShow * const show = [self factoryShow];

    NSArray<HeimdallMediaLibraryMediaItem *> * const episodes = show.episodes;
    XCTAssertEqual(episodes.count, (NSUInteger)1);
    XCTAssertEqual(episodes.firstObject.libraryID, episode.libraryID);
    XCTAssertEqual(episodes.firstObject.showEpisode.seasonNumber,
                   episode.showEpisode.seasonNumber);
    XCTAssertEqual(episodes.firstObject.showEpisode.episodeNumber,
                   episode.showEpisode.episodeNumber);
}

- (void)testShowMediaItemsRelationshipAliasesEpisodes
{
    HeimdallMediaLibraryShow * const show = [self factoryShow];

    XCTAssertEqual(show.mediaItems.count, show.episodes.count);
    XCTAssertEqual(show.mediaItems.firstObject.libraryID, show.episodes.firstObject.libraryID);
}

- (void)testGroupMediaItemsRelationshipContainsThePersistedEpisode
{
    HeimdallMediaLibraryMediaItem * const episode = [self factoryVideoEpisode];
    HeimdallMediaLibraryGroup * const group = [self factoryGroup];

    NSArray<HeimdallMediaLibraryMediaItem *> * const mediaItems = group.mediaItems;
    XCTAssertEqual(mediaItems.count, (NSUInteger)1);
    XCTAssertEqual(mediaItems.firstObject.libraryID, episode.libraryID);
}

- (void)testGroupFirstMediaItemMatchesItsMediaItemsRelationship
{
    HeimdallMediaLibraryGroup * const group = [self factoryGroup];

    XCTAssertEqual(group.firstMediaItem.libraryID, group.mediaItems.firstObject.libraryID);
}

- (void)testMovieMediaItemsRelationshipContainsThePersistedMovie
{
    HeimdallMediaLibraryMovie * const movie = [self factoryMovie];
    NSArray<HeimdallMediaLibraryMediaItem *> * const mediaItems = movie.mediaItems;

    XCTAssertEqual(mediaItems.count, (NSUInteger)1);
    HeimdallMediaLibraryMediaItem * const mediaItem = mediaItems.firstObject;
    XCTAssertEqual(mediaItem.libraryID, movie.libraryID);

    NSURL * const movieURL = [NSURL URLWithString:mediaItem.inputItem.MRL];
    XCTAssertEqualObjects(movieURL.scheme, @"file");
    XCTAssertEqualObjects(movieURL.lastPathComponent, @"Factory Movie.mp4");
    XCTAssertTrue([[NSFileManager defaultManager] fileExistsAtPath:movieURL.path]);
}

- (void)testMovieFirstMediaItemMatchesItsMediaItemsRelationship
{
    HeimdallMediaLibraryMovie * const movie = [self factoryMovie];

    XCTAssertEqual(movie.firstMediaItem.libraryID, movie.mediaItems.firstObject.libraryID);
}

- (void)testMovieMediaItemHasMovieSubtypeAndEntity
{
    HeimdallMediaLibraryMovie * const movie = [self factoryMovie];
    HeimdallMediaLibraryMediaItem * const mediaItem = movie.mediaItems.firstObject;

    XCTAssertEqual(mediaItem.mediaSubType, HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE);
    XCTAssertNotNil(mediaItem.movie);
    XCTAssertEqual(mediaItem.movie.libraryID, movie.libraryID);
}

- (void)testMovieMapsPersistedSummaryAndIMDbID
{
    HeimdallMediaLibraryMovie * const movie = [self factoryMovie];

    XCTAssertEqualObjects(movie.summary, @"Factory movie summary");
    XCTAssertEqualObjects(movie.imdbID, @"tt1234567");
}

- (void)testMovieFavoritePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryMovie * movie = [self factoryMovie];
    const int64_t movieID = movie.libraryID;

    XCTAssertEqual([movie setFavorite:YES], HEIMDALL_SUCCESS);
    movie = [self factoryMovie];
    XCTAssertEqual(movie.libraryID, movieID);
    XCTAssertTrue(movie.favorited);

    XCTAssertEqual([movie toggleFavorite], HEIMDALL_SUCCESS);
    movie = [self factoryMovie];
    XCTAssertFalse(movie.favorited);
}

- (void)testMovieRevealInFinderUsesItsMediaItem
{
    HeimdallMediaLibraryMovie * const movie = [self factoryMovie];

    HeimdallInputItemTestResetAppKitState();
    [movie revealInFinder];

    XCTAssertTrue(HeimdallInputItemTestDidReveal());
}

- (void)testMovieMoveToTrashMovesItsMediaItem
{
    HeimdallMediaLibraryMovie * const movie = [self factoryMovie];
    NSArray<HeimdallMediaLibraryMediaItem *> * const mediaItems = movie.mediaItems;

    HeimdallInputItemTestResetAppKitState();
    HeimdallLibraryDataTypesTestResetTrashState();
    [movie moveToTrash];

    [self assertMediaItemsAreTrashed:mediaItems];
    XCTAssertTrue(HeimdallInputItemTestDidReload());
}

- (void)testShowEpisodePrimaryDetailUsesShowName
{
    HeimdallMediaLibraryMediaItem * const episode = [self factoryVideoEpisode];

    XCTAssertEqualObjects(episode.primaryDetailString, @"Factory Show");
}

- (void)testShowEpisodeInputItemContainsShowName
{
    HeimdallMediaLibraryMediaItem * const episode = [self factoryVideoEpisode];

    XCTAssertEqualObjects(episode.inputItem.showName, @"Factory Show");
}

- (void)testAlbumTrackSecondaryDetailUsesGenre
{
    HeimdallMediaLibraryMediaItem * const track = [self factoryAudioTrack];

    XCTAssertEqualObjects(track.secondaryDetailString, @"Rock");
}

- (void)testArtistSecondaryDetailUsesGenre
{
    HeimdallMediaLibraryArtist * const artist = [self factoryArtist];

    XCTAssertEqualObjects(artist.secondaryDetailString, @"Rock");
}

- (void)testAlbumSecondaryDetailUsesGenre
{
    HeimdallMediaLibraryAlbum * const album = [self factoryAlbum];

    XCTAssertEqualObjects(album.secondaryDetailString, @"Rock");
}

- (void)testPlaylistFactoryResolvesPersistedPlaylist
{
    HeimdallMediaLibraryPlaylist * const playlist = [self integrationPlaylist];
    const int64_t playlistID = playlist.libraryID;

    HeimdallMediaLibraryPlaylist * const refreshedPlaylist =
        [HeimdallMediaLibraryPlaylist playlistForLibraryID:playlistID];
    XCTAssertNotNil(refreshedPlaylist);
    XCTAssertEqual(refreshedPlaylist.libraryID, playlistID);

    heimdall_ml_playlist_delete(HeimdallLibraryDataTypesIntegrationMediaLibrary(), playlistID);
}

- (void)testPlaylistFactoryRejectsUnknownLibraryID
{
    XCTAssertNil([HeimdallMediaLibraryPlaylist playlistForLibraryID:INT64_MAX]);
}

- (void)testMediaItemExposesPersistedInputItem
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    HeimdallInputItem * const inputItem = item.inputItem;

    XCTAssertNotNil(inputItem);
    XCTAssertEqualObjects(inputItem.MRL, @"mock://macosx-datatypes-integration");
}

- (void)testMediaItemInitializerMapsPersistedMedia
{
    heimdall_ml_media_t * const media =
        heimdall_ml_get_media(HeimdallLibraryDataTypesIntegrationMediaLibrary(),
                         HeimdallLibraryDataTypesIntegrationMediaID());
    XCTAssertNotEqual(media, (heimdall_ml_media_t *)NULL);
    if (media == NULL) {
        return;
    }

    HeimdallMediaLibraryMediaItem * const item =
        [[HeimdallMediaLibraryMediaItem alloc] initWithMediaItem:media];
    heimdall_ml_media_release(media);

    XCTAssertNotNil(item);
    XCTAssertEqual(item.libraryID, HeimdallLibraryDataTypesIntegrationMediaID());
    XCTAssertEqualObjects(item.inputItem.MRL, @"mock://macosx-datatypes-integration");
}

- (void)testStreamURLInitializerCreatesPersistedStream
{
    NSURL * const url = [NSURL URLWithString:@"mock://macosx-datatypes-stream"];
    HeimdallMediaLibraryMediaItem * const item = [[HeimdallMediaLibraryMediaItem alloc] initWithStreamURL:url];

    XCTAssertNotNil(item);
    XCTAssertGreaterThan(item.libraryID, (int64_t)0);

    HeimdallMediaLibraryMediaItem * const refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:item.libraryID];
    XCTAssertNotNil(refreshedItem);
    XCTAssertEqualObjects(refreshedItem.inputItem.MRL, url.absoluteString);
    XCTAssertEqual(heimdall_ml_remove_stream(HeimdallLibraryDataTypesIntegrationMediaLibrary(), item.libraryID), HEIMDALL_SUCCESS);
}

- (void)testStreamURLInitializerRejectsNilURL
{
    id const nilURL = nil;

    XCTAssertNil([[HeimdallMediaLibraryMediaItem alloc] initWithStreamURL:nilURL]);
}

- (void)testExternalURLInitializerCreatesPersistedMediaItem
{
    NSURL * const url = [NSURL URLWithString:@"mock://macosx-datatypes-external-url"];
    HeimdallMediaLibraryMediaItem * const item = [[HeimdallMediaLibraryMediaItem alloc] initWithExternalURL:url];

    XCTAssertNotNil(item);
    XCTAssertGreaterThan(item.libraryID, (int64_t)0);

    HeimdallMediaLibraryMediaItem * const refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:item.libraryID];
    XCTAssertNotNil(refreshedItem);
    XCTAssertNotNil(refreshedItem.inputItem);
    XCTAssertEqualObjects(refreshedItem.inputItem.MRL, url.absoluteString);

    XCTAssertEqual(heimdall_ml_remove_stream(HeimdallLibraryDataTypesIntegrationMediaLibrary(), item.libraryID), HEIMDALL_SUCCESS);
    XCTAssertNil([HeimdallMediaLibraryMediaItem mediaItemForLibraryID:item.libraryID]);
}

- (void)testExternalURLInitializerRejectsNilURL
{
    id const nilURL = nil;

    XCTAssertNil([[HeimdallMediaLibraryMediaItem alloc] initWithExternalURL:nilURL]);
}

- (void)testMediaItemRatingRoundTripsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    item.rating = 1;
    XCTAssertEqual(item.rating, 1);

    HeimdallMediaLibraryMediaItem *refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertNotNil(refreshedItem);
    XCTAssertEqual(refreshedItem.rating, 1);

    item.rating = 4;
    XCTAssertEqual(item.rating, 4);

    refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.rating, 4);
}

- (void)testMediaItemRatingPersistsIndependentlyPerMediaItem
{
    const int64_t secondID =
        HeimdallLibraryDataTypesIntegrationCreateExternalMedia("mock://macosx-datatypes-rating-isolation");
    XCTAssertNotEqual(secondID, (int64_t)0);

    HeimdallMediaLibraryMediaItem * const first = [self integrationMediaItem];
    HeimdallMediaLibraryMediaItem * const second =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:secondID];
    XCTAssertNotNil(second);

    first.rating = 2;
    second.rating = 5;

    HeimdallMediaLibraryMediaItem * const refreshedFirst =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:first.libraryID];
    HeimdallMediaLibraryMediaItem * const refreshedSecond =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:second.libraryID];
    XCTAssertEqual(refreshedFirst.rating, 2);
    XCTAssertEqual(refreshedSecond.rating, 5);

    heimdall_ml_remove_stream(HeimdallLibraryDataTypesIntegrationMediaLibrary(), secondID);
}

- (void)testMediaItemFavoritePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    XCTAssertEqual([item setFavorite:YES], HEIMDALL_SUCCESS);

    HeimdallMediaLibraryMediaItem * refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertNotNil(refreshedItem);
    XCTAssertTrue(refreshedItem.favorited);

    XCTAssertEqual([refreshedItem toggleFavorite], HEIMDALL_SUCCESS);
    refreshedItem = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:
                     HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertNotNil(refreshedItem);
    XCTAssertFalse(refreshedItem.favorited);

    XCTAssertEqual([refreshedItem toggleFavorite], HEIMDALL_SUCCESS);
    refreshedItem = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:
                     HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertNotNil(refreshedItem);
    XCTAssertTrue(refreshedItem.favorited);
}

- (void)testArtistToggleFavoritePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryArtist * const artist = [self factoryArtist];
    const int64_t artistID = artist.libraryID;

    XCTAssertEqual([artist setFavorite:YES], HEIMDALL_SUCCESS);
    HeimdallMediaLibraryArtist * refreshedArtist = [HeimdallMediaLibraryArtist artistWithID:artistID];
    XCTAssertTrue(refreshedArtist.favorited);

    XCTAssertEqual([refreshedArtist toggleFavorite], HEIMDALL_SUCCESS);
    refreshedArtist = [HeimdallMediaLibraryArtist artistWithID:artistID];
    XCTAssertFalse(refreshedArtist.favorited);

    XCTAssertEqual([refreshedArtist toggleFavorite], HEIMDALL_SUCCESS);
    refreshedArtist = [HeimdallMediaLibraryArtist artistWithID:artistID];
    XCTAssertTrue(refreshedArtist.favorited);
}

- (void)testAlbumToggleFavoritePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryAlbum * const album = [self factoryAlbum];
    const int64_t albumID = album.libraryID;

    XCTAssertEqual([album setFavorite:YES], HEIMDALL_SUCCESS);
    HeimdallMediaLibraryAlbum * refreshedAlbum = [HeimdallMediaLibraryAlbum albumWithID:albumID];
    XCTAssertTrue(refreshedAlbum.favorited);

    XCTAssertEqual([refreshedAlbum toggleFavorite], HEIMDALL_SUCCESS);
    refreshedAlbum = [HeimdallMediaLibraryAlbum albumWithID:albumID];
    XCTAssertFalse(refreshedAlbum.favorited);

    XCTAssertEqual([refreshedAlbum toggleFavorite], HEIMDALL_SUCCESS);
    refreshedAlbum = [HeimdallMediaLibraryAlbum albumWithID:albumID];
    XCTAssertTrue(refreshedAlbum.favorited);
}

- (void)testGenreToggleFavoritePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryGenre * const genre = [self factoryGenre];
    const int64_t genreID = genre.libraryID;

    XCTAssertEqual([genre setFavorite:YES], HEIMDALL_SUCCESS);
    HeimdallMediaLibraryGenre * refreshedGenre = [HeimdallMediaLibraryGenre genreWithID:genreID];
    XCTAssertTrue(refreshedGenre.favorited);

    XCTAssertEqual([refreshedGenre toggleFavorite], HEIMDALL_SUCCESS);
    refreshedGenre = [HeimdallMediaLibraryGenre genreWithID:genreID];
    XCTAssertFalse(refreshedGenre.favorited);

    XCTAssertEqual([refreshedGenre toggleFavorite], HEIMDALL_SUCCESS);
    refreshedGenre = [HeimdallMediaLibraryGenre genreWithID:genreID];
    XCTAssertTrue(refreshedGenre.favorited);
}

- (void)testShowToggleFavoritePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryShow * const show = [self factoryShow];
    const int64_t showID = show.libraryID;

    XCTAssertEqual([show setFavorite:YES], HEIMDALL_SUCCESS);
    HeimdallMediaLibraryShow * refreshedShow =
        [HeimdallMediaLibraryShow showWithLibraryId:showID];
    XCTAssertNotNil(refreshedShow);
    XCTAssertTrue(refreshedShow.favorited);

    XCTAssertEqual([refreshedShow toggleFavorite], HEIMDALL_SUCCESS);
    refreshedShow = [HeimdallMediaLibraryShow showWithLibraryId:showID];
    XCTAssertNotNil(refreshedShow);
    XCTAssertFalse(refreshedShow.favorited);

    XCTAssertEqual([refreshedShow toggleFavorite], HEIMDALL_SUCCESS);
    refreshedShow = [HeimdallMediaLibraryShow showWithLibraryId:showID];
    XCTAssertNotNil(refreshedShow);
    XCTAssertTrue(refreshedShow.favorited);
}

- (void)testGroupToggleFavoritePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryGroup * const group = [self factoryGroup];
    const int64_t groupID = group.libraryID;

    XCTAssertEqual([group setFavorite:YES], HEIMDALL_SUCCESS);
    HeimdallMediaLibraryGroup * refreshedGroup =
        [HeimdallMediaLibraryGroup groupWithID:groupID];
    XCTAssertNotNil(refreshedGroup);
    XCTAssertTrue(refreshedGroup.favorited);

    XCTAssertEqual([refreshedGroup toggleFavorite], HEIMDALL_SUCCESS);
    refreshedGroup = [HeimdallMediaLibraryGroup groupWithID:groupID];
    XCTAssertNotNil(refreshedGroup);
    XCTAssertFalse(refreshedGroup.favorited);

    XCTAssertEqual([refreshedGroup toggleFavorite], HEIMDALL_SUCCESS);
    refreshedGroup = [HeimdallMediaLibraryGroup groupWithID:groupID];
    XCTAssertNotNil(refreshedGroup);
    XCTAssertTrue(refreshedGroup.favorited);
}

- (void)testMediaItemPlaybackRateRoundTripsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    item.lastPlaybackRate = 0.75f;
    XCTAssertEqualWithAccuracy(item.lastPlaybackRate, 0.75f, 0.001f);

    HeimdallMediaLibraryMediaItem *refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertNotNil(refreshedItem);
    XCTAssertEqualWithAccuracy(refreshedItem.lastPlaybackRate, 0.75f, 0.001f);

    item.lastPlaybackRate = 1.25f;
    XCTAssertEqualWithAccuracy(item.lastPlaybackRate, 1.25f, 0.001f);

    refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqualWithAccuracy(refreshedItem.lastPlaybackRate, 1.25f, 0.001f);
}

- (void)testMediaItemStringPlaybackPreferenceRoundTripsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    item.lastAspectRatio = @"4:3";
    XCTAssertEqualObjects(item.lastAspectRatio, @"4:3");

    HeimdallMediaLibraryMediaItem *refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertNotNil(refreshedItem);
    XCTAssertEqualObjects(refreshedItem.lastAspectRatio, @"4:3");

    item.lastAspectRatio = @"16:9";
    XCTAssertEqualObjects(item.lastAspectRatio, @"16:9");

    refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqualObjects(refreshedItem.lastAspectRatio, @"16:9");
}

- (void)testMediaItemTitlePreferencePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    item.lastTitle = 3;
    XCTAssertEqual(item.lastTitle, 3);
    HeimdallMediaLibraryMediaItem *refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.lastTitle, 3);

    item.lastTitle = 8;
    XCTAssertEqual(item.lastTitle, 8);
    refreshedItem = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.lastTitle, 8);
}

- (void)testMediaItemChapterPreferencePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    item.lastChapter = 7;
    XCTAssertEqual(item.lastChapter, 7);
    HeimdallMediaLibraryMediaItem *refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.lastChapter, 7);

    item.lastChapter = 11;
    XCTAssertEqual(item.lastChapter, 11);
    refreshedItem = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.lastChapter, 11);
}

- (void)testMediaItemProgramPreferencePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    item.lastProgram = 2;
    XCTAssertEqual(item.lastProgram, 2);
    HeimdallMediaLibraryMediaItem *refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.lastProgram, 2);

    item.lastProgram = 5;
    XCTAssertEqual(item.lastProgram, 5);
    refreshedItem = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.lastProgram, 5);
}

- (void)testMediaItemVideoTrackPreferencePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    item.lastVideoTrack = 4;
    XCTAssertEqual(item.lastVideoTrack, 4);
    HeimdallMediaLibraryMediaItem *refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.lastVideoTrack, 4);

    item.lastVideoTrack = 9;
    XCTAssertEqual(item.lastVideoTrack, 9);
    refreshedItem = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.lastVideoTrack, 9);
}

- (void)testMediaItemZoomPreferencePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    item.lastZoom = @"1.5";
    XCTAssertEqualObjects(item.lastZoom, @"1.5");
    HeimdallMediaLibraryMediaItem *refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqualObjects(refreshedItem.lastZoom, @"1.5");

    item.lastZoom = @"2.0";
    XCTAssertEqualObjects(item.lastZoom, @"2.0");
    refreshedItem = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqualObjects(refreshedItem.lastZoom, @"2.0");
}

- (void)testMediaItemCropPreferencePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    item.lastCrop = @"16:9";
    XCTAssertEqualObjects(item.lastCrop, @"16:9");
    HeimdallMediaLibraryMediaItem *refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqualObjects(refreshedItem.lastCrop, @"16:9");

    item.lastCrop = @"4:3";
    XCTAssertEqualObjects(item.lastCrop, @"4:3");
    refreshedItem = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqualObjects(refreshedItem.lastCrop, @"4:3");
}

- (void)testMediaItemDeinterlacePreferencePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    item.lastDeinterlaceFilter = @"yadif";
    XCTAssertEqualObjects(item.lastDeinterlaceFilter, @"yadif");
    HeimdallMediaLibraryMediaItem *refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqualObjects(refreshedItem.lastDeinterlaceFilter, @"yadif");

    item.lastDeinterlaceFilter = @"bwdif";
    XCTAssertEqualObjects(item.lastDeinterlaceFilter, @"bwdif");
    refreshedItem = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqualObjects(refreshedItem.lastDeinterlaceFilter, @"bwdif");
}

- (void)testMediaItemVideoFiltersPreferencePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    item.lastVideoFilters = @"sepia";
    XCTAssertEqualObjects(item.lastVideoFilters, @"sepia");
    HeimdallMediaLibraryMediaItem *refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqualObjects(refreshedItem.lastVideoFilters, @"sepia");

    item.lastVideoFilters = @"grayscale";
    XCTAssertEqualObjects(item.lastVideoFilters, @"grayscale");
    refreshedItem = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqualObjects(refreshedItem.lastVideoFilters, @"grayscale");
}

- (void)testMediaItemAudioTrackPreferencePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    item.lastAudioTrack = 5;
    XCTAssertEqual(item.lastAudioTrack, 5);
    HeimdallMediaLibraryMediaItem *refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.lastAudioTrack, 5);

    item.lastAudioTrack = 8;
    XCTAssertEqual(item.lastAudioTrack, 8);
    refreshedItem = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.lastAudioTrack, 8);
}

- (void)testMediaItemGainPreferencePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    item.lastGain = 1.25f;
    XCTAssertEqualWithAccuracy(item.lastGain, 1.25f, 0.001f);
    HeimdallMediaLibraryMediaItem *refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqualWithAccuracy(refreshedItem.lastGain, 1.25f, 0.001f);

    item.lastGain = 0.75f;
    XCTAssertEqualWithAccuracy(item.lastGain, 0.75f, 0.001f);
    refreshedItem = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqualWithAccuracy(refreshedItem.lastGain, 0.75f, 0.001f);
}

- (void)testMediaItemAudioDelayPreferencePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    item.lastAudioDelay = 120;
    XCTAssertEqual(item.lastAudioDelay, 120);
    HeimdallMediaLibraryMediaItem *refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.lastAudioDelay, 120);

    item.lastAudioDelay = -240;
    XCTAssertEqual(item.lastAudioDelay, -240);
    refreshedItem = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.lastAudioDelay, -240);
}

- (void)testMediaItemSubtitleTrackPreferencePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    item.lastSubtitleTrack = 6;
    XCTAssertEqual(item.lastSubtitleTrack, 6);
    HeimdallMediaLibraryMediaItem *refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.lastSubtitleTrack, 6);

    item.lastSubtitleTrack = 10;
    XCTAssertEqual(item.lastSubtitleTrack, 10);
    refreshedItem = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.lastSubtitleTrack, 10);
}

- (void)testMediaItemSubtitleDelayPreferencePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    item.lastSubtitleDelay = -80;
    XCTAssertEqual(item.lastSubtitleDelay, -80);
    HeimdallMediaLibraryMediaItem *refreshedItem =
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.lastSubtitleDelay, -80);

    item.lastSubtitleDelay = 160;
    XCTAssertEqual(item.lastSubtitleDelay, 160);
    refreshedItem = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:HeimdallLibraryDataTypesIntegrationMediaID()];
    XCTAssertEqual(refreshedItem.lastSubtitleDelay, 160);
}

- (void)testPlaylistAppendMedia
{
    HeimdallMediaLibraryPlaylist * const playlist = [self integrationPlaylist];
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];
    XCTAssertTrue([playlist appendMediaItems:@[item]]);
    XCTAssertEqual(playlist.mediaItems.count, (NSUInteger)1);

    heimdall_ml_playlist_delete(HeimdallLibraryDataTypesIntegrationMediaLibrary(), playlist.libraryID);
}

- (void)testPlaylistAppendPersistsMediaCount
{
    HeimdallMediaLibraryPlaylist * const playlist = [self integrationPlaylist];
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];

    XCTAssertTrue([playlist appendMediaItems:@[item]]);
    HeimdallMediaLibraryPlaylist * const refreshedPlaylist =
        [HeimdallMediaLibraryPlaylist playlistForLibraryID:playlist.libraryID];
    XCTAssertEqual(refreshedPlaylist.numberOfMedia, (unsigned int)1);
    XCTAssertEqual(refreshedPlaylist.mediaItems.firstObject.libraryID, item.libraryID);

    heimdall_ml_playlist_delete(HeimdallLibraryDataTypesIntegrationMediaLibrary(), playlist.libraryID);
}

- (void)testPlaylistAppendPreservesMediaOrder
{
    heimdall_ml_media_t * const secondMedia =
        heimdall_ml_new_external_media(HeimdallLibraryDataTypesIntegrationMediaLibrary(),
                                  "mock://macosx-datatypes-second-item");
    HeimdallMediaLibraryPlaylist * const playlist = [self integrationPlaylist];
    HeimdallMediaLibraryMediaItem * const firstItem = [self integrationMediaItem];
    HeimdallMediaLibraryMediaItem * const secondItem = secondMedia == NULL ? nil :
        [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:secondMedia->i_id];
    if (secondMedia != NULL) {
        heimdall_ml_media_release(secondMedia);
    }

    NSArray<HeimdallMediaLibraryMediaItem *> * const items = @[firstItem, secondItem];
    XCTAssertTrue([playlist appendMediaItems:items]);
    HeimdallMediaLibraryPlaylist * const refreshedPlaylist =
        [HeimdallMediaLibraryPlaylist playlistForLibraryID:playlist.libraryID];
    XCTAssertEqual(refreshedPlaylist.mediaItems.count, (NSUInteger)2);
    XCTAssertEqual(refreshedPlaylist.mediaItems[0].libraryID, firstItem.libraryID);
    XCTAssertEqual(refreshedPlaylist.mediaItems[1].libraryID, secondItem.libraryID);

    heimdall_ml_playlist_delete(HeimdallLibraryDataTypesIntegrationMediaLibrary(), playlist.libraryID);
}

- (void)testPlaylistAppendInvalidatesCachedMediaItems
{
    HeimdallMediaLibraryPlaylist * const playlist = [self integrationPlaylist];
    XCTAssertEqual(playlist.mediaItems.count, (NSUInteger)0);

    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];
    XCTAssertTrue([playlist appendMediaItems:@[item]]);

    XCTAssertEqual(playlist.mediaItems.count, (NSUInteger)1);
    XCTAssertEqual(playlist.mediaItems.firstObject.libraryID, item.libraryID);

    heimdall_ml_playlist_delete(HeimdallLibraryDataTypesIntegrationMediaLibrary(), playlist.libraryID);
}

- (void)testPlaylistRenamePersistsThroughMediaLibrary
{
    HeimdallMediaLibraryPlaylist *playlist = [self integrationPlaylist];

    XCTAssertTrue([playlist renameTo:@"Renamed Integration Playlist"]);
    playlist = [HeimdallMediaLibraryPlaylist playlistForLibraryID:playlist.libraryID];
    XCTAssertEqualObjects(playlist.displayString, @"Renamed Integration Playlist");

    heimdall_ml_playlist_delete(HeimdallLibraryDataTypesIntegrationMediaLibrary(), playlist.libraryID);
}

- (void)testPlaylistRemoveMediaInvalidatesCachedMediaItems
{
    HeimdallMediaLibraryPlaylist * const playlist = [self integrationPlaylist];
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];
    XCTAssertTrue([playlist appendMediaItems:@[item]]);
    XCTAssertEqual(playlist.mediaItems.count, (NSUInteger)1);

    [playlist removeMediaItemsAtPositions:@[@0]];
    XCTAssertEqual(playlist.mediaItems.count, (NSUInteger)0);

    HeimdallMediaLibraryPlaylist * const refreshedPlaylist =
        [HeimdallMediaLibraryPlaylist playlistForLibraryID:playlist.libraryID];
    XCTAssertEqual(refreshedPlaylist.mediaItems.count, (NSUInteger)0);

    heimdall_ml_playlist_delete(HeimdallLibraryDataTypesIntegrationMediaLibrary(), playlist.libraryID);
}

- (void)testPlaylistRemovalWithNoPositionsLeavesMediaUnchanged
{
    HeimdallMediaLibraryPlaylist * const playlist = [self integrationPlaylist];
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];
    XCTAssertTrue([playlist appendMediaItems:@[item]]);
    XCTAssertEqual(playlist.mediaItems.count, (NSUInteger)1);

    [playlist removeMediaItemsAtPositions:@[]];

    XCTAssertEqual(playlist.mediaItems.count, (NSUInteger)1);
    XCTAssertEqual(playlist.mediaItems.firstObject.libraryID, item.libraryID);
    heimdall_ml_playlist_delete(HeimdallLibraryDataTypesIntegrationMediaLibrary(), playlist.libraryID);
}

- (void)testPlaylistRemovalSupportsUnsortedNonContiguousPositions
{
    const int64_t secondID =
        HeimdallLibraryDataTypesIntegrationCreateExternalMedia("mock://macosx-datatypes-removal-2");
    const int64_t thirdID =
        HeimdallLibraryDataTypesIntegrationCreateExternalMedia("mock://macosx-datatypes-removal-3");
    const int64_t fourthID =
        HeimdallLibraryDataTypesIntegrationCreateExternalMedia("mock://macosx-datatypes-removal-4");
    XCTAssertNotEqual(secondID, (int64_t)0);
    XCTAssertNotEqual(thirdID, (int64_t)0);
    XCTAssertNotEqual(fourthID, (int64_t)0);

    HeimdallMediaLibraryPlaylist * const playlist = [self integrationPlaylist];
    HeimdallMediaLibraryMediaItem * const first = [self integrationMediaItem];
    HeimdallMediaLibraryMediaItem * const second = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:secondID];
    HeimdallMediaLibraryMediaItem * const third = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:thirdID];
    HeimdallMediaLibraryMediaItem * const fourth = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:fourthID];
    NSArray<HeimdallMediaLibraryMediaItem *> * const items = @[first, second, third, fourth];
    XCTAssertTrue([playlist appendMediaItems:items]);

    [playlist removeMediaItemsAtPositions:@[@3, @1]];

    HeimdallMediaLibraryPlaylist * const refreshedPlaylist =
        [HeimdallMediaLibraryPlaylist playlistForLibraryID:playlist.libraryID];
    XCTAssertEqual(refreshedPlaylist.mediaItems.count, (NSUInteger)2);
    XCTAssertEqual(refreshedPlaylist.mediaItems[0].libraryID, first.libraryID);
    XCTAssertEqual(refreshedPlaylist.mediaItems[1].libraryID, third.libraryID);

    heimdall_ml_playlist_delete(HeimdallLibraryDataTypesIntegrationMediaLibrary(), playlist.libraryID);
    heimdall_ml_remove_stream(HeimdallLibraryDataTypesIntegrationMediaLibrary(), secondID);
    heimdall_ml_remove_stream(HeimdallLibraryDataTypesIntegrationMediaLibrary(), thirdID);
    heimdall_ml_remove_stream(HeimdallLibraryDataTypesIntegrationMediaLibrary(), fourthID);
}

- (void)testPlaylistRemovalFoldsConsecutivePositions
{
    const int64_t secondID =
        HeimdallLibraryDataTypesIntegrationCreateExternalMedia("mock://macosx-datatypes-consecutive-2");
    const int64_t thirdID =
        HeimdallLibraryDataTypesIntegrationCreateExternalMedia("mock://macosx-datatypes-consecutive-3");
    const int64_t fourthID =
        HeimdallLibraryDataTypesIntegrationCreateExternalMedia("mock://macosx-datatypes-consecutive-4");
    XCTAssertNotEqual(secondID, (int64_t)0);
    XCTAssertNotEqual(thirdID, (int64_t)0);
    XCTAssertNotEqual(fourthID, (int64_t)0);

    HeimdallMediaLibraryPlaylist * const playlist = [self integrationPlaylist];
    HeimdallMediaLibraryMediaItem * const first = [self integrationMediaItem];
    HeimdallMediaLibraryMediaItem * const second = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:secondID];
    HeimdallMediaLibraryMediaItem * const third = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:thirdID];
    HeimdallMediaLibraryMediaItem * const fourth = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:fourthID];
    NSArray<HeimdallMediaLibraryMediaItem *> * const items = @[first, second, third, fourth];
    XCTAssertTrue([playlist appendMediaItems:items]);

    [playlist removeMediaItemsAtPositions:@[@1, @2]];

    HeimdallMediaLibraryPlaylist * const refreshedPlaylist =
        [HeimdallMediaLibraryPlaylist playlistForLibraryID:playlist.libraryID];
    XCTAssertEqual(refreshedPlaylist.mediaItems.count, (NSUInteger)2);
    XCTAssertEqual(refreshedPlaylist.mediaItems[0].libraryID, first.libraryID);
    XCTAssertEqual(refreshedPlaylist.mediaItems[1].libraryID, fourth.libraryID);

    heimdall_ml_playlist_delete(HeimdallLibraryDataTypesIntegrationMediaLibrary(), playlist.libraryID);
    heimdall_ml_remove_stream(HeimdallLibraryDataTypesIntegrationMediaLibrary(), secondID);
    heimdall_ml_remove_stream(HeimdallLibraryDataTypesIntegrationMediaLibrary(), thirdID);
    heimdall_ml_remove_stream(HeimdallLibraryDataTypesIntegrationMediaLibrary(), fourthID);
}

- (void)testPlaylistRemovalIgnoresOutOfRangePositions
{
    HeimdallMediaLibraryPlaylist * const playlist = [self integrationPlaylist];
    HeimdallMediaLibraryMediaItem * const item = [self integrationMediaItem];
    XCTAssertTrue([playlist appendMediaItems:@[item]]);

    [playlist removeMediaItemsAtPositions:@[@99]];

    HeimdallMediaLibraryPlaylist * const refreshedPlaylist =
        [HeimdallMediaLibraryPlaylist playlistForLibraryID:playlist.libraryID];
    XCTAssertEqual(refreshedPlaylist.mediaItems.count, (NSUInteger)1);
    XCTAssertEqual(refreshedPlaylist.mediaItems.firstObject.libraryID, item.libraryID);

    heimdall_ml_playlist_delete(HeimdallLibraryDataTypesIntegrationMediaLibrary(), playlist.libraryID);
}

- (void)testPlaylistRemovalTreatsDuplicatePositionsAsOneSelection
{
    const int64_t secondID =
        HeimdallLibraryDataTypesIntegrationCreateExternalMedia("mock://macosx-datatypes-duplicate-2");
    const int64_t thirdID =
        HeimdallLibraryDataTypesIntegrationCreateExternalMedia("mock://macosx-datatypes-duplicate-3");
    XCTAssertNotEqual(secondID, (int64_t)0);
    XCTAssertNotEqual(thirdID, (int64_t)0);

    HeimdallMediaLibraryPlaylist * const playlist = [self integrationPlaylist];
    HeimdallMediaLibraryMediaItem * const first = [self integrationMediaItem];
    HeimdallMediaLibraryMediaItem * const second = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:secondID];
    HeimdallMediaLibraryMediaItem * const third = [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:thirdID];
    NSArray<HeimdallMediaLibraryMediaItem *> * const items = @[first, second, third];
    XCTAssertTrue([playlist appendMediaItems:items]);

    [playlist removeMediaItemsAtPositions:@[@1, @1]];

    HeimdallMediaLibraryPlaylist * const refreshedPlaylist =
        [HeimdallMediaLibraryPlaylist playlistForLibraryID:playlist.libraryID];
    XCTAssertEqual(refreshedPlaylist.mediaItems.count, (NSUInteger)2);
    XCTAssertEqual(refreshedPlaylist.mediaItems[0].libraryID, first.libraryID);
    XCTAssertEqual(refreshedPlaylist.mediaItems[1].libraryID, third.libraryID);

    heimdall_ml_playlist_delete(HeimdallLibraryDataTypesIntegrationMediaLibrary(), playlist.libraryID);
    heimdall_ml_remove_stream(HeimdallLibraryDataTypesIntegrationMediaLibrary(), secondID);
    heimdall_ml_remove_stream(HeimdallLibraryDataTypesIntegrationMediaLibrary(), thirdID);
}

- (void)testNonFileBackedPlaylistMoveToTrashDeletesPlaylist
{
    HeimdallMediaLibraryPlaylist * const playlist = [self integrationPlaylist];
    const int64_t playlistID = playlist.libraryID;

    [playlist moveToTrash];

    XCTAssertNil([HeimdallMediaLibraryPlaylist playlistForLibraryID:playlistID]);
}

- (void)testMissingFileBackedPlaylistMoveToTrashDeletesPlaylist
{
    HeimdallMediaLibraryPlaylist * const createdPlaylist = [self integrationPlaylist];
    const int64_t playlistID = createdPlaylist.libraryID;
    HeimdallMediaLibraryPlaylist *playlist = nil;
    NSString * const missingMRL = [NSURL fileURLWithPath:
                                   [NSTemporaryDirectory() stringByAppendingPathComponent:
                                    @"heimdall-datatypes-missing-playlist.m3u"]].absoluteString;
    playlist = [self playlistWithLibraryID:playlistID MRL:missingMRL];

    XCTAssertNotNil(playlist);
    [playlist moveToTrash];

    XCTAssertNil([HeimdallMediaLibraryPlaylist playlistForLibraryID:playlistID]);
}

- (void)testNonFileMRLPlaylistMoveToTrashDeletesPlaylist
{
    HeimdallMediaLibraryPlaylist * const createdPlaylist = [self integrationPlaylist];
    const int64_t playlistID = createdPlaylist.libraryID;
    HeimdallMediaLibraryPlaylist * const playlist =
        [self playlistWithLibraryID:playlistID MRL:@"mock://macosx-datatypes-playlist"];

    XCTAssertNotNil(playlist);
    [playlist moveToTrash];

    XCTAssertNil([HeimdallMediaLibraryPlaylist playlistForLibraryID:playlistID]);
}

- (void)testFileBackedPlaylistMoveToTrashFailureDeletesPlaylist
{
    HeimdallMediaLibraryPlaylist * const createdPlaylist = [self integrationPlaylist];
    const int64_t playlistID = createdPlaylist.libraryID;
    NSString * const path = [NSTemporaryDirectory() stringByAppendingPathComponent:
                             @"heimdall-datatypes-failing-playlist.m3u"];
    XCTAssertTrue([[NSFileManager defaultManager] createFileAtPath:path
                                                              contents:[NSData data]
                                                            attributes:nil]);
    HeimdallMediaLibraryPlaylist * const playlist =
        [self playlistWithLibraryID:playlistID
                                MRL:[NSURL fileURLWithPath:path].absoluteString];

    XCTAssertNotNil(playlist);
    HeimdallLibraryDataTypesTestResetTrashState();
    HeimdallLibraryDataTypesTestSetTrashFailure(YES);
    [playlist moveToTrash];

    XCTAssertTrue([[NSFileManager defaultManager] fileExistsAtPath:path]);
    XCTAssertEqual(HeimdallLibraryDataTypesTestTrashDestinationURLs().count, (NSUInteger)0);
    XCTAssertNil([HeimdallMediaLibraryPlaylist playlistForLibraryID:playlistID]);
    HeimdallLibraryDataTypesTestResetTrashState();
    [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
}

- (void)testDeletedPlaylistIsNotReturnedByFactory
{
    HeimdallMediaLibraryPlaylist * const playlist = [self integrationPlaylist];
    XCTAssertEqual(heimdall_ml_playlist_delete(HeimdallLibraryDataTypesIntegrationMediaLibrary(), playlist.libraryID), HEIMDALL_SUCCESS);
    XCTAssertNil([HeimdallMediaLibraryPlaylist playlistForLibraryID:playlist.libraryID]);
}

- (void)testPlaylistFavoriteRoundTripsThroughMediaLibrary
{
    HeimdallMediaLibraryPlaylist *playlist = [self integrationPlaylist];
    XCTAssertTrue([playlist setFavorite:YES] == HEIMDALL_SUCCESS);
    playlist = [HeimdallMediaLibraryPlaylist playlistForLibraryID:playlist.libraryID];
    XCTAssertTrue(playlist.favorited);

    XCTAssertEqual([playlist toggleFavorite], HEIMDALL_SUCCESS);
    playlist = [HeimdallMediaLibraryPlaylist playlistForLibraryID:playlist.libraryID];
    XCTAssertFalse(playlist.favorited);

    XCTAssertEqual([playlist toggleFavorite], HEIMDALL_SUCCESS);
    playlist = [HeimdallMediaLibraryPlaylist playlistForLibraryID:playlist.libraryID];
    XCTAssertTrue(playlist.favorited);

    heimdall_ml_playlist_delete(HeimdallLibraryDataTypesIntegrationMediaLibrary(), playlist.libraryID);
}

@end
