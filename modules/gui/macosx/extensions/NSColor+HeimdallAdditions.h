/*****************************************************************************
 * NSColor+HeimdallAdditions.h: MacOS X interface module
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

NS_ASSUME_NONNULL_BEGIN

@interface NSColor (HeimdallAdditions)

@property (class, readonly) NSColor *HeimdallAccentColor;
@property (class, readonly) NSColor *HeimdallSubtlerAccentColor;
@property (class, readonly) NSColor *HeimdalllibrarySubtitleColor;
@property (class, readonly) NSColor *HeimdalllibraryAnnotationColor;
@property (class, readonly) NSColor *HeimdalllibraryAnnotationBackgroundColor;
@property (class, readonly) NSColor *HeimdalllibrarySeparatorLightColor;
@property (class, readonly) NSColor *HeimdalllibrarySeparatorDarkColor;
@property (class, readonly) NSColor *HeimdalllibraryProgressIndicatorBackgroundColor;
@property (class, readonly) NSColor *HeimdallSliderFillColor;
@property (class, readonly) NSColor *HeimdallSliderLightBackgroundColor;
@property (class, readonly) NSColor *HeimdallSliderDarkBackgroundColor;
@property (class, readonly) NSColor *HeimdallLightSubtleBorderColor;
@property (class, readonly) NSColor *HeimdallDarkSubtleBorderColor;
@property (class, readonly) NSColor *HeimdallSubtleBorderColor;

@end

NS_ASSUME_NONNULL_END
