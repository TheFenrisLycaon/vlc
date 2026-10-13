/*****************************************************************************
 * HeimdallUIWidgets.h: Widgets for Heimdall's extensions dialogs for Mac OS X
 *****************************************************************************
 * Copyright (C) 2009-2015 the VideoLAN team and authors
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Pierre d'Herbemont <pdherbemont # lycaria dot>,
 *          Brendon Justin <brendonjustin@gmail.com>
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

#import <Cocoa/Cocoa.h>
#import <heimdall_extensions.h>

@class HeimdallDialogGridView;

@interface HeimdallDialogButton : NSButton
@property (readwrite) extension_widget_t *widget;
@end

@interface HeimdallDialogPopUpButton : NSPopUpButton
@property (readwrite) extension_widget_t *widget;
@end

@interface HeimdallDialogLabel : NSTextField
@end

@interface HeimdallDialogTextField : NSTextField
@property (readwrite) extension_widget_t *widget;
@end

@interface HeimdallDialogSecureTextField : NSSecureTextField
@property (readwrite) extension_widget_t *widget;
@end

@interface HeimdallDialogWindow : NSWindow
@property (readwrite) extension_dialog_t *dialog;
@property (readwrite) BOOL has_lock;
@end


@interface HeimdallDialogList : NSTableView <NSTableViewDataSource>
@property (readwrite) extension_widget_t *widget;
@property (readwrite, retain) NSMutableArray *contentArray;
@end

@interface HeimdallDialogGridView : NSView

- (void)addSubview:(NSView *)view atRow:(NSUInteger)row column:(NSUInteger)column rowSpan:(NSUInteger)rowSpan colSpan:(NSUInteger)colSpan;
- (NSSize)flexSize:(NSSize)size;
- (void)updateSubview:(NSView *)view
                atRow:(NSUInteger)row
               column:(NSUInteger)column
              rowSpan:(NSUInteger)rowSpan
              colSpan:(NSUInteger)colSpan;
- (void)removeSubview:(NSView *)view;

@property (readonly) NSUInteger numViews;

@end
