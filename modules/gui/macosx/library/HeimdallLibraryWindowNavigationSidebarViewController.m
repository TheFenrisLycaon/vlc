/*****************************************************************************
 * HeimdallLibraryWindowNavigationSidebarViewController.h: MacOS X interface module
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

#import "HeimdallLibraryWindowNavigationSidebarViewController.h"

#import "extensions/NSAnimationContext+HeimdallAdditions.h"
#import "extensions/NSColor+HeimdallAdditions.h"
#import "extensions/NSWindow+HeimdallAdditions.h"

#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibrarySegment.h"
#import "library/HeimdallLibrarySegmentBookmarkedLocation.h"
#import "library/HeimdallLibraryWindow.h"
#import "library/HeimdallLibraryWindowNavigationSidebarOutlineView.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallStatusNotifierView.h"
#import "views/HeimdallUIUnits.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// This needs to match whatever identifier has been set in the library window XIB
static NSString * const HeimdallLibrarySegmentCellIdentifier = @"HeimdallLibrarySegmentCellIdentifier";

@interface HeimdallLibraryWindowNavigationSidebarViewController ()

@property BOOL ignoreSegmentSelectionChanges;
@property (readonly) NSEdgeInsets scrollViewInsets;
@property (readonly) NSMutableDictionary<NSString *, dispatch_source_t> *observedPathDispatchSources;

@end

@implementation HeimdallLibraryWindowNavigationSidebarViewController

- (instancetype)initWithLibraryWindow:(HeimdallLibraryWindow *)libraryWindow
{
    self = [super initWithNibName:@"HeimdallLibraryWindowNavigationSidebarView" bundle:nil];
    if (self) {
        _libraryWindow = libraryWindow;
        _segments = HeimdallLibrarySegment.librarySegments;
        _ignoreSegmentSelectionChanges = NO;
        _observedPathDispatchSources = NSMutableDictionary.dictionary;
    }
    return self;
}

- (void)viewDidLoad
{
    _treeController = [[NSTreeController alloc] init];
    self.treeController.objectClass = HeimdallLibrarySegment.class;
    self.treeController.countKeyPath = @"childCount";
    self.treeController.childrenKeyPath = @"childNodes";
    self.treeController.leafKeyPath = @"leaf";

    self.outlineView.rowSizeStyle = NSTableViewRowSizeStyleMedium;
    self.outlineView.allowsMultipleSelection = NO;
    self.outlineView.allowsEmptySelection = NO;
    self.outlineView.allowsColumnSelection = NO;
    self.outlineView.allowsColumnReordering = NO;

    [self internalNodesChanged:nil];

    const NSEdgeInsets scrollViewInsets = self.outlineViewScrollView.contentInsets;
    _scrollViewInsets =
        NSEdgeInsetsMake(scrollViewInsets.top + HeimdallUIUnits.libraryWindowContentSafeTopInset,
                         scrollViewInsets.left,
                         scrollViewInsets.bottom,
                         scrollViewInsets.right);

    self.statusNotifierView.postsFrameChangedNotifications = YES;

    NSNotificationCenter * const defaultCenter = NSNotificationCenter.defaultCenter;
    [defaultCenter addObserver:self
                      selector:@selector(internalNodesChanged:)
                          name:HeimdallLibraryBookmarkedLocationsChanged
                        object:nil];
    [defaultCenter addObserver:self
                      selector:@selector(internalNodesChanged:)
                          name:HeimdallLibraryModelListOfGroupsReset
                        object:nil];
    [defaultCenter addObserver:self
                      selector:@selector(internalNodesChanged:)
                          name:HeimdallLibraryModelGroupUpdated
                        object:nil];
    [defaultCenter addObserver:self
                      selector:@selector(internalNodesChanged:)
                          name:HeimdallLibraryModelGroupDeleted
                        object:nil];
    [defaultCenter addObserver:self
                      selector:@selector(statusViewActivated:)
                          name:HeimdallStatusNotifierViewActivated
                        object:nil];
    [defaultCenter addObserver:self
                      selector:@selector(statusViewDeactivated:)
                          name:HeimdallStatusNotifierViewDeactivated
                        object:nil];
    [defaultCenter addObserver:self
                      selector:@selector(statusViewSizeChanged:)
                          name:NSViewFrameDidChangeNotification
                        object:nil];
}

- (void)internalNodesChanged:(NSNotification *)notification
{
    const HeimdallLibrarySegmentType currentSegmentType = self.libraryWindow.librarySegmentType;

    NSMutableSet<NSNumber *> * const expandedSegmentTypes = NSMutableSet.set;
    for (HeimdallLibrarySegment * const segment in self.treeController.content) {
        NSTreeNode * const node = [self nodeForSegmentType:segment.segmentType];
        if ([self.outlineView isItemExpanded:node]) {
            [expandedSegmentTypes addObject:@(segment.segmentType)];
        }
    }

    self.ignoreSegmentSelectionChanges = YES;

    _segments = HeimdallLibrarySegment.librarySegments;
    [self.treeController bind:@"contentArray" toObject:self withKeyPath:@"segments" options:nil];
    [self.outlineView bind:@"content"
                  toObject:self.treeController
               withKeyPath:@"arrangedObjects"
                   options:nil];
    [self.outlineView reloadData];

    self.ignoreSegmentSelectionChanges = NO;

    [self updateBookmarkObservation];

    for (HeimdallLibrarySegment * const segment in self.treeController.content) {
        if ([expandedSegmentTypes containsObject:@(segment.segmentType)]) {
            NSTreeNode * const node = [self nodeForSegmentType:segment.segmentType];
            [self.outlineView expandItem:node];
        }
    }

    [self selectSegment:currentSegmentType];
}

- (void)updateBookmarkObservation
{
    NSUserDefaults * const defaults = NSUserDefaults.standardUserDefaults;
    NSArray<NSString *> * const bookmarkedLocations =
        [defaults stringArrayForKey:HeimdallLibraryBookmarkedLocationsKey];
    if (bookmarkedLocations.count == 0) {
        return;
    }

    NSMutableArray<NSString *> * const deletedLocations = self.observedPathDispatchSources.allKeys.mutableCopy;
    const __weak typeof(self) weakSelf = self;

    for (NSString * const locationMrl in bookmarkedLocations) {
        [deletedLocations removeObject:locationMrl];
        if ([self.observedPathDispatchSources objectForKey:locationMrl] != nil) {
            continue;
        }
        NSURL * const locationUrl = [NSURL URLWithString:locationMrl];
        const uintptr_t descriptor = open(locationUrl.path.UTF8String, O_EVTONLY);
        if (descriptor == (uintptr_t)-1) {
            continue;
        }
        struct stat fileStat;
        const int statResult = fstat(descriptor, &fileStat);

        const dispatch_queue_t globalQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
        const dispatch_source_t fileDispatchSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_VNODE, descriptor, DISPATCH_VNODE_DELETE | DISPATCH_VNODE_RENAME, globalQueue);
        dispatch_source_set_event_handler(fileDispatchSource, ^{
            const unsigned long eventFlags = dispatch_source_get_data(fileDispatchSource);
            if (eventFlags & DISPATCH_VNODE_RENAME && statResult != -1) {
                NSURL * const parentLocationUrl = locationUrl.URLByDeletingLastPathComponent;
                NSString * const parentLocationPath = parentLocationUrl.path;
                NSArray<NSString *> * const files = [NSFileManager.defaultManager contentsOfDirectoryAtPath:parentLocationPath error:nil];
                NSString *newFileName = nil;

                for (NSString * const file in files) {
                    NSString * const fullChildPath = [parentLocationPath stringByAppendingPathComponent:file];
                    struct stat currentFileStat;
                    if (stat(fullChildPath.UTF8String, &currentFileStat) == -1) {
                        continue;
                    } else if (currentFileStat.st_ino == fileStat.st_ino) {
                        newFileName = fullChildPath.lastPathComponent;
                        break;
                    }
                }

                if (newFileName != nil) {
                    NSMutableArray<NSString *> * const mutableBookmarkedLocations = bookmarkedLocations.mutableCopy;
                    const NSUInteger locationIndex = [mutableBookmarkedLocations indexOfObject:locationMrl];
                    NSString * const newLocationMrl = [parentLocationUrl URLByAppendingPathComponent:newFileName].absoluteString;
                    [mutableBookmarkedLocations replaceObjectAtIndex:locationIndex withObject:newLocationMrl];
                    [defaults setObject:mutableBookmarkedLocations forKey:HeimdallLibraryBookmarkedLocationsKey];
                }
            }
            dispatch_async(dispatch_get_main_queue(), ^{
                [weakSelf internalNodesChanged:nil];
            });
        });
        dispatch_source_set_cancel_handler(fileDispatchSource, ^{
            close(descriptor);
        });
        dispatch_resume(fileDispatchSource);
        [self.observedPathDispatchSources setObject:fileDispatchSource forKey:locationMrl];
    }

    [self.observedPathDispatchSources removeObjectsForKeys:deletedLocations];
}

- (void)statusViewActivated:(NSNotification *)notification
{
    const CGFloat statusNotifierHeight = self.statusNotifierView.frame.size.height;
    self.outlineViewScrollView.contentInsets =
        NSEdgeInsetsMake(self.scrollViewInsets.top,
                         self.scrollViewInsets.left,
                         self.scrollViewInsets.bottom + statusNotifierHeight,
                         self.scrollViewInsets.right);
    self.statusNotifierView.hidden = NO;

    [NSAnimationContext runAnimationRespectingPreferencesWithChanges:^(NSAnimationContext * const __unused context) {
        self.statusNotifierView.animator.alphaValue = 1.0;
    } completionHandler:nil];
}

- (void)statusViewDeactivated:(NSNotification *)notification
{
    [NSAnimationContext runAnimationRespectingPreferencesWithChanges:^(NSAnimationContext * const __unused context) {
        self.statusNotifierView.animator.alphaValue = 0.0;
    } completionHandler:^{
        self.statusNotifierView.hidden = YES;
        self.outlineViewScrollView.contentInsets = self.scrollViewInsets;
    }];
}

- (void)statusViewSizeChanged:(NSNotification *)notification
{
    if (self.statusNotifierView.hidden) {
        return;
    }

    const CGFloat statusNotifierHeight = self.statusNotifierView.frame.size.height;
    self.outlineViewScrollView.contentInsets =
        NSEdgeInsetsMake(self.scrollViewInsets.top,
                         self.scrollViewInsets.left,
                         self.scrollViewInsets.bottom + statusNotifierHeight,
                         self.scrollViewInsets.right);
}

- (NSTreeNode *)nodeForSegmentType:(HeimdallLibrarySegmentType)segmentType
{
    NSArray<NSTreeNode *> *nodes = self.treeController.arrangedObjects.childNodes;
    while (nodes.count != 0) {
        NSMutableArray<NSTreeNode *> * const nextLevelNodes = NSMutableArray.array;

        const NSInteger nodeIdx = [nodes indexOfObjectPassingTest:^BOOL(NSTreeNode * const obj,
                                                                        const NSUInteger __unused idx,
                                                                        BOOL * const __unused stop) {
            HeimdallLibrarySegment * const segment = obj.representedObject;
            const BOOL matching = segment.segmentType == segmentType;
            if (!matching) {
                [nextLevelNodes addObjectsFromArray:obj.childNodes];
            }
            return matching;
        }];

        if (nodeIdx != NSNotFound) {
            return nodes[nodeIdx];
        }

        nodes = nextLevelNodes.copy;
    }
    return nil;
}

- (void)expandParentsOfNode:(NSTreeNode *)targetNode
{
    NSMutableArray * const parentNodes = NSMutableArray.array;
    NSTreeNode *currentNode = targetNode.parentNode;
    while (currentNode != nil) {
        [parentNodes insertObject:currentNode atIndex:0];
        currentNode = currentNode.parentNode;
    }

    for (NSTreeNode * const node in parentNodes) {
        [self.outlineView expandItem:node];
    }
}

- (void)selectSegment:(NSInteger)segmentType
{
    NSAssert(segmentType > HeimdallLibraryLowSentinelSegment &&
             segmentType < HeimdallLibraryHighSentinelSegment,
             @"Invalid segment type value provided");

    if (segmentType == HeimdallLibraryHeaderSegmentType) {
        return;
    }

    HeimdallLibrarySegment * const segment = [HeimdallLibrarySegment segmentWithSegmentType:segmentType];
    if (segment.mediaLibraryRequired &&
        HeimdallMain.sharedInstance.libraryController.libraryModel == nil) {
        segmentType = HeimdallLibraryBrowseSegmentType;
    }

    NSTreeNode *targetNode = [self nodeForSegmentType:segmentType];
    if (targetNode == nil) {
        // Fallback to Browse if the segment is not available
        segmentType = HeimdallLibraryBrowseSegmentType;
        targetNode = [self nodeForSegmentType:segmentType];
    }

    self.libraryWindow.librarySegmentType = segmentType;

    if (segmentType == HeimdallLibraryMusicSegmentType) {
        [self.outlineView expandItem:[self nodeForSegmentType:HeimdallLibraryMusicSegmentType]];
    }

    [self expandParentsOfNode:targetNode];
    const NSInteger segmentIndex = [self.outlineView rowForItem:targetNode];
    [self.outlineView selectRowIndexes:[NSIndexSet indexSetWithIndex:segmentIndex]
                  byExtendingSelection:NO];
}

# pragma mark - NSOutlineView delegation

- (NSView *)outlineView:(NSOutlineView *)outlineView
     viewForTableColumn:(NSTableColumn *)tableColumn
                   item:(id)item
{
    NSAssert(outlineView == _outlineView, @"HeimdallLibraryWindowNavigationSidebarController should only be a delegate for the libraryWindow nav sidebar outline view!");

    const BOOL isHeader = [self outlineView:outlineView isGroupItem:item];
    NSTableCellView * const cellView = isHeader
        ? [outlineView makeViewWithIdentifier:@"HeimdallLibrarySegmentHeaderCellIdentifier" owner:self]
        : [outlineView makeViewWithIdentifier:@"HeimdallLibrarySegmentCellIdentifier" owner:self];
    NSAssert(cellView != nil, @"Provided cell view for navigation outline view should be valid!");
    [cellView.textField bind:NSValueBinding toObject:cellView withKeyPath:@"objectValue.displayString" options:nil];
    [cellView.imageView bind:NSImageBinding toObject:cellView withKeyPath:@"objectValue.displayImage" options:nil];

    if (@available(macOS 10.14, *)) {
        cellView.imageView.contentTintColor = NSColor.HeimdallAccentColor;
    }

    NSTreeNode * const treeNode = (NSTreeNode *)item;
    HeimdallLibrarySegment * const segment = (HeimdallLibrarySegment *)treeNode.representedObject;
    const BOOL mediaLibraryUnavailable =
        segment.mediaLibraryRequired &&
        HeimdallMain.sharedInstance.libraryController.libraryModel == nil;
    if (mediaLibraryUnavailable) {
        cellView.textField.textColor = NSColor.tertiaryLabelColor;
        if (@available(macOS 10.14, *)) {
            cellView.imageView.contentTintColor = NSColor.tertiaryLabelColor;
        }
    }

    return cellView;
}

- (NSIndexSet *)outlineView:(NSOutlineView *)outlineView selectionIndexesForProposedSelection:(nonnull NSIndexSet *)proposedSelectionIndexes
{
    NSAssert(outlineView == _outlineView, @"HeimdallLibraryWindowNavigationSidebarController should only be a delegate for the libraryWindow nav sidebar outline view!");

    if (proposedSelectionIndexes.count > 0) {
        NSTreeNode * const node = [self.outlineView itemAtRow:proposedSelectionIndexes.firstIndex];
        HeimdallLibrarySegment * const segment = (HeimdallLibrarySegment *)node.representedObject;

        if (segment.segmentType == HeimdallLibraryHeaderSegmentType) {
            return NSIndexSet.indexSet;
        }

        const BOOL mediaLibraryUnavailable =
            segment.mediaLibraryRequired &&
            HeimdallMain.sharedInstance.libraryController.libraryModel == nil;
        if (mediaLibraryUnavailable) {
            return [outlineView selectedRowIndexes];
        }
    }

    return proposedSelectionIndexes;
}

- (void)outlineViewSelectionDidChange:(NSNotification *)notification
{
    if (self.ignoreSegmentSelectionChanges) {
        return;
    }
    
    NSTreeNode * const node = (NSTreeNode *)[_outlineView itemAtRow:_outlineView.selectedRow];
    NSParameterAssert(node != nil);
    HeimdallLibrarySegment * const segment = (HeimdallLibrarySegment *)node.representedObject;
    NSObject * const representedObject = segment.representedObject;
    NSParameterAssert(representedObject != nil);

    if (segment.segmentType == HeimdallLibraryMusicSegmentType) {
        [self.outlineView expandItem:[self nodeForSegmentType:HeimdallLibraryMusicSegmentType]];
        NSTreeNode * const artistsNode = [self nodeForSegmentType:HeimdallLibraryArtistsMusicSubSegmentType];
        const NSInteger artistsIndex = [self.outlineView rowForItem:artistsNode];
        [self.outlineView selectRowIndexes:[NSIndexSet indexSetWithIndex:artistsIndex]
                      byExtendingSelection:NO];
        return;
    } else if ([representedObject isKindOfClass:NSNumber.class]) {
        self.libraryWindow.librarySegmentType = segment.segmentType;
    } else if ([representedObject isKindOfClass:HeimdallLibrarySegmentBookmarkedLocation.class]) {
        HeimdallLibrarySegmentBookmarkedLocation * const bookmarkedLocation =
            (HeimdallLibrarySegmentBookmarkedLocation *)representedObject;
        self.libraryWindow.librarySegmentType = bookmarkedLocation.segmentType;
        [self.libraryWindow browseFolderByMrl:bookmarkedLocation.mrl];
    } else if ([representedObject isKindOfClass:HeimdallMediaLibraryGroup.class]) {
        [self.libraryWindow presentLibraryItem:(HeimdallMediaLibraryGroup *)representedObject];
    }
    
    // Refresh disclosure triangles when selection changes
    [(HeimdallLibraryWindowNavigationSidebarOutlineView *)self.outlineView refreshDisclosureCaret];
}

- (BOOL)outlineView:(NSOutlineView*)outlineView isGroupItem:(id)item
{
    NSTreeNode * const treeNode = (NSTreeNode *)item;
    HeimdallLibrarySegment * const segment = (HeimdallLibrarySegment *)treeNode.representedObject;
    return segment.segmentType == HeimdallLibraryHeaderSegmentType;
}

- (BOOL)outlineView:(nonnull NSOutlineView *)outlineView shouldCollapseItem:(nonnull id)item
{
    NSAssert(outlineView == _outlineView, @"HeimdallLibraryWindowNavigationSidebarController should only be a delegate for the libraryWindow nav sidebar outline view!");

    // Don't allow collapsing the parent segment of the selected segment, if selection is a child
    NSTreeNode * const treeNode = (NSTreeNode *)item;
    HeimdallLibrarySegment * const segment = (HeimdallLibrarySegment *)treeNode.representedObject;
    NSTreeNode * const selectedSegmentItem = (NSTreeNode *)[self.outlineView itemAtRow:self.outlineView.selectedRow];
    HeimdallLibrarySegment * const selectedSegment = (HeimdallLibrarySegment *)selectedSegmentItem.representedObject;
    const NSInteger childNodeIndex = [segment.childNodes indexOfObjectPassingTest:^BOOL(NSTreeNode * const _Nonnull obj, const NSUInteger __unused idx, BOOL * const _Nonnull __unused stop) {
        HeimdallLibrarySegment * const childSegment = (HeimdallLibrarySegment *)obj;
        return childSegment.segmentType == selectedSegment.segmentType;
    }];
    return childNodeIndex == NSNotFound;
}

@end
