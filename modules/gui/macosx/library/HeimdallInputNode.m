/****************************************************************************
 * HeimdallInputNode.m: MacOS X interface module
 ****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne # lycaria -dot- org>
 *          Claudio Cambra <developer@claudiocambra.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any later
 * version.
 *****************************************************************************/

#import "HeimdallInputNode.h"

#import "HeimdallInputItem.h"

#import "extensions/NSString+Helpers.h"

@interface HeimdallInputNode ()
{
    NSArray<HeimdallInputNode *> *_cachedChildren;
}
@end

@implementation HeimdallInputNode

- (instancetype)initWithInputNode:(struct input_item_node_t *)p_inputNode
{
    self = [super init];
    if (self && p_inputNode != NULL) {
        _heimdallInputItemNode = p_inputNode;

        if (_heimdallInputItemNode->p_item) {
            _inputItem = [[HeimdallInputItem alloc] initWithInputItem:_heimdallInputItemNode->p_item];
        }
    }
    return self;
}

- (NSString *)description
{
    NSString *inputItemName;
    if (_heimdallInputItemNode && _heimdallInputItemNode->p_item)
        inputItemName = toNSStr(_heimdallInputItemNode->p_item->psz_name);
    else
        inputItemName = @"p_item == nil";
    return [NSString stringWithFormat:@"%@: node: %p input name: %@, number of children: %i", NSStringFromClass([self class]),_heimdallInputItemNode, inputItemName, self.numberOfChildren];
}

- (void)clearChildrenCache
{
    _cachedChildren = nil;
}

- (int)numberOfChildren
{
    if (_cachedChildren) {
        return (int)_cachedChildren.count;
    }
    return _heimdallInputItemNode ? _heimdallInputItemNode->i_children : 0;
}

- (nullable NSArray<HeimdallInputNode *> *)children
{
    if (_cachedChildren) {
        return _cachedChildren;
    }

    if (_heimdallInputItemNode == NULL) {
        return nil;
    }
    NSMutableArray *mutableArray = [[NSMutableArray alloc] initWithCapacity:_heimdallInputItemNode->i_children];
    for (int i = 0; i < _heimdallInputItemNode->i_children; i++) {
        if (_heimdallInputItemNode->pp_children == NULL) {
            break;
        }
        HeimdallInputNode *inputNode = [[HeimdallInputNode alloc] initWithInputNode:_heimdallInputItemNode->pp_children[i]];
        if (inputNode) {
            [mutableArray addObject:inputNode];
        }
    }
    _cachedChildren = [mutableArray copy];
    return _cachedChildren;
}

@end
