/*****************************************************************************
 * HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.m: MacOS X interface module
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

#import "HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.h"

#import "extensions/NSString+Helpers.h"
#import "extensions/NSFont+HeimdallAdditions.h"
#import "extensions/NSColor+HeimdallAdditions.h"
#import "extensions/NSView+HeimdallAdditions.h"

#import "main/HeimdallMain.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryImageCache.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibraryMenuController.h"
#import "library/HeimdallLibraryRepresentedItem.h"
#import "library/HeimdallLibraryWindow.h"

#import "views/HeimdallImageView.h"
#import "views/HeimdallLibraryRatingIndicator.h"

NSString *const HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewIdentifier = @"HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewIdentifier";
NSCollectionViewSupplementaryElementKind const HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind = @"HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewIdentifier";

@implementation HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView

+ (NSDateFormatter *)lastPlayedDateFormatter
{
    static NSDateFormatter *formatter;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        formatter = [[NSDateFormatter alloc] init];
        formatter.dateStyle = NSDateFormatterFullStyle;
        formatter.timeStyle = NSDateFormatterFullStyle;
    });
    return formatter;
}

- (void)awakeFromNib
{
    _mediaItemTitleTextField.font = NSFont.HeimdallLibrarySubsectionHeaderFont;
    _mediaItemPrimaryDetailButton.font = NSFont.HeimdallLibrarySubsectionSubheaderFont;
    _mediaItemSecondaryDetailButton.font = NSFont.HeimdallLibrarySubsectionSubheaderFont;

    if (@available(macOS 10.14, *)) {
        _mediaItemPrimaryDetailButton.contentTintColor = NSColor.HeimdallAccentColor;
        _mediaItemSecondaryDetailButton.contentTintColor = NSColor.secondaryLabelColor;
    }

    _playMediaItemButton.bezelColor = NSColor.HeimdallAccentColor;
}

- (NSString *)formattedYearAndDurationAndTypeString
{
    if (self.representedItem == nil) {
        return @"";
    }

    HeimdallMediaLibraryMediaItem * const actualItem = self.representedItem.item.firstMediaItem;
    NSMutableArray<NSString *> * const strings = NSMutableArray.array;

    if (actualItem.year > 0) {
        NSDate *yearDate = [NSDate dateWithTimeIntervalSince1970:actualItem.year];
        NSCalendar *calendar = [NSCalendar currentCalendar];
        [strings addObject:[NSString stringWithFormat:@"%ld", [calendar component:NSCalendarUnitYear fromDate:yearDate]]];
    } else if (actualItem.files.count > 0) {
        HeimdallMediaLibraryFile * const firstFile = actualItem.files.firstObject;
        const time_t fileLastModTime = firstFile.lastModificationDate;

        if (fileLastModTime > 0) {
            NSDate * const lastModDate = [NSDate dateWithTimeIntervalSince1970:fileLastModTime];
            NSDateComponents * const components =
                [[NSCalendar currentCalendar] components:NSCalendarUnitYear fromDate:lastModDate];
            [strings addObject:[NSString stringWithFormat:@"%ld", components.year]];
        }
    }

    if (![actualItem.primaryDetailString isEqualToString:actualItem.durationString]) {
        [strings addObject:actualItem.durationString];
    }

    [strings addObjectsFromArray:@[actualItem.readableMediaType, actualItem.readableMediaSubType]];

    HeimdallInputItem * const inputItem = actualItem.inputItem;
    NSString * const discNumber = inputItem.discNumber;
    if (![discNumber isEqualToString:@""] && ![discNumber isEqualToString:@"0"]) {
        [strings addObject:[NSString stringWithFormat:@"%@ %@", _NS("Disc"), discNumber]];
    }

    return [strings componentsJoinedByString:@" · "];
}

- (void)updateRepresentation
{
    NSAssert(self.representedItem, @"no represented item assigned for collection view item", nil);
    HeimdallMediaLibraryMediaItem * const actualItem = self.representedItem.item.firstMediaItem;
    NSAssert(actualItem != nil, @"represented item is not a media item", nil);

    _mediaItemTitleTextField.stringValue = actualItem.displayString;
    _mediaItemPrimaryDetailButton.title = actualItem.primaryDetailString;
    _mediaItemSecondaryDetailButton.title = actualItem.secondaryDetailString;
    _mediaItemYearAndDurationAndTypeTextField.stringValue = [self formattedYearAndDurationAndTypeString];
    _mediaItemFileNameTextField.stringValue = actualItem.inputItem.name;
    _mediaItemPathTextField.stringValue = actualItem.inputItem.decodedMRL;

    const BOOL primaryActionableDetail = actualItem.primaryActionableDetail;
    const BOOL secondaryActionableDetail = actualItem.secondaryActionableDetail;
    self.mediaItemPrimaryDetailButton.enabled = primaryActionableDetail;
    self.mediaItemSecondaryDetailButton.enabled = secondaryActionableDetail;
    if (@available(macOS 10.14, *)) {
        NSColor * const primaryDetailButtonColor = 
            primaryActionableDetail ? NSColor.HeimdallAccentColor : NSColor.labelColor;
        NSColor * const secondaryDetailButtonColor = 
            secondaryActionableDetail ? NSColor.HeimdallAccentColor : NSColor.labelColor;
        self.mediaItemPrimaryDetailButton.contentTintColor = primaryDetailButtonColor;
        self.mediaItemSecondaryDetailButton.contentTintColor = secondaryDetailButtonColor;
    }
    self.mediaItemFavoriteButton.state = actualItem.favorited ? NSOnState : NSOffState;

    self.mediaItemPrimaryDetailButton.target = self;
    self.mediaItemSecondaryDetailButton.target = self;
    self.mediaItemFavoriteButton.target = self;

    self.mediaItemPrimaryDetailButton.action = @selector(primaryDetailAction:);
    self.mediaItemSecondaryDetailButton.action = @selector(secondaryDetailAction:);
    self.mediaItemFavoriteButton.action = @selector(favoriteAction:);

    const double ratingControlMax = self.mediaItemRatingIndicator.maxValue;
    const double proportionOfMaxRating = 100 / ratingControlMax;
    self.mediaItemRatingIndicator.doubleValue = (double)actualItem.rating / proportionOfMaxRating;

    NSArray<NSString *> * const mediaItemLabels = actualItem.labels;
    self.mediaItemLabelsTextField.hidden = mediaItemLabels.count == 0;
    self.mediaItemLabelsTitleTextField.hidden = self.mediaItemLabelsTextField.hidden;
    if (!self.mediaItemLabelsTextField.hidden) {
        self.mediaItemLabelsTextField.stringValue = [mediaItemLabels componentsJoinedByString:@", "];
    }

    self.mediaItemLastPlayedTextField.hidden = actualItem.lastPlayedDate == 0;
    self.mediaItemLastPlayedTitleTextField.hidden = self.mediaItemLastPlayedTextField.hidden;
    if (actualItem.lastPlayedDate > 0) {
        NSDate * const lastPlayedDate =
            [NSDate dateWithTimeIntervalSince1970:actualItem.lastPlayedDate];
        NSString * const lastPlayedString =
            [self.class.lastPlayedDateFormatter stringFromDate:lastPlayedDate];
        self.mediaItemLastPlayedTextField.stringValue = lastPlayedString;
    }

    NSString * const summary = actualItem.movie != nil
        ? actualItem.movie.summary : actualItem.showEpisode != nil
            ? actualItem.showEpisode.summary : @"";
    self.mediaItemSummaryStackView.hidden = summary == nil || [summary isEqualToString:@""];
    self.mediaItemSummarySeparator.hidden = self.mediaItemSummaryStackView.hidden;
    self.mediaItemSummaryTextField.stringValue = summary;

    HeimdallInputItem * const inputItem = actualItem.inputItem;

    NSString * const copyright = inputItem.copyright;
    self.mediaItemCopyrightTextField.hidden = [copyright isEqualToString:@""];
    self.mediaItemCopyrightTitleTextField.hidden = self.mediaItemCopyrightTextField.hidden;
    self.mediaItemCopyrightTextField.stringValue = copyright;

    NSString * const contentDescription = inputItem.contentDescription;
    self.mediaItemContentDescriptionTextField.hidden = [contentDescription isEqualToString:@""];
    self.mediaItemContentDescriptionTitleTextField.hidden = self.mediaItemContentDescriptionTextField.hidden;
    self.mediaItemContentDescriptionTextField.stringValue = contentDescription;

    NSString * const director = inputItem.director;
    self.mediaItemDirectorTextField.hidden = [inputItem.director isEqualToString:@""];
    self.mediaItemDirectorTitleTextField.hidden = self.mediaItemDirectorTextField.hidden;
    self.mediaItemDirectorTextField.stringValue = director;

    NSString * const actors = inputItem.actors;
    self.mediaItemActorsTextField.hidden = [inputItem.actors isEqualToString:@""];
    self.mediaItemActorsTitleTextField.hidden = self.mediaItemActorsTextField.hidden;
    self.mediaItemActorsTextField.stringValue = actors;

    __weak typeof(self) weakSelf = self;
    [HeimdallLibraryImageCache thumbnailForLibraryItem:actualItem withCompletion:^(NSImage * const thumbnail) {
        if (!weakSelf || weakSelf.representedItem.item != actualItem) {
            return;
        }
        HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView * const strongSelf = weakSelf;
        strongSelf->_mediaItemArtworkImageView.image = thumbnail;
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
    const id<HeimdallMediaLibraryItemProtocol> actualItem = self.representedItem.item;
    if (actualItem == nil || !actualItem.primaryActionableDetail) {
        return;
    }

    HeimdallLibraryWindow * const libraryWindow = HeimdallMain.sharedInstance.libraryWindow;
    const id<HeimdallMediaLibraryItemProtocol> libraryItem = actualItem.primaryActionableDetailLibraryItem;
    [libraryWindow presentLibraryItem:libraryItem];
}

- (IBAction)secondaryDetailAction:(id)sender
{
    const id<HeimdallMediaLibraryItemProtocol> actualItem = self.representedItem.item;
    if (actualItem == nil || !actualItem.secondaryActionableDetail) {
        return;
    }

    HeimdallLibraryWindow * const libraryWindow = HeimdallMain.sharedInstance.libraryWindow;
    const id<HeimdallMediaLibraryItemProtocol> libraryItem = actualItem.secondaryActionableDetailLibraryItem;
    [libraryWindow presentLibraryItem:libraryItem];
}

- (IBAction)favoriteAction:(id)sender
{
    const id<HeimdallMediaLibraryItemProtocol> mediaItem = self.representedItem.item;
    if ([mediaItem toggleFavorite] == HEIMDALL_SUCCESS) {
        HeimdallMediaLibraryMediaItem * const updatedItem =
            [HeimdallMediaLibraryMediaItem mediaItemForLibraryID:mediaItem.libraryID];
        self.representedItem =
            [[HeimdallLibraryRepresentedItem alloc] initWithItem:updatedItem
                                                 parentType:self.representedItem.parentType];
    }
}

- (IBAction)ratingAction:(id)sender
{
    NSParameterAssert([sender isKindOfClass:HeimdallLibraryRatingIndicator.class]);
    HeimdallLibraryRatingIndicator * const control = sender;
    const double proportion = 100 / control.maxValue;
    const double rating = control.doubleValue * proportion;
    ((HeimdallMediaLibraryMediaItem *)self.representedItem.item).rating = (int)rating;

    // NSLevelIndicatorCell handles mouse-up during tracking, so the view's mouseUp: is not called.
    [control commitCurrentValue];
}

- (IBAction)openPathAction:(id)sender
{
    [self.representedItem revealInFinder];
}

@end
