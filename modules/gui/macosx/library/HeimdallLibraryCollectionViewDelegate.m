/*****************************************************************************
 * HeimdallLibraryCollectionViewDelegate.m: MacOS X interface module
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

#import "HeimdallLibraryCollectionViewDelegate.h"

#import "HeimdallLibraryCollectionViewDataSource.h"
#import "HeimdallLibraryCollectionViewFlowLayout.h"
#import "HeimdallLibraryCollectionViewItem.h"
#import "HeimdallLibraryDataTypes.h"

@implementation HeimdallLibraryCollectionViewDelegate

- (instancetype)init
{
    self = [super init];
    if (self) {
        _dynamicItemSizing = YES;
        _staticItemSize = HeimdallUIUnits.defaultMediaItemCollectionViewItemSize;
        _itemsAspectRatio = HeimdallLibraryCollectionViewItemAspectRatioDefaultItem;
    }
    return self;
}

- (void)collectionView:(NSCollectionView *)collectionView didSelectItemsAtIndexPaths:(NSSet<NSIndexPath *> *)indexPaths
{
    NSIndexPath *indexPath = indexPaths.anyObject;
    if (!indexPath) {
        return;
    }

    if (collectionView.selectionIndexPaths.count != 1) {
        return;
    }

    HeimdallLibraryCollectionViewFlowLayout *collectionViewFlowLayout = (HeimdallLibraryCollectionViewFlowLayout*)collectionView.collectionViewLayout;
    if(collectionViewFlowLayout) {
        [collectionViewFlowLayout expandDetailSectionAtIndex:indexPath];
    }
}

- (void)collectionView:(NSCollectionView *)collectionView didDeselectItemsAtIndexPaths:(NSSet<NSIndexPath *> *)indexPaths
{
    if (collectionView.selectionIndexPaths.count > 0) {
        return;
    }

    HeimdallLibraryCollectionViewFlowLayout * const collectionViewFlowLayout = 
        (HeimdallLibraryCollectionViewFlowLayout*)collectionView.collectionViewLayout;

    if (collectionViewFlowLayout) {
        for (NSIndexPath * const indexPath in indexPaths) {
            [collectionViewFlowLayout collapseDetailSectionAtIndex:indexPath];
        }
    }
}

- (NSSize)collectionView:(NSCollectionView *)collectionView
                  layout:(NSCollectionViewLayout *)collectionViewLayout
  sizeForItemAtIndexPath:(NSIndexPath *)indexPath
{
    if (!_dynamicItemSizing) {
        return _staticItemSize;
    }
    
    HeimdallLibraryCollectionViewFlowLayout * const collectionViewFlowLayout = (HeimdallLibraryCollectionViewFlowLayout *)collectionViewLayout;
    if (collectionViewLayout) {
        HeimdallLibraryCollectionViewItemAspectRatio aspectRatio = _itemsAspectRatio;
        
        if ([collectionView.dataSource conformsToProtocol:@protocol(HeimdallLibraryCollectionViewDataSource)]) {
            id<HeimdallLibraryCollectionViewDataSource> libraryDataSource =
                (id<HeimdallLibraryCollectionViewDataSource>)collectionView.dataSource;

            if ([libraryDataSource respondsToSelector:@selector(collectionView:aspectRatioForSection:)]) {
                aspectRatio = [libraryDataSource collectionView:collectionView
                                          aspectRatioForSection:indexPath.section];
            }
        }
        
        return [HeimdallUIUnits adjustedCollectionViewItemSizeForCollectionView:collectionView
                                                                withLayout:collectionViewFlowLayout
                                                      withItemsAspectRatio:aspectRatio];
    }

    return NSZeroSize;
}

- (BOOL)collectionView:(NSCollectionView *)collectionView
canDragItemsAtIndexPaths:(NSSet<NSIndexPath *> *)indexPaths
             withEvent:(NSEvent *)event
{
    return YES;
}

- (BOOL)collectionView:(NSCollectionView *)collectionView
writeItemsAtIndexPaths:(NSSet<NSIndexPath *> *)indexPaths
          toPasteboard:(NSPasteboard *)pasteboard
{
    if (![collectionView.dataSource conformsToProtocol:@protocol(HeimdallLibraryCollectionViewDataSource)]) {
        return NO;
    }

    NSObject<HeimdallLibraryCollectionViewDataSource> *heimdallDataSource = (NSObject<HeimdallLibraryCollectionViewDataSource>*)collectionView.dataSource;

    NSUInteger numberOfIndexPaths = indexPaths.count;
    NSMutableArray *encodedLibraryItemsArray = [NSMutableArray arrayWithCapacity:numberOfIndexPaths];
    NSMutableArray *filePathsArray = [NSMutableArray arrayWithCapacity:numberOfIndexPaths];

    for (NSIndexPath *indexPath in indexPaths) {

        id<HeimdallMediaLibraryItemProtocol> libraryItem = [heimdallDataSource libraryItemAtIndexPath:indexPath
                                                                          forCollectionView:collectionView];

        [libraryItem iterateMediaItemsWithBlock:^(HeimdallMediaLibraryMediaItem *mediaItem) {
            [encodedLibraryItemsArray addObject:mediaItem];

            HeimdallMediaLibraryFile *file = mediaItem.files.firstObject;
            if (file) {
                NSURL *url = [NSURL URLWithString:file.MRL];
                [filePathsArray addObject:url.path];
            }
        }];
    }

    NSError *archiveError = nil;
    NSData * const data = [NSKeyedArchiver archivedDataWithRootObject:encodedLibraryItemsArray
                                                requiringSecureCoding:YES
                                                                error:&archiveError];
    if (data == nil) {
        NSLog(@"Failed to archive MediaLibrary Item drag payload: %@", archiveError);
        return NO;
    }
    [pasteboard declareTypes:@[HeimdallMediaLibraryMediaItemPasteboardType, NSFilenamesPboardType] owner:self];
    [pasteboard setPropertyList:filePathsArray forType:NSFilenamesPboardType];
    [pasteboard setData:data forType:HeimdallMediaLibraryMediaItemPasteboardType];

    return YES;
}

@end
