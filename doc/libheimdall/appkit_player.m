/* appkit_player.m: AppKit libheimdall integration sample
 *
 * Copyright (C) 2022 Videolabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Marvin Scholz <epirat07 at gmail dot com>
 *          Alexandre Janniaux <ajanni@videolabs.io>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
    To compile the sample:

    clang doc/libheimdall/appkit_player.m -fobjc-arc -o appkit_player -I include/ -Wl,-framework,AppKit -I include -L build-macosx/lib/.libs -lheimdall
    install_name_tool -add_rpath build-macosx/lib/.libs appkit_player
    install_name_tool -add_rpath build-macosx/src/.libs appkit_player

    HEIMDALL_PLUGIN_PATH=$(pwd)/build-macosx/modules/ ./appkit_player $MRL
*/

#import <Cocoa/Cocoa.h>
#import <heimdall/heimdall.h>

#define ARRAY_SIZE(A) (sizeof (A) / sizeof (A[0]))

@interface AppDelegate : NSObject <NSApplicationDelegate>
{
    libheimdall_instance_t *instance;
    libheimdall_media_player_t *player;
    libheimdall_media_t *media;
}
@property NSWindow *window;
@property NSView *view;
@end

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
     // Insert code here to initialize your application

     NSWindowStyleMask windowMask =
         NSWindowStyleMaskTitled | NSWindowStyleMaskMiniaturizable |
         NSWindowStyleMaskResizable | NSWindowStyleMaskClosable;
     _window = [[NSWindow alloc]
         initWithContentRect: NSMakeRect(300, 300, 200, 100)
                   styleMask: windowMask
                     backing: NSBackingStoreBuffered
                       defer: NO];
    [_window setTitle: @"LibHeimdall AppKit sample"];
    [_window makeKeyAndOrderFront:nil];

    _view = [[NSView alloc] initWithFrame:NSMakeRect(300, 300, 200, 100)];
    [_window setContentView: _view];

    const char *const heimdall_args[] = { "-vv", "--aout=dummy" };
    instance = libheimdall_new(ARRAY_SIZE(heimdall_args), heimdall_args);
    NSAssert(instance != NULL, @"Failed to allocate libheimdall instance");

    player = libheimdall_media_player_new(instance, NULL, NULL);
    NSAssert(player != NULL, @"Failed to allocate player instance");

    NSArray *args = [[NSProcessInfo processInfo] arguments];
    NSString *location = [args objectAtIndex:1];
    media = libheimdall_media_new_location([location UTF8String]);
    libheimdall_media_player_set_media(player, media);

    libheimdall_media_player_set_nsobject(player, (__bridge void*)_view);
    libheimdall_media_player_play(player);
}
@end

int main(int argc, char *argv[])
{
    if (argc < 2) return -1;
    AppDelegate *delegate = [[AppDelegate alloc] init];
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp setDelegate: delegate];
    [NSApp activateIgnoringOtherApps: YES];
    return NSApplicationMain(argc, (const char **)argv);
}
