/*****************************************************************************
 * HeimdallPlayQueueItem.m: MacOS X interface module
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

#import "HeimdallPlayQueueItem.h"

#import <heimdall_input.h>
#import <heimdall_url.h>

#import "extensions/NSString+Helpers.h"
#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryDataTypes.h"

NSString *HeimdallPlaylistItemPasteboardType = @"com.lycaria.heimdall.playlistitemtype";

@implementation HeimdallPlayQueueItem

- (instancetype)initWithPlaylistItem:(heimdall_playlist_item_t *)p_item
{
    self = [super init];
    if (self) {
        _playlistItem = p_item;
        heimdall_playlist_item_Hold(_playlistItem);
        [self updateRepresentation];
    }
    return self;
}

- (void)dealloc
{
    heimdall_playlist_item_Release(_playlistItem);
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"item %p, title: %@ duration %lli", &_playlistItem, _title, _duration];
}

- (uint64_t)uniqueID
{
    return heimdall_playlist_item_GetId(_playlistItem);
}

- (HeimdallInputItem *)inputItem
{
    if (!_playlistItem) {
        return nil;
    }
    input_item_t *p_input = heimdall_playlist_item_GetMedia(_playlistItem);
    if (!p_input) {
        return nil;
    }
    HeimdallInputItem *inputItem = [[HeimdallInputItem alloc] initWithInputItem:p_input];
    return inputItem;
}

- (HeimdallMediaLibraryMediaItem *)mediaLibraryItem
{
    return [HeimdallMediaLibraryMediaItem mediaItemForURL:self.url];
}

- (void)updateRepresentation
{
    input_item_t *p_media = heimdall_playlist_item_GetMedia(_playlistItem);
    heimdall_mutex_lock(&p_media->lock);
    _title = toNSStr(p_media->psz_name);
    _duration = p_media->i_duration;

    if (p_media->p_meta) {
        _artistName = toNSStr(heimdall_meta_Get(p_media->p_meta, heimdall_meta_Artist));
        _albumName = toNSStr(heimdall_meta_Get(p_media->p_meta, heimdall_meta_Album));

        NSString *artworkURLString = toNSStr(heimdall_meta_Get(p_media->p_meta, heimdall_meta_ArtworkURL));
        if (artworkURLString.length > 1) {
            _artworkURL = [NSURL URLWithString:artworkURLString];
        }
    }
    heimdall_mutex_unlock(&p_media->lock);
}

- (NSURL *)url
{
    if (!_playlistItem) {
        return nil;
    }

    input_item_t *p_media = heimdall_playlist_item_GetMedia(_playlistItem);
    if (!p_media) {
        return nil;
    }
    char *psz_url = input_item_GetURI(p_media);
    if (!psz_url)
        return nil;

    NSURL *url = [NSURL URLWithString:toNSStr(psz_url)];
    free(psz_url);
    return url;
}

- (NSString *)path
{
    if (!_playlistItem) {
        return nil;
    }
    input_item_t *p_media = heimdall_playlist_item_GetMedia(_playlistItem);
    if (!p_media) {
        return nil;
    }
    char *psz_url = input_item_GetURI(p_media);
    if (!psz_url)
        return nil;

    char *psz_path = heimdall_uri2path(psz_url);
    NSString *path = toNSStr(psz_path);
    free(psz_url);
    free(psz_path);
    return path;
}

@end
