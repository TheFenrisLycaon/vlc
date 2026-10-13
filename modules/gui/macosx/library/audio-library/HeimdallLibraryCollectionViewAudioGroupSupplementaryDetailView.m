/*****************************************************************************
 * HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailView.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Claudio Cambra <claudio.cambra@gmail.com>
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

#import "HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailView.h"

#import "extensions/NSFont+HeimdallAdditions.h"

#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibraryRepresentedItem.h"

#import "library/audio-library/HeimdallLibraryAudioGroupDataSource.h"
#import "library/audio-library/HeimdallLibraryAudioGroupTableViewDelegate.h"

NSString *const HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailViewIdentifier = @"HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailViewIdentifier";
NSCollectionViewSupplementaryElementKind const HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailViewKind = @"HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailViewIdentifier";

@interface HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailView ()
{
    HeimdallLibraryAudioGroupDataSource *_audioGroupAlbumsDataSource;
    HeimdallLibraryAudioGroupTableViewDelegate *_audioGroupAlbumsTableViewDelegate;
}

@end

@implementation HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailView

- (void)awakeFromNib
{
    _audioGroupAlbumsDataSource = [[HeimdallLibraryAudioGroupDataSource alloc] init];
    _audioGroupAlbumsDataSource.tableViews = @[_audioGroupAlbumsTableView];

    _audioGroupAlbumsTableViewDelegate = [[HeimdallLibraryAudioGroupTableViewDelegate alloc] init];

    _audioGroupAlbumsTableView.dataSource = _audioGroupAlbumsDataSource;
    _audioGroupAlbumsTableView.delegate = _audioGroupAlbumsTableViewDelegate;

    _audioGroupNameTextField.font = NSFont.HeimdallLibrarySubsectionHeaderFont;

    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(handleAudioGroupUpdated:)
                               name:HeimdallLibraryModelAlbumUpdated
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(handleAudioGroupUpdated:)
                               name:HeimdallLibraryModelArtistUpdated
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(handleAudioGroupUpdated:)
                               name:HeimdallLibraryModelGenreUpdated
                             object:nil];
}

- (void)handleAudioGroupUpdated:(NSNotification *)notification
{
    NSParameterAssert(notification);

    if (self.representedItem == nil ||
        notification.object == nil ||
        ![notification.object conformsToProtocol:@protocol(HeimdallMediaLibraryAudioGroupProtocol)]) {

        return;
    }

    const id<HeimdallMediaLibraryAudioGroupProtocol> audioGroup = (id<HeimdallMediaLibraryAudioGroupProtocol>)notification.object;
    HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:audioGroup parentType:self.representedItem.parentType];
    self.representedItem = representedItem;
}

- (void)updateRepresentation
{
    NSAssert(self.representedItem != nil, @"no media item assigned for collection view item", nil);
    const id<HeimdallMediaLibraryAudioGroupProtocol> audioGroup = (id<HeimdallMediaLibraryAudioGroupProtocol>)self.representedItem.item;
    NSAssert(audioGroup != nil, @"audio group should not be nil!");

    _audioGroupNameTextField.stringValue = audioGroup.displayString;
    _audioGroupAlbumsDataSource.representedAudioGroup = audioGroup;
}

@end
