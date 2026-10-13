/*****************************************************************************
 * HeimdallAddonListItem.m: Addons manager for the Mac
 ****************************************************************************
 * Copyright (C) 2014 VideoLAN and authors
 * Copyright (C) 2026 TheFenrisLycaon
 * Authors:       Felix Paul Kühne <fkuehne # videolan.org>
 *                David Fuhrmann <dfuhrmann # videolan.org>
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

#import "HeimdallAddonListItem.h"

#import "main/HeimdallMain.h"
#import "extensions/NSString+Helpers.h"

@interface HeimdallAddonListItem ()
{
    addon_entry_t *p_addon_entry;
}
@end

@implementation HeimdallAddonListItem

- (id)initWithAddon:(addon_entry_t *)p_entry
{
    self = [super init];
    if(self) {
        p_addon_entry = addon_entry_Hold(p_entry);
    }

    return self;
}

-(void)dealloc
{
    addon_entry_Release(p_addon_entry);
}

- (NSData *)uuid
{
    heimdall_mutex_lock(&p_addon_entry->lock);
    NSData *o_uuid = [NSData dataWithBytes:p_addon_entry->uuid length:sizeof(p_addon_entry->uuid)];
    heimdall_mutex_unlock(&p_addon_entry->lock);

    return o_uuid;
}

- (NSString *)name
{
    heimdall_mutex_lock(&p_addon_entry->lock);
    NSString *o_str = toNSStr(p_addon_entry->psz_name);
    heimdall_mutex_unlock(&p_addon_entry->lock);

    return o_str;
}
- (NSString *)author
{
    heimdall_mutex_lock(&p_addon_entry->lock);
    NSString *o_str = toNSStr(p_addon_entry->psz_author);
    heimdall_mutex_unlock(&p_addon_entry->lock);

    return o_str;
}

- (NSString *)version
{
    heimdall_mutex_lock(&p_addon_entry->lock);
    NSString *o_str = toNSStr(p_addon_entry->psz_version);
    heimdall_mutex_unlock(&p_addon_entry->lock);

    return o_str;
}

- (NSString *)description
{
    heimdall_mutex_lock(&p_addon_entry->lock);
    NSString *o_str = toNSStr(p_addon_entry->psz_description);
    heimdall_mutex_unlock(&p_addon_entry->lock);

    return o_str;
}

- (BOOL)isInstalled
{
    heimdall_mutex_lock(&p_addon_entry->lock);
    BOOL b_installed = p_addon_entry->e_state == ADDON_INSTALLED;
    heimdall_mutex_unlock(&p_addon_entry->lock);

    return b_installed;
}

- (addon_type_t)type
{
    heimdall_mutex_lock(&p_addon_entry->lock);
    addon_type_t type = p_addon_entry->e_type;
    heimdall_mutex_unlock(&p_addon_entry->lock);

    return type;
}

@end
