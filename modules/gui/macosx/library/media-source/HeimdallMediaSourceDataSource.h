/*****************************************************************************
 * HeimdallMediaSourceDataSource.h: MacOS X interface module
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

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@class HeimdallInputItem;
@class HeimdallInputNode;
@class HeimdallInputNodePathControl;
@class HeimdallLibraryMediaSourceViewNavigationStack;
@class HeimdallMediaSource;
@class HeimdallMediaSourceBaseDataSource;

extern NSString * const HeimdallMediaSourceDataSourceNodeChanged;
extern NSString * const HeimdallMediaSourceDataSourceLoadingStarted;
extern NSString * const HeimdallMediaSourceDataSourceLoadingEnded;

@interface HeimdallMediaSourceDataSource : NSObject <NSCollectionViewDataSource,
                                                NSCollectionViewDelegate,
                                                NSCollectionViewDelegateFlowLayout,
                                                NSTableViewDelegate,
                                                NSTableViewDataSource>

@property (readwrite, retain) HeimdallMediaSource *displayedMediaSource;
@property (readwrite, retain, nonatomic) HeimdallInputNode *nodeToDisplay;
@property (readwrite, weak) NSCollectionView *collectionView;
@property (readwrite, weak) NSTableView *tableView;
@property (readwrite, weak) HeimdallInputNodePathControl *pathControl;
@property (readwrite, weak) HeimdallLibraryMediaSourceViewNavigationStack *navigationStack;
@property (readwrite, weak) HeimdallMediaSourceBaseDataSource *parentBaseDataSource;
@property (readonly, nonatomic) BOOL hasDisplayedItems;

- (instancetype)initWithParentBaseDataSource:(HeimdallMediaSourceBaseDataSource *)parentBaseDataSource;
- (void)setupViews;
- (HeimdallInputItem *)mediaSourceInputItemAtRow:(NSInteger)tableViewRow;
- (NSArray<HeimdallInputItem *> *)mediaSourceInputItemsAtIndexPaths:(NSSet<NSIndexPath *> *const)indexPaths;

@end

NS_ASSUME_NONNULL_END
