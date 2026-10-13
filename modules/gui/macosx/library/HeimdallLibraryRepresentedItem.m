/*****************************************************************************
 * HeimdallLibraryRepresentedItem.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2023 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Claudio Cambra <developer@claudiocambra.com>
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

#import "HeimdallLibraryRepresentedItem.h"

#import "extensions/NSString+Helpers.h"

#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryModel.h"

#import "library/audio-library/HeimdallLibraryAllAudioGroupsMediaLibraryItem.h"
#import "library/favorites-library/HeimdallLibraryFavoritesDataSource.h"

#import "main/HeimdallMain.h"

#import "playqueue/HeimdallPlayQueueController.h"

@interface HeimdallLibraryRepresentedItem ()
{
    NSInteger _itemIndexInParent; // Call self.itemIndexInParent, don't access directly
    NSArray<HeimdallMediaLibraryMediaItem *> * _parentMediaArray;
}

@property (readwrite) enum heimdall_ml_media_type_t mediaType;

@end

@implementation HeimdallLibraryRepresentedItem

- (instancetype)initWithItem:(const id<HeimdallMediaLibraryItemProtocol>)item
                  parentType:(const HeimdallMediaLibraryParentGroupType)parentType
{
    return [self initWithItem:item parentType:parentType parentItem:nil positionInParent:NSNotFound];
}

- (instancetype)initWithItem:(const id<HeimdallMediaLibraryItemProtocol>)item
                  parentType:(const HeimdallMediaLibraryParentGroupType)parentType
                  parentItem:(nullable const id<HeimdallMediaLibraryItemProtocol>)parentItem
            positionInParent:(NSInteger)positionInParent
{
    self = [self init];
    if (self) {
        _item = item;
        _parentType = parentType;
        _parentItem = parentItem;
        _positionInParent = positionInParent;
        _mediaType = item.firstMediaItem.mediaType;
    }
    return self;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        [self setup];
    }
    return self;
}

- (void)setup
{
    _itemIndexInParent = NSNotFound;
    _positionInParent = NSNotFound;
}

- (NSInteger)itemIndexInParent
{
    if (_parentItem != nil && _positionInParent != NSNotFound) {
        return _positionInParent;
    }

    @synchronized(self) {
        if (_itemIndexInParent == NSNotFound) {
            _itemIndexInParent = [self findItemIndexInParent];
        }

        return _itemIndexInParent;
    }
}

- (NSInteger)findItemIndexInParent
{
    NSArray<HeimdallMediaLibraryMediaItem *> * items = nil;

    if (self.parentItem != nil) {
        items = self.parentItem.mediaItems;
    } else if (self.parentType == HeimdallMediaLibraryParentGroupTypeUnknown) {
        HeimdallLibraryModel * const libraryModel = HeimdallMain.sharedInstance.libraryController.libraryModel;
        const BOOL isVideo = self.mediaType == HEIMDALL_ML_MEDIA_TYPE_VIDEO;
        items = isVideo ? libraryModel.listOfVideoMedia : libraryModel.listOfAudioMedia;
    } else {
        items = self.parentMediaArray;
    }

    // We search by mediaItem, so we want to find the index of the item in the parent.
    // Specifically, the first media item. For a media item, we will just receive its self.
    // For other types, we will get the first item. Albums, genres, and artists all provide
    // their media items in an album-by-album manner, so this works for us.
    //
    // Remember that library IDs for albums, genres, and artists are specific to them and we
    // cannot use these to search the list of audio media

    const int64_t itemId = self.item.firstMediaItem.libraryID;
    return [items indexOfObjectPassingTest:^BOOL(HeimdallMediaLibraryMediaItem * const mediaItem,
                                                 const NSUInteger __unused idx,
                                                 BOOL * const __unused stop) {
        return mediaItem.libraryID == itemId;
    }];
}

- (int64_t)parentItemIdForAudioItem:(const id<HeimdallMediaLibraryItemProtocol>)item
{
    // Decide which other items we are going to be adding to the play queue when playing the item.
    // Key for playing in library mode, not in individual mode
    int64_t parentItemId = NSNotFound;

    if ([item isKindOfClass:HeimdallMediaLibraryMediaItem.class]) {
        HeimdallMediaLibraryMediaItem * const mediaItem = (HeimdallMediaLibraryMediaItem *)item;

        switch (self.parentType) {
        case HeimdallMediaLibraryParentGroupTypeAlbum:
            parentItemId = mediaItem.albumID;
            break;
        case HeimdallMediaLibraryParentGroupTypeArtist:
            parentItemId = mediaItem.artistID;
            break;
        case HeimdallMediaLibraryParentGroupTypeGenre:
            parentItemId = mediaItem.genreID;
            break;
        default:
            break;
        }
    } else if ([item isKindOfClass:HeimdallMediaLibraryAlbum.class]) {
        HeimdallMediaLibraryAlbum * const album = (HeimdallMediaLibraryAlbum *)item;

        switch (self.parentType) {
        case HeimdallMediaLibraryParentGroupTypeArtist:
            parentItemId = album.artistID;
            break;
        case HeimdallMediaLibraryParentGroupTypeGenre:
        {
            HeimdallMediaLibraryMediaItem * const firstChildItem = album.firstMediaItem;
            parentItemId = firstChildItem.genreID;
            break;
        }
        default:
            break;
        }
    }

    return parentItemId;
}

- (const id<HeimdallMediaLibraryItemProtocol>)parentItemForAudioItem:(const id<HeimdallMediaLibraryItemProtocol>)item
{
    // If we have no defined parent type, use the all audio groups item.
    // This item essentially represents the entirety of the library and all of its media items.
    const HeimdallMediaLibraryParentGroupType parentType = self.parentType;
    if (parentType == HeimdallMediaLibraryParentGroupTypeUnknown || parentType == HeimdallMediaLibraryParentGroupTypeAudioLibrary) {
        return [[HeimdallLibraryAllAudioGroupsMediaLibraryItem alloc] initWithDisplayString:_NS("All items")];
    } else if ([self.item conformsToProtocol:@protocol(HeimdallMediaLibraryAudioGroupProtocol)]) {
        // If the parent item class and the actual item class are the same, we likely want
        // to also play the entirety of the library -- think of playing an album within the
        // albums view, or playing a song within the songs view.
        const id<HeimdallMediaLibraryAudioGroupProtocol> audioGroupItem = (id<HeimdallMediaLibraryAudioGroupProtocol>)self.item;
        if (audioGroupItem.matchingParentType == parentType) {
            return [[HeimdallLibraryAllAudioGroupsMediaLibraryItem alloc] initWithDisplayString:_NS("All items")
                                                                     accordingToParentType:self.parentType];
        }
    }

    const int64_t parentItemId = [self parentItemIdForAudioItem:item];
    if (parentItemId == NSNotFound) {
        return nil;
    }

    switch (parentType) {
    case HeimdallMediaLibraryParentGroupTypeAlbum:
        return [HeimdallMediaLibraryAlbum albumWithID:parentItemId];
    case HeimdallMediaLibraryParentGroupTypeArtist:
        return [HeimdallMediaLibraryArtist artistWithID:parentItemId];
    case HeimdallMediaLibraryParentGroupTypeGenre:
        return [HeimdallMediaLibraryGenre genreWithID:parentItemId];
    default:
        return nil;
    }
}

- (NSArray<HeimdallMediaLibraryMediaItem *> *)parentMediaArrayForItem:(const id<HeimdallMediaLibraryItemProtocol>)item
{
    if (self.parentItem != nil) {
        return self.parentItem.mediaItems;
    }

    if (self.parentType == HeimdallMediaLibraryParentGroupTypeAllFavorites) {
        HeimdallLibraryModel * const libraryModel = HeimdallMain.sharedInstance.libraryController.libraryModel;
        return [libraryModel listOfMediaItemsForParentType:self.parentType];
    }

    const BOOL isVideo = self.mediaType == HEIMDALL_ML_MEDIA_TYPE_VIDEO;
    if (isVideo) {
        HeimdallLibraryModel * const libraryModel = HeimdallMain.sharedInstance.libraryController.libraryModel;
        return [libraryModel listOfMediaItemsForParentType:self.parentType];
    } else {
        const id<HeimdallMediaLibraryItemProtocol> parentAudioItem = [self parentItemForAudioItem:item];
        return parentAudioItem.mediaItems;
    }
}

- (NSArray<HeimdallMediaLibraryMediaItem *> *)parentMediaArray
{
    @synchronized(self) {
        if ((self.parentType != HeimdallMediaLibraryParentGroupTypeUnknown || self.parentItem != nil) &&
            (_parentMediaArray == nil || _parentMediaArray.count == 0)) {
            _parentMediaArray = [self parentMediaArrayForItem:self.item];
        }

        return _parentMediaArray;
    }
}

- (void)playIndividualModeImmediately:(BOOL)playImmediately
{
    HeimdallLibraryController * const libraryController = HeimdallMain.sharedInstance.libraryController;

    // If play immediately, play first item, queue following items
    // If not then just queue all items
    __block BOOL startingPlayImmediately = playImmediately;

    [self.item iterateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem* mediaItem) {
        [libraryController appendItemToPlayQueue:mediaItem playImmediately:startingPlayImmediately];

        if (startingPlayImmediately) {
            startingPlayImmediately = NO;
        }
    }];
}

- (void)playLibraryModeImmediately:(BOOL)playImmediately
{
    HeimdallPlayQueueController * const playQueueController = HeimdallMain.sharedInstance.playQueueController;
    HeimdallLibraryController * const libraryController = HeimdallMain.sharedInstance.libraryController;

    // If play immediately, play first item, queue following items
    // If not then just queue all items
    __block BOOL startingPlayImmediately = playImmediately;

    NSArray<HeimdallMediaLibraryMediaItem *> * const parentItems = self.parentMediaArray;
    const NSUInteger parentItemCount = parentItems.count;

    if (parentItemCount == 0) {
        [self playIndividualModeImmediately:playImmediately];
        return;
    }

    const NSUInteger itemIndexInParent = self.itemIndexInParent;
    const NSUInteger startingIndex = itemIndexInParent == NSNotFound ? 0 : itemIndexInParent;

    for (NSUInteger i = startingIndex; i < parentItemCount; i++) {
        HeimdallMediaLibraryMediaItem * const mediaItem = [parentItems objectAtIndex:i];
        [libraryController appendItemToPlayQueue:mediaItem playImmediately:startingPlayImmediately];

        if (startingPlayImmediately) {
            startingPlayImmediately = NO;
        }
    }

    if (playQueueController.playbackRepeat != HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_NONE) {
        for (NSUInteger i = 0; i < startingIndex; i++) {
            HeimdallMediaLibraryMediaItem * const mediaItem = [parentItems objectAtIndex:i];
            [libraryController appendItemToPlayQueue:mediaItem playImmediately:NO];
        }
    }
}

- (void)playImmediately:(BOOL)playImmediately
{
    HeimdallPlayQueueController * const playQueueController = HeimdallMain.sharedInstance.playQueueController;
    if (playQueueController.libraryPlayQueueMode &&
        (self.parentType != HeimdallMediaLibraryParentGroupTypeUnknown || self.parentItem != nil)) {
        [self playLibraryModeImmediately:playImmediately];
    } else {
        [self playIndividualModeImmediately:playImmediately];
    }
}

- (void)play
{
    [self playImmediately:YES];
}

- (void)queue
{
    [self playImmediately:NO];
}

- (void)revealInFinder
{
    [self.item revealInFinder];
}

- (void)moveToTrash
{
    [self.item moveToTrash];
}

@end
