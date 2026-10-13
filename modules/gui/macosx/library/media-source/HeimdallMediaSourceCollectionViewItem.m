/*****************************************************************************
 * HeimdallMediaSourceCollectionViewItem.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne # lycaria -dot- org>
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

#import "HeimdallMediaSourceCollectionViewItem.h"

#import "extensions/NSString+Helpers.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryImageCache.h"
#import "library/HeimdallLibraryMenuController.h"

#import "library/media-source/HeimdallMediaSourceDataSource.h"

#import "main/HeimdallMain.h"

#import "playqueue/HeimdallPlayQueueController.h"

#import "views/HeimdallImageView.h"
#import "views/HeimdallMediaItemCollectionViewItem.h"

NSString *HeimdallMediaSourceCollectionViewItemIdentifier = @"HeimdallMediaSourceCollectionViewItemIdentifier";

@interface HeimdallMediaSourceCollectionViewItem ()
{
    HeimdallLibraryMenuController *_menuController;
}
@end

@implementation HeimdallMediaSourceCollectionViewItem

- (instancetype)initWithNibName:(NSNibName)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:@"HeimdallMediaItemCollectionViewItem" bundle:nibBundleOrNil];
    return self;
}

- (void)prepareForReuse
{
    [super prepareForReuse];
    _representedItem = nil;
}

- (void)setRepresentedItem:(HeimdallInputItem *)representedItem
{
    if (_representedItem == representedItem) {
        return;
    }
    _representedItem = representedItem;
    [self updateRepresentation];
}

- (void)updateRepresentation
{
    HeimdallInputItem * const inputItem = self.representedItem;
    if (inputItem == nil) {
        return;
    }

    self.mediaTitleTextField.stringValue = inputItem.name;

    switch (inputItem.inputType) {
        case ITEM_TYPE_STREAM:
            self.annotationTextField.stringValue = _NS("Stream");
            self.annotationTextField.hidden = NO;
            break;
        case ITEM_TYPE_PLAYLIST:
            self.annotationTextField.stringValue = _NS("Playlist");
            self.annotationTextField.hidden = NO;
            break;
        case ITEM_TYPE_DISC:
            self.annotationTextField.stringValue = _NS("Disk");
            self.annotationTextField.hidden = NO;
            break;
        default:
            self.annotationTextField.hidden = YES;
            break;
    }

    __weak typeof(self) weakSelf = self;
    [HeimdallLibraryImageCache thumbnailForInputItem:inputItem
                                 withCompletion:^(NSImage * const thumbnail) {
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (!strongSelf || strongSelf.representedItem != inputItem) {
            return;
        }
        strongSelf.mediaImageView.image = thumbnail;
    }];

    NSArray<NSString *> *tags = inputItem.finderTags;
    if (tags.count > 0) {
        self.secondaryInfoTextField.stringValue = [tags componentsJoinedByString:@", "];
        self.secondaryInfoTextField.hidden = NO;
    } else {
        self.secondaryInfoTextField.hidden = YES;
    }
}

#pragma mark - actions

- (IBAction)playInstantly:(id)sender
{
    [HeimdallMain.sharedInstance.playQueueController
        addInputItem:self.representedItem.heimdallInputItem
           atPosition:-1
        startPlayback:YES];
}

- (IBAction)addToPlayQueue:(id)sender
{
    [HeimdallMain.sharedInstance.playQueueController
        addInputItem:self.representedItem.heimdallInputItem
           atPosition:-1
        startPlayback:NO];
}

- (void)openContextMenu:(NSEvent *)event
{
    if (!_menuController) {
        _menuController = [[HeimdallLibraryMenuController alloc] init];
    }

    NSCollectionView * const collectionView = self.collectionView;
    HeimdallMediaSourceDataSource * const dataSource =
        (HeimdallMediaSourceDataSource *)collectionView.dataSource;
    if (dataSource == nil) {
        return;
    }
    NSSet<NSIndexPath *> * const indexPaths = collectionView.selectionIndexPaths;
    NSArray<HeimdallInputItem *> * const selectedInputItems =
        [dataSource mediaSourceInputItemsAtIndexPaths:indexPaths];
    const NSInteger representedItemIndex = [selectedInputItems indexOfObjectPassingTest:^BOOL(
        HeimdallInputItem * const inputItem, const NSUInteger __unused idx, BOOL * const __unused stop
    ) {
        return [inputItem.MRL isEqualToString:self.representedItem.MRL];
    }];
    NSArray<HeimdallInputItem *> *items = nil;
    if (representedItemIndex == NSNotFound) {
        items = @[self.representedItem];
    } else {
        items = selectedInputItems;
    }
    _menuController.representedInputItems = items;
    [_menuController popupMenuWithEvent:event forView:self.view];
}

- (void)mouseDown:(NSEvent *)event
{
    if (event.modifierFlags & NSEventModifierFlagControl) {
        [self openContextMenu:event];
        return;
    }
    [super mouseDown:event];
}

@end
