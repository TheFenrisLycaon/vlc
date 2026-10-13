/*****************************************************************************
 * HeimdallPlayQueueTableViewCell.h: MacOS X interface module
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

#import <Cocoa/Cocoa.h>

@class HeimdallImageView;
@class HeimdallPlayQueueItem;

NS_ASSUME_NONNULL_BEGIN

extern NSString * const HeimdallDisplayTrackNumberPlayQueueKey;
extern NSString * const HeimdallDisplayTrackNumberPlayQueueSettingChanged;

@interface HeimdallPlayQueueTableCellView : NSTableCellView

@property (readwrite, assign, nonatomic) HeimdallPlayQueueItem *representedPlayQueueItem;
@property (readwrite, nonatomic) BOOL representsCurrentPlayQueueItem;

@property (readwrite, assign) IBOutlet NSTextField *artistTextField;
@property (readwrite, assign) IBOutlet NSTextField *secondaryMediaTitleTextField;
@property (readwrite, assign) IBOutlet NSTextField *mediaTitleTextField;
@property (readwrite, assign) IBOutlet NSTextField *durationTextField;
@property (readwrite, assign) IBOutlet HeimdallImageView *mediaImageView;
@property (readwrite, assign) IBOutlet NSTextField *audioMediaTypeIndicator;
@property (readwrite, assign) IBOutlet HeimdallImageView *audioArtworkImageView;

@end

NS_ASSUME_NONNULL_END
