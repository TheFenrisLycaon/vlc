/*****************************************************************************
 * HeimdallLibrarySearchProvider.h: MacOS X interface module
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

#import <Cocoa/Cocoa.h>

#import <heimdall_media_library.h>

NS_ASSUME_NONNULL_BEGIN

@protocol HeimdallMediaLibraryItemProtocol;

extern NSString * const HeimdallLibrarySearchProviderResultsUpdated;

// Block called on a background queue with query params containing the search pattern.
typedef NSArray<id<HeimdallMediaLibraryItemProtocol>> * _Nonnull
    (^HeimdallLibrarySearchProviderFetchBlock)(const heimdall_ml_query_params_t * _Nonnull params);

@interface HeimdallLibrarySearchProvider : NSObject

@property (readonly) NSString *displayTitle;
@property (readonly) NSImage *displayImage;
@property (readonly) NSArray<id<HeimdallMediaLibraryItemProtocol>> *results;

- (instancetype)initWithDisplayTitle:(NSString *)displayTitle
                        displayImage:(NSImage *)displayImage
                          fetchBlock:(HeimdallLibrarySearchProviderFetchBlock)fetchBlock;

+ (NSArray<HeimdallLibrarySearchProvider *> *)defaultProviders;

- (void)searchForString:(NSString *)string;
- (void)clearSearch;

@end

NS_ASSUME_NONNULL_END
