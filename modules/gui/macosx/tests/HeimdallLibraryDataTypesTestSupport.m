/****************************************************************************
 * HeimdallLibraryDataTypesTestSupport.m: Heimdall library data type test fixtures
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
 ****************************************************************************/

#import "HeimdallLibraryDataTypesTestSupport.h"

#import "library/HeimdallInputItem.h"
#import <objc/runtime.h>

@interface NSFileManager (HeimdallLibraryDataTypesTestSupport)
- (BOOL)heimdall_test_trashItemAtURL:(NSURL *)url
                resultingItemURL:(NSURL **)resultingURL
                           error:(NSError **)error;
@end

@implementation NSFileManager (HeimdallLibraryDataTypesTestSupport)

+ (void)load
{
    Method original = class_getInstanceMethod(self, @selector(trashItemAtURL:resultingItemURL:error:));
    Method replacement = class_getInstanceMethod(self, @selector(heimdall_test_trashItemAtURL:resultingItemURL:error:));
    method_exchangeImplementations(original, replacement);
}

- (BOOL)heimdall_test_trashItemAtURL:(NSURL *)url
                resultingItemURL:(NSURL **)resultingURL
                           error:(NSError **)error
{
    return HeimdallLibraryDataTypesTestMoveItemToTrash(url, resultingURL, error);
}

@end

static NSMutableArray<NSURL *> *sTrashedSourceURLs;
static NSMutableArray<NSURL *> *sTrashDestinationURLs;
static NSMutableDictionary<NSURL *, NSURL *> *sTrashSourceForDestination;
static NSUInteger sTrashCallCount;
static NSUInteger sTrashFailureCall;

static void HeimdallLibraryDataTypesTestEnsureTrashState(void)
{
    if (sTrashedSourceURLs == nil) {
        sTrashedSourceURLs = NSMutableArray.array;
        sTrashDestinationURLs = NSMutableArray.array;
        sTrashSourceForDestination = NSMutableDictionary.dictionary;
    }
}

void HeimdallLibraryDataTypesTestResetTrashState(void)
{
    HeimdallLibraryDataTypesTestEnsureTrashState();
    for (NSURL * const destinationURL in sTrashDestinationURLs) {
        NSURL * const sourceURL = sTrashSourceForDestination[destinationURL];
        if ([[NSFileManager defaultManager] fileExistsAtPath:destinationURL.path]) {
            [[NSFileManager defaultManager] moveItemAtURL:destinationURL
                                                    toURL:sourceURL
                                                    error:nil];
        }
    }
    [sTrashedSourceURLs removeAllObjects];
    [sTrashDestinationURLs removeAllObjects];
    [sTrashSourceForDestination removeAllObjects];
    sTrashCallCount = 0;
    sTrashFailureCall = 0;
}

void HeimdallLibraryDataTypesTestSetTrashFailure(BOOL shouldFail)
{
    HeimdallLibraryDataTypesTestEnsureTrashState();
    sTrashFailureCall = shouldFail ? 1 : 0;
}

void HeimdallLibraryDataTypesTestSetTrashFailureOnCall(NSUInteger callNumber)
{
    HeimdallLibraryDataTypesTestEnsureTrashState();
    sTrashFailureCall = callNumber;
}

NSArray<NSURL *> *HeimdallLibraryDataTypesTestTrashedSourceURLs(void)
{
    HeimdallLibraryDataTypesTestEnsureTrashState();
    return [sTrashedSourceURLs copy];
}

NSArray<NSURL *> *HeimdallLibraryDataTypesTestTrashDestinationURLs(void)
{
    HeimdallLibraryDataTypesTestEnsureTrashState();
    return [sTrashDestinationURLs copy];
}

BOOL HeimdallLibraryDataTypesTestMoveItemToTrash(NSURL *url,
                                            NSURL * _Nullable * _Nullable resultingURL,
                                            NSError * _Nullable * _Nullable error)
{
    HeimdallLibraryDataTypesTestEnsureTrashState();
    [sTrashedSourceURLs addObject:url];
    sTrashCallCount++;

    if (sTrashFailureCall != 0 && sTrashCallCount == sTrashFailureCall) {
        if (resultingURL != NULL) {
            *resultingURL = nil;
        }
        if (error != NULL) {
            *error = [NSError errorWithDomain:NSCocoaErrorDomain
                                         code:NSFileWriteUnknownError
                                     userInfo:nil];
        }
        return NO;
    }

    if (![[NSFileManager defaultManager] fileExistsAtPath:url.path]) {
        if (resultingURL != NULL) {
            *resultingURL = nil;
        }
        if (error != NULL) {
            *error = nil;
        }
        return YES;
    }

    NSURL * const trashDirectoryURL =
        [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:
                                @"heimdall-datatypes-test-trash"] isDirectory:YES];
    [[NSFileManager defaultManager] createDirectoryAtURL:trashDirectoryURL
                              withIntermediateDirectories:YES
                                               attributes:nil
                                                    error:nil];
    NSURL * const destinationURL = [trashDirectoryURL URLByAppendingPathComponent:[NSUUID UUID].UUIDString];
    NSError *moveError = nil;
    const BOOL moved = [[NSFileManager defaultManager] moveItemAtURL:url
                                                               toURL:destinationURL
                                                               error:&moveError];
    if (moved) {
        [sTrashDestinationURLs addObject:destinationURL];
        sTrashSourceForDestination[destinationURL] = url;
    }
    if (resultingURL != NULL) {
        *resultingURL = moved ? destinationURL : nil;
    }
    if (error != NULL) {
        *error = moveError;
    }
    return moved;
}

NSString * const HeimdallLibraryDataTypesTestInputItemNameKey = @"name";
NSString * const HeimdallLibraryDataTypesTestInputItemTitleKey = @"title";
NSString * const HeimdallLibraryDataTypesTestInputItemArtistKey = @"artist";
NSString * const HeimdallLibraryDataTypesTestInputItemAlbumKey = @"album";
NSString * const HeimdallLibraryDataTypesTestInputItemTrackNumberKey = @"trackNumber";
NSString * const HeimdallLibraryDataTypesTestInputItemGenreKey = @"genre";
NSString * const HeimdallLibraryDataTypesTestInputItemCopyrightKey = @"copyright";
NSString * const HeimdallLibraryDataTypesTestInputItemPublisherKey = @"publisher";
NSString * const HeimdallLibraryDataTypesTestInputItemLanguageKey = @"language";
NSString * const HeimdallLibraryDataTypesTestInputItemDateKey = @"date";
NSString * const HeimdallLibraryDataTypesTestInputItemDescriptionKey = @"contentDescription";
NSString * const HeimdallLibraryDataTypesTestInputItemDirectorKey = @"director";
NSString * const HeimdallLibraryDataTypesTestInputItemShowNameKey = @"showName";
NSString * const HeimdallLibraryDataTypesTestInputItemActorsKey = @"actors";
NSString * const HeimdallLibraryDataTypesTestInputItemArtworkURLKey = @"artworkURL";

@interface HeimdallMediaLibraryMediaItem (HeimdallLibraryDataTypesTestPrivate)
- (instancetype)initWithMediaItem:(struct heimdall_ml_media_t *)mediaItem
                          library:(heimdall_medialibrary_t *)mediaLibrary;
@end

static HeimdallMediaLibraryMediaItem *HeimdallLibraryDataTypesTestMediaItemWithTypeAndSubtypeAndTitle(
    heimdall_ml_media_type_t type,
    heimdall_ml_media_subtype_t subtype,
    const char *title,
    HeimdallInputItem * _Nullable inputItem,
    NSArray<NSURL *> * _Nullable fileURLs);

@interface HeimdallLibraryDataTypesTestMediaItem : HeimdallMediaLibraryMediaItem
@property (nonatomic, strong) HeimdallInputItem *testInputItem;
@end

@implementation HeimdallLibraryDataTypesTestMediaItem

- (HeimdallInputItem *)inputItem
{
    return self.testInputItem;
}

@end

HeimdallMediaLibraryMediaItem *HeimdallLibraryDataTypesTestMediaItemWithSubtype(heimdall_ml_media_subtype_t subtype)
{
    return HeimdallLibraryDataTypesTestMediaItemWithTypeAndSubtype(HEIMDALL_ML_MEDIA_TYPE_VIDEO, subtype);
}

HeimdallMediaLibraryMediaItem *HeimdallLibraryDataTypesTestMediaItemWithEmptyTitle(heimdall_ml_media_subtype_t subtype)
{
    return HeimdallLibraryDataTypesTestMediaItemWithTypeAndSubtypeAndTitle(HEIMDALL_ML_MEDIA_TYPE_VIDEO,
                                                                       subtype,
                                                                       "",
                                                                       nil,
                                                                       nil);
}

HeimdallMediaLibraryMediaItem *HeimdallLibraryDataTypesTestMediaItemWithTypeAndSubtype(heimdall_ml_media_type_t type,
                                                                             heimdall_ml_media_subtype_t subtype)
{
    return HeimdallLibraryDataTypesTestMediaItemWithTypeAndSubtypeAndTitle(type,
                                                                       subtype,
                                                                       "Media",
                                                                       nil,
                                                                       nil);
}

static HeimdallMediaLibraryMediaItem *HeimdallLibraryDataTypesTestMediaItemWithTypeAndSubtypeAndTitle(
    heimdall_ml_media_type_t type,
    heimdall_ml_media_subtype_t subtype,
    const char *title,
    HeimdallInputItem * _Nullable inputItem,
    NSArray<NSURL *> * _Nullable fileURLs)
{
    NSArray<NSURL *> * const resolvedFileURLs = fileURLs != nil
        ? fileURLs
        : @[ [NSURL fileURLWithPath:@"/tmp/media.mp4"] ];
    NSCAssert(resolvedFileURLs.count <= 2, @"Test fixture supports at most two files");

    struct TestFileList {
        size_t i_nb_items;
        heimdall_ml_file_t p_items[2];
    } files = { 0 };
    files.i_nb_items = resolvedFileURLs.count;
    for (NSUInteger index = 0; index < resolvedFileURLs.count; ++index) {
        files.p_items[index].psz_mrl = (char *)resolvedFileURLs[index].absoluteString.UTF8String;
        files.p_items[index].i_type = index == 0 ? HEIMDALL_ML_FILE_TYPE_MAIN : HEIMDALL_ML_FILE_TYPE_UNKNOWN;
    }

    struct TestTrackList {
        size_t i_nb_items;
        heimdall_ml_media_track_t p_items[2];
    } tracks = { 0 };
    tracks.i_nb_items = 2;
    tracks.p_items[0].psz_codec = (char *)"avc1";
    tracks.p_items[0].i_type = HEIMDALL_ML_TRACK_TYPE_VIDEO;
    tracks.p_items[0].v.i_width = 1920;
    tracks.p_items[0].v.i_height = 1080;
    tracks.p_items[0].v.i_sarNum = 16;
    tracks.p_items[0].v.i_sarDen = 9;
    tracks.p_items[0].v.i_fpsNum = 24;
    tracks.p_items[0].v.i_fpsDen = 1;
    tracks.p_items[1].psz_codec = (char *)"mp4a";
    tracks.p_items[1].i_type = HEIMDALL_ML_TRACK_TYPE_AUDIO;

    struct heimdall_ml_media_t media = { 0 };
    media.i_id = 13;
    media.i_type = type;
    media.i_subtype = subtype;
    media.p_files = (heimdall_ml_file_list_t *)&files;
    media.p_tracks = (heimdall_ml_media_track_list_t *)&tracks;
    media.i_year = 2024;
    media.i_duration = 123000;
    media.i_playcount = 2;
    media.f_progress = .5;
    media.i_last_played_date = 4567;
    media.psz_title = (char *)title;
    media.thumbnails[HEIMDALL_ML_THUMBNAIL_SMALL].psz_mrl = (char *)"file:///tmp/media.jpg";
    media.b_is_favorite = true;

    if (subtype == HEIMDALL_ML_MEDIA_SUBTYPE_SHOW_EPISODE) {
        media.show_episode.i_episode_nb = 4;
        media.show_episode.i_season_number = 2;
    } else if (subtype == HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE) {
        media.movie.psz_summary = (char *)"Movie summary";
        media.movie.psz_imdb_id = (char *)"tt123";
    } else if (subtype == HEIMDALL_ML_MEDIA_SUBTYPE_ALBUMTRACK) {
        media.album_track.i_artist_id = 7;
        media.album_track.i_album_id = 8;
        media.album_track.i_genre_id = 9;
        media.album_track.i_track_nb = 3;
        media.album_track.i_disc_nb = 1;
    }

    if (inputItem == nil) {
        return [[HeimdallMediaLibraryMediaItem alloc]
            initWithMediaItem:&media library:(heimdall_medialibrary_t *)0x1];
    }

    HeimdallLibraryDataTypesTestMediaItem * const item =
            [[HeimdallLibraryDataTypesTestMediaItem alloc]
            initWithMediaItem:&media
                      library:(heimdall_medialibrary_t *)0x1];
    item.testInputItem = inputItem;
    return item;
}

HeimdallMediaLibraryMediaItem *HeimdallLibraryDataTypesTestMediaItemWithInputMetadata(
    heimdall_ml_media_subtype_t subtype,
    NSDictionary<NSString *, id> * _Nullable metadata)
{
    return HeimdallLibraryDataTypesTestMediaItemWithInputMetadataAndFileURLs(subtype,
                                                                         metadata,
                                                                         nil);
}

HeimdallMediaLibraryMediaItem *HeimdallLibraryDataTypesTestMediaItemWithInputMetadataAndFileURLs(
    heimdall_ml_media_subtype_t subtype,
    NSDictionary<NSString *, id> * _Nullable metadata,
    NSArray<NSURL *> * _Nullable fileURLs)
{
    NSDictionary<NSString *, id> * const defaultMetadata = @{
        HeimdallLibraryDataTypesTestInputItemNameKey: @"Detail test item",
        HeimdallLibraryDataTypesTestInputItemTitleKey: @"Detail test item",
        HeimdallLibraryDataTypesTestInputItemArtistKey: @"Detail Artist",
        HeimdallLibraryDataTypesTestInputItemAlbumKey: @"Detail Album",
        HeimdallLibraryDataTypesTestInputItemTrackNumberKey: @"1",
        HeimdallLibraryDataTypesTestInputItemGenreKey: @"Detail Genre",
        HeimdallLibraryDataTypesTestInputItemCopyrightKey: @"Detail Copyright",
        HeimdallLibraryDataTypesTestInputItemPublisherKey: @"Detail Publisher",
        HeimdallLibraryDataTypesTestInputItemLanguageKey: @"en",
        HeimdallLibraryDataTypesTestInputItemDescriptionKey: @"Detail Description",
        HeimdallLibraryDataTypesTestInputItemActorsKey: @"Detail Actor",
        HeimdallLibraryDataTypesTestInputItemArtworkURLKey: [NSURL fileURLWithPath:@"/tmp/detail-test.jpg"],
    };
    NSMutableDictionary<NSString *, id> * const resolvedMetadata =
        [defaultMetadata mutableCopy];
    [resolvedMetadata addEntriesFromDictionary:metadata ?: @{}];

    HeimdallInputItem * const inputItem =
        [HeimdallInputItem inputItemFromURL:[NSURL URLWithString:@"file:///tmp/detail-test.mp4"]];
    inputItem.name = resolvedMetadata[HeimdallLibraryDataTypesTestInputItemNameKey];
    inputItem.title = resolvedMetadata[HeimdallLibraryDataTypesTestInputItemTitleKey];
    inputItem.artist = resolvedMetadata[HeimdallLibraryDataTypesTestInputItemArtistKey];
    inputItem.album = resolvedMetadata[HeimdallLibraryDataTypesTestInputItemAlbumKey];
    inputItem.trackNumber = resolvedMetadata[HeimdallLibraryDataTypesTestInputItemTrackNumberKey];
    inputItem.genre = resolvedMetadata[HeimdallLibraryDataTypesTestInputItemGenreKey];
    inputItem.copyright = resolvedMetadata[HeimdallLibraryDataTypesTestInputItemCopyrightKey];
    inputItem.publisher = resolvedMetadata[HeimdallLibraryDataTypesTestInputItemPublisherKey];
    inputItem.language = resolvedMetadata[HeimdallLibraryDataTypesTestInputItemLanguageKey];
    inputItem.date = resolvedMetadata[HeimdallLibraryDataTypesTestInputItemDateKey];
    inputItem.contentDescription = resolvedMetadata[HeimdallLibraryDataTypesTestInputItemDescriptionKey];
    inputItem.director = resolvedMetadata[HeimdallLibraryDataTypesTestInputItemDirectorKey];
    inputItem.showName = resolvedMetadata[HeimdallLibraryDataTypesTestInputItemShowNameKey];
    inputItem.actors = resolvedMetadata[HeimdallLibraryDataTypesTestInputItemActorsKey];
    inputItem.artworkURL = resolvedMetadata[HeimdallLibraryDataTypesTestInputItemArtworkURLKey];

    const heimdall_ml_media_type_t type = subtype == HEIMDALL_ML_MEDIA_SUBTYPE_ALBUMTRACK
        ? HEIMDALL_ML_MEDIA_TYPE_AUDIO
        : HEIMDALL_ML_MEDIA_TYPE_VIDEO;
    return HeimdallLibraryDataTypesTestMediaItemWithTypeAndSubtypeAndTitle(type,
                                                                       subtype,
                                                                       "Detail test item",
                                                                       inputItem,
                                                                       fileURLs);
}
