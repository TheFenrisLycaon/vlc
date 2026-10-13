/*****************************************************************************
 * HeimdallInputItem.m: MacOS X interface module
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

#import "HeimdallInputItem.h"

#import "main/HeimdallMain.h"

#import "extensions/NSImage+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"

#import "library/HeimdallLibraryController.h"

#import <heimdall_url.h>
#import <heimdall_preparser.h>

NSString *HeimdallInputItemParsingSucceeded = @"HeimdallInputItemParsingSucceeded";
NSString *HeimdallInputItemParsingFailed = @"HeimdallInputItemParsingFailed";
NSString *HeimdallInputItemSubtreeAdded = @"HeimdallInputItemSubtreeAdded";
NSString * const HeimdallInputItemCommonDataDifferingFlagString = @"<differing>";

@interface HeimdallInputItem()
{
    input_item_parser_id_t *_p_parserID;
}

- (nullable NSURL *)localFileURL;

- (void)parsingEnded:(int)status;
- (void)subTreeAdded:(input_item_node_t *)p_node;

@end

static void cb_parsing_ended(input_item_t *p_item, int status, void *p_data)
{
    HEIMDALL_UNUSED(p_item);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallInputItem *inputItem = (__bridge HeimdallInputItem *)p_data;
        [inputItem parsingEnded:status];
    });
}

static void cb_subtree_added(input_item_t *p_item, input_item_node_t *p_node, void *p_data)
{
    HEIMDALL_UNUSED(p_item);
    dispatch_async(dispatch_get_main_queue(), ^{
        HeimdallInputItem *inputItem = (__bridge HeimdallInputItem *)p_data;
        [inputItem subTreeAdded:p_node];
    });
}

static const struct input_item_parser_cbs_t parserCallbacks =
{
    .on_ended = cb_parsing_ended,
    .on_subtree_added = cb_subtree_added,
};

@implementation HeimdallInputItem

+ (nullable instancetype)inputItemFromURL:(NSURL *)url
{
    const char * const psz_uri = url.absoluteString.UTF8String;
    const char * const psz_name = url.lastPathComponent.stringByDeletingPathExtension.UTF8String;
    input_item_t * const p_input_item = input_item_New(psz_uri, psz_name);
    if (p_input_item == NULL)
        return nil;
    HeimdallInputItem * const inputItem = [[HeimdallInputItem alloc] initWithInputItem:p_input_item];
    input_item_Release(p_input_item);
    return inputItem;
}

- (nullable instancetype)initWithInputItem:(struct input_item_t *)p_inputItem
{
    self = [super init];
    if (self && p_inputItem != NULL) {
        _heimdallInputItem = input_item_Hold(p_inputItem);
    } else {
        return nil;
    }
    return self;
}

- (void)dealloc
{
    if (_p_parserID)
        input_item_parser_id_Release(_p_parserID);
    if (_heimdallInputItem)
        input_item_Release(_heimdallInputItem);
    if (_subTree)
        input_item_node_Delete(_subTree);
}

- (NSString *)stringMetadata:(char *(*)(input_item_t *))getter
{
    char *value = getter(_heimdallInputItem);
    NSString *result = toNSStr(value);
    FREENULL(value);
    return result;
}

- (NSString *)name
{
    return toNSStr(_heimdallInputItem->psz_name);
}
- (void)setName:(NSString *)name
{
    input_item_SetName(_heimdallInputItem, [name UTF8String]);
}

- (NSString *)title
{
    char *psz_title = input_item_GetTitle(_heimdallInputItem);
    if (!psz_title) {
        return self.name;
    }

    NSString *returnValue = toNSStr(psz_title);
    FREENULL(psz_title);
    return returnValue;
}

-(void)setTitle:(NSString *)title
{
    input_item_SetTitle(_heimdallInputItem, [title UTF8String]);
}

- (NSString *)artist
{
    return [self stringMetadata:input_item_GetArtist];
}
- (void)setArtist:(NSString *)artist
{
    input_item_SetArtist(_heimdallInputItem, [artist UTF8String]);
}

- (NSString *)album
{
    return [self stringMetadata:input_item_GetAlbum];
}
- (void)setAlbum:(NSString *)albumName
{
    input_item_SetAlbum(_heimdallInputItem, [albumName UTF8String]);
}

- (NSString *)trackNumber
{
    return [self stringMetadata:input_item_GetTrackNumber];
}
- (void)setTrackNumber:(NSString *)trackNumber
{
    input_item_SetTrackNumber(_heimdallInputItem, [trackNumber UTF8String]);
}

- (NSString *)genre
{
    return [self stringMetadata:input_item_GetGenre];
}
- (void)setGenre:(NSString *)genre
{
    input_item_SetGenre(_heimdallInputItem, [genre UTF8String]);
}

- (NSString *)copyright
{
    return [self stringMetadata:input_item_GetCopyright];
}
- (void)setCopyright:(NSString *)copyright
{
    input_item_SetCopyright(_heimdallInputItem, [copyright UTF8String]);
}

- (NSString *)publisher
{
    return [self stringMetadata:input_item_GetPublisher];
}
- (void)setPublisher:(NSString *)publisher
{
    input_item_SetPublisher(_heimdallInputItem, [publisher UTF8String]);
}

- (NSString *)nowPlaying
{
    return [self stringMetadata:input_item_GetNowPlaying];
}

- (NSString *)language
{
    return [self stringMetadata:input_item_GetLanguage];
}
- (void)setLanguage:(NSString *)language
{
    input_item_SetLanguage(_heimdallInputItem, [language UTF8String]);
}

- (NSString *)date
{
    return [self stringMetadata:input_item_GetDate];
}
- (void)setDate:(NSString *)date
{
    input_item_SetDate(_heimdallInputItem, [date UTF8String]);
}

- (NSString *)contentDescription
{
    return [self stringMetadata:input_item_GetDescription];
}
- (void)setContentDescription:(NSString *)contentDescription
{
    input_item_SetDescription(_heimdallInputItem, [contentDescription UTF8String]);
}

- (NSString *)encodedBy
{
    return [self stringMetadata:input_item_GetEncodedBy];
}

- (NSString *)trackID
{
    return [self stringMetadata:input_item_GetTrackID];
}

- (NSString *)trackTotal
{
    return [self stringMetadata:input_item_GetTrackTotal];
}

- (NSString *)director
{
    return [self stringMetadata:input_item_GetDirector];
}
- (void)setDirector:(NSString *)director
{
    input_item_SetDirector(_heimdallInputItem, [director UTF8String]);
}

- (NSString *)season
{
    return [self stringMetadata:input_item_GetSeason];
}

- (NSString *)episode
{
    return [self stringMetadata:input_item_GetEpisode];
}

- (NSString *)showName
{
    return [self stringMetadata:input_item_GetShowName];
}
- (void)setShowName:(NSString *)showName
{
    input_item_SetShowName(_heimdallInputItem, [showName UTF8String]);
}

- (NSString *)actors
{
    return [self stringMetadata:input_item_GetActors];
}
- (void)setActors:(NSString *)actors
{
    input_item_SetActors(_heimdallInputItem, [actors UTF8String]);
}

- (NSString *)discNumber
{
    return [self stringMetadata:input_item_GetDiscNumber];
}

- (NSString *)totalNumberOfDiscs
{
    return [self stringMetadata:input_item_GetDiscTotal];
}

- (NSString *)MRL
{
    return toNSStr(_heimdallInputItem->psz_uri);
}

- (NSString *)decodedMRL
{
    char *psz_url = heimdall_uri_decode(input_item_GetURI(_heimdallInputItem));
    NSString *returnValue = toNSStr(psz_url);
    FREENULL(psz_url);
    return returnValue;
}

- (NSString*)path
{
    if (!_heimdallInputItem->b_net) {
        char *psz_url = input_item_GetURI(_heimdallInputItem);
        if (!psz_url) {
            return @"";
        }

        char *psz_path = heimdall_uri2path(psz_url);
        NSString *path = toNSStr(psz_path);
        free(psz_url);
        free(psz_path);

        return path;
    }

    return @"";
}

- (NSArray<NSString *> *)extraMetaNames
{
    char **ppsz_names;
    const unsigned i_count = input_item_GetMetaExtraNames(_heimdallInputItem, &ppsz_names);
    if (i_count == 0) {
        return @[];
    }

    NSMutableArray * const names = [NSMutableArray arrayWithCapacity:i_count];
    for (unsigned i = 0; i < i_count; i++) {
        [names addObject:toNSStr(ppsz_names[i])];
        free(ppsz_names[i]);
    }
    free(ppsz_names);

    return [names copy];
}

- (nullable NSString *)extraMetaForKey:(NSString *)key
{
    char * const psz_value = input_item_GetMetaExtra(_heimdallInputItem, [key UTF8String]);
    NSString * const returnValue = toNSStr(psz_value);
    free(psz_value);
    return returnValue;
}

- (heimdall_tick_t)duration
{
    return _heimdallInputItem->i_duration;
}

- (enum input_item_type_e)inputType
{
    return _heimdallInputItem->i_type;
}

- (NSURL *)artworkURL
{
    char *p_artworkURL = input_item_GetArtworkURL(_heimdallInputItem);
    if (p_artworkURL) {
        NSString *artworkURLString = toNSStr(p_artworkURL);
        FREENULL(p_artworkURL);
        return [NSURL URLWithString:artworkURLString];
    }
    return nil;
}

- (void)setArtworkURL:(NSURL *)artworkURL
{

    if (artworkURL != nil) {
        input_item_SetArtworkURL(_heimdallInputItem, artworkURL.absoluteString.UTF8String);
    } else {
        input_item_SetArtworkURL(_heimdallInputItem, NULL);
    }
}

- (nullable NSString *)radioCountryCodeForFlagArtwork
{
    NSURL * const url = [NSURL URLWithString:self.MRL];
    if (![url.scheme isEqualToString:@"radio"]) {
        return nil;
    }

    NSString * const countryCode = url.host.uppercaseString;
    return flagEmojiStringForCountryCode(countryCode) ? countryCode : nil;
}

- (void)parseInputItem
{
    const struct input_item_parser_cfg cfg = {
        .cbs = &parserCallbacks,
        .cbs_data = (__bridge void *) self,
    };

    if (_p_parserID)
        input_item_parser_id_Release(_p_parserID);
    _p_parserID = input_item_Parse(HEIMDALL_OBJECT(getIntf()), _heimdallInputItem, &cfg);
}

- (void)cancelParsing
{
    if (_p_parserID) {
        input_item_parser_id_Interrupt(_p_parserID);
    }
}

- (void)parsingEnded:(int)status
{
    NSNotificationCenter *notificationCenter = NSNotificationCenter.defaultCenter;
    if (status) {
        [notificationCenter postNotificationName:HeimdallInputItemParsingSucceeded object:self];
    } else {
        [notificationCenter postNotificationName:HeimdallInputItemParsingFailed object:self];
    }
    if (_p_parserID) {
        input_item_parser_id_Release(_p_parserID);
        _p_parserID = NULL;
    }
}

- (BOOL)preparsed
{
    return input_item_IsPreparsed(_heimdallInputItem);
}

- (BOOL)isStream
{
    return (BOOL)_heimdallInputItem->b_net;
}

- (void)subTreeAdded:(input_item_node_t *)p_node
{
    if (_subTree) {
        input_item_node_Delete(_subTree);
    }
    _subTree = p_node;
    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallInputItemSubtreeAdded object:self];
}

- (int)writeMetadataToFile
{
    return input_item_WriteMeta(HEIMDALL_OBJECT(getIntf()), _heimdallInputItem);
}

- (void)thumbnailWithSize:(NSSize)size completionHandler:(void(^)(NSImage * image))completionHandler
{
    if (self.isStream) {
        completionHandler(nil);
        return;
    }

    char * const psz_url = input_item_GetURI(_heimdallInputItem);
    if (psz_url == NULL) {
        completionHandler(nil);
        return;
    }

    char * const psz_path = heimdall_uri2path(psz_url);
    free(psz_url);
    if (psz_path == NULL) {
        completionHandler(nil);
        return;
    }

    NSString * const path = toNSStr(psz_path);
    free(psz_path);

    [NSImage quickLookPreviewForLocalPath:path 
                                 withSize:size
                        completionHandler:^(NSImage * image) {
        if (image) {
            completionHandler(image);
            return;
        }

        NSImage * const workspaceImage = [NSWorkspace.sharedWorkspace iconForFile:path];
        if (workspaceImage) {
            workspaceImage.size = size;
            completionHandler(workspaceImage);
            return;
        }

        completionHandler(nil);
    }];
}

- (nullable NSURL *)localFileURL
{
    if (self.isStream) {
        return nil;
    }

    NSString * const path = self.path;
    if (path.length == 0) {
        return nil;
    }

    return [NSURL fileURLWithPath:path];
}

- (void)moveToTrash
{
    NSURL * const pathUrl = self.localFileURL;
    if (pathUrl == nil) {
        return;
    }

    [NSFileManager.defaultManager trashItemAtURL:pathUrl
                                resultingItemURL:nil
                                           error:nil];
    
    HeimdallLibraryController * const libraryController = HeimdallMain.sharedInstance.libraryController;
    [libraryController reloadMediaLibraryFoldersForInputItems:@[self]];
}

- (void)revealInFinder
{
    NSURL * const pathUrl = self.localFileURL;
    if (pathUrl == nil) {
        return;
    }

    [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:@[pathUrl]];
}

- (nullable NSArray<NSString *> *)options
{
    const int i_options = _heimdallInputItem->i_options;
    NSMutableArray * const options = [NSMutableArray arrayWithCapacity:i_options];
    for (int i = 0; i < i_options; ++i) {
        const char * const psz_option = _heimdallInputItem->ppsz_options[i];
        NSString * const option = NSTR(psz_option);
        [options addObject:option];
    }
    return options.copy;
}

- (nullable NSArray<NSString *> *)finderTags
{
    NSString * const mrl = self.MRL;
    if (![mrl hasPrefix:@"file://"]) {
        return nil;
    }
    NSURL * const url = [NSURL URLWithString:mrl];
    if (!url) {
        return nil;
    }
    NSArray<NSString *> *tags = nil;
    [url getResourceValue:&tags forKey:NSURLTagNamesKey error:nil];
    return tags;
}

@end


NSDictionary<NSString *, id> *commonInputItemData(NSArray<HeimdallInputItem *> * const inputItems)
{
    if (inputItems.count == 0) {
        return @{};
    }

    HeimdallInputItem * const firstInputItem = inputItems.firstObject;

    if (inputItems.count == 1) {
        return @{@"inputItem": firstInputItem};
    }

    NSMutableDictionary<NSString *, id> *const commonData = [[NSMutableDictionary alloc] init];

#define PERFORM_ACTION_PER_INPUTITEM_NSSTRING_PROP(action)                                  \
action(MRL);                                                                                \
action(decodedMRL);                                                                         \
action(title);                                                                              \
action(artist);                                                                             \
action(album);                                                                              \
action(trackNumber);                                                                        \
action(trackTotal);                                                                         \
action(genre);                                                                              \
action(date);                                                                               \
action(episode);                                                                            \
action(actors);                                                                             \
action(director);                                                                           \
action(showName);                                                                           \
action(copyright);                                                                          \
action(publisher);                                                                          \
action(nowPlaying);                                                                         \
action(language);                                                                           \
action(contentDescription);                                                                 \
action(encodedBy);

#define PERFORM_ACTION_PER_INPUTITEM_PROP(action)                                           \
PERFORM_ACTION_PER_INPUTITEM_NSSTRING_PROP(action)                                          \
action(artworkURL);

#define CREATE_DIFFER_BOOL(prop)                                                            \
BOOL differing_##prop = NO;

#define CREATE_PROP_VAR(prop)                                                               \
NSString * const firstItem_##prop = firstInputItem.prop;

#define UPDATE_IF_DIFFERING_BOOL(prop)                                                          \
differing_##prop = differing_##prop || ![NSString equalOrBothNil:inputItem.prop                \
                                                                   to:firstItem_##prop];

#define ADD_PROP_TO_DICT(prop)                                                              \
NSString * firstItemValue_##prop = firstItem_##prop == nil ? @"" : firstItem_##prop;        \
NSString * const value_##prop =                                                             \
    differing_##prop ? HeimdallInputItemCommonDataDifferingFlagString : firstItemValue_##prop;   \
[commonData setObject:value_##prop forKey:[NSString stringWithUTF8String:#prop]];           \

    PERFORM_ACTION_PER_INPUTITEM_PROP(CREATE_DIFFER_BOOL);
    PERFORM_ACTION_PER_INPUTITEM_NSSTRING_PROP(CREATE_PROP_VAR);
    // Since artworkURL is a URL, we have to handle it differently
    NSString * const firstItem_artworkURL = firstInputItem.artworkURL.absoluteString;

    // Skip first item
    for (uint i = 1; i < inputItems.count; ++i) {
        HeimdallInputItem * const inputItem = inputItems[i];

        PERFORM_ACTION_PER_INPUTITEM_NSSTRING_PROP(UPDATE_IF_DIFFERING_BOOL);
        NSString * const inputItem_artworkURL = inputItem.artworkURL.absoluteString;
        differing_artworkURL |= ![NSString equalOrBothNil:inputItem_artworkURL
                                                          to:firstItem_artworkURL];
    }

    PERFORM_ACTION_PER_INPUTITEM_PROP(ADD_PROP_TO_DICT);

#undef PERFORM_ACTION_PER_INPUTITEM_PROP
#undef CREATE_DIFFER_BOOL
#undef CREATE_PROP_VAR
#undef SET_IF_DIFFERING
#undef ADD_PROP_TO_DICT_IF_DIFFERING

    return [commonData copy];
}
