/****************************************************************************
 * HeimdallLibraryDataTypesTest.m: Heimdall library data type native tests
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

#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallInputItem.h"
#import "tests/HeimdallLibraryDataTypesTestSupport.h"
#import "tests/HeimdallInputItemTestSupport.h"

@interface HeimdallLibraryDataTypesTestAlbum : HeimdallMediaLibraryAlbum
@property (nonatomic, copy) NSArray<HeimdallMediaLibraryMediaItem *> *relationshipMediaItems;
@end

@implementation HeimdallLibraryDataTypesTestAlbum

- (NSArray<HeimdallMediaLibraryMediaItem *> *)mediaItems
{
    return self.relationshipMediaItems;
}

@end

@interface HeimdallLibraryDataTypesTestArtist : HeimdallMediaLibraryArtist
@property (nonatomic, copy) NSArray<HeimdallMediaLibraryAlbum *> *relationshipAlbums;
@end

@implementation HeimdallLibraryDataTypesTestArtist

- (NSArray<HeimdallMediaLibraryAlbum *> *)albums
{
    return self.relationshipAlbums;
}

@end

@interface HeimdallLibraryDataTypesTestGenre : HeimdallMediaLibraryGenre
@property (nonatomic, copy) NSArray<HeimdallMediaLibraryArtist *> *relationshipArtists;
@property (nonatomic, copy) NSArray<HeimdallMediaLibraryAlbum *> *relationshipAlbums;
@property (nonatomic, copy) NSArray<HeimdallMediaLibraryMediaItem *> *relationshipMediaItems;
@end

@implementation HeimdallLibraryDataTypesTestGenre

- (NSArray<HeimdallMediaLibraryArtist *> *)artists
{
    return self.relationshipArtists;
}

- (NSArray<HeimdallMediaLibraryAlbum *> *)albums
{
    return self.relationshipAlbums;
}

- (NSArray<HeimdallMediaLibraryMediaItem *> *)mediaItems
{
    return self.relationshipMediaItems;
}

@end

@interface HeimdallLibraryDataTypesTestShow : HeimdallMediaLibraryShow
@property (nonatomic, copy) NSArray<HeimdallMediaLibraryMediaItem *> *relationshipEpisodes;
@end

@implementation HeimdallLibraryDataTypesTestShow

- (NSArray<HeimdallMediaLibraryMediaItem *> *)episodes
{
    return self.relationshipEpisodes;
}

@end

@interface HeimdallLibraryDataTypesTestGroup : HeimdallMediaLibraryGroup
@property (nonatomic, copy) NSArray<HeimdallMediaLibraryMediaItem *> *relationshipMediaItems;
@end

@implementation HeimdallLibraryDataTypesTestGroup

- (NSArray<HeimdallMediaLibraryMediaItem *> *)mediaItems
{
    return self.relationshipMediaItems;
}

@end

@interface HeimdallLibraryDataTypesTestRelationshipMediaItem : HeimdallMediaLibraryMediaItem
@property (nonatomic) int64_t relationshipID;
@end

@implementation HeimdallLibraryDataTypesTestRelationshipMediaItem

- (int64_t)libraryID
{
    return self.relationshipID;
}

@end

@interface HeimdallLibraryDataTypesTestFavoriteMediaItem : HeimdallMediaLibraryMediaItem
@property (nonatomic) BOOL testFavorited;
@end

@implementation HeimdallLibraryDataTypesTestFavoriteMediaItem

- (BOOL)favorited
{
    return self.testFavorited;
}

- (int)setFavorite:(BOOL)favorite
{
    self.testFavorited = favorite;
    return HEIMDALL_SUCCESS;
}

@end

@interface HeimdallMediaLibraryGenre (HeimdallLibraryDataTypesTestPrivate)
- (void)iterateMediaItemsWithBlock:(void (^)(HeimdallMediaLibraryMediaItem *item))mediaItemBlock
                          orderedBy:(int)mediaItemParentType;
- (void)enumerateMediaItemsWithBlock:(void (^)(HeimdallMediaLibraryMediaItem *item, BOOL *stop))mediaItemBlock
                            orderedBy:(int)mediaItemParentType;
@end

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"

@interface HeimdallLibraryDataTypesTest : XCTestCase
@end

@implementation HeimdallLibraryDataTypesTest

- (HeimdallMediaLibraryMediaItem *)relationshipMediaItemWithID:(int64_t)libraryID
{
    HeimdallLibraryDataTypesTestRelationshipMediaItem * const item =
        [HeimdallLibraryDataTypesTestRelationshipMediaItem new];
    item.relationshipID = libraryID;
    return item;
}

- (HeimdallLibraryDataTypesTestArtist *)artistWithMultipleAlbums
{
    HeimdallMediaLibraryMediaItem * const first = [self relationshipMediaItemWithID:1];
    HeimdallMediaLibraryMediaItem * const second = [self relationshipMediaItemWithID:2];
    HeimdallMediaLibraryMediaItem * const third = [self relationshipMediaItemWithID:3];

    HeimdallLibraryDataTypesTestAlbum * const firstAlbum = [HeimdallLibraryDataTypesTestAlbum new];
    firstAlbum.relationshipMediaItems = @[first, second];
    HeimdallLibraryDataTypesTestAlbum * const secondAlbum = [HeimdallLibraryDataTypesTestAlbum new];
    secondAlbum.relationshipMediaItems = @[third];

    HeimdallLibraryDataTypesTestArtist * const artist = [HeimdallLibraryDataTypesTestArtist new];
    artist.relationshipAlbums = @[firstAlbum, secondAlbum];
    return artist;
}

- (HeimdallLibraryDataTypesTestGenre *)genreWithMultipleParentRelationships
{
    HeimdallMediaLibraryMediaItem * const first = [self relationshipMediaItemWithID:1];
    HeimdallMediaLibraryMediaItem * const second = [self relationshipMediaItemWithID:2];
    HeimdallMediaLibraryMediaItem * const third = [self relationshipMediaItemWithID:3];

    HeimdallLibraryDataTypesTestAlbum * const firstAlbum = [HeimdallLibraryDataTypesTestAlbum new];
    firstAlbum.relationshipMediaItems = @[first, second];
    HeimdallLibraryDataTypesTestAlbum * const secondAlbum = [HeimdallLibraryDataTypesTestAlbum new];
    secondAlbum.relationshipMediaItems = @[third];

    HeimdallLibraryDataTypesTestArtist * const firstArtist = [HeimdallLibraryDataTypesTestArtist new];
    firstArtist.relationshipAlbums = @[firstAlbum];
    HeimdallLibraryDataTypesTestArtist * const secondArtist = [HeimdallLibraryDataTypesTestArtist new];
    secondArtist.relationshipAlbums = @[secondAlbum];

    HeimdallLibraryDataTypesTestGenre * const genre = [HeimdallLibraryDataTypesTestGenre new];
    genre.relationshipMediaItems = @[first, third, second];
    genre.relationshipAlbums = @[firstAlbum, secondAlbum];
    genre.relationshipArtists = @[firstArtist, secondArtist];
    return genre;
}

- (void)testFileMapsFields
{
    struct heimdall_ml_file_t file = { 0 };
    file.psz_mrl = (char *)"file:///tmp/Heimdall%20Test.mp4";
    file.i_last_modification_date = 1234;
    file.i_type = HEIMDALL_ML_FILE_TYPE_SUBTITLE;
    file.b_external = true;
    file.b_removable = true;
    file.b_present = true;

    HeimdallMediaLibraryFile * const libraryFile = [[HeimdallMediaLibraryFile alloc] initWithFile:&file];

    XCTAssertEqualObjects(libraryFile.MRL, @"file:///tmp/Heimdall%20Test.mp4");
    XCTAssertEqualObjects(libraryFile.fileURL,
                          [NSURL URLWithString:@"file:///tmp/Heimdall%20Test.mp4"]);
    XCTAssertEqual(libraryFile.fileType, HEIMDALL_ML_FILE_TYPE_SUBTITLE);
    XCTAssertTrue(libraryFile.external);
    XCTAssertTrue(libraryFile.removable);
    XCTAssertTrue(libraryFile.present);
    XCTAssertEqual(libraryFile.lastModificationDate, (time_t)1234);
}

- (void)testFileReadableTypes
{
    const heimdall_ml_file_type_t fileTypes[] = {
        HEIMDALL_ML_FILE_TYPE_MAIN,
        HEIMDALL_ML_FILE_TYPE_PART,
        HEIMDALL_ML_FILE_TYPE_PLAYLIST,
        HEIMDALL_ML_FILE_TYPE_SOUNDTRACK,
        HEIMDALL_ML_FILE_TYPE_SUBTITLE,
        HEIMDALL_ML_FILE_TYPE_UNKNOWN,
    };
    NSArray<NSString *> * const readableTypes = @[
        @"Main", @"Part", @"Playlist", @"Soundtrack", @"Subtitle", @"Unknown"
    ];

    for (NSUInteger i = 0; i < sizeof(fileTypes) / sizeof(fileTypes[0]); ++i) {
        struct heimdall_ml_file_t file = { 0 };
        file.i_type = fileTypes[i];
        HeimdallMediaLibraryFile * const typedFile =
            [[HeimdallMediaLibraryFile alloc] initWithFile:&file];
        XCTAssertEqualObjects(typedFile.readableFileType, readableTypes[i]);
    }
}

- (void)testTrackMapsFields
{
    struct heimdall_ml_media_track_t track = { 0 };
    track.psz_codec = (char *)"mp4a";
    track.psz_language = (char *)"en";
    track.psz_description = (char *)"Main audio";
    track.i_type = HEIMDALL_ML_TRACK_TYPE_AUDIO;
    track.i_bitrate = 192000;
    track.a.i_nbChannels = 2;
    track.a.i_sampleRate = 48000;

    HeimdallMediaLibraryTrack * const audioTrack =
        [[HeimdallMediaLibraryTrack alloc] initWithTrack:&track];

    XCTAssertEqualObjects(audioTrack.codec, @"mp4a");
    XCTAssertEqualObjects(audioTrack.language, @"en");
    XCTAssertEqualObjects(audioTrack.trackDescription, @"Main audio");
    XCTAssertEqual(audioTrack.trackType, HEIMDALL_ML_TRACK_TYPE_AUDIO);
    XCTAssertEqual(audioTrack.bitrate, (uint32_t)192000);
    XCTAssertEqual(audioTrack.numberOfAudioChannels, (uint32_t)2);
    XCTAssertEqual(audioTrack.audioSampleRate, (uint32_t)48000);
}

- (void)testTrackReadableTypes
{
    const heimdall_ml_track_type_t trackTypes[] = {
        HEIMDALL_ML_TRACK_TYPE_AUDIO,
        HEIMDALL_ML_TRACK_TYPE_VIDEO,
        HEIMDALL_ML_TRACK_TYPE_UNKNOWN,
    };
    NSArray<NSString *> * const readableTypes = @[
        @"Audio", @"Video", @"Unknown"
    ];

    for (NSUInteger i = 0; i < sizeof(trackTypes) / sizeof(trackTypes[0]); ++i) {
        struct heimdall_ml_media_track_t track = { 0 };
        track.i_type = trackTypes[i];
        HeimdallMediaLibraryTrack * const typedTrack =
            [[HeimdallMediaLibraryTrack alloc] initWithTrack:&track];
        XCTAssertEqualObjects(typedTrack.readableTrackType, readableTypes[i]);
    }
}

- (void)testTrackReadableCodecNames
{
    const struct {
        const char *codec;
        NSString *readableName;
    } codecs[] = {
        { "avc1", @"H264 - MPEG-4 AVC (part 10)" },
        { "mp4a", @"MPEG AAC Audio" },
    };

    for (NSUInteger i = 0; i < sizeof(codecs) / sizeof(codecs[0]); ++i) {
        struct heimdall_ml_media_track_t track = { 0 };
        track.psz_codec = (char *)codecs[i].codec;
        HeimdallMediaLibraryTrack * const typedTrack =
            [[HeimdallMediaLibraryTrack alloc] initWithTrack:&track];
        XCTAssertEqualObjects(typedTrack.readableCodecName, codecs[i].readableName);
    }
}

- (void)testAudioTrackHasNoResolutionLabel
{
    struct heimdall_ml_media_track_t track = { 0 };
    track.i_type = HEIMDALL_ML_TRACK_TYPE_AUDIO;
    track.v.i_width = 1920;
    track.v.i_height = 1080;
    HeimdallMediaLibraryTrack * const audioTrack =
        [[HeimdallMediaLibraryTrack alloc] initWithTrack:&track];
    XCTAssertNil(audioTrack.resolutionLabel);
}

- (void)testTrackResolutionLabels
{
    struct heimdall_ml_media_track_t track = { 0 };
    const struct {
        uint32_t width;
        uint32_t height;
        NSString *label;
    } resolutions[] = {
        { 640, 360, nil },
        { 960, 540, @"SD" },
        { 1920, 1080, @"HD" },
        { 3840, 2160, @"4K" },
        { 7680, 4320, @"8K" },
    };

    for (NSUInteger i = 0; i < sizeof(resolutions) / sizeof(resolutions[0]); ++i) {
        track = (struct heimdall_ml_media_track_t){ 0 };
        track.i_type = HEIMDALL_ML_TRACK_TYPE_VIDEO;
        track.v.i_width = resolutions[i].width;
        track.v.i_height = resolutions[i].height;
        HeimdallMediaLibraryTrack * const videoTrack =
            [[HeimdallMediaLibraryTrack alloc] initWithTrack:&track];
        XCTAssertEqualObjects(videoTrack.resolutionLabel, resolutions[i].label);
    }
}

- (void)testVideoTrackMapsVideoFields
{
    struct heimdall_ml_media_track_t track = { 0 };
    track.i_type = HEIMDALL_ML_TRACK_TYPE_VIDEO;
    track.v.i_width = 1920;
    track.v.i_height = 1080;
    track.v.i_sarNum = 16;
    track.v.i_sarDen = 9;
    track.v.i_fpsNum = 24;
    track.v.i_fpsDen = 1;

    HeimdallMediaLibraryTrack * const videoTrack =
        [[HeimdallMediaLibraryTrack alloc] initWithTrack:&track];
    XCTAssertEqual(videoTrack.videoWidth, (uint32_t)1920);
    XCTAssertEqual(videoTrack.videoHeight, (uint32_t)1080);
    XCTAssertEqual(videoTrack.sourceAspectRatio, (uint32_t)16);
    XCTAssertEqual(videoTrack.sourceAspectRatioDenominator, (uint32_t)9);
    XCTAssertEqual(videoTrack.frameRate, (uint32_t)24);
    XCTAssertEqual(videoTrack.frameRateDenominator, (uint32_t)1);
}

- (void)testShowEpisodeMapsFields
{
    struct heimdall_ml_show_episode_t episode = { 0 };
    episode.psz_summary = (char *)"Episode summary";
    episode.psz_tvdb_id = (char *)"tvdb-123";
    episode.i_episode_nb = 3;
    episode.i_season_number = 2;

    HeimdallMediaLibraryShowEpisode * const showEpisode =
        [[HeimdallMediaLibraryShowEpisode alloc] initWithShowEpisode:&episode];

    XCTAssertEqualObjects(showEpisode.summary, @"Episode summary");
    XCTAssertEqualObjects(showEpisode.tvdbID, @"tvdb-123");
    XCTAssertEqual(showEpisode.episodeNumber, (uint32_t)3);
    XCTAssertEqual(showEpisode.seasonNumber, (uint32_t)2);
}

- (void)testArtistProperties
{
    struct heimdall_ml_artist_t artistData = { 0 };
    artistData.i_id = 7;
    artistData.psz_name = (char *)"Artist";
    artistData.psz_shortbio = (char *)"Biography";
    artistData.psz_mb_id = (char *)"mb-artist";
    artistData.i_nb_album = 2;
    artistData.i_nb_tracks = 3;
    artistData.b_is_favorite = true;
    artistData.thumbnails[HEIMDALL_ML_THUMBNAIL_SMALL].psz_mrl =
        (char *)"file:///tmp/artist.jpg";

    HeimdallMediaLibraryArtist * const artist =
        [[HeimdallMediaLibraryArtist alloc] initWithArtist:&artistData];
    XCTAssertEqual(artist.libraryID, (int64_t)7);
    XCTAssertEqualObjects(artist.name, @"Artist");
    XCTAssertEqualObjects(artist.displayString, @"Artist");
    XCTAssertEqualObjects(artist.shortBiography, @"Biography");
    XCTAssertEqualObjects(artist.musicBrainzID, @"mb-artist");
    XCTAssertEqual(artist.numberOfAlbums, (unsigned int)2);
    XCTAssertEqualObjects(artist.durationString, @"2 albums, 3 songs");
    XCTAssertTrue(artist.favorited);
    XCTAssertTrue(artist.smallArtworkGenerated);
    XCTAssertEqualObjects(artist.smallArtworkMRL, @"file:///tmp/artist.jpg");
    XCTAssertEqual(artist.matchingParentType, HeimdallMediaLibraryParentGroupTypeArtist);
}

- (void)testArtistDurationStringUsesSingularForms
{
    struct heimdall_ml_artist_t artistData = { 0 };
    artistData.i_nb_album = 1;
    artistData.i_nb_tracks = 1;
    HeimdallMediaLibraryArtist * const artist =
        [[HeimdallMediaLibraryArtist alloc] initWithArtist:&artistData];
    XCTAssertEqualObjects(artist.durationString, @"1 album, 1 song");
}

- (void)testArtistDisplayStringFallsBackForMissingName
{
    struct heimdall_ml_artist_t artistData = { 0 };
    artistData.psz_name = (char *)"";
    HeimdallMediaLibraryArtist * const unknownArtist =
        [[HeimdallMediaLibraryArtist alloc] initWithArtist:&artistData];
    XCTAssertEqualObjects(unknownArtist.displayString, @"Unknown Artist");
}

- (void)testEmptyArtistHasSafeDefaults
{
    struct heimdall_ml_artist_t emptyArtistData = { 0 };
    HeimdallMediaLibraryArtist * const emptyArtist =
        [[HeimdallMediaLibraryArtist alloc] initWithArtist:&emptyArtistData];
    XCTAssertEqualObjects(emptyArtist.displayString, @"Unknown Artist");
    XCTAssertEqualObjects(emptyArtist.durationString, @"0 albums, 0 songs");
    XCTAssertEqualObjects(emptyArtist.genreString, @"");
}

- (void)testEmptyAlbumHasSafeDefaults
{
    struct heimdall_ml_album_t emptyAlbumData = { 0 };
    HeimdallMediaLibraryAlbum * const emptyAlbum =
        [[HeimdallMediaLibraryAlbum alloc] initWithAlbum:&emptyAlbumData];
    XCTAssertEqualObjects(emptyAlbum.displayString, @"Unknown Album");
    XCTAssertEqualObjects(emptyAlbum.durationString, @"--:--");
    XCTAssertEqualObjects(emptyAlbum.genreString, @"");
}

- (void)testEmptyGenreHasSafeDefaults
{
    struct heimdall_ml_genre_t emptyGenreData = { 0 };
    HeimdallMediaLibraryGenre * const emptyGenre =
        [[HeimdallMediaLibraryGenre alloc] initWithGenre:&emptyGenreData];
    XCTAssertEqualObjects(emptyGenre.displayString, @"Unknown Genre");
    XCTAssertEqualObjects(emptyGenre.durationString, @"0 songs");
}

- (void)testGenreDurationStringUsesSingularForm
{
    struct heimdall_ml_genre_t genreData = { 0 };
    genreData.i_nb_tracks = 1;
    HeimdallMediaLibraryGenre * const genre =
        [[HeimdallMediaLibraryGenre alloc] initWithGenre:&genreData];
    XCTAssertEqualObjects(genre.durationString, @"1 song");
}

- (void)testArtistEnumerationTraversesMultipleAlbums
{
    HeimdallLibraryDataTypesTestArtist * const artist = [self artistWithMultipleAlbums];
    NSMutableArray<HeimdallMediaLibraryMediaItem *> * const enumerated = NSMutableArray.array;

    [artist enumerateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem * const item,
                                           BOOL * const __unused stop) {
        [enumerated addObject:item];
    }];

    XCTAssertEqualObjects(enumerated,
                          (@[ artist.relationshipAlbums[0].mediaItems[0],
                              artist.relationshipAlbums[0].mediaItems[1],
                              artist.relationshipAlbums[1].mediaItems[0] ]));
}

- (void)testArtistEnumerationStopsAcrossMultipleAlbums
{
    HeimdallLibraryDataTypesTestArtist * const artist = [self artistWithMultipleAlbums];
    __block NSUInteger enumeratedCount = 0;

    [artist enumerateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem * const __unused item,
                                           BOOL * const stop) {
        enumeratedCount++;
        *stop = YES;
    }];

    XCTAssertEqual(enumeratedCount, (NSUInteger)1);
}

- (void)testGenreDefaultEnumerationUsesAlbumOrdering
{
    HeimdallLibraryDataTypesTestGenre * const genre = [self genreWithMultipleParentRelationships];
    NSMutableArray<HeimdallMediaLibraryMediaItem *> * const enumerated = NSMutableArray.array;

    [genre enumerateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem * const item,
                                          BOOL * const __unused stop) {
        [enumerated addObject:item];
    }];

    XCTAssertEqualObjects(enumerated,
                          (@[ genre.relationshipAlbums[0].mediaItems[0],
                              genre.relationshipAlbums[0].mediaItems[1],
                              genre.relationshipAlbums[1].mediaItems[0] ]));
}

- (void)testGenreAlbumEnumerationTraversesAlbumsInOrder
{
    HeimdallLibraryDataTypesTestGenre * const genre = [self genreWithMultipleParentRelationships];
    NSMutableArray<HeimdallMediaLibraryMediaItem *> * const enumerated = NSMutableArray.array;

    [genre iterateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem * const item) {
        [enumerated addObject:item];
    } orderedBy:HEIMDALL_ML_PARENT_ALBUM];

    XCTAssertEqualObjects(enumerated,
                          (@[ genre.relationshipAlbums[0].mediaItems[0],
                              genre.relationshipAlbums[0].mediaItems[1],
                              genre.relationshipAlbums[1].mediaItems[0] ]));
}

- (void)testGenreArtistEnumerationTraversesArtistsInOrder
{
    HeimdallLibraryDataTypesTestGenre * const genre = [self genreWithMultipleParentRelationships];
    NSMutableArray<HeimdallMediaLibraryMediaItem *> * const enumerated = NSMutableArray.array;

    [genre iterateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem * const item) {
        [enumerated addObject:item];
    } orderedBy:HEIMDALL_ML_PARENT_ARTIST];

    XCTAssertEqualObjects(enumerated,
                          (@[ genre.relationshipArtists[0].albums[0].mediaItems[0],
                              genre.relationshipArtists[0].albums[0].mediaItems[1],
                              genre.relationshipArtists[1].albums[0].mediaItems[0] ]));
}

- (void)testGenreEnumerationStopsThroughNestedRelationships
{
    HeimdallLibraryDataTypesTestGenre * const genre = [self genreWithMultipleParentRelationships];
    __block NSUInteger enumeratedCount = 0;

    [genre enumerateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem * const __unused item,
                                          BOOL * const stop) {
        enumeratedCount++;
        *stop = YES;
    } orderedBy:HEIMDALL_ML_PARENT_ALBUM];

    XCTAssertEqual(enumeratedCount, (NSUInteger)1);
}

- (void)testShowFavoriteIsTrueWhenEveryEpisodeIsFavorited
{
    HeimdallLibraryDataTypesTestFavoriteMediaItem * const first =
        [HeimdallLibraryDataTypesTestFavoriteMediaItem new];
    first.testFavorited = YES;
    HeimdallLibraryDataTypesTestFavoriteMediaItem * const second =
        [HeimdallLibraryDataTypesTestFavoriteMediaItem new];
    second.testFavorited = YES;

    HeimdallLibraryDataTypesTestShow * const show = [HeimdallLibraryDataTypesTestShow new];
    show.relationshipEpisodes = @[first, second];

    XCTAssertTrue(show.favorited);
}

- (void)testShowFavoriteIsFalseWhenEpisodesAreMixed
{
    HeimdallLibraryDataTypesTestFavoriteMediaItem * const first =
        [HeimdallLibraryDataTypesTestFavoriteMediaItem new];
    first.testFavorited = YES;
    HeimdallLibraryDataTypesTestFavoriteMediaItem * const second =
        [HeimdallLibraryDataTypesTestFavoriteMediaItem new];
    second.testFavorited = NO;

    HeimdallLibraryDataTypesTestShow * const show = [HeimdallLibraryDataTypesTestShow new];
    show.relationshipEpisodes = @[first, second];

    XCTAssertFalse(show.favorited);
}

- (void)testGroupFavoriteIsTrueWhenEveryMediaItemIsFavorited
{
    HeimdallLibraryDataTypesTestFavoriteMediaItem * const first =
        [HeimdallLibraryDataTypesTestFavoriteMediaItem new];
    first.testFavorited = YES;
    HeimdallLibraryDataTypesTestFavoriteMediaItem * const second =
        [HeimdallLibraryDataTypesTestFavoriteMediaItem new];
    second.testFavorited = YES;

    HeimdallLibraryDataTypesTestGroup * const group = [HeimdallLibraryDataTypesTestGroup new];
    group.relationshipMediaItems = @[first, second];

    XCTAssertTrue(group.favorited);
}

- (void)testGroupFavoriteIsFalseWhenMediaItemsAreMixed
{
    HeimdallLibraryDataTypesTestFavoriteMediaItem * const first =
        [HeimdallLibraryDataTypesTestFavoriteMediaItem new];
    first.testFavorited = YES;
    HeimdallLibraryDataTypesTestFavoriteMediaItem * const second =
        [HeimdallLibraryDataTypesTestFavoriteMediaItem new];
    second.testFavorited = NO;

    HeimdallLibraryDataTypesTestGroup * const group = [HeimdallLibraryDataTypesTestGroup new];
    group.relationshipMediaItems = @[first, second];

    XCTAssertFalse(group.favorited);
}

- (void)testAlbumProperties
{
    struct heimdall_ml_album_t albumData = { 0 };
    albumData.i_id = 8;
    albumData.psz_title = (char *)"Album";
    albumData.psz_summary = (char *)"Album summary";
    albumData.psz_artist = (char *)"Artist";
    albumData.i_artist_id = 7;
    albumData.i_nb_tracks = 10;
    albumData.i_duration = 61000;
    albumData.i_year = 2026;
    albumData.b_is_favorite = true;
    albumData.thumbnails[HEIMDALL_ML_THUMBNAIL_SMALL].psz_mrl =
        (char *)"file:///tmp/album.jpg";

    HeimdallMediaLibraryAlbum * const album =
        [[HeimdallMediaLibraryAlbum alloc] initWithAlbum:&albumData];
    XCTAssertEqual(album.libraryID, (int64_t)8);
    XCTAssertEqualObjects(album.title, @"Album");
    XCTAssertEqualObjects(album.displayString, @"Album");
    XCTAssertEqualObjects(album.summary, @"Album summary");
    XCTAssertEqualObjects(album.artistName, @"Artist");
    XCTAssertEqual(album.artistID, (int64_t)7);
    XCTAssertEqual(album.numberOfTracks, (unsigned int)10);
    XCTAssertEqual(album.duration, (int64_t)61000);
    XCTAssertEqual(album.year, (unsigned int)2026);
    XCTAssertEqualObjects(album.durationString, @"01:01");
    XCTAssertEqualObjects(album.albums, @[ album ]);
    XCTAssertTrue(album.favorited);
    XCTAssertTrue(album.smallArtworkGenerated);
    XCTAssertEqualObjects(album.smallArtworkMRL, @"file:///tmp/album.jpg");
    XCTAssertEqual(album.matchingParentType, HeimdallMediaLibraryParentGroupTypeAlbum);
    XCTAssertEqualObjects(album.genreString, @"");
}

- (void)testGenreProperties
{
    struct heimdall_ml_genre_t genreData = { 0 };
    genreData.i_id = 9;
    genreData.psz_name = (char *)"Genre";
    genreData.i_nb_tracks = 4;
    genreData.b_is_favorite = true;
    genreData.thumbnails[HEIMDALL_ML_THUMBNAIL_SMALL].psz_mrl =
        (char *)"file:///tmp/genre.jpg";

    HeimdallMediaLibraryGenre * const genre =
        [[HeimdallMediaLibraryGenre alloc] initWithGenre:&genreData];
    XCTAssertEqual(genre.libraryID, (int64_t)9);
    XCTAssertEqualObjects(genre.name, @"Genre");
    XCTAssertEqualObjects(genre.displayString, @"Genre");
    XCTAssertEqualObjects(genre.durationString, @"4 songs");
    XCTAssertTrue(genre.favorited);
    XCTAssertEqualObjects(genre.genres, @[ genre ]);
    XCTAssertEqual(genre.matchingParentType, HeimdallMediaLibraryParentGroupTypeGenre);
    XCTAssertTrue(genre.smallArtworkGenerated);
    XCTAssertEqualObjects(genre.smallArtworkMRL, @"file:///tmp/genre.jpg");
}

- (void)testShowProperties
{
    struct heimdall_ml_show_t showData = { 0 };
    showData.i_id = 10;
    showData.psz_name = (char *)"Show";
    showData.psz_summary = (char *)"Show summary";
    showData.psz_artwork_mrl = (char *)"file:///tmp/show.jpg";
    showData.psz_tvdb_id = (char *)"tvdb-show";
    showData.i_release_year = 2025;
    showData.i_nb_episodes = 12;
    showData.i_nb_seasons = 3;

    HeimdallMediaLibraryShow * const show = [[HeimdallMediaLibraryShow alloc] initWithShow:&showData];
    XCTAssertEqual(show.libraryID, (int64_t)10);
    XCTAssertEqualObjects(show.name, @"Show");
    XCTAssertEqualObjects(show.displayString, @"Show");
    XCTAssertEqualObjects(show.summary, @"Show summary");
    XCTAssertEqualObjects(show.tvdbId, @"tvdb-show");
    XCTAssertEqual(show.releaseYear, (unsigned int)2025);
    XCTAssertEqual(show.episodeCount, (uint32_t)12);
    XCTAssertEqual(show.seasonCount, (uint32_t)3);
    XCTAssertEqualObjects(show.primaryDetailString, @"3 seasons, 12 episodes");
    XCTAssertEqualObjects(show.secondaryDetailString, @"Released in 2025");
    XCTAssertTrue(show.smallArtworkGenerated);
    XCTAssertEqualObjects(show.smallArtworkMRL, @"file:///tmp/show.jpg");
}

- (void)testShowPrimaryDetailStringUsesSingularForms
{
    struct heimdall_ml_show_t showData = { 0 };
    showData.i_nb_seasons = 1;
    showData.i_nb_episodes = 1;
    HeimdallMediaLibraryShow * const show =
        [[HeimdallMediaLibraryShow alloc] initWithShow:&showData];
    XCTAssertEqualObjects(show.primaryDetailString, @"1 season, 1 episode");
}

- (void)testGroupProperties
{
    struct heimdall_ml_group_t groupData = { 0 };
    groupData.i_id = 11;
    groupData.psz_name = (char *)"Group";
    groupData.i_nb_total_media = 4;
    groupData.i_nb_video = 2;
    groupData.i_nb_audio = 1;
    groupData.i_nb_unknown = 1;
    groupData.i_nb_present_media = 3;
    groupData.i_nb_present_video = 2;
    groupData.i_nb_present_audio = 1;
    groupData.i_nb_present_unknown = 0;
    groupData.i_nb_seen = 2;
    groupData.i_nb_present_seen = 1;
    groupData.i_duration = 120000;
    groupData.i_creation_date = 100;
    groupData.i_last_modification_date = 200;

    HeimdallMediaLibraryGroup * const group =
        [[HeimdallMediaLibraryGroup alloc] initWithGroup:&groupData];
    XCTAssertEqual(group.libraryID, (int64_t)11);
    XCTAssertEqualObjects(group.name, @"Group");
    XCTAssertEqual(group.numberOfTotalItems, (NSUInteger)4);
    XCTAssertEqual(group.numberOfVideoItems, (NSUInteger)2);
    XCTAssertEqual(group.numberOfAudioItems, (NSUInteger)1);
    XCTAssertEqual(group.numberOfUnknownItems, (NSUInteger)1);
    XCTAssertEqual(group.numberOfPresentTotalItems, (NSUInteger)3);
    XCTAssertEqual(group.numberOfSeenItems, (NSUInteger)2);
    XCTAssertEqual(group.duration, (int64_t)120000);
    XCTAssertEqualObjects(group.durationString, @"02:00");
    XCTAssertEqualObjects(group.creationDate, [NSDate dateWithTimeIntervalSince1970:100]);
    XCTAssertEqualObjects(group.lastModificationDate,
                          [NSDate dateWithTimeIntervalSince1970:200]);
}

- (void)testGroupPrimaryDetailStringUsesSingularForm
{
    struct heimdall_ml_group_t groupData = { 0 };
    groupData.i_nb_total_media = 1;
    HeimdallMediaLibraryGroup * const group =
        [[HeimdallMediaLibraryGroup alloc] initWithGroup:&groupData];
    XCTAssertEqualObjects(group.primaryDetailString, @"1 item");
}

- (void)testPlaylistProperties
{
    struct heimdall_ml_playlist_t playlistData = { 0 };
    playlistData.i_id = 12;
    playlistData.psz_name = (char *)"Playlist";
    playlistData.psz_mrl = (char *)"file:///tmp/playlist.m3u";
    playlistData.psz_artwork_mrl = (char *)"file:///tmp/playlist.jpg";
    playlistData.i_nb_media = 2;
    playlistData.i_nb_audio = 1;
    playlistData.i_nb_video = 1;
    playlistData.i_duration = 90000;
    playlistData.i_nb_duration_unknown = 1;
    playlistData.i_creation_date = 300;
    playlistData.b_is_read_only = true;
    playlistData.b_is_favorite = true;

    HeimdallMediaLibraryPlaylist * const playlist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];
    XCTAssertEqual(playlist.libraryID, (int64_t)12);
    XCTAssertEqualObjects(playlist.MRL, @"file:///tmp/playlist.m3u");
    XCTAssertEqualObjects(playlist.displayString, @"Playlist");
    XCTAssertTrue(playlist.smallArtworkGenerated);
    XCTAssertEqualObjects(playlist.smallArtworkMRL, @"file:///tmp/playlist.jpg");
    XCTAssertEqualObjects(playlist.primaryDetailString, @"2 items");
    XCTAssertEqualObjects(playlist.durationString, @"01:30");
    XCTAssertEqual(playlist.numberOfMedia, (unsigned int)2);
    XCTAssertEqual(playlist.numberOfAudios, (uint32_t)1);
    XCTAssertEqual(playlist.numberOfVideos, (uint32_t)1);
    XCTAssertEqual(playlist.duration, (int64_t)90000);
    XCTAssertEqual(playlist.numberDurationUnknown, (uint32_t)1);
    XCTAssertTrue(playlist.readOnly);
    XCTAssertTrue(playlist.favorited);
}

- (void)testPlaylistPrimaryDetailStringUsesSingularForm
{
    struct heimdall_ml_playlist_t playlistData = { 0 };
    playlistData.i_nb_media = 1;
    HeimdallMediaLibraryPlaylist * const playlist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];
    XCTAssertEqualObjects(playlist.primaryDetailString, @"1 item");
}

- (void)testPlaylistMapsPresentCounts
{
    struct heimdall_ml_playlist_t playlistData = { 0 };
    playlistData.i_nb_present_media = 3;
    playlistData.i_nb_present_audio = 1;
    playlistData.i_nb_present_video = 2;
    playlistData.i_nb_present_unknown = 0;

    HeimdallMediaLibraryPlaylist * const playlist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];
    XCTAssertEqual(playlist.numberOfPresentMedia, (unsigned int)3);
    XCTAssertEqual(playlist.numberOfPresentAudios, (uint32_t)1);
    XCTAssertEqual(playlist.numberOfPresentVideos, (uint32_t)2);
    XCTAssertEqual(playlist.numberOfPresentUnknowns, (uint32_t)0);
}

- (void)testPlaylistMapsArtworkAndCreationDate
{
    struct heimdall_ml_playlist_t playlistData = { 0 };
    playlistData.i_creation_date = 300;
    playlistData.psz_artwork_mrl = (char *)"file:///tmp/playlist.jpg";

    HeimdallMediaLibraryPlaylist * const playlist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];
    XCTAssertEqualObjects(playlist.creationDate,
                          [NSDate dateWithTimeIntervalSince1970:300]);
    XCTAssertEqualObjects(playlist.smallArtworkMRL, @"file:///tmp/playlist.jpg");
}

- (void)testEntryPointProperties
{
    struct heimdall_ml_folder_t folder = { 0 };
    char folderMRL[] = "file:///tmp/Heimdall%20Folder";
    folder.psz_mrl = folderMRL;
    folder.b_present = true;
    folder.b_banned = true;
    HeimdallMediaLibraryEntryPoint * const entryPoint =
        [[HeimdallMediaLibraryEntryPoint alloc] initWithEntryPoint:&folder];
    XCTAssertEqualObjects(entryPoint.MRL, @"file:///tmp/Heimdall%20Folder");
    XCTAssertEqualObjects(entryPoint.decodedMRL, @"file:///tmp/Heimdall Folder");
    XCTAssertTrue(entryPoint.isPresent);
    XCTAssertTrue(entryPoint.isBanned);
}

- (void)testGroupPresentCounts
{
    struct heimdall_ml_group_t groupData = { 0 };
    groupData.psz_name = (char *)"";
    groupData.i_nb_present_media = 2;
    groupData.i_nb_present_video = 1;
    groupData.i_nb_present_audio = 1;
    groupData.i_nb_present_unknown = 0;
    groupData.i_nb_present_seen = 1;

    HeimdallMediaLibraryGroup * const group =
        [[HeimdallMediaLibraryGroup alloc] initWithGroup:&groupData];
    XCTAssertEqual(group.numberOfPresentTotalItems, (NSUInteger)2);
    XCTAssertEqual(group.numberOfPresentVideoItems, (NSUInteger)1);
    XCTAssertEqual(group.numberOfPresentAudioItems, (NSUInteger)1);
    XCTAssertEqual(group.numberOfPresentUnknownItems, (NSUInteger)0);
    XCTAssertEqual(group.numberOfPresentSeenItems, (NSUInteger)1);
}

- (void)testGroupDisplayStringFallsBackForMissingName
{
    struct heimdall_ml_group_t groupData = { 0 };
    HeimdallMediaLibraryGroup * const group =
        [[HeimdallMediaLibraryGroup alloc] initWithGroup:&groupData];
    XCTAssertEqualObjects(group.displayString, @"Unknown Group");
}

- (void)testEmptyGroupHasNoMediaItems
{
    struct heimdall_ml_group_t groupData = { 0 };
    HeimdallMediaLibraryGroup * const group =
        [[HeimdallMediaLibraryGroup alloc] initWithGroup:&groupData];
    XCTAssertEqualObjects(group.mediaItems, @[]);
    XCTAssertNil(group.firstMediaItem);
}

- (void)testEmptyPlaylistDefaults
{
    struct heimdall_ml_playlist_t playlistData = { 0 };
    HeimdallMediaLibraryPlaylist * const emptyPlaylist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];
    XCTAssertEqualObjects(emptyPlaylist.primaryDetailString, @"No item");
    XCTAssertEqualObjects(emptyPlaylist.durationString, @"--:--");
    XCTAssertFalse(emptyPlaylist.readOnly);
    XCTAssertFalse(emptyPlaylist.favorited);
}

- (void)testPlaylistIsFileBackedOnlyForExistingLocalFiles
{
    struct heimdall_ml_playlist_t playlistData = { 0 };
    playlistData.psz_mrl = (char *)"https://example.com/playlist.m3u";
    HeimdallMediaLibraryPlaylist * const remotePlaylist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];
    XCTAssertFalse(remotePlaylist.isFileBacked);

    NSString * const path = [NSTemporaryDirectory() stringByAppendingPathComponent:
                             @"heimdall-datatypes-file-backed-test.m3u"];
    XCTAssertTrue([[NSFileManager defaultManager] createFileAtPath:path
                                                              contents:[NSData data]
                                                            attributes:nil]);
    NSString * const fileMRL = [NSURL fileURLWithPath:path].absoluteString;
    playlistData.psz_mrl = (char *)fileMRL.UTF8String;
    HeimdallMediaLibraryPlaylist * const filePlaylist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];
    XCTAssertTrue(filePlaylist.isFileBacked);
    [[NSFileManager defaultManager] removeItemAtPath:path error:nil];

    HeimdallMediaLibraryPlaylist * const missingFilePlaylist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];
    XCTAssertFalse(missingFilePlaylist.isFileBacked);
}

- (void)testPlaylistRevealInFinder
{
    HeimdallInputItemTestResetAppKitState();
    struct heimdall_ml_playlist_t playlistData = { 0 };
    NSString * const path = [NSTemporaryDirectory() stringByAppendingPathComponent:
                             @"heimdall-datatypes-reveal-test.m3u"];
    XCTAssertTrue([[NSFileManager defaultManager] createFileAtPath:path
                                                              contents:[NSData data]
                                                            attributes:nil]);
    NSString * const fileMRL = [NSURL fileURLWithPath:path].absoluteString;
    playlistData.psz_mrl = (char *)fileMRL.UTF8String;
    HeimdallMediaLibraryPlaylist * const playlist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];
    [playlist revealInFinder];
    XCTAssertTrue(HeimdallInputItemTestDidReveal());
    [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
}

- (void)testPlaylistRevealInFinderWithoutMRLDoesNothing
{
    HeimdallInputItemTestResetAppKitState();
    struct heimdall_ml_playlist_t playlistData = { 0 };
    HeimdallMediaLibraryPlaylist * const playlist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];
    [playlist revealInFinder];
    XCTAssertFalse(HeimdallInputItemTestDidReveal());
}

- (void)testPlaylistRevealInFinderWithMalformedMRLDoesNothing
{
    HeimdallInputItemTestResetAppKitState();
    struct heimdall_ml_playlist_t playlistData = { 0 };
    playlistData.psz_mrl = (char *)"http://[invalid";
    HeimdallMediaLibraryPlaylist * const playlist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];

    [playlist revealInFinder];

    XCTAssertFalse(HeimdallInputItemTestDidReveal());
    XCTAssertEqual(HeimdallInputItemTestRevealedURLs().count, (NSUInteger)0);
}

- (void)testMediaItemRevealInFinderSelectsFirstFile
{
    HeimdallInputItemTestResetAppKitState();
    NSURL * const firstURL = [NSURL fileURLWithPath:
                              [NSTemporaryDirectory() stringByAppendingPathComponent:
                               @"heimdall-datatypes-reveal-first-file.mp4"]];
    NSURL * const secondURL = [NSURL fileURLWithPath:
                               [NSTemporaryDirectory() stringByAppendingPathComponent:
                                @"heimdall-datatypes-reveal-second-file.mp4"]];
    HeimdallMediaLibraryMediaItem * const item =
        HeimdallLibraryDataTypesTestMediaItemWithInputMetadataAndFileURLs(
            HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE,
            nil,
            @[ firstURL, secondURL ]);

    [item revealInFinder];

    XCTAssertTrue(HeimdallInputItemTestDidReveal());
    XCTAssertEqualObjects(HeimdallInputItemTestRevealedURLs(), @[firstURL]);
}

- (void)testMediaItemRevealInFinderWithoutFilesDoesNothing
{
    HeimdallInputItemTestResetAppKitState();
    HeimdallMediaLibraryMediaItem * const item =
        HeimdallLibraryDataTypesTestMediaItemWithInputMetadataAndFileURLs(
            HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE,
            nil,
            @[]);

    [item revealInFinder];

    XCTAssertFalse(HeimdallInputItemTestDidReveal());
    XCTAssertEqual(HeimdallInputItemTestRevealedURLs().count, (NSUInteger)0);
}

- (void)testDummyItemRevealInFinderUsesFirstChildFile
{
    HeimdallInputItemTestResetAppKitState();
    NSURL * const childURL = [NSURL fileURLWithPath:
                              [NSTemporaryDirectory() stringByAppendingPathComponent:
                               @"heimdall-datatypes-dummy-reveal-file.mp4"]];
    HeimdallMediaLibraryMediaItem * const child =
        HeimdallLibraryDataTypesTestMediaItemWithInputMetadataAndFileURLs(
            HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE,
            nil,
            @[ childURL ]);
    HeimdallMediaLibraryDummyItem * const item =
        [[HeimdallMediaLibraryDummyItem alloc] initWithDisplayString:@"Collection"
                                                 withMediaItems:@[child]];

    [item revealInFinder];

    XCTAssertTrue(HeimdallInputItemTestDidReveal());
    XCTAssertEqualObjects(HeimdallInputItemTestRevealedURLs(), @[childURL]);
}

- (void)testEmptyDummyItemRevealInFinderDoesNothing
{
    HeimdallInputItemTestResetAppKitState();
    HeimdallMediaLibraryDummyItem * const item =
        [[HeimdallMediaLibraryDummyItem alloc] initWithDisplayString:@"Collection"
                                                 withMediaItems:@[]];

    [item revealInFinder];

    XCTAssertFalse(HeimdallInputItemTestDidReveal());
    XCTAssertEqual(HeimdallInputItemTestRevealedURLs().count, (NSUInteger)0);
}

- (void)testFileBackedPlaylistMoveToTrashInvokesFileManager
{
    HeimdallLibraryDataTypesTestResetTrashState();
    NSString * const path = [NSTemporaryDirectory() stringByAppendingPathComponent:
                             @"heimdall-datatypes-move-playlist-to-trash.m3u"];
    XCTAssertTrue([[NSFileManager defaultManager] createFileAtPath:path
                                                          contents:[NSData data]
                                                        attributes:nil]);

    struct heimdall_ml_playlist_t playlistData = { 0 };
    NSString * const fileMRL = [NSURL fileURLWithPath:path].absoluteString;
    playlistData.psz_mrl = (char *)fileMRL.UTF8String;
    HeimdallMediaLibraryPlaylist * const playlist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];

    [playlist moveToTrash];

    XCTAssertEqual(HeimdallLibraryDataTypesTestTrashedSourceURLs().count, (NSUInteger)1);
    XCTAssertFalse([[NSFileManager defaultManager] fileExistsAtPath:path]);
    XCTAssertEqual(HeimdallLibraryDataTypesTestTrashDestinationURLs().count, (NSUInteger)1);
    XCTAssertTrue([[NSFileManager defaultManager]
                   fileExistsAtPath:HeimdallLibraryDataTypesTestTrashDestinationURLs().firstObject.path]);
    HeimdallLibraryDataTypesTestResetTrashState();
    [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
}

- (void)testReadOnlyPlaylistRejectsAppendingMedia
{
    struct heimdall_ml_playlist_t playlistData = { 0 };
    playlistData.b_is_read_only = true;
    HeimdallMediaLibraryPlaylist * const playlist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];
    HeimdallMediaLibraryMediaItem * const item =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE);

    XCTAssertFalse([playlist appendMediaItems:@[item]]);
}

- (void)testWritablePlaylistRejectsAppendingNoMedia
{
    struct heimdall_ml_playlist_t playlistData = { 0 };
    HeimdallMediaLibraryPlaylist * const playlist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];

    XCTAssertFalse([playlist appendMediaItems:@[]]);
}

- (void)testReadOnlyPlaylistRejectsRename
{
    struct heimdall_ml_playlist_t playlistData = { 0 };
    playlistData.b_is_read_only = true;
    HeimdallMediaLibraryPlaylist * const playlist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];

    XCTAssertFalse([playlist renameTo:@"Renamed playlist"]);
}

- (void)testPlaylistRejectsRenameToEmptyName
{
    struct heimdall_ml_playlist_t playlistData = { 0 };
    HeimdallMediaLibraryPlaylist * const playlist =
        [[HeimdallMediaLibraryPlaylist alloc] initWithPlaylist:&playlistData];

    XCTAssertFalse([playlist renameTo:@""]);
}

- (void)testMediaItemMapsCommonFields
{
    HeimdallMediaLibraryMediaItem * const item =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE);
    XCTAssertEqual(item.libraryID, (int64_t)13);
    XCTAssertEqual(item.mediaType, HEIMDALL_ML_MEDIA_TYPE_VIDEO);
    XCTAssertEqualObjects(item.readableMediaType, @"Video");
    XCTAssertEqualObjects(item.title, @"Media");
    XCTAssertEqualObjects(item.displayString, @"Media");
    XCTAssertEqual(item.year, 2024);
    XCTAssertEqual(item.duration, (int64_t)123000);
    XCTAssertEqualObjects(item.durationString, @"02:03");
    XCTAssertEqual(item.playCount, (uint32_t)2);
    XCTAssertEqual(item.lastPlayedDate, (time_t)4567);
    XCTAssertEqual(item.progress, .5);
    XCTAssertTrue(item.favorited);
    XCTAssertTrue(item.smallArtworkGenerated);
    XCTAssertEqualObjects(item.smallArtworkMRL, @"file:///tmp/media.jpg");
}

- (void)testMediaItemDisplayStringFallsBackForMissingTitle
{
    HeimdallMediaLibraryMediaItem * const item =
        HeimdallLibraryDataTypesTestMediaItemWithEmptyTitle(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE);
    XCTAssertEqualObjects(item.displayString, @"Unknown item");
}

- (void)testMediaItemPrimaryDetailFallsBackToDuration
{
    HeimdallMediaLibraryMediaItem * const item =
        HeimdallLibraryDataTypesTestMediaItemWithTypeAndSubtype(HEIMDALL_ML_MEDIA_TYPE_VIDEO,
                                                           HEIMDALL_ML_MEDIA_SUBTYPE_UNKNOWN);

    XCTAssertEqualObjects(item.primaryDetailString, item.durationString);
}

- (void)testMoviePrimaryDetailUsesDirector
{
    HeimdallMediaLibraryMediaItem * const movie =
        HeimdallLibraryDataTypesTestMediaItemWithInputMetadata(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE,
            @{ HeimdallLibraryDataTypesTestInputItemDirectorKey: @"Director" });

    XCTAssertEqualObjects(movie.primaryDetailString, @"Director");
}

- (void)testMoviePrimaryDetailFallsBackForEmptyDirector
{
    HeimdallMediaLibraryMediaItem * const movie =
        HeimdallLibraryDataTypesTestMediaItemWithInputMetadata(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE,
            @{ HeimdallLibraryDataTypesTestInputItemDirectorKey: @"" });

    XCTAssertEqualObjects(movie.primaryDetailString, movie.durationString);
}

- (void)testShowEpisodePrimaryDetailUsesShowName
{
    HeimdallMediaLibraryMediaItem * const episode =
        HeimdallLibraryDataTypesTestMediaItemWithInputMetadata(HEIMDALL_ML_MEDIA_SUBTYPE_SHOW_EPISODE,
            @{ HeimdallLibraryDataTypesTestInputItemShowNameKey: @"Show" });

    XCTAssertEqualObjects(episode.primaryDetailString, @"Show");
}

- (void)testShowEpisodePrimaryDetailFallsBackToDurationWithoutShowName
{
    HeimdallMediaLibraryMediaItem * const episode =
        HeimdallLibraryDataTypesTestMediaItemWithInputMetadata(HEIMDALL_ML_MEDIA_SUBTYPE_SHOW_EPISODE,
            @{ HeimdallLibraryDataTypesTestInputItemShowNameKey: @"" });

    XCTAssertEqualObjects(episode.primaryDetailString, episode.durationString);
}

- (void)testAlbumTrackPrimaryDetailUsesArtistName
{
    HeimdallInputItemTestSetArtistName(@"Artist");
    HeimdallMediaLibraryMediaItem * const track =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_ALBUMTRACK);

    XCTAssertEqualObjects(track.primaryDetailString, @"Artist");
    HeimdallInputItemTestSetArtistName(nil);
}

- (void)testAlbumTrackPrimaryDetailFallsBackForMissingArtistName
{
    HeimdallInputItemTestSetArtistName(nil);
    HeimdallMediaLibraryMediaItem * const track =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_ALBUMTRACK);

    XCTAssertEqualObjects(track.primaryDetailString, track.durationString);
}

- (void)testMediaItemSecondaryDetailUsesInputItemDate
{
    HeimdallMediaLibraryMediaItem * const item =
        HeimdallLibraryDataTypesTestMediaItemWithInputMetadata(HEIMDALL_ML_MEDIA_SUBTYPE_UNKNOWN,
            @{ HeimdallLibraryDataTypesTestInputItemDateKey: @"2026-09-05" });

    XCTAssertEqualObjects(item.secondaryDetailString, @"2026-09-05");
}

- (void)testAlbumTrackSecondaryDetailFallsBackToInputItemDateWithoutGenre
{
    HeimdallMediaLibraryMediaItem * const track =
        HeimdallLibraryDataTypesTestMediaItemWithInputMetadata(HEIMDALL_ML_MEDIA_SUBTYPE_ALBUMTRACK,
            @{ HeimdallLibraryDataTypesTestInputItemDateKey: @"2026-09-05" });

    XCTAssertEqualObjects(track.secondaryDetailString, @"2026-09-05");
}

- (void)testMediaItemMapsFilesAndTracks
{
    HeimdallMediaLibraryMediaItem * const item =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE);
    XCTAssertEqual(item.files.count, (NSUInteger)1);
    XCTAssertEqual(item.tracks.count, (NSUInteger)2);
    XCTAssertEqual(item.firstVideoTrack, item.tracks.firstObject);
    XCTAssertEqual(item.firstVideoTrack.videoWidth, (uint32_t)1920);
    XCTAssertEqual(item.firstVideoTrack.videoHeight, (uint32_t)1080);
}

- (void)testMediaItemMapsAlbumTrackFields
{
    HeimdallMediaLibraryMediaItem * const albumTrack =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_ALBUMTRACK);
    XCTAssertEqual(albumTrack.mediaSubType, HEIMDALL_ML_MEDIA_SUBTYPE_ALBUMTRACK);
    XCTAssertEqualObjects(albumTrack.readableMediaSubType, @"Album Track");
    XCTAssertEqual(albumTrack.artistID, (int64_t)7);
    XCTAssertEqual(albumTrack.albumID, (int64_t)8);
    XCTAssertEqual(albumTrack.genreID, (int64_t)9);
    XCTAssertEqual(albumTrack.trackNumber, 3);
    XCTAssertEqual(albumTrack.discNumber, 1);
}

- (void)testMediaItemMapsMovieFields
{
    HeimdallMediaLibraryMediaItem * const movie =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE);
    XCTAssertEqual(movie.mediaSubType, HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE);
    XCTAssertEqualObjects(movie.readableMediaSubType, @"Movie");
    XCTAssertEqualObjects(movie.movie.summary, @"Movie summary");
    XCTAssertEqualObjects(movie.movie.imdbID, @"tt123");
}

- (void)testMediaItemMapsShowEpisodeFields
{
    HeimdallMediaLibraryMediaItem * const episode =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_SHOW_EPISODE);
    XCTAssertEqual(episode.mediaSubType, HEIMDALL_ML_MEDIA_SUBTYPE_SHOW_EPISODE);
    XCTAssertEqualObjects(episode.readableMediaSubType, @"Show Episode");
    XCTAssertEqual(episode.showEpisode.seasonNumber, (uint32_t)2);
    XCTAssertEqual(episode.showEpisode.episodeNumber, (uint32_t)4);
}

- (void)testShowEpisodeSecondaryDetailUsesEpisodeNumbers
{
    HeimdallMediaLibraryMediaItem * const episode =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_SHOW_EPISODE);

    XCTAssertEqualObjects(episode.secondaryDetailString, @"Season 2, Episode 4");
}

- (void)testMediaItemMapsUnknownSubtype
{
    HeimdallMediaLibraryMediaItem * const unknown =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_UNKNOWN);
    XCTAssertEqualObjects(unknown.readableMediaSubType, @"Unknown Media Type");
}

- (void)testMediaItemReadableMediaType
{
    HeimdallMediaLibraryMediaItem * const audio =
        HeimdallLibraryDataTypesTestMediaItemWithTypeAndSubtype(HEIMDALL_ML_MEDIA_TYPE_AUDIO,
                                                           HEIMDALL_ML_MEDIA_SUBTYPE_UNKNOWN);
    XCTAssertEqualObjects(audio.readableMediaType, @"Audio");

    HeimdallMediaLibraryMediaItem * const unknown =
        HeimdallLibraryDataTypesTestMediaItemWithTypeAndSubtype(HEIMDALL_ML_MEDIA_TYPE_UNKNOWN,
                                                           HEIMDALL_ML_MEDIA_SUBTYPE_UNKNOWN);
    XCTAssertEqualObjects(unknown.readableMediaType, @"Unknown Media Type");
}

- (void)testMediaItemEnumeratesItself
{
    HeimdallMediaLibraryMediaItem * const item =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE);
    __block NSUInteger enumeratedCount = 0;
    [item enumerateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem *childItem, BOOL *stop) {
        XCTAssertEqual(childItem, item);
        XCTAssertFalse(*stop);
        enumeratedCount++;
    }];
    XCTAssertEqual(enumeratedCount, (NSUInteger)1);
}

- (void)testMediaItemIteratesItself
{
    HeimdallMediaLibraryMediaItem * const item =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE);
    __block NSUInteger iteratedCount = 0;
    [item iterateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem *childItem) {
        XCTAssertEqual(childItem, item);
        iteratedCount++;
    }];
    XCTAssertEqual(iteratedCount, (NSUInteger)1);
}

- (void)testMediaItemCollections
{
    HeimdallMediaLibraryMediaItem * const item =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE);
    XCTAssertEqual(item.firstMediaItem, item);
    XCTAssertEqualObjects(item.mediaItems, @[ item ]);
}

- (void)testMediaItemActionableDetailsDependOnSubtype
{
    HeimdallMediaLibraryMediaItem * const movie =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE);
    XCTAssertFalse(movie.primaryActionableDetail);
    XCTAssertFalse(movie.secondaryActionableDetail);
    XCTAssertNil(movie.primaryActionableDetailLibraryItem);
    XCTAssertNil(movie.secondaryActionableDetailLibraryItem);

    HeimdallMediaLibraryMediaItem * const albumTrack =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_ALBUMTRACK);
    XCTAssertTrue(albumTrack.primaryActionableDetail);
    XCTAssertTrue(albumTrack.secondaryActionableDetail);
}

- (void)testMediaItemSecureCoding
{
    HeimdallMediaLibraryMediaItem * const item =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE);
    XCTAssertTrue([HeimdallMediaLibraryMediaItem supportsSecureCoding]);
    NSKeyedArchiver * const archiver =
        [[NSKeyedArchiver alloc] initRequiringSecureCoding:NO];
    [item encodeWithCoder:archiver];
    [archiver finishEncoding];
    NSData * const data = archiver.encodedData;

    NSError *error = nil;
    NSKeyedUnarchiver * const unarchiver =
        [[NSKeyedUnarchiver alloc] initForReadingFromData:data error:&error];
    XCTAssertNil(error);
    XCTAssertEqual([unarchiver decodeInt64ForKey:@"HeimdallMediaLibraryMediaItemLibraryID"],
                   (int64_t)13);
    [unarchiver finishDecoding];
    XCTAssertNil([[HeimdallMediaLibraryMediaItem alloc] initWithCoder:nil]);
}

- (void)testMediaItemsFromPasteboardDataRejectsMalformedData
{
    NSData * const malformedData = [@"not a keyed archive"
        dataUsingEncoding:NSUTF8StringEncoding];

    XCTAssertNil([HeimdallMediaLibraryMediaItem mediaItemsFromPasteboardData:malformedData]);
}

- (void)testMediaItemsFromPasteboardDataRejectsUnexpectedArchiveRoot
{
    NSError *archiveError = nil;
    NSData * const data = [NSKeyedArchiver archivedDataWithRootObject:@{
        @"unexpected": @"root"
    } requiringSecureCoding:NO error:&archiveError];

    XCTAssertNil(archiveError);
    XCTAssertNotNil(data);
    XCTAssertNil([HeimdallMediaLibraryMediaItem mediaItemsFromPasteboardData:data]);
}

- (void)testMovieEmptyDefaults
{
    HeimdallMediaLibraryMovie * const emptyMovie =
        [[HeimdallMediaLibraryMovie alloc] initWithMediaItem:NULL];
    XCTAssertNotNil(emptyMovie);
    XCTAssertNil(emptyMovie.summary);
    XCTAssertNil(emptyMovie.imdbID);
    XCTAssertNil(emptyMovie.displayString);
}

- (void)testMovieDisplayStringUsesTitle
{
    struct heimdall_ml_media_t movieData = { 0 };
    movieData.psz_title = (char *)"Movie title";

    HeimdallMediaLibraryMovie * const movie =
        [[HeimdallMediaLibraryMovie alloc] initWithMediaItem:&movieData];

    XCTAssertEqualObjects(movie.displayString, @"Movie title");
}

- (void)testShowEmptyDefaults
{
    struct heimdall_ml_show_t emptyShowData = { 0 };
    HeimdallMediaLibraryShow * const emptyShow =
        [[HeimdallMediaLibraryShow alloc] initWithShow:&emptyShowData];
    XCTAssertNotNil(emptyShow);
    XCTAssertEqualObjects(emptyShow.name, @"");
    XCTAssertEqualObjects(emptyShow.summary, @"");
    XCTAssertEqualObjects(emptyShow.tvdbId, @"");
    XCTAssertEqualObjects(emptyShow.primaryDetailString, @"0 seasons, 0 episodes");
    XCTAssertEqualObjects(emptyShow.secondaryDetailString, @"Released in 0");
}

- (void)testEntryPointEmptyDefaults
{
    HeimdallMediaLibraryEntryPoint * const emptyEntryPoint =
        [[HeimdallMediaLibraryEntryPoint alloc] initWithEntryPoint:NULL];
    XCTAssertNotNil(emptyEntryPoint);
    XCTAssertNil(emptyEntryPoint.MRL);
    XCTAssertNil(emptyEntryPoint.decodedMRL);
    XCTAssertFalse(emptyEntryPoint.isPresent);
    XCTAssertFalse(emptyEntryPoint.isBanned);
}

- (void)testEmptyDummyItemDefaults
{
    HeimdallMediaLibraryDummyItem * const emptyItem =
        [[HeimdallMediaLibraryDummyItem alloc] initWithDisplayString:@"Empty"
                                           withPrimaryDetailString:nil
                                         withSecondaryDetailString:nil];
    XCTAssertEqualObjects(emptyItem.displayString, @"Empty");
    XCTAssertNil(emptyItem.primaryDetailString);
    XCTAssertNil(emptyItem.secondaryDetailString);
    XCTAssertNil(emptyItem.mediaItems);
    XCTAssertEqual(emptyItem.libraryID, (int64_t)-1);
    XCTAssertFalse(emptyItem.isFileBacked);
}

- (void)testEmptyDummyItemCannotSetFavorite
{
    HeimdallMediaLibraryDummyItem * const emptyItem =
        [[HeimdallMediaLibraryDummyItem alloc] initWithDisplayString:@"Empty"
                                           withPrimaryDetailString:nil
                                         withSecondaryDetailString:nil];
    XCTAssertEqual([emptyItem setFavorite:YES], HEIMDALL_EGENERIC);
}

- (void)testDummyItemStoresItsChildren
{
    HeimdallMediaLibraryMediaItem * const first =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE);
    HeimdallMediaLibraryMediaItem * const second =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_SHOW_EPISODE);
    HeimdallMediaLibraryDummyItem * const item =
        [[HeimdallMediaLibraryDummyItem alloc] initWithDisplayString:@"Collection"
                                                 withMediaItems:@[ first, second ]];

    XCTAssertEqualObjects(item.primaryDetailString, @"2 items");
    XCTAssertEqualObjects(item.secondaryDetailString, @"");
    XCTAssertEqualObjects(item.mediaItems, (@[ first, second ]));
    XCTAssertEqual(item.firstMediaItem, first);
}

- (void)testDummyItemEnumeratesItsChildren
{
    HeimdallMediaLibraryMediaItem * const first =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE);
    HeimdallMediaLibraryMediaItem * const second =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_SHOW_EPISODE);
    HeimdallMediaLibraryDummyItem * const item =
        [[HeimdallMediaLibraryDummyItem alloc] initWithDisplayString:@"Collection"
                                                 withMediaItems:@[ first, second ]];
    NSMutableArray<HeimdallMediaLibraryMediaItem *> * enumerated = NSMutableArray.array;
    [item enumerateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem *childItem, BOOL *stop) {
        [enumerated addObject:childItem];
        *stop = childItem == first;
    }];
    XCTAssertEqualObjects(enumerated, @[ first ]);
}

- (void)testDummyItemIteratesItsChildren
{
    HeimdallMediaLibraryMediaItem * const first =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE);
    HeimdallMediaLibraryMediaItem * const second =
        HeimdallLibraryDataTypesTestMediaItemWithSubtype(HEIMDALL_ML_MEDIA_SUBTYPE_SHOW_EPISODE);
    HeimdallMediaLibraryDummyItem * const item =
        [[HeimdallMediaLibraryDummyItem alloc] initWithDisplayString:@"Collection"
                                                 withMediaItems:@[ first, second ]];
    NSMutableArray<HeimdallMediaLibraryMediaItem *> * const enumerated = NSMutableArray.array;
    [item iterateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem *childItem) {
        [enumerated addObject:childItem];
    }];
    XCTAssertEqualObjects(enumerated, (@[ first, second ]));
}

- (void)testDummyItemMoveToTrashMovesChildMediaItems
{
    HeimdallInputItemTestResetAppKitState();
    HeimdallLibraryDataTypesTestResetTrashState();
    HeimdallMediaLibraryMediaItem * const child =
        HeimdallLibraryDataTypesTestMediaItemWithInputMetadata(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE, nil);
    NSURL * const sourceURL = child.files.firstObject.fileURL;
    XCTAssertTrue([[NSFileManager defaultManager] createFileAtPath:sourceURL.path
                                                           contents:[NSData data]
                                                         attributes:nil]);
    HeimdallMediaLibraryDummyItem * const item =
        [[HeimdallMediaLibraryDummyItem alloc] initWithDisplayString:@"Collection"
                                                 withMediaItems:@[child]];

    [item moveToTrash];

    XCTAssertEqualObjects(HeimdallLibraryDataTypesTestTrashedSourceURLs(), @[sourceURL]);
    XCTAssertFalse([[NSFileManager defaultManager] fileExistsAtPath:sourceURL.path]);
    XCTAssertEqual(HeimdallLibraryDataTypesTestTrashDestinationURLs().count, (NSUInteger)1);
    XCTAssertTrue(HeimdallInputItemTestDidReload());
    HeimdallLibraryDataTypesTestResetTrashState();
    [[NSFileManager defaultManager] removeItemAtURL:sourceURL error:nil];
}

- (void)testDummyItemMoveToTrashMovesAllChildMediaItems
{
    HeimdallInputItemTestResetAppKitState();
    HeimdallLibraryDataTypesTestResetTrashState();
    NSURL * const firstURL = [NSURL fileURLWithPath:
                              [NSTemporaryDirectory() stringByAppendingPathComponent:
                               @"heimdall-datatypes-first-child-file.mp4"]];
    NSURL * const secondURL = [NSURL fileURLWithPath:
                               [NSTemporaryDirectory() stringByAppendingPathComponent:
                                @"heimdall-datatypes-second-child-file.mp4"]];
    XCTAssertTrue([[NSFileManager defaultManager] createFileAtPath:firstURL.path
                                                              contents:[NSData data]
                                                            attributes:nil]);
    XCTAssertTrue([[NSFileManager defaultManager] createFileAtPath:secondURL.path
                                                              contents:[NSData data]
                                                            attributes:nil]);
    HeimdallMediaLibraryMediaItem * const firstChild =
        HeimdallLibraryDataTypesTestMediaItemWithInputMetadataAndFileURLs(
            HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE,
            nil,
            @[ firstURL ]);
    HeimdallMediaLibraryMediaItem * const secondChild =
        HeimdallLibraryDataTypesTestMediaItemWithInputMetadataAndFileURLs(
            HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE,
            nil,
            @[ secondURL ]);
    HeimdallMediaLibraryDummyItem * const item =
        [[HeimdallMediaLibraryDummyItem alloc] initWithDisplayString:@"Collection"
                                                 withMediaItems:@[ firstChild, secondChild ]];

    [item moveToTrash];

    XCTAssertEqualObjects([NSSet setWithArray:HeimdallLibraryDataTypesTestTrashedSourceURLs()],
                          ([NSSet setWithObjects:firstURL, secondURL, nil]));
    XCTAssertFalse([[NSFileManager defaultManager] fileExistsAtPath:firstURL.path]);
    XCTAssertFalse([[NSFileManager defaultManager] fileExistsAtPath:secondURL.path]);
    XCTAssertEqual(HeimdallLibraryDataTypesTestTrashDestinationURLs().count, (NSUInteger)2);
    XCTAssertTrue(HeimdallInputItemTestDidReload());
    HeimdallLibraryDataTypesTestResetTrashState();
    [[NSFileManager defaultManager] removeItemAtURL:firstURL error:nil];
    [[NSFileManager defaultManager] removeItemAtURL:secondURL error:nil];
}

- (void)testMediaItemMoveToTrashTrashesFilesAndReloadsLibrary
{
    HeimdallInputItemTestResetAppKitState();
    HeimdallLibraryDataTypesTestResetTrashState();
    HeimdallMediaLibraryMediaItem * const item =
        HeimdallLibraryDataTypesTestMediaItemWithInputMetadata(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE, nil);
    NSURL * const sourceURL = item.files.firstObject.fileURL;
    XCTAssertTrue([[NSFileManager defaultManager] createFileAtPath:sourceURL.path
                                                           contents:[NSData data]
                                                         attributes:nil]);

    [item moveToTrash];

    XCTAssertEqualObjects(HeimdallLibraryDataTypesTestTrashedSourceURLs(), @[sourceURL]);
    XCTAssertFalse([[NSFileManager defaultManager] fileExistsAtPath:sourceURL.path]);
    XCTAssertEqual(HeimdallLibraryDataTypesTestTrashDestinationURLs().count, (NSUInteger)1);
    XCTAssertTrue(HeimdallInputItemTestDidReload());
    HeimdallLibraryDataTypesTestResetTrashState();
    [[NSFileManager defaultManager] removeItemAtURL:sourceURL error:nil];
}

- (void)testMediaItemMoveToTrashTrashesEveryFile
{
    HeimdallInputItemTestResetAppKitState();
    HeimdallLibraryDataTypesTestResetTrashState();
    NSURL * const firstURL = [NSURL fileURLWithPath:
                              [NSTemporaryDirectory() stringByAppendingPathComponent:
                               @"heimdall-datatypes-first-media-file.mp4"]];
    NSURL * const secondURL = [NSURL fileURLWithPath:
                               [NSTemporaryDirectory() stringByAppendingPathComponent:
                                @"heimdall-datatypes-second-media-file.mp4"]];
    XCTAssertTrue([[NSFileManager defaultManager] createFileAtPath:firstURL.path
                                                              contents:[NSData data]
                                                            attributes:nil]);
    XCTAssertTrue([[NSFileManager defaultManager] createFileAtPath:secondURL.path
                                                              contents:[NSData data]
                                                            attributes:nil]);
    HeimdallMediaLibraryMediaItem * const item =
        HeimdallLibraryDataTypesTestMediaItemWithInputMetadataAndFileURLs(
            HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE,
            nil,
            @[ firstURL, secondURL ]);

    [item moveToTrash];

    XCTAssertEqualObjects([NSSet setWithArray:HeimdallLibraryDataTypesTestTrashedSourceURLs()],
                          ([NSSet setWithObjects:firstURL, secondURL, nil]));
    XCTAssertFalse([[NSFileManager defaultManager] fileExistsAtPath:firstURL.path]);
    XCTAssertFalse([[NSFileManager defaultManager] fileExistsAtPath:secondURL.path]);
    XCTAssertEqual(HeimdallLibraryDataTypesTestTrashDestinationURLs().count, (NSUInteger)2);
    XCTAssertTrue(HeimdallInputItemTestDidReload());
    HeimdallLibraryDataTypesTestResetTrashState();
    [[NSFileManager defaultManager] removeItemAtURL:firstURL error:nil];
    [[NSFileManager defaultManager] removeItemAtURL:secondURL error:nil];
}

- (void)testMediaItemMoveToTrashReloadsWhenTrashFails
{
    HeimdallInputItemTestResetAppKitState();
    HeimdallLibraryDataTypesTestResetTrashState();
    HeimdallLibraryDataTypesTestSetTrashFailure(YES);
    HeimdallMediaLibraryMediaItem * const item =
        HeimdallLibraryDataTypesTestMediaItemWithInputMetadata(HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE, nil);
    NSURL * const sourceURL = item.files.firstObject.fileURL;
    XCTAssertTrue([[NSFileManager defaultManager] createFileAtPath:sourceURL.path
                                                              contents:[NSData data]
                                                            attributes:nil]);

    [item moveToTrash];

    XCTAssertEqualObjects(HeimdallLibraryDataTypesTestTrashedSourceURLs(), @[sourceURL]);
    XCTAssertTrue([[NSFileManager defaultManager] fileExistsAtPath:sourceURL.path]);
    XCTAssertEqual(HeimdallLibraryDataTypesTestTrashDestinationURLs().count, (NSUInteger)0);
    XCTAssertTrue(HeimdallInputItemTestDidReload());
    HeimdallLibraryDataTypesTestResetTrashState();
    [[NSFileManager defaultManager] removeItemAtURL:sourceURL error:nil];
}

- (void)testMediaItemMoveToTrashContinuesAfterPartialFailure
{
    HeimdallInputItemTestResetAppKitState();
    HeimdallLibraryDataTypesTestResetTrashState();
    NSURL * const firstURL = [NSURL fileURLWithPath:
                              [NSTemporaryDirectory() stringByAppendingPathComponent:
                               @"heimdall-datatypes-partial-first-file.mp4"]];
    NSURL * const secondURL = [NSURL fileURLWithPath:
                               [NSTemporaryDirectory() stringByAppendingPathComponent:
                                @"heimdall-datatypes-partial-second-file.mp4"]];
    XCTAssertTrue([[NSFileManager defaultManager] createFileAtPath:firstURL.path
                                                              contents:[NSData data]
                                                            attributes:nil]);
    XCTAssertTrue([[NSFileManager defaultManager] createFileAtPath:secondURL.path
                                                              contents:[NSData data]
                                                            attributes:nil]);
    HeimdallMediaLibraryMediaItem * const item =
        HeimdallLibraryDataTypesTestMediaItemWithInputMetadataAndFileURLs(
            HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE,
            nil,
            @[ firstURL, secondURL ]);
    HeimdallLibraryDataTypesTestSetTrashFailureOnCall(2);

    [item moveToTrash];

    XCTAssertEqualObjects(HeimdallLibraryDataTypesTestTrashedSourceURLs(),
                          (@[ firstURL, secondURL ]));
    XCTAssertFalse([[NSFileManager defaultManager] fileExistsAtPath:firstURL.path]);
    XCTAssertTrue([[NSFileManager defaultManager] fileExistsAtPath:secondURL.path]);
    XCTAssertEqual(HeimdallLibraryDataTypesTestTrashDestinationURLs().count, (NSUInteger)1);
    XCTAssertTrue(HeimdallInputItemTestDidReload());
    HeimdallLibraryDataTypesTestResetTrashState();
    [[NSFileManager defaultManager] removeItemAtURL:firstURL error:nil];
    [[NSFileManager defaultManager] removeItemAtURL:secondURL error:nil];
}

@end

#pragma clang diagnostic pop
