/****************************************************************************
 * HeimdallLibraryDataTypesTestSupport.h: Heimdall library data type test support
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

#import "library/HeimdallLibraryDataTypes.h"

HeimdallMediaLibraryMediaItem * _Nonnull HeimdallLibraryDataTypesTestMediaItemWithSubtype(
    heimdall_ml_media_subtype_t subtype);
HeimdallMediaLibraryMediaItem * _Nonnull HeimdallLibraryDataTypesTestMediaItemWithEmptyTitle(
    heimdall_ml_media_subtype_t subtype);
HeimdallMediaLibraryMediaItem * _Nonnull HeimdallLibraryDataTypesTestMediaItemWithTypeAndSubtype(
    heimdall_ml_media_type_t type,
    heimdall_ml_media_subtype_t subtype);

FOUNDATION_EXPORT NSString * _Nonnull const HeimdallLibraryDataTypesTestInputItemNameKey;
FOUNDATION_EXPORT NSString * _Nonnull const HeimdallLibraryDataTypesTestInputItemTitleKey;
FOUNDATION_EXPORT NSString * _Nonnull const HeimdallLibraryDataTypesTestInputItemArtistKey;
FOUNDATION_EXPORT NSString * _Nonnull const HeimdallLibraryDataTypesTestInputItemAlbumKey;
FOUNDATION_EXPORT NSString * _Nonnull const HeimdallLibraryDataTypesTestInputItemTrackNumberKey;
FOUNDATION_EXPORT NSString * _Nonnull const HeimdallLibraryDataTypesTestInputItemGenreKey;
FOUNDATION_EXPORT NSString * _Nonnull const HeimdallLibraryDataTypesTestInputItemCopyrightKey;
FOUNDATION_EXPORT NSString * _Nonnull const HeimdallLibraryDataTypesTestInputItemPublisherKey;
FOUNDATION_EXPORT NSString * _Nonnull const HeimdallLibraryDataTypesTestInputItemLanguageKey;
FOUNDATION_EXPORT NSString * _Nonnull const HeimdallLibraryDataTypesTestInputItemDateKey;
FOUNDATION_EXPORT NSString * _Nonnull const HeimdallLibraryDataTypesTestInputItemDescriptionKey;
FOUNDATION_EXPORT NSString * _Nonnull const HeimdallLibraryDataTypesTestInputItemDirectorKey;
FOUNDATION_EXPORT NSString * _Nonnull const HeimdallLibraryDataTypesTestInputItemShowNameKey;
FOUNDATION_EXPORT NSString * _Nonnull const HeimdallLibraryDataTypesTestInputItemActorsKey;
FOUNDATION_EXPORT NSString * _Nonnull const HeimdallLibraryDataTypesTestInputItemArtworkURLKey;

HeimdallMediaLibraryMediaItem * _Nonnull HeimdallLibraryDataTypesTestMediaItemWithInputMetadata(
    heimdall_ml_media_subtype_t subtype,
    NSDictionary<NSString *, id> * _Nullable metadata);

HeimdallMediaLibraryMediaItem * _Nonnull HeimdallLibraryDataTypesTestMediaItemWithInputMetadataAndFileURLs(
    heimdall_ml_media_subtype_t subtype,
    NSDictionary<NSString *, id> * _Nullable metadata,
    NSArray<NSURL *> * _Nullable fileURLs);

void HeimdallLibraryDataTypesTestResetTrashState(void);
void HeimdallLibraryDataTypesTestSetTrashFailure(BOOL shouldFail);
void HeimdallLibraryDataTypesTestSetTrashFailureOnCall(NSUInteger callNumber);
NSArray<NSURL *> * _Nonnull HeimdallLibraryDataTypesTestTrashedSourceURLs(void);
NSArray<NSURL *> * _Nonnull HeimdallLibraryDataTypesTestTrashDestinationURLs(void);
BOOL HeimdallLibraryDataTypesTestMoveItemToTrash(NSURL * _Nonnull url,
                                            NSURL * _Nullable * _Nullable resultingURL,
                                            NSError * _Nullable * _Nullable error);
