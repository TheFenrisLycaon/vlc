/*****************************************************************************
 * HeimdallMediaSource.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne # lycaria -dot- org>
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

#import <Foundation/Foundation.h>

#import <heimdall_media_source.h>
#import <heimdall_preparser.h>

#import "HeimdallMediaSourceNodeObservation.h"

@class HeimdallInputItem;
@class HeimdallInputNode;

NS_ASSUME_NONNULL_BEGIN

extern NSString *HeimdallMediaSourceChildrenReset;
extern NSString *HeimdallMediaSourceChildrenAdded;
extern NSString *HeimdallMediaSourceChildrenRemoved;
extern NSString *HeimdallMediaSourcePreparsingStarted;
extern NSString *HeimdallMediaSourcePreparsingEnded;
extern NSString * const HeimdallMediaSourcePreparseInputItemKey;
extern NSString * const HeimdallMediaSourcePreparseStatusKey;

@interface HeimdallMediaSource : NSObject

- (instancetype)initForLocalDevices:(heimdall_preparser_t *)p_preparser;
- (instancetype)initWithMediaSource:(heimdall_media_source_t *)p_mediaSource
                  andPreparser:(heimdall_preparser_t *)p_preparser
                        forCategory:(enum services_discovery_category_e)category;
- (instancetype)initMyFoldersMediaSourceWithPreparser:(heimdall_preparser_t *)p_preparser;
- (instancetype)initWithFolderMrl:(NSString *)mrl
                     andPreparser:(heimdall_preparser_t *)p_preparser;

- (nullable NSError *)preparseInputNodeWithinTree:(HeimdallInputNode *)inputNode;
/// Returns nil without an error when this media source cannot count children directly.
- (nullable NSNumber *)childCountForInputNode:(HeimdallInputNode *)inputNode
                                        error:(NSError * _Nullable * _Nullable)error;
- (void)clearChildNodesForNode:(input_item_node_t*)inputNode;
- (nullable NSError *)generateChildNodesForDirectoryNode:(HeimdallInputNode *)directoryInputNode
                                                 withUrl:(NSURL *)directoryUrl;
- (nullable id<HeimdallMediaSourceNodeObservation>)observeInputNode:(HeimdallInputNode *)inputNode
                                                      onChange:(void (^)(HeimdallMediaSourceNodeChange change))changeHandler;

@property (nonatomic, readonly) NSString *mediaSourceDescription;
@property (nonatomic, readonly) HeimdallInputNode *rootNode;
@property (nonatomic, readonly) enum services_discovery_category_e category;
@property (readwrite) void (^willStartGeneratingChildNodesForNodeHandler)(input_item_node_t *);
@property (readwrite) void (^didFinishGeneratingChildNodesForNodeHandler)(input_item_node_t *);

@end

NS_ASSUME_NONNULL_END
