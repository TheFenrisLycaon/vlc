/*****************************************************************************
 * HeimdallPlaybackEndViewController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2025 VLC authors and VideoLAN
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

#import "HeimdallPlaybackEndViewController.h"

#import "extensions/NSArray+HeimdallAdditions.h"
#import "extensions/NSColor+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryModel.h"

#import "main/HeimdallMain.h"

#import "playqueue/HeimdallPlayQueueController.h"

#import "views/HeimdallUIUnits.h"

#include <heimdall_interface.h>

static const NSTimeInterval kHeimdallPlaybackEndTimeout = 10;
static const NSTimeInterval kHeimdallPlaybackEndUpdateInterval = 0.1;

NSString * const HeimdallPlaybackEndViewEnabledKey = @"HeimdallPlaybackEndViewEnabledKey";
NSString * const HeimdallPlaybackEndViewHideNotificationName = @"HeimdallPlaybackEndViewRequestHide";
NSString * const HeimdallPlaybackEndViewReturnToLibraryNotificationName = @"HeimdallPlaybackEndViewReturnToLibrary";

@interface HeimdallPlaybackEndViewController ()

@property NSDate *timeoutDate;
@property HeimdallInputItem *nextItem;

@end

@implementation HeimdallPlaybackEndViewController

- (instancetype)init
{
    return [super initWithNibName:@"HeimdallPlaybackEndView" bundle:nil];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.view.wantsLayer = YES;
    self.view.layer.cornerRadius = HeimdallUIUnits.cornerRadius;
    self.view.layer.borderColor = NSColor.HeimdallSubtleBorderColor.CGColor;
    self.view.layer.borderWidth = HeimdallUIUnits.borderThickness;
    self.largeTitleLabel.stringValue = _NS("Reached the end of the play queue");
    self.returnToLibraryButton.stringValue = _NS("Return to library");
    self.returnToLibraryButton.target = self;
    self.returnToLibraryButton.action = @selector(returnToLibrary:);
    self.restartPlayQueueButton.stringValue = _NS("Restart play queue");
    self.restartPlayQueueButton.target = self;
    self.restartPlayQueueButton.action = @selector(restartPlayQueue:);
    self.playNextItemButton.stringValue = _NS("Play next item");
    self.playNextItemButton.target = self;
    self.playNextItemButton.action = @selector(playNextItem:);
}

- (void)startCountdown
{
    [self.countdownTimer invalidate];

    HeimdallInputItem * const currentInputItem =
        HeimdallMain.sharedInstance.playQueueController.currentlyPlayingInputItem;
    NSURL * const inputItemUrl = [NSURL URLWithString:currentInputItem.MRL];
    HeimdallMediaLibraryMediaItem * const mediaLibraryItem =
        [HeimdallMediaLibraryMediaItem mediaItemForURL:inputItemUrl];

    if (mediaLibraryItem) {
        self.nextItem = [self nextItemForMediaLibraryItem:mediaLibraryItem];
    } else {
        self.nextItem = [self nextItemForInputItem:currentInputItem];
    }
    self.playNextItemButton.hidden = self.nextItem == nil;
    self.timeoutDate = [NSDate dateWithTimeIntervalSinceNow:kHeimdallPlaybackEndTimeout];
    _countdownTimer = [NSTimer scheduledTimerWithTimeInterval:kHeimdallPlaybackEndUpdateInterval
                                                       target:self
                                                     selector:@selector(handleUpdateInterval:)
                                                     userInfo:nil
                                                      repeats:YES];
    [self handleUpdateInterval:nil];
}

- (nullable HeimdallInputItem *)nextItemForMediaLibraryItem:(HeimdallMediaLibraryMediaItem *)item
{
    NSArray<HeimdallMediaLibraryMediaItem *> *parentMediaItemCollection = nil;
    __block NSInteger itemIndex = NSNotFound;

    if (item.mediaSubType == HEIMDALL_ML_MEDIA_SUBTYPE_ALBUMTRACK) {
        parentMediaItemCollection = [HeimdallMediaLibraryAlbum albumWithID:item.albumID].mediaItems;
        itemIndex = [parentMediaItemCollection indexOfMediaLibraryItem:item];
    } else if (item.mediaSubType == HEIMDALL_ML_MEDIA_SUBTYPE_SHOW_EPISODE) {
        NSArray<HeimdallMediaLibraryShow *> * const shows =
            HeimdallMain.sharedInstance.libraryController.libraryModel.listOfShows;
        const NSInteger showContainingEpisodeIdx = [shows indexOfObjectPassingTest:^BOOL(HeimdallMediaLibraryShow * const show, const NSUInteger __unused idx, BOOL * const __unused stop) {
            itemIndex = [show.mediaItems indexOfMediaLibraryItem:item];
            return itemIndex != NSNotFound;
        }];
        if (showContainingEpisodeIdx != NSNotFound) {
            parentMediaItemCollection = shows[showContainingEpisodeIdx].mediaItems;
        }
    }

    if (parentMediaItemCollection == nil ||
        itemIndex == NSNotFound ||
        (NSUInteger)(itemIndex + 1) >= parentMediaItemCollection.count
    ) {
        return [self nextItemForInputItem:item.inputItem];
    }
    return parentMediaItemCollection[itemIndex + 1].inputItem;
}

- (nullable HeimdallInputItem *)nextItemForInputItem:(HeimdallInputItem *)item
{
    if (item.isStream) {
        return nil;
    }

    NSURL * const itemUrl = [NSURL URLWithString:item.MRL];
    NSParameterAssert(itemUrl != nil);
    NSString * const parentFolderPath = itemUrl.URLByDeletingLastPathComponent.path;
    NSFileManager * const fm = NSFileManager.defaultManager;
    NSError *error = nil;
    NSArray<NSString *> * const itemSiblingItemPaths =
        [fm contentsOfDirectoryAtPath:parentFolderPath error:&error];

    if (error != nil) {
        NSLog(@"Could not find siblings for item: %@\n\tReceived error:%@", item.decodedMRL, error.localizedDescription);
        return nil;
    }

    NSArray<NSString *> * const playableExtensions = [NSString extensionsArrayFromHeimdallStyleString:EXTENSIONS_MEDIA];
    NSArray<NSString *> * const itemPlayableSiblingItemPaths = [[itemSiblingItemPaths
        sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)]
        filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(NSString * const _Nonnull siblingItemPath, NSDictionary<NSString *, id> * const _Nullable __unused bindings) {
            return [playableExtensions containsObject:siblingItemPath.pathExtension.lowercaseString];
        }]];
    const NSInteger itemIdx = [itemPlayableSiblingItemPaths indexOfObject:itemUrl.lastPathComponent];
    NSParameterAssert(itemIdx != NSNotFound);
    if ((NSUInteger)(itemIdx + 1) >= itemPlayableSiblingItemPaths.count) {
        NSLog(@"Played item was last in parent folder.");
        return nil;
    }
    NSString * const nextItemFileName = itemPlayableSiblingItemPaths[itemIdx + 1];
    NSURL * const nextItemURL =
        [[NSURL fileURLWithPath:parentFolderPath] URLByAppendingPathComponent:nextItemFileName];
    return [HeimdallInputItem inputItemFromURL:nextItemURL];
}

- (void)handleUpdateInterval:(nullable NSTimer *)timer
{
    NSDate * const now = NSDate.date;
    NSDate * const timeout = self.timeoutDate;
    const NSTimeInterval timeRemaining = [timeout timeIntervalSinceDate:now];
    if (timeRemaining <= 0) {
        if (self.nextItem) {
            [self playNextItem:self];
        } else {
            [self returnToLibrary:self];
        }
        return;
    }

    NSString *remainingTimeString = @"";
    if (@available(macOS 10.15, *)) {
        NSRelativeDateTimeFormatter * const formatter = [[NSRelativeDateTimeFormatter alloc] init];
        remainingTimeString = [formatter localizedStringForDate:timeout relativeToDate:now];
    } else {
        NSDateComponentsFormatter * const formatter = [[NSDateComponentsFormatter alloc] init];
        NSString * const timeString = [formatter stringFromTimeInterval:timeRemaining];
        remainingTimeString = [NSString stringWithFormat:_NS("in %@"), timeString];
    }

    NSString *countdownLabelString = nil;
    if (self.nextItem) {
        countdownLabelString =
            [NSString stringWithFormat:_NS("Playing %@ %@"), self.nextItem.title, remainingTimeString];
    } else {
        countdownLabelString =
            [NSString stringWithFormat:_NS("Returning to library %@"), remainingTimeString];
    }
    NSParameterAssert(countdownLabelString != nil);
    self.countdownLabel.stringValue = countdownLabelString;
}

- (void)setHideLibraryControls:(BOOL)hideLibraryControls
{
    if (self.hideLibraryControls == hideLibraryControls)
        return;
    _hideLibraryControls = hideLibraryControls;
    self.countdownLabel.hidden = hideLibraryControls;
    self.returnToLibraryButton.hidden = hideLibraryControls;
}

- (void)returnToLibrary:(id)sender
{
    [self.countdownTimer invalidate];
    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallPlaybackEndViewReturnToLibraryNotificationName
                                                      object:self];
    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallPlaybackEndViewHideNotificationName
                                                      object:self];
}

- (void)restartPlayQueue:(id)sender
{
    [self.countdownTimer invalidate];
    [HeimdallMain.sharedInstance.playQueueController playItemAtIndex:0];
    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallPlaybackEndViewHideNotificationName
                                                      object:self];
}

- (void)playNextItem:(id)sender
{
    [self.countdownTimer invalidate];
    if (self.nextItem == nil)
        return;
    [HeimdallMain.sharedInstance.playQueueController addInputItem:self.nextItem.heimdallInputItem
                                                  atPosition:-1
                                               startPlayback:YES];
    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallPlaybackEndViewHideNotificationName
                                                      object:self];
}

@end
