/*****************************************************************************
 * NSColor+HeimdallAdditions.m: MacOS X interface module
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

#import "NSColor+HeimdallAdditions.h"

@implementation NSColor (HeimdallAdditions)

+ (NSColor *)HeimdallAccentColor
{
    if (@available(macOS 10.14, *)) {
        return [NSColor controlAccentColor];
    }

    return [NSColor HeimdallOrangeElementColor];
}

+ (NSColor *)HeimdallOrangeElementColor
{
    return [NSColor colorWithRed:1. green:.38 blue:.04 alpha:1.];
}

+ (NSColor *)HeimdallSubtlerAccentColor
{
    return [NSColor.HeimdallAccentColor colorWithAlphaComponent:0.8];
}

+ (NSColor *)HeimdalllibrarySubtitleColor
{
    return [NSColor colorWithRed:0.52 green:0.57 blue:0.61 alpha:1.];
}

+ (NSColor *)HeimdalllibraryAnnotationColor
{
    return [NSColor whiteColor];
}

+ (NSColor *)HeimdalllibraryAnnotationBackgroundColor
{
    return [NSColor colorWithRed:0. green:0. blue:0. alpha:.2];
}

+ (NSColor *)HeimdalllibrarySeparatorLightColor
{
    return [NSColor colorWithRed:0.89 green:0.91 blue:0.93 alpha:1.];
}

+ (NSColor *)HeimdalllibrarySeparatorDarkColor
{
    return [NSColor colorWithRed:0.11 green:0.09 blue:0.07 alpha:1.];
}

+ (NSColor *)HeimdalllibraryProgressIndicatorBackgroundColor
{
    return [NSColor colorWithRed:37./255. green:41./255. blue:44./255. alpha:.8];
}

+ (NSColor *)HeimdallSliderFillColor
{
    return NSColor.HeimdallAccentColor;
}

+ (NSColor *)HeimdallSliderLightBackgroundColor
{
    return [NSColor colorWithCalibratedWhite:0.5 alpha:0.5];
}

+ (NSColor *)HeimdallSliderDarkBackgroundColor
{
    return [NSColor colorWithCalibratedWhite:1 alpha:0.2];
}

+ (NSColor *)HeimdallLightSubtleBorderColor
{
    return [NSColor colorWithCalibratedWhite:0.85 alpha:1.0];
}

+ (NSColor *)HeimdallDarkSubtleBorderColor
{
    return [NSColor colorWithCalibratedWhite:0.3 alpha:1.0];
}

+ (NSColor *)HeimdallSubtleBorderColor
{
    return [NSColor colorNamed:@"HeimdallSubtleBorderColor"];
}

@end
