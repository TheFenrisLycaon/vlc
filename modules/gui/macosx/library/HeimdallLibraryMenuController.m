/*****************************************************************************
 * HeimdallLibraryMenuController.m: MacOS X interface module
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

#import "HeimdallLibraryMenuController.h"

#import "extensions/NSMenu+HeimdallAdditions.h"
#import "extensions/NSMenuItem+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryAddToPlaylistMenuController.h"
#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibraryRepresentedItem.h"
#import "library/HeimdallLibrarySegment.h"
#import "library/HeimdallLibraryWindow.h"

#import "main/HeimdallMain.h"

#import "panels/HeimdallInformationWindowController.h"

#import "playqueue/HeimdallPlayQueueController.h"

#import <heimdall_input.h>
#import <heimdall_url.h>
#import <heimdall_common.h>

@interface HeimdallLibraryMenuController ()
{
    HeimdallInformationWindowController *_informationWindowController;

    NSHashTable<NSMenuItem*> *_mediaItemRequiringMenuItems;
    NSHashTable<NSMenuItem*> *_recentsMediaItemRequiringMenuItems;
    NSHashTable<NSMenuItem*> *_inputItemRequiringMenuItems;
    NSHashTable<NSMenuItem*> *_localInputItemRequiringMenuItems;
    NSHashTable<NSMenuItem*> *_folderInputItemRequiringMenuItems;

    NSMenuItem *_revealItem;
    NSMenuItem *_deleteItem;
    NSMenuItem *_removeFromPlaylistItem;
    NSMenuItem *_informationItem;
    NSMenuItem *_renamePlaylistItem;

    HeimdallLibraryAddToPlaylistMenuController *_addToPlaylistMenuController;
    NSMenuItem *_addToPlaylistItem;
}

@property (readwrite) NSMenuItem *favoriteItem;

@end

@implementation HeimdallLibraryMenuController

- (instancetype)init
{
    self = [super init];
    if (self) {
        [self createLibraryMenu];
    }
    return self;
}

- (void)createLibraryMenu
{
    NSMenuItem *playItem = [[NSMenuItem alloc] initWithTitle:_NS("Play") action:@selector(play:) keyEquivalent:@""];
    playItem.target = self;

    NSMenuItem *appendItem = [[NSMenuItem alloc] initWithTitle:_NS("Append to Play Queue") action:@selector(appendToPlayQueue:) keyEquivalent:@""];
    appendItem.target = self;

    NSMenuItem *addItem = [[NSMenuItem alloc] initWithTitle:_NS("Add Media Folder...") action:@selector(addMedia:) keyEquivalent:@""];
    addItem.target = self;

    _revealItem = [[NSMenuItem alloc] initWithTitle:_NS("Reveal in Finder") action:@selector(revealInFinder:) keyEquivalent:@""];
    _revealItem.target = self;

    _deleteItem = [[NSMenuItem alloc] initWithTitle:_NS("Move to Trash") action:@selector(moveToTrash:) keyEquivalent:@""];
    _deleteItem.target = self;

    NSMenuItem *markUnseenItem = [[NSMenuItem alloc] initWithTitle:_NS("Mark as Unseen") action:@selector(markUnseen:) keyEquivalent:@""];
    markUnseenItem.target = self;

    _informationItem = [[NSMenuItem alloc] initWithTitle:_NS("Information...") action:@selector(showInformation:) keyEquivalent:@""];
    _informationItem.target = self;

    NSMenuItem * const bookmarkItem = [[NSMenuItem alloc] initWithTitle:_NS("Toggle Bookmark")
                                                                 action:@selector(toggleBookmark:)
                                                          keyEquivalent:@""];
    bookmarkItem.target = self;

    NSMenuItem * const addToLibraryItem = [[NSMenuItem alloc] initWithTitle:_NS("Add to Media Library")
                                                                      action:@selector(addToMediaLibrary:)
                                                               keyEquivalent:@""];
    addToLibraryItem.target = self;

    _favoriteItem = [[NSMenuItem alloc] initWithTitle:_NS("Toggle Favorite") action:@selector(toggleFavorite:) keyEquivalent:@""];
    self.favoriteItem.target = self;

    _addToPlaylistMenuController = [[HeimdallLibraryAddToPlaylistMenuController alloc] init];
    _addToPlaylistItem = [_addToPlaylistMenuController createAddToPlaylistMenuItem];

    _removeFromPlaylistItem = [[NSMenuItem alloc] initWithTitle:_NS("Remove from Playlist")
                                                         action:@selector(removeFromPlaylist:)
                                                  keyEquivalent:@""];
    _removeFromPlaylistItem.target = self;

    _renamePlaylistItem = [[NSMenuItem alloc] initWithTitle:_NS("Rename playlist")
                                                     action:@selector(renamePlaylist:)
                                              keyEquivalent:@""];
    _renamePlaylistItem.target = self;

    [playItem heimdall_setActionImageWithSystemSymbolName:@"play.fill"];
    [appendItem heimdall_setActionImageWithSystemSymbolName:@"text.line.last.and.arrowtriangle.forward"];
    [_removeFromPlaylistItem heimdall_setActionImageWithSystemSymbolName:@"minus.circle"];
    [_renamePlaylistItem heimdall_setActionImageWithSystemSymbolName:@"pencil"];
    [self.favoriteItem heimdall_setActionImageWithSystemSymbolName:@"heart"];
    [bookmarkItem heimdall_setActionImageWithSystemSymbolName:@"bookmark"];
    [addToLibraryItem heimdall_setActionImageWithSystemSymbolName:@"plus.rectangle.on.folder"];
    [_revealItem heimdall_setActionImageWithSystemSymbolName:@"folder"];
    [_deleteItem heimdall_setActionImageWithSystemSymbolName:@"trash"];
    [markUnseenItem heimdall_setActionImageWithSystemSymbolName:@"eye.slash"];
    [_informationItem heimdall_setActionImageWithSystemSymbolName:@"info.circle"];
    [addItem heimdall_setActionImageWithSystemSymbolName:@"folder.badge.plus"];

    _libraryMenu = [[NSMenu alloc] initWithTitle:@""];
    [_libraryMenu addMenuItemsFromArray:@[
        playItem,
        appendItem,
        _addToPlaylistItem,
        _renamePlaylistItem,
        self.favoriteItem,
        bookmarkItem,
        addToLibraryItem,
        _revealItem,
        _removeFromPlaylistItem,
        _deleteItem,
        markUnseenItem,
        _informationItem,
        [NSMenuItem separatorItem],
        addItem
    ]];

    _mediaItemRequiringMenuItems = [NSHashTable weakObjectsHashTable];
    [_mediaItemRequiringMenuItems addObject:playItem];
    [_mediaItemRequiringMenuItems addObject:appendItem];
    [_mediaItemRequiringMenuItems addObject:_addToPlaylistItem];
    [_mediaItemRequiringMenuItems addObject:self.favoriteItem];
    [_mediaItemRequiringMenuItems addObject:_revealItem];
    [_mediaItemRequiringMenuItems addObject:_deleteItem];
    [_mediaItemRequiringMenuItems addObject:_informationItem];

    _recentsMediaItemRequiringMenuItems = [NSHashTable weakObjectsHashTable];
    [_recentsMediaItemRequiringMenuItems addObject:markUnseenItem];

    _inputItemRequiringMenuItems = [NSHashTable weakObjectsHashTable];
    [_inputItemRequiringMenuItems addObject:playItem];
    [_inputItemRequiringMenuItems addObject:appendItem];

    _localInputItemRequiringMenuItems = [NSHashTable weakObjectsHashTable];
    [_localInputItemRequiringMenuItems addObject:_revealItem];
    [_localInputItemRequiringMenuItems addObject:_deleteItem];

    _folderInputItemRequiringMenuItems = [NSHashTable weakObjectsHashTable];
    [_folderInputItemRequiringMenuItems addObject:bookmarkItem];
    [_folderInputItemRequiringMenuItems addObject:addToLibraryItem];
}

- (void)menuItems:(NSHashTable<NSMenuItem*>*)menuItems
        setHidden:(BOOL)hidden
{
    for (NSMenuItem * const menuItem in menuItems) {
        menuItem.hidden = hidden;
    }
}

- (BOOL)representedItemHasInformationTarget:(HeimdallLibraryRepresentedItem * const)representedItem
{
    id<HeimdallMediaLibraryItemProtocol> const item = representedItem.item;

    if ([item isKindOfClass:HeimdallMediaLibraryPlaylist.class]) {
        return ((HeimdallMediaLibraryPlaylist *)item).numberOfMedia > 0;
    }

    return item.mediaItems.count > 0;
}

- (BOOL)representedItemsCanBeRevealedInFinder
{
    for (HeimdallLibraryRepresentedItem * const representedItem in self.representedItems) {
        id<HeimdallMediaLibraryItemProtocol> const item = representedItem.item;
        if ([item isKindOfClass:HeimdallMediaLibraryPlaylist.class] &&
            ((HeimdallMediaLibraryPlaylist *)item).MRL.length == 0) {
            return NO;
        }
    }

    return self.representedItems.count > 0;
}

- (void)updateMenuItems
{
    HeimdallLibraryModel * const libraryModel = HeimdallMain.sharedInstance.libraryController.libraryModel;
    NSArray<HeimdallMediaLibraryMediaItem *> * const recents = libraryModel.listOfRecentMedia;
    const BOOL isFavoritesSegment =
        HeimdallMain.sharedInstance.libraryWindow.librarySegmentType == HeimdallLibraryFavoritesSegmentType;

    _removeFromPlaylistItem.hidden = YES;
    _renamePlaylistItem.hidden = YES;

    /* no represented items (right-click on empty space) - media actions are disabled */
    if (self.representedItems.count == 0 && self.representedInputItems.count == 0) {
        [self menuItems:_inputItemRequiringMenuItems setHidden:YES];
        [self menuItems:_localInputItemRequiringMenuItems setHidden:YES];
        [self menuItems:_folderInputItemRequiringMenuItems setHidden:YES];
        [self menuItems:_recentsMediaItemRequiringMenuItems setHidden:YES];
        [self menuItems:_mediaItemRequiringMenuItems setHidden:NO];
        _revealItem.hidden = NO;
        self.favoriteItem.hidden = isFavoritesSegment;
        return;
    }

    if (self.representedItems != nil && self.representedItems.count > 0) {
        [self menuItems:_inputItemRequiringMenuItems setHidden:YES];
        [self menuItems:_localInputItemRequiringMenuItems setHidden:YES];
        [self menuItems:_folderInputItemRequiringMenuItems setHidden:YES];
        [self menuItems:_mediaItemRequiringMenuItems setHidden:NO];
        _revealItem.hidden = ![self representedItemsCanBeRevealedInFinder];

        BOOL allInPlaylist = YES;
        for (HeimdallLibraryRepresentedItem * const item in self.representedItems) {
            if (item.parentType != HeimdallMediaLibraryParentGroupTypePlaylist ||
                ![item.item isKindOfClass:HeimdallMediaLibraryMediaItem.class] ||
                item.parentItem == nil ||
                item.positionInParent == NSNotFound) {
                allInPlaylist = NO;
                break;
            }
        }
        _removeFromPlaylistItem.hidden = !allInPlaylist;
        if (self.representedItems.count == 1 &&
            [self.representedItems.firstObject.item isKindOfClass:HeimdallMediaLibraryPlaylist.class]) {
            HeimdallMediaLibraryPlaylist * const playlist =
                (HeimdallMediaLibraryPlaylist *)self.representedItems.firstObject.item;
            _renamePlaylistItem.hidden = playlist.readOnly;
        }

        BOOL anyNonRecent = NO;
        for (HeimdallLibraryRepresentedItem * const item in self.representedItems) {
            if ([recents indexOfObjectPassingTest:^BOOL(HeimdallMediaLibraryMediaItem * _Nonnull obj, NSUInteger __unused idx, BOOL * __unused stop) {
                return obj.libraryID == item.item.libraryID;
            }] == NSNotFound) {
                anyNonRecent = YES;
                break;
            }
        }
        [self menuItems:_recentsMediaItemRequiringMenuItems setHidden:anyNonRecent];
        
        BOOL anyUnfavorited = NO;
        for (HeimdallLibraryRepresentedItem * const item in self.representedItems) {
            if (!item.item.favorited) {
                anyUnfavorited = YES;
                break;
            }
        }
        self.favoriteItem.title = anyUnfavorited ? _NS("Add to Favorites") : _NS("Remove from Favorites");
        self.favoriteItem.action = anyUnfavorited ? @selector(addFavorite:) : @selector(removeFavorite:);
        [self.favoriteItem heimdall_setActionImageWithSystemSymbolName:(anyUnfavorited ? @"heart" : @"heart.slash")];
        self.favoriteItem.hidden = isFavoritesSegment;
        
        // Update delete menu item title based on whether items are file-backed
        BOOL hasFileBacked = NO;
        BOOL hasNonFileBacked = NO;
        
        for (HeimdallLibraryRepresentedItem * const item in self.representedItems) {
            if (item.item.isFileBacked) {
                hasFileBacked = YES;
            } else {
                hasNonFileBacked = YES;
            }
            if (hasFileBacked && hasNonFileBacked) {
                break;
            }
        }
        
        if (hasFileBacked && hasNonFileBacked) {
            _deleteItem.title = _NS("Move to Trash / Delete from Library");
        } else if (hasFileBacked) {
            _deleteItem.title = _NS("Move to Trash");
        } else {
            _deleteItem.title = _NS("Delete from Library");
        }

    } else if (_representedInputItems != nil && self.representedInputItems.count > 0) {
        [self menuItems:_mediaItemRequiringMenuItems setHidden:YES];
        _removeFromPlaylistItem.hidden = YES;
        [self menuItems:_inputItemRequiringMenuItems setHidden:NO];
        self.favoriteItem.hidden = YES;

        BOOL anyStream = NO;
        for (HeimdallInputItem * const inputItem in self.representedInputItems) {
            if (inputItem.isStream) {
                anyStream = YES;
                break;
            }
        }

        const BOOL bookmarkable =
            self.representedInputItems.count == 1 &&
            self.representedInputItems.firstObject.inputType == ITEM_TYPE_DIRECTORY;

        [self menuItems:_localInputItemRequiringMenuItems setHidden:anyStream];
        [self menuItems:_folderInputItemRequiringMenuItems setHidden:!bookmarkable];
    }
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    /* add media action is always available, every other action needs a represented item. */
    if (menuItem.action == @selector(addMedia:)) {
        return YES;
    }

    if (self.representedItems.count == 0 && self.representedInputItems.count == 0) {
        return NO;
    }

    if (menuItem == _informationItem) {
        for (HeimdallLibraryRepresentedItem * const item in self.representedItems) {
            if ([self representedItemHasInformationTarget:item]) {
                return YES;
            }
        }

        return NO;
    }

    if (menuItem == _revealItem && self.representedItems.count > 0) {
        return [self representedItemsCanBeRevealedInFinder];
    }

    return YES;
}

- (void)popupMenuWithEvent:(NSEvent *)theEvent forView:(NSView *)theView
{
    [NSMenu popUpContextMenu:_libraryMenu withEvent:theEvent forView:theView];
}

#pragma mark - actions

- (void)addInputItemToPlayQueue:(HeimdallInputItem*)inputItem
                playImmediately:(BOOL)playImmediately
{
    NSParameterAssert(inputItem);
    [HeimdallMain.sharedInstance.playQueueController addInputItem:inputItem.heimdallInputItem
                                                 atPosition:-1
                                              startPlayback:playImmediately];
}

- (void)play:(id)sender
{
    if (self.representedItems != nil && self.representedItems.count > 0) {
        [self.representedItems.firstObject play];

        if (self.representedItems.count > 1) {
            for (NSUInteger i = 1; i < self.representedItems.count; i++) {
                [self.representedItems[i] queue];
            }
        }

    } else if (self.representedInputItems != nil && self.representedInputItems.count > 0) {
        [self addInputItemToPlayQueue:self.representedInputItems.firstObject
                      playImmediately:YES];

        if (self.representedInputItems.count > 1) {
            for (NSUInteger i = 1; i < self.representedInputItems.count; i++) {
                [self addInputItemToPlayQueue:self.representedInputItems[i]
                              playImmediately:NO];
            }
        }
    }
}

- (void)appendToPlayQueue:(id)sender
{
    if (self.representedItems != nil && self.representedItems.count > 0) {
        for (HeimdallLibraryRepresentedItem * const item in self.representedItems) {
            [item queue];
        }
    } else if (self.representedInputItems != nil && self.representedInputItems.count > 0) {
        for (HeimdallInputItem * const inputItem in self.representedInputItems) {
            [self addInputItemToPlayQueue:inputItem playImmediately:NO];
        }
    }
}

- (void)removeFromPlaylist:(id)sender
{
    if (self.representedItems.count == 0) {
        return;
    }

    HeimdallMediaLibraryPlaylist * const playlist =
        (HeimdallMediaLibraryPlaylist *)self.representedItems.firstObject.parentItem;
    if (![playlist isKindOfClass:HeimdallMediaLibraryPlaylist.class]) {
        return;
    }

    NSMutableArray<NSNumber *> * const positions =
        [NSMutableArray arrayWithCapacity:self.representedItems.count];
    for (HeimdallLibraryRepresentedItem * const repItem in self.representedItems) {
        if (repItem.positionInParent != NSNotFound) {
            [positions addObject:@(repItem.positionInParent)];
        }
    }

    [playlist removeMediaItemsAtPositions:positions];
}

- (void)renamePlaylist:(id)sender
{
    if (self.representedItems.count != 1) {
        return;
    }

    id<HeimdallMediaLibraryItemProtocol> const item = self.representedItems.firstObject.item;
    if (![item isKindOfClass:HeimdallMediaLibraryPlaylist.class]) {
        return;
    }

    HeimdallMediaLibraryPlaylist * const playlist = (HeimdallMediaLibraryPlaylist *)item;
    [HeimdallMain.sharedInstance.libraryController showRenamePlaylistDialogForPlaylist:playlist];
}

- (void)addMedia:(id)sender
{
    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    [openPanel setCanChooseFiles: NO];
    [openPanel setCanChooseDirectories: YES];
    [openPanel setAllowsMultipleSelection: YES];

    NSModalResponse modalResponse = [openPanel runModal];

    if (modalResponse == NSModalResponseOK) {
        HeimdallLibraryController *libraryController = HeimdallMain.sharedInstance.libraryController;
        for (NSURL *url in [openPanel URLs]) {
            [libraryController addFolderWithFileURL:url];
        }
    }
}

- (void)revealInFinder:(id)sender
{
    if (self.representedItems != nil && self.representedItems.count > 0) {
        [self.representedItems.firstObject revealInFinder];
    } else if (self.representedInputItems != nil && self.representedInputItems.count > 0) {
        [self.representedInputItems.firstObject revealInFinder];
    }
}

- (void)moveToTrash:(id)sender
{
    if (self.representedItems != nil && self.representedItems.count > 0) {
        for (HeimdallLibraryRepresentedItem * const item in self.representedItems) {
            [item moveToTrash];
        }
    } else if (self.representedInputItems != nil && self.representedInputItems.count > 0) {
        for (HeimdallInputItem * const inputItem in self.representedInputItems) {
            [inputItem moveToTrash];
        }
    }
}

- (void)showInformation:(id)sender
{
    if (!_informationWindowController) {
        _informationWindowController = [[HeimdallInformationWindowController alloc] init];
    }

    if (self.representedItems != nil && self.representedItems.count > 0) {
        [_informationWindowController setRepresentedMediaLibraryItems:self.representedItems];
    } else if (self.representedInputItems != nil && self.representedInputItems.count > 0) {
        _informationWindowController.representedInputItems = self.representedInputItems;
    }

    [_informationWindowController toggleWindow:sender];
}

- (void)addFavorite:(id)sender
{
    [self setItemsFavorite:YES];
}

- (void)removeFavorite:(id)sender
{
    [self setItemsFavorite:NO];
}

- (void)setItemsFavorite:(BOOL)favorite
{
    for (HeimdallLibraryRepresentedItem * const item in self.representedItems) {
        [item.item setFavorite:favorite];
    }
}

- (void)addToMediaLibrary:(id)sender
{
    if (self.representedInputItems == nil || self.representedInputItems.count == 0) {
        return;
    }
    
    HeimdallLibraryController * const libraryController = HeimdallMain.sharedInstance.libraryController;
    
    for (HeimdallInputItem * const inputItem in self.representedInputItems) {
        if (inputItem.inputType != ITEM_TYPE_DIRECTORY) {
            continue;
        }
        
        NSString * const inputItemMRL = inputItem.MRL;
        NSURL * const folderURL = [NSURL URLWithString:inputItemMRL];
        
        if (folderURL == nil) {
            msg_Warn(getIntf(), "Invalid URL for folder: %s", inputItemMRL.UTF8String);
            continue;
        }
        
        const int result = [libraryController addFolderWithFileURL:folderURL];
        if (result == HEIMDALL_SUCCESS) {
            msg_Info(getIntf(), "Added folder to media library: %s", inputItemMRL.UTF8String);
        } else {
            msg_Warn(getIntf(), "Failed to add folder to media library: %s (error %d)", inputItemMRL.UTF8String, result);
        }
    }
}

- (void)toggleBookmark:(id)sender
{
    if (self.representedInputItems == nil || 
        self.representedInputItems.count != 1 ||
        self.representedInputItems.firstObject.inputType != ITEM_TYPE_DIRECTORY) {
        return;
    }

    HeimdallInputItem * const inputItem = self.representedInputItems.firstObject;
    NSString * const inputItemMRL = inputItem.MRL;
    NSUserDefaults * const defaults = NSUserDefaults.standardUserDefaults;
    NSMutableArray<NSString *> * const bookmarkedLocations =
        [defaults stringArrayForKey:HeimdallLibraryBookmarkedLocationsKey].mutableCopy;
    NSNotificationCenter * const defaultCenter = NSNotificationCenter.defaultCenter;

    if ([bookmarkedLocations containsObject:inputItemMRL]) {
        [bookmarkedLocations removeObject:inputItemMRL];
    } else {
        [bookmarkedLocations addObject:inputItemMRL];
    }
    [defaults setObject:bookmarkedLocations forKey:HeimdallLibraryBookmarkedLocationsKey];
    [defaultCenter postNotificationName:HeimdallLibraryBookmarkedLocationsChanged object:inputItemMRL];
}

- (void)markUnseen:(id)sender
{
    heimdall_medialibrary_t * const p_ml = heimdall_ml_instance_get(getIntf());
    for (HeimdallLibraryRepresentedItem * const item in self.representedItems) {
        heimdall_ml_media_set_played(p_ml, item.item.libraryID, false);
    }
}

- (void)setRepresentedItems:(NSArray<HeimdallLibraryRepresentedItem *> *)items
{
    _representedItems = items;
    _representedInputItems = nil;
    _addToPlaylistMenuController.representedItems = items;
    [self updateMenuItems];
}

- (void)setRepresentedInputItems:(NSArray<HeimdallInputItem *> *)representedInputItems
{
    _representedInputItems = representedInputItems;
    _representedItems = nil;
    _addToPlaylistMenuController.representedItems = nil;
    [self updateMenuItems];
}

@end
