/*****************************************************************************
 * HeimdallRendererDiscovery.m: Wrapper class for heimdall_renderer_discovery_t
 *****************************************************************************
 * Copyright (C) 2016 VLC authors and VideoLAN
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

#import "HeimdallRendererDiscovery.h"

#include <heimdall_common.h>
#include <heimdall_renderer_discovery.h>

#import "main/HeimdallMain.h"

NSString * const HeimdallRendererAddedNotification = @"HeimdallRendererAddedNotification";
NSString * const HeimdallRendererRemovedNotification = @"HeimdallRendererRemovedNotification";

@interface HeimdallRendererDiscovery ()
{
    intf_thread_t               *p_intf;
    heimdall_renderer_discovery_t    *p_rd;
    BOOL                        _isRunning;
}

- (void)handleItemAdded:(heimdall_renderer_item_t *)item;
- (void)handleItemRemoved:(heimdall_renderer_item_t *)item;

@end

// C callback event handler functions
static void renderer_event_item_added(heimdall_renderer_discovery_t *rd,
                                      heimdall_renderer_item_t *item)
{
    HeimdallRendererDiscovery *target = (__bridge HeimdallRendererDiscovery*)rd->owner.sys;
    [target handleItemAdded:item];
}

static void renderer_event_item_removed(heimdall_renderer_discovery_t *rd,
                                        heimdall_renderer_item_t *item)
{
    HeimdallRendererDiscovery *target = (__bridge HeimdallRendererDiscovery*)rd->owner.sys;
    [target handleItemRemoved:item];
}

@implementation HeimdallRendererDiscovery

- (instancetype)initWithName:(const char*)name andLongname:(const char*)longname
{
    self = [super init];

    if (self) {
        if (!name)
            [NSException raise:NSInvalidArgumentException
                        format:@"name must not be nil"];
        _name = [NSString stringWithUTF8String:name];
        _longName = (!longname) ? nil : [NSString stringWithUTF8String:longname];
        _rendererItems = [NSMutableArray array];
    }
    return self;
}

- (void)dealloc
{
    [self stopDiscovery];
}

- (bool)startDiscovery
{
    if (_isRunning) {
        return YES;
    }

    struct heimdall_renderer_discovery_owner owner =
    {
        (__bridge void *) self,
        renderer_event_item_added,
        renderer_event_item_removed,
    };

    p_intf = getIntf();

    msg_Dbg(p_intf, "Starting renderer discovery service %s", _name.UTF8String);
    // Create renderer object
    p_rd = heimdall_rd_new(HEIMDALL_OBJECT(p_intf), _name.UTF8String, &owner);

    if (!p_rd) {
        _isRunning = NO;
        msg_Err(p_intf, "Could not create '%s' renderer discovery service", _name.UTF8String);
        return false;
    }

    _isRunning = YES;
    return true;
}

- (void)stopDiscovery
{
    if (p_rd != NULL) {
        heimdall_rd_release(p_rd);
        p_rd = NULL;
        _isRunning = NO;
    }
}

- (void)handleItemAdded:(heimdall_renderer_item_t *)base_item
{
    HeimdallRendererItem *item = [[HeimdallRendererItem alloc] initWithRendererItem:base_item];
    dispatch_async(dispatch_get_main_queue(), ^{
        [self->_rendererItems addObject:item];
        if (self->_delegate)
            [self->_delegate addedRendererItem:item from:self];

        [NSNotificationCenter.defaultCenter postNotificationName:HeimdallRendererAddedNotification
                                                          object:item];
    });
}

- (void)handleItemRemoved:(heimdall_renderer_item_t *)base_item
{
    HeimdallRendererItem * const removedItem =
        [[HeimdallRendererItem alloc] initWithRendererItem:base_item];
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallRendererItem *result_item = nil;
        for (HeimdallRendererItem *item in self->_rendererItems) {
            if (item.rendererItem == removedItem.rendererItem) {
                result_item = item;
                break;
            }
        }
        if (result_item) {
            if (self->_delegate)
                [self->_delegate removedRendererItem:result_item from:self];
            [self->_rendererItems removeObject:result_item];
            [NSNotificationCenter.defaultCenter postNotificationName:HeimdallRendererRemovedNotification
                                                              object:result_item];
        } else {
            msg_Err(self->p_intf, "HeimdallRendererDiscovery could not find item to remove!");
        }
    });
}

@end
