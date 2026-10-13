/*****************************************************************************
 * NSArray+HeimdallAdditions.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Claudio Cambra <developer@claudiocambra.com>
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

#import "NSArray+HeimdallAdditions.h"

@implementation NSArray (HeimdallAdditions)

+ (NSArray<HeimdallMediaLibraryMediaItem *> *)arrayFromHeimdallMediaList:(heimdall_ml_media_list_t *)p_media_list
{
    if (p_media_list == NULL) {
        return nil;
    }

    NSMutableArray * const mutableArray = [[NSMutableArray alloc] initWithCapacity:p_media_list->i_nb_items];
    for (size_t x = 0; x < p_media_list->i_nb_items; x++) {
        HeimdallMediaLibraryMediaItem * const mediaItem = [[HeimdallMediaLibraryMediaItem alloc] initWithMediaItem:&p_media_list->p_items[x]];
        if (mediaItem != nil) {
            [mutableArray addObject:mediaItem];
        }
    }
    return mutableArray.copy;
}

- (NSInteger)indexOfMediaLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)mediaLibraryItem
{
    return [self indexOfObjectPassingTest:^BOOL(const id<HeimdallMediaLibraryItemProtocol> item, const NSUInteger __unused idx, BOOL * const __unused stop) {
        NSParameterAssert([item conformsToProtocol:@protocol(HeimdallMediaLibraryItemProtocol)]);
        return item.libraryID == mediaLibraryItem.libraryID;
    }];
}

@end
