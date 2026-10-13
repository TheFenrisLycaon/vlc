/*****************************************************************************
 * HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2021 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Samuel Bassaly <shkshk90 # gmail -dot- com>
 *          Claudio Cambra <developer@claudiocambra.com>
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

#import "HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView.h"

#import "extensions/NSString+Helpers.h"
#import "extensions/NSFont+HeimdallAdditions.h"
#import "extensions/NSColor+HeimdallAdditions.h"
#import "extensions/NSView+HeimdallAdditions.h"

#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryImageCache.h"
#import "library/HeimdallLibraryItemInternalMediaItemsDataSource.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibraryMenuController.h"
#import "library/HeimdallLibraryRepresentedItem.h"
#import "library/HeimdallLibraryWindow.h"

#import "library/audio-library/HeimdallLibraryAlbumTracksTableViewDelegate.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallImageView.h"

NSString * const HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewIdentifier =
    @"HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewIdentifier";
NSCollectionViewSupplementaryElementKind const HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind =
    @"HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewIdentifier";

@interface HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView ()
{
    HeimdallLibraryItemInternalMediaItemsDataSource *_tracksDataSource;
    HeimdallLibraryAlbumTracksTableViewDelegate *_tracksTableViewDelegate;
    HeimdallLibraryController *_libraryController;
}

@end

@implementation HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView

- (void)awakeFromNib
{
    _tracksDataSource = [[HeimdallLibraryItemInternalMediaItemsDataSource alloc] init];
    _tracksTableViewDelegate = [[HeimdallLibraryAlbumTracksTableViewDelegate alloc] init];

    self.tableView.dataSource = _tracksDataSource;
    self.tableView.delegate = _tracksTableViewDelegate;
    self.tableView.rowHeight = HeimdallLibraryInternalMediaItemRowHeight;

    self.titleTextField.font = NSFont.HeimdallLibrarySubsectionHeaderFont;
    self.primaryDetailTextButton.font = NSFont.HeimdallLibrarySubsectionSubheaderFont;
    self.secondaryDetailTextButton.font = NSFont.HeimdallLibrarySubsectionSubheaderFont;

    self.primaryDetailTextButton.action = @selector(primaryDetailAction:);
    self.secondaryDetailTextButton.action = @selector(secondaryDetailAction:);

    if (@available(macOS 10.14, *)) {
        self.primaryDetailTextButton.contentTintColor = NSColor.HeimdallAccentColor;
        self.secondaryDetailTextButton.contentTintColor = NSColor.secondaryLabelColor;
    }

    self.playButton.bezelColor = NSColor.HeimdallAccentColor;

    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(handleItemUpdated:)
                               name:HeimdallLibraryModelAlbumUpdated
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(handleItemUpdated:)
                               name:HeimdallLibraryModelArtistUpdated
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(handleItemUpdated:)
                               name:HeimdallLibraryModelGenreUpdated
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(handleItemUpdated:)
                               name:HeimdallLibraryModelPlaylistUpdated
                             object:nil];
}

- (void)handleItemUpdated:(NSNotification *)notification
{
    NSParameterAssert(notification);
    const id<HeimdallMediaLibraryItemProtocol> item = notification.object;
    if (self.representedItem == nil ||
        item == nil ||
        ![self.representedItem.item.class isKindOfClass:item.class] ||
        self.representedItem.item.libraryID != item.libraryID) {
        return;
    }

    HeimdallLibraryRepresentedItem * const representedItem = 
        [[HeimdallLibraryRepresentedItem alloc] initWithItem:item
                                             parentType:self.representedItem.parentType];
    self.representedItem = representedItem;
}

- (void)updateRepresentation
{
    NSAssert(self.representedItem != nil, @"no media item assigned for collection view item", nil);
    const id<HeimdallMediaLibraryItemProtocol> item = self.representedItem.item;

    self.titleTextField.stringValue = item.displayString;
    self.primaryDetailTextButton.title = item.primaryDetailString;
    self.secondaryDetailTextButton.title = item.secondaryDetailString;

    NSUInteger itemCount = item.mediaItems.count;
    NSMutableArray * const components = [NSMutableArray arrayWithCapacity:2];

    if ([item isKindOfClass:HeimdallMediaLibraryAlbum.class]) {
        const int year = [(HeimdallMediaLibraryAlbum *)item year];
        if (year != 0) {
            [components addObject:@(year)];
        }
    }
    if (itemCount > 1) {
        [components addObject:item.durationString];
    }
    self.yearAndDurationTextField.stringValue = [components componentsJoinedByString:@" · "];

    const BOOL primaryActionableDetail = item.primaryActionableDetail;
    const BOOL secondaryActionableDetail = item.secondaryActionableDetail;
    self.primaryDetailTextButton.enabled = primaryActionableDetail;
    self.secondaryDetailTextButton.enabled = secondaryActionableDetail;
    if (@available(macOS 10.14, *)) {
        self.primaryDetailTextButton.contentTintColor =
            primaryActionableDetail ? NSColor.HeimdallAccentColor : NSColor.secondaryLabelColor;
        self.secondaryDetailTextButton.contentTintColor =
            secondaryActionableDetail ? NSColor.secondaryLabelColor : NSColor.tertiaryLabelColor;
    }

    __weak typeof(self) weakSelf = self; // Prevent retain cycle
    [HeimdallLibraryImageCache thumbnailForLibraryItem:item withCompletion:^(NSImage * const thumbnail) {
        if (!weakSelf || weakSelf.representedItem.item != item) {
            return;
        }
        weakSelf.artworkImageView.image = thumbnail;
    }];

    [_tracksDataSource setRepresentedItem:item withCompletion:^{
        if (weakSelf) {
            [weakSelf.tableView reloadData];
        }
    }];
}

- (IBAction)playAction:(id)sender
{
    [self.representedItem play];
}

- (IBAction)enqueueAction:(id)sender
{
    [self.representedItem queue];
}

- (IBAction)primaryDetailAction:(id)sender
{
    const id<HeimdallMediaLibraryItemProtocol> item = self.representedItem.item;
    if (item == nil || !item.primaryActionableDetail) {
        return;
    }

    HeimdallLibraryWindow * const libraryWindow = HeimdallMain.sharedInstance.libraryWindow;
    const id<HeimdallMediaLibraryItemProtocol> libraryItem = item.primaryActionableDetailLibraryItem;
    [libraryWindow presentLibraryItem:libraryItem];
}

- (IBAction)secondaryDetailAction:(id)sender
{
    const id<HeimdallMediaLibraryItemProtocol> item = self.representedItem.item;
    if (item == nil || !item.secondaryActionableDetail) {
        return;
    }

    HeimdallLibraryWindow * const libraryWindow = HeimdallMain.sharedInstance.libraryWindow;
    const id<HeimdallMediaLibraryItemProtocol> libraryItem = item.secondaryActionableDetailLibraryItem;
    [libraryWindow presentLibraryItem:libraryItem];
}

@end
