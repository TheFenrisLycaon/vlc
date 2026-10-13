/*****************************************************************************
 * HeimdallRendererItem.m: Wrapper class for heimdall_renderer_item_t
 *****************************************************************************
 * Copyright (C) 2016, 2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Marvin Scholz <epirat07 at gmail dot com>
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

#import "HeimdallRendererItem.h"

#import "extensions/NSString+Helpers.h"
#import "playqueue/HeimdallPlayerController.h"

#include <heimdall_common.h>
#include <heimdall_renderer_discovery.h>

@implementation HeimdallRendererItem

- (instancetype)initWithRendererItem:(heimdall_renderer_item_t*)item
{
    self = [super init];
    if (self) {
        if (!item)
            [NSException raise:NSInvalidArgumentException
                        format:@"item must not be nil"];
        _rendererItem = heimdall_renderer_item_hold(item);
    }
    return self;
}

- (void)dealloc
{
    heimdall_renderer_item_release(_rendererItem);
    _rendererItem = nil;
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"%@: name: %@, type: %@", [self className], self.name, self.type];
}

- (NSString *)name
{
    return toNSStr(heimdall_renderer_item_name(_rendererItem));
}

- (NSString*)identifier
{
    return toNSStr(heimdall_renderer_item_sout(_rendererItem));
}

- (NSString*)iconURI
{
    return toNSStr(heimdall_renderer_item_icon_uri(_rendererItem));
}

- (NSString *)type
{
    return toNSStr(heimdall_renderer_item_type(_rendererItem));
}

- (NSString *)userReadableType
{
    NSString *type = [self type];
    if ([type isEqualToString:@"stream_out_dlna"]) {
        return @"DLNA";
    } else if ([type isEqualToString:@"chromecast"]) {
        return @"Chromecast";
    }
    return type;
}

- (int)capabilityFlags
{
    return heimdall_renderer_item_flags(_rendererItem);
}

- (void)setRendererForPlayerController:(HeimdallPlayerController *)playerController
{
    [playerController setRendererItem:_rendererItem];
}

- (BOOL)isEqual:(id)object
{
    if (![object isKindOfClass:[HeimdallRendererItem class]]) {
        return NO;
    }
    HeimdallRendererItem *other = object;
    return (other.rendererItem == self.rendererItem);
}

@end
