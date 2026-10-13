/*****************************************************************************
 * HeimdallLibrarySegment.m: MacOS X interface module
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

#import "HeimdallLibrarySegment.h"

#import "main/HeimdallMain.h"

#import "extensions/NSImage+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallInputNode.h"
#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibrarySegmentBookmarkedLocation.h"
#import "library/HeimdallLibraryWindow.h"
#import "library/HeimdallLibraryWindowPersistentPreferences.h"
#import "library/HeimdallLibraryWindowToolbarDelegate.h"

#import "library/audio-library/HeimdallLibraryAudioViewController.h"

#import "library/favorites-library/HeimdallLibraryFavoritesViewController.h"

#import "library/groups-library/HeimdallLibraryGroupsViewController.h"

#import "library/home-library/HeimdallLibraryHomeViewController.h"

#import "library/media-source/HeimdallLibraryMediaSourceViewController.h"
#import "library/media-source/HeimdallMediaSource.h"
#import "library/media-source/HeimdallMediaSourceProvider.h"

#import "library/playlist-library/HeimdallLibraryPlaylistViewController.h"

#import "library/search-library/HeimdallLibrarySearchViewController.h"

#import "library/video-library/HeimdallLibraryVideoViewController.h"

#import <heimdall_modules.h>

NSString * const HeimdallLibraryBookmarkedLocationsKey = @"HeimdallLibraryBookmarkedLocations";
NSString * const HeimdallLibraryBookmarkedLocationsChanged = @"HeimdallLibraryBookmarkedLocationsChanged";

static const HeimdallLibraryWindowToolbarDisplayFlags standardLibraryViewToolbarDisplayFlags =
    HeimdallLibraryWindowToolbarDisplayFlagSortOrderButton |
    HeimdallLibraryWindowToolbarDisplayFlagLibrarySearchBar |
    HeimdallLibraryWindowToolbarDisplayFlagToggleViewModeSegmentButton;

static const HeimdallLibraryWindowToolbarDisplayFlags mediaSourceViewToolbarDisplayFlags =
    HeimdallLibraryWindowToolbarDisplayFlagNavigationButtons |
    HeimdallLibraryWindowToolbarDisplayFlagToggleViewModeSegmentButton;

NSArray<NSString *> *defaultBookmarkedLocations()
{
    NSMutableArray<NSString *> * const locationMrls = NSMutableArray.array;
    NSArray<HeimdallMediaSource *> * const localMediaSources =
        HeimdallMediaSourceProvider.listOfLocalMediaSources;

    for (HeimdallMediaSource * const mediaSource in localMediaSources) {
        HeimdallInputNode * const rootNode = mediaSource.rootNode;
        NSError * const error = [mediaSource preparseInputNodeWithinTree:rootNode];
        if (error)
            continue;
        

        if (rootNode.children != nil) {
            for (HeimdallInputNode * const node in rootNode.children) {
                [locationMrls addObject:node.inputItem.MRL];
            }
        }
    }

    return locationMrls.copy;
}


// MARK: - Properties used by public HeimdallLibrarySegment methods internally and set by subclasses

@interface HeimdallLibrarySegment ()

@property NSString *internalDisplayString;
@property (nullable) NSString *internalSearchFieldPlaceholder;
@property NSImage *internalDisplayImage;
@property (nullable) Class internalLibraryViewControllerClass;
@property (nullable) NSArray<NSTreeNode *> *internalChildNodes;
@property (nullable) HeimdallLibraryAbstractSegmentViewController *(^internalLibraryViewControllerCreator)(void);
@property (nullable) void (^internalLibraryViewPresenter)(HeimdallLibraryAbstractSegmentViewController *);
@property (nullable) void (^internalSaveViewModePreference)(NSInteger);
@property (nullable) NSInteger (^internalGetViewModePreference)(void);
@property HeimdallLibraryWindowToolbarDisplayFlags internalToolbarDisplayFlags;
@property BOOL internalMediaLibraryRequired;

- (instancetype)initWithSegmentType:(HeimdallLibrarySegmentType)segmentType;

@end


// MARK: - HeimdallLibraryHeaderSegment

@interface HeimdallLibraryHeaderSegment : HeimdallLibrarySegment

- (instancetype)initWithDisplayString:(NSString *)displayString;

@end

@implementation HeimdallLibraryHeaderSegment

- (instancetype)initWithDisplayString:(NSString *)displayString{
    self = [super initWithSegmentType:HeimdallLibraryHeaderSegmentType];
    if (self) {
        self.internalDisplayString = displayString;
    }
    return self;
}

@end


// MARK: - HeimdallLibraryHomeSegment

@interface HeimdallLibraryHomeSegment : HeimdallLibrarySegment
@end

@implementation HeimdallLibraryHomeSegment

- (instancetype)init
{
    self = [super initWithSegmentType:HeimdallLibraryHomeSegmentType];
    if (self) {
        self.internalDisplayString = _NS("Home");
        self.internalSearchFieldPlaceholder = _NS("Search the library");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage = [NSImage imageWithSystemSymbolName:@"house"
                                                  accessibilityDescription:@"Home icon"];
        } else {
            self.internalDisplayImage = NSImage.HeimdallBWHomeImage;
            self.internalDisplayImage.template = YES;
        }
        self.internalMediaLibraryRequired = YES;
        self.internalLibraryViewControllerClass = HeimdallLibraryHomeViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryHomeViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryHomeViewController *)controller presentHomeView];
        };
    }
    return self;
}

@end

// MARK: - HeimdallLibraryFavoritesSegment

@interface HeimdallLibraryFavoritesSegment : HeimdallLibrarySegment
@end

@implementation HeimdallLibraryFavoritesSegment

- (instancetype)init
{
    self = [super initWithSegmentType:HeimdallLibraryFavoritesSegmentType];
    if (self) {
        self.internalDisplayString = _NS("Favorites");
        self.internalSearchFieldPlaceholder = _NS("Search your favorites");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage = [NSImage imageWithSystemSymbolName:@"heart"
                                                  accessibilityDescription:@"Favorites icon"];
        } else {
            self.internalDisplayImage = NSImage.HeimdallBWHomeImage;
            self.internalDisplayImage.template = YES;
        }
        self.internalMediaLibraryRequired = YES;
        self.internalLibraryViewControllerClass = HeimdallLibraryFavoritesViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryFavoritesViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryFavoritesViewController *)controller presentFavoritesView];
        };
        self.internalSaveViewModePreference = ^(const NSInteger viewMode) {
            HeimdallLibraryWindowPersistentPreferences.sharedInstance.favoritesLibraryViewMode = viewMode;
        };
        self.internalGetViewModePreference = ^{
            return HeimdallLibraryWindowPersistentPreferences.sharedInstance.favoritesLibraryViewMode;
        };
        self.internalToolbarDisplayFlags = standardLibraryViewToolbarDisplayFlags;
    }
    return self;
}

@end

// MARK: - Video library view segments

@interface HeimdallLibraryVideoShowsSubSegment : HeimdallLibrarySegment
@end

@interface HeimdallLibraryVideoMoviesSubSegment : HeimdallLibrarySegment
@end

@implementation HeimdallLibraryVideoMoviesSubSegment

- (instancetype)init
{
    self = [super initWithSegmentType:HeimdallLibraryMoviesVideoSubSegmentType];
    if (self) {
        self.internalDisplayString = _NS("Movies");
        self.internalSearchFieldPlaceholder = _NS("Search your movies");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage = [NSImage imageWithSystemSymbolName:@"film"
                                                  accessibilityDescription:@"Movies icon"];
        } else {
            self.internalDisplayImage = NSImage.HeimdallSidebarMovieImage;
            self.internalDisplayImage.template = YES;
        }
        self.internalMediaLibraryRequired = YES;
        self.internalLibraryViewControllerClass = HeimdallLibraryVideoViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryVideoViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryVideoViewController *)controller presentMoviesView];
        };
        self.internalSaveViewModePreference = ^(const NSInteger viewMode) {
            HeimdallLibraryWindowPersistentPreferences.sharedInstance.moviesLibraryViewMode = viewMode;
        };
        self.internalGetViewModePreference = ^{
            return HeimdallLibraryWindowPersistentPreferences.sharedInstance.moviesLibraryViewMode;
        };
        self.internalToolbarDisplayFlags = standardLibraryViewToolbarDisplayFlags;
    }
    return self;
}

@end

@implementation HeimdallLibraryVideoShowsSubSegment

- (instancetype)init
{
    self = [super initWithSegmentType:HeimdallLibraryShowsVideoSubSegmentType];
    if (self) {
        self.internalDisplayString = _NS("Shows");
        self.internalSearchFieldPlaceholder = _NS("Search your shows");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage = [NSImage imageWithSystemSymbolName:@"tv"
                                                  accessibilityDescription:@"Shows icon"];
        } else {
            self.internalDisplayImage = NSImage.HeimdallSidebarMovieImage;
            self.internalDisplayImage.template = YES;
        }
        self.internalMediaLibraryRequired = YES;
        self.internalLibraryViewControllerClass = HeimdallLibraryVideoViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryVideoViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryVideoViewController *)controller presentShowsView];
        };
        self.internalSaveViewModePreference = ^(const NSInteger viewMode) {
            HeimdallLibraryWindowPersistentPreferences.sharedInstance.showsLibraryViewMode = viewMode;
        };
        self.internalGetViewModePreference = ^{
            return HeimdallLibraryWindowPersistentPreferences.sharedInstance.showsLibraryViewMode;
        };
        self.internalToolbarDisplayFlags = standardLibraryViewToolbarDisplayFlags;
    }
    return self;
}

@end


@interface HeimdallLibraryVideoSegment : HeimdallLibrarySegment
@end

@implementation HeimdallLibraryVideoSegment

- (instancetype)init
{
    self = [super initWithSegmentType:HeimdallLibraryVideoSegmentType];
    if (self) {
        self.internalDisplayString = _NS("Videos");
        self.internalSearchFieldPlaceholder = _NS("Search your videos");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage = [NSImage imageWithSystemSymbolName:@"film.stack"
                                                  accessibilityDescription:@"Video icon"];
        } else {
            self.internalDisplayImage = NSImage.HeimdallSidebarMovieImage;
            self.internalDisplayImage.template = YES;
        }
        self.internalMediaLibraryRequired = YES;
        self.internalLibraryViewControllerClass = HeimdallLibraryVideoViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryVideoViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryVideoViewController *)controller presentVideoView];
        };
        self.internalSaveViewModePreference = ^(const NSInteger viewMode) {
            HeimdallLibraryWindowPersistentPreferences.sharedInstance.videoLibraryViewMode = viewMode;
        };
        self.internalGetViewModePreference = ^{
            return HeimdallLibraryWindowPersistentPreferences.sharedInstance.videoLibraryViewMode;
        };
        self.internalToolbarDisplayFlags = standardLibraryViewToolbarDisplayFlags;
        self.internalChildNodes = @[
            [[HeimdallLibraryVideoShowsSubSegment alloc] init],
            [[HeimdallLibraryVideoMoviesSubSegment alloc] init]
        ];
    }
    return self;
}

@end


// MARK: - Audio library view segments

@interface HeimdallLibraryMusicArtistSubSegment : HeimdallLibrarySegment
@end

@implementation HeimdallLibraryMusicArtistSubSegment

- (instancetype)init
{
    self = [super initWithSegmentType:HeimdallLibraryArtistsMusicSubSegmentType];
    if (self) {
        self.internalDisplayString = _NS("Artists");
        self.internalSearchFieldPlaceholder = _NS("Search your artists");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage = [NSImage imageWithSystemSymbolName:@"music.mic"
                                                  accessibilityDescription:@"Music artists icon"];
        } else {
            self.internalDisplayImage = NSImage.HeimdallSidebarMusicImage;
            self.internalDisplayImage.template = YES;
        }
        self.internalMediaLibraryRequired = YES;
        self.internalLibraryViewControllerClass = HeimdallLibraryAudioViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryAudioViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryAudioViewController *)controller presentAudioView];
        };
        self.internalSaveViewModePreference = ^(const NSInteger viewMode) {
            HeimdallLibraryWindowPersistentPreferences.sharedInstance.artistLibraryViewMode = viewMode;
        };
        self.internalGetViewModePreference = ^{
            return HeimdallLibraryWindowPersistentPreferences.sharedInstance.artistLibraryViewMode;
        };
        self.internalToolbarDisplayFlags = standardLibraryViewToolbarDisplayFlags;
    }
    return self;
}

@end


@interface HeimdallLibraryMusicAlbumSubSegment : HeimdallLibrarySegment
@end

@implementation HeimdallLibraryMusicAlbumSubSegment

- (instancetype)init
{
    self = [super initWithSegmentType:HeimdallLibraryAlbumsMusicSubSegmentType];
    if (self) {
        self.internalDisplayString = _NS("Albums");
        self.internalSearchFieldPlaceholder = _NS("Search your albums");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage = [NSImage imageWithSystemSymbolName:@"square.stack"
                                                  accessibilityDescription:@"Music albums icon"];
        } else {
            self.internalDisplayImage = NSImage.HeimdallSidebarMusicImage;
            self.internalDisplayImage.template = YES;
        }
        self.internalMediaLibraryRequired = YES;
        self.internalLibraryViewControllerClass = HeimdallLibraryAudioViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryAudioViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryAudioViewController *)controller presentAudioView];
        };
        self.internalSaveViewModePreference = ^(const NSInteger viewMode) {
            HeimdallLibraryWindowPersistentPreferences.sharedInstance.albumLibraryViewMode = viewMode;
        };
        self.internalGetViewModePreference = ^{
            return HeimdallLibraryWindowPersistentPreferences.sharedInstance.albumLibraryViewMode;
        };
        self.internalToolbarDisplayFlags = standardLibraryViewToolbarDisplayFlags;
    }
    return self;
}

@end


@interface HeimdallLibraryMusicSongSubSegment : HeimdallLibrarySegment
@end

@implementation HeimdallLibraryMusicSongSubSegment

- (instancetype)init
{
    self = [super initWithSegmentType:HeimdallLibrarySongsMusicSubSegmentType];
    if (self) {
        self.internalDisplayString = _NS("Songs");
        self.internalSearchFieldPlaceholder = _NS("Search your songs");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage = [NSImage imageWithSystemSymbolName:@"music.note"
                                                  accessibilityDescription:@"Music songs icon"];
        } else {
            self.internalDisplayImage = NSImage.HeimdallSidebarMusicImage;
            self.internalDisplayImage.template = YES;
        }
        self.internalMediaLibraryRequired = YES;
        self.internalLibraryViewControllerClass = HeimdallLibraryAudioViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryAudioViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryAudioViewController *)controller presentAudioView];
        };
        self.internalSaveViewModePreference = ^(const NSInteger viewMode) {
            HeimdallLibraryWindowPersistentPreferences.sharedInstance.songsLibraryViewMode = viewMode;
        };
        self.internalGetViewModePreference = ^{
            return HeimdallLibraryWindowPersistentPreferences.sharedInstance.songsLibraryViewMode;
        };
        self.internalToolbarDisplayFlags = standardLibraryViewToolbarDisplayFlags;
    }
    return self;
}

@end


@interface HeimdallLibraryMusicGenreSubSegment : HeimdallLibrarySegment
@end

@implementation HeimdallLibraryMusicGenreSubSegment

- (instancetype)init
{
    self = [super initWithSegmentType:HeimdallLibraryGenresMusicSubSegmentType];
    if (self) {
        self.internalDisplayString = _NS("Genres");
        self.internalSearchFieldPlaceholder = _NS("Search your genres");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage = [NSImage imageWithSystemSymbolName:@"guitars"
                                                  accessibilityDescription:@"Music genres icon"];
        } else {
            self.internalDisplayImage = NSImage.HeimdallSidebarMusicImage;
            self.internalDisplayImage.template = YES;
        }
        self.internalMediaLibraryRequired = YES;
        self.internalLibraryViewControllerClass = HeimdallLibraryAudioViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryAudioViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryAudioViewController *)controller presentAudioView];
        };
        self.internalSaveViewModePreference = ^(const NSInteger viewMode) {
            HeimdallLibraryWindowPersistentPreferences.sharedInstance.genreLibraryViewMode = viewMode;
        };
        self.internalGetViewModePreference = ^{
            return HeimdallLibraryWindowPersistentPreferences.sharedInstance.genreLibraryViewMode;
        };
        self.internalToolbarDisplayFlags = standardLibraryViewToolbarDisplayFlags;
    }
    return self;
}

@end


@interface HeimdallLibraryMusicSegment : HeimdallLibrarySegment
@end

@implementation HeimdallLibraryMusicSegment

- (instancetype)init
{
    self = [super initWithSegmentType:HeimdallLibraryMusicSegmentType];
    if (self) {
        self.internalDisplayString = _NS("Music");
        self.internalSearchFieldPlaceholder = _NS("Search your music");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage = [NSImage imageWithSystemSymbolName:@"music.note"
                                                  accessibilityDescription:@"Music icon"];
        } else {
            self.internalDisplayImage = NSImage.HeimdallSidebarMusicImage;
            self.internalDisplayImage.template = YES;
        }
        self.internalMediaLibraryRequired = YES;
        self.internalLibraryViewControllerClass = HeimdallLibraryAudioViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryAudioViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryAudioViewController *)controller presentAudioView];
        };
        self.internalToolbarDisplayFlags = standardLibraryViewToolbarDisplayFlags;
        self.internalChildNodes = @[
            [[HeimdallLibraryMusicArtistSubSegment alloc] init],
            [[HeimdallLibraryMusicAlbumSubSegment alloc] init],
            [[HeimdallLibraryMusicSongSubSegment alloc] init],
            [[HeimdallLibraryMusicGenreSubSegment alloc] init],
        ];
    }
    return self;
}

@end


// MARK: - Playlist library view segments

@interface HeimdallLibraryPlaylistMusicPlaylistSubSegment : HeimdallLibrarySegment
@end

@implementation HeimdallLibraryPlaylistMusicPlaylistSubSegment

- (instancetype)init
{
    self = [super initWithSegmentType:HeimdallLibraryPlaylistsMusicOnlyPlaylistsSubSegmentType];
    if (self) {
        self.internalDisplayString = _NS("Music playlists");
        self.internalSearchFieldPlaceholder = _NS("Search your music playlists");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage = [NSImage imageWithSystemSymbolName:@"music.note.list"
                                                  accessibilityDescription:@"Music playlists icon"];
        } else {
            self.internalDisplayImage = NSImage.HeimdallSidebarMusicImage;
            self.internalDisplayImage.template = YES;
        }
        self.internalMediaLibraryRequired = YES;
        self.internalLibraryViewControllerClass = HeimdallLibraryPlaylistViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryPlaylistViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryPlaylistViewController *)controller presentPlaylistsViewForPlaylistType:HEIMDALL_ML_PLAYLIST_TYPE_AUDIO_ONLY];
        };
        self.internalSaveViewModePreference = ^(const NSInteger viewMode) {
            HeimdallLibraryWindowPersistentPreferences.sharedInstance.musicOnlyPlaylistLibraryViewMode = viewMode;
        };
        self.internalGetViewModePreference = ^{
            return HeimdallLibraryWindowPersistentPreferences.sharedInstance.musicOnlyPlaylistLibraryViewMode;
        };
        self.internalToolbarDisplayFlags = standardLibraryViewToolbarDisplayFlags;
    }
    return self;
}

@end


@interface HeimdallLibraryPlaylistVideoPlaylistSubSegment : HeimdallLibrarySegment
@end

@implementation HeimdallLibraryPlaylistVideoPlaylistSubSegment

- (instancetype)init
{
    self = [super initWithSegmentType:HeimdallLibraryPlaylistsVideoOnlyPlaylistsSubSegmentType];
    if (self) {
        self.internalDisplayString = _NS("Video playlists");
        self.internalSearchFieldPlaceholder = _NS("Search your video playlists");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage = [NSImage imageWithSystemSymbolName:@"list.and.film"
                                                  accessibilityDescription:@"Video playlists icon"];
        } else {
            self.internalDisplayImage = NSImage.HeimdallSidebarMovieImage;
            self.internalDisplayImage.template = YES;
        }
        self.internalMediaLibraryRequired = YES;
        self.internalLibraryViewControllerClass = HeimdallLibraryPlaylistViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryPlaylistViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryPlaylistViewController *)controller presentPlaylistsViewForPlaylistType:HEIMDALL_ML_PLAYLIST_TYPE_VIDEO_ONLY];
        };
        self.internalSaveViewModePreference = ^(const NSInteger viewMode) {
            HeimdallLibraryWindowPersistentPreferences.sharedInstance.videoOnlyPlaylistLibraryViewMode = viewMode;
        };
        self.internalGetViewModePreference = ^{
            return HeimdallLibraryWindowPersistentPreferences.sharedInstance.videoOnlyPlaylistLibraryViewMode;
        };
        self.internalToolbarDisplayFlags = standardLibraryViewToolbarDisplayFlags;
    }
    return self;
}

@end


@interface HeimdallLibraryPlaylistSegment : HeimdallLibrarySegment
@end

@implementation HeimdallLibraryPlaylistSegment

- (instancetype)init
{
    self = [super initWithSegmentType:HeimdallLibraryPlaylistsSegmentType];
    if (self) {
        self.internalDisplayString = _NS("Playlists");
        self.internalSearchFieldPlaceholder = _NS("Search your playlists");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage = [NSImage imageWithSystemSymbolName:@"list.triangle"
                                                  accessibilityDescription:@"Playlists icon"];
        } else {
            self.internalDisplayImage = [NSImage imageNamed:NSImageNameListViewTemplate];
            self.internalDisplayImage.template = YES;
        }
        self.internalMediaLibraryRequired = YES;
        self.internalLibraryViewControllerClass = HeimdallLibraryPlaylistViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryPlaylistViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryPlaylistViewController *)controller presentPlaylistsViewForPlaylistType:HEIMDALL_ML_PLAYLIST_TYPE_ALL];
        };
        self.internalSaveViewModePreference = ^(const NSInteger viewMode) {
            HeimdallLibraryWindowPersistentPreferences.sharedInstance.playlistLibraryViewMode = viewMode;
        };
        self.internalGetViewModePreference = ^{
            return HeimdallLibraryWindowPersistentPreferences.sharedInstance.playlistLibraryViewMode;
        };
        self.internalToolbarDisplayFlags = standardLibraryViewToolbarDisplayFlags;
        self.internalChildNodes = @[
            [[HeimdallLibraryPlaylistMusicPlaylistSubSegment alloc] init],
            [[HeimdallLibraryPlaylistVideoPlaylistSubSegment alloc] init]
        ];
    }
    return self;
}

@end


// MARK: - Group library view segments

@interface HeimdallLibraryGroupSubSegment : HeimdallLibrarySegment

- (instancetype)initWithGroup:(HeimdallMediaLibraryGroup *)group;

@end

@implementation HeimdallLibraryGroupSubSegment

- (instancetype)initWithGroup:(HeimdallMediaLibraryGroup *)group
{
    self = [super initWithRepresentedObject:group];
    if (self) {
        self.internalDisplayString = group.displayString;
        self.internalSearchFieldPlaceholder = _NS("Search this group");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage = [NSImage imageWithSystemSymbolName:@"play.rectangle"
                                                  accessibilityDescription:@"Group icon"];
        } else {
            self.internalDisplayImage = [NSImage imageNamed:NSImageNameTouchBarTagIconTemplate];
            self.internalDisplayImage.template = YES;
        }
        self.internalMediaLibraryRequired = YES;
        self.internalLibraryViewControllerClass = HeimdallLibraryGroupsViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryGroupsViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryGroupsViewController *)controller presentGroupsView];
        };
        self.internalSaveViewModePreference = ^(const NSInteger viewMode) {
            HeimdallLibraryWindowPersistentPreferences.sharedInstance.groupsLibraryViewMode = viewMode;
        };
        self.internalGetViewModePreference = ^{
            return HeimdallLibraryWindowPersistentPreferences.sharedInstance.groupsLibraryViewMode;
        };
        self.internalToolbarDisplayFlags = standardLibraryViewToolbarDisplayFlags;
    }
    return self;
}

@end


@interface HeimdallLibraryGroupSegment : HeimdallLibrarySegment
@end

@implementation HeimdallLibraryGroupSegment

- (instancetype)init
{
    self = [super initWithSegmentType:HeimdallLibraryGroupsSegmentType];
    if (self) {
        self.internalDisplayString = _NS("Groups");
        self.internalSearchFieldPlaceholder = _NS("Search your groups");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage = [NSImage imageWithSystemSymbolName:@"rectangle.3.group"
                                                  accessibilityDescription:@"Groups icon"];
        } else {
            self.internalDisplayImage = [NSImage imageNamed:NSImageNameTouchBarTagIconTemplate];
            self.internalDisplayImage.template = YES;
        }
        self.internalMediaLibraryRequired = YES;
        self.internalLibraryViewControllerClass = HeimdallLibraryGroupsViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryGroupsViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryGroupsViewController *)controller presentGroupsView];
        };
        self.internalSaveViewModePreference = ^(const NSInteger viewMode) {
            HeimdallLibraryWindowPersistentPreferences.sharedInstance.groupsLibraryViewMode = viewMode;
        };
        self.internalGetViewModePreference = ^{
            return HeimdallLibraryWindowPersistentPreferences.sharedInstance.groupsLibraryViewMode;
        };
        self.internalToolbarDisplayFlags = standardLibraryViewToolbarDisplayFlags;

        HeimdallLibraryModel * const libraryModel =
            HeimdallMain.sharedInstance.libraryController.libraryModel;
        const NSUInteger groupCount = libraryModel.numberOfGroups;

        if (groupCount > 0) {
            NSArray<HeimdallMediaLibraryGroup *> * const groups = libraryModel.listOfGroups;
            NSMutableArray<HeimdallLibrarySegment *> * const groupNodes =
                [NSMutableArray arrayWithCapacity:groupCount];

            for (HeimdallMediaLibraryGroup * const group in groups) {
                [groupNodes addObject:[[HeimdallLibraryGroupSubSegment alloc] initWithGroup:group]];
            }

            self.internalChildNodes = groupNodes.copy;
        }
    }
    return self;
}

@end


// MARK: - Media source-based library view segments

@interface HeimdallLibraryBrowseBookmarkedLocationSubSegment : HeimdallLibrarySegment

- (instancetype)initWithBookmarkedLocation:(HeimdallLibrarySegmentBookmarkedLocation *)descriptor;

@end

@implementation HeimdallLibraryBrowseBookmarkedLocationSubSegment

- (instancetype)initWithBookmarkedLocation:(HeimdallLibrarySegmentBookmarkedLocation *)descriptor
{
    self = [super initWithRepresentedObject:descriptor];
    if (self) {
        self.internalDisplayString = descriptor.name;
        self.internalSearchFieldPlaceholder = _NS("Search this location");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage =
                [NSImage imageWithSystemSymbolName:@"folder"
                          accessibilityDescription:@"Bookmarked location icon"];
        } else {
            self.internalDisplayImage = [NSImage imageNamed:NSImageNameFolder];
            self.internalDisplayImage.template = YES;
        }
        self.internalLibraryViewControllerClass = HeimdallLibraryGroupsViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryMediaSourceViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryMediaSourceViewController *)controller presentBrowseView];
        };
        self.internalSaveViewModePreference = ^(const NSInteger viewMode) {
            HeimdallLibraryWindowPersistentPreferences.sharedInstance.browseLibraryViewMode = viewMode;
        };
        self.internalGetViewModePreference = ^{
            return HeimdallLibraryWindowPersistentPreferences.sharedInstance.browseLibraryViewMode;
        };
        self.internalToolbarDisplayFlags = mediaSourceViewToolbarDisplayFlags;
    }
    return self;
}

@end


@interface HeimdallLibraryBrowseSegment : HeimdallLibrarySegment
@end

@implementation HeimdallLibraryBrowseSegment

- (instancetype)init
{
    self = [super initWithSegmentType:HeimdallLibraryBrowseSegmentType];
    if (self) {
        self.internalDisplayString = _NS("Browse");
        self.internalSearchFieldPlaceholder = _NS("Search this location");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage = [NSImage imageWithSystemSymbolName:@"folder"
                                                  accessibilityDescription:@"Browse icon"];
        } else {
            self.internalDisplayImage = [NSImage imageNamed:NSImageNameFolder];
            self.internalDisplayImage.template = YES;
        }
        self.internalLibraryViewControllerClass = HeimdallLibraryMediaSourceViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryMediaSourceViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryMediaSourceViewController *)controller presentBrowseView];
        };
        self.internalSaveViewModePreference = ^(const NSInteger viewMode) {
            HeimdallLibraryWindowPersistentPreferences.sharedInstance.browseLibraryViewMode = viewMode;
        };
        self.internalGetViewModePreference = ^{
            return HeimdallLibraryWindowPersistentPreferences.sharedInstance.browseLibraryViewMode;
        };
        self.internalToolbarDisplayFlags = mediaSourceViewToolbarDisplayFlags;

        NSUserDefaults * const defaults = NSUserDefaults.standardUserDefaults;
        NSArray<NSString *> *bookmarkedLocations =
            [defaults stringArrayForKey:HeimdallLibraryBookmarkedLocationsKey];
        if (bookmarkedLocations == nil) {
            bookmarkedLocations = defaultBookmarkedLocations();
            [defaults setObject:bookmarkedLocations forKey:HeimdallLibraryBookmarkedLocationsKey];
        }

        const HeimdallLibrarySegmentType segmentType = HeimdallLibraryBrowseBookmarkedLocationSubSegmentType;
        NSMutableArray<NSTreeNode *> * const bookmarkedLocationNodes = NSMutableArray.array;
        NSMutableArray<NSString *> * const remainingBookmarkedLocations = bookmarkedLocations.mutableCopy;

        for (NSString * const locationMrl in bookmarkedLocations) {
            if (![NSFileManager.defaultManager fileExistsAtPath:[NSURL URLWithString:locationMrl].path]) {
                [remainingBookmarkedLocations removeObject:locationMrl];
                continue;
            }
            NSString * const locationName = [locationMrl.lastPathComponent stringByRemovingPercentEncoding];
            HeimdallLibrarySegmentBookmarkedLocation * const descriptor =
                [[HeimdallLibrarySegmentBookmarkedLocation alloc] initWithSegmentType:segmentType
                                                                            name:locationName
                                                                             mrl:locationMrl];
            HeimdallLibraryBrowseBookmarkedLocationSubSegment * const node =
                [[HeimdallLibraryBrowseBookmarkedLocationSubSegment alloc] initWithBookmarkedLocation:descriptor];
            [bookmarkedLocationNodes addObject:node];
        }

        self.internalChildNodes = bookmarkedLocationNodes.copy;

        if (bookmarkedLocations.count != remainingBookmarkedLocations.count) {
            [defaults setObject:remainingBookmarkedLocations forKey:HeimdallLibraryBookmarkedLocationsKey];
        }
    }
    return self;
}

@end


@interface HeimdallLibraryStreamsSegment : HeimdallLibrarySegment
@end

@implementation HeimdallLibraryStreamsSegment

- (instancetype)init
{
    self = [super initWithSegmentType:HeimdallLibraryStreamsSegmentType];
    if (self) {
        self.internalDisplayString = _NS("Streams");
        self.internalSearchFieldPlaceholder = _NS("Search your streams");
        if (@available(macOS 11.0, *)) {
            self.internalDisplayImage =
                [NSImage imageWithSystemSymbolName:@"antenna.radiowaves.left.and.right"
                          accessibilityDescription:@"Streams icon"];
        } else {
            self.internalDisplayImage = [NSImage imageNamed:NSImageNameActionTemplate];
            self.internalDisplayImage.template = YES;
        }
        self.internalLibraryViewControllerClass = HeimdallLibraryMediaSourceViewController.class;
        self.internalLibraryViewControllerCreator = ^{
            return [[HeimdallLibraryMediaSourceViewController alloc] initWithLibraryWindow:HeimdallMain.sharedInstance.libraryWindow];
        };
        self.internalLibraryViewPresenter = ^(HeimdallLibraryAbstractSegmentViewController * const controller) {
            [(HeimdallLibraryMediaSourceViewController *)controller presentStreamsView];
        };
        self.internalSaveViewModePreference = ^(const NSInteger viewMode) {
            HeimdallLibraryWindowPersistentPreferences.sharedInstance.streamLibraryViewMode = viewMode;
        };
        self.internalGetViewModePreference = ^{
            return HeimdallLibraryWindowPersistentPreferences.sharedInstance.streamLibraryViewMode;
        };
        self.internalToolbarDisplayFlags = mediaSourceViewToolbarDisplayFlags;
    }
    return self;
}

@end

// MARK: - HeimdallLibrarySegment

@implementation HeimdallLibrarySegment

+ (NSArray<HeimdallLibrarySegment *> *)librarySegments
{
    NSMutableArray *segments = [NSMutableArray array];

    if (HeimdallMain.sharedInstance.libraryController.shouldUseMediaLibrary) {
        [segments addObjectsFromArray:@[
            [[HeimdallLibraryHomeSegment alloc] init],
            [[HeimdallLibraryHeaderSegment alloc] initWithDisplayString:_NS("Library")],
            [[HeimdallLibraryFavoritesSegment alloc] init],
            [[HeimdallLibraryVideoSegment alloc] init],
            [[HeimdallLibraryMusicSegment alloc] init],
            [[HeimdallLibraryPlaylistSegment alloc] init],
            [[HeimdallLibraryGroupSegment alloc] init]
        ]];
    }

    [segments addObjectsFromArray:@[
        [[HeimdallLibraryHeaderSegment alloc] initWithDisplayString:_NS("Explore")],
        [[HeimdallLibraryBrowseSegment alloc] init],
        [[HeimdallLibraryStreamsSegment alloc] init],
    ]];

    return segments.copy;
}

+ (instancetype)segmentWithSegmentType:(HeimdallLibrarySegmentType)segmentType
{
    switch (segmentType) {
        case HeimdallLibraryHomeSegmentType:
            return [[HeimdallLibraryHomeSegment alloc] init];
        case HeimdallLibraryFavoritesSegmentType:
            return [[HeimdallLibraryFavoritesSegment alloc] init];
        case HeimdallLibraryVideoSegmentType:
            return [[HeimdallLibraryVideoSegment alloc] init];
        case HeimdallLibraryShowsVideoSubSegmentType:
            return [[HeimdallLibraryVideoShowsSubSegment alloc] init];
        case HeimdallLibraryMoviesVideoSubSegmentType:
            return [[HeimdallLibraryVideoMoviesSubSegment alloc] init];
        case HeimdallLibraryMusicSegmentType:
            return [[HeimdallLibraryMusicSegment alloc] init];
        case HeimdallLibraryArtistsMusicSubSegmentType:
            return [[HeimdallLibraryMusicArtistSubSegment alloc] init];
        case HeimdallLibraryAlbumsMusicSubSegmentType:
            return [[HeimdallLibraryMusicAlbumSubSegment alloc] init];
        case HeimdallLibrarySongsMusicSubSegmentType:
            return [[HeimdallLibraryMusicSongSubSegment alloc] init];
        case HeimdallLibraryGenresMusicSubSegmentType:
            return [[HeimdallLibraryMusicGenreSubSegment alloc] init];
        case HeimdallLibraryPlaylistsSegmentType:
            return [[HeimdallLibraryPlaylistSegment alloc] init];
        case HeimdallLibraryPlaylistsMusicOnlyPlaylistsSubSegmentType:
            return [[HeimdallLibraryPlaylistMusicPlaylistSubSegment alloc] init];
        case HeimdallLibraryPlaylistsVideoOnlyPlaylistsSubSegmentType:
            return [[HeimdallLibraryPlaylistVideoPlaylistSubSegment alloc] init];
        case HeimdallLibraryGroupsSegmentType:
            return [[HeimdallLibraryGroupSegment alloc] init];
        case HeimdallLibraryBrowseSegmentType:
            return [[HeimdallLibraryBrowseSegment alloc] init];
        case HeimdallLibraryStreamsSegmentType:
            return [[HeimdallLibraryStreamsSegment alloc] init];
        default:
            return nil;
    }
}

+ (instancetype)segmentForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if ([libraryItem isKindOfClass:HeimdallMediaLibraryAlbum.class]) {
        return [HeimdallLibrarySegment segmentWithSegmentType:HeimdallLibraryAlbumsMusicSubSegmentType];
    } else if ([libraryItem isKindOfClass:HeimdallMediaLibraryArtist.class]) {
        return [HeimdallLibrarySegment segmentWithSegmentType:HeimdallLibraryArtistsMusicSubSegmentType];
    } else if ([libraryItem isKindOfClass:HeimdallMediaLibraryGenre.class]) {
        return [HeimdallLibrarySegment segmentWithSegmentType:HeimdallLibraryGenresMusicSubSegmentType];
    } else if ([libraryItem isKindOfClass:HeimdallMediaLibraryGroup.class]) {
        return [HeimdallLibrarySegment segmentWithSegmentType:HeimdallLibraryGroupsSegmentType];
    }

    HeimdallMediaLibraryMediaItem * const mediaItem = (HeimdallMediaLibraryMediaItem *)libraryItem;
    const BOOL validMediaItem = mediaItem != nil;
    if (validMediaItem && mediaItem.mediaType == HEIMDALL_ML_MEDIA_TYPE_AUDIO) {
        return [HeimdallLibrarySegment segmentWithSegmentType:HeimdallLibraryMusicSegmentType];
    } else if (validMediaItem && mediaItem.mediaType == HEIMDALL_ML_MEDIA_TYPE_VIDEO) {
        if (mediaItem.mediaSubType == HEIMDALL_ML_MEDIA_SUBTYPE_SHOW_EPISODE) {
            return [HeimdallLibrarySegment segmentWithSegmentType:HeimdallLibraryShowsVideoSubSegmentType];
        } else if (mediaItem.mediaSubType == HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE) {
            return [HeimdallLibrarySegment segmentWithSegmentType:HeimdallLibraryMoviesVideoSubSegmentType];
        }
        return [HeimdallLibrarySegment segmentWithSegmentType:HeimdallLibraryVideoSegmentType];
    }

    NSLog(@"Unknown library item type provided, cannot find segment for it: %@", libraryItem.displayString);
    return nil;
}

- (instancetype)initWithSegmentType:(HeimdallLibrarySegmentType)segmentType
{
    return [HeimdallLibrarySegment treeNodeWithRepresentedObject:@(segmentType)];
}

- (instancetype)initWithRepresentedObject:(id)modelObject
{
    NSInteger segmentValue = HeimdallLibraryLowSentinelSegment;

    if ([modelObject isKindOfClass:NSNumber.class]) {
        NSNumber * const segmentNumber = (NSNumber *)modelObject;
        segmentValue = segmentNumber.integerValue;
    } else if ([modelObject isKindOfClass:HeimdallLibrarySegmentBookmarkedLocation.class]) {
        HeimdallLibrarySegmentBookmarkedLocation * const descriptor =
            (HeimdallLibrarySegmentBookmarkedLocation *)modelObject;
        segmentValue = descriptor.segmentType;
    } else if ([modelObject isKindOfClass:HeimdallMediaLibraryGroup.class]) {
        segmentValue = HeimdallLibraryGroupsGroupSubSegmentType;
    }

    NSAssert(segmentValue > HeimdallLibraryLowSentinelSegment &&
             segmentValue < HeimdallLibraryHighSentinelSegment,
             @"HeimdallLibrarySegment represented object must be a library segment type value!");

    self = [super initWithRepresentedObject:modelObject];
    if (self) {
        _segmentType = segmentValue;
    }
    return self;
}

- (NSArray<NSTreeNode *> *)childNodes
{
    return self.internalChildNodes;
}

- (NSInteger)childCount
{
    return [self childNodes].count;
}

- (NSString *)displayString
{
    return self.internalDisplayString;
}

- (nullable NSString *)searchFieldPlaceholder
{
    return self.internalSearchFieldPlaceholder;
}

- (NSImage *)displayImage
{
    return self.internalDisplayImage;
}

- (nullable Class)libraryViewControllerClass
{
    return self.internalLibraryViewControllerClass;
}

- (nullable HeimdallLibraryAbstractSegmentViewController *)newLibraryViewController
{
    return self.internalLibraryViewControllerCreator();
}

- (void)presentLibraryViewUsingController:(HeimdallLibraryAbstractSegmentViewController *)controller
{
    self.internalLibraryViewPresenter(controller);
}

- (NSInteger)viewMode
{
    return self.internalGetViewModePreference();
}

- (void)setViewMode:(NSInteger)viewMode
{
    self.internalSaveViewModePreference(viewMode);
}

- (NSUInteger)toolbarDisplayFlags
{
    return self.internalToolbarDisplayFlags;
}

- (BOOL)mediaLibraryRequired
{
    return self.internalMediaLibraryRequired;
}

@end
