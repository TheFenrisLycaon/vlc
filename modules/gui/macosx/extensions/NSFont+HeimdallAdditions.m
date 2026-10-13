/*****************************************************************************
 * NSFont+HeimdallAdditions.m: MacOS X interface module
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

#import "NSFont+HeimdallAdditions.h"

@implementation NSFont (HeimdallAdditions)

+ (NSFont *)HeimdalllibrarySectionHeaderFont
{
    return [NSFont systemFontOfSize:24. weight:NSFontWeightBold];
}

+ (NSFont *)HeimdallLibrarySubsectionHeaderFont
{
    return [NSFont systemFontOfSize:17. weight:NSFontWeightSemibold];
}

+ (NSFont *)HeimdallLibrarySubsectionSubheaderFont
{
    return [NSFont systemFontOfSize:15. weight:NSFontWeightMedium];
}

+ (NSFont *)HeimdallLibraryItemAnnotationFont
{
    if (@available(macOS 11.0, *)) {
        return [NSFont preferredFontForTextStyle:NSFontTextStyleCaption1 options:@{}];
    }
    return [NSFont systemFontOfSize:10 weight:NSFontWeightRegular];
}

@end
