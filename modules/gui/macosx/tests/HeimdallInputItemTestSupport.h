/****************************************************************************
 * HeimdallInputItemTestSupport.h: Heimdall input item test support declarations
 ****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any later
 * version.
 ****************************************************************************/

#import <Cocoa/Cocoa.h>
#import <heimdall_interface.h>

void HeimdallInputItemTestSetQuickLookImage(NSImage * _Nullable image);
void HeimdallInputItemTestSetWorkspaceImage(NSImage * _Nullable image);
void HeimdallInputItemTestSetArtistName(NSString * _Nullable artistName);
void HeimdallInputItemTestSetInterface(intf_thread_t * _Nullable interfaceThread);
void HeimdallInputItemTestResetAppKitState(void);
BOOL HeimdallInputItemTestDidReveal(void);
NSArray<NSURL *> * _Nonnull HeimdallInputItemTestRevealedURLs(void);
BOOL HeimdallInputItemTestDidReload(void);
