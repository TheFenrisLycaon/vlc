/*****************************************************************************
 * HeimdallMediaSourceDataSource.m: MacOS X interface module
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

#import "HeimdallMediaSourceDataSource.h"

#import "HeimdallLibraryMediaSourceViewNavigationStack.h"
#import "HeimdallMediaSource.h"
#import "HeimdallMediaSourceBaseDataSource.h"

#import "extensions/NSPasteboardItem+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"
#import "extensions/NSTableCellView+HeimdallAdditions.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallInputNode.h"
#import "library/HeimdallInputNodePathControl.h"
#import "library/HeimdallInputNodePathControlItem.h"
#import "library/HeimdallLibraryImageCache.h"
#import "library/HeimdallLibraryTableCellView.h"
#import "library/HeimdallLibraryWindow.h"

#import "library/media-source/HeimdallMediaSourceCollectionViewItem.h"

#import "main/HeimdallMain.h"

#import "playqueue/HeimdallPlayQueueController.h"

#import "views/HeimdallFileDragRecognisingView.h"
#import "views/HeimdallImageView.h"
#import "views/HeimdallUIUnits.h"

NSString * const HeimdallMediaSourceDataSourceNodeChanged = @"HeimdallMediaSourceDataSourceNodeChanged";
NSString * const HeimdallMediaSourceDataSourceLoadingStarted = @"HeimdallMediaSourceDataSourceLoadingStarted";
NSString * const HeimdallMediaSourceDataSourceLoadingEnded = @"HeimdallMediaSourceDataSourceLoadingEnded";

@interface HeimdallMediaSourceDataSource()
{
    HeimdallInputItem *_childRootInput;
}

@property (readwrite) dispatch_source_t observedPathDispatchSource;
@property (readwrite, strong, nullable, nonatomic) id<HeimdallMediaSourceNodeObservation> nodeObservation;
@property (readwrite, strong) NSMutableSet<NSValue *> *preparingInputItemIdentifiers;
@property (readwrite, strong) NSMutableSet<NSValue *> *preparedInputItemIdentifiers;
@property (readwrite, strong) NSMutableSet<NSValue *> *unavailableInputItemIdentifiers;
@property (readwrite, strong) NSMutableDictionary<NSValue *, NSNumber *> *childCountsByInputItemIdentifier;
@property (readwrite) dispatch_queue_t childCountQueue;

@end

static NSValue * _Nullable inputItemIdentifier(HeimdallInputItem * _Nullable const inputItem)
{
    return inputItem.heimdallInputItem == NULL
        ? nil
        : [NSValue valueWithPointer:inputItem.heimdallInputItem];
}

@implementation HeimdallMediaSourceDataSource

- (instancetype)initWithParentBaseDataSource:(HeimdallMediaSourceBaseDataSource *)parentBaseDataSource
{
    self = [super init];
    if (self) {
        self.parentBaseDataSource = parentBaseDataSource;
        self.preparingInputItemIdentifiers = NSMutableSet.set;
        self.preparedInputItemIdentifiers = NSMutableSet.set;
        self.unavailableInputItemIdentifiers = NSMutableSet.set;
        self.childCountsByInputItemIdentifier = NSMutableDictionary.dictionary;
        dispatch_queue_attr_t const childCountQueueAttributes =
            dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_UTILITY, 0);
        self.childCountQueue = dispatch_queue_create("com.lycaria.heimdall.media-source-child-count",
                                                     childCountQueueAttributes);
        NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
        [notificationCenter addObserver:self
                               selector:@selector(mediaSourceChildrenChanged:)
                                   name:HeimdallMediaSourceChildrenReset
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(mediaSourceChildrenChanged:)
                                   name:HeimdallMediaSourceChildrenAdded
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(mediaSourceChildrenChanged:)
                                   name:HeimdallMediaSourceChildrenRemoved
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(preparseStateChanged:)
                                   name:HeimdallMediaSourcePreparsingStarted
                                 object:nil];
        [notificationCenter addObserver:self
                               selector:@selector(preparseStateChanged:)
                                   name:HeimdallMediaSourcePreparsingEnded
                                 object:nil];
    }
    return self;
}

- (void)dealloc
{
    [self.nodeObservation cancel];
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)preparseStateChanged:(NSNotification *)notification
{
    if (notification.object != self.displayedMediaSource)
        return;

    HeimdallInputItem * const inputItem = notification.userInfo[HeimdallMediaSourcePreparseInputItemKey];
    NSValue * const identifier = inputItemIdentifier(inputItem);
    if (identifier == nil) {
        return;
    }

    const BOOL preparseStarted = [notification.name isEqualToString:HeimdallMediaSourcePreparsingStarted];
    if (preparseStarted) {
        [self.preparingInputItemIdentifiers addObject:identifier];
    } else {
        [self.preparingInputItemIdentifiers removeObject:identifier];

        NSNumber * const status = notification.userInfo[HeimdallMediaSourcePreparseStatusKey];
        if (status == nil || status.intValue == HEIMDALL_SUCCESS) {
            [self.preparedInputItemIdentifiers addObject:identifier];
            [self.unavailableInputItemIdentifiers removeObject:identifier];
        } else {
            [self.unavailableInputItemIdentifiers addObject:identifier];
            [self.preparedInputItemIdentifiers removeObject:identifier];
        }
    }

    NSValue * const displayedNodeIdentifier = inputItemIdentifier(self.nodeToDisplay.inputItem);
    if (![identifier isEqual:displayedNodeIdentifier]) {
        if (!preparseStarted) {
            [self reloadCountForInputItemIdentifier:identifier];
        }
        return;
    }

    if (!preparseStarted) {
        [self.nodeToDisplay clearChildrenCache];
        [self reloadData];
    }

    NSString * const loadingNotificationName = preparseStarted
        ? HeimdallMediaSourceDataSourceLoadingStarted
        : HeimdallMediaSourceDataSourceLoadingEnded;
    [NSNotificationCenter.defaultCenter postNotificationName:loadingNotificationName
                                                      object:self];
}

- (void)reloadCountForInputItemIdentifier:(NSValue *)identifier
{
    NSArray<HeimdallInputNode *> * const children = self.nodeToDisplay.children;
    const NSUInteger row = [children indexOfObjectPassingTest:^BOOL(HeimdallInputNode * const childNode,
                                                                    NSUInteger __unused idx,
                                                                    BOOL * const __unused stop) {
        return [inputItemIdentifier(childNode.inputItem) isEqual:identifier];
    }];
    if (row == NSNotFound) {
        return;
    }

    [children[row] clearChildrenCache];

    const NSInteger countColumn = [self.tableView columnWithIdentifier:@"HeimdallMediaSourceTableCountColumn"];
    if (self.tableView.hidden || countColumn == -1) {
        return;
    }

    [self.tableView reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:row]
                              columnIndexes:[NSIndexSet indexSetWithIndex:countColumn]];
}

- (void)clearDisplayedNodeChildrenCaches
{
    [self.nodeToDisplay clearChildrenCache];
    for (HeimdallInputNode * const childNode in self.nodeToDisplay.children) {
        [childNode clearChildrenCache];
    }
}

- (void)mediaSourceChildrenChanged:(NSNotification *)notification
{
    if (notification.object != self.displayedMediaSource)
        return;
    dispatch_async(dispatch_get_main_queue(), ^{
        [self clearDisplayedNodeChildrenCaches];
        [self reloadData];
    });
}

- (void)setNodeToDisplay:(nonnull HeimdallInputNode*)nodeToDisplay
{
    NSAssert(nodeToDisplay, @"Nil node to display, will not set");
    _nodeToDisplay = nodeToDisplay;

    NSParameterAssert(self.parentBaseDataSource);
    [self.nodeObservation cancel];

    const __weak typeof(self) weakSelf = self;
    self.nodeObservation = [self.displayedMediaSource observeInputNode:nodeToDisplay
                                                              onChange:^(HeimdallMediaSourceNodeChange change) {
        const typeof(self) strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        switch (change) {
            case HeimdallMediaSourceNodeChangeChildrenUpdated:
                [strongSelf reloadData];
                break;
            case HeimdallMediaSourceNodeChangeInvalidated:
                [strongSelf.parentBaseDataSource homeButtonAction:strongSelf];
                break;
        }
    }];

    [self reloadData];
    NSValue * const identifier = inputItemIdentifier(nodeToDisplay.inputItem);
    NSString * const loadingNotificationName =
        [self.preparingInputItemIdentifiers containsObject:identifier]
        ? HeimdallMediaSourceDataSourceLoadingStarted
        : HeimdallMediaSourceDataSourceLoadingEnded;
    [NSNotificationCenter.defaultCenter postNotificationName:loadingNotificationName
                                                      object:self];
}

- (BOOL)hasDisplayedItems
{
    return _nodeToDisplay.numberOfChildren > 0;
}

- (void)setupViews
{
    [self.tableView setDoubleAction:@selector(tableViewAction:)];
    [self.tableView setTarget:self];
    [self.tableView registerForDraggedTypes:@[NSFilenamesPboardType]];
    [self.tableView setDraggingSourceOperationMask:NSDragOperationCopy forLocal:NO];
    [self.tableView setDraggingSourceOperationMask:NSDragOperationCopy forLocal:YES];
}

- (nullable HeimdallInputNode *)inputNodeForIndexPath:(NSIndexPath *)indexPath
{
    HeimdallInputNode * const rootNode = self.nodeToDisplay;
    NSArray * const nodeChildren = rootNode.children;
    return nodeChildren ? nodeChildren[indexPath.item] : nil;
}

- (NSArray<HeimdallInputItem *> *)mediaSourceInputItemsAtIndexPaths:(NSSet<NSIndexPath *> *const)indexPaths
{
    NSMutableArray<HeimdallInputItem *> * const inputItems =
        [NSMutableArray arrayWithCapacity:indexPaths.count];

    for (NSIndexPath * const indexPath in indexPaths) {
        HeimdallInputNode * const inputNode = [self inputNodeForIndexPath:indexPath];
        if (!inputNode) {
            continue;
        }
        HeimdallInputItem * const inputItem = inputNode.inputItem;
        [inputItems addObject:inputItem];
    }

    return inputItems.copy;
}

#pragma mark - collection view data source and delegation

- (NSInteger)numberOfSectionsInCollectionView:(NSCollectionView *)collectionView
{
    return 1;
}

- (NSInteger)collectionView:(NSCollectionView *)collectionView
     numberOfItemsInSection:(NSInteger)section
{
    if (_nodeToDisplay) {
        return _nodeToDisplay.numberOfChildren;
    }

    return 0;
}

- (NSCollectionViewItem *)collectionView:(NSCollectionView *)collectionView
     itemForRepresentedObjectAtIndexPath:(NSIndexPath *)indexPath
{
    HeimdallMediaSourceCollectionViewItem * const viewItem =
        [collectionView makeItemWithIdentifier:HeimdallMediaSourceCollectionViewItemIdentifier
                                       forIndexPath:indexPath];

    HeimdallInputNode *rootNode = _nodeToDisplay;
    NSArray *nodeChildren = rootNode.children;
    if (nodeChildren == nil) {
        NSLog(@"No children for node %@, cannot provide correctly setup viewItem", rootNode);
        return viewItem;
    }
    HeimdallInputNode *childNode = nodeChildren[indexPath.item];
    HeimdallInputItem *childRootInput = childNode.inputItem;

    viewItem.representedItem = childRootInput;

    return viewItem;
}

- (void)collectionView:(NSCollectionView *)collectionView didSelectItemsAtIndexPaths:(NSSet<NSIndexPath *> *)indexPaths
{
    if (indexPaths.count != 1) {
        return;
    }

    NSIndexPath * const indexPath = indexPaths.anyObject;
    if (!indexPath) {
        return;
    }
    HeimdallInputNode * const childNode = [self inputNodeForIndexPath:indexPath];
    if (childNode) {
        [self performActionForNode:childNode allowPlayback:YES];
    }
}

- (NSSize)collectionView:(NSCollectionView *)collectionView
                  layout:(NSCollectionViewLayout *)collectionViewLayout
  sizeForItemAtIndexPath:(NSIndexPath *)indexPath
{
    HeimdallLibraryCollectionViewFlowLayout *collectionViewFlowLayout = (HeimdallLibraryCollectionViewFlowLayout*)collectionViewLayout;
    NSAssert(collectionViewLayout, @"This should be a flow layout and thus a valid pointer");
    return [HeimdallUIUnits adjustedCollectionViewItemSizeForCollectionView:collectionView
                                                            withLayout:collectionViewFlowLayout
                                                  withItemsAspectRatio:HeimdallLibraryCollectionViewItemAspectRatioDefaultItem];
}

#pragma mark - table view data source and delegation

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    if (_nodeToDisplay) {
        return _nodeToDisplay.numberOfChildren;
    }

    return 0;
}

- (NSView *)tableView:(NSTableView *)tableView
   viewForTableColumn:(NSTableColumn *)tableColumn
                  row:(NSInteger)row
{
    HeimdallInputNode * const inputNode = [self mediaSourceInputNodeAtRow:row];

    if ([tableColumn.identifier isEqualToString:@"HeimdallMediaSourceTableNameColumn"]) {
        HeimdallLibraryTableCellView * const cellView =
            [tableView makeViewWithIdentifier:HeimdallLibraryTableCellViewIdentifier owner:self];
        cellView.representedInputItem = inputNode.inputItem;
        cellView.primaryTitleTextField.hidden = YES;
        cellView.secondaryTitleTextField.hidden = YES;
        return cellView;
    }

     // Only present count view for folders
    if ([tableColumn.identifier isEqualToString:@"HeimdallMediaSourceTableCountColumn"] &&
        inputNode.inputItem.inputType != ITEM_TYPE_DIRECTORY) {
        return nil;
    }

    static NSString * const basicCellViewIdentifier = @"BasicTableCellViewIdentifier";
    NSTableCellView *cellView =
        [tableView makeViewWithIdentifier:basicCellViewIdentifier owner:self];
    if (cellView == nil) {
        cellView =
            [NSTableCellView tableCellViewWithIdentifier:basicCellViewIdentifier showingString:@""];
    }
    NSAssert(cellView, @"Cell view should not be nil");

    if ([tableColumn.identifier isEqualToString:@"HeimdallMediaSourceTableCountColumn"]) {
        NSValue * const identifier = inputItemIdentifier(inputNode.inputItem);
        const int numberOfChildren = inputNode.numberOfChildren;
        NSNumber * const cachedChildCount = self.childCountsByInputItemIdentifier[identifier];
        if ([self.unavailableInputItemIdentifiers containsObject:identifier]) {
            cellView.textField.stringValue = NSTR("Unavailable");
        } else if (numberOfChildren > 0 || [self.preparedInputItemIdentifiers containsObject:identifier]) {
            cellView.textField.stringValue =
                [NSString stringWithFormat:@"%i items", numberOfChildren];
        } else if (cachedChildCount != nil) {
            cellView.textField.stringValue =
                [NSString stringWithFormat:@"%li items", cachedChildCount.integerValue];
        } else {
            cellView.textField.stringValue = NSTR("Loading…");
            if (![self.preparingInputItemIdentifiers containsObject:identifier]) {
                [self.preparingInputItemIdentifiers addObject:identifier];
                HeimdallMediaSource * const mediaSource = self.displayedMediaSource;
                dispatch_async(self.childCountQueue, ^{
                    NSError *error = nil;
                    NSNumber * const childCount = [mediaSource childCountForInputNode:inputNode
                                                                                error:&error];
                    if (childCount == nil && error == nil) {
                        [mediaSource preparseInputNodeWithinTree:inputNode];
                        return;
                    }

                    dispatch_async(dispatch_get_main_queue(), ^{
                        [self.preparingInputItemIdentifiers removeObject:identifier];
                        if (error != nil) {
                            [self.unavailableInputItemIdentifiers addObject:identifier];
                        } else if (childCount != nil) {
                            [self.unavailableInputItemIdentifiers removeObject:identifier];
                            self.childCountsByInputItemIdentifier[identifier] = childCount;
                        }
                        [self reloadCountForInputItemIdentifier:identifier];
                    });
                });
            }
        }
    } else if ([tableColumn.identifier isEqualToString:@"HeimdallMediaSourceTableKindColumn"]) {
        NSString *typeName = NSTR("Unknown");
        switch (inputNode.inputItem.inputType) {
            case ITEM_TYPE_UNKNOWN:
                typeName = NSTR("Unknown");
                break;
            case ITEM_TYPE_FILE:
            {
                NSString * const filePath = inputNode.inputItem.MRL;
                NSString * const extension = filePath.pathExtension.lowercaseString;
                if (extension.length > 0) {
                    typeName = [NSString stringWithFormat:@"%@ File", extension.capitalizedString];

                    const CFStringRef extCF = (__bridge CFStringRef)extension;
                    const CFStringRef uti = UTTypeCreatePreferredIdentifierForTag(kUTTagClassFilenameExtension, extCF, NULL);
                    if (uti) {
                        CFStringRef descriptionCF = UTTypeCopyDescription(uti);
                        if (descriptionCF) {
                            typeName = CFBridgingRelease(descriptionCF);
                        }
                        CFRelease(uti);
                    }
                } else {
                    typeName = NSTR("File");
                }
                break;
            }
            case ITEM_TYPE_DIRECTORY:
                typeName = NSTR("Directory");
                break;
            case ITEM_TYPE_DISC:
                typeName = NSTR("Disc");
                break;
            case ITEM_TYPE_CARD:
                typeName = NSTR("Card");
                break;
            case ITEM_TYPE_STREAM:
                typeName = NSTR("Stream");
                break;
            case ITEM_TYPE_PLAYLIST:
                typeName = NSTR("Playlist");
                break;
            case ITEM_TYPE_NODE:
                typeName = NSTR("Node");
                break;
            case ITEM_TYPE_NUMBER:
                typeName = NSTR("Undefined");
                break;
        }
        cellView.textField.stringValue = typeName;
    } else if ([tableColumn.identifier isEqualToString:@"HeimdallMediaSourceTableTagsColumn"]) {
        static NSString * const basicCellViewIdentifier = @"BasicTableCellViewIdentifier";
        NSTableCellView *cellView = [tableView makeViewWithIdentifier:basicCellViewIdentifier
                                                                owner:self];
        if (cellView == nil) {
            cellView = [NSTableCellView tableCellViewWithIdentifier:basicCellViewIdentifier
                                                      showingString:@""];
        }

        NSArray<NSString *> * const tags = inputNode.inputItem.finderTags;
        if (tags.count > 0) {
            cellView.textField.stringValue = [tags componentsJoinedByString:@", "];
        } else {
            cellView.textField.stringValue = @"";
        }
        return cellView;
    }
    return cellView;
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
    NSInteger selectedIndex = self.tableView.selectedRow;
    if (selectedIndex < 0) {
        return;
    }

    HeimdallInputNode *childNode = [self mediaSourceInputNodeAtRow:selectedIndex];
    if (childNode) {
        [self performActionForNode:childNode allowPlayback:NO];
    }
}

- (void)tableViewAction:(id)sender
{
    NSInteger selectedIndex = self.tableView.selectedRow;
    if (selectedIndex < 0) {
        return;
    }

    HeimdallInputNode *childNode = [self mediaSourceInputNodeAtRow:selectedIndex];
    if (childNode) {
        [self performActionForNode:childNode allowPlayback:YES];
    }
}

- (id<NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row
{
    HeimdallInputItem * const inputItem = [self mediaSourceInputItemAtRow:row];
    return [self.parentBaseDataSource pasteboardWriterForInputItem:inputItem];
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

- (nullable HeimdallInputNode *)mediaSourceInputNodeAtRow:(NSInteger)tableViewRow
{
    if (_nodeToDisplay == nil) {
        return nil;
    }

    HeimdallInputNode *rootNode = _nodeToDisplay;
    NSArray *nodeChildren = rootNode.children;

    if (nodeChildren == nil || nodeChildren.count == 0) {
        return nil;
    }

    return nodeChildren[tableViewRow];
}

- (HeimdallInputItem*)mediaSourceInputItemAtRow:(NSInteger)tableViewRow
{
    HeimdallInputNode *childNode = [self mediaSourceInputNodeAtRow:tableViewRow];

    if (childNode == nil) {
        return nil;
    }

    return childNode.inputItem;
}

#pragma mark - generic actions

- (void)performActionForNode:(HeimdallInputNode *)node allowPlayback:(BOOL)allowPlayback
{
    if(node == nil || node.inputItem == nil) {
        return;
    }

    HeimdallInputItem *childRootInput = node.inputItem;

    if (childRootInput.inputType == ITEM_TYPE_DIRECTORY || childRootInput.inputType == ITEM_TYPE_NODE || childRootInput.inputType == ITEM_TYPE_PLAYLIST) {
        HeimdallInputNodePathControlItem *nodePathItem = [[HeimdallInputNodePathControlItem alloc] initWithInputNode:node];
        [self.pathControl appendInputNodePathControlItem:nodePathItem];

        NSError * const error = [self.displayedMediaSource preparseInputNodeWithinTree:node];
        if (error) {
            NSAlert * const alert = [NSAlert alertWithError:error];
            alert.alertStyle = NSAlertStyleCritical;
            [alert runModal];
            return;
        }
        self.nodeToDisplay = node;

        [self.navigationStack appendCurrentLibraryState];
    } else if (childRootInput.inputType == ITEM_TYPE_FILE && allowPlayback) {
        [HeimdallMain.sharedInstance.playQueueController addInputItem:childRootInput.heimdallInputItem atPosition:-1 startPlayback:YES];
    }
}

- (void)reloadData
{
    if (!_collectionView.hidden) {
        [_collectionView reloadData];
    }

    if(!_tableView.hidden) {
        [_tableView reloadData];
    }

    [NSNotificationCenter.defaultCenter postNotificationName:HeimdallMediaSourceDataSourceNodeChanged
                                                      object:self];
}

@end
