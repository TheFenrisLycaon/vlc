/*****************************************************************************
 * intf-prefs.m
 *****************************************************************************
 * Copyright (C) 2001-2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Pierre d'Herbemont <pdherbemont # lycaria org>
 *          Felix Paul Kühne <fkuehne at lycaria dot org>
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

#import "HeimdallMain+OldPrefs.h"

#include <unistd.h> /* execl() */

#import <heimdall_configuration.h>
#import <heimdall_interface.h>

#import "extensions/NSString+Helpers.h"
#import "preferences/HeimdallSimplePrefsController.h"

@implementation HeimdallMain(OldPrefs)

static const int kCurrentPreferencesVersion = 4;

+ (void)initialize
{
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObject:[NSNumber numberWithInt:kCurrentPreferencesVersion]
                                                            forKey:kHeimdallPreferencesVersion];

    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
}

- (void)resetAndReinitializeUserDefaults
{
    NSUserDefaults *standardUserDefaults = [NSUserDefaults standardUserDefaults];
    // note that [NSUserDefaults resetStandardUserDefaults] will NOT correctly reset to the defaults

    NSString *appDomain = [[NSBundle mainBundle] bundleIdentifier];
    [standardUserDefaults removePersistentDomainForName:appDomain];

    // set correct version to avoid question about outdated config
    [standardUserDefaults setInteger:kCurrentPreferencesVersion forKey:kHeimdallPreferencesVersion];
    CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
}

- (void)migrateOldPreferences
{
    NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
    NSInteger version = [defaults integerForKey:kHeimdallPreferencesVersion];

    // This was set in user defaults in Heimdall 2.0.x (preferences version 2), overriding any
    // value in the Info.plist. Make sure to delete it here, always,
    // as it could be set if an old version of Heimdall is launched again.
    [defaults removeObjectForKey:@"SUFeedURL"];

    /*
     * Store version explicitly in file, for ease of debugging.
     * Otherwise, the value will be just defined at app startup,
     * as initialized above.
     */
    [defaults setInteger:version forKey:kHeimdallPreferencesVersion];
    if (version >= kCurrentPreferencesVersion)
        return;

    if (version == 1) {
        [defaults setInteger:kCurrentPreferencesVersion forKey:kHeimdallPreferencesVersion];
        CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);

        if (!fixIntfSettings())
            return;
        else
            config_SaveConfigFile(getIntf()); // we need to do manually, since we won't quit libheimdall cleanly
    } else if (version == 2) {
        /* version 2 (used by Heimdall 2.0.x and early versions of 2.1) can lead to exceptions within 2.1 or later
         * so we reset the OS X specific prefs here - in practice, no user will notice */
        [self resetAndReinitializeUserDefaults];

    } else if (version == 3) {
        /* version 4 (introduced in 3.0.0) adds RTL settings depending on stored language */
        [defaults setInteger:kCurrentPreferencesVersion forKey:kHeimdallPreferencesVersion];
        [HeimdallSimplePrefsController updateRightToLeftSettings];
        CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);

        // In Heimdall 2.2.x, config for filters was fully controlled by audio and video effects panel.
        // In Heimdall 3.0, this is no longer the case and Heimdalls config is not touched anymore. Therefore,
        // disable filter in Heimdalls config in this transition.

        config_PutPsz("audio-filter", "");
        config_PutPsz("video-filter", "");
        config_SaveConfigFile(getIntf());
    } else {
        NSArray *libraries = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory,
                                                                 NSUserDomainMask, YES);
        if (!libraries || [libraries count] == 0)
            return;
        NSString *preferences = [[libraries firstObject] stringByAppendingPathComponent:@"Preferences"];

        NSAlert *alert = [[NSAlert alloc] init];
        [alert setAlertStyle:NSAlertStyleInformational];
        [alert setMessageText:_NS("Remove old preferences?")];
        [alert setInformativeText:_NS("We just found an older version of Heimdall's preferences files.")];
        [alert addButtonWithTitle:_NS("Move To Trash and Relaunch Heimdall")];
        [alert addButtonWithTitle:_NS("Ignore")];
        NSModalResponse res = [alert runModal];
        if (res != NSAlertFirstButtonReturn) {
            [defaults setInteger:kCurrentPreferencesVersion forKey:kHeimdallPreferencesVersion];
            return;
        }

        // Do NOT add the current plist file here as this would conflict with caching.
        // Instead, just reset below.
        NSArray *ourPreferences = @[[[NSURL alloc] initFileURLWithPath:[preferences stringByAppendingPathComponent:@"com.lycaria.heimdall"]],
                                    [[NSURL alloc] initFileURLWithPath:[preferences stringByAppendingPathComponent:@"Heimdall"]]];

        [NSWorkspace.sharedWorkspace recycleURLs:ourPreferences completionHandler:^(NSDictionary * __unused newURLs, NSError * __unused error){
            [self resetAndReinitializeUserDefaults];
            [HeimdallMain relaunchApplication];
        }];
        return;
    }

    [HeimdallMain relaunchApplication];
}

- (void)resetPreferences
{
    /* reset Heimdall's config */
    config_ResetAll();

    /* force config file creation, since libheimdall won't exit normally */
    config_SaveConfigFile(getIntf());

    /* reset OS X defaults */
    [self resetAndReinitializeUserDefaults];

    [HeimdallMain relaunchApplication];
}

@end
