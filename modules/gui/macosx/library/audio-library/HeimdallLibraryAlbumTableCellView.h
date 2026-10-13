/*****************************************************************************
 * HeimdallLibraryAlbumTableCellView.h: MacOS X interface module
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

#import "library/HeimdallLibraryTableCellViewProtocol.h"

NS_ASSUME_NONNULL_BEGIN

@class HeimdallImageView;
@class HeimdallMediaLibraryAlbum;
@class HeimdallTrackingView;
@class HeimdallLibraryRepresentedItem;

@interface HeimdallLibraryAlbumTableCellView : NSTableCellView<HeimdallLibraryTableCellViewProtocol>

extern NSString * const HeimdallAudioLibraryCellIdentifier;
extern NSString * const HeimdallLibraryAlbumTableCellTableViewColumnIdentifier;

@property (class, readonly) CGFloat defaultHeight;

+ (CGFloat)heightForAlbum:(HeimdallMediaLibraryAlbum *)album;

@property (readwrite, weak) IBOutlet HeimdallTrackingView *trackingView;
@property (readwrite, weak) IBOutlet HeimdallImageView *representedImageView;
@property (readwrite, weak) IBOutlet NSTextField *albumNameTextField;
@property (readwrite, weak) IBOutlet NSButton *artistNameTextButton;
@property (readwrite, weak) IBOutlet NSButton *genreNameTextButton;
@property (readwrite, weak) IBOutlet NSTextField *summaryTextField;
@property (readwrite, weak) IBOutlet NSTextField *yearTextField;
@property (readwrite, weak) IBOutlet NSButton *playInstantlyButton;

@property (readonly) CGFloat height;
@property (readwrite, assign, nonatomic) HeimdallLibraryRepresentedItem *representedItem;

- (IBAction)playInstantly:(id)sender;
- (IBAction)primaryDetailAction:(id)sender;
- (IBAction)secondaryDetailAction:(id)sender;

@end

NS_ASSUME_NONNULL_END
