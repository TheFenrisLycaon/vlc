/*****************************************************************************
 * HeimdallMediaItemCollectionViewItem.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne # lycaria -dot- org>
 *          Claudio Cambra <developer@claudiocambra.com>
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

#import "HeimdallMediaItemCollectionViewItem.h"

#import "extensions/NSColor+HeimdallAdditions.h"
#import "extensions/NSFont+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

#import "views/HeimdallImageView.h"
#import "views/HeimdallLinearProgressIndicator.h"
#import "views/HeimdallTrackingView.h"
#import "views/HeimdallUIUnits.h"

NSString *HeimdallMediaItemCollectionViewItemIdentifier = @"HeimdallMediaItemCollectionViewItemIdentifier";

const CGFloat HeimdallMediaItemCollectionViewItemMinimalDisplayedProgress = 0.05;
const CGFloat HeimdallMediaItemCollectionViewItemMaximumDisplayedProgress = 0.95;

@implementation HeimdallMediaItemCollectionViewItem

- (instancetype)init
{
    return [self initWithNibName:nil bundle:nil];
}

- (instancetype)initWithNibName:(NSNibName)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        if (nibNameOrNil == nil) {
            [NSBundle.mainBundle loadNibNamed:@"HeimdallMediaItemCollectionViewItem"
                                        owner:self
                              topLevelObjects:nil];
        }
    }
    return self;
}

- (void)awakeFromNib
{
    if (@available(macOS 26.0, *)) {
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 260000
        self.playInstantlyButton.bordered = YES;
        self.playInstantlyButton.bezelStyle = NSBezelStyleGlass;
        self.playInstantlyButton.borderShape = NSControlBorderShapeCircle;
        self.playInstantlyButton.image = [NSImage imageWithSystemSymbolName:@"play.fill"
                                                   accessibilityDescription:nil];
        self.playInstantlyButton.imageScaling = NSImageScaleProportionallyUpOrDown;
        self.playInstantlyButton.controlSize = NSControlSizeExtraLarge;

        const CGFloat playButtonSize = HeimdallUIUnits.mediumPlaybackControlButtonSize;
        [NSLayoutConstraint activateConstraints:@[
            [self.playInstantlyButton.widthAnchor constraintEqualToConstant:playButtonSize],
            [self.playInstantlyButton.heightAnchor constraintEqualToConstant:playButtonSize],
        ]];

        self.addToPlayQueueButton.bordered = YES;
        self.addToPlayQueueButton.bezelStyle = NSBezelStyleGlass;
        self.addToPlayQueueButton.borderShape = NSControlBorderShapeCapsule;
        self.addToPlayQueueButton.image = [NSImage imageWithSystemSymbolName:@"ellipsis"
                                                    accessibilityDescription:nil];
        self.addToPlayQueueButton.imageScaling = NSImageScaleProportionallyUpOrDown;
        self.addToPlayQueueButton.controlSize = NSControlSizeSmall;
#endif
    }

    NSArray * const viewsToHide = @[self.playInstantlyButton, self.addToPlayQueueButton];
    [(HeimdallTrackingView *)self.view setViewsToHide:viewsToHide];

    self.secondaryInfoTextField.textColor = NSColor.HeimdalllibrarySubtitleColor;
    self.annotationTextField.font = NSFont.HeimdallLibraryItemAnnotationFont;
    self.annotationTextField.textColor = NSColor.HeimdalllibraryAnnotationColor;
    self.annotationTextField.backgroundColor = NSColor.HeimdalllibraryAnnotationBackgroundColor;
    self.highlightBox.borderColor = NSColor.HeimdallAccentColor;
    self.unplayedIndicatorTextField.textColor = NSColor.HeimdallAccentColor;
    [self prepareForReuse];
}

#pragma mark - view representation

- (void)prepareForReuse
{
    [super prepareForReuse];
    self.playInstantlyButton.hidden = YES;
    self.addToPlayQueueButton.hidden = YES;
    self.mediaTitleTextField.stringValue = @"";
    self.mediaImageView.image = nil;
    self.annotationTextField.hidden = YES;
    self.secondaryInfoTextField.stringValue = [NSString stringWithTime:0];
    self.secondaryInfoTextField.hidden = YES;
    self.unplayedIndicatorTextField.hidden = YES;
    self.progressIndicator.hidden = YES;
    self.highlightBox.hidden = YES;
}

- (void)setSelected:(BOOL)selected
{
    [super setSelected:selected];
    self.highlightBox.hidden = !selected;
}

- (void)rightMouseDown:(NSEvent *)event
{
    [self openContextMenu:event];
    [super rightMouseDown:event];
}

- (void)openContextMenu:(NSEvent *)event
{
    NSAssert(NO, @"Subclasses must override openContextMenu:");
}

@end
