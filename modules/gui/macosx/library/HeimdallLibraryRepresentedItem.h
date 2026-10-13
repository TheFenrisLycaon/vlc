/*****************************************************************************
 * HeimdallLibraryRepresentedItem.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2023 VLC authors and VideoLAN
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

#import <Cocoa/Cocoa.h>

#import "library/HeimdallLibraryDataTypes.h"

NS_ASSUME_NONNULL_BEGIN

@class HeimdallMediaLibraryMediaItem;
@protocol HeimdallMediaLibraryItemProtocol;

@interface HeimdallLibraryRepresentedItem : NSObject<HeimdallLocallyManipulableItemProtocol>

@property (readonly) id<HeimdallMediaLibraryItemProtocol> item;
// Parent media array is lazy loaded
@property (readonly) NSArray<HeimdallMediaLibraryMediaItem *> *parentMediaArray;
// If unknown, will always default to individual play mode
@property (readonly) HeimdallMediaLibraryParentGroupType parentType;
// Optional explicit parent (e.g. the containing playlist)
@property (readonly, nullable) id<HeimdallMediaLibraryItemProtocol> parentItem;
@property (readonly) NSInteger itemIndexInParent;
// Required for playlists where libraryID lookup collapses duplicate items
@property (readonly) NSInteger positionInParent;

- (instancetype)initWithItem:(const id<HeimdallMediaLibraryItemProtocol>)item
                  parentType:(const HeimdallMediaLibraryParentGroupType)parentType;
- (instancetype)initWithItem:(const id<HeimdallMediaLibraryItemProtocol>)item
                  parentType:(const HeimdallMediaLibraryParentGroupType)parentType
                  parentItem:(nullable const id<HeimdallMediaLibraryItemProtocol>)parentItem
            positionInParent:(NSInteger)positionInParent;

- (void)play;
- (void)queue;
- (void)revealInFinder;
- (void)moveToTrash;

@end

NS_ASSUME_NONNULL_END
