/****************************************************************************
 * HeimdallLibraryDataTypesIntegrationTestSupport.h: medialibrary test lifecycle
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

#import <heimdall_media_library.h>

BOOL HeimdallLibraryDataTypesIntegrationStart(void);
void HeimdallLibraryDataTypesIntegrationStop(void);
heimdall_medialibrary_t *HeimdallLibraryDataTypesIntegrationMediaLibrary(void);
int64_t HeimdallLibraryDataTypesIntegrationMediaID(void);
int64_t HeimdallLibraryDataTypesIntegrationCreateExternalMedia(const char *mrl);
int64_t HeimdallLibraryDataTypesIntegrationCreatePlaylist(void);
BOOL HeimdallLibraryDataTypesIntegrationPrepareFactoryFixtures(void);
BOOL HeimdallLibraryDataTypesIntegrationPrepareMovieFixture(void);
int64_t HeimdallLibraryDataTypesIntegrationFactoryAudioMediaID(void);
int64_t HeimdallLibraryDataTypesIntegrationFactoryVideoMediaID(void);
int64_t HeimdallLibraryDataTypesIntegrationFactoryMovieMediaID(void);
int64_t HeimdallLibraryDataTypesIntegrationAlbumID(void);
int64_t HeimdallLibraryDataTypesIntegrationGenreID(void);
int64_t HeimdallLibraryDataTypesIntegrationShowID(void);
int64_t HeimdallLibraryDataTypesIntegrationGroupID(void);
