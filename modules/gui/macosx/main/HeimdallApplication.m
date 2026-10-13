/*****************************************************************************
 * HeimdallApplication.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2002-2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Derk-Jan Hartman <hartman at videolan.org>
 *          Felix Paul Kühne <fkuehne at lycaria dot org>
 *          Pierre d'Herbemont <pdherbemont # lycaria org>
 *          David Fuhrmann <david dot fuhrmann at googlemail dot com>
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#import "HeimdallApplication.h"
#import "extensions/NSImage+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

#import <heimdall_configuration.h>
#import <heimdall_interface.h>

/*****************************************************************************
 * HeimdallApplication implementation
 *****************************************************************************/

@interface HeimdallApplication ()
{
    NSURL *_appLocationURL;
    NSImage *_heimdallAppIconImage;
    intf_thread_t *_intf;
}

@end

@implementation HeimdallApplication

- (instancetype)init
{
    self = [super init];
    if (self) {
        [NSNotificationCenter.defaultCenter addObserver:self
                                                 selector:@selector(appBecameActive:)
                                                     name:NSApplicationDidBecomeActiveNotification
                                                   object:nil];
        /* we need to keep a file reference to the app's current location so we can find out where
         * it ends-up after being relocated or rename */
        _appLocationURL = [[[NSBundle mainBundle] bundleURL] fileReferenceURL];

        if (config_GetInt("macosx-icon-change")) {
            NSCalendar *const gregorian =
                [[NSCalendar alloc] initWithCalendarIdentifier:NSCalendarIdentifierGregorian];
            const NSUInteger dayOfYear = [gregorian ordinalityOfUnit:NSCalendarUnitDay
                                                              inUnit:NSCalendarUnitYear
                                                             forDate:[NSDate date]];

            if (dayOfYear >= 354) {
                _winterHolidaysTheming = YES;
            }
        }

    }
    return self;
}

- (void)setIntf:(intf_thread_t*)intf
{
    _intf = intf;
}

- (void)dealloc
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (NSImage *)heimdallAppIconImage
{
    NSAssert(NSThread.isMainThread, @"I must be called from the main thread only!");

    // If we already have an image, immediately return that
    if (_heimdallAppIconImage != nil)
        return _heimdallAppIconImage;

    if (self.winterHolidaysTheming) {
        /* After day 354 of the year, the usual Heimdall cone is replaced by another cone
         * wearing a Father Xmas hat.
         * Note: this icon doesn't represent an endorsement of The Coca-Cola Company.
         */
        _heimdallAppIconImage = NSImage.HeimdallXmasAppIconImage;
    }

    if (_heimdallAppIconImage == nil)
        _heimdallAppIconImage = NSImage.HeimdallAppIconImage;

    return _heimdallAppIconImage;
}

- (void)appBecameActive:(NSNotification *)aNotification
{
    if ([[[NSBundle mainBundle] bundleURL] checkResourceIsReachableAndReturnError:nil]) {
        return;
    }

    NSAlert *alert = [[NSAlert alloc] init];
    [alert setAlertStyle:NSAlertStyleCritical];
    [alert setMessageText:_NS("Heimdall has been moved or renamed")];
    [alert setInformativeText:_NS("To prevent errors, Heimdall must be relaunched.\n\nIf you cannot quit immediately, click Continue, then quit and relaunch as soon as possible to avoid problems.")];
    [alert addButtonWithTitle:_NS("Restart")];
    [alert addButtonWithTitle:_NS("Quit")];
    [alert addButtonWithTitle:_NS("Continue (Not Recommended)")];

    NSInteger alertButton = [alert runModal];

    if (alertButton == NSAlertThirdButtonReturn) {
        return;
    }

    if (alertButton == NSAlertFirstButtonReturn) {
        /* terminate and restart
         * NOTE you may not use [HeimdallMain relaunchApplication] here as it depends on the app not having moved and WILL crash */
        [NSWorkspace.sharedWorkspace launchApplicationAtURL:_appLocationURL.absoluteURL
                                                      options:NSWorkspaceLaunchNewInstance
                                                configuration:@{}
                                                        error:nil];
    }

    [self terminate:self];
}

// when user selects the quit menu from dock it sends a terminate:
// but we need to send a stop: to properly exits libheimdall.
// However, we are not able to change the action-method sent by this standard menu item.
// thus we override terminate: to send a stop:
- (void)terminate:(id)sender
{
    [self activateIgnoringOtherApps:YES];
    libheimdall_Quit(heimdall_object_instance(_intf));
}

@end
