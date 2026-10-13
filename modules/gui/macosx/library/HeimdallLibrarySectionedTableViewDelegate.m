/*****************************************************************************
 * HeimdallLibrarySectionedTableViewDelegate.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2025 VLC authors and VideoLAN
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

#import "HeimdallLibrarySectionedTableViewDelegate.h"

#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryHeaderView.h"
#import "library/HeimdallLibraryRepresentedItem.h"
#import "library/HeimdallLibrarySectionedTableViewDataSource.h"

#import "views/HeimdallUIUnits.h"

@interface HeimdallLibrarySectionedHeaderRowView : NSTableRowView
@end

@implementation HeimdallLibrarySectionedTableViewDelegate

- (instancetype)init
{
    self = [super init];
    if (self) {
        _rowHeight = HeimdallUIUnits.mediumTableViewRowHeight;
    }
    return self;
}

#pragma mark - NSTableViewDelegate

- (NSView *)tableView:(NSTableView *)tableView
    viewForTableColumn:(NSTableColumn *)tableColumn
                   row:(NSInteger)row
{
    if (![tableView.dataSource conformsToProtocol:@protocol(HeimdallLibrarySectionedTableViewDataSource)]) {
        return [super tableView:tableView viewForTableColumn:tableColumn row:row];
    }

    NSObject<HeimdallLibrarySectionedTableViewDataSource> * const sectionedDataSource =
        (NSObject<HeimdallLibrarySectionedTableViewDataSource> *)tableView.dataSource;

    if ([sectionedDataSource isHeaderRow:row]) {
        HeimdallLibraryHeaderView *headerView =
            (HeimdallLibraryHeaderView *)[tableView makeViewWithIdentifier:HeimdallLibraryHeaderViewIdentifier
                                                                               owner:self];
        if (headerView == nil) {
            headerView = [[HeimdallLibraryHeaderView alloc] initWithFrame:NSZeroRect];
            headerView.identifier = HeimdallLibraryHeaderViewIdentifier;
        }

        NSString * const title = [sectionedDataSource titleForRow:row];
        HeimdallLibraryRepresentedItem *representedItem = nil;
        if ([sectionedDataSource respondsToSelector:@selector(representedItemForHeaderRow:)]) {
            representedItem = [sectionedDataSource representedItemForHeaderRow:row];
        }
        [headerView updateWithRepresentedItem:representedItem
                                fallbackTitle:title
                               fallbackDetail:nil];
        return headerView;
    }

    return [super tableView:tableView viewForTableColumn:tableColumn row:row];
}

- (CGFloat)tableView:(NSTableView *)tableView heightOfRow:(NSInteger)row
{
    if ([tableView.dataSource conformsToProtocol:@protocol(HeimdallLibrarySectionedTableViewDataSource)]) {
        NSObject<HeimdallLibrarySectionedTableViewDataSource> * const sectionedDataSource =
            (NSObject<HeimdallLibrarySectionedTableViewDataSource> *)tableView.dataSource;
        if ([sectionedDataSource isHeaderRow:row]) {
            return HeimdallLibraryHeaderViewHeight;
        }
    }

    return self.rowHeight;
}

- (BOOL)tableView:(NSTableView *)tableView shouldSelectRow:(NSInteger)row
{
    if ([tableView.dataSource conformsToProtocol:@protocol(HeimdallLibrarySectionedTableViewDataSource)]) {
        NSObject<HeimdallLibrarySectionedTableViewDataSource> * const sectionedDataSource =
            (NSObject<HeimdallLibrarySectionedTableViewDataSource> *)tableView.dataSource;
        return ![sectionedDataSource isHeaderRow:row];
    }
    return YES;
}

- (BOOL)tableView:(NSTableView *)tableView isGroupRow:(NSInteger)row
{
    if ([tableView.dataSource conformsToProtocol:@protocol(HeimdallLibrarySectionedTableViewDataSource)]) {
        NSObject<HeimdallLibrarySectionedTableViewDataSource> * const sectionedDataSource =
            (NSObject<HeimdallLibrarySectionedTableViewDataSource> *)tableView.dataSource;
        return [sectionedDataSource isHeaderRow:row];
    }
    return NO;
}

- (NSTableRowView *)tableView:(NSTableView *)tableView rowViewForRow:(NSInteger)row
{
    if ([tableView.dataSource conformsToProtocol:@protocol(HeimdallLibrarySectionedTableViewDataSource)]) {
        NSObject<HeimdallLibrarySectionedTableViewDataSource> * const sectionedDataSource =
            (NSObject<HeimdallLibrarySectionedTableViewDataSource> *)tableView.dataSource;
        if ([sectionedDataSource isHeaderRow:row]) {
            HeimdallLibrarySectionedHeaderRowView * const rowView = [[HeimdallLibrarySectionedHeaderRowView alloc] init];
            return rowView;
        }
    }
    return nil;
}

@end

@implementation HeimdallLibrarySectionedHeaderRowView

- (instancetype)init
{
    self = [super init];
    if (self) {
        self.wantsLayer = YES;
        self.layer.masksToBounds = NO;
    }
    return self;
}

@end
