/*****************************************************************************
 * HeimdallLibraryMediaSourceViewNavigationStack.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Claudio Cambra <claudio.cambra@gmail.com>
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

#import "HeimdallLibraryMediaSourceViewNavigationStack.h"

#import "extensions/NSString+Helpers.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallInputNode.h"
#import "library/HeimdallInputNodePathControl.h"
#import "library/HeimdallInputNodePathControlItem.h"
#import "library/HeimdallLibraryWindow.h"

#import "library/audio-library/HeimdallLibraryAudioViewController.h"

#import "HeimdallLibraryMediaSourceViewController.h"
#import "HeimdallLibraryMediaSourceViewNavigationState.h"
#import "HeimdallMediaSource.h"
#import "HeimdallMediaSourceBaseDataSource.h"
#import "HeimdallMediaSourceDataSource.h"

@interface HeimdallLibraryMediaSourceViewNavigationCurrentStackPosition : NSObject

@property (readonly) NSUInteger navigationStackIndex;
@property (readonly) HeimdallLibraryMediaSourceViewNavigationState *navigationState;

- (instancetype)initWithStackIndex:(NSUInteger)index andState:(HeimdallLibraryMediaSourceViewNavigationState *)state;

@end

@implementation HeimdallLibraryMediaSourceViewNavigationCurrentStackPosition

- (instancetype)initWithStackIndex:(NSUInteger)index andState:(HeimdallLibraryMediaSourceViewNavigationState *)state
{
    self = [super init];

    if(self) {
        _navigationStackIndex = index;
        _navigationState = state;
    }

    return self;
}

@end


@interface HeimdallLibraryMediaSourceViewNavigationStack ()
{
    NSMutableArray<HeimdallLibraryMediaSourceViewNavigationState *> *_navigationStates;
    HeimdallLibraryMediaSourceViewNavigationCurrentStackPosition *_currentPosition;
}

@property (readwrite) NSMutableDictionary<NSURL *, HeimdallLibraryMediaSourceViewNavigationState *> *affectedPathControlStates;

@end

@implementation HeimdallLibraryMediaSourceViewNavigationStack

- (instancetype)init
{
    self = [super init];
    if (self) {
        _navigationStates = [[NSMutableArray alloc] init];
        _affectedPathControlStates = NSMutableDictionary.dictionary;
    }
    return self;
}

- (void)installHandlersOnMediaSource:(HeimdallMediaSource *)mediaSource
{
    mediaSource.willStartGeneratingChildNodesForNodeHandler = ^(input_item_node_t * const node) {
        const NSInteger rootMostAffectedState = [_navigationStates indexOfObjectPassingTest:^BOOL(HeimdallLibraryMediaSourceViewNavigationState * const _Nonnull obj, const NSUInteger __unused idx, BOOL * const __unused stop) {
            return obj.currentNodeDisplayed.heimdallInputItemNode == node;
        }];

        if (rootMostAffectedState != NSNotFound) {
            for (NSUInteger i = rootMostAffectedState + 1; i < _navigationStates.count; i++) {
                HeimdallLibraryMediaSourceViewNavigationState * const state = _navigationStates[i];
                NSURL * const url = [NSURL URLWithString:state.currentNodeDisplayed.inputItem.MRL];
                [self.affectedPathControlStates setObject:state forKey:url];
            }
        }
    };

    mediaSource.didFinishGeneratingChildNodesForNodeHandler = ^(input_item_node_t * const node) {
        for (int i = 0; i < node->i_children; i++) {
            input_item_node_t * const childNode = node->pp_children[i];
            NSURL * const url = [NSURL URLWithString:toNSStr(childNode->p_item->psz_uri)];
            HeimdallLibraryMediaSourceViewNavigationState * const affectedState = [self.affectedPathControlStates objectForKey:url];
            if (affectedState != nil) {
                affectedState.currentNodeDisplayed = [[HeimdallInputNode alloc] initWithInputNode:childNode];
                [self.affectedPathControlStates removeObjectForKey:url];
            }
        }

        // Give orphan input nodes for remaining affected path control nodes
        for (HeimdallLibraryMediaSourceViewNavigationState * const state in self.affectedPathControlStates.allValues) {
            input_item_t * const urlInputItem = input_item_NewExt(state.currentNodeDisplayed.inputItem.MRL.UTF8String,
                                                                state.currentNodeDisplayed.inputItem.name.UTF8String,
                                                                0,
                                                                ITEM_TYPE_DIRECTORY,
                                                                ITEM_LOCAL);
            if (urlInputItem != NULL) {
                input_item_node_t * const urlNode = input_item_node_Create(urlInputItem);
                if (urlNode) {
                    state.currentNodeDisplayed = [[HeimdallInputNode alloc] initWithInputNode:urlNode];
                }
                input_item_Release(urlInputItem);
            }
        }

        [self.affectedPathControlStates removeAllObjects];
    };
}

- (void)setLibraryWindow:(HeimdallLibraryWindow *)delegate
{
    _libraryWindow = delegate;
    [self updateDelegateNavigationButtons];
}

- (BOOL)forwardsAvailable
{
    NSUInteger numNavigationStates = _navigationStates.count;

    return _currentPosition != nil && 
        _currentPosition.navigationStackIndex < numNavigationStates - 1 && 
        numNavigationStates > 1;
}

- (BOOL)backwardsAvailable
{
    return _currentPosition != nil && 
        _currentPosition.navigationStackIndex > 0 && 
        _navigationStates.count > 1;
}

- (void)forwards
{
    if (self.libraryWindow == nil || !self.forwardsAvailable) {
        return;
    }

    NSUInteger newPositionIndex = _currentPosition.navigationStackIndex + 1;
    _currentPosition =
        [[HeimdallLibraryMediaSourceViewNavigationCurrentStackPosition alloc] initWithStackIndex:newPositionIndex andState:_navigationStates[newPositionIndex]];

    [self setMediaSourceViewToState:_currentPosition.navigationState];
}

- (void)backwards
{
    if (self.libraryWindow == nil || !self.backwardsAvailable) {
        return;
    }

    NSUInteger newPositionIndex = _currentPosition.navigationStackIndex - 1;
    _currentPosition =
        [[HeimdallLibraryMediaSourceViewNavigationCurrentStackPosition alloc] initWithStackIndex:newPositionIndex andState:_navigationStates[newPositionIndex]];

    [self setMediaSourceViewToState:_currentPosition.navigationState];
}

- (void)appendCurrentLibraryState
{
    if (self.libraryWindow == nil) {
        return;
    }

    if (self.forwardsAvailable) {
        NSUInteger firstIndexToRemove = _currentPosition.navigationStackIndex + 1;
        // -1 to account for the array count
        NSRange rangeToRemove = NSMakeRange(firstIndexToRemove, (_navigationStates.count - 1) - _currentPosition.navigationStackIndex);
        [_navigationStates removeObjectsInRange:rangeToRemove];
    }

    HeimdallLibraryMediaSourceViewNavigationState * const navigationState =
        [[HeimdallLibraryMediaSourceViewNavigationState alloc] initFromMediaSourceDataSource:self.baseDataSource.childDataSource];
    navigationState.pathControlItems =
        self.libraryWindow.mediaSourcePathControl.orderedInputNodePathControlItems;
    _currentPosition =
        [[HeimdallLibraryMediaSourceViewNavigationCurrentStackPosition alloc] initWithStackIndex:_navigationStates.count andState:navigationState];
    [_navigationStates addObject:navigationState];

    [self updateDelegateNavigationButtons];
}

- (void)updateDelegateNavigationButtons
{
    if (self.libraryWindow == nil) {
        return;
    }

    self.libraryWindow.forwardsNavigationButton.enabled = self.forwardsAvailable;
    self.libraryWindow.backwardsNavigationButton.enabled = self.backwardsAvailable;
}

- (void)setMediaSourceViewToState:(HeimdallLibraryMediaSourceViewNavigationState *)state
{
    [self.baseDataSource setChildDataSource:state.currentMediaSource];
    [self.baseDataSource.childDataSource setNodeToDisplay:state.currentNodeDisplayed];
    [self.libraryWindow.mediaSourcePathControl setPathWithInputNodePathControlItems:state.pathControlItems];

    [self updateDelegateNavigationButtons];
}

- (void)clear
{
    _navigationStates = [NSMutableArray array];
    _currentPosition = nil;
    [self updateDelegateNavigationButtons];
}

@end
