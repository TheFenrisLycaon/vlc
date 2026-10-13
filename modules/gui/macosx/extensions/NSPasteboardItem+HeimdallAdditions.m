/*****************************************************************************
 * NSPasteboardItem+HeimdallAdditions.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2023 VLC authors and VideoLAN
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

#import "NSPasteboardItem+HeimdallAdditions.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryDataTypes.h"

@implementation NSPasteboardItem (HeimdallAdditions)

+ (instancetype)pasteboardItemWithLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    NSPasteboardItem * const pboardItem = [[NSPasteboardItem alloc] init];
    NSMutableArray * const encodedLibraryItemsArray = [NSMutableArray array];

    [libraryItem iterateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem * const mediaItem) {
        [encodedLibraryItemsArray addObject:mediaItem];
    }];

    NSError *archiveError = nil;
    NSData * const data = [NSKeyedArchiver archivedDataWithRootObject:encodedLibraryItemsArray
                                                requiringSecureCoding:YES
                                                                error:&archiveError];
    if (data == nil) {
        NSLog(@"Failed to archive MediaLibrary Item drag payload: %@", archiveError);
        return nil;
    }
    [pboardItem setData:data forType:HeimdallMediaLibraryMediaItemUTI];
    return pboardItem;
}

+ (nullable instancetype)pasteboardItemWithInputItem:(HeimdallInputItem *)inputItem
{
    if (inputItem == nil || inputItem.MRL.length == 0) {
        return nil;
    }

    NSURL * const itemURL = [NSURL URLWithString:inputItem.MRL];
    if (itemURL == nil) {
        return nil;
    }

    NSPasteboardItem * const pboardItem = [[NSPasteboardItem alloc] init];
    [pboardItem setString:itemURL.absoluteString forType:NSPasteboardTypeString];
    [pboardItem setString:itemURL.absoluteString
                  forType:itemURL.isFileURL ? NSPasteboardTypeFileURL : NSPasteboardTypeURL];

    if (itemURL.isFileURL) {
        NSString * const localPath = itemURL.path;
        if (localPath.length > 0) {
            [pboardItem setPropertyList:@[localPath] forType:NSFilenamesPboardType];
        }
    }

    return pboardItem;
}

@end
