/*****************************************************************************
 * HeimdallLibraryHomeViewAudioCarouselContainerView.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
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

#import "HeimdallLibraryHomeViewAudioCarouselContainerView.h"

#import "extensions/NSString+Helpers.h"

#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryModel.h"

#import "library/audio-library/HeimdallLibraryAudioDataSource.h"

#import "main/HeimdallMain.h"

@implementation HeimdallLibraryHomeViewAudioCarouselContainerView

@synthesize dataSource = _dataSource;

- (void)setup
{
    [super setup];
    [self setupDataSource];
}

- (void)setupDataSource
{
    HeimdallLibraryAudioDataSource * const audioDataSource = [[HeimdallLibraryAudioDataSource alloc] init];
    self.carouselView.dataSource = audioDataSource;
    audioDataSource.libraryModel = HeimdallMain.sharedInstance.libraryController.libraryModel;
    audioDataSource.carouselView = self.carouselView;
    [audioDataSource setup];
    _dataSource = audioDataSource;
}

- (HeimdallAudioLibrarySegment)audioLibrarySegment
{
    return ((HeimdallLibraryAudioDataSource *)self.dataSource).audioLibrarySegment;
}

- (void)setAudioLibrarySegment:(HeimdallAudioLibrarySegment)audioLibrarySegment
{
    if (audioLibrarySegment == self.audioLibrarySegment) {
        return;
    }

    switch (audioLibrarySegment) {
        case HeimdallAudioLibraryRecentsSegment:
            self.titleView.stringValue = _NS("Recent music");
            break;
        case HeimdallAudioLibraryArtistsSegment:
            self.titleView.stringValue = _NS("Artists");
            break;
        case HeimdallAudioLibraryAlbumsSegment:
            self.titleView.stringValue = _NS("Albums");
            break;
        case HeimdallAudioLibrarySongsSegment:
            self.titleView.stringValue = _NS("Songs");
            break;
        case HeimdallAudioLibraryGenresSegment:
            self.titleView.stringValue = _NS("Genres");
            break;
        case HeimdallAudioLibraryUnknownSegment:
        default:
            self.titleView.stringValue = _NS("Unknown");
            break;
    }

    ((HeimdallLibraryAudioDataSource *)self.dataSource).audioLibrarySegment = audioLibrarySegment;
    [self.carouselView reloadData];
}

@end
