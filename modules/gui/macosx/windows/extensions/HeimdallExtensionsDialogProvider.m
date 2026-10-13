/*****************************************************************************
 * HeimdallExtensionsDialogProvider.m: Mac OS X Extensions Dialogs
 *****************************************************************************
 * Copyright (C) 2010-2015 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Pierre d'Herbemont <pdherbemont # lycaria org>
 *          Brendon Justin <brendonjustin@gmail.com>,
 *          Derk-Jan Hartman <hartman@lycaria dot org>,
 *          Felix Paul Kühne <fkuehne@lycaria dot org>
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

#import "HeimdallExtensionsDialogProvider.h"

#import "main/HeimdallMain.h"
#import "HeimdallExtensionsManager.h"
#import "extensions/NSString+Helpers.h"
#import "windows/extensions/HeimdallUIWidgets.h"

#import <WebKit/WebKit.h>

/*****************************************************************************
 * HeimdallExtensionsDialogProvider implementation
 *****************************************************************************/

static void extensionDialogCallback(extension_dialog_t *p_ext_dialog,
                                    void *p_data);

static NSView *createControlFromWidget(extension_widget_t *widget, id self)
{
    @autoreleasepool {
        assert(!widget->p_sys_intf);
        switch (widget->type) {
            case EXTENSION_WIDGET_HTML:
            {
                WebView *webView = [[WebView alloc] initWithFrame:NSMakeRect (0,0,1,1)];
                [webView setAutoresizingMask:NSViewHeightSizable | NSViewWidthSizable];
                [webView setDrawsBackground:NO];
                return webView;
            }
            case EXTENSION_WIDGET_LABEL:
            {
                HeimdallDialogLabel *field = [[HeimdallDialogLabel alloc] init];
                [field setEditable:NO];
                [field setBordered:NO];
                [field setDrawsBackground:NO];
                [field setAllowsEditingTextAttributes:YES];
                [field setSelectable:YES];
                [field setFont:[NSFont systemFontOfSize:0]];
                [[field cell] setControlSize:NSControlSizeRegular];
                [field setAutoresizingMask:NSViewNotSizable];
                return field;
            }
            case EXTENSION_WIDGET_TEXT_FIELD:
            {
                HeimdallDialogTextField *field = [[HeimdallDialogTextField alloc] init];
                [field setWidget:widget];
                [field setAutoresizingMask:NSViewWidthSizable];
                [field setFont:[NSFont systemFontOfSize:0]];
                [[field cell] setControlSize:NSControlSizeRegular];
                [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(syncTextField:)  name:NSControlTextDidChangeNotification object:field];
                return field;
            }
            case EXTENSION_WIDGET_PASSWORD:
            {
                HeimdallDialogSecureTextField *field = [[HeimdallDialogSecureTextField alloc] init];
                [field setWidget:widget];
                [field setAutoresizingMask:NSViewWidthSizable];
                [field setFont:[NSFont systemFontOfSize:0]];
                [[field cell] setControlSize:NSControlSizeRegular];
                [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(syncTextField:)  name:NSControlTextDidChangeNotification object:field];
                return field;
            }

            case EXTENSION_WIDGET_CHECK_BOX:
            {
                HeimdallDialogButton *button = [[HeimdallDialogButton alloc] init];
                [button setButtonType:NSSwitchButton];
                [button setWidget:widget];
                [button setAction:@selector(triggerClick:)];
                [button setTarget:self];
                [button setFont:[NSFont systemFontOfSize:0.0]];
                [[button cell] setControlSize:NSControlSizeRegular];
                [button setAutoresizingMask:NSViewWidthSizable];
                return button;
            }
            case EXTENSION_WIDGET_BUTTON:
            {
                HeimdallDialogButton *button = [[HeimdallDialogButton alloc] init];
                [button setBezelStyle:NSRoundedBezelStyle];
                [button setWidget:widget];
                [button setAction:@selector(triggerClick:)];
                [button setTarget:self];
                [button setFont:[NSFont systemFontOfSize:0.0]];
                [[button cell] setControlSize:NSControlSizeRegular];
                [button setAutoresizingMask:NSViewNotSizable];
                return button;
            }
            case EXTENSION_WIDGET_DROPDOWN:
            {
                HeimdallDialogPopUpButton *popup = [[HeimdallDialogPopUpButton alloc] init];
                [popup setAction:@selector(popUpSelectionChanged:)];
                [popup setTarget:self];
                [popup setWidget:widget];
                return popup;
            }
            case EXTENSION_WIDGET_LIST:
            {
                NSScrollView *scrollView = [[NSScrollView alloc] init];
                [scrollView setHasVerticalScroller:YES];
                HeimdallDialogList *list = [[HeimdallDialogList alloc] init];
                [list setUsesAlternatingRowBackgroundColors:YES];
                [list setHeaderView:nil];
                [list setAllowsMultipleSelection:YES];
                [scrollView setDocumentView:list];
                [scrollView setAutoresizingMask:NSViewHeightSizable | NSViewWidthSizable];

                NSTableColumn *column = [[NSTableColumn alloc] init];
                [list addTableColumn:column];
                [list setDataSource:list];
                [list setDelegate:self];
                [list setWidget:widget];
                return scrollView;
            }
            case EXTENSION_WIDGET_IMAGE:
            {
                NSImageView *imageView = [[NSImageView alloc] init];
                [imageView setAutoresizingMask:NSViewHeightSizable | NSViewWidthSizable];
                [imageView setImageFrameStyle:NSImageFramePhoto];
                [imageView setImageScaling:NSImageScaleProportionallyUpOrDown];
                return imageView;
            }
            case EXTENSION_WIDGET_SPIN_ICON:
            {
                NSProgressIndicator *spinner = [[NSProgressIndicator alloc] init];
                [spinner setUsesThreadedAnimation:YES];
                [spinner setStyle:NSProgressIndicatorSpinningStyle];
                [spinner setDisplayedWhenStopped:YES];
                [spinner startAnimation:self];
                return spinner;
            }
            default:
                msg_Err(getIntf(), "Unhandled Widget type %i", widget->type);
                return nil;
        }
    }
}

static void updateControlFromWidget(NSView *control, extension_widget_t *widget, id self)
{
    @autoreleasepool {
        NSString * const defaultStyleCSS = @"<style>*{ font-family: \
            -apple-system-body, -apple-system, \
            HelveticaNeue, Arial, sans-serif; }</style>";
        switch (widget->type) {
            case EXTENSION_WIDGET_HTML:
            {
                // Get the web view
                assert([control isKindOfClass:[WebView class]]);
                WebView *webView = (WebView *)control;
                NSString *string = [defaultStyleCSS stringByAppendingString:toNSStr(widget->psz_text)];
                [[webView mainFrame] loadHTMLString:string baseURL:[NSURL URLWithString:@""]];
                [webView setNeedsDisplay:YES];
                break;
            }
            case EXTENSION_WIDGET_LABEL:
            case EXTENSION_WIDGET_PASSWORD:
            case EXTENSION_WIDGET_TEXT_FIELD:
            {
                if (!widget->psz_text)
                    break;
                assert([control isKindOfClass:[NSControl class]]);
                NSControl *field = (NSControl *)control;
                NSString *string = [defaultStyleCSS stringByAppendingString:toNSStr(widget->psz_text)];
                NSAttributedString *attrString = [[NSAttributedString alloc] initWithHTML:[string dataUsingEncoding: NSISOLatin1StringEncoding] documentAttributes:NULL];
                [field setAttributedStringValue:attrString];
                break;
            }
            case EXTENSION_WIDGET_CHECK_BOX:
            case EXTENSION_WIDGET_BUTTON:
            {
                assert([control isKindOfClass:[NSButton class]]);
                NSButton *button = (NSButton *)control;
                [button setTitle:toNSStr(widget->psz_text)];
                if (widget->type == EXTENSION_WIDGET_CHECK_BOX)
                    [button setState:widget->b_checked ? NSOnState : NSOffState];
                break;
            }
            case EXTENSION_WIDGET_DROPDOWN:
            {
                assert([control isKindOfClass:[NSPopUpButton class]]);
                NSPopUpButton *popup = (NSPopUpButton *)control;
                [popup removeAllItems];
                struct extension_widget_value_t *value;
                for (value = widget->p_values; value != NULL; value = value->p_next)
                    [[popup menu] addItemWithTitle:toNSStr(value->psz_text) action:nil keyEquivalent:@""];

                [popup synchronizeTitleAndSelectedItem];
                [self popUpSelectionChanged:popup];
                break;
            }
            case EXTENSION_WIDGET_LIST:
            {
                assert([control isKindOfClass:[NSScrollView class]]);
                NSScrollView *scrollView = (NSScrollView *)control;
                assert([[scrollView documentView] isKindOfClass:[HeimdallDialogList class]]);
                HeimdallDialogList *list = (HeimdallDialogList *)[scrollView documentView];

                NSMutableArray *contentArray = [NSMutableArray array];
                struct extension_widget_value_t *value;
                for (value = widget->p_values; value != NULL; value = value->p_next)
                {
                    NSDictionary *entry = [NSDictionary dictionaryWithObjectsAndKeys:
                                           [NSNumber numberWithInt:value->i_id], @"id",
                                           toNSStr(value->psz_text), @"text",
                                           nil];
                    [contentArray addObject:entry];
                }
                list.contentArray = contentArray;
                [list reloadData];
                break;
            }
            case EXTENSION_WIDGET_IMAGE:
            {
                assert([control isKindOfClass:[NSImageView class]]);
                NSImageView *imageView = (NSImageView *)control;
                NSString *string = widget->psz_text ? toNSStr(widget->psz_text) : nil;
                NSImage *image = nil;
                if (string)
                    image = [[NSImage alloc] initWithContentsOfURL:[NSURL fileURLWithPath:string]];
                [imageView setImage:image];
                break;
            }
            case EXTENSION_WIDGET_SPIN_ICON:
            {
                assert([control isKindOfClass:[NSProgressIndicator class]]);
                NSProgressIndicator *progressIndicator = (NSProgressIndicator *)control;
                if (widget->i_spin_loops != 0)
                    [progressIndicator startAnimation:self];
                else
                    [progressIndicator stopAnimation:self];
                break;
            }
        }
    }
}

/**
 * Ask the dialogs provider to create a new dialog
 **/

static void extensionDialogCallback(extension_dialog_t *p_ext_dialog,
                                    void *p_data)

{
    @autoreleasepool {
        HeimdallExtensionsDialogProvider *provider = (__bridge HeimdallExtensionsDialogProvider *)p_data;
        if (!provider)
            return;

        [provider manageDialog:p_ext_dialog];
        return;
    }
}

@implementation HeimdallExtensionsDialogProvider

- (id)init
{
    self = [super init];
    if (self) {
        intf_thread_t *p_intf = getIntf();
        heimdall_dialog_provider_set_ext_callback(p_intf, extensionDialogCallback, (__bridge void *)self);
    }
    return self;
}

- (void)dealloc
{
    heimdall_dialog_provider_set_ext_callback(getIntf(), NULL, NULL);
}

- (void)performEventWithObject:(NSValue *)objectValue ofType:(const char*)type
{
    NSString *typeString = toNSStr(type);

    if ([typeString isEqualToString: @"dialog-extension"]) {
        [self performSelectorOnMainThread:@selector(updateExtensionDialog:)
                               withObject:objectValue
                            waitUntilDone:YES];

    }
    else
        msg_Err(getIntf(), "unhandled dialog type: '%s'", type);
}

- (void)triggerClick:(id)sender
{
    assert([sender isKindOfClass:[HeimdallDialogButton class]]);
    HeimdallDialogButton *button = sender;
    extension_widget_t *widget = [button widget];

    heimdall_mutex_lock(&widget->p_dialog->lock);
    if (widget->type == EXTENSION_WIDGET_BUTTON)
        extension_WidgetClicked(widget->p_dialog, widget);
    else
        widget->b_checked = [button state] == NSOnState;
    heimdall_mutex_unlock(&widget->p_dialog->lock);
}

- (void)syncTextField:(NSNotification *)notification
{
    id sender = [notification object];
    assert([sender isKindOfClass:[HeimdallDialogTextField class]] ||
        [sender isKindOfClass:[HeimdallDialogSecureTextField class]]);
    NSTextField *field = sender;
    extension_widget_t *widget;

    if ([sender isKindOfClass:[HeimdallDialogTextField class]])
        widget = [(HeimdallDialogTextField*)field widget];
    else if ([sender isKindOfClass:[HeimdallDialogSecureTextField class]])
        widget = [(HeimdallDialogSecureTextField*)field widget];
    else
        return;

    heimdall_mutex_lock(&widget->p_dialog->lock);
    free(widget->psz_text);
    widget->psz_text = strdup([[field stringValue] UTF8String]);
    heimdall_mutex_unlock(&widget->p_dialog->lock);
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
    id sender = [notification object];
    assert(sender && [sender isKindOfClass:[HeimdallDialogList class]]);
    HeimdallDialogList *list = sender;

    struct extension_widget_value_t *value;
    unsigned i = 0;
    NSIndexSet *selectedIndexes = [list selectedRowIndexes];
    for (value = [list widget]->p_values; value != NULL; value = value->p_next, i++)
        value->b_selected = (YES == [selectedIndexes containsIndex:i]);
}

- (void)popUpSelectionChanged:(id)sender
{
    assert([sender isKindOfClass:[HeimdallDialogPopUpButton class]]);
    HeimdallDialogPopUpButton *popup = sender;
    struct extension_widget_value_t *value;
    unsigned i = 0;
    for (value = [popup widget]->p_values; value != NULL; value = value->p_next, i++)
        value->b_selected = (i == [popup indexOfSelectedItem]);

}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize
{
    NSView *contentView = [sender contentView];
    assert([contentView isKindOfClass:[HeimdallDialogGridView class]]);
    HeimdallDialogGridView *gridView = (HeimdallDialogGridView *)contentView;

    NSRect rect = NSMakeRect(0, 0, 0, 0);
    rect.size = frameSize;
    rect = [sender contentRectForFrameRect:rect];
    rect.size = [gridView flexSize:rect.size];
    rect = [sender frameRectForContentRect:rect];
    return rect.size;
}

- (BOOL)windowShouldClose:(id)sender
{
    assert([sender isKindOfClass:[HeimdallDialogWindow class]]);
    HeimdallDialogWindow *window = sender;
    extension_dialog_t *dialog = [window dialog];
    extension_DialogClosed(dialog);
    
    heimdall_mutex_lock(&dialog->lock);
    if (dialog->p_sys_intf) {
        CFRelease(dialog->p_sys_intf);
        dialog->p_sys_intf = NULL;
    }
    heimdall_mutex_unlock(&dialog->lock);
    
    return YES;
}

- (void)updateWidgets:(extension_dialog_t *)dialog
{
    extension_widget_t *widget;
    HeimdallDialogWindow *dialogWindow = (__bridge HeimdallDialogWindow *)(dialog->p_sys_intf);

    ARRAY_FOREACH(widget, dialog->widgets) {
        if (!widget)
            continue; /* Some widgets may be NULL@this point */

        BOOL shouldDestroy = widget->b_kill;

        /* Ownership should not be transferred back to ARC here, as
         * we might just want to update something.
         */
        NSView *control = (__bridge NSView *)widget->p_sys_intf;
        BOOL update = widget->b_update;

        if (!control && !shouldDestroy) {
            control = createControlFromWidget(widget, self);
            if (control == NULL)
                msg_Err(getIntf(), "Failed to create control from widget!");
            updateControlFromWidget(control, widget, self);
            /* Ownership needs to be given-up, if ARC would remain with the
             * ownership, the object could be freed while it is still referenced
             * and the invalid reference would be used later.
             */
            widget->p_sys_intf = (__bridge_retained void *)control;
            update = YES; // Force update and repositionning
            [control setHidden:widget->b_hide];
        }

        if (update && !shouldDestroy) {
            updateControlFromWidget(control, widget, self);
            [control setHidden:widget->b_hide];

            int row = widget->i_row - 1;
            int col = widget->i_column - 1;
            int hsp = MAX(1, widget->i_horiz_span);
            int vsp = MAX(1, widget->i_vert_span);
            if (row < 0) {
                row = 4;
                col = 0;
            }

            HeimdallDialogGridView *gridView = (HeimdallDialogGridView *)[dialogWindow contentView];
            [gridView updateSubview:control atRow:row column:col rowSpan:vsp colSpan:hsp];

            widget->b_update = false;
        }

        if (shouldDestroy) {
            HeimdallDialogGridView *gridView = (HeimdallDialogGridView *)[dialogWindow contentView];
            [gridView removeSubview:control];
            /* Explicitly release here, as we do not have transferred ownership to ARC,
             * given that not in all cases we want to destroy the widget.
             */
            if (widget->p_sys_intf) {
                CFRelease(widget->p_sys_intf);
                widget->p_sys_intf = NULL;
            }
        }
    }
}

/** Create a dialog
 * Note: Lock on p_dialog->lock must be held. */
- (HeimdallDialogWindow *)createExtensionDialog:(extension_dialog_t *)p_dialog
{
    HeimdallDialogWindow *dialogWindow;

    BOOL shouldDestroy = p_dialog->b_kill;
    if (!shouldDestroy) {
        NSRect content = NSMakeRect(0, 0, 1, 1);
        dialogWindow = [[HeimdallDialogWindow alloc] initWithContentRect:content
                                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                                            backing:NSBackingStoreBuffered
                                                              defer:NO];
        [dialogWindow setReleasedWhenClosed:NO];
        [dialogWindow setDelegate:self];
        [dialogWindow setDialog:p_dialog];
        [dialogWindow setTitle:toNSStr(p_dialog->psz_title)];

        HeimdallDialogGridView *gridView = [[HeimdallDialogGridView alloc] init];
        [gridView setAutoresizingMask:NSViewHeightSizable | NSViewWidthSizable];
        [dialogWindow setContentView:gridView];

        p_dialog->p_sys_intf = (void *)CFBridgingRetain(dialogWindow);
    }

    [self updateWidgets:p_dialog];

    if (shouldDestroy) {
        [dialogWindow setDelegate:nil];
        [dialogWindow close];
        p_dialog->p_sys_intf = NULL;
        dialogWindow = nil;
    }

    return dialogWindow;
}

/** Destroy a dialog
 * Note: Lock on p_dialog->lock must be held. */
- (int)destroyExtensionDialog:(extension_dialog_t *)p_dialog
{
    assert(p_dialog);

    HeimdallDialogWindow *dialogWindow = (__bridge HeimdallDialogWindow*)p_dialog->p_sys_intf;
    if (!dialogWindow) {
        msg_Warn(getIntf(), "dialog window not found");
        return HEIMDALL_EGENERIC;
    }

    [dialogWindow setDelegate:nil];
    [dialogWindow close];
    dialogWindow = nil;

    /* Release the reference retained by CFBridgingRetain in
     * createExtensionDialog. We use CFRelease rather than
     * CFBridgingRelease to avoid transferring ownership back
     * to ARC, which would crash as the pointer is still live
     * on the call stack.
     */
    CFRelease(p_dialog->p_sys_intf);
    p_dialog->p_sys_intf = NULL;
    heimdall_cond_signal(&p_dialog->cond);
    return HEIMDALL_SUCCESS;
}

/**
 * Update/Create/Destroy a dialog
 **/
- (HeimdallDialogWindow *)updateExtensionDialog:(NSValue *)o_value
{
    extension_dialog_t *p_dialog = [o_value pointerValue];

    HeimdallDialogWindow *dialogWindow = (__bridge HeimdallDialogWindow*) p_dialog->p_sys_intf;
    if (p_dialog->b_kill && !dialogWindow) {
        /* This extension could not be activated properly but tried
           to create a dialog. We must ignore it. */
        return NULL;
    }

    heimdall_mutex_lock(&p_dialog->lock);
    if (!p_dialog->b_kill && !dialogWindow) {
        dialogWindow = [self createExtensionDialog:p_dialog];

        BOOL visible = !p_dialog->b_hide;
        if (visible) {
            [dialogWindow center];
            [dialogWindow makeKeyAndOrderFront:self];
        } else
            [dialogWindow orderOut:nil];

        [dialogWindow setHas_lock:NO];
    }
    else if (!p_dialog->b_kill && dialogWindow) {
        [dialogWindow setHas_lock:YES];
        [self updateWidgets:p_dialog];
        if (strcmp([[dialogWindow title] UTF8String],
                    p_dialog->psz_title) != 0) {
            NSString *titleString = toNSStr(p_dialog->psz_title);

            [dialogWindow setTitle:titleString];
        }

        [dialogWindow setHas_lock:NO];

        BOOL visible = !p_dialog->b_hide;
        if (visible)
            [dialogWindow makeKeyAndOrderFront:self];
        else
            [dialogWindow orderOut:nil];
    }
    else if (p_dialog->b_kill) {
        [self destroyExtensionDialog:p_dialog];
    }
    heimdall_cond_signal(&p_dialog->cond);
    heimdall_mutex_unlock(&p_dialog->lock);
    return dialogWindow;
}

/**
 * Ask the dialog manager to create/update/kill the dialog. Thread-safe.
 **/
- (void)manageDialog:(extension_dialog_t *)p_dialog
{
    assert(p_dialog);

    NSValue *o_value = [NSValue valueWithPointer:p_dialog];
    [self performSelectorOnMainThread:@selector(updateExtensionDialog:)
                           withObject:o_value
                        waitUntilDone:YES];
}

@end
