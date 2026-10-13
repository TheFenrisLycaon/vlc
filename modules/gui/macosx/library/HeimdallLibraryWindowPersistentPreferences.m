/*****************************************************************************
 * HeimdallLibraryWindowPersistentPreferences.m: MacOS X interface module
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

#import "HeimdallLibraryWindowPersistentPreferences.h"

NSString * const HeimdallLibraryWindowPreferencePrefix = @"HeimdallLibraryWindow";

NSString * const HeimdallLibraryHomeLibraryViewModePreferenceKey = @"HomeLibraryViewMode";
NSString * const HeimdallLibraryFavoritesLibraryViewModePreferenceKey = @"FavoritesLibraryViewMode";
NSString * const HeimdallLibraryVideoLibraryViewModePreferenceKey = @"VideoLibraryViewMode";
NSString * const HeimdallLibraryShowsLibraryViewModePreferenceKey = @"ShowsLibraryViewMode";
NSString * const HeimdallLibraryMoviesLibraryViewModePreferenceKey = @"MoviesLibraryViewMode";
NSString * const HeimdallLibraryAlbumLibraryViewModePreferenceKey = @"AlbumLibraryViewMode";
NSString * const HeimdallLibraryGenreLibraryViewModePreferenceKey = @"GenreLibraryViewMode";
NSString * const HeimdallLibrarySongsLibraryViewModePreferenceKey = @"SongsLibraryViewMode";
NSString * const HeimdallLibraryArtistLibraryViewModePreferenceKey = @"ArtistLibraryViewMode";
NSString * const HeimdallLibraryPlaylistLibraryViewModePreferenceKey = @"PlaylistLibraryViewMode";
NSString * const HeimdallLibraryMusicOnlyPlaylistLibraryViewModePreferenceKey = 
    @"MusicOnlyPlaylistLibraryViewMode";
NSString * const HeimdallLibraryVideoOnlyPlaylistLibraryViewModePreferenceKey = 
    @"VideoOnlyPlaylistLibraryViewMode";
NSString * const HeimdallLibraryBrowseLibraryViewModePreferenceKey = @"BrowseLibraryViewMode";
NSString * const HeimdallLibraryStreamLibraryViewModePreferenceKey = @"StreamLibraryViewMode";
NSString * const HeimdallLibraryGroupsLibraryViewModePreferenceKey = @"GroupsLibraryViewMode";
NSString * const HeimdallLibrarySearchLibraryViewModePreferenceKey = @"SearchLibraryViewMode";

@implementation HeimdallLibraryWindowPersistentPreferences

static HeimdallLibraryWindowPersistentPreferences *sharedInstance = nil;

+ (HeimdallLibraryWindowPersistentPreferences *)sharedInstance
{
    static dispatch_once_t pred;
    dispatch_once(&pred, ^{
        sharedInstance = [[HeimdallLibraryWindowPersistentPreferences alloc] init];
    });

    return sharedInstance;
}

- (NSString * const)fullLibraryWindowKey:(NSString *)partialKey
{
    return [NSString stringWithFormat:@"%@.%@", HeimdallLibraryWindowPreferencePrefix, partialKey];
}

- (const NSInteger)libraryViewModePreferenceWithKey:(NSString *)key
{
    NSUserDefaults * const standardUserDefaults = NSUserDefaults.standardUserDefaults;
    NSString * const fullKey = [self fullLibraryWindowKey:key];
    NSInteger viewModeValue = [standardUserDefaults integerForKey:fullKey];

    if (viewModeValue <= HeimdallLibrarySmallestSentinelViewModeSegment ||
        viewModeValue >= HeimdallLibraryLargestSentinelViewModeSegment) {

        NSLog(@"WARNING: Retrieved invalid values for library view mode. Resorting to default.");
        viewModeValue = HeimdallLibraryGridViewModeSegment; // Set to workable default
    }

    return viewModeValue;
}

- (void)setLibraryWindowViewModePreferenceWithKey:(NSString *)key
                                            value:(HeimdallLibraryViewModeSegment)viewMode
{
    NSParameterAssert(viewMode > HeimdallLibrarySmallestSentinelViewModeSegment &&
                      viewMode < HeimdallLibraryLargestSentinelViewModeSegment);

    NSUserDefaults * const standardUserDefaults = NSUserDefaults.standardUserDefaults;
    NSString * const fullKey = [self fullLibraryWindowKey:key];
    [standardUserDefaults setInteger:viewMode forKey:fullKey];
}

- (HeimdallLibraryViewModeSegment)homeLibraryViewMode
{
    return [self libraryViewModePreferenceWithKey:HeimdallLibraryHomeLibraryViewModePreferenceKey];
}

- (void)setHomeLibraryViewMode:(HeimdallLibraryViewModeSegment)homeLibraryViewMode
{
    [self setLibraryWindowViewModePreferenceWithKey:HeimdallLibraryHomeLibraryViewModePreferenceKey
                                              value:homeLibraryViewMode];
}

- (HeimdallLibraryViewModeSegment)favoritesLibraryViewMode
{
    return [self libraryViewModePreferenceWithKey:HeimdallLibraryFavoritesLibraryViewModePreferenceKey];
}

- (void)setFavoritesLibraryViewMode:(HeimdallLibraryViewModeSegment)favoritesLibraryViewMode
{
    [self setLibraryWindowViewModePreferenceWithKey:HeimdallLibraryFavoritesLibraryViewModePreferenceKey
                                              value:favoritesLibraryViewMode];
}

- (HeimdallLibraryViewModeSegment)videoLibraryViewMode
{
    return [self libraryViewModePreferenceWithKey:HeimdallLibraryVideoLibraryViewModePreferenceKey];
}

- (void)setVideoLibraryViewMode:(HeimdallLibraryViewModeSegment)videoLibraryViewMode
{
    [self setLibraryWindowViewModePreferenceWithKey:HeimdallLibraryVideoLibraryViewModePreferenceKey
                                              value:videoLibraryViewMode];
}

- (HeimdallLibraryViewModeSegment)showsLibraryViewMode
{
    return [self libraryViewModePreferenceWithKey:HeimdallLibraryShowsLibraryViewModePreferenceKey];
}

- (void)setShowsLibraryViewMode:(HeimdallLibraryViewModeSegment)showsLibraryViewMode
{
    [self setLibraryWindowViewModePreferenceWithKey:HeimdallLibraryShowsLibraryViewModePreferenceKey
                                              value:showsLibraryViewMode];
}

- (HeimdallLibraryViewModeSegment)moviesLibraryViewMode
{
    return [self libraryViewModePreferenceWithKey:HeimdallLibraryMoviesLibraryViewModePreferenceKey];
}

- (void)setMoviesLibraryViewMode:(HeimdallLibraryViewModeSegment)moviesLibraryViewMode
{
    [self setLibraryWindowViewModePreferenceWithKey:HeimdallLibraryMoviesLibraryViewModePreferenceKey
                                              value:moviesLibraryViewMode];
}

- (HeimdallLibraryViewModeSegment)albumLibraryViewMode
{
    return [self libraryViewModePreferenceWithKey:HeimdallLibraryAlbumLibraryViewModePreferenceKey];
}

- (void)setAlbumLibraryViewMode:(HeimdallLibraryViewModeSegment)albumLibraryViewMode
{
    [self setLibraryWindowViewModePreferenceWithKey:HeimdallLibraryAlbumLibraryViewModePreferenceKey
                                              value:albumLibraryViewMode];

}

- (HeimdallLibraryViewModeSegment)genreLibraryViewMode
{
    return [self libraryViewModePreferenceWithKey:HeimdallLibraryGenreLibraryViewModePreferenceKey];
}

- (void)setGenreLibraryViewMode:(HeimdallLibraryViewModeSegment)genreLibraryViewMode
{
    [self setLibraryWindowViewModePreferenceWithKey:HeimdallLibraryGenreLibraryViewModePreferenceKey
                                              value:genreLibraryViewMode];
}

- (HeimdallLibraryViewModeSegment)songsLibraryViewMode
{
    return [self libraryViewModePreferenceWithKey:HeimdallLibrarySongsLibraryViewModePreferenceKey];
}

- (void)setSongsLibraryViewMode:(HeimdallLibraryViewModeSegment)songsLibraryViewMode
{
    [self setLibraryWindowViewModePreferenceWithKey:HeimdallLibrarySongsLibraryViewModePreferenceKey
                                              value:songsLibraryViewMode];
}

- (HeimdallLibraryViewModeSegment)artistLibraryViewMode
{
    return [self libraryViewModePreferenceWithKey:HeimdallLibraryArtistLibraryViewModePreferenceKey];
}

- (void)setArtistLibraryViewMode:(HeimdallLibraryViewModeSegment)artistLibraryViewMode
{
    [self setLibraryWindowViewModePreferenceWithKey:HeimdallLibraryArtistLibraryViewModePreferenceKey
                                              value:artistLibraryViewMode];
}

- (HeimdallLibraryViewModeSegment)playlistLibraryViewMode
{
    return [self libraryViewModePreferenceWithKey:HeimdallLibraryPlaylistLibraryViewModePreferenceKey];
}

- (void)setPlaylistLibraryViewMode:(HeimdallLibraryViewModeSegment)playlistLibraryViewMode
{
    [self setLibraryWindowViewModePreferenceWithKey:HeimdallLibraryPlaylistLibraryViewModePreferenceKey
                                              value:playlistLibraryViewMode];
}

- (HeimdallLibraryViewModeSegment)musicOnlyPlaylistLibraryViewMode
{
    return [self libraryViewModePreferenceWithKey:HeimdallLibraryMusicOnlyPlaylistLibraryViewModePreferenceKey];
}

- (void)setMusicOnlyPlaylistLibraryViewMode:(HeimdallLibraryViewModeSegment)musicOnlyPlaylistLibraryViewMode
{
    [self setLibraryWindowViewModePreferenceWithKey:HeimdallLibraryMusicOnlyPlaylistLibraryViewModePreferenceKey
                                              value:musicOnlyPlaylistLibraryViewMode];
}

- (HeimdallLibraryViewModeSegment)videoOnlyPlaylistLibraryViewMode
{
    return [self libraryViewModePreferenceWithKey:HeimdallLibraryVideoOnlyPlaylistLibraryViewModePreferenceKey];
}

- (void)setVideoOnlyPlaylistLibraryViewMode:(HeimdallLibraryViewModeSegment)videoOnlyPlaylistLibraryViewMode
{
    [self setLibraryWindowViewModePreferenceWithKey:HeimdallLibraryVideoOnlyPlaylistLibraryViewModePreferenceKey
                                              value:videoOnlyPlaylistLibraryViewMode];
}

- (HeimdallLibraryViewModeSegment)browseLibraryViewMode
{
    return [self libraryViewModePreferenceWithKey:HeimdallLibraryBrowseLibraryViewModePreferenceKey];
}

- (void)setBrowseLibraryViewMode:(HeimdallLibraryViewModeSegment)browseLibraryViewMode
{
    [self setLibraryWindowViewModePreferenceWithKey:HeimdallLibraryBrowseLibraryViewModePreferenceKey
                                              value:browseLibraryViewMode];
}

- (HeimdallLibraryViewModeSegment)streamLibraryViewMode
{
    return [self libraryViewModePreferenceWithKey:HeimdallLibraryStreamLibraryViewModePreferenceKey];
}

- (void)setStreamLibraryViewMode:(HeimdallLibraryViewModeSegment)streamLibraryViewMode
{
    [self setLibraryWindowViewModePreferenceWithKey:HeimdallLibraryStreamLibraryViewModePreferenceKey
                                              value:streamLibraryViewMode];
}

- (HeimdallLibraryViewModeSegment)groupsLibraryViewMode
{
    return [self libraryViewModePreferenceWithKey:HeimdallLibraryGroupsLibraryViewModePreferenceKey];
}

- (void)setGroupsLibraryViewMode:(HeimdallLibraryViewModeSegment)groupsLibraryViewMode
{
    [self setLibraryWindowViewModePreferenceWithKey:HeimdallLibraryGroupsLibraryViewModePreferenceKey
                                              value:groupsLibraryViewMode];
}

- (HeimdallLibraryViewModeSegment)searchLibraryViewMode
{
    return [self libraryViewModePreferenceWithKey:HeimdallLibrarySearchLibraryViewModePreferenceKey];
}

- (void)setSearchLibraryViewMode:(HeimdallLibraryViewModeSegment)searchLibraryViewMode
{
    [self setLibraryWindowViewModePreferenceWithKey:HeimdallLibrarySearchLibraryViewModePreferenceKey
                                              value:searchLibraryViewMode];
}

@end
