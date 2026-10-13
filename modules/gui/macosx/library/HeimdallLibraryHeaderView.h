/*****************************************************************************
 * HeimdallLibraryHeaderView.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2025 VLC authors and VideoLAN
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

#import <Cocoa/Cocoa.h>

@class HeimdallLibraryRepresentedItem;

NS_ASSUME_NONNULL_BEGIN

extern const CGFloat HeimdallLibraryHeaderViewHeight;
extern NSString * const HeimdallLibraryHeaderViewIdentifier;

@interface HeimdallLibraryHeaderView : NSTableHeaderView

@property (nullable, nonatomic) HeimdallLibraryRepresentedItem *representedItem;
@property (readwrite, nonatomic) BOOL internalPaddingAddedForContentView;

+ (instancetype)paddedHeaderView;
- (instancetype)initWithFrame:(NSRect)frameRect withInternalPaddingAddedForContentView:(BOOL)internalPaddingAddedForContentView;
- (void)updateWithRepresentedItem:(nullable HeimdallLibraryRepresentedItem *)representedItem
                    fallbackTitle:(nullable NSString *)fallbackTitle
                   fallbackDetail:(nullable NSString *)fallbackDetail;

@end

NS_ASSUME_NONNULL_END
