/*****************************************************************************
 * HeimdallOpenInputMetadata.h: macOS interface
 *****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne # lycaria dot org>
 *          Marvin Scholz <epirat07 at lycaria dot org>
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

#ifdef HAVE_CONFIG_H
# import <config.h>
#endif

#import <heimdall_common.h>
#import <heimdall_url.h>

#import "HeimdallOpenInputMetadata.h"

@implementation HeimdallOpenInputMetadata

+ (instancetype)inputMetaWithPath:(NSString *)path
{
    return [[HeimdallOpenInputMetadata alloc] initWithPath:path];
}

- (instancetype)initWithPath:(NSString *)path
{
    if (!path)
        return nil;

    self = [super init];
    if (!self)
        return nil;

    char *heimdall_uri_psz = heimdall_path2uri([path UTF8String], "file");
    if (!heimdall_uri_psz)
        return nil;

    NSString *heimdall_uri = [NSString stringWithUTF8String:heimdall_uri_psz];
    FREENULL(heimdall_uri_psz);

    if (!heimdall_uri)
        return nil;

    self.MRLString = heimdall_uri;
    return self;
}

@end
