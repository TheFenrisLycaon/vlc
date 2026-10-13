/*****************************************************************************
 * HeimdallLibrarySongTableCellView.m: MacOS X interface module
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

#import "HeimdallLibrarySongTableCellView.h"

#import "extensions/NSColor+HeimdallAdditions.h"
#import "extensions/NSFont+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"
#import "extensions/NSView+HeimdallAdditions.h"

#import "views/HeimdallImageView.h"
#import "views/HeimdallTrackingView.h"

#import "main/HeimdallMain.h"

#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryItemInternalMediaItemsDataSource.h"
#import "library/HeimdallLibraryTableCellView.h"
#import "library/HeimdallLibraryRepresentedItem.h"

#import "playqueue/HeimdallPlayerController.h"
#import "playqueue/HeimdallPlayQueueController.h"

NSString *HeimdallAudioLibrarySongCellIdentifier = @"HeimdallAudioLibrarySongCellIdentifier";

@interface HeimdallLibrarySongTableCellView ()
{
    HeimdallLibraryController *_libraryController;
}
@end

@implementation HeimdallLibrarySongTableCellView

- (void)awakeFromNib
{
    self.playInstantlyButton.target = self;
    self.playInstantlyButton.action = @selector(playInstantly:);

    self.trackingView.viewToHide = self.playInstantlyButton;
    self.trackingView.viewToShow = self.trackNumberTextField;

    if (@available(macOS 10.14, *)) {
        self.playInstantlyButton.contentTintColor = NSColor.HeimdallAccentColor;
    }

    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(playStateOrItemChanged:)
                               name:HeimdallPlayerCurrentMediaItemChanged
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(playStateOrItemChanged:)
                               name:HeimdallPlayerStateChanged
                             object:nil];

    [self prepareForReuse];
}

- (void)playStateOrItemChanged:(NSNotification *)notification
{
    const BOOL isCurrentItemAndIsPlaying =
        [self.representedItem.item.firstMediaItem.inputItem.MRL isEqualToString:HeimdallMain.sharedInstance.playQueueController.currentlyPlayingInputItem.MRL];
    NSFont * const fontToUse = isCurrentItemAndIsPlaying
        ? [NSFont boldSystemFontOfSize:NSFont.systemFontSize]
        : [NSFont systemFontOfSize:NSFont.systemFontSize];
    self.songNameTextField.font = fontToUse;
    self.durationTextField.font = fontToUse;
    self.trackNumberTextField.font = fontToUse;
}

- (void)prepareForReuse
{
    [super prepareForReuse];
    self.songNameTextField.stringValue = @"";
    self.durationTextField.stringValue = @"";
    self.trackNumberTextField.stringValue = @"";
    self.playInstantlyButton.hidden = YES;
    self.trackNumberTextField.hidden = NO;
    [self playStateOrItemChanged:nil];
}

- (IBAction)playInstantly:(id)sender
{
    [self.representedItem play];
}

- (void)setRepresentedItem:(HeimdallLibraryRepresentedItem *)representedItem
{
    if (representedItem == nil) {
        NSLog(@"Represented item is nil, cannot set in song table cell view");
        return;
    }

    _representedItem = representedItem;

    HeimdallMediaLibraryMediaItem * const mediaItem = (HeimdallMediaLibraryMediaItem *)representedItem.item;
    NSAssert(mediaItem != nil, @"Represented item should be a medialibrarymediaitem!");

    self.songNameTextField.stringValue = mediaItem.displayString;
    self.durationTextField.stringValue = mediaItem.durationString;

    if (mediaItem.trackNumber == 0) {
        self.trackNumberTextField.stringValue = @"—";
    } else {
        self.trackNumberTextField.stringValue = [NSString stringWithFormat:@"%d", mediaItem.trackNumber];
    }

    [self playStateOrItemChanged:nil];
}

@end
