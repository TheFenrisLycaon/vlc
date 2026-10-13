/*****************************************************************************
 * HeimdallLibraryWindowTitlesSidebarViewController.m: MacOS X interface module
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

#import "HeimdallLibraryWindowTitlesSidebarViewController.h"

#import "extensions/NSString+Helpers.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "main/HeimdallMain.h"
#import "playqueue/HeimdallPlayerController.h"
#import "playqueue/HeimdallPlayerTitle.h"
#import "playqueue/HeimdallPlayQueueController.h"

@interface HeimdallLibraryWindowTitlesSidebarViewController ()

@property (readwrite) NSUInteger internalItemCount;

@end

@implementation HeimdallLibraryWindowTitlesSidebarViewController

@synthesize counterLabel = _counterLabel;

- (instancetype)initWithLibraryWindow:(HeimdallLibraryWindow *)libraryWindow
{
    return [super initWithLibraryWindow:libraryWindow
                                nibName:@"HeimdallLibraryWindowTitlesView"];
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    _titlesArrayController = [[NSArrayController alloc] init];

    [self.tableView deselectAll:self];
    [self.tableView bind:NSContentBinding
                toObject:self.titlesArrayController
             withKeyPath:@"arrangedObjects"
                 options:nil];
    [self.tableView bind:NSSelectionIndexesBinding
                toObject:self.titlesArrayController
             withKeyPath:@"selectionIndexes"
                 options:nil];
    [self.tableView bind:NSSortDescriptorsBinding
                toObject:self.titlesArrayController
             withKeyPath:@"sortDescriptors"
                 options:nil];

    [self updateTitleList];
    [self updateTitleSelection];

    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(titleListChanged:)
                               name:HeimdallPlayerTitleListChanged
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(titleSelectionChanged:)
                               name:HeimdallPlayerTitleSelectionChanged
                             object:nil];
}

- (NSString *)title
{
    return _NS("Titles");
}

- (BOOL)supportsItemCount
{
    return YES;
}

- (void)setCounterLabel:(NSTextField *)counterLabel
{
    _counterLabel = counterLabel;
    self.counterLabel.stringValue = [NSString stringWithFormat:@"%lu", self.internalItemCount];
}

- (void)titleListChanged:(NSNotification *)notification
{
    [self updateTitleList];
}

- (void)updateTitleList
{
    HeimdallPlayerController * const playerController =
        HeimdallMain.sharedInstance.playQueueController.playerController;
    const size_t titleCount = playerController.numberOfTitlesOfCurrentMedia;
    self.internalItemCount = titleCount;
    self.titlesArrayController.content = playerController.titlesOfCurrentMedia;
    self.counterLabel.stringValue = [NSString stringWithFormat:@"%lu", titleCount];
}

- (IBAction)tableViewAction:(id)sender
{
    HeimdallPlayerTitle * const selectedTitle =
        self.titlesArrayController.selectedObjects.firstObject;
    if (selectedTitle == nil) {
        return;
    }

    [HeimdallMain.sharedInstance.playQueueController.playerController setSelectedTitleIndex:selectedTitle.index];
}

- (void)titleSelectionChanged:(NSNotification *)notification
{
    [self updateTitleSelection];
}

- (void)updateTitleSelection
{
    HeimdallPlayerController * const playerController =
        HeimdallMain.sharedInstance.playQueueController.playerController;
    const NSUInteger selectedTitleIndex = playerController.selectedTitleIndex;
    NSIndexSet * const indexSet = [NSIndexSet indexSetWithIndex:selectedTitleIndex];
    [self.tableView selectRowIndexes:indexSet byExtendingSelection:NO];
}

# pragma mark - NSTableView delegation

- (NSView *)tableView:(NSTableView *)tableView
   viewForTableColumn:(NSTableColumn *)tableColumn
                  row:(NSInteger)row
{
    if ([tableColumn.identifier isEqualToString:@"HeimdallLibraryWindowTitlesTableViewNameColumnIdentifier"]) {
        NSTableCellView * const cellView =
            [tableView makeViewWithIdentifier:@"HeimdallLibraryWindowTitlesTableViewNameCellIdentifier"
                                        owner:self];
        [cellView.textField bind:NSValueBinding
                            toObject:cellView
                         withKeyPath:@"objectValue.name"
                             options:nil];
        return cellView;
    } else if ([tableColumn.identifier isEqualToString:@"HeimdallLibraryWindowTitlesTableViewLengthColumnIdentifier"]) {
        NSTableCellView * const cellView =
            [tableView makeViewWithIdentifier:@"HeimdallLibraryWindowTitlesTableViewLengthCellIdentifier"
                                        owner:self];
        [cellView.textField bind:NSValueBinding
                            toObject:cellView
                         withKeyPath:@"objectValue.lengthString"
                             options:nil];
        return cellView;
    }

    NSAssert(NO, @"Provided cell view for titles table view should be valid!");
    return nil;
}

@end
