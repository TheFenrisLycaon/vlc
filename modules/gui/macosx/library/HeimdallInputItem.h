/*****************************************************************************
 * HeimdallInputItem.h: MacOS X interface module
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

#import "library/HeimdallLibraryDataTypes.h"

#import <heimdall_common.h>
#import <heimdall_input_item.h>
#import <heimdall_tick.h>

NS_ASSUME_NONNULL_BEGIN

extern NSString *HeimdallInputItemParsingSucceeded;
extern NSString *HeimdallInputItemParsingFailed;
extern NSString *HeimdallInputItemSubtreeAdded;
extern NSString *HeimdallInputItemPreparsingSkipped;
extern NSString *HeimdallInputItemPreparsingFailed;
extern NSString *HeimdallInputItemPreparsingTimeOut;
extern NSString *HeimdallInputItemPreparsingSucceeded;
extern NSString * const HeimdallInputItemCommonDataDifferingFlagString;

@interface HeimdallInputItem : NSObject<HeimdallLocallyManipulableItemProtocol>

+ (nullable instancetype)inputItemFromURL:(NSURL *)url;
- (nullable instancetype)initWithInputItem:(struct input_item_t *)p_inputItem;

@property (readonly) struct input_item_t *heimdallInputItem;
@property (readonly) NSString *MRL;
@property (readonly) NSString *decodedMRL;
@property (readonly) NSString *path;
@property (readwrite) NSString *name;
@property (readwrite) NSString *title;
@property (readwrite) NSString *artist;
@property (readwrite) NSString *album;
@property (readwrite) NSString *trackNumber;
@property (readwrite) NSString *genre;
@property (readwrite) NSString *copyright;
@property (readwrite) NSString *publisher;
@property (readonly) NSString *nowPlaying;
@property (readwrite) NSString *language;
@property (readwrite) NSString *date;
@property (readwrite) NSString *contentDescription;
@property (readonly) NSString *encodedBy;
@property (readonly) NSString *trackID;
@property (readonly) NSString *trackTotal;
@property (readwrite) NSString *director;
@property (readonly) NSString *season;
@property (readonly) NSString *episode;
@property (readwrite) NSString *showName;
@property (readwrite) NSString *actors;
@property (readonly) NSString *discNumber;
@property (readonly) NSString *totalNumberOfDiscs;
@property (readonly) heimdall_tick_t duration;
@property (readonly) enum input_item_type_e inputType;
@property (readonly) struct input_item_node_t *subTree;
@property (readwrite) NSURL *artworkURL;
@property (readonly) BOOL preparsed;
@property (readonly) BOOL isStream;
@property (readonly, nullable) NSArray<NSString *> *options;
@property (readonly) NSArray<NSString *> *extraMetaNames;
@property (readonly, nullable) NSArray<NSString *> *finderTags;

- (nullable NSString *)extraMetaForKey:(NSString *)key;
- (nullable NSString *)radioCountryCodeForFlagArtwork;

- (void)parseInputItem;
- (void)cancelParsing;

- (int)writeMetadataToFile;

- (void)thumbnailWithSize:(NSSize)size 
        completionHandler:(void (^)(NSImage * _Nullable))completionHandler;

@end

NSDictionary<NSString *, id> *commonInputItemData(NSArray<HeimdallInputItem*> * const inputItems);

NS_ASSUME_NONNULL_END
