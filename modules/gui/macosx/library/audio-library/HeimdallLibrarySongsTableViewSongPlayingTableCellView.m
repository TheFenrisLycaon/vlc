/*****************************************************************************
 * HeimdallLibrarySongsTableViewSongPlayingTableCellView.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
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

#import "HeimdallLibrarySongsTableViewSongPlayingTableCellView.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "main/HeimdallMain.h"
#import "playqueue/HeimdallPlayerController.h"
#import "playqueue/HeimdallPlayQueueController.h"

@implementation HeimdallLibrarySongsTableViewSongPlayingTableCellView

- (void)awakeFromNib
{
    NSNotificationCenter *notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(playStateOrItemChanged:)
                               name:HeimdallPlayerCurrentMediaItemChanged
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(playStateOrItemChanged:)
                               name:HeimdallPlayerStateChanged
                             object:nil];
}

- (void)dealloc
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)prepareForReuse
{
    [super prepareForReuse];
    self.textField.stringValue = @"";
}

- (void)setRepresentedMediaItem:(HeimdallMediaLibraryMediaItem *)representedMediaItem
{
    _representedMediaItem = representedMediaItem;
    [self updatePlayState];
}

- (BOOL)isCurrentSong
{
    if (!_representedMediaItem ||
        !_representedMediaItem.inputItem ||
        !HeimdallMain.sharedInstance ||
        !HeimdallMain.sharedInstance.playQueueController ||
        !HeimdallMain.sharedInstance.playQueueController.currentlyPlayingInputItem) {

        return false;
    }

    return [_representedMediaItem.inputItem.MRL isEqualToString:HeimdallMain.sharedInstance.playQueueController.currentlyPlayingInputItem.MRL];
}

- (void)updatePlayState
{
    if (!_representedMediaItem || ![self isCurrentSong]) {
        self.textField.stringValue = @"";
        return;
    }

    NSString *text = @"";

    switch(HeimdallMain.sharedInstance.playQueueController.playerController.playerState) {
        case HEIMDALL_PLAYER_STATE_PAUSED:
            text = @"⏸︎";
            break;
        case HEIMDALL_PLAYER_STATE_PLAYING:
            text = @"▶";
        default:
            break;
    }

    self.textField.stringValue = text;
}

- (void)playStateOrItemChanged:(id)sender
{
    [self updatePlayState];
}

@end
