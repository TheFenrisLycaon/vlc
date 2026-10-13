/****************************************************************************
 * HeimdallInputItemTestSupport.m: Heimdall input item test-only application stubs
 ****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Claudio Cambra <developer@claudiocambra.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any later
 * version.
 *****************************************************************************/

#import <Cocoa/Cocoa.h>
#import <heimdall_interface.h>

#import "HeimdallInputItemTestSupport.h"

const char heimdall_module_name[] = "HeimdallMacOSXTests";

static NSImage *sQuickLookImage;
static NSImage *sWorkspaceImage;
static NSString *sArtistName;
static BOOL sDidReveal;
static NSMutableArray<NSURL *> *sRevealedURLs;
static BOOL sDidReload;
static intf_thread_t *sInterfaceThread;

@interface HeimdallLibraryController : NSObject
@end

@interface HeimdallMain : NSObject
+ (instancetype)sharedInstance;
@end

@interface HeimdallLibraryNameCache : NSObject
+ (instancetype)sharedInstance;
- (NSString *)artistNameForID:(int64_t)artistID;
@end

@implementation HeimdallLibraryController

- (void)reloadMediaLibraryFoldersForInputItems:(NSArray * __unused)inputItems
{
    sDidReload = YES;
}

@end

@implementation HeimdallMain

+ (instancetype)sharedInstance
{
    static HeimdallMain *main;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        main = [[HeimdallMain alloc] init];
    });
    return main;
}

- (HeimdallLibraryController *)libraryController
{
    static HeimdallLibraryController *libraryController;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        libraryController = [[HeimdallLibraryController alloc] init];
    });
    return libraryController;
}

@end

@implementation HeimdallLibraryNameCache

+ (instancetype)sharedInstance
{
    static HeimdallLibraryNameCache *cache;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        cache = [[HeimdallLibraryNameCache alloc] init];
    });
    return cache;
}

- (NSString *)artistNameForID:(int64_t __unused)artistID
{
    return sArtistName;
}

@end

intf_thread_t *getIntf(void)
{
    return sInterfaceThread;
}

void HeimdallInputItemTestSetQuickLookImage(NSImage *image)
{
    sQuickLookImage = image;
}

void HeimdallInputItemTestSetWorkspaceImage(NSImage *image)
{
    sWorkspaceImage = image;
}

void HeimdallInputItemTestSetArtistName(NSString *artistName)
{
    sArtistName = [artistName copy];
}

void HeimdallInputItemTestSetInterface(intf_thread_t *interfaceThread)
{
    sInterfaceThread = interfaceThread;
}

void HeimdallInputItemTestResetAppKitState(void)
{
    sQuickLookImage = nil;
    sWorkspaceImage = nil;
    sArtistName = nil;
    sDidReveal = NO;
    sRevealedURLs = NSMutableArray.array;
    sDidReload = NO;
}

BOOL HeimdallInputItemTestDidReveal(void)
{
    return sDidReveal;
}

NSArray<NSURL *> *HeimdallInputItemTestRevealedURLs(void)
{
    return [sRevealedURLs copy] ?: @[];
}

BOOL HeimdallInputItemTestDidReload(void)
{
    return sDidReload;
}

@interface NSImage (HeimdallInputItemTestSupport)
+ (void)quickLookPreviewForLocalPath:(NSString *)path
                             withSize:(NSSize)size
                    completionHandler:(void (^)(NSImage *image))completionHandler;
@end

@interface NSWorkspace (HeimdallInputItemTestSupport)
- (NSImage *)iconForFile:(NSString *)fullPath;
- (void)activateFileViewerSelectingURLs:(NSArray<NSURL *> *)fileURLs;
@end

@implementation NSWorkspace (HeimdallInputItemTestSupport)

- (NSImage *)iconForFile:(NSString * __unused)fullPath
{
    return sWorkspaceImage;
}

- (void)activateFileViewerSelectingURLs:(NSArray<NSURL *> *)fileURLs
{
    sDidReveal = YES;
    [sRevealedURLs addObjectsFromArray:fileURLs];
}

@end

@implementation NSImage (HeimdallInputItemTestSupport)

+ (void)quickLookPreviewForLocalPath:(NSString * __unused)path
                             withSize:(NSSize __unused)size
                    completionHandler:(void (^)(NSImage *image))completionHandler
{
    completionHandler(sQuickLookImage);
}

@end
