/*****************************************************************************
* HeimdallLibraryImageCache.m: MacOS X interface module
*****************************************************************************
* Copyright (C) 2020-2026 VLC authors and VideoLAN
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

#import "HeimdallLibraryImageCache.h"

#import "extensions/NSImage+HeimdallAdditions.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallThumbnailRequest.h"

#import "main/HeimdallMain.h"

#import "playqueue/HeimdallPlayQueueItem.h"

#import <ImageIO/ImageIO.h>

#import <heimdall_configuration.h>
#import <heimdall_hash.h>
#import <heimdall_preparser.h>
#import <heimdall_strings.h>

static const NSTimeInterval kHeimdallRemoteArtworkRequestTimeout = 15;
static const NSTimeInterval kHeimdallRemoteArtworkResourceTimeout = 30;
static const NSInteger kHeimdallRemoteArtworkMaximumConnectionsPerHost = 4;

NSUInteger kHeimdallMaximumLibraryImageCacheSize = 500;
/* 256 MB cost limit based on estimated pixel data size per image */
static const NSUInteger kHeimdallLibraryImageCacheCostLimit = 256 * 1024 * 1024;
static const NSTimeInterval kHeimdallThumbnailCacheMaximumAge = 30 * 24 * 60 * 60;
uint32_t kHeimdallDesiredThumbnailWidth = 512;
uint32_t kHeimdallDesiredThumbnailHeight = 512;
float kHeimdallDefaultThumbnailPosition = .15;
const NSUInteger kHeimdallCompositeImageDefaultCompositedGridItemCount = 4;

@interface HeimdallLibraryImageCache()
{
    NSCache *_imageCache;
    NSImage *_noArtImage;
    heimdall_medialibrary_t *_p_libraryInstance;
    NSString *_thumbnailCacheDirectory;
    NSMutableDictionary<NSString *, HeimdallThumbnailRequest *> *_pendingThumbnailRequests;
    NSMutableDictionary<NSString *, NSMutableArray<void (^)(const NSImage *)> *> *_pendingRemoteArtworkRequests;
    NSURLSession *_remoteArtworkSession;
}

- (void)thumbnailRequest:(HeimdallThumbnailRequest *)request
     didFinishWithStatus:(int)status
                 results:(const bool *)results
             resultCount:(size_t)resultCount;

@end

static void heimdallThumbnailerToFilesOnEnded(heimdall_preparser_req *req,
                                         int status,
                                         const bool *results,
                                         size_t resultCount,
                                         void *data)
{
    @autoreleasepool {
        HeimdallThumbnailRequest * const request = (__bridge HeimdallThumbnailRequest *)data;
        [request.imageCache thumbnailRequest:request
                         didFinishWithStatus:status
                                     results:results
                                 resultCount:resultCount];
    }
    heimdall_preparser_req_Release(req);
}

static NSString *thumbnailHashForString(NSString *string)
{
    NSData * const data = [string dataUsingEncoding:NSUTF8StringEncoding];
    char hash[HEIMDALL_HASH_MD5_DIGEST_HEX_SIZE];
    heimdall_hash_md5_t md5;
    heimdall_hash_md5_Init(&md5);
    heimdall_hash_md5_Update(&md5, data.bytes, data.length);
    heimdall_hash_FinishHex(&md5, hash);
    return [NSString stringWithUTF8String:hash];
}

@implementation HeimdallLibraryImageCache

+ (NSImage *)downsampledImageFromSource:(CGImageSourceRef)imageSource
                           maxPixelSize:(uint32_t)maxPixelSize
{
    NSDictionary * const downsampleOptions = @{
        (NSString *)kCGImageSourceCreateThumbnailFromImageAlways : @YES,
        (NSString *)kCGImageSourceCreateThumbnailWithTransform : @YES,
        (NSString *)kCGImageSourceThumbnailMaxPixelSize : @(maxPixelSize),
        (NSString *)kCGImageSourceShouldCacheImmediately : @YES,
    };

    CGImageRef const downsampledImage =
        CGImageSourceCreateThumbnailAtIndex(imageSource, 0, (__bridge CFDictionaryRef)downsampleOptions);
    if (!downsampledImage) {
        return nil;
    }

    NSImage * const image = [[NSImage alloc] initWithCGImage:downsampledImage
                                                        size:NSZeroSize];
    CGImageRelease(downsampledImage);
    return image;
}

+ (NSImage *)downsampledImageFromURL:(NSURL *)url maxPixelSize:(uint32_t)maxPixelSize
{
    CGImageSourceRef const imageSource = CGImageSourceCreateWithURL((__bridge CFURLRef)url, NULL);
    if (!imageSource) {
        return nil;
    }
    NSImage * const image = [self downsampledImageFromSource:imageSource maxPixelSize:maxPixelSize];
    CFRelease(imageSource);
    return image;
}

+ (BOOL)artworkURLRequiresRemoteFetch:(NSURL *)url
{
    return [url.scheme isEqualToString:@"http"] ||
           [url.scheme isEqualToString:@"https"];
}

+ (BOOL)remoteArtworkResponseCanContainImage:(NSURLResponse *)response
{
    if ([response isKindOfClass:NSHTTPURLResponse.class]) {
        NSHTTPURLResponse * const httpResponse = (NSHTTPURLResponse *)response;
        if (httpResponse.statusCode < 200 || httpResponse.statusCode >= 300) {
            return NO;
        }
    }

    NSString * const MIMEType = response.MIMEType.lowercaseString;
    if (MIMEType.length > 0 &&
        ![MIMEType hasPrefix:@"image/"] &&
        ![MIMEType isEqualToString:@"application/octet-stream"]) {
        return NO;
    }

    return YES;
}

+ (NSUInteger)costForImage:(NSImage *)image
{
    NSBitmapImageRep *bitmapRep = nil;
    for (NSImageRep *rep in image.representations) {
        if ([rep isKindOfClass:[NSBitmapImageRep class]]) {
            bitmapRep = (NSBitmapImageRep *)rep;
            break;
        }
    }
    if (bitmapRep) {
        return bitmapRep.pixelsWide * bitmapRep.pixelsHigh * bitmapRep.bitsPerPixel / 8;
    }
    return (NSUInteger)(image.size.width * image.size.height * 4);
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        _imageCache = [[NSCache alloc] init];
        _imageCache.countLimit = kHeimdallMaximumLibraryImageCacheSize;
        _imageCache.totalCostLimit = kHeimdallLibraryImageCacheCostLimit;
        _noArtImage = NSImage.HeimdallNoArtImage;
        _pendingThumbnailRequests = NSMutableDictionary.dictionary;
        _pendingRemoteArtworkRequests = NSMutableDictionary.dictionary;

        NSURLSessionConfiguration * const remoteArtworkSessionConfiguration =
            NSURLSessionConfiguration.ephemeralSessionConfiguration;
        remoteArtworkSessionConfiguration.timeoutIntervalForRequest =
            kHeimdallRemoteArtworkRequestTimeout;
        remoteArtworkSessionConfiguration.timeoutIntervalForResource =
            kHeimdallRemoteArtworkResourceTimeout;
        remoteArtworkSessionConfiguration.requestCachePolicy =
            NSURLRequestReloadIgnoringLocalCacheData;
        remoteArtworkSessionConfiguration.HTTPMaximumConnectionsPerHost =
            kHeimdallRemoteArtworkMaximumConnectionsPerHost;
        _remoteArtworkSession =
            [NSURLSession sessionWithConfiguration:remoteArtworkSessionConfiguration];

        char * const cacheDirectory = config_GetUserDir(HEIMDALL_CACHE_DIR);
        if (cacheDirectory != NULL) {
            NSString * const thumbnailCacheDirectory =
                [NSString stringWithFormat:@"%s/macosx-thumbnails", cacheDirectory];
            free(cacheDirectory);

            NSError *error = nil;
            if ([NSFileManager.defaultManager createDirectoryAtPath:thumbnailCacheDirectory
                                        withIntermediateDirectories:YES
                                                         attributes:@{NSFilePosixPermissions: @0700}
                                                              error:&error]) {
                _thumbnailCacheDirectory = [thumbnailCacheDirectory copy];
                dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{
                    [self pruneStoredThumbnailCache];
                });
            } else {
                NSLog(@"Failed to create thumbnail cache directory %@: %@",
                      thumbnailCacheDirectory, error);
            }
        }

        NSNotificationCenter * const notificationCenter = [NSNotificationCenter defaultCenter];
        [notificationCenter addObserver:self
                               selector:@selector(mediaItemThumbnailGenerated:)
                                   name:HeimdallLibraryModelMediaItemThumbnailGenerated
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(mediaItemUpdated:)
                                   name:HeimdallLibraryModelAudioMediaItemUpdated
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(mediaItemUpdated:)
                                   name:HeimdallLibraryModelVideoMediaItemUpdated
                                 object:nil];
    }
    return self;
}

- (void)dealloc
{
    [_remoteArtworkSession invalidateAndCancel];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)pruneStoredThumbnailCache
{
    if (_thumbnailCacheDirectory == nil) {
        return;
    }

    @synchronized (self) {
        NSArray<NSString *> * const cachedFiles =
            [NSFileManager.defaultManager contentsOfDirectoryAtPath:_thumbnailCacheDirectory
                                                              error:NULL];
        NSDate * const now = NSDate.date;

        for (NSString * const cachedFile in cachedFiles) {
            if (![cachedFile.pathExtension isEqualToString:@"jpg"]) {
                continue;
            }

            NSString * const cachedPath =
                [_thumbnailCacheDirectory stringByAppendingPathComponent:cachedFile];
            NSDictionary * const attributes =
                [NSFileManager.defaultManager attributesOfItemAtPath:cachedPath error:NULL];
            NSDate * const modificationDate = attributes[NSFileModificationDate];
            if (modificationDate != nil &&
                [now timeIntervalSinceDate:modificationDate] > kHeimdallThumbnailCacheMaximumAge) {
                [NSFileManager.defaultManager removeItemAtPath:cachedPath error:NULL];
            }
        }
    }
}

- (nullable NSString *)thumbnailPathForInputItem:(HeimdallInputItem *)inputItem
                                        cacheKey:(NSString *)cacheKey
{
    if (_thumbnailCacheDirectory == nil || inputItem.path.length == 0 || cacheKey.length == 0) {
        return nil;
    }

    NSString * const sourceKey = inputItem.path;
    NSString * const sourceHash = thumbnailHashForString(sourceKey);
    NSDictionary * const sourceAttributes =
        [NSFileManager.defaultManager attributesOfItemAtPath:inputItem.path error:NULL];
    NSDate * const modificationDate = sourceAttributes[NSFileModificationDate];
    NSNumber * const sourceSize = sourceAttributes[NSFileSize];
    NSString * const identifier = [NSString stringWithFormat:@"%@-%@-%llu-%.6f-%ux%u-%.6f",
                                   sourceKey,
                                   cacheKey,
                                   sourceSize.unsignedLongLongValue,
                                   modificationDate.timeIntervalSince1970,
                                   kHeimdallDesiredThumbnailWidth,
                                   kHeimdallDesiredThumbnailHeight,
                                   kHeimdallDefaultThumbnailPosition];
    NSString * const thumbnailHash = thumbnailHashForString(identifier);

    return [_thumbnailCacheDirectory stringByAppendingPathComponent:
            [NSString stringWithFormat:@"%@-%@.jpg", sourceHash, thumbnailHash]];
}

- (nullable NSImage *)thumbnailFromCacheForInputItem:(HeimdallInputItem *)inputItem
                                            cacheKey:(NSString *)cacheKey
{
    NSString * const thumbnailPath = [self thumbnailPathForInputItem:inputItem
                                                            cacheKey:cacheKey];
    if (thumbnailPath == nil) {
        return nil;
    }

    [self removeStaleThumbnailFilesForInputItem:inputItem
                                       cacheKey:cacheKey
                                    keepingPath:thumbnailPath];
    NSImage * const image = [[NSImage alloc] initWithContentsOfFile:thumbnailPath];
    if (image == nil && [NSFileManager.defaultManager fileExistsAtPath:thumbnailPath]) {
        [NSFileManager.defaultManager removeItemAtPath:thumbnailPath error:NULL];
    }
    return image;
}

- (void)removeStaleThumbnailFilesForInputItem:(HeimdallInputItem *)inputItem
                                     cacheKey:(NSString *)cacheKey
                                  keepingPath:(NSString *)keepingPath
{
    if (_thumbnailCacheDirectory == nil || inputItem.path.length == 0 || cacheKey.length == 0) {
        return;
    }

    NSString * const sourceKey = inputItem.path;
    NSString * const sourcePrefix = [thumbnailHashForString(sourceKey) stringByAppendingString:@"-"];
    NSArray<NSString *> * const cachedFiles =
        [NSFileManager.defaultManager contentsOfDirectoryAtPath:_thumbnailCacheDirectory error:NULL];
    for (NSString * const cachedFile in cachedFiles) {
        if (![cachedFile hasPrefix:sourcePrefix] || ![cachedFile.pathExtension isEqualToString:@"jpg"]) {
            continue;
        }

        NSString * const cachedPath = [_thumbnailCacheDirectory stringByAppendingPathComponent:cachedFile];
        if (![cachedPath isEqualToString:keepingPath]) {
            [NSFileManager.defaultManager removeItemAtPath:cachedPath error:NULL];
        }
    }
}

+ (instancetype)sharedImageCache
{
    static dispatch_once_t onceToken;
    static HeimdallLibraryImageCache *sharedImageCache;
    dispatch_once(&onceToken, ^{
        sharedImageCache = [[HeimdallLibraryImageCache alloc] init];
    });
    return sharedImageCache;
}

- (void)mediaItemThumbnailGenerated:(NSNotification *)aNotification
{
    HeimdallMediaLibraryMediaItem * const mediaItem = aNotification.object;
    NSString * const artworkMRL = mediaItem.smallArtworkMRL;
    if (mediaItem == nil || artworkMRL == nil) {
        return;
    }
    [_imageCache removeObjectForKey:artworkMRL];
}

- (void)mediaItemUpdated:(NSNotification *)aNotification
{
    HeimdallMediaLibraryMediaItem * const mediaItem = aNotification.object;
    NSString * const artworkMRL = mediaItem.smallArtworkMRL;
    if (mediaItem == nil || artworkMRL == nil) {
        return;
    }
    [_imageCache removeObjectForKey:artworkMRL];
}

- (void)imageForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
             withCompletion:(void(^)(const NSImage *))completionHandler
{
    NSString * const artworkMRL = libraryItem.smallArtworkMRL;
    if (artworkMRL) {
        NSImage * const cachedImage = [_imageCache objectForKey:artworkMRL];
        if (cachedImage) {
            dispatch_async(dispatch_get_main_queue(), ^{
                completionHandler(cachedImage);
            });
            return;
        }
    }

    if (libraryItem.smallArtworkGenerated && artworkMRL) {
        NSURL * const artworkURL = [NSURL URLWithString:artworkMRL];
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
            NSImage * const image =
                [HeimdallLibraryImageCache downsampledImageFromURL:artworkURL
                                                 maxPixelSize:kHeimdallDesiredThumbnailWidth];
            if (image) {
                const NSUInteger cost = [HeimdallLibraryImageCache costForImage:image];
                [self->_imageCache setObject:image forKey:artworkMRL cost:cost];
            }
            dispatch_async(dispatch_get_main_queue(), ^{
                completionHandler(image ?: self->_noArtImage);
            });
        });
    } else if ([libraryItem isKindOfClass:[HeimdallMediaLibraryMediaItem class]]) {
        HeimdallMediaLibraryMediaItem * const mediaItem = (HeimdallMediaLibraryMediaItem *)libraryItem;
        if (mediaItem.mediaType != HEIMDALL_ML_MEDIA_TYPE_AUDIO) {
            [self generateThumbnailForMediaItem:mediaItem.libraryID];
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            completionHandler(self->_noArtImage);
        });
    } else {
        dispatch_async(dispatch_get_main_queue(), ^{
            completionHandler(self->_noArtImage);
        });
    }
}

- (void)generateThumbnailForMediaItem:(int64_t)mediaID
{
    if (!_p_libraryInstance) {
        _p_libraryInstance = heimdall_ml_instance_get(getIntf());
    }
    heimdall_ml_media_generate_thumbnail(_p_libraryInstance,
                                    mediaID,
                                    HEIMDALL_ML_THUMBNAIL_SMALL,
                                    kHeimdallDesiredThumbnailWidth,
                                    kHeimdallDesiredThumbnailHeight,
                                    kHeimdallDefaultThumbnailPosition);
}

+ (void)thumbnailForInputItem:(HeimdallInputItem *)inputItem 
               withCompletion:(nonnull void (^)(const NSImage * _Nonnull))completionHandler
{
    [HeimdallLibraryImageCache.sharedImageCache imageForInputItem:inputItem withCompletion:completionHandler];
}

- (void)imageForCountryCode:(NSString *)countryCode
             withCompletion:(void(^)(const NSImage * _Nullable))completionHandler
{
    NSString * const normalizedCountryCode = countryCode.uppercaseString;
    NSString * const cacheKey = [@"flag://" stringByAppendingString:normalizedCountryCode];
    NSImage * const cachedImage = [_imageCache objectForKey:cacheKey];
    if (cachedImage) {
        dispatch_async(dispatch_get_main_queue(), ^{
            completionHandler(cachedImage);
        });
        return;
    }

    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        const NSSize imageSize =
            NSMakeSize(kHeimdallDesiredThumbnailWidth, kHeimdallDesiredThumbnailHeight);
        NSImage * const image = [NSImage flagImageForCountryCode:normalizedCountryCode
                                                           size:imageSize];
        if (image) {
            const NSUInteger cost = [HeimdallLibraryImageCache costForImage:image];
            [self->_imageCache setObject:image forKey:cacheKey cost:cost];
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            completionHandler(image);
        });
    });
}

- (void)imageForInputItem:(HeimdallInputItem *)inputItem
           withCompletion:(nonnull void (^)(const NSImage * _Nonnull))completionHandler
{
    NSURL * const artworkURL = inputItem.artworkURL;
    NSString * const cacheKey = [HeimdallLibraryImageCache artworkURLRequiresRemoteFetch:artworkURL]
        ? artworkURL.absoluteString
        : inputItem.MRL;

    NSImage * const cachedImage = [_imageCache objectForKey:cacheKey];
    if (cachedImage) {
        dispatch_async(dispatch_get_main_queue(), ^{
            completionHandler(cachedImage);
        });
        return;
    }
    [self generateImageForInputItem:inputItem withCompletion:completionHandler];
}

- (void)fetchRemoteArtwork:(NSURL *)url
               forCacheKey:(NSString *)cacheKey
            withCompletion:(void(^)(const NSImage *))completionHandler
{
    @synchronized (self) {
        NSMutableArray<void (^)(const NSImage *)> * const pendingCompletions =
            _pendingRemoteArtworkRequests[cacheKey];
        if (pendingCompletions != nil) {
            [pendingCompletions addObject:[completionHandler copy]];
            return;
        }

        _pendingRemoteArtworkRequests[cacheKey] =
            [NSMutableArray arrayWithObject:[completionHandler copy]];
    }

    [[_remoteArtworkSession dataTaskWithURL:url
                          completionHandler:^(NSData *data,
                                              NSURLResponse *response,
                                              NSError *error) {
        NSImage *image = nil;

        if (!error && [HeimdallLibraryImageCache remoteArtworkResponseCanContainImage:response] && data) {
            CGImageSourceRef const source = CGImageSourceCreateWithData((__bridge CFDataRef)data, NULL);
            if (source) {
                image = [HeimdallLibraryImageCache downsampledImageFromSource:source
                                                            maxPixelSize:kHeimdallDesiredThumbnailWidth];
                CFRelease(source);
            }
        }
        if (image) {
            const NSUInteger cost = [HeimdallLibraryImageCache costForImage:image];
            [self->_imageCache setObject:image forKey:cacheKey cost:cost];
        }

        NSArray<void (^)(const NSImage *)> *completionHandlers;
        @synchronized (self) {
            completionHandlers = [self->_pendingRemoteArtworkRequests[cacheKey] copy];
            [self->_pendingRemoteArtworkRequests removeObjectForKey:cacheKey];
        }

        dispatch_async(dispatch_get_main_queue(), ^{
            const NSImage * const artwork = image ?: self->_noArtImage;
            for (void (^handler)(const NSImage *) in completionHandlers) {
                handler(artwork);
            }
        });
    }] resume];
}

- (void)generateArtworkForInputItem:(HeimdallInputItem *)inputItem
                     withCompletion:(void(^)(const NSImage *))completionHandler
{
    NSURL * const artworkURL = inputItem.artworkURL;
    const NSSize imageSize = NSMakeSize(kHeimdallDesiredThumbnailWidth, kHeimdallDesiredThumbnailHeight);
    NSString * const memoryCacheKey = inputItem.MRL;

    if ([HeimdallLibraryImageCache artworkURLRequiresRemoteFetch:artworkURL]) {
        [self fetchRemoteArtwork:artworkURL
                     forCacheKey:artworkURL.absoluteString
                  withCompletion:completionHandler];
        return;
    }

    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        NSImage * const image = artworkURL
            ? [HeimdallLibraryImageCache downsampledImageFromURL:artworkURL
                                               maxPixelSize:kHeimdallDesiredThumbnailWidth]
            : nil;

        if (image) {
            const NSUInteger cost = [HeimdallLibraryImageCache costForImage:image];
            [self->_imageCache setObject:image forKey:memoryCacheKey cost:cost];
            dispatch_async(dispatch_get_main_queue(), ^{
                completionHandler(image);
            });
            return;
        }

        if (inputItem.inputType == ITEM_TYPE_FILE &&
            !inputItem.isStream &&
            input_item_Playable(inputItem.path.UTF8String)) {
            NSImage * const cachedThumbnail =
                [self thumbnailFromCacheForInputItem:inputItem
                                            cacheKey:memoryCacheKey];
            if (cachedThumbnail != nil) {
                const NSUInteger cost = [HeimdallLibraryImageCache costForImage:cachedThumbnail];
                [self->_imageCache setObject:cachedThumbnail forKey:memoryCacheKey cost:cost];
                dispatch_async(dispatch_get_main_queue(), ^{
                    completionHandler(cachedThumbnail);
                });
                return;
            }

            [self generateHeimdallThumbnailForInputItem:inputItem
                                          cacheKey:inputItem.MRL
                                        completion:^(NSImage * const thumbnail) {
                if (thumbnail != nil) {
                    const NSUInteger cost = [HeimdallLibraryImageCache costForImage:thumbnail];
                    [self->_imageCache setObject:thumbnail forKey:memoryCacheKey cost:cost];
                    dispatch_async(dispatch_get_main_queue(), ^{
                        completionHandler(thumbnail);
                    });
                    return;
                }

                [inputItem thumbnailWithSize:imageSize completionHandler:^(NSImage * const quickLookThumbnail) {
                    if (quickLookThumbnail) {
                        const NSUInteger cost = [HeimdallLibraryImageCache costForImage:quickLookThumbnail];
                        [self->_imageCache setObject:quickLookThumbnail forKey:memoryCacheKey cost:cost];
                    } else {
                        NSLog(@"Failed to generate thumbnail for input item %@", inputItem.MRL);
                    }
                    dispatch_async(dispatch_get_main_queue(), ^{
                        completionHandler(quickLookThumbnail ?: self->_noArtImage);
                    });
                }];
            }];
            return;
        }

        if (inputItem.isStream || inputItem.path.length == 0) {
            dispatch_async(dispatch_get_main_queue(), ^{
                completionHandler(self->_noArtImage);
            });
            return;
        }

        [inputItem thumbnailWithSize:imageSize completionHandler:^(NSImage * const quickLookThumbnail) {
            if (quickLookThumbnail) {
                const NSUInteger cost = [HeimdallLibraryImageCache costForImage:quickLookThumbnail];
                [self->_imageCache setObject:quickLookThumbnail forKey:memoryCacheKey cost:cost];
            } else {
                NSLog(@"Failed to generate thumbnail for input item %@", inputItem.MRL);
            }
            dispatch_async(dispatch_get_main_queue(), ^{
                completionHandler(quickLookThumbnail ?: self->_noArtImage);
            });
        }];
    });
}

- (void)generateImageForInputItem:(HeimdallInputItem *)inputItem
                   withCompletion:(void(^)(const NSImage *))completionHandler
{
    NSString * const radioCountryCode = inputItem.radioCountryCodeForFlagArtwork;
    if (!radioCountryCode) {
        [self generateArtworkForInputItem:inputItem withCompletion:completionHandler];
        return;
    }

    [self imageForCountryCode:radioCountryCode
               withCompletion:^(const NSImage * const flagImage) {
        if (flagImage) {
            completionHandler(flagImage);
            return;
        }
        [self generateArtworkForInputItem:inputItem withCompletion:completionHandler];
    }];
}

- (void)generateHeimdallThumbnailForInputItem:(HeimdallInputItem *)inputItem
                                cacheKey:(NSString *)cacheKey
                              completion:(HeimdallThumbnailCompletion)completion
{
    HeimdallThumbnailRequest *request;
    @synchronized (self) {
        request = _pendingThumbnailRequests[cacheKey];
        if (request != nil) {
            [request.completionHandlers addObject:[completion copy]];
            return;
        }

        request = [[HeimdallThumbnailRequest alloc] init];
        request.imageCache = self;
        request.cacheKey = cacheKey;
        request.thumbnailPath = [self thumbnailPathForInputItem:inputItem
                                                       cacheKey:cacheKey];
        request.completionHandlers = [NSMutableArray arrayWithObject:[completion copy]];
        _pendingThumbnailRequests[cacheKey] = request;
    }

    heimdall_preparser_t * const thumbnailer = getThumbnailer();
    if (thumbnailer == NULL || request.thumbnailPath == nil) {
        [self thumbnailRequest:request
           didFinishWithStatus:HEIMDALL_EGENERIC
                       results:NULL
                   resultCount:0];
        return;
    }

    struct heimdall_thumbnailer_arg thumbnailerArgument = {
        .seek = {
            .type = HEIMDALL_THUMBNAILER_SEEK_POS,
            .pos = kHeimdallDefaultThumbnailPosition,
            .speed = HEIMDALL_THUMBNAILER_SEEK_FAST,
        },
        .hw_dec = false,
    };
    const struct heimdall_thumbnailer_output output = {
        .format = HEIMDALL_THUMBNAILER_FORMAT_JPEG,
        .width = kHeimdallDesiredThumbnailWidth,
        .height = kHeimdallDesiredThumbnailHeight,
        .crop = true,
        .file_path = request.thumbnailPath.fileSystemRepresentation,
        .creat_mode = 0600,
    };
    static const struct heimdall_thumbnailer_to_files_cbs callbacks = {
        .on_ended = heimdallThumbnailerToFilesOnEnded,
    };

    heimdall_preparser_req * const requestHandle = heimdall_preparser_req_NewThumbnailToFiles(
        thumbnailer,
        inputItem.heimdallInputItem,
        &thumbnailerArgument,
        &output,
        1,
        &callbacks,
        (__bridge void *)request);
    if (requestHandle != NULL) {
        if (heimdall_preparser_Submit(thumbnailer, requestHandle) == HEIMDALL_SUCCESS) {
            return;
        }
        heimdall_preparser_req_Release(requestHandle);
    }

    [self thumbnailRequest:request
       didFinishWithStatus:HEIMDALL_EGENERIC
                   results:NULL
               resultCount:0];
}

- (void)thumbnailRequest:(HeimdallThumbnailRequest *)request
     didFinishWithStatus:(int)status
                 results:(const bool *)results
             resultCount:(size_t)resultCount
{
    NSArray<HeimdallThumbnailCompletion> *completionHandlers;
    @synchronized (self) {
        [_pendingThumbnailRequests removeObjectForKey:request.cacheKey];
        completionHandlers = [request.completionHandlers copy];
    }

    if (status == -EINTR) {
        for (HeimdallThumbnailCompletion completion in completionHandlers) {
            completion(nil);
        }
        return;
    }

    NSImage *image = nil;
    if (status == HEIMDALL_SUCCESS && resultCount == 1 && results != NULL && results[0]) {
        image = [[NSImage alloc] initWithContentsOfFile:request.thumbnailPath];
    }

    if (image == nil && request.thumbnailPath != nil) {
        [NSFileManager.defaultManager removeItemAtPath:request.thumbnailPath error:NULL];
    }

    for (HeimdallThumbnailCompletion completion in completionHandlers) {
        completion(image);
    }
}

+ (void)thumbnailForPlayQueueItem:(HeimdallPlayQueueItem *)playQueueItem
                   withCompletion:(nonnull void (^)(const NSImage * _Nonnull))completionHandler
{
    HeimdallMediaLibraryMediaItem * const mediaItem = playQueueItem.mediaLibraryItem;
    if (mediaItem.smallArtworkGenerated && mediaItem.smallArtworkMRL.length > 0) {
        return [HeimdallLibraryImageCache thumbnailForLibraryItem:mediaItem
                                              withCompletion:completionHandler];
    }

    return [HeimdallLibraryImageCache thumbnailForInputItem:playQueueItem.inputItem
                                        withCompletion:completionHandler];
}

+ (void)thumbnailForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
                 withCompletion:(void(^)(const NSImage *))completionHandler
{
    HeimdallLibraryImageCache * const cache = [HeimdallLibraryImageCache sharedImageCache];

    if (![libraryItem isKindOfClass:HeimdallMediaLibraryAlbum.class] &&
        ![libraryItem isKindOfClass:HeimdallMediaLibraryMediaItem.class]) {

        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
            NSMutableSet<NSString *> * const seenMRLs = NSMutableSet.set;
            NSMutableArray<HeimdallMediaLibraryMediaItem *> * const uniqueItems =
                [NSMutableArray arrayWithCapacity:kHeimdallCompositeImageDefaultCompositedGridItemCount];

            [libraryItem enumerateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem * const item, BOOL * const stop) {
                if (uniqueItems.count == (NSUInteger)kHeimdallCompositeImageDefaultCompositedGridItemCount) {
                    *stop = YES;
                    return;
                }
                NSString * const mrl = item.smallArtworkMRL;
                if (mrl == nil || [seenMRLs containsObject:mrl]) {
                    return;
                }
                [seenMRLs addObject:mrl];
                [uniqueItems addObject:item];
            }];

            if (uniqueItems.count == 0) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    completionHandler(cache->_noArtImage);
                });
                return;
            }

            NSMutableArray<NSImage *> * const itemImages =
                [NSMutableArray arrayWithCapacity:uniqueItems.count];
            dispatch_group_t const group = dispatch_group_create();

            for (HeimdallMediaLibraryMediaItem * const item in uniqueItems) {
                dispatch_group_enter(group);
                [cache imageForLibraryItem:item withCompletion:^(const NSImage * thumbnail) {
                    NSImage * const mutableRef = (NSImage *)thumbnail;
                    if (mutableRef && ![mutableRef isEqual:cache->_noArtImage]) {
                        @synchronized (itemImages) {
                            [itemImages addObject:mutableRef];
                        }
                    }
                    dispatch_group_leave(group);
                }];
            }

            dispatch_group_notify(group, dispatch_get_main_queue(), ^{
                if (itemImages.count == 0) {
                    completionHandler(cache->_noArtImage);
                    return;
                }

                const NSSize size = NSMakeSize(kHeimdallDesiredThumbnailWidth, kHeimdallDesiredThumbnailHeight);
                NSArray<NSValue *> * const frames =
                    [NSImage framesForCompositeImageSquareGridWithImages:itemImages
                                                                    size:size
                                                           gridItemCount:kHeimdallCompositeImageDefaultCompositedGridItemCount];
                NSImage * const compositeImage =
                    [NSImage compositeImageWithImages:itemImages frames:frames size:size];

                completionHandler(compositeImage);
            });
        });
    } else {
        [cache imageForLibraryItem:libraryItem withCompletion:completionHandler];
    }
}

@end
