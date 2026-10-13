/*****************************************************************************
 * HeimdallPlayQueueController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2019-2026 VLC authors and VideoLAN
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

#import "HeimdallPlayQueueController.h"

#import <heimdall_interface.h>
#import <heimdall_player.h>
#import <heimdall_media_library.h>

#import "extensions/NSString+Helpers.h"
#import "main/HeimdallMain.h"
#import "playqueue/HeimdallPlayQueueModel.h"
#import "playqueue/HeimdallPlayQueueItem.h"
#import "playqueue/HeimdallPlayQueueDataSource.h"
#import "playqueue/HeimdallPlayerController.h"
#import "windows/HeimdallOpenInputMetadata.h"
#import "library/HeimdallInputItem.h"

#include <sys/xattr.h>
#include <time.h>

NSString * const HeimdallPlaybackOrderChanged = @"HeimdallPlaybackOrderChanged";
NSString * const HeimdallPlaybackRepeatChanged = @"HeimdallPlaybackRepeatChanged";
NSString * const HeimdallPlaybackHasPreviousChanged = @"HeimdallPlaybackHasPreviousChanged";
NSString * const HeimdallPlaybackHasNextChanged = @"HeimdallPlaybackHasNextChanged";
NSString * const HeimdallPlayQueueCurrentItemIndexChanged = @"HeimdallPlayQueueCurrentItemIndexChanged";
NSString * const HeimdallPlayQueueItemsAdded = @"HeimdallPlayQueueItemsAdded";
NSString * const HeimdallPlayQueueItemsRemoved = @"HeimdallPlayQueueItemsRemoved";

NSString * const HeimdallLibraryPlayQueueModeDefaultsKey = @"HeimdallLibraryPlayQueueMode";

@interface HeimdallPlayQueueController ()
{
    NSNotificationCenter *_defaultNotificationCenter;

    heimdall_playlist_t *_p_playlist;
    heimdall_playlist_listener_id *_playlistListenerID;
}

- (void)playQueueResetWithItems:(NSArray *)items;
- (void)playQueueAdded:(NSArray *)items atIndex:(size_t)insertionIndex count:(size_t)numberOfItems;
- (void)playQueueMovedIndex:(size_t)index toTarget:(size_t)target numberOfItems:(size_t)count;
- (void)playQueueRemovedItemsAtIndex:(size_t)index count:(size_t)numberOfItems;
- (void)playQueueUpdatedForIndex:(size_t)firstUpdatedIndex items:(heimdall_playlist_item_t *const *)items count:(size_t)numberOfItems;
- (void)playQueuePlaybackRepeatUpdated:(enum heimdall_playlist_playback_repeat)currentRepeatMode;
- (void)playQueuePlaybackOrderUpdated:(enum heimdall_playlist_playback_order)currentOrder;
- (void)currentPlayQueueItemIndexChanged:(size_t)index;
- (void)playQueueHasPreviousItem:(BOOL)hasPrevious;
- (void)playQueueHasNextItem:(BOOL)hasNext;
- (void)stopActionChanged:(enum heimdall_playlist_media_stopped_action)stoppedAction;

@end

#pragma mark -
#pragma mark core callbacks

static void
cb_playlist_items_reset(heimdall_playlist_t *const __unused playlist,
                        heimdall_playlist_item_t *const items[],
                        size_t numberOfItems,
                        void *p_data)
{
    @autoreleasepool {
        NSMutableArray * const array = [NSMutableArray arrayWithCapacity:numberOfItems];
        for (size_t i = 0; i < numberOfItems; i++) {
            HeimdallPlayQueueItem * const item = [[HeimdallPlayQueueItem alloc] initWithPlaylistItem:items[i]];
            [array addObject:item];
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            HeimdallPlayQueueController * const playQueueController = (__bridge HeimdallPlayQueueController *)p_data;
            [playQueueController playQueueResetWithItems:array];
        });
    }
}

static void
cb_playlist_items_added(heimdall_playlist_t *const __unused playlist,
                        size_t insertionIndex,
                        heimdall_playlist_item_t *const items[],
                        size_t numberOfAddedItems,
                        void *p_data)
{
    @autoreleasepool {
        NSMutableArray * const array = [NSMutableArray arrayWithCapacity:numberOfAddedItems];
        for (size_t i = 0; i < numberOfAddedItems; i++) {
            HeimdallPlayQueueItem * const item = [[HeimdallPlayQueueItem alloc] initWithPlaylistItem:items[i]];
            [array addObject:item];
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            HeimdallPlayQueueController * const playQueueController = (__bridge HeimdallPlayQueueController *)p_data;
            [playQueueController playQueueAdded:array atIndex:insertionIndex count:numberOfAddedItems];
        });
    }
}

static void
cb_playlist_items_moved(heimdall_playlist_t *const __unused playlist,
                        size_t index,
                        size_t numberOfMovedItems,
                        size_t target,
                        void *p_data)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayQueueController * const playQueueController = (__bridge HeimdallPlayQueueController *)p_data;
        [playQueueController playQueueMovedIndex:index toTarget:target numberOfItems:numberOfMovedItems];
    });
}

static void
cb_playlist_items_removed(heimdall_playlist_t *const __unused playlist,
                          size_t index,
                          size_t count,
                          void *p_data)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayQueueController * const playQueueController = (__bridge HeimdallPlayQueueController *)p_data;
        [playQueueController playQueueRemovedItemsAtIndex:index count:count];
    });
}

static void
cb_playlist_items_updated(heimdall_playlist_t *const __unused playlist,
                          size_t firstUpdatedIndex,
                          heimdall_playlist_item_t *const items[],
                          size_t numberOfUpdatedItems,
                          void *p_data)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayQueueController * const playQueueController = (__bridge HeimdallPlayQueueController *)p_data;
        [playQueueController playQueueUpdatedForIndex:firstUpdatedIndex items:items count:numberOfUpdatedItems];
    });
}

static void
cb_playlist_playback_repeat_changed(heimdall_playlist_t *const __unused playlist,
                                    enum heimdall_playlist_playback_repeat repeat,
                                    void *p_data)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayQueueController * const playQueueController = (__bridge HeimdallPlayQueueController *)p_data;
        [playQueueController playQueuePlaybackRepeatUpdated:repeat];
    });
}

static void
cb_playlist_playback_order_changed(heimdall_playlist_t *const __unused playlist,
                                   enum heimdall_playlist_playback_order order,
                                   void *p_data)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayQueueController * const playQueueController = (__bridge HeimdallPlayQueueController *)p_data;
        [playQueueController playQueuePlaybackOrderUpdated:order];
    });
}

static void
cb_playlist_current_item_index_changed(heimdall_playlist_t *const __unused playlist,
                                 ssize_t index,
                                 void *p_data)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayQueueController * const playQueueController = (__bridge HeimdallPlayQueueController *)p_data;
        [playQueueController currentPlayQueueItemIndexChanged:index];
    });
}

static void
cb_playlist_has_prev_changed(heimdall_playlist_t *const __unused playlist,
                             bool has_prev,
                             void *p_data)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayQueueController * const playQueueController = (__bridge HeimdallPlayQueueController *)p_data;
        [playQueueController playQueueHasPreviousItem:has_prev];
    });
}

static void
cb_playlist_has_next_changed(heimdall_playlist_t *const __unused playlist,
                             bool has_next,
                             void *p_data)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayQueueController * const playQueueController = (__bridge HeimdallPlayQueueController *)p_data;
        [playQueueController playQueueHasNextItem:has_next];
    });
}

static void
cb_playlist_media_stopped_action_changed(heimdall_playlist_t *const __unused p_playlist,
                                         enum heimdall_playlist_media_stopped_action newAction,
                                         void *p_data)
{
    HEIMDALL_UNUSED(p_playlist);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallPlayQueueController * const playQueueController = (__bridge HeimdallPlayQueueController *)p_data;
        [playQueueController stopActionChanged:newAction];
    });
}

static const struct heimdall_playlist_callbacks playlist_callbacks = {
    cb_playlist_items_reset,
    cb_playlist_items_added,
    cb_playlist_items_moved,
    cb_playlist_items_removed,
    cb_playlist_items_updated,
    cb_playlist_playback_repeat_changed,
    cb_playlist_playback_order_changed,
    cb_playlist_current_item_index_changed,
    cb_playlist_has_prev_changed,
    cb_playlist_has_next_changed,
    cb_playlist_media_stopped_action_changed,
};

#pragma mark -
#pragma mark class initialization

@implementation HeimdallPlayQueueController

- (instancetype)initWithPlaylist:(heimdall_playlist_t *)playlist
{
    self = [super init];
    if (self) {
        _defaultNotificationCenter = NSNotificationCenter.defaultCenter;
        [_defaultNotificationCenter addObserver:self
                                       selector:@selector(applicationWillTerminate:)
                                           name:NSApplicationWillTerminateNotification
                                         object:nil];
        [_defaultNotificationCenter addObserver:self
                                       selector:@selector(applicationDidFinishLaunching:)
                                           name:NSApplicationDidFinishLaunchingNotification
                                         object:nil];
        _p_playlist = playlist;

        /* set initial values, further updates through callbacks */
        heimdall_playlist_Lock(_p_playlist);
        _unsorted = YES;
        _libraryPlayQueueMode =
            [NSUserDefaults.standardUserDefaults boolForKey:HeimdallLibraryPlayQueueModeDefaultsKey];
        _playbackOrder = heimdall_playlist_GetPlaybackOrder(_p_playlist);
        _playbackRepeat = heimdall_playlist_GetPlaybackRepeat(_p_playlist);
        _playlistListenerID = heimdall_playlist_AddListener(_p_playlist,
                                                       &playlist_callbacks,
                                                       (__bridge void *)self,
                                                       YES);
        heimdall_playlist_Unlock(_p_playlist);
        _playQueueModel = [[HeimdallPlayQueueModel alloc] init];
        _playQueueModel.playQueueController = self;
        _playerController = [[HeimdallPlayerController alloc] initWithPlayer:heimdall_playlist_GetPlayer(_p_playlist)];
    }
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    /* Handle sleep notification */
    [NSWorkspace.sharedWorkspace.notificationCenter addObserver:self
                                                           selector:@selector(computerWillSleep:)
                                                               name:NSWorkspaceWillSleepNotification
                                                             object:nil];

    // respect playlist-autostart
    if (var_GetBool(getIntf(), "playlist-autostart")) {
        if ([self.playQueueModel numberOfPlayQueueItems] > 0) {
            [self startPlayQueue];
        }
    }
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    if (_p_playlist) {
        if (_playlistListenerID) {
            heimdall_playlist_Lock(_p_playlist);
            heimdall_playlist_RemoveListener(_p_playlist, _playlistListenerID);
            heimdall_playlist_Unlock(_p_playlist);
        }
    }
}

- (void)computerWillSleep:(NSNotification *)notification
{
    [self pausePlayback];
}

- (void)dealloc
{
    [NSWorkspace.sharedWorkspace.notificationCenter removeObserver:self];
    [_defaultNotificationCenter removeObserver:self];
}

#pragma mark - callback forwarders

- (void)playQueueResetWithItems:(NSArray *)items
{
    // Clear all items (reset)
    [_playQueueModel dropExistingData];

    [_playQueueModel addItems:items];

    [_playQueueDataSource playQueueUpdated];
}

- (void)playQueueAdded:(NSArray *)items atIndex:(size_t)insertionIndex count:(size_t)numberOfItems
{
    [_playQueueModel addItems:items atIndex:insertionIndex count:numberOfItems];

    [_playQueueDataSource playQueueItemsInsertedAtIndex:insertionIndex count:numberOfItems];
    [_defaultNotificationCenter postNotificationName:HeimdallPlayQueueItemsAdded object:self];
}

- (void)playQueueMovedIndex:(size_t)index toTarget:(size_t)target numberOfItems:(size_t)count
{
    [_playQueueModel moveItemAtIndex:index toTarget:target];
    [_playQueueDataSource playQueueUpdated];
}

- (void)playQueueRemovedItemsAtIndex:(size_t)index count:(size_t)numberOfItems
{
    NSRange range = NSMakeRange(index, numberOfItems);
    [_playQueueModel removeItemsInRange:range];

    [_playQueueDataSource playQueueUpdated];
    [_defaultNotificationCenter postNotificationName:HeimdallPlayQueueItemsRemoved object:self];
}

- (void)playQueueUpdatedForIndex:(size_t)firstUpdatedIndex items:(heimdall_playlist_item_t *const *)items count:(size_t)numberOfItems
{
    heimdall_playlist_Lock(_p_playlist);
    for (size_t i = 0; i < numberOfItems; i++) {
        size_t replaceIndex = firstUpdatedIndex + i;
        if (replaceIndex < heimdall_playlist_Count(_p_playlist)) {
            heimdall_playlist_item_t *currentItem = heimdall_playlist_Get(_p_playlist, replaceIndex);
            HeimdallPlayQueueItem *newItem = [[HeimdallPlayQueueItem alloc] initWithPlaylistItem:currentItem];
            [_playQueueModel replaceItemAtIndex:replaceIndex withItem:newItem];
        }
    }
    heimdall_playlist_Unlock(_p_playlist);

    [_playQueueDataSource playQueueItemsUpdatedAtIndex:firstUpdatedIndex count:numberOfItems];

}

- (void)playQueuePlaybackRepeatUpdated:(enum heimdall_playlist_playback_repeat)currentRepeatMode
{
    _playbackRepeat = currentRepeatMode;
    [_defaultNotificationCenter postNotificationName:HeimdallPlaybackRepeatChanged object:self];
}

- (void)playQueuePlaybackOrderUpdated:(enum heimdall_playlist_playback_order)currentOrder
{
    _playbackOrder = currentOrder;
    [_defaultNotificationCenter postNotificationName:HeimdallPlaybackOrderChanged object:self];
}

- (void)currentPlayQueueItemIndexChanged:(size_t)index
{
    _currentPlayQueueIndex = index;
    [_playQueueDataSource scrollToCurrentPlayQueueItem];
    [_defaultNotificationCenter postNotificationName:HeimdallPlayQueueCurrentItemIndexChanged object:self];
}

- (void)playQueueHasPreviousItem:(BOOL)hasPrevious
{
    _hasPreviousPlayQueueItem = hasPrevious;
    [_defaultNotificationCenter postNotificationName:HeimdallPlaybackHasPreviousChanged object:self];
}

- (void)playQueueHasNextItem:(BOOL)hasNext
{
    _hasNextPlayQueueItem = hasNext;
    [_defaultNotificationCenter postNotificationName:HeimdallPlaybackHasNextChanged object:self];
}

#pragma mark - controller functions for use within the UI

- (void)addPlayQueueItems:(NSArray <HeimdallOpenInputMetadata *> *)array
{
    BOOL b_autoplay = var_InheritBool(getIntf(), "macosx-autoplay");
    [self addPlayQueueItems:array atPosition:NSUIntegerMax startPlayback:b_autoplay];
}

- (void)addPlayQueueItems:(NSArray <HeimdallOpenInputMetadata *> *)itemArray
              atPosition:(size_t)insertionIndex
           startPlayback:(BOOL)startPlayback
{
    /* note: we don't add the item as cached data to the model here
     * because this will be done asynchronously through the callback */

    intf_thread_t *p_intf = getIntf();
    NSUInteger numberOfItems = [itemArray count];

    for (NSUInteger i = 0; i < numberOfItems; i++) {
        HeimdallOpenInputMetadata *itemMetadata = itemArray[i];
        input_item_t *p_input = [self createInputItemBasedOnMetadata:itemMetadata];
        NSString *itemURLString = itemMetadata.MRLString;

        if (!p_input) {
            if (itemURLString) {
                msg_Warn(p_intf, "failed to create input for %s", [itemURLString UTF8String]);
            } else {
                msg_Warn(p_intf, "failed to create input because no URL was provided");
            }
            continue;
        }

        heimdall_playlist_Lock(_p_playlist);

        int ret = 0;
        size_t actualInsertionIndex = insertionIndex;
        if (insertionIndex == NSUIntegerMax) {
            actualInsertionIndex = heimdall_playlist_Count(_p_playlist);
        }
        ret = heimdall_playlist_Insert(_p_playlist,
                                  actualInsertionIndex,
                                  &p_input,
                                  1);

        if (ret != HEIMDALL_SUCCESS) {
            msg_Err(p_intf, "failed to insert input item at insertion index: %zu", insertionIndex);
        } else {
            msg_Dbg(p_intf, "Added item %s at insertion index: %zu", [itemURLString UTF8String], insertionIndex);
        }

        if (i == 0 && startPlayback) {
            heimdall_playlist_PlayAt(_p_playlist, actualInsertionIndex);
        }

        heimdall_playlist_Unlock(_p_playlist);
        input_item_Release(p_input);

        if (insertionIndex != NSUIntegerMax) {
            insertionIndex++;
        }
    }
}

- (int)addInputItem:(input_item_t *)p_inputItem atPosition:(size_t)insertionIndex startPlayback:(BOOL)startPlayback
{
    if (p_inputItem == NULL) {
        return HEIMDALL_EINVAL;
    }
    int ret = 0;

    heimdall_playlist_Lock(_p_playlist);
    if (insertionIndex == NSUIntegerMax) {
        insertionIndex = heimdall_playlist_Count(_p_playlist);
    }
    ret = heimdall_playlist_InsertOne(_p_playlist, insertionIndex, p_inputItem);
    if (ret != HEIMDALL_SUCCESS) {
        heimdall_playlist_Unlock(_p_playlist);
        return ret;
    }
    if (startPlayback) {
        ret = heimdall_playlist_PlayAt(_p_playlist, insertionIndex);
    }
    heimdall_playlist_Unlock(_p_playlist);
    return ret;
}

- (int)moveItemWithID:(int64_t)uniqueID toPosition:(size_t)target
{
    heimdall_playlist_item_t **items = calloc(1, sizeof(heimdall_playlist_item_t *));
    heimdall_playlist_Lock(_p_playlist);
    ssize_t itemIndex = heimdall_playlist_IndexOfId(_p_playlist, uniqueID);
    heimdall_playlist_item_t *p_item = heimdall_playlist_Get(_p_playlist, itemIndex);
    items[0] = p_item;
    int ret = heimdall_playlist_RequestMove(_p_playlist, items, 1, target, itemIndex);
    heimdall_playlist_Unlock(_p_playlist);
    free(items);
    return ret;
}

- (void)removeItemsAtIndexes:(NSIndexSet *)indexes
{
    if (indexes.count == 0)
        return;

    __block heimdall_playlist_item_t **items = calloc(indexes.count, sizeof(heimdall_playlist_item_t *));
    __block NSUInteger pos = 0;
    [indexes enumerateIndexesUsingBlock:^(const NSUInteger idx, BOOL * const __unused stop) {
        HeimdallPlayQueueItem *item = [self->_playQueueModel playQueueItemAtIndex:idx];
        items[pos++] = item.playlistItem;
    }];

    /* note: we don't remove the cached data from the model here
     * because this will be done asynchronously through the callback */

    heimdall_playlist_Lock(_p_playlist);
    heimdall_playlist_RequestRemove(_p_playlist, items, pos, indexes.firstIndex);
    heimdall_playlist_Unlock(_p_playlist);

    free(items);
}

- (void)clearPlayQueue
{
    heimdall_playlist_Lock(_p_playlist);
    heimdall_playlist_Clear(_p_playlist);
    heimdall_playlist_Unlock(_p_playlist);
}

- (int)sortByKey:(enum heimdall_playlist_sort_key)sortKey andOrder:(enum heimdall_playlist_sort_order)sortOrder
{
    struct heimdall_playlist_sort_criterion sortCriterion = { sortKey, sortOrder };
    int returnValue = HEIMDALL_SUCCESS;
    heimdall_playlist_Lock(_p_playlist);
    returnValue = heimdall_playlist_Sort(_p_playlist, &sortCriterion, 1);
    heimdall_playlist_Unlock(_p_playlist);
    if (returnValue == HEIMDALL_SUCCESS) {
        _lastSortKey = sortKey;
        _lastSortOrder = sortOrder;
        _unsorted = NO;
    }
    return returnValue;
}

- (int)startPlayQueue
{
    const NSInteger selectedIndex = [_playQueueDataSource.tableView selectedRow];
    return [self playItemAtIndex:selectedIndex];
}

- (int)playPreviousItem
{
    heimdall_playlist_Lock(_p_playlist);
    int ret = heimdall_playlist_Prev(_p_playlist);
    heimdall_playlist_Unlock(_p_playlist);
    return ret;
}

- (int)playItemAtIndex:(size_t)index
{
    heimdall_playlist_Lock(_p_playlist);
    size_t playlistLength = heimdall_playlist_Count(_p_playlist);
    int ret = 0;
    if (index >= playlistLength) {
        ret = HEIMDALL_EGENERIC;
    } else {
        ret = heimdall_playlist_PlayAt(_p_playlist, index);
    }
    heimdall_playlist_Unlock(_p_playlist);
    return ret;
}

- (int)playNextItem
{
    heimdall_playlist_Lock(_p_playlist);
    int ret = heimdall_playlist_Next(_p_playlist);
    heimdall_playlist_Unlock(_p_playlist);
    return ret;
}

- (void)stopPlayback
{
    heimdall_playlist_Lock(_p_playlist);
    heimdall_playlist_Stop(_p_playlist);
    heimdall_playlist_Unlock(_p_playlist);
}

- (void)pausePlayback
{
    heimdall_playlist_Lock(_p_playlist);
    heimdall_playlist_Pause(_p_playlist);
    heimdall_playlist_Unlock(_p_playlist);
}

- (void)resumePlayback
{
    heimdall_playlist_Lock(_p_playlist);
    heimdall_playlist_Resume(_p_playlist);
    heimdall_playlist_Unlock(_p_playlist);
}

- (void)setPlaybackOrder:(enum heimdall_playlist_playback_order)playbackOrder
{
    // Set the ivar first and then we will receive whatever has been applied via heimdall_playlist_SetPlaybackOrder
    [self playQueuePlaybackOrderUpdated:playbackOrder];

    heimdall_playlist_Lock(_p_playlist);
    heimdall_playlist_SetPlaybackOrder(_p_playlist, playbackOrder);
    heimdall_playlist_Unlock(_p_playlist);
}

- (void)setPlaybackRepeat:(enum heimdall_playlist_playback_repeat)playbackRepeat
{
    // Set the ivar first and then we will receive whatever has been applied via heimdall_playlist_SetPlaybackRepeat
    [self playQueuePlaybackRepeatUpdated:playbackRepeat];

    heimdall_playlist_Lock(_p_playlist);
    heimdall_playlist_SetPlaybackRepeat(_p_playlist, playbackRepeat);
    heimdall_playlist_Unlock(_p_playlist);
}

- (void)setLibraryPlayQueueMode:(BOOL)libraryPlayQueueMode
{
    _libraryPlayQueueMode = libraryPlayQueueMode;
    [NSUserDefaults.standardUserDefaults setBool:libraryPlayQueueMode
                                          forKey:HeimdallLibraryPlayQueueModeDefaultsKey];
}

#pragma mark - properties

- (HeimdallInputItem *)currentlyPlayingInputItem
{
    heimdall_player_t *player = heimdall_playlist_GetPlayer(_p_playlist);
    HeimdallInputItem *inputItem;
    heimdall_player_Lock(player);
    input_item_t *heimdallInputItem = heimdall_player_GetCurrentMedia(player);
    if (heimdallInputItem) {
        inputItem = [[HeimdallInputItem alloc] initWithInputItem:heimdallInputItem];
    }
    heimdall_player_Unlock(player);
    return inputItem;
}

#pragma mark - helper methods

- (void)updateLastOpenedDate:(NSURL *)url
{
    if (!url.isFileURL) {
        return;
    }

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return;
    }

    setxattr(url.fileSystemRepresentation, "com.apple.lastuseddate#PS", &ts, sizeof(ts), 0, 0);
}

- (input_item_t *)createInputItemBasedOnMetadata:(HeimdallOpenInputMetadata *)itemMetadata
{
    intf_thread_t *p_intf = getIntf();

    input_item_t *p_input;
    BOOL b_rem = FALSE, b_dir = FALSE, b_writable = FALSE;
    NSString *uri, *name, *path;
    NSURL * url;
    NSArray *optionsArray;

    /* Get the item */
    uri = itemMetadata.MRLString;
    url = [NSURL URLWithString: uri];
    path = [url path];
    name = itemMetadata.itemName;
    optionsArray = itemMetadata.playbackOptions;

    if ([[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&b_dir]
        && b_dir &&
        [NSWorkspace.sharedWorkspace getFileSystemInfoForPath:path
                                                    isRemovable:&b_rem
                                                     isWritable:&b_writable
                                                  isUnmountable:NULL
                                                    description:NULL
                                                           type:NULL]
        && b_rem && !b_writable && [url isFileURL]) {

        NSString *diskType = getVolumeTypeFromMountPath(path);
        msg_Dbg(p_intf, "detected optical media of type %s in the file input", [diskType UTF8String]);

        if ([diskType isEqualToString: kHeimdallMediaDVD])
            uri = [NSString stringWithFormat: @"dvdnav://%@", getBSDNodeFromMountPath(path)];
        else if ([diskType isEqualToString: kHeimdallMediaAudioDVD])
            uri = [NSString stringWithFormat: @"dvda://%@", getBSDNodeFromMountPath(path)];
        else if ([diskType isEqualToString: kHeimdallMediaVideoTSFolder])
            uri = [NSString stringWithFormat: @"dvdnav://%@", path];
        else if ([diskType isEqualToString: kHeimdallMediaAudioCD])
            uri = [NSString stringWithFormat: @"cdda://%@", getBSDNodeFromMountPath(path)];
        else if ([diskType isEqualToString: kHeimdallMediaVCD])
            uri = [NSString stringWithFormat: @"vcd://%@#0:0", getBSDNodeFromMountPath(path)];
        else if ([diskType isEqualToString: kHeimdallMediaSVCD])
            uri = [NSString stringWithFormat: @"vcd://%@@0:0", getBSDNodeFromMountPath(path)];
        else if ([diskType isEqualToString: kHeimdallMediaBD] || [diskType isEqualToString: kHeimdallMediaBDMVFolder])
            uri = [NSString stringWithFormat: @"bluray://%@", path];
        else
            msg_Warn(getIntf(), "unknown disk type, treating %s as regular input", [path UTF8String]);

        p_input = input_item_New([uri UTF8String], [[[NSFileManager defaultManager] displayNameAtPath:path] UTF8String]);
    } else {
        p_input = input_item_New([uri fileSystemRepresentation], name ? [name UTF8String] : NULL);
    }

    if (!p_input)
        return NULL;

    if (optionsArray) {
        NSUInteger count = [optionsArray count];
        for (NSUInteger i = 0; i < count; i++)
            input_item_AddOption(p_input, [[optionsArray objectAtIndex:i] UTF8String], HEIMDALL_INPUT_OPTION_TRUSTED);
    }

    /* Recent documents menu */
    if (url != nil && var_InheritBool(getIntf(), "macosx-recentitems")) {
        [[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:url];
        [self updateLastOpenedDate:url];
    }

    /* Recent Streams menu */
    if (url != nil && !url.isFileURL) {
        heimdall_medialibrary_t *ml = heimdall_ml_instance_get(getIntf());
        if (ml != NULL) {
            const char *psz_uri = uri.UTF8String;
            heimdall_ml_media_t * ml_media = heimdall_ml_get_media_by_mrl(ml, psz_uri);
            if (ml_media == NULL) {
                // create new entry if we don't have it yet
                ml_media = heimdall_ml_new_stream(ml, psz_uri);
            }
            if (ml_media != NULL) {
                heimdall_ml_media_set_played(ml, ml_media->i_id, true);
                heimdall_ml_media_release(ml_media);
            }
        } else {
            msg_Dbg(p_intf, "ML unavailable, won't remember stream");
        }
    }

    return p_input;
}

- (NSArray<HeimdallPlaylistExportModuleDescription *> *)availablePlaylistExportModules
{
    HeimdallPlaylistExportModuleDescription *xspf = [[HeimdallPlaylistExportModuleDescription alloc] init];
    xspf.humanReadableName = _NS("XSPF playlist");
    xspf.fileExtension = @"xspf";
    xspf.moduleName = @"export-xspf";

    HeimdallPlaylistExportModuleDescription *m3u = [[HeimdallPlaylistExportModuleDescription alloc] init];
    m3u.humanReadableName = _NS("M3U playlist");
    m3u.fileExtension = @"m3u";
    m3u.moduleName = @"export-m3u";

    HeimdallPlaylistExportModuleDescription *m3u8 = [[HeimdallPlaylistExportModuleDescription alloc] init];
    m3u8.humanReadableName = _NS("M3U8 playlist");
    m3u8.fileExtension = @"m3u8";
    m3u8.moduleName = @"export-m3u8";

    HeimdallPlaylistExportModuleDescription *html = [[HeimdallPlaylistExportModuleDescription alloc] init];
    html.humanReadableName = _NS("HTML playlist");
    html.fileExtension = @"html";
    html.moduleName = @"export-html";

    return @[xspf, m3u, m3u8, html];
}

- (int)exportPlaylistToPath:(NSString *)path exportModule:(HeimdallPlaylistExportModuleDescription *)exportModule
{
    heimdall_playlist_Lock(_p_playlist);
    int ret = heimdall_playlist_Export(_p_playlist,
                                  path.fileSystemRepresentation,
                                  exportModule.moduleName.UTF8String);
    heimdall_playlist_Unlock(_p_playlist);
    return ret;
}

- (void)stopActionChanged:(enum heimdall_playlist_media_stopped_action)stoppedAction
{
    _actionAfterStop = stoppedAction;
}

- (void)setActionAfterStop:(enum heimdall_playlist_media_stopped_action)actionAfterStop
{
    // Set the ivar first and then we will receive whatever has been enabled via actionAfterStopChanged
    [self stopActionChanged:actionAfterStop];

    heimdall_playlist_Lock(_p_playlist);
    heimdall_playlist_SetMediaStoppedAction(_p_playlist, actionAfterStop);
    heimdall_playlist_Unlock(_p_playlist);
}

@end

@implementation HeimdallPlaylistExportModuleDescription

@end
