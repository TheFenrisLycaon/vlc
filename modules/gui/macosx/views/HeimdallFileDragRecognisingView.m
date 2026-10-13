/*****************************************************************************
 * HeimdallFileDragRecognisingView.m
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

#import "HeimdallFileDragRecognisingView.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "main/HeimdallMain.h"
#import "playqueue/HeimdallPlayQueueController.h"
#import "windows/HeimdallOpenInputMetadata.h"

@implementation HeimdallFileDragRecognisingView

+ (BOOL)handlePasteboardFromDragSessionAsPlayQueueItems:(NSPasteboard *)pasteboard
{
    NSMutableArray<HeimdallMediaLibraryMediaItem *> * const allMediaItems = [NSMutableArray array];
    for (NSPasteboardItem * const pboardItem in pasteboard.pasteboardItems) {
        NSData *itemData = [pboardItem dataForType:HeimdallMediaLibraryMediaItemPasteboardType];
        if (!itemData) {
            itemData = [pboardItem dataForType:HeimdallMediaLibraryMediaItemUTI];
        }
        if (!itemData) {
            continue;
        }

        NSArray<HeimdallMediaLibraryMediaItem *> * const items = [HeimdallMediaLibraryMediaItem mediaItemsFromPasteboardData:itemData];
        if (items) {
            [allMediaItems addObjectsFromArray:items];
        }
    }

    if (allMediaItems.count > 0) {
        for (HeimdallMediaLibraryMediaItem * const mediaItem in allMediaItems) {
            [HeimdallMain.sharedInstance.playQueueController addInputItem:mediaItem.inputItem.heimdallInputItem
                                                          atPosition:-1
                                                       startPlayback:NO];
        }
        return YES;
    }

    NSMutableArray<HeimdallOpenInputMetadata *> * const URLMetadataArray = [NSMutableArray array];
    for (NSPasteboardItem * const pboardItem in pasteboard.pasteboardItems) {
        NSString * const URLString = [pboardItem stringForType:NSPasteboardTypeFileURL] ?:
                                     [pboardItem stringForType:NSPasteboardTypeURL] ?:
                                     [pboardItem stringForType:NSPasteboardTypeString];
        if (URLString.length <= 0) {
            continue;
        }

        NSURL * const droppedURL = [NSURL URLWithString:URLString];
        HeimdallOpenInputMetadata *inputMetadata = nil;
        if (droppedURL.isFileURL) {
            NSURL * const filePathURL = droppedURL.filePathURL;
            NSString * const path = filePathURL.path;
            if (path.length > 0) {
                inputMetadata = [HeimdallOpenInputMetadata inputMetaWithPath:path];
            }
        } else if (droppedURL != nil && droppedURL.scheme.length > 0) {
            inputMetadata = [[HeimdallOpenInputMetadata alloc] init];
            inputMetadata.MRLString = droppedURL.absoluteString;
        } else if (URLString.length > 0 && [URLString isAbsolutePath]) {
            inputMetadata = [HeimdallOpenInputMetadata inputMetaWithPath:URLString];
        }

        if (inputMetadata == nil) {
            continue;
        }

        [URLMetadataArray addObject:inputMetadata];
    }

    if (URLMetadataArray.count > 0) {
        [HeimdallMain.sharedInstance.playQueueController addPlayQueueItems:URLMetadataArray];
        return YES;
    }

    const id propertyList = [pasteboard propertyListForType:NSFilenamesPboardType];
    if (propertyList == nil) {
        return NO;
    }

    NSArray * const values = propertyList;
    const NSUInteger valueCount = values.count;
    if (valueCount <= 0) {
        return NO;
    }

    NSMutableArray * const metadataArray = [NSMutableArray arrayWithCapacity:valueCount];

    for (NSString * const filepath in values) {
        HeimdallOpenInputMetadata * const inputMetadata = [HeimdallOpenInputMetadata inputMetaWithPath:filepath];
        if (inputMetadata != nil) {
            [metadataArray addObject:inputMetadata];
        }
    }

    [HeimdallMain.sharedInstance.playQueueController addPlayQueueItems:metadataArray];
    return YES;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        [self setupDragRecognition];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (self) {
        [self setupDragRecognition];
    }
    return self;
}

- (instancetype)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    if (self) {
        [self setupDragRecognition];
    }
    return self;
}

- (void)dealloc
{
    [self unregisterDraggedTypes];
}

- (void)awakeFromNib
{
    [self setupDragRecognition];
}

- (void)setupDragRecognition
{
    [self registerForDraggedTypes:@[
        HeimdallMediaLibraryMediaItemPasteboardType,
        HeimdallMediaLibraryMediaItemUTI,
        NSPasteboardTypeURL,
        NSPasteboardTypeFileURL,
        NSPasteboardTypeString,
        NSFilenamesPboardType
    ]];
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
    const NSDragOperation sourceOperationMask = [sender draggingSourceOperationMask];
    if ((sourceOperationMask & NSDragOperationCopy) == NSDragOperationCopy ||
        (sourceOperationMask & NSDragOperationGeneric) == NSDragOperationGeneric) {
        return NSDragOperationCopy;
    }
    return NSDragOperationNone;
}

- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)sender
{
    return YES;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender
{
    NSPasteboard * const pasteboard = [sender draggingPasteboard];
    if (pasteboard == nil) {
        return NO;
    }

    [self setNeedsDisplay:YES];
    if (self.dropTarget) {
        return [self.dropTarget handlePasteBoardFromDragSession:pasteboard];
    }
    return NO;
}

@end
