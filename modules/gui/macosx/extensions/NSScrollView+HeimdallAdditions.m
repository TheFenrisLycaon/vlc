/*****************************************************************************
 * NSScrollView+HeimdallAdditions.m: MacOS X interface module
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

#import "NSScrollView+HeimdallAdditions.h"

#import "views/HeimdallUIUnits.h"

@implementation NSScrollView (HeimdallAdditions)

+ (instancetype)libraryScrollViewWithDocumentView:(NSView *)documentView
                                    contentInsets:(NSEdgeInsets)contentInsets
{
    NSScrollView * const scrollView = [[NSScrollView alloc] init];
    scrollView.translatesAutoresizingMaskIntoConstraints = NO;
    scrollView.hasHorizontalScroller = NO;
    scrollView.borderType = NSNoBorder;
    scrollView.automaticallyAdjustsContentInsets = NO;
    scrollView.contentInsets = contentInsets;
    scrollView.scrollerInsets = HeimdallUIUnits.libraryViewScrollViewScrollerInsets;
    scrollView.documentView = documentView;
    return scrollView;
}

+ (instancetype)libraryScrollViewWithDocumentView:(NSView *)documentView
{
    return [self libraryScrollViewWithDocumentView:documentView
                                     contentInsets:HeimdallUIUnits.libraryViewScrollViewContentInsets];
}

+ (instancetype)libraryDetailScrollViewWithDocumentView:(NSView *)documentView
{
    return [self libraryScrollViewWithDocumentView:documentView
                                     contentInsets:HeimdallUIUnits.libraryViewScrollViewDetailListContentInsets];
}

@end
