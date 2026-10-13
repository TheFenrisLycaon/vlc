/*****************************************************************************
 * HeimdallMediaSourceProvider.m: MacOS X interface module
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

#import "HeimdallMediaSourceProvider.h"

#import "HeimdallMediaSource.h"
#import "HeimdallLocalMediaSource.h"

#import "main/HeimdallMain.h"

#import <heimdall_media_source.h>

static inline void getMediaSourcesForCategory(NSMutableArray <HeimdallMediaSource *> *const restrict __unsafe_unretained outputArray,
                                              heimdall_preparser_t * restrict p_preparser,
                                              heimdall_media_source_provider_t *const restrict p_sourceProvider,
                                              enum services_discovery_category_e const category)
{
    heimdall_media_source_meta_list_t *const p_sourceMetaList = heimdall_media_source_provider_List(p_sourceProvider,
                                                                                          category);
    const size_t count = heimdall_media_source_meta_list_Count(p_sourceMetaList);

    for (size_t i = 0; i < count; ++i) {
        struct heimdall_media_source_meta *const p_sourceMetaItem = heimdall_media_source_meta_list_Get(p_sourceMetaList, i);

        heimdall_media_source_t *const p_mediaSource = heimdall_media_source_provider_GetMediaSource(p_sourceProvider, p_sourceMetaItem->name);
        if (p_mediaSource == NULL) {
            continue;
        }

        HeimdallMediaSource *const mediaSource = [[HeimdallMediaSource alloc] initWithMediaSource:p_mediaSource andPreparser:p_preparser forCategory:category];
        [outputArray addObject:mediaSource];
    }

    heimdall_media_source_meta_list_Delete(p_sourceMetaList);
}

@implementation HeimdallMediaSourceProvider

+ (NSArray <HeimdallMediaSource *> *)listOfMediaSourcesForCategory:(enum services_discovery_category_e)category
{
    libheimdall_int_t *p_libheimdallInstance = heimdall_object_instance(getIntf());
    heimdall_preparser_t *p_preparser = getNetworkPreparser();
    heimdall_media_source_provider_t *p_sourceProvider = heimdall_media_source_provider_Get(p_libheimdallInstance);

    if (p_sourceProvider == NULL) {
        return @[];
    }

    NSMutableArray *mutableArray = [[NSMutableArray alloc] initWithCapacity:32]; // A sane default

    getMediaSourcesForCategory(mutableArray, p_preparser, p_sourceProvider, category);

    return [mutableArray copy];
}

+ (NSArray<HeimdallMediaSource *> *)listOfLocalMediaSources
{
    libheimdall_int_t *p_libheimdallInstance = heimdall_object_instance(getIntf());
    heimdall_preparser_t *p_preparser = getNetworkPreparser();
    NSMutableArray<HeimdallMediaSource *> * const mutableArray = [[NSMutableArray alloc] initWithCapacity:32]; // A sane default

    // "My Folders" and "My Machine" entries are a bit of a custom implementation so it gets treated differently
    HeimdallMediaSource * const myFoldersMediaSource = [[HeimdallLocalMediaSource alloc] initMyFoldersMediaSourceWithPreparser:p_preparser];
    HeimdallMediaSource * const localDevicesMediaSource = [[HeimdallLocalMediaSource alloc] initForLocalDevices:p_preparser];
    [mutableArray addObject:myFoldersMediaSource];
    [mutableArray addObject:localDevicesMediaSource];

    heimdall_media_source_provider_t * const p_sourceProvider = heimdall_media_source_provider_Get(p_libheimdallInstance);
    if (p_sourceProvider != NULL) {
        // Currently, SD_CAT_MYCOMPUTER and SD_CAT_DEVICES return empty list.
        // They are left for future implementation.
        getMediaSourcesForCategory(mutableArray, p_preparser, p_sourceProvider, SD_CAT_MYCOMPUTER);
        getMediaSourcesForCategory(mutableArray, p_preparser, p_sourceProvider, SD_CAT_DEVICES);
        getMediaSourcesForCategory(mutableArray, p_preparser, p_sourceProvider, SD_CAT_LAN);
    }

    return [mutableArray copy];
}

@end
