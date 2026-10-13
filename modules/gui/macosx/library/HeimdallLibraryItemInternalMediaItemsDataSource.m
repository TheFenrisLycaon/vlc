/*****************************************************************************
 * HeimdallLibraryItemInternalMediaItemsDataSource.m: MacOS X interface module
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

#import "HeimdallLibraryItemInternalMediaItemsDataSource.h"

#import "extensions/NSPasteboardItem+HeimdallAdditions.h"
#import "library/HeimdallLibraryDataTypes.h"

const CGFloat HeimdallLibraryInternalMediaItemRowHeight = 40.;

@interface HeimdallLibraryItemInternalMediaItemsDataSource ()

@property (readwrite, atomic) id<HeimdallMediaLibraryItemProtocol> internalItem;
@property (readwrite, atomic) NSArray<HeimdallMediaLibraryMediaItem*> *internalMediaItems;

@end

@implementation HeimdallLibraryItemInternalMediaItemsDataSource

// TODO: Connect to library model

- (id<HeimdallMediaLibraryItemProtocol>)representedItem
{
    return self.internalItem;
}

- (void)setRepresentedItem:(id<HeimdallMediaLibraryItemProtocol>)representedItem
{
    [self setRepresentedItem:representedItem withCompletion:nil];
}

- (void)setRepresentedItem:(id<HeimdallMediaLibraryItemProtocol>)item
            withCompletion:(nullable void (^)(void))completionHandler
{
    self.internalItem = item;

    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
        self.internalMediaItems = item.mediaItems;

        dispatch_async(dispatch_get_main_queue(), ^{
            if (completionHandler != nil) {
                completionHandler();
            }
        });
    });
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    return self.representedItem == nil ? 0 : self.internalMediaItems.count;
}

- (id<NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row
{
    const id<HeimdallMediaLibraryItemProtocol> libraryItem =
        [self libraryItemAtRow:row forTableView:tableView];
    return [NSPasteboardItem pasteboardItemWithLibraryItem:libraryItem];
}

- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtRow:(NSInteger)row
                                       forTableView:(NSTableView *)tableView
{
    if (row < 0 || (NSUInteger)row >= self.internalMediaItems.count) {
        return nil;
    }

    return self.internalMediaItems[row];
}

- (NSInteger)rowForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if (libraryItem == nil) {
        return NSNotFound;
    }

    return [self.internalMediaItems indexOfObjectPassingTest:^BOOL(HeimdallMediaLibraryMediaItem * const item, const NSUInteger __unused index, BOOL * const __unused stop) {
        return item.libraryID == libraryItem.libraryID;
    }];
}

- (HeimdallMediaLibraryParentGroupType)currentParentType
{
    if ([self.internalItem isKindOfClass:HeimdallMediaLibraryPlaylist.class]) {
        return HeimdallMediaLibraryParentGroupTypePlaylist;
    }
    return HeimdallMediaLibraryParentGroupTypeAlbum;
}

- (id<HeimdallMediaLibraryItemProtocol>)parentItemForTableView:(NSTableView *)tableView
{
    if ([self.internalItem isKindOfClass:HeimdallMediaLibraryPlaylist.class]) {
        return self.internalItem;
    }
    return nil;
}

@end
