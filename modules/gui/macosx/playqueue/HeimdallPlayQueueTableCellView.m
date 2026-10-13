/*****************************************************************************
 * HeimdallPlayQueueTableViewCell.m: MacOS X interface module
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

#import "HeimdallPlayQueueTableCellView.h"

#import "extensions/NSColor+HeimdallAdditions.h"
#import "extensions/NSFont+HeimdallAdditions.h"
#import "extensions/NSImage+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

#import "library/HeimdallLibraryImageCache.h"
#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryDataTypes.h"

#import "main/HeimdallMain.h"

#import "playqueue/HeimdallPlayQueueItem.h"

#import "views/HeimdallImageView.h"

#import <heimdall_common.h>
#import <heimdall_configuration.h>

NSString * const HeimdallDisplayTrackNumberPlayQueueKey = @"HeimdallDisplayTrackNumberPlayQueueKey";
NSString * const HeimdallDisplayTrackNumberPlayQueueSettingChanged = @"HeimdallDisplayTrackNumberPlayQueueSettingChanged";

@implementation HeimdallPlayQueueTableCellView

- (void)dealloc
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)awakeFromNib
{
    [super awakeFromNib];
    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(updateRepresentation)
                                               name:HeimdallDisplayTrackNumberPlayQueueSettingChanged
                                             object:nil];
}

- (void)prepareForReuse
{
    [super prepareForReuse];
    [self updateFonts];
    [self updateColouredElements];
}

- (void)setRepresentsCurrentPlayQueueItem:(BOOL)representsCurrentPlayQueueItem
{
    _representsCurrentPlayQueueItem = representsCurrentPlayQueueItem;

    [self updateFonts];
    [self updateColouredElements];
}

- (void)updateFonts
{
    NSFont * const displayedFont = _representsCurrentPlayQueueItem ?
        [NSFont boldSystemFontOfSize:NSFont.systemFontSize] :
        [NSFont systemFontOfSize:NSFont.systemFontSize];

    NSFont * const sublineDisplayedFont = _representsCurrentPlayQueueItem ?
        [NSFont boldSystemFontOfSize:NSFont.smallSystemFontSize] :
        [NSFont systemFontOfSize:NSFont.smallSystemFontSize];

    self.mediaTitleTextField.font = displayedFont;
    self.secondaryMediaTitleTextField.font = displayedFont;
    self.durationTextField.font = displayedFont;
    self.artistTextField.font = sublineDisplayedFont;
}

- (void)updateColouredElements
{
    self.audioMediaTypeIndicator.textColor = _representsCurrentPlayQueueItem ?
        NSColor.labelColor :
        NSColor.secondaryLabelColor;
}

- (void)setRepresentedPlayQueueItem:(HeimdallPlayQueueItem *)item
{
    _representedPlayQueueItem = item;
    [self updateRepresentation];
}

- (void)updateRepresentation
{
    HeimdallPlayQueueItem * const item = self.representedPlayQueueItem;

    self.audioArtworkImageView.contentGravity = HeimdallImageViewContentGravityResizeAspect;
    self.mediaImageView.contentGravity = HeimdallImageViewContentGravityResizeAspect;

    __weak typeof(self) weakSelf = self;
    [HeimdallLibraryImageCache thumbnailForPlayQueueItem:item withCompletion:^(NSImage * const thumbnail) {
        if (!weakSelf || item != weakSelf.representedPlayQueueItem) {
            return;
        }
        weakSelf.audioArtworkImageView.image = thumbnail;
        weakSelf.mediaImageView.image = thumbnail;
    }];

    const BOOL validArtistString = item.artistName && item.artistName.length > 0;
    const BOOL validAlbumString = item.albumName && item.albumName.length > 0;
    HeimdallInputItem * const inputItem = item.inputItem;
    HeimdallMediaLibraryMediaItem * const mediaItem = item.mediaLibraryItem;
    NSString * const libraryTitle = mediaItem.title;
    NSString * const inputTitle = inputItem.title;
    const BOOL validTitleString = libraryTitle.length > 0 || inputTitle.length > 0;

    NSString *playTitle = libraryTitle.length > 0 ? libraryTitle : inputTitle ?: item.title;
    if (validTitleString) {
        NSString * const trackNumberString = inputItem.trackNumber;
        if ([NSUserDefaults.standardUserDefaults boolForKey:HeimdallDisplayTrackNumberPlayQueueKey] && trackNumberString && trackNumberString.length > 0 && ![trackNumberString isEqualToString:@"0"]) {
            playTitle = [NSString stringWithFormat:@"%@ · %@", trackNumberString, playTitle];
        }
    }

    NSString *songDetailString = @"";

    if (validArtistString && validAlbumString) {
        songDetailString = [NSString stringWithFormat:@"%@ · %@", item.artistName, item.albumName];
    } else if (validArtistString) {
        songDetailString = item.artistName;
    }

    if (songDetailString && ![songDetailString isEqualToString:@""]) {
        self.mediaTitleTextField.hidden = YES;
        self.secondaryMediaTitleTextField.hidden = NO;
        self.artistTextField.hidden = NO;
        self.secondaryMediaTitleTextField.stringValue = playTitle;
        self.artistTextField.stringValue = songDetailString;
        self.audioMediaTypeIndicator.hidden = NO;

        self.audioArtworkImageView.hidden = NO;
        self.mediaImageView.hidden = YES;
    } else {
        self.mediaTitleTextField.hidden = NO;
        self.secondaryMediaTitleTextField.hidden = YES;
        self.artistTextField.hidden = YES;
        self.mediaTitleTextField.stringValue = playTitle;
        self.audioMediaTypeIndicator.hidden = YES;

        self.audioArtworkImageView.hidden = YES;
        self.mediaImageView.hidden = NO;
    }

    self.durationTextField.stringValue = [NSString stringWithTimeFromTicks:item.duration];
}

@end
