/*****************************************************************************
 * HeimdallLibraryController.m: MacOS X interface module
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

#import "HeimdallLibraryController.h"

#import "extensions/NSString+Helpers.h"

#import "main/HeimdallMain.h"

#import "playqueue/HeimdallPlayQueueController.h"
#import "playqueue/HeimdallPlayerController.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallMediaLibraryFolderObserver.h"

#import "playqueue/HeimdallPlayQueueItem.h"
#import "playqueue/HeimdallPlayQueueModel.h"

#import <heimdall_media_library.h>
#import <heimdall_modules.h>

typedef int (*folder_action_f)(heimdall_medialibrary_t*, const char*);

@interface HeimdallLibraryController()
{
    heimdall_medialibrary_t *_p_libraryInstance;
}
@end

@implementation HeimdallLibraryController

- (instancetype)init
{
    self = [super init];
    if (self) {
        if (self.shouldUseMediaLibrary) {
            _p_libraryInstance = heimdall_ml_instance_get(getIntf());
        } else {
            return self;
        }

        if (!_p_libraryInstance && self.shouldUseMediaLibrary) {
            msg_Info(getIntf(), "Heimdall runs without media library support");

            NSUserDefaults * const defaults = NSUserDefaults.standardUserDefaults;
            NSString * const suppressKey = @"HeimdallSuppressMediaLibraryFailedAlert";
            if (![defaults boolForKey:suppressKey]) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    NSAlert * const alert = [[NSAlert alloc] init];
                    alert.messageText = _NS("Media Library Unavailable");
                    alert.informativeText = _NS("The media library module could not be loaded. "
                                                "Library features such as browsing your media "
                                                "collection will not be available.");
                    alert.alertStyle = NSAlertStyleWarning;
                    [alert addButtonWithTitle:_NS("OK")];
                    alert.showsSuppressionButton = YES;
                    alert.suppressionButton.title = _NS("Do not show this message again");
                    [alert runModal];
                    if (alert.suppressionButton.state == NSControlStateValueOn) {
                        [defaults setBool:YES forKey:suppressKey];
                    }
                });
            }

            return self;
        }

        // Reset suppression so the alert reappears if a future load fails.
        [NSUserDefaults.standardUserDefaults removeObjectForKey:@"HeimdallSuppressMediaLibraryFailedAlert"];
        _libraryModel = [[HeimdallLibraryModel alloc] initWithLibrary:_p_libraryInstance];
        _unsorted = YES;

        NSNotificationCenter *defaultNotificationCenter = NSNotificationCenter.defaultCenter;
        [defaultNotificationCenter addObserver:self
                                      selector:@selector(playbackStateChanged:)
                                          name:HeimdallPlayerStateChanged
                                        object:nil];
        [defaultNotificationCenter addObserver:self
                                      selector:@selector(monitoredFolderChanged:)
                                          name:HeimdallMediaLibraryFolderFSEvent
                                        object:nil];
    }
    return self;
}

- (void)dealloc
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
    _p_libraryInstance = NULL;
}

- (void)playbackStateChanged:(NSNotification *)aNotification
{
    if (!_p_libraryInstance) {
        return;
    }
    HeimdallPlayerController *playerController = aNotification.object;
    if (playerController.playerState == HEIMDALL_PLAYER_STATE_PLAYING) {
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
            heimdall_ml_pause_background(self->_p_libraryInstance);
        });
    } else {
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
            heimdall_ml_resume_background(self->_p_libraryInstance);
        });
    }
}

- (void)monitoredFolderChanged:(NSNotification *)notification
{
    NSURL * const folderURL = (NSURL *)notification.object;
    [self reloadFolderWithFileURL:folderURL];
}

- (int)appendItemToPlayQueue:(HeimdallMediaLibraryMediaItem *)mediaItem playImmediately:(BOOL)playImmediately
{
    if (!_p_libraryInstance) {
        return HEIMDALL_EACCES;
    }
    input_item_t *p_inputItem = heimdall_ml_get_input_item(_p_libraryInstance, mediaItem.libraryID);
    if (!p_inputItem) {
        msg_Err(getIntf(), "No input item found for media id %lli", mediaItem.libraryID);
        return HEIMDALL_ENOENT;
    }
    int ret = [HeimdallMain.sharedInstance.playQueueController addInputItem:p_inputItem atPosition:-1 startPlayback:playImmediately];
    input_item_Release(p_inputItem);
    return ret;
}

- (int)appendItemsToPlayQueue:(NSArray <HeimdallMediaLibraryMediaItem *> *)mediaItemArray playFirstItemImmediately:(BOOL)playFirstItemImmediately
{
    if (!_p_libraryInstance) {
        return HEIMDALL_EACCES;
    }

    NSUInteger itemCount = [mediaItemArray count];
    int ret = HEIMDALL_SUCCESS;
    for (NSUInteger x = 0; x < itemCount; x++) {
        if (unlikely(x == 0 && playFirstItemImmediately)) {
            ret = [self appendItemToPlayQueue:mediaItemArray[x] playImmediately:YES];
        } else {
            ret = [self appendItemToPlayQueue:mediaItemArray[x] playImmediately:NO];
        }
        if (unlikely(ret != HEIMDALL_SUCCESS)) {
            break;
        }
    }
    return ret;
}

#pragma mark - folder management

- (int)performFolderAction:(folder_action_f)action withFileUrl:(NSURL *)fileURL
{
    if (!_p_libraryInstance) {
        return HEIMDALL_EACCES;
    }
    if (!fileURL) {
        return HEIMDALL_EINVAL;
    }

    return action(_p_libraryInstance, fileURL.absoluteString.UTF8String);
}

- (int)addFolderWithFileURL:(NSURL *)fileURL
{
    return [self performFolderAction:heimdall_ml_add_folder withFileUrl:fileURL];
}

- (int)banFolderWithFileURL:(NSURL *)fileURL
{
    return [self performFolderAction:heimdall_ml_ban_folder withFileUrl:fileURL];
}

- (int)unbanFolderWithFileURL:(NSURL *)fileURL
{
    return [self performFolderAction:heimdall_ml_unban_folder withFileUrl:fileURL];
}

- (int)removeFolderWithFileURL:(NSURL *)fileURL
{
    return [self performFolderAction:heimdall_ml_remove_folder withFileUrl:fileURL];
}

- (int)reloadFolderWithFileURL:(NSURL *)fileURL
{
    if (!_p_libraryInstance) {
        return HEIMDALL_EACCES;
    }
    if (!fileURL) {
        return HEIMDALL_EINVAL;
    }
    return heimdall_ml_reload_folder(_p_libraryInstance, fileURL.absoluteString.UTF8String);
}

- (void)reloadMediaLibraryFoldersForInputItems:(NSArray<HeimdallInputItem *> *)inputItems
{
    NSArray<HeimdallMediaLibraryEntryPoint *> * const entryPoints = self.libraryModel.listOfMonitoredFolders;
    NSMutableSet<NSString *> * const reloadMRLs = NSMutableSet.set;
    NSMutableSet<HeimdallInputItem *> * const checkedInputItems = NSMutableSet.set;

    for (HeimdallMediaLibraryEntryPoint * const entryPoint in entryPoints) {
        for (HeimdallInputItem * const inputItem in inputItems) {
            if ([checkedInputItems containsObject:inputItem]) {
                continue;
            }

            if ([inputItem.decodedMRL hasPrefix:entryPoint.decodedMRL]) {
                [reloadMRLs addObject:entryPoint.MRL];
                [checkedInputItems addObject:inputItem];
                break;
            }
        }
    }

    for (NSString * const entryPointMRL in reloadMRLs) {
        NSURL * const entryPointURL = [NSURL URLWithString:entryPointMRL];
        [self reloadFolderWithFileURL:entryPointURL];
    }
}

- (int)clearHistory
{
    if (!_p_libraryInstance) {
        return HEIMDALL_EACCES;
    }
    return heimdall_ml_clear_history(_p_libraryInstance, HEIMDALL_ML_HISTORY_TYPE_GLOBAL);
}

#pragma mark - Playlist creation

- (void)showCreatePlaylistDialogForPlayQueue
{
    HeimdallPlayQueueController * const playQueueController = HeimdallMain.sharedInstance.playQueueController;
    NSArray<HeimdallPlayQueueItem *> * const items = playQueueController.playQueueModel.playQueueItems;
    [self showCreatePlaylistDialogForPlayQueueItems:items];
}

- (void)showCreatePlaylistDialogForPlayQueueItems:(NSArray<HeimdallPlayQueueItem *> *)items
{
    if (!items || items.count == 0) {
        return;
    }

    NSString * const playlistName = [self runCreatePlaylistDialog];
    if (!playlistName) {
        return;
    }

    if (![self createPlaylistWithName:playlistName fromPlayQueueItems:items]) {
        msg_Err(getIntf(), "Failed to create playlist '%s' from play queue items", playlistName.UTF8String);
    }
}

- (void)showCreatePlaylistDialogForMediaItems:(NSArray<HeimdallMediaLibraryMediaItem *> *)mediaItems
{
    if (!mediaItems || mediaItems.count == 0) {
        return;
    }

    NSString * const playlistName = [self runCreatePlaylistDialog];
    if (!playlistName) {
        return;
    }

    if (![self createPlaylistWithName:playlistName fromMediaItems:mediaItems]) {
        msg_Err(getIntf(), "Failed to create playlist '%s' from media items", playlistName.UTF8String);
    }
}

- (void)showRenamePlaylistDialogForPlaylist:(HeimdallMediaLibraryPlaylist *)playlist
{
    if (playlist == nil || playlist.readOnly) {
        return;
    }

    NSString * const playlistName = [self runRenamePlaylistDialogForPlaylist:playlist];
    if (!playlistName) {
        return;
    }

    [playlist renameTo:playlistName];
}

- (nullable NSString *)runCreatePlaylistDialog
{
    return [self runPlaylistNameDialogWithTitle:_NS("Create New Playlist")
                                informativeText:_NS("Enter a name for the new playlist:")
                             primaryButtonTitle:_NS("Create")
                                    initialName:nil
                             requireNameChange:NO];
}

- (nullable NSString *)runRenamePlaylistDialogForPlaylist:(HeimdallMediaLibraryPlaylist *)playlist
{
    return [self runPlaylistNameDialogWithTitle:_NS("Rename playlist")
                                informativeText:_NS("Please enter the new playlist name:")
                             primaryButtonTitle:_NS("Rename")
                                    initialName:playlist.displayString
                             requireNameChange:YES];
}

- (nullable NSString *)runPlaylistNameDialogWithTitle:(NSString *)title
                                      informativeText:(NSString *)informativeText
                                   primaryButtonTitle:(NSString *)primaryButtonTitle
                                          initialName:(nullable NSString *)initialName
                                   requireNameChange:(BOOL)requireNameChange
{
    NSAlert * const alert = [[NSAlert alloc] init];
    alert.messageText = title;
    alert.informativeText = informativeText;
    NSButton * const primaryButton = [alert addButtonWithTitle:primaryButtonTitle];
    primaryButton.enabled = NO;
    [alert addButtonWithTitle:_NS("Cancel")];

    NSTextField * const input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 300, 24)];
    input.stringValue = initialName ?: @"";
    input.placeholderString = _NS("Playlist Name");
    alert.accessoryView = input;
    alert.window.initialFirstResponder = input;

    NSCharacterSet * const whitespace = NSCharacterSet.whitespaceAndNewlineCharacterSet;
    NSString * const currentName =
        [(initialName ?: @"") stringByTrimmingCharactersInSet:whitespace];
    id observer = [NSNotificationCenter.defaultCenter
        addObserverForName:NSControlTextDidChangeNotification
                   object:input
                    queue:NSOperationQueue.mainQueue
               usingBlock:^(NSNotification * __unused note) {
        NSString * const newName =
            [input.stringValue stringByTrimmingCharactersInSet:whitespace];
        primaryButton.enabled = newName.length > 0 &&
            (!requireNameChange || ![newName isEqualToString:currentName]);
    }];

    const NSModalResponse response = [alert runModal];
    [NSNotificationCenter.defaultCenter removeObserver:observer];

    if (response != NSAlertFirstButtonReturn) {
        return nil;
    }

    NSString * const name = [input.stringValue stringByTrimmingCharactersInSet:whitespace];
    if (name.length == 0 || (requireNameChange && [name isEqualToString:currentName])) {
        return nil;
    }
    return name;
}

- (BOOL)createPlaylistWithName:(NSString *)playlistName fromPlayQueueItems:(NSArray<HeimdallPlayQueueItem *> *)items
{
    if (!_p_libraryInstance || !playlistName || playlistName.length == 0) {
        return NO;
    }
    
    heimdall_ml_playlist_t * const playlist = heimdall_ml_playlist_create(_p_libraryInstance, playlistName.UTF8String);
    if (!playlist) {
        msg_Err(getIntf(), "Failed to create playlist with name: %s", playlistName.UTF8String);
        return NO;
    }
    
    const int64_t playlistId = playlist->i_id;
    heimdall_ml_playlist_release(playlist);
    
    for (HeimdallPlayQueueItem * const item in items) {
        HeimdallMediaLibraryMediaItem * const mediaLibraryItem = item.mediaLibraryItem;
        
        if (!mediaLibraryItem) {
            msg_Warn(getIntf(), "No media library item found for playqueue item with name '%s'", item.title.UTF8String);
            continue;
        }
        
        const int64_t mediaId = mediaLibraryItem.libraryID;
        const int result = heimdall_ml_playlist_append(_p_libraryInstance, playlistId, &mediaId, 1);
        if (result != HEIMDALL_SUCCESS) {
            msg_Warn(getIntf(), "Failed to add media library item %lld to playlist", mediaLibraryItem.libraryID);
        }
    }
    
    return YES;
}

- (BOOL)createPlaylistWithName:(NSString *)playlistName fromMediaItems:(NSArray<HeimdallMediaLibraryMediaItem *> *)mediaItems
{
    if (!_p_libraryInstance || !playlistName || playlistName.length == 0) {
        return NO;
    }

    heimdall_ml_playlist_t * const playlist = heimdall_ml_playlist_create(_p_libraryInstance, playlistName.UTF8String);
    if (!playlist) {
        msg_Err(getIntf(), "Failed to create playlist with name: %s", playlistName.UTF8String);
        return NO;
    }

    const int64_t playlistId = playlist->i_id;
    heimdall_ml_playlist_release(playlist);

    HeimdallMediaLibraryPlaylist * const mlPlaylist =
        [HeimdallMediaLibraryPlaylist playlistForLibraryID:playlistId];
    [mlPlaylist appendMediaItems:mediaItems];

    return YES;
}

- (void)sortByCriteria:(enum heimdall_ml_sorting_criteria_t)sortCriteria andDescending:(bool)descending
{
    _unsorted = NO;
    _lastSortingCriteria = sortCriteria;
    _descendingLibrarySorting = descending;
    [_libraryModel sortByCriteria:sortCriteria andDescending:descending];
}

- (void)filterByString:(NSString*)filterString
{
    self.libraryModel.filterString = filterString;
}

- (BOOL)shouldUseMediaLibrary
{
    return module_exists("medialibrary") && var_InheritBool(getIntf(), "media-library");
}

@end
