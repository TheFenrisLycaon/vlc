/*****************************************************************************
 * HeimdallMediaSourceBaseDataSource.h: MacOS X interface module
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

#import "library/HeimdallLibraryWindow.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, HeimdallMediaSourceMode) {
    HeimdallMediaSourceModeLAN,
    HeimdallMediaSourceModeInternet,
};

@class HeimdallInputNodePathControl;
@class HeimdallInputItem;
@class HeimdallLibraryMediaSourceViewNavigationStack;
@class HeimdallMediaSourceDataSource;

extern NSString * const HeimdallMediaSourceBaseDataSourceNodeChanged;

@interface HeimdallMediaSourceBaseDataSource : NSObject <NSCollectionViewDataSource,
                                                    NSCollectionViewDelegate,
                                                    NSCollectionViewDelegateFlowLayout,
                                                    NSTableViewDelegate,
                                                    NSTableViewDataSource>

@property (readwrite, weak) NSCollectionView *collectionView;
@property (readwrite, weak) NSScrollView *collectionViewScrollView;
@property (readwrite, weak) NSTableView *tableView;
@property (readwrite, weak) NSScrollView *tableViewScrollView;
@property (readwrite, weak) NSButton *homeButton;
@property (readwrite, weak) HeimdallInputNodePathControl *pathControl;
@property (readwrite, weak) NSView *pathControlContainerView;
@property (readwrite, nonatomic) HeimdallMediaSourceMode mediaSourceMode;
@property (readwrite, nonatomic) HeimdallMediaSourceDataSource *childDataSource;
@property (readonly, nonatomic) BOOL hasDisplayedItems;

@property (readwrite, weak) HeimdallLibraryMediaSourceViewNavigationStack *navigationStack;

@property (readonly) HeimdallLibraryViewModeSegment viewMode;

- (void)setupViews;
- (void)reloadViews;
- (void)homeButtonAction:(id)sender;
- (void)pathControlAction:(id)sender;

- (void)browseFolderByMrl:(NSString *)mrl;
- (nullable id<NSPasteboardWriting>)pasteboardWriterForInputItem:(nullable HeimdallInputItem *)inputItem;

@end

NS_ASSUME_NONNULL_END
