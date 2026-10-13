/*****************************************************************************
 * HeimdallStatusBarIcon.m: Status bar icon controller/delegate
 *****************************************************************************
 * Copyright (C) 2016-2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Goran Dokic <heimdall at 8hz dot com>
 *          Felix Paul Kühne <fkuehne # videolan.org>
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

#import "HeimdallStatusBarIcon.h"

#import "extensions/NSImage+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"
#import "main/HeimdallMain.h"
#import "playqueue/HeimdallPlayQueueController.h"
#import "playqueue/HeimdallPlayerController.h"
#import "library/HeimdallLibraryImageCache.h"
#import "library/HeimdallInputItem.h"
#import "windows/HeimdallDetachedAudioWindow.h"

#import <heimdall_configuration.h>

@interface HeimdallStatusBarIcon ()
{
    intf_thread_t *_intf;

    NSMenuItem *_heimdallStatusBarMenuItem;

    /* Outlets for Now Playing labels */
    IBOutlet NSTextField *titleField;
    IBOutlet NSTextField *artistField;
    IBOutlet NSTextField *albumField;
    IBOutlet NSTextField *progressField;
    IBOutlet NSTextField *separatorField;
    IBOutlet NSTextField *totalField;
    IBOutlet NSImageView *coverImageView;

    /* Outlets for player controls */
    IBOutlet NSButton *backwardsButton;
    IBOutlet NSButton *playPauseButton;
    IBOutlet NSButton *forwardButton;
    IBOutlet NSButton *randomButton;

    /* Outlets for menu items */
    IBOutlet NSMenuItem *pathActionItem;
    IBOutlet NSMenuItem *showMainWindowItem;
    IBOutlet NSMenuItem *quitItem;

    BOOL _showTimeElapsed;
    NSString *_currentPlaybackUrl;

    HeimdallDetachedAudioWindow *_detachedAudioWindow;
}
@end

#pragma mark -
#pragma mark Implementation

@implementation HeimdallStatusBarIcon

#pragma mark -
#pragma mark Init

- (instancetype)init:(intf_thread_t *)intf
{
    self = [super init];
    if (self) {
        _intf = intf;
        [[NSBundle mainBundle] loadNibNamed:@"HeimdallStatusBarIconMainMenu" owner:self topLevelObjects:nil];
    }
    return self;
}

- (void)awakeFromNib
{
    [super awakeFromNib];

    [_controlsView setAutoresizingMask:NSViewWidthSizable];
    [_playbackInfoView setAutoresizingMask:NSViewWidthSizable];

    if (@available(macOS 11.0, *)) {
        backwardsButton.image = [NSImage imageWithSystemSymbolName:@"backward.fill"
                                       accessibilityDescription:_NS("Previous")];
        backwardsButton.alternateImage = backwardsButton.image;
        playPauseButton.image = [NSImage imageWithSystemSymbolName:@"play.fill"
                                         accessibilityDescription:_NS("Play")];
        playPauseButton.alternateImage = [NSImage imageWithSystemSymbolName:@"pause.fill"
                                                     accessibilityDescription:_NS("Pause")];
        forwardButton.image = [NSImage imageWithSystemSymbolName:@"forward.fill"
                                    accessibilityDescription:_NS("Next")];
        forwardButton.alternateImage = forwardButton.image;
        randomButton.image = [NSImage imageWithSystemSymbolName:@"shuffle"
                                    accessibilityDescription:_NS("Shuffle")];
    }

    [self configurationChanged:nil];

    // Set Accessibility Attributes for Image Buttons
    backwardsButton.accessibilityLabel = _NS("Go to previous item");
    playPauseButton.accessibilityLabel = _NS("Toggle Play/Pause");
    forwardButton.accessibilityLabel = _NS("Go to next item");
    randomButton.accessibilityLabel = _NS("Toggle random order playback");

    // Populate menu items with localized strings
    [showMainWindowItem setTitle:_NS("Show Main Window")];
    [pathActionItem setTitle:_NS("Path/URL Action")];
    [quitItem setTitle:_NS("Quit")];

    _showTimeElapsed = YES;

    // Set our selves up as delegate, to receive menuNeedsUpdate messages, so
    // we can update our menu as needed/before it's drawn
    [_heimdallStatusBarIconMenu setDelegate:self];

    // Register notifications
    NSNotificationCenter *notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(updateTimeAndPosition:)
                               name:HeimdallPlayerTimeAndPositionChanged
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(inputItemChanged:)
                               name:HeimdallPlayerCurrentMediaItemChanged
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(hasPreviousChanged:)
                               name:HeimdallPlaybackHasPreviousChanged
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(hasNextChanged:)
                               name:HeimdallPlaybackHasNextChanged
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(playerStateChanged:)
                               name:HeimdallPlayerStateChanged
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(configurationChanged:)
                               name:HeimdallConfigurationChangedNotification
                             object:nil];

    [self inputItemChanged:nil];

    [self setMetadataTitle:_NS("Heimdall media player") artist:_NS("Nothing playing") album:nil andCover:NSImage.HeimdallNoArtImage];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
    if ([keyPath isEqualToString: NSStringFromSelector(@selector(isVisible))]) {
        bool isVisible = [[change objectForKey:NSKeyValueChangeNewKey] boolValue];

        // Sync status bar visibility with Heimdall setting
        msg_Dbg(getIntf(), "Status bar icon visibility changed to %i", isVisible);
        config_PutInt("macosx-statusicon", isVisible ? 1 : 0);
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)configurationChanged:(NSNotification *)aNotification
{
    if (var_InheritBool(getIntf(), "macosx-statusicon"))
        [self enableMenuIcon];
    else
        [self disableStatusItem];
}

/* Enables the Status Bar Item and initializes it's image
 * and context menu
 */
- (void)enableMenuIcon
{
    if (!self.statusItem) {
        // Init the status item
        self.statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
        [self.statusItem setHighlightMode:YES];
        [self.statusItem setEnabled:YES];

        // Set the status item image
        NSImage *menuIcon = NSImage.HeimdallStatusBarIconImage;
        [menuIcon setTemplate:YES];
        [self.statusItem setImage:menuIcon];

        // Attach pull-down menu
        [self.statusItem setMenu:_heimdallStatusBarIconMenu];

        [self.statusItem setBehavior:NSStatusItemBehaviorRemovalAllowed];
        [self.statusItem setAutosaveName:@"statusBarItem"];
        [self.statusItem addObserver:self forKeyPath:NSStringFromSelector(@selector(isVisible))
                             options:NSKeyValueObservingOptionNew context:NULL];
    }

    // Sync Heimdall setting with status bar visibility setting
    [self.statusItem setVisible:YES];
}

- (void)disableStatusItem
{
    if (!self.statusItem)
        return;

    self.statusItem.visible = NO;
}

- (void)dealloc
{
    if (self.statusItem && [self.statusItem respondsToSelector:@selector(isVisible)]) {
        [self.statusItem removeObserver:self forKeyPath:NSStringFromSelector(@selector(isVisible)) context:NULL];
    }

    // Cleanup
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark -
#pragma mark Event callback functions

/* Menu update delegate
 * Called before menu is opened/displayed
 */
- (void)menuNeedsUpdate:(NSMenu *)menu
{
    [self updateMenuItemRandom];
    [self updateDynamicMenuItemText];
}

/* Callback to update current playback time
 * Called by InputManager
 */
- (void)updateTimeAndPosition:(NSNotification *)aNotification
{
    HeimdallPlayerController *playerController = aNotification.object;

    HeimdallInputItem *inputItem = playerController.currentMedia;

    if (inputItem) {
        heimdall_tick_t duration = inputItem.duration;
        heimdall_tick_t time = playerController.time;

        if (duration == 0) {
            /* Infinite duration */
            [progressField setStringValue:[NSString stringWithDuration:duration currentTime:time negative:NO]];
            [totalField setStringValue:@"∞"];
        } else {
            /* Not unknown, update displayed duration */
            if (_showTimeElapsed) {
                [progressField setStringValue:[NSString stringWithDuration:duration currentTime:time negative:NO]];
            } else {
                [progressField setStringValue:[NSString stringWithDuration:duration currentTime:time negative:YES]];
            }

            [totalField setStringValue:[NSString stringWithTimeFromTicks:duration]];
        }
    } else {
        /* Nothing playing */
        [progressField setStringValue:@"--:--"];
        [totalField setStringValue:@"--:--"];
    }
}

#pragma mark -
#pragma mark Update functions

- (void)updateCachedURLOfCurrentMedia:(HeimdallInputItem *)inputItem
{
    if (!inputItem) {
        _currentPlaybackUrl = nil;
        return;
    }

    _currentPlaybackUrl = inputItem.decodedMRL;
}

- (void)hasPreviousChanged:(NSNotification *)aNotification
{
    backwardsButton.enabled = HeimdallMain.sharedInstance.playQueueController.hasPreviousPlayQueueItem;
}

- (void)hasNextChanged:(NSNotification *)aNotification
{
    forwardButton.enabled = HeimdallMain.sharedInstance.playQueueController.hasNextPlayQueueItem;
}

- (void)playerStateChanged:(NSNotification *)aNotification
{
    HeimdallPlayerController *playerController = aNotification.object;
    enum heimdall_player_state playerState = playerController.playerState;
    HeimdallInputItem *inputItem = playerController.currentMedia;

    switch (playerState) {
        case HEIMDALL_PLAYER_STATE_PLAYING:
            [playPauseButton setState:NSControlStateValueOn];
            [self setProgressTimeEnabled:YES];
            [pathActionItem setEnabled:YES];
            [self updateCachedURLOfCurrentMedia:inputItem];
            break;
        case HEIMDALL_PLAYER_STATE_PAUSED:
            [playPauseButton setState:NSControlStateValueOff];
            [self setProgressTimeEnabled:YES];
            [pathActionItem setEnabled:YES];
            [self updateCachedURLOfCurrentMedia:inputItem];
            break;
        case HEIMDALL_PLAYER_STATE_STOPPED:
            [playPauseButton setState:NSControlStateValueOff];
            [self setProgressTimeEnabled:NO];
            [pathActionItem setEnabled:NO];
            _currentPlaybackUrl = nil;
            break;
        default:
            break;
    }
}

/* Updates the Metadata for the currently
 * playing item or resets it if nothing is playing
 */
- (void)inputItemChanged:(NSNotification *)aNotification
{
    NSImage         *coverArtImage;
    NSString        *title;
    NSString        *nowPlaying;
    NSString        *artist;
    NSString        *album;

    HeimdallPlayerController *playerController = aNotification.object;
    HeimdallInputItem *inputItem = playerController.currentMedia;

    [self updateCachedURLOfCurrentMedia:inputItem];

    if (inputItem) {
        coverArtImage = NSImage.HeimdallNoArtImage;
        title = inputItem.title;
        nowPlaying = inputItem.nowPlaying;
        artist = inputItem.artist;
        album = inputItem.album;
    } else {
        /* Nothing playing */
        title = _NS("Heimdall media player");
        artist = _NS("Nothing playing");
    }

    // Set fallback coverart
    if (!coverArtImage) {
        coverArtImage = NSImage.HeimdallNoArtImage;
    }

    // Hack to show now playing for streams (ICY)
    if (nowPlaying && !artist) {
        artist = nowPlaying;
    }

    // Set the metadata in the UI
    [self setMetadataTitle:title artist:artist album:album andCover:coverArtImage];

    if (inputItem) {
        __weak typeof(self) weakSelf = self;
        [HeimdallLibraryImageCache thumbnailForInputItem:inputItem
                                     withCompletion:^(NSImage * const thumbnail) {
            HeimdallStatusBarIcon * const strongSelf = weakSelf;
            if (!strongSelf || inputItem != playerController.currentMedia) {
                return;
            }
            strongSelf->coverImageView.image = thumbnail ?: NSImage.HeimdallNoArtImage;
        }];
    }
}

// Update dynamic copy/open menu item status
- (void)updateDynamicMenuItemText
{
    if (!_currentPlaybackUrl) {
        pathActionItem.hidden = YES;
        return;
    }

    NSURL *itemURI = [NSURL URLWithString:_currentPlaybackUrl];
    pathActionItem.hidden = NO;

    if ([itemURI.scheme isEqualToString:@"file"]) {
        [pathActionItem setTitle:_NS("Reveal in Finder")];
    } else {
        [pathActionItem setTitle:_NS("Copy URL to Clipboard")];
    }
}

// Update the random menu item status
- (void)updateMenuItemRandom
{
    // Get current random status
    [randomButton setState:HeimdallMain.sharedInstance.playQueueController.playbackOrder == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM ? NSOnState : NSOffState];
}

#pragma mark -
#pragma mark Utility functions

/* Update the UI to the specified metadata
 * Any of the values can be nil and will be replaced with empty strings
 * or no cover Image at all
 */
- (void)setMetadataTitle:(NSString *)title
                  artist:(NSString *)artist
                   album:(NSString *)album
                andCover:(NSImage *)cover
{
    [titleField setStringValue:(title) ? title : @""];
    [artistField setStringValue:(artist) ? artist : @""];
    [albumField setStringValue:(album) ? album : @""];
    [coverImageView setImage:cover];
}

// Set the play/pause menu item status
- (void)setStoppedStatus:(BOOL)stopped
{
    if (stopped) {
        [playPauseButton setState:NSOffState];
    } else {
        [playPauseButton setState:NSOnState];
    }
}

- (void)setProgressTimeEnabled:(BOOL)enabled
{
    [progressField setEnabled:enabled];
    [separatorField setEnabled:enabled];
    [totalField setEnabled:enabled];
}

#pragma mark -
#pragma mark Menu item Actions

/* Action: Select the currently playing file in Finder
 *         or in case of a network stream, copy the URL
 */
- (IBAction)copyOrOpenCurrentPlaybackItem:(id)sender
{
    // If nothing playing, there is nothing to do
    if (!_currentPlaybackUrl) {
        return;
    }

    // Check if path or URL
    NSURL *itemURI = [NSURL URLWithString:_currentPlaybackUrl];

    if ([itemURI.scheme isEqualToString:@"file"]) {
        // Local file, open in Finder
        [NSWorkspace.sharedWorkspace selectFile:itemURI.path
                         inFileViewerRootedAtPath:itemURI.path];
    } else {
        // URL, copy to pasteboard
        NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
        [pasteboard setString:_currentPlaybackUrl forType:NSPasteboardTypeString];
    }
}

// Action: Show Heimdall main window
- (IBAction)statusBarIconShowMainWindow:(id)sender
{
    [NSApplication.sharedApplication activateIgnoringOtherApps:YES];
    [(NSWindow *)HeimdallMain.sharedInstance.libraryWindow makeKeyAndOrderFront:sender];
}

// Action: Toggle Play / Pause
- (IBAction)statusBarIconTogglePlayPause:(id)sender
{
    HeimdallPlayQueueController * const playQueueController = HeimdallMain.sharedInstance.playQueueController;
    HeimdallPlayerController * const playerController = playQueueController.playerController;
    enum heimdall_player_state playerState = playerController.playerState;
    if (playerState != HEIMDALL_PLAYER_STATE_PAUSED) {
        [playerController pause];
    } else if (playerState == HEIMDALL_PLAYER_STATE_PAUSED) {
        [playerController resume];
    } else {
        [playQueueController startPlayQueue];
    }
}

// Action: Go to next track
- (IBAction)statusBarIconNext:(id)sender
{
    [HeimdallMain.sharedInstance.playQueueController playNextItem];
}

// Action: Go to previous track
- (IBAction)statusBarIconPrevious:(id)sender
{
    [HeimdallMain.sharedInstance.playQueueController playPreviousItem];
}

// Action: Toggle random playback (shuffle)
- (IBAction)statusBarIconToggleRandom:(id)sender
{
    HeimdallPlayQueueController * const playQueueController = HeimdallMain.sharedInstance.playQueueController;
    playQueueController.playbackOrder = (playQueueController.playbackOrder == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM) ? HEIMDALL_PLAYLIST_PLAYBACK_ORDER_NORMAL : HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM;
}

// Action: Toggle between elapsed and remaining time
- (IBAction)toggelProgressTime:(id)sender
{
    _showTimeElapsed = (!_showTimeElapsed);
}

// Action: Quit Heimdall
- (IBAction)quitAction:(id)sender
{
    libheimdall_Quit(heimdall_object_instance(_intf));
}

- (IBAction)statusBarIconShowMiniAudioPlayer:(id)sender
{
    [HeimdallMain.sharedInstance.detachedAudioWindow makeKeyAndOrderFront:sender];
}

@end
