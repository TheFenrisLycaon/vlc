/*****************************************************************************
 * HeimdallPlaybackContinuityController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2015-2020 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne # videolan.org>
 *          David Fuhrmann <dfuhrmann # videolan.org>
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

#import "HeimdallPlaybackContinuityController.h"

#import "extensions/NSString+Helpers.h"
#import "main/HeimdallMain.h"
#import "panels/dialogs/HeimdallResumeDialogController.h"
#import "playqueue/HeimdallPlayQueueController.h"
#import "playqueue/HeimdallPlayerController.h"
#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryDataTypes.h"

#import <heimdall_configuration.h>

static const int64_t SecInMillisecs = 1000;
static const int64_t MinInMillisecs = SecInMillisecs * 60;
static const int64_t MinimumDuration = 3 * MinInMillisecs;
static const float MinimumStorePercent = 0.05;
static const float MaximumStorePercent = 0.95;
static const int64_t MinimumStoreTime = MinInMillisecs;
static const int64_t MinimumStoreRemainingTime = MinInMillisecs;
static NSString *HeimdallRecentlyPlayedMediaKey = @"recentlyPlayedMedia";
static NSString *HeimdallRecentlyPlayedMediaListKey = @"recentlyPlayedMediaList";

@interface HeimdallPlaybackContinuityController()
{
    HeimdallInputItem *_currentInput;
    HeimdallResumeDialogController *_resumeDialogController;
}
@end

@implementation HeimdallPlaybackContinuityController

+ (void)initialize
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                 [NSArray array], HeimdallRecentlyPlayedMediaListKey,
                                 [NSDictionary dictionary], HeimdallRecentlyPlayedMediaKey, nil];

    [defaults registerDefaults:appDefaults];
}

- (id)init
{
    self = [super init];
    if (self) {
        NSNotificationCenter *notificationCenter = NSNotificationCenter.defaultCenter;
        [notificationCenter addObserver:self
                               selector:@selector(inputItemChanged:)
                                   name:HeimdallPlayerCurrentMediaItemChanged
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(playbackStatusUpdated:)
                                   name:HeimdallPlayerStateChanged
                                 object:nil];
    }
    return self;
}

- (void)dealloc
{
    msg_Dbg(getIntf(), "Deinitializing input manager");

    [NSNotificationCenter.defaultCenter removeObserver:self];

    if (_currentInput) {
        /* continue playback where you left off */
        [self storePlaybackPositionForItem:_currentInput player:HeimdallMain.sharedInstance.playQueueController.playerController];
    }
}

- (void)inputItemChanged:(NSNotification *)aNotification
{
    HeimdallPlayerController * const playerController = HeimdallMain.sharedInstance.playQueueController.playerController;

    // Cancel pending resume dialogs
    [_resumeDialogController cancel];

    // object is hold here and released then it is dead
    _currentInput = playerController.currentMedia;
    if (_currentInput) {
        [self continuePlaybackWhereYouLeftOff:_currentInput
                                       player:playerController];
    }
}

- (void)playbackStatusUpdated:(NSNotification *)aNotification
{
    // On shutdown, input might not be dead yet. Cleanup actions like itunes playback
    // and playback positon are done in different code paths (dealloc and appWillTerminate:).
    if ([HeimdallMain.sharedInstance isTerminating]) {
        return;
    }

    HeimdallPlayerController *playerController = aNotification.object;
    enum heimdall_player_state playerState = [playerController playerState];

    if (playerState == HEIMDALL_PLAYER_STATE_STOPPED || playerState == HEIMDALL_PLAYER_STATE_STOPPING) {
        /* continue playback where you left off */
        if (_currentInput)
            [self storePlaybackPositionForItem:_currentInput player:playerController];
    }
}

- (BOOL)isValidResumeItem:(HeimdallInputItem *)inputItem
{
    NSString *urlString = inputItem.MRL;

    if ([urlString isEqualToString:@""])
        return NO;

    NSURL *url = [NSURL URLWithString:urlString];

    if (![url isFileURL])
        return NO;

    BOOL isDir = false;
    if (![[NSFileManager defaultManager] fileExistsAtPath:[url path] isDirectory:&isDir])
        return NO;

    if (isDir)
        return NO;

    return YES;
}

- (void)continuePlaybackWhereYouLeftOff:(HeimdallInputItem *)inputItem player:(HeimdallPlayerController *)playerController
{
    int settingValue = (int)config_GetInt("macosx-continue-playback");
    if (settingValue == 2) // never resume
        return;

    if (!inputItem)
        return;

    /* allow the user to over-write the start/stop/run-time */
    NSArray<NSString *> * const options = inputItem.options;
    if (options == nil) {
        return;
    }

    const NSUInteger runtimeOption = [options indexOfObjectPassingTest:^BOOL(NSString * const obj, const NSUInteger __unused idx, BOOL * const __unused stop){
        return [obj hasPrefix:@"start-time"] || [obj hasPrefix:@"stop-time"];
    }];

    if (runtimeOption != NSNotFound) {
        return;
    }

    /* check for file existence before resuming */
    if (![self isValidResumeItem:inputItem])
        return;

    NSURL *url = [NSURL URLWithString:inputItem.MRL];
    if (!url)
        return;

    HeimdallMediaLibraryMediaItem * const libraryMediaItem = [HeimdallMediaLibraryMediaItem mediaItemForURL:url];
    if (!libraryMediaItem) {
        // input item isn't part of the library
        [self resumeLegacyPlaybackPositionForInputItem:inputItem
                                                   ask:(settingValue == 0)
                                                player:playerController];
        return;
    }

    [self resumePlaybackPositionForLibraryMediaItem:libraryMediaItem
                                                ask:(settingValue == 0)
                                             player:playerController];
}

- (void)resumePlaybackPositionForLibraryMediaItem:(HeimdallMediaLibraryMediaItem *)libraryMediaItem
                                              ask:(BOOL)ask
                                           player:(HeimdallPlayerController *)playerController
{
    NSParameterAssert(libraryMediaItem != nil);

    const float lastPlaybackPosition = libraryMediaItem.progress;
    const int64_t duration = playerController.durationOfCurrentMediaItem;
    const BOOL isAlbumTrack = libraryMediaItem.mediaSubType == HEIMDALL_ML_MEDIA_SUBTYPE_ALBUMTRACK;

    if (lastPlaybackPosition < MinimumStorePercent || duration < MinimumDuration || isAlbumTrack) {
        return;
    }

    CompletionBlock completionBlock = ^(enum ResumeResult result) {
        if (result == RESUME_RESTART)
            return;

        msg_Dbg(getIntf(), "continuing playback at %0.2f", lastPlaybackPosition);
        [playerController setPositionPrecise:lastPlaybackPosition];
    };

    if (!ask) { // always
        completionBlock(RESUME_NOW);
        return;
    }

    if (!_resumeDialogController) {
        _resumeDialogController = [[HeimdallResumeDialogController alloc] init];
    }

    [_resumeDialogController showWindowWithItem:libraryMediaItem.inputItem
                               withLastPosition:(lastPlaybackPosition * duration) / 1000
                                completionBlock:completionBlock];
}

- (void)resumeLegacyPlaybackPositionForInputItem:(HeimdallInputItem *)inputItem
                                             ask:(BOOL)ask
                                          player:(HeimdallPlayerController *)playerController
{
    NSDictionary *recentlyPlayedFiles = [[NSUserDefaults standardUserDefaults] objectForKey:HeimdallRecentlyPlayedMediaKey];
    if (!recentlyPlayedFiles)
        return;

    NSNumber *lastPosition = [recentlyPlayedFiles objectForKey:inputItem.MRL];
    if (!lastPosition || lastPosition.intValue <= 0)
        return;

    CompletionBlock completionBlock = ^(enum ResumeResult result) {
        if (result == RESUME_RESTART)
            return;

        heimdall_tick_t lastPos = heimdall_tick_from_sec( lastPosition.intValue );
        msg_Dbg(getIntf(), "continuing playback at %lld", lastPos);

        [playerController setTimePrecise: lastPos];
    };

    if (!ask) { // always
        completionBlock(RESUME_NOW);
        return;
    }

    if (!_resumeDialogController) {
        _resumeDialogController = [[HeimdallResumeDialogController alloc] init];
    }

    [_resumeDialogController showWindowWithItem:inputItem
                               withLastPosition:lastPosition.intValue
                                completionBlock:completionBlock];
}

BOOL ShouldStorePlaybackPosition(float position, int64_t duration)
{
    int64_t positionTime = position * duration;
    int64_t remainingTime = duration - positionTime;

    if (duration < MinimumDuration) {
        return NO;
    }

    if (position < MinimumStorePercent && positionTime < MinimumStoreTime) {
        return NO;
    }

    if (position > MaximumStorePercent && remainingTime < MinimumStoreRemainingTime) {
        return NO;
    }

    return YES;
}

- (void)storePlaybackPositionForItem:(HeimdallInputItem *)inputItem
                              player:(HeimdallPlayerController *)playerController
{
    if (!var_InheritBool(getIntf(), "macosx-recentitems"))
        return;

    if (!inputItem)
        return;

    if (![self isValidResumeItem:inputItem])
        return;

    NSURL *url = [NSURL URLWithString:inputItem.MRL];
    if (!url)
        return;

    HeimdallMediaLibraryMediaItem *libraryMediaItem = [HeimdallMediaLibraryMediaItem mediaItemForURL:url];
    if (!libraryMediaItem) {
        // input item isn't part of the library
        [self storeLegacyPlaybackPositionForInputItem:inputItem withPlayer:playerController];
    }
}

- (void)storeLegacyPlaybackPositionForInputItem:(HeimdallInputItem *)inputItem
                                     withPlayer:(HeimdallPlayerController *)playerController
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSMutableDictionary *mutDict = [[NSMutableDictionary alloc] initWithDictionary:[defaults objectForKey:HeimdallRecentlyPlayedMediaKey]];

    float relativePos = playerController.position;
    long long pos = SEC_FROM_HEIMDALL_TICK(playerController.time);
    long long dur = SEC_FROM_HEIMDALL_TICK(inputItem.duration);

    NSMutableArray *mediaList = [[defaults objectForKey:HeimdallRecentlyPlayedMediaListKey] mutableCopy];
    NSString *mrl = inputItem.MRL;

    if (ShouldStorePlaybackPosition(relativePos, dur*1000)) {
        msg_Dbg(getIntf(), "Store current playback position of %f", relativePos);
        [mutDict setObject:[NSNumber numberWithInteger:pos] forKey:inputItem.MRL];

        [mediaList removeObject:mrl];
        [mediaList addObject:mrl];
        NSUInteger mediaListCount = mediaList.count;
        if (mediaListCount > 30) {
            for (NSUInteger x = 0; x < mediaListCount - 30; x++) {
                [mutDict removeObjectForKey:[mediaList firstObject]];
                [mediaList removeObjectAtIndex:0];
            }
        }
    } else {
        [mutDict removeObjectForKey:mrl];
        [mediaList removeObject:mrl];
    }
    [defaults setObject:mutDict forKey:HeimdallRecentlyPlayedMediaKey];
    [defaults setObject:mediaList forKey:HeimdallRecentlyPlayedMediaListKey];
}

@end
