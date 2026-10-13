/*****************************************************************************
 * HeimdallRendererMenuController.m: Controller class for the renderer menu
 *****************************************************************************
 * Copyright (C) 2016-2018 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Marvin Scholz <epirat07 at gmail dot com>
 *          Felix Paul Kühne <fkuehne -at- lycaria -dot- org>
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

#import "HeimdallRendererMenuController.h"

#import "extensions/NSImage+HeimdallAdditions.h"
#import "main/HeimdallMain.h"
#import "menus/renderers/HeimdallRendererItem.h"
#import "playqueue/HeimdallPlayQueueController.h"
#import "playqueue/HeimdallPlayerController.h"

#include <heimdall_renderer_discovery.h>

@interface HeimdallRendererMenuController ()
{
    NSMutableArray              *_rendererDiscoveries;
    BOOL                         _isDiscoveryEnabled;
    NSMenuItem                  *_selectedItem;

    intf_thread_t               *p_intf;
    heimdall_renderer_discovery_t    *p_rd;
}
@end

@implementation HeimdallRendererMenuController

- (void)setRendererMenuItem:(NSMenuItem *)rendererMenuItem
{
    _rendererMenuItem = rendererMenuItem;
    [self updateRendererMenuItemEnablement];
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        _rendererDiscoveries = [NSMutableArray array];
        _rendererItems = @[];
        _isDiscoveryEnabled = NO;
        p_intf = getIntf();

        [self loadRendererDiscoveries];
    }
    return self;
}

- (void)awakeFromNib
{
    _selectedItem = _rendererNoneItem;
}

- (void)dealloc
{
    [self stopRendererDiscoveries];
}

- (void)loadRendererDiscoveries
{
    // Service Discovery subnodes
    char **ppsz_longnames;
    char **ppsz_names;

    if (heimdall_rd_get_names(HEIMDALL_OBJECT(p_intf), &ppsz_names, &ppsz_longnames) != HEIMDALL_SUCCESS) {
        return;
    }
    char **ppsz_name = ppsz_names;
    char **ppsz_longname = ppsz_longnames;

    for( ; *ppsz_name; ppsz_name++, ppsz_longname++) {
        HeimdallRendererDiscovery *dc = [[HeimdallRendererDiscovery alloc] initWithName:*ppsz_name andLongname:*ppsz_longname];
        [dc setDelegate:self];
        [_rendererDiscoveries addObject:dc];
        free(*ppsz_name);
        free(*ppsz_longname);
    }
    free(ppsz_names);
    free(ppsz_longnames);
}

#pragma mark - Renderer item management

- (void)addRendererItem:(HeimdallRendererItem *)item
{
    NSParameterAssert(item != nil);

    NSMutableArray * const mutableRenderers = _rendererItems.mutableCopy;
    [mutableRenderers addObject:item];
    _rendererItems = mutableRenderers.copy;
    [self updateRendererMenuItemEnablement];

    // Check if the item is already selected
    if (_selectedItem.representedObject != nil) {
        HeimdallRendererItem *selected_rd_item = _selectedItem.representedObject;
        if ([selected_rd_item.identifier isEqualToString:item.identifier]) {
            [_selectedItem setRepresentedObject:item];
            return;
        }
    }

    NSMenuItem *menuItem = [[NSMenuItem alloc] init];
    menuItem.target = self;
    menuItem.action = @selector(selectRenderer:);
    menuItem.keyEquivalent = @"";
    if (item.capabilityFlags & HEIMDALL_RENDERER_CAN_VIDEO)
        menuItem.image = NSImage.HeimdallSidebarMovieImage;
    else
        menuItem.image = NSImage.HeimdallSidebarMusicImage;
    menuItem.representedObject = item;

    NSString *unformattedTitle = [NSString stringWithFormat:@"%@ %@", item.name, item.userReadableType];
    NSMutableAttributedString *title = [[NSMutableAttributedString alloc] initWithString:unformattedTitle
                                                                              attributes:nil];
    [title addAttributes:@{NSBaselineOffsetAttributeName : @(2),
                           NSFontAttributeName : [NSFont boldSystemFontOfSize:8]}
                   range:[unformattedTitle rangeOfString:item.userReadableType options:NSBackwardsSearch]];
    menuItem.attributedTitle = title;

    [_rendererMenu insertItem:menuItem atIndex:[_rendererMenu indexOfItem:_rendererNoneItem] + 1];
}

- (void)removeRendererItem:(HeimdallRendererItem *)item
{
    NSParameterAssert(item != nil);

    NSMutableArray * const mutableRenderers = _rendererItems.mutableCopy;
    [mutableRenderers removeObject:item];
    _rendererItems = mutableRenderers.copy;
    [self updateRendererMenuItemEnablement];

    const NSInteger index = [_rendererMenu indexOfItemWithRepresentedObject:item];
    if (index >= 0) {
        NSMenuItem * const menuItem = [_rendererMenu itemAtIndex:index];
        // Don't remove selected item
        if (menuItem != _selectedItem) {
            [_rendererMenu removeItemAtIndex:index];
        }
    }
}

- (void)startRendererDiscoveries
{
    _isDiscoveryEnabled = YES;
    for (HeimdallRendererDiscovery *dc in _rendererDiscoveries) {
        [dc startDiscovery];
    }
}

- (void)stopRendererDiscoveries
{
    _isDiscoveryEnabled = NO;
    for (HeimdallRendererDiscovery *dc in _rendererDiscoveries) {
        [dc stopDiscovery];
    }
}

- (void)updateRendererMenuItemEnablement
{
    _rendererMenuItem.enabled = _rendererItems.count > 0;
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem {
    if (menuItem == _rendererNoneItem ||
        [[menuItem representedObject] isKindOfClass:[HeimdallRendererItem class]]) {
        return _isDiscoveryEnabled;
    }
    return [menuItem isEnabled];
}

- (void)selectRenderer:(NSMenuItem *)sender
{
    [_rendererNoneItem setState:NSOffState];

    [_selectedItem setState:NSOffState];
    [sender setState:NSOnState];
    _selectedItem = sender;

    HeimdallRendererItem *item = [sender representedObject];
    HeimdallPlayerController *playerController = HeimdallMain.sharedInstance.playQueueController.playerController;

    if (item) {
        [item setRendererForPlayerController:playerController];
    } else {
        [playerController setRendererItem:NULL];
    }
}

#pragma mark HeimdallRendererDiscovery delegate methods
- (void)addedRendererItem:(HeimdallRendererItem *)item from:(HeimdallRendererDiscovery *)sender
{
    [self addRendererItem:item];
}

- (void)removedRendererItem:(HeimdallRendererItem *)item from:(HeimdallRendererDiscovery *)sender
{
    [self removeRendererItem:item];
}

#pragma mark Menu actions
- (IBAction)toggleRendererDiscovery:(id)sender {
    if (_isDiscoveryEnabled) {
        [self stopRendererDiscoveries];
    } else {
        [self startRendererDiscoveries];
    }
}

@end
