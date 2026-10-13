/*****************************************************************************
 * HeimdallMediaSourceBaseDataSource.m: MacOS X interface module
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

#import "HeimdallMediaSourceBaseDataSource.h"

#import "HeimdallLibraryMediaSourceViewNavigationStack.h"
#import "HeimdallMediaSource.h"
#import "HeimdallLocalMediaSource.h"
#import "HeimdallMediaSourceDataSource.h"
#import "HeimdallMediaSourceDeviceCollectionViewItem.h"
#import "HeimdallMediaSourceProvider.h"

#import "extensions/NSImage+HeimdallAdditions.h"
#import "extensions/NSPasteboardItem+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"
#import "extensions/NSTableCellView+HeimdallAdditions.h"
#import "extensions/NSWindow+HeimdallAdditions.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallInputNode.h"
#import "library/HeimdallInputNodePathControl.h"
#import "library/HeimdallInputNodePathControlItem.h"
#import "library/HeimdallLibraryCollectionViewSupplementaryElementView.h"
#import "library/HeimdallLibraryImageCache.h"
#import "library/HeimdallLibraryTableCellView.h"
#import "library/HeimdallLibraryWindow.h"
#import "library/HeimdallLibraryWindowPersistentPreferences.h"

#import "library/media-source/HeimdallMediaSourceCollectionViewItem.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallFileDragRecognisingView.h"
#import "views/HeimdallImageView.h"
#import "views/HeimdallUIUnits.h"

NSString * const HeimdallMediaSourceBaseDataSourceNodeChanged = @"HeimdallMediaSourceBaseDataSourceNodeChanged";
NSString * const HeimdallMediaSourceTableTagsColumnIdentifier = @"HeimdallMediaSourceTableTagsColumn";

@interface HeimdallLANDeviceRecord : NSObject
@property (readonly) HeimdallMediaSource *mediaSource;
@property (readonly) HeimdallInputNode *inputNode;

- (instancetype)initWithMediaSource:(HeimdallMediaSource *)mediaSource
                          inputNode:(HeimdallInputNode *)inputNode;
@end

@implementation HeimdallLANDeviceRecord

- (instancetype)initWithMediaSource:(HeimdallMediaSource *)mediaSource
                          inputNode:(HeimdallInputNode *)inputNode
{
    self = [super init];
    if (self) {
        _mediaSource = mediaSource;
        _inputNode = inputNode;
    }
    return self;
}

@end

@interface HeimdallMediaSourceBaseDataSource () <NSCollectionViewDataSource, NSCollectionViewDelegate, NSTableViewDelegate, NSTableViewDataSource>
{
    NSArray<HeimdallMediaSource *> *_mediaSources;
    NSArray<NSString *> *_mediaSourceNotificationNames;
    NSMapTable<HeimdallMediaSource *, NSArray<HeimdallLANDeviceRecord *> *> *_sourceRecords;
    NSArray<HeimdallLANDeviceRecord *> *_lanDeviceSnapshot;
}
@end

@implementation HeimdallMediaSourceBaseDataSource

- (instancetype)init
{
    self = [super init];
    if (self) {
        _mediaSources = @[];
        _lanDeviceSnapshot = @[];
        _mediaSourceNotificationNames = @[
            HeimdallMediaSourceChildrenReset,
            HeimdallMediaSourceChildrenAdded,
            HeimdallMediaSourceChildrenRemoved,
            HeimdallMediaSourcePreparsingEnded
        ];
        _sourceRecords = [NSMapTable mapTableWithKeyOptions:NSPointerFunctionsWeakMemory |
                                                           NSPointerFunctionsObjectPointerPersonality
                                              valueOptions:NSPointerFunctionsStrongMemory |
                                                          NSPointerFunctionsObjectPersonality];
        _mediaSourceMode = HeimdallMediaSourceModeLAN;
        [self loadMediaSources];
        [self returnHome];
    }
    return self;
}

- (void)dealloc
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark - view and model state management

- (HeimdallLibraryViewModeSegment)viewMode
{
    HeimdallLibraryWindowPersistentPreferences * const libraryWindowPrefs =
        HeimdallLibraryWindowPersistentPreferences.sharedInstance;

    switch (_mediaSourceMode) {
        case HeimdallMediaSourceModeLAN:
            return libraryWindowPrefs.browseLibraryViewMode;
        case HeimdallMediaSourceModeInternet:
            return libraryWindowPrefs.streamLibraryViewMode;
        default:
            return HeimdallLibraryGridViewModeSegment;
    }
}

- (void)setupViews
{
    self.collectionView.dataSource = self;
    self.collectionView.delegate = self;
    [self.collectionView registerClass:[HeimdallMediaSourceDeviceCollectionViewItem class]
                 forItemWithIdentifier:HeimdallMediaSourceDeviceCellIdentifier];
    [self.collectionView registerClass:HeimdallMediaSourceCollectionViewItem.class
                 forItemWithIdentifier:HeimdallMediaSourceCollectionViewItemIdentifier];
    [self.collectionView registerClass:[HeimdallLibraryCollectionViewSupplementaryElementView class]
               forSupplementaryViewOfKind:NSCollectionElementKindSectionHeader
                           withIdentifier:HeimdallLibrarySupplementaryElementViewIdentifier];

    self.homeButton.action = @selector(homeButtonAction:);
    self.homeButton.target = self;
    [self.pathControl clearInputNodePathControlItems];
    self.pathControl.action = @selector(pathControlAction:);
    self.pathControl.target = self;

    [self togglePathControlVisibility:NO];

    self.tableView.dataSource = self;
    self.tableView.delegate = self;
    [self.tableView registerForDraggedTypes:@[NSFilenamesPboardType]];
    [self.tableView setDraggingSourceOperationMask:NSDragOperationCopy forLocal:NO];
    [self.tableView setDraggingSourceOperationMask:NSDragOperationCopy forLocal:YES];

    NSNib * const tableCellViewNib = [[NSNib alloc] initWithNibNamed:NSStringFromClass(HeimdallLibraryTableCellView.class) bundle:nil];
    [self.tableView registerNib:tableCellViewNib forIdentifier:HeimdallLibraryTableCellViewIdentifier];

    [self updateTableColumnVisibility];
    [self reloadViews];
}

- (void)updateTableColumnVisibility
{
    NSTableColumn * const tagsColumn =
        [self.tableView tableColumnWithIdentifier:HeimdallMediaSourceTableTagsColumnIdentifier];
    tagsColumn.hidden = self.mediaSourceMode == HeimdallMediaSourceModeInternet;
}

- (void)reloadViews
{
    const HeimdallLibraryViewModeSegment viewModeSegment = self.viewMode;
    if (viewModeSegment == HeimdallLibraryGridViewModeSegment) {
        self.collectionViewScrollView.hidden = NO;
        self.tableViewScrollView.hidden = YES;
        [self.collectionView reloadData];
    } else if (viewModeSegment == HeimdallLibraryListViewModeSegment) {
        self.collectionViewScrollView.hidden = YES;
        self.tableViewScrollView.hidden = NO;
        [self.tableView reloadData];
    } else {
        NSAssert(false, @"View mode must be grid or list mode");
    }
    [self togglePathControlVisibility:self.childDataSource != nil];
}

- (void)loadMediaSources
{
    [self.pathControl clearInputNodePathControlItems];

    NSArray *mediaSources;
    if (self.mediaSourceMode == HeimdallMediaSourceModeLAN) {
        mediaSources = HeimdallMediaSourceProvider.listOfLocalMediaSources;
    } else {
        mediaSources = [HeimdallMediaSourceProvider listOfMediaSourcesForCategory:SD_CAT_INTERNET];
    }
    NSAssert(mediaSources != nil, @"Media sources array should not be nil");

    for (HeimdallMediaSource * const mediaSource in mediaSources) {
        HeimdallInputNode * const rootNode = [mediaSource rootNode];
        if (rootNode == nil)
            continue;
        NSError * const error = [mediaSource preparseInputNodeWithinTree:rootNode];
        if (error == nil)
            [self.navigationStack installHandlersOnMediaSource:mediaSource];
    }

    [self setMediaSources:mediaSources];
    _lanDeviceSnapshot = self.mediaSourceMode == HeimdallMediaSourceModeLAN ? [self buildMediaSourceSnapshot] : @[];
    [self reloadData];
}

- (void)setMediaSources:(NSArray<HeimdallMediaSource *> *)mediaSources
{
    NSNotificationCenter * const nc = NSNotificationCenter.defaultCenter;

    for (HeimdallMediaSource * const source in _mediaSources) {
        for (NSString * const name in _mediaSourceNotificationNames) {
            [nc removeObserver:self name:name object:source];
        }
    }

    _mediaSources = mediaSources;

    for (HeimdallMediaSource * const source in _mediaSources) {
        [nc addObserver:self
               selector:@selector(mediaSourceChildrenReset:)
                   name:HeimdallMediaSourceChildrenReset
                 object:source];
        [nc addObserver:self
               selector:@selector(mediaSourceChildrenAdded:)
                   name:HeimdallMediaSourceChildrenAdded
                 object:source];
        [nc addObserver:self
               selector:@selector(mediaSourceChildrenRemoved:)
                   name:HeimdallMediaSourceChildrenRemoved
                 object:source];
        [nc addObserver:self
               selector:@selector(mediaSourcePreparingEnded:)
                   name:HeimdallMediaSourcePreparsingEnded
                 object:source];
    }
}

- (void)setMediaSourceMode:(HeimdallMediaSourceMode)mediaSourceMode
{
    if (mediaSourceMode == self.mediaSourceMode) {
        return;
    }
    _mediaSourceMode = mediaSourceMode;
    [self updateTableColumnVisibility];
    [self loadMediaSources];
    [self returnHome];
}

- (BOOL)hasDisplayedItems
{
    if (_childDataSource != nil) {
        return _childDataSource.nodeToDisplay.numberOfChildren > 0;
    }

    if (_mediaSourceMode == HeimdallMediaSourceModeLAN) {
        return _lanDeviceSnapshot.count > 0;
    }

    return _mediaSources.count > 0;
}

#pragma mark - collection view data source

- (NSInteger)numberOfSectionsInCollectionView:(NSCollectionView *)collectionView
{
    if (_mediaSourceMode == HeimdallMediaSourceModeLAN) {
        return _mediaSources.count;
    }

    return 1;
}

- (NSInteger)collectionView:(NSCollectionView *)collectionView
     numberOfItemsInSection:(NSInteger)section
{
    if (_mediaSourceMode == HeimdallMediaSourceModeLAN) {
        HeimdallMediaSource * const mediaSource = _mediaSources[section];
        HeimdallInputNode * const rootNode = mediaSource.rootNode;
        return rootNode.numberOfChildren;
    }

    return _mediaSources.count;
}

- (NSCollectionViewItem *)collectionView:(NSCollectionView *)collectionView
     itemForRepresentedObjectAtIndexPath:(NSIndexPath *)indexPath
{
    HeimdallMediaSourceDeviceCollectionViewItem * const viewItem = [collectionView makeItemWithIdentifier:HeimdallMediaSourceDeviceCellIdentifier forIndexPath:indexPath];
    HeimdallMediaSource * const mediaSource = _mediaSources[indexPath.section];
    
    if (_mediaSourceMode == HeimdallMediaSourceModeLAN) {
        HeimdallInputNode * const rootNode = mediaSource.rootNode;
        NSArray * const nodeChildren = rootNode.children;
        if (nodeChildren == nil) {
            NSLog(@"No children found for media source root node: %@ cannot provide viewItem correctly", rootNode);
            return viewItem;
        }
        HeimdallInputNode * const childNode = nodeChildren[indexPath.item];
        HeimdallInputItem * const childRootInput = childNode.inputItem;
        viewItem.titleTextField.stringValue = childRootInput.name;

        const enum input_item_type_e inputType = childRootInput.inputType;
        const BOOL isStream = childRootInput.isStream;

        NSImage *placeholder;
        if (mediaSource.category == SD_CAT_LAN) {
            placeholder = NSImage.HeimdallBWMusicImage;
        } else {
            switch (inputType) {
                case ITEM_TYPE_DIRECTORY:
                    if ([childRootInput.name containsString:@"home"]) {
                        placeholder = NSImage.HeimdallBWHomeImage;
                    } else {
                        placeholder = indexPath.item % 2 ? NSImage.HeimdallBWServer1Image : NSImage.HeimdallBWServer2Image;
                    }
                    break;
                case ITEM_TYPE_DISC:
                    if (isStream) {
                        placeholder = indexPath.item % 2 ? NSImage.HeimdallBWServer1Image : NSImage.HeimdallBWServer2Image;
                    } else {
                        placeholder = indexPath.item % 2 ? NSImage.HeimdallBWUsb1Image : NSImage.HeimdallBWUsb2Image;;
                    }
                    break;
                default:
                    placeholder = NSImage.HeimdallBWMusicImage;
                    break;
            }
        }
        NSAssert(placeholder != nil, @"Placeholder image should not be nil");

        NSURL * const artworkURL = childRootInput.artworkURL;
        if (childRootInput.radioCountryCodeForFlagArtwork) {
            viewItem.mediaImageView.image = placeholder;
            [HeimdallLibraryImageCache thumbnailForInputItem:childRootInput
                                         withCompletion:^(const NSImage * const thumbnail) {
                viewItem.mediaImageView.image = (NSImage *)thumbnail;
            }];
        } else if (artworkURL) {
            [viewItem.mediaImageView setImageURL:artworkURL placeholderImage:placeholder];
        } else {
            viewItem.mediaImageView.image = placeholder;
        }
    } else {
        HeimdallMediaSource * const mediaSource = _mediaSources[indexPath.item];
        viewItem.titleTextField.stringValue = mediaSource.mediaSourceDescription;
        viewItem.mediaImageView.image = NSImage.HeimdallBWMusicImage;
    }

    return viewItem;
}

- (void)collectionView:(NSCollectionView *)collectionView didSelectItemsAtIndexPaths:(NSSet<NSIndexPath *> *)indexPaths
{
    NSIndexPath * const indexPath = indexPaths.anyObject;
    if (!indexPath) {
        return;
    }

    HeimdallMediaSource *mediaSource;
    HeimdallInputNode *childNode;

    if (_mediaSourceMode == HeimdallMediaSourceModeLAN) {
        mediaSource = _mediaSources[indexPath.section];
        HeimdallInputNode * const rootNode = mediaSource.rootNode;
        NSArray * const nodeChildren = rootNode.children;
        if (nodeChildren == nil) {
            NSLog(@"No children found for media source root node: %@ cannot access item correctly", rootNode);
            return;
        }
        childNode = nodeChildren[indexPath.item];
    } else {
        mediaSource = _mediaSources[indexPath.item];
        childNode = mediaSource.rootNode;
    }

    NSAssert(mediaSource != nil, @"Media source should not be nil");
    NSAssert(childNode != nil, @"Child node should not be nil");

    [self configureChildDataSourceWithNode:childNode andMediaSource:mediaSource];
    [self reloadData];
}

- (NSView *)collectionView:(NSCollectionView *)collectionView
viewForSupplementaryElementOfKind:(NSCollectionViewSupplementaryElementKind)kind
               atIndexPath:(NSIndexPath *)indexPath
{
    NSAssert([kind compare:NSCollectionElementKindSectionHeader] == NSOrderedSame, @"View request for non-existing footer.");

    HeimdallLibraryCollectionViewSupplementaryElementView *labelView = [collectionView makeSupplementaryViewOfKind:kind
                                                                                               withIdentifier:HeimdallLibrarySupplementaryElementViewIdentifier
                                                                                                 forIndexPath:indexPath];
    
    labelView.stringValue = _mediaSources[indexPath.section].mediaSourceDescription;

    return labelView;
}

- (CGSize)collectionView:(NSCollectionView *)collectionView
                  layout:(NSCollectionViewLayout *)collectionViewLayout
referenceSizeForHeaderInSection:(NSInteger)section
{
    if (_mediaSourceMode == HeimdallMediaSourceModeLAN) {
        HeimdallMediaSource * const mediaSource = _mediaSources[section];
        HeimdallInputNode * const rootNode = mediaSource.rootNode;
        // Hide Section if no children under the root node are found.
        return rootNode.numberOfChildren == 0 ? CGSizeZero : HeimdallLibraryCollectionViewSupplementaryElementView.defaultHeaderSize;
    }
    
    return HeimdallLibraryCollectionViewSupplementaryElementView.defaultHeaderSize;
}

- (NSSize)collectionView:(NSCollectionView *)collectionView
                  layout:(NSCollectionViewLayout *)collectionViewLayout
  sizeForItemAtIndexPath:(NSIndexPath *)indexPath
{
    HeimdallLibraryCollectionViewFlowLayout * const collectionViewFlowLayout = (HeimdallLibraryCollectionViewFlowLayout*)collectionViewLayout;
    NSAssert(collectionViewLayout, @"This should be a flow layout and thus a valid pointer");
    return [HeimdallUIUnits adjustedCollectionViewItemSizeForCollectionView:collectionView
                                                            withLayout:collectionViewFlowLayout
                                                  withItemsAspectRatio:HeimdallLibraryCollectionViewItemAspectRatioDefaultItem];
}

#pragma mark - table view data source and delegation

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    if (_mediaSourceMode == HeimdallMediaSourceModeLAN) {
        return _lanDeviceSnapshot.count;
    }

    return _mediaSources.count;
}

- (NSView *)tableView:(NSTableView *)tableView
   viewForTableColumn:(NSTableColumn *)tableColumn
                  row:(NSInteger)row
{
    if ([tableColumn.identifier isEqualToString:@"HeimdallMediaSourceTableNameColumn"]) {
        HeimdallLibraryTableCellView * const cellView =
            [tableView makeViewWithIdentifier:HeimdallLibraryTableCellViewIdentifier owner:self];

        if (_mediaSourceMode == HeimdallMediaSourceModeLAN) {
            HeimdallInputItem * const currentNodeInput = _lanDeviceSnapshot[row].inputNode.inputItem;
            NSURL * const artworkURL = currentNodeInput.artworkURL;
            NSImage * const placeholder = NSImage.HeimdallDefaultAppIconImage;
            if (currentNodeInput.radioCountryCodeForFlagArtwork) {
                cellView.representedImageView.image = placeholder;
                [HeimdallLibraryImageCache thumbnailForInputItem:currentNodeInput
                                             withCompletion:^(const NSImage * const thumbnail) {
                    cellView.representedImageView.image = (NSImage *)thumbnail;
                }];
            } else if (artworkURL) {
                [cellView.representedImageView setImageURL:artworkURL placeholderImage:placeholder];
            } else {
                cellView.representedImageView.image = placeholder;
            }
        } else {
            cellView.representedImageView.image = NSImage.HeimdallFollowImage;
        }

        NSString * const name = _mediaSourceMode == HeimdallMediaSourceModeLAN
            ? _lanDeviceSnapshot[row].inputNode.inputItem.name
            : _mediaSources[row].mediaSourceDescription;

        cellView.primaryTitleTextField.hidden = YES;
        cellView.secondaryTitleTextField.hidden = YES;
        cellView.singlePrimaryTitleTextField.hidden = NO;
        cellView.singlePrimaryTitleTextField.stringValue = name;
        return cellView;
    } else if ([tableColumn.identifier isEqualToString:@"HeimdallMediaSourceTableKindColumn"]) {
        static NSString * const basicCellViewIdentifier = @"BasicTableCellViewIdentifier";
        NSTableCellView *cellView =
            [tableView makeViewWithIdentifier:basicCellViewIdentifier owner:self];
        if (cellView == nil) {
            cellView = [NSTableCellView tableCellViewWithIdentifier:basicCellViewIdentifier
                                                      showingString:@""];
        }
        NSAssert(cellView, @"Cell view should not be nil");

        if (_mediaSourceMode == HeimdallMediaSourceModeLAN) {
            HeimdallInputItem * const currentNodeInput = _lanDeviceSnapshot[row].inputNode.inputItem;
            if (currentNodeInput.inputType == ITEM_TYPE_DIRECTORY) {
                cellView.textField.stringValue = _NS("Directory");
            }
        } else {
            HeimdallMediaSource * const mediaSource = _mediaSources[row];
            switch(mediaSource.category) {
                case SD_CAT_DEVICES:
                    cellView.textField.stringValue = _NS("Devices");
                    break;
                case SD_CAT_LAN:
                    cellView.textField.stringValue = _NS("LAN");
                    break;
                case SD_CAT_INTERNET:
                    cellView.textField.stringValue = _NS("Internet");
                    break;
                case SD_CAT_MYCOMPUTER:
                    cellView.textField.stringValue = _NS("My Computer");
                    break;
            }
        }
        return cellView;
    }
    return nil;
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
    const NSInteger selectedRow = self.tableView.selectedRow;
    if (selectedRow < 0) {
        return;
    }

    HeimdallMediaSource *mediaSource = nil;
    HeimdallInputNode *childNode = nil;
    if (_mediaSourceMode == HeimdallMediaSourceModeLAN) {
        HeimdallLANDeviceRecord * const record = _lanDeviceSnapshot[selectedRow];
        mediaSource = record.mediaSource;
        childNode = record.inputNode;
    } else {
        mediaSource = _mediaSources[selectedRow];
        childNode = mediaSource.rootNode;
    }
    NSAssert(mediaSource != nil, @"Media source should not be nil");
    NSAssert(childNode != nil, @"Child node should not be nil");

    [self configureChildDataSourceWithNode:childNode andMediaSource:mediaSource];
    [self reloadData];
}

- (id<NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row
{
    HeimdallInputItem * const inputItem = _mediaSourceMode == HeimdallMediaSourceModeLAN
        ? _lanDeviceSnapshot[row].inputNode.inputItem
        : _mediaSources[row].rootNode.inputItem;
    return [self pasteboardWriterForInputItem:inputItem];
}

- (NSDragOperation)tableView:(NSTableView *)tableView
                validateDrop:(id<NSDraggingInfo>)info
                 proposedRow:(NSInteger)row
       proposedDropOperation:(NSTableViewDropOperation)dropOperation
{
    const id propertyList = [info.draggingPasteboard propertyListForType:NSFilenamesPboardType];
    if (propertyList == nil) {
        return NSDragOperationNone;
    }

    [tableView setDropRow:-1 dropOperation:NSTableViewDropOn];
    return NSDragOperationCopy;
}

- (BOOL)tableView:(NSTableView *)tableView
       acceptDrop:(id<NSDraggingInfo>)info
              row:(NSInteger)row
    dropOperation:(NSTableViewDropOperation)dropOperation
{
    return [HeimdallFileDragRecognisingView
        handlePasteboardFromDragSessionAsPlayQueueItems:info.draggingPasteboard];
}

- (nullable id<NSPasteboardWriting>)pasteboardWriterForInputItem:(nullable HeimdallInputItem *)inputItem
{
    if (inputItem == nil || inputItem.inputType == ITEM_TYPE_DIRECTORY) {
        return nil;
    }

    return [NSPasteboardItem pasteboardItemWithInputItem:inputItem];
}

#pragma mark - LAN device snapshot

- (NSArray<HeimdallLANDeviceRecord *> *)buildMediaSourceSnapshot
{
    [_sourceRecords removeAllObjects];
    NSMutableArray<HeimdallLANDeviceRecord *> * const flat = [NSMutableArray array];
    for (HeimdallMediaSource * const mediaSource in _mediaSources) {
        NSMutableArray<HeimdallLANDeviceRecord *> * const sourceRecs = [NSMutableArray array];
        for (HeimdallInputNode * const child in mediaSource.rootNode.children) {
            HeimdallLANDeviceRecord * const record = [[HeimdallLANDeviceRecord alloc] initWithMediaSource:mediaSource
                                                                                          inputNode:child];
            [sourceRecs addObject:record];
        }
        NSArray<HeimdallLANDeviceRecord *> * const frozen = [sourceRecs copy];
        [_sourceRecords setObject:frozen forKey:mediaSource];
        [flat addObjectsFromArray:frozen];
    }
    return [flat copy];
}

- (NSArray<HeimdallLANDeviceRecord *> *)snapshotByUpdatingSource:(HeimdallMediaSource *)mediaSource
{
    NSMutableArray<HeimdallLANDeviceRecord *> * const sourceRecs = [NSMutableArray array];
    for (HeimdallInputNode * const child in mediaSource.rootNode.children) {
        HeimdallLANDeviceRecord * const record = [[HeimdallLANDeviceRecord alloc] initWithMediaSource:mediaSource
                                                                                      inputNode:child];
        [sourceRecs addObject:record];
    }
    [_sourceRecords setObject:[sourceRecs copy] forKey:mediaSource];

    NSMutableArray<HeimdallLANDeviceRecord *> * const flat = [NSMutableArray array];
    for (HeimdallMediaSource * const source in _mediaSources) {
        NSArray<HeimdallLANDeviceRecord *> * const recs = [_sourceRecords objectForKey:source];
        if (recs) {
            [flat addObjectsFromArray:recs];
        }
    }
    return [flat copy];
}

#pragma mark - glue code

- (void)configureChildDataSourceWithNode:(HeimdallInputNode *)node andMediaSource:(HeimdallMediaSource *)mediaSource
{
    if (!node || !mediaSource) {
        NSLog(@"Received bad node or media source, could not configure child data media source");
        return;
    }

    NSError * const error = [mediaSource preparseInputNodeWithinTree:node];
    if (error) {
        NSAlert * const alert = [NSAlert alertWithError:error];
        alert.alertStyle = NSAlertStyleCritical;
        [alert runModal];
        return;
    }
    
    HeimdallMediaSourceDataSource * const newChildDataSource =
        [[HeimdallMediaSourceDataSource alloc] initWithParentBaseDataSource:self];
    
    newChildDataSource.displayedMediaSource = mediaSource;
    newChildDataSource.nodeToDisplay = node;
    newChildDataSource.collectionView = self.collectionView;
    newChildDataSource.pathControl = self.pathControl;
    newChildDataSource.tableView = self.tableView;
    newChildDataSource.navigationStack = self.navigationStack;

    [self setChildDataSource:newChildDataSource];
    [self.navigationStack appendCurrentLibraryState];

    [self togglePathControlVisibility:YES];
}

- (void)setChildDataSource:(HeimdallMediaSourceDataSource *)childDataSource
{
    if (!childDataSource) {
        NSLog(@"Received bad childDataSource, returning home");
        [self returnHome];
        return;
    } else if (childDataSource == _childDataSource) {
        NSLog(@"Received same childDataSource");
        return;
    }
    
    _childDataSource = childDataSource;

    if (_mediaSourceMode == HeimdallMediaSourceModeLAN) {
        HeimdallInputNode * const node = childDataSource.nodeToDisplay;
        HeimdallInputNodePathControlItem * const nodePathItem = 
            [[HeimdallInputNodePathControlItem alloc] initWithInputNode:node];

        [self.pathControl appendInputNodePathControlItem:nodePathItem];
    }
    self.pathControl.hidden = NO;
    
    [_childDataSource setupViews];

    self.collectionView.dataSource = _childDataSource;
    self.collectionView.delegate = _childDataSource;

    self.tableView.dataSource = _childDataSource;
    self.tableView.delegate = _childDataSource;
}

#pragma mark - user interaction with generic buttons

- (void)togglePathControlVisibility:(BOOL)visible
{
    _pathControlContainerView.hidden = !visible;

    const CGFloat pathControlContainerViewHeight = _pathControlContainerView.frame.size.height;
    const CGFloat scrollViewsTopSpace = visible ? pathControlContainerViewHeight : 0;

    NSEdgeInsets scrollViewInsets = HeimdallUIUnits.libraryViewScrollViewContentInsets;
    scrollViewInsets.top += scrollViewsTopSpace;
    const NSEdgeInsets scrollerInsets = HeimdallUIUnits.libraryViewScrollViewScrollerInsets;

    _collectionViewScrollView.automaticallyAdjustsContentInsets = NO;
    _collectionViewScrollView.contentInsets = scrollViewInsets;
    _collectionViewScrollView.scrollerInsets = scrollerInsets;

    _tableViewScrollView.automaticallyAdjustsContentInsets = NO;
    _tableViewScrollView.contentInsets = NSEdgeInsetsMake(scrollViewsTopSpace + HeimdallUIUnits.libraryWindowContentSafeTopInset, 0, scrollViewInsets.bottom, 0);
    _tableViewScrollView.scrollerInsets = NSEdgeInsetsMake(0, 0, -scrollViewInsets.bottom, 0);
}

- (void)returnHome
{
    self.collectionView.dataSource = self;
    self.collectionView.delegate = self;
    self.tableView.dataSource = self;
    self.tableView.delegate = self;

    _childDataSource = nil;
    [self.pathControl clearInputNodePathControlItems];
    [self.navigationStack clear];

    [self reloadData];

    [self togglePathControlVisibility:NO];
}

- (void)homeButtonAction:(id)sender
{
    [self returnHome];
}

- (void)pathControlAction:(id)sender
{
    if (self.pathControl.clickedPathItem == nil || self.childDataSource == nil) {
        return;
    }

    NSPathControlItem * const selectedItem = self.pathControl.clickedPathItem;
    NSString * const itemNodeMrl = selectedItem.image.accessibilityDescription;

    HeimdallInputNodePathControlItem * const matchingItem = [self.pathControl.inputNodePathControlItems objectForKey:itemNodeMrl];
    if (matchingItem != nil) {
        HeimdallInputNode * const currentNode = self.childDataSource.nodeToDisplay;
        if (currentNode != nil &&
            [matchingItem.inputNode.inputItem.MRL isEqualToString:currentNode.inputItem.MRL]) {
            return;
        }

        // Jumping to an ancestor via the path control is a navigation in its own
        // right. Record it as a new state (with the trimmed breadcrumb) so that
        // pressing back returns to the descendant we were previously viewing,
        // rather than to the parent of the clicked item. The breadcrumb for each
        // state is restored wholesale on back/forward, so the path stays correct.
        self.childDataSource.nodeToDisplay = matchingItem.inputNode;
        [self.pathControl clearPathControlItemsAheadOf:selectedItem];
        [self.navigationStack appendCurrentLibraryState];
    } else {
        NSLog(@"Could not find matching item for clicked path item: %@", selectedItem);
    }
}

#pragma mark - HeimdallMediaSource Delegation

- (void)mediaSourceChildrenReset:(NSNotification *)aNotification
{
    msg_Dbg(getIntf(), "Reset nodes: %s", [[aNotification.object description] UTF8String]);
    dispatch_async(dispatch_get_main_queue(), ^{
        [self reloadDataForNotification:aNotification];
    });
}

- (void)mediaSourceChildrenAdded:(NSNotification *)aNotification
{
    msg_Dbg(getIntf(), "Received new nodes: %s", [[aNotification.object description] UTF8String]);
    dispatch_async(dispatch_get_main_queue(), ^{
        [self reloadDataForNotification:aNotification];
    });
}

- (void)mediaSourceChildrenRemoved:(NSNotification *)aNotification
{
    msg_Dbg(getIntf(), "Removed nodes: %s", [[aNotification.object description] UTF8String]);
    dispatch_async(dispatch_get_main_queue(), ^{
        [self reloadDataForNotification:aNotification];
    });
}

- (void)mediaSourcePreparingEnded:(NSNotification *)aNotification
{
    msg_Dbg(getIntf(), "Preparsing ended: %s", [[aNotification.object description] UTF8String]);
    dispatch_async(dispatch_get_main_queue(), ^{
        [self reloadDataForNotification:aNotification];
    });
}

- (void)reloadDataForNotification:(NSNotification *)aNotification
{
    if (self.mediaSourceMode == HeimdallMediaSourceModeLAN) {
        HeimdallMediaSource * const source = aNotification.object;
        if ([aNotification.name isEqualToString:HeimdallMediaSourceChildrenReset]) {
            _lanDeviceSnapshot = [self buildMediaSourceSnapshot];
        } else {
            _lanDeviceSnapshot = [self snapshotByUpdatingSource:source];
        }
    }
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [self reloadData];
    });
}

- (void)reloadData
{
    if (self.viewMode == HeimdallLibraryGridViewModeSegment) {
        [self.collectionView reloadData];
    } else {
        [self.tableView reloadData];
    }

    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallMediaSourceBaseDataSourceNodeChanged
                                                      object:self];
}

- (void)browseFolderByMrl:(NSString *)mrl
{
    heimdall_preparser_t *p_preparser = getNetworkPreparser();
    NSURL * const folderURL = [NSURL URLWithString:mrl];
    HeimdallMediaSource * const mediaSource = folderURL.isFileURL
        ? [[HeimdallLocalMediaSource alloc] initWithFolderMrl:mrl andPreparser:p_preparser]
        : [[HeimdallMediaSource alloc] initWithFolderMrl:mrl andPreparser:p_preparser];
    if (mediaSource == nil) {
        NSLog(@"Could not create valid media source for mrl: %@", mrl);
        return;
    }
    HeimdallInputNode * const entryNode = mediaSource.category == SD_CAT_LAN
        ? mediaSource.rootNode.children.firstObject
        : mediaSource.rootNode;
    if (entryNode == nil) {
        NSLog(@"No entry node for mrl: %@", mrl);
        return;
    }
    [self configureChildDataSourceWithNode:entryNode andMediaSource:mediaSource];
    [self reloadData];
}

@end
