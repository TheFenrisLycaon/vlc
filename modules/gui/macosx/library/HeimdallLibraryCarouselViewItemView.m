/*****************************************************************************
 * HeimdallLibraryCarouselViewItemView.m: MacOS X interface module
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

#import "HeimdallLibraryCarouselViewItemView.h"

#import "extensions/NSColor+HeimdallAdditions.h"
#import "extensions/NSFont+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"
#import "extensions/NSView+HeimdallAdditions.h"

#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryImageCache.h"
#import "library/HeimdallLibraryMenuController.h"
#import "library/HeimdallLibraryRepresentedItem.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallImageView.h"
#import "views/HeimdallLinearProgressIndicator.h"

@interface HeimdallLibraryCarouselViewItemView ()

@property (readonly) HeimdallLibraryMenuController *menuController;

@end

@implementation HeimdallLibraryCarouselViewItemView

- (instancetype)init
{
    self = [super init];
    if (self) {
        [self setup];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (self) {
        [self setup];
    }
    return self;
}

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self setup];
    }
    return self;
}

- (void)awakeFromNib
{
    [self setup];
}

- (void)setup
{
    self.highlightBox.borderColor = NSColor.HeimdallAccentColor;
    self.titleTextField.font = NSFont.HeimdallLibrarySubsectionHeaderFont;
    self.detailTextField.font = NSFont.HeimdallLibrarySubsectionSubheaderFont;
    self.annotationTextField.font = NSFont.HeimdallLibraryItemAnnotationFont;
    self.annotationTextField.textColor = NSColor.HeimdalllibraryAnnotationColor;
    self.annotationTextField.backgroundColor = NSColor.HeimdalllibraryAnnotationBackgroundColor;

    [self prepareForReuse];
}

- (void)prepareForReuse
{
    self.selected = NO;
    self.highlightBox.hidden = YES;
    self.playButton.hidden = YES;
    self.annotationTextField.hidden = YES;
    self.progressIndicator.hidden = YES;
    self.titleTextField.stringValue = @"";
    self.detailTextField.stringValue = @"";
    self.imageView.image = nil;
}

- (void)updateRepresentation
{
    [self prepareForReuse];

    const id<HeimdallMediaLibraryItemProtocol> actualItem = self.representedItem.item;

    __weak typeof(self) weakSelf = self;
    [HeimdallLibraryImageCache thumbnailForLibraryItem:actualItem withCompletion:^(NSImage * const thumbnail) {
        if (!weakSelf || weakSelf.representedItem.item != actualItem) {
            return;
        }
        weakSelf.imageView.image = thumbnail;
    }];

    self.titleTextField.stringValue = actualItem.displayString;
    self.detailTextField.stringValue = actualItem.primaryDetailString;

    if ([actualItem isKindOfClass:HeimdallMediaLibraryMediaItem.class]) {
        HeimdallMediaLibraryMediaItem * const mediaItem = (HeimdallMediaLibraryMediaItem *)actualItem;

        if (mediaItem.mediaType == HEIMDALL_ML_MEDIA_TYPE_VIDEO) {
            HeimdallMediaLibraryTrack * const videoTrack = mediaItem.firstVideoTrack;
            NSString * const resolutionLabel = videoTrack.resolutionLabel;
            if (resolutionLabel) {
                self.annotationTextField.stringValue = resolutionLabel;
                self.annotationTextField.hidden = NO;
            }
        }

        const CGFloat position = mediaItem.progress;
        if (position > 0.05 && position < 0.95) {
            self.progressIndicator.progress = position;
            self.progressIndicator.hidden = NO;
        }
    }
}

- (void)setRepresentedItem:(HeimdallLibraryRepresentedItem *)representedItem
{
    if (representedItem == _representedItem) {
        return;
    }

    _representedItem = representedItem;
    [self updateRepresentation];
}

- (void)setSelected:(BOOL)selected
{
    if (_selected == selected) {
        return;
    }

    _selected = selected;
    self.highlightBox.hidden = !selected;
}

- (void)playRepresentedItem
{
    [self.representedItem play];
}

- (void)openContextMenu:(NSEvent *)event
{
    if (self.menuController == nil) {
        _menuController = [[HeimdallLibraryMenuController alloc] init];
    }

    self.menuController.representedItems = @[self.representedItem];
    [self.menuController popupMenuWithEvent:event forView:self];
}

@end
