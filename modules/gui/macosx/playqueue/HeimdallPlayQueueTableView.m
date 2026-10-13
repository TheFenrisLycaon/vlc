/*****************************************************************************
 * HeimdallPlayQueueTableView.m: table view subclass for the play queue
 *****************************************************************************
 * Copyright (C) 2003-2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Derk-Jan Hartman <hartman at videola/n dot org>
 *          Benjamin Pracht <bigben at videolab dot org>
 *          Felix Paul Kühne <fkuehne at lycaria dot org>
 *          David Fuhrmann <dfuhrmann # videolan.org>
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

#import "HeimdallPlayQueueTableView.h"

#import "main/HeimdallMain.h"
#import "playqueue/HeimdallPlayQueueController.h"
#import "playqueue/HeimdallPlayQueueMenuController.h"

@interface HeimdallPlayQueueTableView ()

@property (readonly, atomic) HeimdallPlayQueueMenuController *menuController;

@end

@implementation HeimdallPlayQueueTableView

- (NSMenu *)menuForEvent:(NSEvent *)event
{
    if (self.menuController == nil) {
        _menuController = [[HeimdallPlayQueueMenuController alloc] init];
        self.menuController.playQueueTableView = self;
    }

    const NSPoint pt = [self convertPoint: [event locationInWindow] fromView: nil];
    const NSInteger row = [self rowAtPoint:pt];
    if (row != -1 && ![[self selectedRowIndexes] containsIndex: row])
        [self selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];

    const HeimdallPlayQueueContextMenuTarget target =
        row != -1 ? HeimdallPlayQueueContextMenuTargetRow : HeimdallPlayQueueContextMenuTargetBackground;
    [self.menuController prepareForContextMenuTarget:target];

    return self.menuController.playQueueMenu;
}

- (void)keyDown:(NSEvent *)event
{
    unichar key = 0;

    if ([[event characters] length]) {
        /* we evaluate the first key only */
        key = [[event characters] characterAtIndex: 0];
    }

    size_t indexOfSelectedItem = self.selectedRow;
    NSIndexSet *selectedIndexes = [self selectedRowIndexes];

    switch(key) {
        case NSDeleteCharacter:
        case NSDeleteFunctionKey:
        case NSDeleteCharFunctionKey:
        case NSBackspaceCharacter:
        {
            HeimdallPlayQueueController *playQueueController = HeimdallMain.sharedInstance.playQueueController;
            [playQueueController removeItemsAtIndexes:selectedIndexes];
            break;
        }

        case NSEnterCharacter:
        case NSCarriageReturnCharacter:
            [HeimdallMain.sharedInstance.playQueueController playItemAtIndex:indexOfSelectedItem];
            break;

        default:
            [super keyDown: event];
            break;
    }
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    [self setNeedsDisplay:YES];
    return YES;
}

- (BOOL)resignFirstResponder
{
    [self setNeedsDisplay:YES];
    return YES;
}

@end
