/*****************************************************************************
 * HeimdallLibraryWindowChaptersSidebarViewController.m: MacOS X interface module
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

#import "HeimdallLibraryWindowChaptersSidebarViewController.h"

#import "extensions/NSString+Helpers.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "main/HeimdallMain.h"
#import "playqueue/HeimdallPlayerChapter.h"
#import "playqueue/HeimdallPlayerController.h"
#import "playqueue/HeimdallPlayQueueController.h"

@interface HeimdallLibraryWindowChaptersSidebarViewController ()

@property (readwrite) NSUInteger internalItemCount;

@end

@implementation HeimdallLibraryWindowChaptersSidebarViewController

@synthesize counterLabel = _counterLabel;

- (instancetype)initWithLibraryWindow:(HeimdallLibraryWindow *)libraryWindow
{
    return [super initWithLibraryWindow:libraryWindow
                                nibName:@"HeimdallLibraryWindowChaptersView"];
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    _chaptersArrayController = [[NSArrayController alloc] init];

    [self.tableView deselectAll:self];
    [self.tableView bind:NSContentBinding
                toObject:self.chaptersArrayController
             withKeyPath:@"arrangedObjects" 
                 options:nil];
    [self.tableView bind:NSSelectionIndexesBinding 
                toObject:self.chaptersArrayController
             withKeyPath:@"selectionIndexes"
                 options:nil];
    [self.tableView bind:NSSortDescriptorsBinding 
                toObject:self.chaptersArrayController
             withKeyPath:@"sortDescriptors"
                 options:nil];

    [self updateChapterList];
    [self updateSelectedChapter];
    
    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(titleListChanged:)
                               name:HeimdallPlayerTitleListChanged
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(chapterSelectionChanged:)
                               name:HeimdallPlayerChapterSelectionChanged
                             object:nil];
}

- (NSString *)title
{
    return _NS("Chapters");
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
    [self updateChapterList];
}

- (void)updateChapterList
{
    HeimdallPlayerController * const playerController =
        HeimdallMain.sharedInstance.playQueueController.playerController;

    const struct heimdall_player_title * const title = playerController.selectedTitle;
    if (title == NULL) {
        self.chaptersArrayController.content = @[];
        return;
    }
    const struct heimdall_player_chapter * const pp_chapters = title->chapters;
    const size_t chapterCount = title->chapter_count;

    self.internalItemCount = chapterCount;
    NSMutableArray * const chapters = [NSMutableArray arrayWithCapacity:chapterCount];
    for (size_t i = 0; i < chapterCount; i++) {
        struct heimdall_player_chapter p_chapter = pp_chapters[i];
        const heimdall_tick_t nextTime = (i + 1 < chapterCount)
            ? pp_chapters[i + 1].time
            : title->length;
        const heimdall_tick_t duration = nextTime - p_chapter.time;
        HeimdallPlayerChapter * const chapter =
            [[HeimdallPlayerChapter alloc] initWithChapter:&p_chapter duration:duration];
        [chapters addObject:chapter];
    }
    self.chaptersArrayController.content = chapters.copy;
    self.counterLabel.stringValue = [NSString stringWithFormat:@"%lu", chapterCount];
}

- (IBAction)tableViewAction:(id)sender
{
    const NSUInteger selectedIndex = self.chaptersArrayController.selectionIndex;
    if (selectedIndex == NSNotFound) {
        return;
    }

    HeimdallMain.sharedInstance.playQueueController.playerController.selectedChapterIndex =
        selectedIndex;
}

- (void)chapterSelectionChanged:(NSNotification *)notification
{
    [self updateSelectedChapter];
}

- (void)updateSelectedChapter
{
    const NSUInteger selectedChapterIndex =
        HeimdallMain.sharedInstance.playQueueController.playerController.selectedChapterIndex;
    NSIndexSet * const indexSet = [NSIndexSet indexSetWithIndex:selectedChapterIndex];
    [self.tableView selectRowIndexes:indexSet byExtendingSelection:NO];
}

# pragma mark - NSTableView delegation

- (NSView *)tableView:(NSTableView *)tableView
   viewForTableColumn:(NSTableColumn *)tableColumn
                  row:(NSInteger)row
{
    if ([tableColumn.identifier isEqualToString:@"HeimdallLibraryWindowChaptersTableViewNameColumnIdentifier"]) {
        NSTableCellView * const cellView =
            [tableView makeViewWithIdentifier:@"HeimdallLibraryWindowChaptersTableViewNameCellIdentifier"
                                        owner:self];
        [cellView.textField bind:NSValueBinding
                            toObject:cellView
                         withKeyPath:@"objectValue.name"
                             options:nil];
        return cellView;
    } else if ([tableColumn.identifier isEqualToString:@"HeimdallLibraryWindowChaptersTableViewTimeColumnIdentifier"]) {
        NSTableCellView * const cellView =
            [tableView makeViewWithIdentifier:@"HeimdallLibraryWindowChaptersTableViewTimeCellIdentifier"
                                        owner:self];
        [cellView.textField bind:NSValueBinding
                            toObject:cellView
                         withKeyPath:@"objectValue.durationString"
                             options:nil];
        return cellView;
    }

    NSAssert(NO, @"Provided cell view for chapters table view should be valid!");
    return nil;
}

@end
