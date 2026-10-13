/*****************************************************************************
 * HeimdallLibraryHomeViewVideoCarouselContainerView.m: MacOS X interface module
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

#import "HeimdallLibraryHomeViewVideoCarouselContainerView.h"

#import "extensions/NSFont+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

#import "library/HeimdallLibraryCarouselViewItemView.h"
#import "library/HeimdallLibraryDataTypes.h"

#import "library/home-library/HeimdallLibraryHomeViewVideoContainerViewDataSource.h"

#import "views/HeimdallUIUnits.h"

#include "library/video-library/HeimdallLibraryVideoGroupDescriptor.h"

@implementation HeimdallLibraryHomeViewVideoCarouselContainerView

@synthesize groupDescriptor = _groupDescriptor;
@synthesize videoGroup = _videoGroup;
@synthesize dataSource = _dataSource;

- (void)setup
{
    [super setup];
    [self setupDataSource];
}

- (void)setupDataSource
{
    _dataSource = [[HeimdallLibraryHomeViewVideoContainerViewDataSource alloc] init];
    self.dataSource.carouselView = self.carouselView;
    [self.dataSource setup];
}

- (void)setGroupDescriptor:(HeimdallLibraryVideoCollectionViewGroupDescriptor *)groupDescriptor
{
    _groupDescriptor = groupDescriptor;
    _videoGroup = groupDescriptor.group;
    self.dataSource.groupDescriptor = groupDescriptor;
    if (groupDescriptor.group == HeimdallMediaLibraryParentGroupTypeRecentVideos) {
        self.titleView.stringValue = _NS("Recent videos");
    } else {
        self.titleView.stringValue = groupDescriptor.name;
    }
 }

- (void)setVideoGroup:(HeimdallMediaLibraryParentGroupType)group
{
    if (_groupDescriptor.group == group) {
        return;
    }

    HeimdallLibraryVideoCollectionViewGroupDescriptor * const descriptor = [[HeimdallLibraryVideoCollectionViewGroupDescriptor alloc] initWithHeimdallVideoLibraryGroup:group];
    [self setGroupDescriptor:descriptor];
}

- (void)presentLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if (libraryItem == nil) {
        return;
    }

    NSIndexPath * const indexPath = [self.dataSource indexPathForLibraryItem:libraryItem];
    if (indexPath == nil) {
        return;
    }

    [self.carouselView scrollToItemAtIndex:indexPath.item animated:YES];
}

@end
