/*****************************************************************************
 * HeimdallLibraryHeroView.m: MacOS X interface module
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

#import "HeimdallLibraryHeroView.h"

#import "extensions/NSColor+HeimdallAdditions.h"
#import "extensions/NSFont+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"
#import "extensions/NSView+HeimdallAdditions.h"

#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryImageCache.h"
#import "library/HeimdallLibraryMenuController.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibraryRepresentedItem.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallImageView.h"

@interface HeimdallLibraryHeroView ()

@property (readonly) HeimdallLibraryMenuController *menuController;

@end

@implementation HeimdallLibraryHeroView

- (void)awakeFromNib
{
    self.largeImageView.contentGravity = HeimdallImageViewContentGravityResizeAspectFill;
    self.playButton.bezelColor = NSColor.HeimdallAccentColor;
    self.playButton.font = NSFont.HeimdallLibrarySubsectionSubheaderFont;
    if (@available(macOS 10.14, *)) {
        self.playButton.contentTintColor = NSColor.HeimdallAccentColor;
    }
    self.titleTextField.font = NSFont.HeimdallLibrarySubsectionHeaderFont;
    self.titleTextField.maximumNumberOfLines = 3;
    self.detailTextField.font = NSFont.HeimdallLibrarySubsectionSubheaderFont;
    self.detailTextField.maximumNumberOfLines = 1;
    [self connectItemUpdaters];
}

- (void)updateRepresentedItem
{
    NSAssert(self.representedItem != nil, @"Should not update nil represented item!");
    const id<HeimdallMediaLibraryItemProtocol> actualItem = self.representedItem.item;
    self.titleTextField.stringValue = actualItem.displayString;
    self.detailTextField.stringValue = actualItem.primaryDetailString;

    __weak typeof(self) weakSelf = self;
    [HeimdallLibraryImageCache thumbnailForLibraryItem:actualItem withCompletion:^(NSImage * const image) {
        if (!weakSelf || weakSelf.representedItem.item != actualItem) {
            return;
        }
        weakSelf.largeImageView.image = image;
    }];
}

- (void)setRepresentedItem:(HeimdallLibraryRepresentedItem *)representedItem
{
    NSParameterAssert(representedItem != nil);
    if (representedItem == self.representedItem) {
        return;
    }

    _representedItem = representedItem;
    [self updateRepresentedItem];
}

- (HeimdallMediaLibraryMediaItem *)randomItem
{
    HeimdallLibraryModel * const libraryModel = HeimdallMain.sharedInstance.libraryController.libraryModel;
    NSArray * const videos = libraryModel.listOfVideoMedia;
    const NSInteger videoCount = videos.count;
    if (videoCount == 0) {
        return nil;
    }
    const uint32_t randIdx = arc4random_uniform((uint32_t)(videoCount - 1));
    return [videos objectAtIndex:randIdx];
}

- (HeimdallMediaLibraryMediaItem *)latestPartiallyPlayedItem
{
    HeimdallLibraryModel * const libraryModel = HeimdallMain.sharedInstance.libraryController.libraryModel;
    NSArray<HeimdallMediaLibraryMediaItem *> * const recentMedia = libraryModel.listOfRecentMedia;
    const NSUInteger firstPartialPlayItemIdx = [recentMedia indexOfObjectPassingTest:^BOOL(HeimdallMediaLibraryMediaItem *testedItem, NSUInteger __unused idx, BOOL * const __unused stop) {
        const float playProgress = testedItem.progress;
        return playProgress > 0 && playProgress < 100;
    }];

    if (firstPartialPlayItemIdx == NSNotFound) {
        return nil;
    }

    return [recentMedia objectAtIndex:firstPartialPlayItemIdx];
}

- (void)setOptimalRepresentedItem
{
    HeimdallMediaLibraryMediaItem * const latestPartialPlayItem = self.latestPartiallyPlayedItem;
    if (latestPartialPlayItem != nil) {
        const BOOL isVideo = latestPartialPlayItem.mediaType == HEIMDALL_ML_MEDIA_TYPE_VIDEO;
        const HeimdallMediaLibraryParentGroupType parentType = isVideo ? HeimdallMediaLibraryParentGroupTypeRecentVideos : HeimdallMediaLibraryParentGroupTypeAudioLibrary;
        HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:latestPartialPlayItem parentType:parentType];

        self.representedItem = representedItem;
        self.explanationTextField.stringValue = _NS("Last watched");
        self.playButton.title = _NS("Resume playing");
        return;
    }

    HeimdallMediaLibraryMediaItem * const randomItem = self.randomItem;
    if (randomItem != nil) {
        const BOOL isVideo = randomItem.mediaType == HEIMDALL_ML_MEDIA_TYPE_VIDEO;
        const HeimdallMediaLibraryParentGroupType parentType = isVideo ? HeimdallMediaLibraryParentGroupTypeVideoLibrary : HeimdallMediaLibraryParentGroupTypeAudioLibrary;
        HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:randomItem parentType:parentType];

        self.representedItem = representedItem;
        self.explanationTextField.stringValue = _NS("From your library");
        self.playButton.title = _NS("Play now");
        return;
    }

    NSLog(@"Could not find a good media item for hero view!");
    [self connectForNewVideo];
}

- (IBAction)playRepresentedItem:(id)sender
{
    [self.representedItem play];
}

- (void)connectForNewVideo
{
    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(newVideosAvailable:)
                               name:HeimdallLibraryModelVideoMediaListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(newVideosAvailable:)
                               name:HeimdallLibraryModelRecentsMediaListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(newVideosAvailable:)
                               name:HeimdallLibraryModelAllCachesDropped
                             object:nil];
}

- (void)disconnectForNewVideo
{
    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter removeObserver:self name:HeimdallLibraryModelVideoMediaListReset object:nil];
    [notificationCenter removeObserver:self name:HeimdallLibraryModelRecentsMediaListReset object:nil];
    [notificationCenter removeObserver:self name:HeimdallLibraryModelAllCachesDropped object:nil];
}

- (void)newVideosAvailable:(NSNotification *)notification
{
    [self setOptimalRepresentedItem];
    [self disconnectForNewVideo];
}

- (void)connectItemUpdaters
{
    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(itemUpdated:)
                               name:HeimdallLibraryModelVideoMediaItemUpdated
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(itemDeleted:)
                               name:HeimdallLibraryModelVideoMediaItemDeleted
                             object:nil];
}

- (void)itemUpdated:(NSNotification *)notification
{
    HeimdallMediaLibraryMediaItem * const mediaItem = notification.object;
    NSAssert(mediaItem != nil, @"Notification should contain a media item!");
    if (mediaItem.libraryID != self.representedItem.item.libraryID) {
        return;
    } else if (mediaItem.progress == 0 || mediaItem.progress == 100 || mediaItem.playCount == 0) {
        // If the item is now unplayed or fully played, it is not optimal anymore
        [self setOptimalRepresentedItem];
        return;
    }

    HeimdallLibraryRepresentedItem * const item =
        [[HeimdallLibraryRepresentedItem alloc] initWithItem:mediaItem
                                             parentType:self.representedItem.parentType];
    self.representedItem = item;
}

- (void)itemDeleted:(NSNotification *)notification
{
    HeimdallMediaLibraryMediaItem * const mediaItem = notification.object;
    NSAssert(mediaItem != nil, @"Notification should contain a media item!");
    if (mediaItem.libraryID != self.representedItem.item.libraryID) {
        return;
    }

    [self setOptimalRepresentedItem];
}

- (void)openContextMenu:(NSEvent *)event
{
    if (self.menuController == nil) {
        _menuController = [[HeimdallLibraryMenuController alloc] init];
    }

    self.menuController.representedItems = @[self.representedItem];
    [self.menuController popupMenuWithEvent:event forView:self];
}

- (void)mouseDown:(NSEvent *)event
{
    if (event.modifierFlags & NSEventModifierFlagControl) {
        [self openContextMenu:event];
    }

    [super mouseDown:event];
}

- (void)rightMouseDown:(NSEvent *)event
{
    [self openContextMenu:event];
    [super rightMouseDown:event];
}

@end
