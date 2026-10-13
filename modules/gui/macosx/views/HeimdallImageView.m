/*****************************************************************************
 * HeimdallImageView.m: MacOS X interface module
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

#import "HeimdallImageView.h"

#import <QuartzCore/QuartzCore.h>

#import "extensions/NSColor+HeimdallAdditions.h"
#import "extensions/NSView+HeimdallAdditions.h"

#import "views/HeimdallUIUnits.h"

@interface HeimdallImageView()
{
    NSURL *_currentArtworkURL;
}
@end

@implementation HeimdallImageView

- (instancetype)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    if (self) {
        [self setup];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super initWithCoder:decoder];
    if (self) {
        [self setup];
    }
    return self;
}

- (void)setup
{
    self.wantsLayer = YES;
    self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawOnSetNeedsDisplay;
    self.contentGravity = HeimdallImageViewContentGravityResizeAspectFill;
    self.cropsImagesToRoundedCorners = YES;
    self.needsDisplay = YES;
}

- (void)updateLayer
{
    self.layer.borderColor = self.shouldShowDarkAppearance ? NSColor.HeimdallDarkSubtleBorderColor.CGColor : NSColor.HeimdallLightSubtleBorderColor.CGColor;
    [self updateLayerImageCornerCropping];
    [self updateLayerContentGravity];
    [self updateLayerImage];
}

- (BOOL)wantsUpdateLayer
{
    return YES;
}

- (void)updateLayerImageCornerCropping
{
    if (self.cropsImagesToRoundedCorners) {
        self.layer.cornerRadius = HeimdallUIUnits.cornerRadius;
        self.layer.borderWidth = HeimdallUIUnits.borderThickness;
    } else {
        self.layer.cornerRadius = 0.;
        self.layer.borderWidth = 0.;
    }
}

- (void)setCropsImagesToRoundedCorners:(BOOL)cropsImagesToRoundedCorners
{
    self.layer.masksToBounds = cropsImagesToRoundedCorners;
    [self updateLayerImageCornerCropping];
}

- (BOOL)cropsImagesToRoundedCorners
{
    return self.layer.masksToBounds;
}

- (void)updateLayerImage
{
    const CGFloat desiredScaleFactor = [self.window backingScaleFactor];
    const CGFloat actualScaleFactor = [_image recommendedLayerContentsScale:desiredScaleFactor];

    const id layerContents = [_image layerContentsForContentsScale:actualScaleFactor];

    [self updateLayerContentGravity];
    self.layer.contents = layerContents;
    self.layer.contentsScale = actualScaleFactor;
}

- (void)setImage:(NSImage *)image
{
    _image = image;
    [self updateLayerImage];
}

- (void)updateLayerContentGravity
{
    switch (_contentGravity) {
        case HeimdallImageViewContentGravityCenter:
            self.layer.contentsGravity = kCAGravityCenter;
            break;
        case HeimdallImageViewContentGravityTop:
            self.layer.contentsGravity = kCAGravityTop;
            break;
        case HeimdallImageViewContentGravityBottom:
            self.layer.contentsGravity = kCAGravityBottom;
            break;
        case HeimdallImageViewContentGravityLeft:
            self.layer.contentsGravity = kCAGravityLeft;
            break;
        case HeimdallImageViewContentGravityRight:
            self.layer.contentsGravity = kCAGravityRight;
            break;
        case HeimdallImageViewContentGravityTopLeft:
            self.layer.contentsGravity = kCAGravityTopLeft;
            break;
        case HeimdallImageViewContentGravityTopRight:
            self.layer.contentsGravity = kCAGravityTopRight;
            break;
        case HeimdallImageViewContentGravityBottomLeft:
            self.layer.contentsGravity = kCAGravityBottomLeft;
            break;
        case HeimdallImageViewContentGravityBottomRight:
            self.layer.contentsGravity = kCAGravityBottomRight;
            break;
        case HeimdallImageViewContentGravityResize:
            self.layer.contentsGravity = kCAGravityResize;
            break;
        case HeimdallImageViewContentGravityResizeAspect:
            self.layer.contentsGravity = kCAGravityResizeAspect;
            break;
        case HeimdallImageViewContentGravityResizeAspectFill:
        default:
            self.layer.contentsGravity = kCAGravityResizeAspectFill;
            break;
    }
}

- (void)setContentGravity:(HeimdallImageViewContentGravity)contentGravity
{
    _contentGravity = contentGravity;
    [self updateLayerContentGravity];
}

- (void)setImageURL:(NSURL * _Nonnull)artworkURL placeholderImage:(NSImage * _Nullable)image
{
    if([_currentArtworkURL isEqual:artworkURL]) {
        return;
    }

    _currentArtworkURL = artworkURL;
    self.image = image;

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        NSImage * const downloadedImage = [[NSImage alloc] initWithContentsOfURL:artworkURL];
        dispatch_async(dispatch_get_main_queue(), ^{
            self.image = downloadedImage ?: image;
        });
    });
}

@end
