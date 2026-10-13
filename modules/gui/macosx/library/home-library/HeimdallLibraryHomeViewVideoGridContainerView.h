/*****************************************************************************
 * HeimdallLibraryHomeViewVideoGridContainerView.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
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

#import "library/home-library/HeimdallLibraryHomeViewVideoContainerView.h"
#import "library/video-library/HeimdallLibraryVideoGroupDescriptor.h"

@class HeimdallSubScrollView;
@class HeimdallLibraryCollectionView;
@class HeimdallLibraryCollectionViewDelegate;
@class HeimdallLibraryCollectionViewFlowLayout;
@class HeimdallLibraryVideoCollectionViewGroupDescriptor;

NS_ASSUME_NONNULL_BEGIN

@interface HeimdallLibraryHomeViewVideoGridContainerView : NSView<HeimdallLibraryHomeViewVideoContainerView>

@property (readonly) HeimdallLibraryCollectionView *collectionView;
@property (readonly) HeimdallLibraryCollectionViewDelegate *collectionViewDelegate;
@property (readonly) HeimdallLibraryCollectionViewFlowLayout *collectionViewLayout;
@property (readonly) HeimdallSubScrollView *scrollView;

@end

NS_ASSUME_NONNULL_END
