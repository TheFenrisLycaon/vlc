/*****************************************************************************
 * NSImage+HeimdallAdditions.h: MacOS X interface module
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

NS_ASSUME_NONNULL_BEGIN

struct picture_t;
struct heimdall_object_t;

@interface NSImage (HeimdallAdditions)

// App images
@property (class, readonly) NSImage *HeimdallAppIconImage;
@property (class, readonly) NSImage *HeimdallXmasAppIconImage;
@property (class, readonly) NSImage *HeimdallStatusBarIconImage;

// Sidebar images
@property (class, readonly) NSImage *HeimdallSidebarMovieImage;
@property (class, readonly) NSImage *HeimdallSidebarMusicImage;

// B&W media source images
@property (class, readonly) NSImage *HeimdallBWHomeImage;
@property (class, readonly) NSImage *HeimdallBWMusicImage;
@property (class, readonly) NSImage *HeimdallBWServer1Image;
@property (class, readonly) NSImage *HeimdallBWServer2Image;
@property (class, readonly) NSImage *HeimdallBWUsb1Image;
@property (class, readonly) NSImage *HeimdallBWUsb2Image;
@property (class, readonly) NSImage *HeimdallDefaultAppIconImage;
@property (class, readonly) NSImage *HeimdallFollowImage;

// Placeholder images
@property (class, readonly) NSImage *HeimdallNoArtImage;
@property (class, readonly) NSImage *HeimdallPlaceholderVideoImage;
@property (class, readonly) NSImage *HeimdallPlaceholderGroupImage;
@property (class, readonly) NSImage *HeimdallPlaceholderMusicImage;
@property (class, readonly) NSImage *HeimdallGenericImage;

// Playback control images
@property (class, readonly) NSImage *HeimdallPlayTemplateImage;
@property (class, readonly) NSImage *HeimdallPauseTemplateImage;
@property (class, readonly) NSImage *HeimdallBackwardTemplateImage;
@property (class, readonly) NSImage *HeimdallForwardTemplateImage;
@property (class, readonly) NSImage *HeimdallFullscreenOffTemplateImage;
@property (class, readonly) NSImage *HeimdallVolumeOnTemplateImage;
@property (class, readonly) NSImage *HeimdallVolumeOffTemplateImage;
@property (class, readonly) NSImage *HeimdallStopImage;
@property (class, readonly) NSImage *HeimdallStopPressedImage;
@property (class, readonly) NSImage *HeimdallShuffleOffImage;
@property (class, readonly) NSImage *HeimdallShuffleOnImage;
@property (class, readonly) NSImage *HeimdallRepeatAllImage;
@property (class, readonly) NSImage *HeimdallRepeatOneImage;
@property (class, readonly) NSImage *HeimdallRepeatOffImage;

+ (void)quickLookPreviewForLocalPath:(NSString *)path
                            withSize:(NSSize)size 
                   completionHandler:(void (^)(NSImage * _Nullable))completionHandler;
+ (void)quickLookPreviewForLocalURL:(NSURL *)url 
                           withSize:(NSSize)size 
                  completionHandler:(void (^)(NSImage * _Nullable))completionHandler;
+ (instancetype)quickLookPreviewForLocalPath:(NSString*)path withSize:(NSSize)size;
+ (instancetype)quickLookPreviewForLocalURL:(NSURL*)url withSize:(NSSize)size;
+ (nullable NSImage *)imageFromHeimdallPicture:(struct picture_t *)picture
                                heimdallObject:(struct heimdall_object_t *)object
                                     size:(NSSize)size;
+ (instancetype)compositeImageWithImages:(NSArray<NSImage *> * const)images
                                  frames:(NSArray<NSValue *> * const)frames
                                    size:(const NSSize)size;
+ (NSArray<NSValue *> *)framesForCompositeImageSquareGridWithImages:(NSArray<NSImage *> * const)images
                                                               size:(const NSSize)size
                                                      gridItemCount:(const NSUInteger)gridItemCount;
+ (nullable NSImage *)flagImageForCountryCode:(NSString *)countryCode
                                         size:(NSSize)size;

- (instancetype)imageTintedWithColor:(NSColor *)color;

@end

NS_ASSUME_NONNULL_END
