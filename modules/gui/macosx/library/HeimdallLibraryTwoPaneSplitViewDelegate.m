/*****************************************************************************
 * HeimdallLibraryTwoPaneSplitViewDelegate.m: MacOS X interface module
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

#import "HeimdallLibraryTwoPaneSplitViewDelegate.h"

#import "library/HeimdallLibraryWindow.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallUIUnits.h"

NSString * const HeimdallLibrarySplitViewDetailViewWidthKey = @"HeimdallLibrarySplitViewDetailViewWidth";

@implementation HeimdallLibraryTwoPaneSplitViewDelegate

- (void)resetDefaultSplitForSplitView:(NSSplitView *)splitView
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSParameterAssert(splitView != nil);
        NSParameterAssert(splitView.window != nil);
        NSParameterAssert(splitView.subviews.count >= 2);
        
        NSUserDefaults * const defaults = NSUserDefaults.standardUserDefaults;
        CGFloat width = [defaults floatForKey:HeimdallLibrarySplitViewDetailViewWidthKey];
        if (width <= 0) {
            width = HeimdallUIUnits.librarySplitViewSelectionViewDefaultWidth;
        }
        [splitView setPosition:width ofDividerAtIndex:0];
    });
}

- (CGFloat)splitView:(NSSplitView *)splitView
constrainMinCoordinate:(CGFloat)proposedMinimumPosition
         ofSubviewAt:(NSInteger)dividerIndex
{
    if (dividerIndex != 0) {
        return proposedMinimumPosition;
    }

    return HeimdallUIUnits.librarySplitViewSelectionViewMinimumWidth;
}

- (CGFloat)splitView:(NSSplitView *)splitView
constrainMaxCoordinate:(CGFloat)proposedMinimumPosition
         ofSubviewAt:(NSInteger)dividerIndex
{
    if (dividerIndex != 0) {
        return proposedMinimumPosition;
    }

    HeimdallLibraryWindow * const libraryWindow = HeimdallMain.sharedInstance.libraryWindow;
    const CGFloat libraryWindowWidth = libraryWindow.frame.size.width;
    return libraryWindowWidth - HeimdallUIUnits.librarySplitViewMainViewMinimumWidth;
}

- (void)splitViewDidResizeSubviews:(NSNotification *)notification
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSSplitView * const splitView = notification.object;
        NSParameterAssert(splitView != nil);
        NSParameterAssert(splitView.subviews.count >= 2);

        NSUserDefaults * const defaults = NSUserDefaults.standardUserDefaults;
        if ([defaults floatForKey:HeimdallLibrarySplitViewDetailViewWidthKey] <= 0) {
            return;
        }
        
        NSView * const detailView = splitView.arrangedSubviews[0];
        const CGFloat detailViewWidth = detailView.frame.size.width;
        [defaults setFloat:detailViewWidth forKey:HeimdallLibrarySplitViewDetailViewWidthKey];
    });
}

@end
