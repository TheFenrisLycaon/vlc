/*****************************************************************************
 * HeimdallRecentStreamsMenuController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne -at- lycaria -dot- org>
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

#import "HeimdallRecentStreamsMenuController.h"
#import "extensions/NSString+Helpers.h"
#import "main/HeimdallMain.h"
#import "playqueue/HeimdallPlayQueueController.h"
#import "windows/HeimdallOpenInputMetadata.h"
#import <heimdall_media_library.h>

@interface HeimdallRecentStreamsMenuController ()
{
    heimdall_medialibrary_t *_mediaLibrary;
    heimdall_ml_event_callback_t *_eventCallback;
    NSMenu *_submenu;
}

- (void)rebuild;

@end

static void recentStreamsLibraryCallback(void *p_data, const heimdall_ml_event_t *p_event)
{
    if (p_event->i_type != HEIMDALL_ML_EVENT_HISTORY_CHANGED)
        return;

    HeimdallRecentStreamsMenuController *controller =
        (__bridge HeimdallRecentStreamsMenuController *)p_data;
    dispatch_async(dispatch_get_main_queue(), ^{
        [controller rebuild];
    });
}

@implementation HeimdallRecentStreamsMenuController

- (instancetype)initWithSubmenu:(NSMenu *)submenu
{
    if (self = [super init]) {
        _submenu = submenu;
        _submenu.autoenablesItems = NO;

        _mediaLibrary = heimdall_ml_instance_get(getIntf());
        if (_mediaLibrary != NULL) {
            _eventCallback = heimdall_ml_event_register_callback(_mediaLibrary,
                                                            recentStreamsLibraryCallback,
                                                            (__bridge void *)self);
            [NSNotificationCenter.defaultCenter addObserver:self
                                                   selector:@selector(applicationWillTerminate:)
                                                       name:NSApplicationWillTerminateNotification
                                                     object:nil];
        }
        [self rebuild];
    }
    return self;
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
    if (_eventCallback != NULL) {
        heimdall_ml_event_unregister_callback(_mediaLibrary, _eventCallback);
        _eventCallback = NULL;
    }
}

- (void)dealloc
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)rebuild
{
    [_submenu removeAllItems];
    _hasEntries = NO;

    if (_mediaLibrary == NULL) {
        return;
    }

    BOOL hasEntries = NO;
    heimdall_ml_query_params_t params = heimdall_ml_query_params_create();
    params.i_nbResults = 30;
    heimdall_ml_media_list_t *list = heimdall_ml_list_history(_mediaLibrary, &params, HEIMDALL_ML_HISTORY_TYPE_NETWORK);
    if (list != NULL) {
        for (size_t i = 0; i < list->i_nb_items; i++) {
            const heimdall_ml_media_t *ml_media = &list->p_items[i];
            if (ml_media->p_files == NULL || ml_media->p_files->i_nb_items == 0)
                continue;

            NSString *mrlString = toNSStr(ml_media->p_files->p_items[0].psz_mrl);
            NSString *title = toNSStr(ml_media->psz_title);
            if (title.length == 0)
                title = mrlString;

            NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:title
                                                          action:@selector(openRecentStreamItem:)
                                                   keyEquivalent:@""];
            item.target = self;
            item.representedObject = mrlString;
            item.toolTip = mrlString;
            [_submenu addItem:item];
            hasEntries = YES;
        }

        if (hasEntries)
            [_submenu addItem:[NSMenuItem separatorItem]];

        heimdall_ml_media_list_release(list);
    }

    _hasEntries = hasEntries;

    NSMenuItem *clearItem = [[NSMenuItem alloc] initWithTitle:_NS("Clear Menu")
                                                       action:@selector(clearRecentStreams:)
                                                keyEquivalent:@""];
    clearItem.target = self;
    clearItem.enabled = hasEntries;
    [_submenu addItem:clearItem];
}

- (void)openRecentStreamItem:(NSMenuItem *)sender
{
    HeimdallOpenInputMetadata *meta = [[HeimdallOpenInputMetadata alloc] init];
    meta.MRLString = sender.representedObject;
    [HeimdallMain.sharedInstance.playQueueController addPlayQueueItems:@[meta]];
}

- (void)clearRecentStreams:(id)sender
{
    heimdall_ml_clear_history(_mediaLibrary, HEIMDALL_ML_HISTORY_TYPE_NETWORK);
}

@end
