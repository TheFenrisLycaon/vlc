/*****************************************************************************
 * HeimdallLibraryAudioTableViewDelegate.m: MacOS X interface module
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

#import "HeimdallLibraryAudioTableViewDelegate.h"

#import "HeimdallLibraryAlbumTableCellView.h"
#import "HeimdallLibraryAudioDataSource.h"
#import "HeimdallLibrarySongTableCellView.h"
#import "HeimdallLibrarySongsTableViewSongPlayingTableCellView.h"

#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryNameCache.h"
#import "library/HeimdallLibraryTableCellView.h"

@implementation HeimdallLibraryAudioTableViewDelegate

- (instancetype)init
{
    self = [super init];
    if (self) {
        self.cellViewIdentifier = HeimdallAudioLibraryCellIdentifier;
    }
    return self;
}

- (NSView *)tableView:(NSTableView *)tableView
   viewForTableColumn:(NSTableColumn *)tableColumn
                  row:(NSInteger)row
{
    if (![tableView.dataSource conformsToProtocol:@protocol(HeimdallLibraryTableViewDataSource)]) {
        return nil;
    }

    NSObject<HeimdallLibraryTableViewDataSource> * const heimdallDataSource = (NSObject<HeimdallLibraryTableViewDataSource>*)tableView.dataSource;
    NSAssert(heimdallDataSource != nil, @"Should be a valid data source");

    const id<HeimdallMediaLibraryItemProtocol> libraryItem = [heimdallDataSource libraryItemAtRow:row forTableView:tableView];

    // The table view for songs in the list view mode of the audio library is different from the other audio groupings
    // and we use a vanilla NSTableView created in the HeimdallLibraryWindow XIB for it
    if ([tableView.identifier isEqualToString:@"HeimdallLibrarySongsTableViewIdentifier"]) {
        const NSString * const columnIdentifier = tableColumn.identifier;

        if (![libraryItem isKindOfClass:[HeimdallMediaLibraryMediaItem class]]) {
            return nil;
        }

        const HeimdallMediaLibraryMediaItem * const mediaItem = (HeimdallMediaLibraryMediaItem *)libraryItem;

        if (!mediaItem) {
            return nil;
        }

        HeimdallLibraryNameCache * const cache = HeimdallLibraryNameCache.sharedInstance;
        NSString * const albumArtist = [cache albumArtistForID:mediaItem.albumID];
        NSString * const albumTitle = [cache albumTitleForID:mediaItem.albumID];
        NSString * const genreName = [cache genreNameForID:mediaItem.genreID];

        NSString *cellText = @"";
        NSString *cellIdentifier = @"";

        if ([columnIdentifier isEqualToString:HeimdallLibrarySongsTableViewSongPlayingColumnIdentifier]) {
            HeimdallLibrarySongsTableViewSongPlayingTableCellView *cellView = (HeimdallLibrarySongsTableViewSongPlayingTableCellView*)[tableView makeViewWithIdentifier:@"HeimdallLibrarySongsTableViewSongPlayingTableCellViewIdentifier" owner:self];
            NSAssert(cellView, @"Unexpectedly received null cellview");
            cellView.representedMediaItem = (HeimdallMediaLibraryMediaItem *)mediaItem;
            return cellView;
        } else if ([columnIdentifier isEqualToString:HeimdallLibrarySongsTableViewTitleColumnIdentifier]) {
            cellIdentifier = @"HeimdallLibrarySongsTableViewTitleTableCellViewIdentifier";
            cellText = mediaItem.title;
        } else if ([columnIdentifier isEqualToString:HeimdallLibrarySongsTableViewDurationColumnIdentifier]) {
            cellIdentifier = @"HeimdallLibrarySongsTableViewDurationTableCellViewIdentifier";
            cellText = mediaItem.durationString;
        } else if ([columnIdentifier isEqualToString:HeimdallLibrarySongsTableViewArtistColumnIdentifier]) {
            cellIdentifier = @"HeimdallLibrarySongsTableViewArtistTableCellViewIdentifier";
            cellText = albumArtist.length == 0 ? @"" : albumArtist;
        } else if ([columnIdentifier isEqualToString:HeimdallLibrarySongsTableViewAlbumColumnIdentifier]) {
            cellIdentifier = @"HeimdallLibrarySongsTableViewAlbumTableCellViewIdentifier";
            cellText = albumTitle.length == 0 ? @"" : albumTitle;
        } else if ([columnIdentifier isEqualToString:HeimdallLibrarySongsTableViewGenreColumnIdentifier]) {
            cellIdentifier = @"HeimdallLibrarySongsTableViewGenreTableCellViewIdentifier";
            cellText = genreName.length == 0 ? @"" : genreName;
        } else if ([columnIdentifier isEqualToString:HeimdallLibrarySongsTableViewPlayCountColumnIdentifier]) {
            cellIdentifier = @"HeimdallLibrarySongsTableViewPlayCountTableCellViewIdentifier";
            cellText = [@(mediaItem.playCount) stringValue];
        } else if ([columnIdentifier isEqualToString:HeimdallLibrarySongsTableViewYearColumnIdentifier]) {
            cellIdentifier = @"HeimdallLibrarySongsTableViewYearTableCellViewIdentifier";
            if (mediaItem.year == 0) {
                cellText = @"";
            } else {
                NSDate * const yearDate = [NSDate dateWithTimeIntervalSince1970:mediaItem.year];
                NSDateComponents * const components =
                    [NSCalendar.currentCalendar components:NSCalendarUnitYear fromDate:yearDate];
                cellText = @(components.year).stringValue;
            }
        } else {
            NSAssert(true, @"Received unknown column identifier %@", columnIdentifier);
        }

        NSTableCellView *cellView = [tableView makeViewWithIdentifier:cellIdentifier owner:self];
        cellView.textField.stringValue = cellText;
        return cellView;
    }

    return [super tableView:tableView viewForTableColumn:tableColumn row:row];
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
    NSParameterAssert(notification);
    NSTableView * const tableView = (NSTableView *)notification.object;
    NSAssert(tableView, @"Must be a valid table view");
    NSIndexSet * const selectedIndices = tableView.selectedRowIndexes;

    if (![tableView.dataSource conformsToProtocol:@protocol(HeimdallLibraryTableViewDataSource)]) {
        return;
    }

    NSObject<HeimdallLibraryTableViewDataSource> * const heimdallDataSource = (NSObject<HeimdallLibraryTableViewDataSource>*)tableView.dataSource;
    NSAssert(heimdallDataSource != nil, @"Should be a valid data source");

    if ([heimdallDataSource isKindOfClass:[HeimdallLibraryAudioDataSource class]]) {
        [(HeimdallLibraryAudioDataSource*)heimdallDataSource tableView:tableView 
                                            selectRowIndices:selectedIndices];
    }
}

@end
