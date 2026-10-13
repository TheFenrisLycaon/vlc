/*****************************************************************************
 * HeimdallHotkeysController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2003-2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne # lycaria dot org>
 *          David Fuhrmann <dfuhrmann # lycaria dot org>
 *          Derk-Jan Hartman <hartman # lycaria dot org>
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

#import "HeimdallHotkeysController.h"

#import "main/HeimdallMain.h"
#import "extensions/NSString+Helpers.h"
#import "playqueue/HeimdallPlayQueueController.h"
#import "playqueue/HeimdallPlayerController.h"

#import <heimdall_configuration.h>
#import <heimdall_actions.h>
#import <heimdall_plugin.h>
#import <heimdall_modules.h>

@interface HeimdallHotkeysController()
{
    NSArray *_usedHotkeys;
}
@end

@implementation HeimdallHotkeysController

- (instancetype)init
{
    self = [super init];
    if (self) {
        [self updateCurrentlyUsedHotkeys];
    }
    return self;
}

- (void)updateCurrentlyUsedHotkeys
{
    NSMutableArray *mutArray = [[NSMutableArray alloc] init];
    /* Get the main Module */
    module_t *p_main = module_get_main();
    assert(p_main);
    unsigned confsize;
    module_config_t *p_config;

    p_config = module_config_get(p_main, &confsize);

    for (size_t i = 0; i < confsize; i++) {
        module_config_t *p_item = p_config + i;

        if (p_item->i_type == CONFIG_ITEM_KEY
            && strncmp( p_item->psz_name, "global-", 7 ) != 0
            && !EMPTY_STR(p_item->psz_text)) {
            if (p_item->value.psz)
                [mutArray addObject:toNSStr(p_item->value.psz)];
        }
    }
    module_config_free(p_config);

    _usedHotkeys = [[NSArray alloc] initWithArray:mutArray copyItems:YES];
}

- (BOOL)handleVideoOutputKeyDown:(id)anEvent forVideoOutput:(vout_thread_t *)p_vout
{
    HeimdallPlayerController * const playerController = HeimdallMain.sharedInstance.playQueueController.playerController;
    unichar key = 0;
    heimdall_value_t val;
    NSEventModifierFlags i_pressed_modifiers = 0;
    val.i_int = 0;

    i_pressed_modifiers = [anEvent modifierFlags];

    if (i_pressed_modifiers & NSEventModifierFlagShift)
        val.i_int |= KEY_MODIFIER_SHIFT;
    if (i_pressed_modifiers & NSEventModifierFlagControl)
        val.i_int |= KEY_MODIFIER_CTRL;
    if (i_pressed_modifiers & NSEventModifierFlagOption)
        val.i_int |= KEY_MODIFIER_ALT;
    if (i_pressed_modifiers & NSEventModifierFlagCommand)
        val.i_int |= KEY_MODIFIER_COMMAND;

    NSString *characters = [anEvent charactersIgnoringModifiers];
    if ([characters length] > 0) {
        key = [[characters lowercaseString] characterAtIndex: 0];

        if (key) {
            /* Escape should always get you out of fullscreen */
            if (key == (unichar) 0x1b) {
                if (playerController.fullscreen) {
                    [playerController toggleFullscreen];
                }
            }
            /* handle Lion's default key combo for fullscreen-toggle in addition to our own hotkeys */
            else if (key == 'f' && i_pressed_modifiers & NSEventModifierFlagControl && i_pressed_modifiers & NSEventModifierFlagCommand) {
                [playerController toggleFullscreen];
            } else if (p_vout) {
                val.i_int |= (int)CocoaKeyToHeimdall(key);
                var_Set(heimdall_object_instance(p_vout), "key-pressed", val);
            }
            else
                msg_Dbg(getIntf(), "could not send keyevent to Heimdall core");

            return YES;
        }
    }

    return NO;
}

- (BOOL)performKeyEquivalent:(NSEvent *)anEvent
{
    BOOL enforced = NO;
    // these are key events which should be handled by heimdall core, but are attached to a main menu item
    if (![self isEvent:anEvent forKey:"key-vol-up"] &&
        ![self isEvent:anEvent forKey:"key-vol-down"] &&
        ![self isEvent:anEvent forKey:"key-vol-mute"] &&
        ![self isEvent:anEvent forKey:"key-prev"] &&
        ![self isEvent:anEvent forKey:"key-next"] &&
        ![self isEvent:anEvent forKey:"key-jump+short"] &&
        ![self isEvent:anEvent forKey:"key-jump-short"]) {
        /* We indeed want to prioritize some Cocoa key equivalent against libheimdall,
         so we perform the menu equivalent now. */
        if ([[NSApp mainMenu] performKeyEquivalent:anEvent]) {
            return TRUE;
        }
    } else {
        enforced = YES;
    }

    return [self hasDefinedShortcutKey:anEvent force:enforced] || [self keyEvent:anEvent];
}

- (BOOL)isEvent:(NSEvent *)anEvent forKey:(const char *)keyString
{
    char *key = config_GetPsz(keyString);
    unsigned int keyModifiers = HeimdallModifiersToCocoa(key);
    NSString *heimdallKeyString = HeimdallKeyToString(key);
    FREENULL(key);

    NSString *characters = [anEvent charactersIgnoringModifiers];
    if ([characters length] > 0) {
        return [[characters lowercaseString] isEqualToString: heimdallKeyString] &&
        (keyModifiers & NSEventModifierFlagShift)     == ([anEvent modifierFlags] & NSEventModifierFlagShift) &&
        (keyModifiers & NSEventModifierFlagControl)   == ([anEvent modifierFlags] & NSEventModifierFlagControl) &&
        (keyModifiers & NSEventModifierFlagOption) == ([anEvent modifierFlags] & NSEventModifierFlagOption) &&
        (keyModifiers & NSEventModifierFlagCommand)   == ([anEvent modifierFlags] & NSEventModifierFlagCommand);
    }
    return NO;
}

- (BOOL)keyEvent:(NSEvent *)anEvent
{
    BOOL eventHandled = NO;
    NSString * characters = [anEvent charactersIgnoringModifiers];
    if ([characters length] > 0) {
        unichar key = [characters characterAtIndex: 0];

        if (key) {
            HeimdallPlayerController * const playerController = HeimdallMain.sharedInstance.playQueueController.playerController;
            vout_thread_t * const p_vout = playerController.mainVideoOutputThread;
            if (p_vout != NULL) {
                /* Escape */
                if (key == (unichar) 0x1b) {
                    if (var_GetBool(p_vout, "fullscreen")) {
                        [playerController toggleFullscreen];
                        eventHandled = YES;
                    }
                }
                vout_Release(p_vout);
            }
        }
    }
    return eventHandled;
}

- (BOOL)hasDefinedShortcutKey:(NSEvent *)o_event force:(BOOL)enforced
{
    intf_thread_t *p_intf = getIntf();
    if (!p_intf)
        return NO;

    unichar key = 0;
    heimdall_value_t val;
    NSEventModifierFlags i_pressed_modifiers = 0;

    val.i_int = 0;
    i_pressed_modifiers = [o_event modifierFlags];

    if (i_pressed_modifiers & NSEventModifierFlagControl)
        val.i_int |= KEY_MODIFIER_CTRL;

    if (i_pressed_modifiers & NSEventModifierFlagOption)
        val.i_int |= KEY_MODIFIER_ALT;

    if (i_pressed_modifiers & NSEventModifierFlagShift)
        val.i_int |= KEY_MODIFIER_SHIFT;

    if (i_pressed_modifiers & NSEventModifierFlagCommand)
        val.i_int |= KEY_MODIFIER_COMMAND;

    NSString * characters = [o_event charactersIgnoringModifiers];
    if ([characters length] > 0) {
        key = [[characters lowercaseString] characterAtIndex: 0];

        /* handle Lion's default key combo for fullscreen-toggle in addition to our own hotkeys */
        if (key == 'f' && i_pressed_modifiers & NSEventModifierFlagControl && i_pressed_modifiers & NSEventModifierFlagCommand) {
            [HeimdallMain.sharedInstance.playQueueController.playerController toggleFullscreen];
            return YES;
        }

        if (!enforced) {
            switch(key) {
                case NSDeleteCharacter:
                case NSDeleteFunctionKey:
                case NSDeleteCharFunctionKey:
                case NSBackspaceCharacter:
                case NSUpArrowFunctionKey:
                case NSDownArrowFunctionKey:
                case NSEnterCharacter:
                case NSCarriageReturnCharacter:
                    return NO;
            }
        }

        val.i_int |= CocoaKeyToHeimdall(key);

        BOOL b_found_key = NO;
        NSUInteger numberOfUsedHotkeys = [_usedHotkeys count];
        for (NSUInteger i = 0; i < numberOfUsedHotkeys; i++) {
            const char *str = [[_usedHotkeys objectAtIndex:i] UTF8String];
            unsigned int i_keyModifiers = HeimdallModifiersToCocoa((char *)str);

            if ([[characters lowercaseString] isEqualToString:HeimdallKeyToString((char *)str)] &&
                (i_keyModifiers & NSEventModifierFlagShift)     == (i_pressed_modifiers & NSEventModifierFlagShift) &&
                (i_keyModifiers & NSEventModifierFlagControl)   == (i_pressed_modifiers & NSEventModifierFlagControl) &&
                (i_keyModifiers & NSEventModifierFlagOption) == (i_pressed_modifiers & NSEventModifierFlagOption) &&
                (i_keyModifiers & NSEventModifierFlagCommand)   == (i_pressed_modifiers & NSEventModifierFlagCommand)) {
                b_found_key = YES;
                break;
            }
        }

        if (b_found_key) {
            var_SetInteger(heimdall_object_instance(p_intf), "key-pressed", val.i_int);
            return YES;
        }
    }

    return NO;
}

@end
