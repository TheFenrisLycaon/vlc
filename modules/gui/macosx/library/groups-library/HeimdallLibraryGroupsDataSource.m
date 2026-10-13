/*****************************************************************************
 * HeimdallLibraryGroupsDataSource.m: MacOS X interface module
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

#import "HeimdallLibraryGroupsDataSource.h"

#import "extensions/NSString+Helpers.h"
#import "library/HeimdallLibraryModel.h"

@implementation HeimdallLibraryGroupsDataSource

- (instancetype)init
{
    self = [super init];
    if(self) {
        [self connect];
    }
    return self;
}

- (void)connect
{
    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;

    [notificationCenter addObserver:self
                           selector:@selector(libraryModelGroupsListReset:)
                               name:HeimdallLibraryModelListOfGroupsReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelGroupsListReset:)
                               name:HeimdallLibraryModelAllCachesDropped
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelGroupDeleted:)
                               name:HeimdallLibraryModelGroupDeleted
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelGroupUpdated:)
                               name:HeimdallLibraryModelGroupUpdated
                             object:nil];

    [self reloadData];
}

- (void)disconnect
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)libraryModelGroupsListReset:(NSNotification *)notification
{
    [self reloadData];
}

- (NSArray<HeimdallMediaLibraryGroup *> *)sourceBackingArray
{
    return self.libraryModel.listOfGroups;
}

- (HeimdallMediaLibraryParentGroupType)currentParentType
{
    return HeimdallMediaLibraryParentGroupTypeGroup;
}

- (void)libraryModelGroupUpdated:(NSNotification *)notification
{
    HeimdallMediaLibraryGroup * const group = notification.object;
    [self applySnapshotForChangedItemID:group.libraryID isDeletion:NO];
}

- (void)libraryModelGroupDeleted:(NSNotification *)notification
{
    HeimdallMediaLibraryGroup * const group = notification.object;
    [self applySnapshotForChangedItemID:group.libraryID isDeletion:YES];
}

- (NSString *)dataSourceTypeDisplayString
{
    return _NS("Group");
}

@end
