/*****************************************************************************
 * HeimdallMediaItemCollectionViewItem.h: MacOS X interface module
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

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@class HeimdallImageView;
@class HeimdallLinearProgressIndicator;

extern NSString *HeimdallMediaItemCollectionViewItemIdentifier;

extern const CGFloat HeimdallMediaItemCollectionViewItemMinimalDisplayedProgress;
extern const CGFloat HeimdallMediaItemCollectionViewItemMaximumDisplayedProgress;

@interface HeimdallMediaItemCollectionViewItem : NSCollectionViewItem

@property (readwrite, assign) BOOL deselectWhenClickedIfSelected;

@property (readwrite, weak) IBOutlet NSTextField *mediaTitleTextField;
@property (readwrite, weak) IBOutlet NSTextField *annotationTextField;
@property (readwrite, weak) IBOutlet NSTextField *unplayedIndicatorTextField;
@property (readwrite, weak) IBOutlet NSTextField *secondaryInfoTextField;
@property (readwrite, weak) IBOutlet HeimdallImageView *mediaImageView;
@property (readwrite, weak) IBOutlet NSButton *playInstantlyButton;
@property (readwrite, weak) IBOutlet NSButton *addToPlayQueueButton;
@property (readwrite, weak) IBOutlet HeimdallLinearProgressIndicator *progressIndicator;
@property (readwrite, weak) IBOutlet NSBox *highlightBox;
@property (readwrite, weak) IBOutlet NSLayoutConstraint *imageViewAspectRatioConstraint;
@property (readwrite, weak) IBOutlet NSLayoutConstraint *trailingSecondaryTextToLeadingUnplayedIndicatorConstraint;
@property (readwrite, weak) IBOutlet NSLayoutConstraint *trailingSecondaryTextToTrailingSuperviewConstraint;

@end

NS_ASSUME_NONNULL_END
