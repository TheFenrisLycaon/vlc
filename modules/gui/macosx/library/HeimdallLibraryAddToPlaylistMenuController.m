/*****************************************************************************
 * HeimdallLibraryAddToPlaylistMenuController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Serhii Bykov <esphynox@gmail.com>
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

#import "HeimdallLibraryAddToPlaylistMenuController.h"

#import "extensions/NSMenuItem+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibraryRepresentedItem.h"

#import "main/HeimdallMain.h"

#import <heimdall_media_library.h>

@interface HeimdallLibraryAddToPlaylistMenuController () <NSMenuDelegate>
@end

@implementation HeimdallLibraryAddToPlaylistMenuController

- (instancetype)init
{
    self = [super init];
    if (self) {
        _addToPlaylistMenu = [[NSMenu alloc] initWithTitle:@""];
        _addToPlaylistMenu.delegate = self;
    }
    return self;
}

- (NSMenuItem *)createAddToPlaylistMenuItem
{
    NSMenuItem * const item = [[NSMenuItem alloc] initWithTitle:_NS("Add to Playlist")
                                                        action:nil
                                                 keyEquivalent:@""];
    [item setSubmenu:self.addToPlaylistMenu];
    [item heimdall_setActionImageWithSystemSymbolName:@"text.badge.plus"];
    return item;
}

#pragma mark - Helpers

- (nullable NSArray<HeimdallMediaLibraryPlaylist *> *)writablePlaylists
{
    HeimdallLibraryModel * const libraryModel = HeimdallMain.sharedInstance.libraryController.libraryModel;
    NSArray<HeimdallMediaLibraryPlaylist *> * const allPlaylists =
        [libraryModel listOfPlaylistsOfType:HEIMDALL_ML_PLAYLIST_TYPE_ALL];
    if (allPlaylists.count == 0) {
        return nil;
    }

    NSMutableArray<HeimdallMediaLibraryPlaylist *> * const writable =
        [NSMutableArray arrayWithCapacity:allPlaylists.count];
    for (HeimdallMediaLibraryPlaylist * const playlist in allPlaylists) {
        if (!playlist.readOnly) {
            [writable addObject:playlist];
        }
    }

    [writable sortUsingComparator:^NSComparisonResult(HeimdallMediaLibraryPlaylist * _Nonnull lhs,
                                                      HeimdallMediaLibraryPlaylist * _Nonnull rhs) {
        return [lhs.displayString caseInsensitiveCompare:rhs.displayString];
    }];

    return writable;
}

- (NSArray<HeimdallMediaLibraryMediaItem *> *)selectedMediaItems
{
    if (self.representedMediaItems.count > 0) {
        return self.representedMediaItems;
    }

    NSMutableArray<HeimdallMediaLibraryMediaItem *> * const mediaItems =
        [NSMutableArray arrayWithCapacity:self.representedItems.count];
    for (HeimdallLibraryRepresentedItem * const representedItem in self.representedItems) {
        [mediaItems addObjectsFromArray:representedItem.item.mediaItems];
    }
    return mediaItems;
}

#pragma mark - NSMenuDelegate

// Rebuilt on demand so newly created playlists appear without cache invalidation.
- (void)menuNeedsUpdate:(NSMenu *)menu
{
    [menu removeAllItems];

    NSMenuItem * const newPlaylistItem =
        [[NSMenuItem alloc] initWithTitle:_NS("New Playlist...")
                                   action:@selector(createNewPlaylist:)
                            keyEquivalent:@""];
    newPlaylistItem.target = self;
    [menu addItem:newPlaylistItem];

    NSArray<HeimdallMediaLibraryPlaylist *> * const playlists = [self writablePlaylists];
    if (playlists.count == 0) {
        return;
    }

    [menu addItem:NSMenuItem.separatorItem];

    for (HeimdallMediaLibraryPlaylist * const playlist in playlists) {
        NSMenuItem * const item = [[NSMenuItem alloc] initWithTitle:playlist.displayString
                                                             action:@selector(addToSelectedPlaylist:)
                                                      keyEquivalent:@""];
        item.target = self;
        item.representedObject = playlist;
        [menu addItem:item];
    }
}

#pragma mark - Actions

- (void)createNewPlaylist:(NSMenuItem *)sender
{
    NSArray<HeimdallMediaLibraryMediaItem *> * const mediaItems = [self selectedMediaItems];
    if (mediaItems.count == 0) {
        return;
    }

    [HeimdallMain.sharedInstance.libraryController showCreatePlaylistDialogForMediaItems:mediaItems];
}

- (void)addToSelectedPlaylist:(NSMenuItem *)sender
{
    HeimdallMediaLibraryPlaylist * const playlist = sender.representedObject;
    if (![playlist isKindOfClass:HeimdallMediaLibraryPlaylist.class]) {
        return;
    }

    [playlist appendMediaItems:[self selectedMediaItems]];
}

@end
