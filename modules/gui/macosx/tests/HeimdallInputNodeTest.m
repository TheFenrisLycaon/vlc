/****************************************************************************
 * HeimdallInputNodeTest.m: Heimdall input node native tests
 ****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Claudio Cambra <developer@claudiocambra.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any later
 * version.
 *****************************************************************************/

#import <XCTest/XCTest.h>

#import "library/HeimdallInputItem.h"
#import "library/HeimdallInputNode.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"

@interface HeimdallInputNodeTest : XCTestCase
@end

@implementation HeimdallInputNodeTest

- (input_item_node_t *)newRootNodeWithRootItem:(input_item_t **)rootItem
                                       childOne:(input_item_t **)childOne
                                       childTwo:(input_item_t **)childTwo
{
    *rootItem = input_item_NewDirectory("heimdall://root", "Root", ITEM_LOCAL);
    *childOne = input_item_NewFile("file:///tmp/One.mp4", "One", HEIMDALL_TICK_FROM_SEC(1), ITEM_LOCAL);
    *childTwo = input_item_NewFile("file:///tmp/Two.mp4", "Two", HEIMDALL_TICK_FROM_SEC(2), ITEM_LOCAL);

    input_item_node_t * const rootNode = input_item_node_Create(*rootItem);
    input_item_node_AppendItem(rootNode, *childOne);
    return rootNode;
}

- (void)testRootAndChildrenMapToInputItems
{
    input_item_t *rootItem = NULL;
    input_item_t *childOne = NULL;
    input_item_t *childTwo = NULL;
    input_item_node_t * const rootNode =
        [self newRootNodeWithRootItem:&rootItem childOne:&childOne childTwo:&childTwo];
    input_item_node_AppendItem(rootNode, childTwo);

    HeimdallInputNode * const inputNode = [[HeimdallInputNode alloc] initWithInputNode:rootNode];

    XCTAssertEqual(inputNode.heimdallInputItemNode, rootNode);
    XCTAssertNotNil(inputNode.inputItem);
    XCTAssertEqualObjects(inputNode.inputItem.name, @"Root");
    XCTAssertEqual(inputNode.numberOfChildren, 2);

    NSArray<HeimdallInputNode *> * const children = inputNode.children;
    XCTAssertEqual(children.count, (NSUInteger)2);
    XCTAssertEqualObjects(children[0].inputItem.name, @"One");
    XCTAssertEqualObjects(children[1].inputItem.name, @"Two");
    XCTAssertEqualObjects(children[0].inputItem.MRL, @"file:///tmp/One.mp4");
    XCTAssertEqualObjects(children[1].inputItem.MRL, @"file:///tmp/Two.mp4");
    XCTAssertEqual(children[0].heimdallInputItemNode, rootNode->pp_children[0]);
    XCTAssertEqual(children[1].heimdallInputItemNode, rootNode->pp_children[1]);

    NSString * const description = inputNode.description;
    XCTAssertTrue([description containsString:@"Root"]);
    XCTAssertTrue([description containsString:@"number of children: 2"]);

    input_item_node_Delete(rootNode);
    input_item_Release(rootItem);
    input_item_Release(childOne);
    input_item_Release(childTwo);
}

- (void)testLeafNodeWithAnInputItemHasNoChildren
{
    input_item_t * const item =
        input_item_NewFile("file:///tmp/Leaf.mp4", "Leaf", HEIMDALL_TICK_FROM_SEC(3), ITEM_LOCAL);
    input_item_node_t * const node = input_item_node_Create(item);
    HeimdallInputNode * const inputNode = [[HeimdallInputNode alloc] initWithInputNode:node];

    XCTAssertEqual(inputNode.heimdallInputItemNode, node);
    XCTAssertEqualObjects(inputNode.inputItem.name, @"Leaf");
    XCTAssertEqual(inputNode.numberOfChildren, 0);
    XCTAssertNotNil(inputNode.children);
    XCTAssertEqual(inputNode.children.count, (NSUInteger)0);
    XCTAssertTrue([inputNode.description containsString:@"number of children: 0"]);

    input_item_node_Delete(node);
    input_item_Release(item);
}

- (void)testNestedNodesAreWrappedRecursively
{
    input_item_t * const rootItem =
        input_item_NewDirectory("heimdall://root", "Root", ITEM_LOCAL);
    input_item_t * const directoryItem =
        input_item_NewDirectory("heimdall://directory", "Directory", ITEM_LOCAL);
    input_item_t * const leafItem =
        input_item_NewFile("file:///tmp/Nested.mp4", "Nested", HEIMDALL_TICK_FROM_SEC(4), ITEM_LOCAL);

    input_item_node_t * const rootNode = input_item_node_Create(rootItem);
    input_item_node_t * const directoryNode =
        input_item_node_AppendItem(rootNode, directoryItem);
    input_item_node_t * const leafNode =
        input_item_node_AppendItem(directoryNode, leafItem);

    HeimdallInputNode * const inputNode = [[HeimdallInputNode alloc] initWithInputNode:rootNode];
    HeimdallInputNode * const directoryInputNode = inputNode.children.firstObject;
    HeimdallInputNode * const leafInputNode = directoryInputNode.children.firstObject;

    XCTAssertEqual(inputNode.numberOfChildren, 1);
    XCTAssertEqual(directoryInputNode.heimdallInputItemNode, directoryNode);
    XCTAssertEqualObjects(directoryInputNode.inputItem.name, @"Directory");
    XCTAssertEqual(directoryInputNode.numberOfChildren, 1);
    XCTAssertEqual(leafInputNode.heimdallInputItemNode, leafNode);
    XCTAssertEqualObjects(leafInputNode.inputItem.name, @"Nested");
    XCTAssertEqual(leafInputNode.numberOfChildren, 0);
    XCTAssertEqualObjects(leafInputNode.children, @[]);

    input_item_node_Delete(rootNode);
    input_item_Release(rootItem);
    input_item_Release(directoryItem);
    input_item_Release(leafItem);
}

- (void)testChildrenAreCachedUntilCleared
{
    input_item_t *rootItem = NULL;
    input_item_t *childOne = NULL;
    input_item_t *childTwo = NULL;
    input_item_node_t * const rootNode =
        [self newRootNodeWithRootItem:&rootItem childOne:&childOne childTwo:&childTwo];
    HeimdallInputNode * const inputNode = [[HeimdallInputNode alloc] initWithInputNode:rootNode];

    NSArray<HeimdallInputNode *> * const initialChildren = inputNode.children;
    XCTAssertEqual(initialChildren.count, (NSUInteger)1);
    XCTAssertEqual(inputNode.numberOfChildren, 1);

    input_item_node_AppendItem(rootNode, childTwo);
    XCTAssertEqual(inputNode.numberOfChildren, 1);
    XCTAssertEqual(inputNode.children, initialChildren);

    [inputNode clearChildrenCache];
    XCTAssertEqual(inputNode.numberOfChildren, 2);
    NSArray<HeimdallInputNode *> * const refreshedChildren = inputNode.children;
    XCTAssertNotEqual(refreshedChildren, initialChildren);
    XCTAssertEqual(refreshedChildren.count, (NSUInteger)2);
    XCTAssertEqualObjects(refreshedChildren[1].inputItem.name, @"Two");

    input_item_node_Delete(rootNode);
    input_item_Release(rootItem);
    input_item_Release(childOne);
    input_item_Release(childTwo);
}

- (void)testNodeWithoutAnInputItem
{
    input_item_node_t node = { 0 };
    HeimdallInputNode * const inputNode = [[HeimdallInputNode alloc] initWithInputNode:&node];

    XCTAssertNotNil(inputNode);
    XCTAssertEqual(inputNode.heimdallInputItemNode, &node);
    XCTAssertNil(inputNode.inputItem);
    XCTAssertEqual(inputNode.numberOfChildren, 0);
    XCTAssertEqualObjects(inputNode.children, @[]);
    XCTAssertTrue([inputNode.description containsString:@"p_item == nil"]);
}

- (void)testNilNodeHasEmptyState
{
    input_item_node_t * const nilNode = NULL;
    HeimdallInputNode * const inputNode = [[HeimdallInputNode alloc] initWithInputNode:nilNode];

    XCTAssertNotNil(inputNode);
    XCTAssertTrue(inputNode.heimdallInputItemNode == NULL);
    XCTAssertNil(inputNode.inputItem);
    XCTAssertEqual(inputNode.numberOfChildren, 0);
    XCTAssertNil(inputNode.children);
    [inputNode clearChildrenCache];
    XCTAssertNil(inputNode.children);
    XCTAssertTrue([inputNode.description containsString:@"p_item == nil"]);
}

- (void)testMalformedNodeWithMissingChildrenArrayDoesNotCrash
{
    input_item_node_t node = {
        .p_item = NULL,
        .i_children = 1,
        .pp_children = NULL,
    };
    HeimdallInputNode * const inputNode = [[HeimdallInputNode alloc] initWithInputNode:&node];

    XCTAssertEqual(inputNode.numberOfChildren, 1);
    XCTAssertEqualObjects(inputNode.children, @[]);
    XCTAssertEqual(inputNode.numberOfChildren, 0);
}

- (void)testMalformedNodeWithNullChildSlotIsWrappedSafely
{
    input_item_node_t *nullChild = NULL;
    input_item_node_t node = {
        .p_item = NULL,
        .i_children = 1,
        .pp_children = &nullChild,
    };
    HeimdallInputNode * const inputNode = [[HeimdallInputNode alloc] initWithInputNode:&node];

    NSArray<HeimdallInputNode *> * const children = inputNode.children;
    XCTAssertEqual(inputNode.numberOfChildren, 1);
    XCTAssertEqual(children.count, (NSUInteger)1);
    XCTAssertTrue(children.firstObject.heimdallInputItemNode == NULL);
    XCTAssertNil(children.firstObject.inputItem);
    XCTAssertNil(children.firstObject.children);
}

@end

#pragma clang diagnostic pop
