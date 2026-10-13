/****************************************************************************
 * HeimdallHexNumberFormatterTest.m: HeimdallHexNumberFormatter native tests
 ****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Claudio Cambra <developer@claudiocambra.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *****************************************************************************/

#import <XCTest/XCTest.h>

#import "extensions/HeimdallHexNumberFormatter.h"

@interface HeimdallHexNumberFormatterTest : XCTestCase
@end

@implementation HeimdallHexNumberFormatterTest

- (void)testStringForObjectValue
{
    HeimdallHexNumberFormatter *formatter = [[HeimdallHexNumberFormatter alloc] init];

    XCTAssertEqualObjects([formatter stringForObjectValue:@0], @"000000");
    XCTAssertEqualObjects([formatter stringForObjectValue:@(0x12AB)], @"0012AB");
    XCTAssertNil([formatter stringForObjectValue:@"12AB"]);
    XCTAssertNil([formatter stringForObjectValue:nil]);
}

- (void)testGetObjectValue
{
    HeimdallHexNumberFormatter *formatter = [[HeimdallHexNumberFormatter alloc] init];
    id object = nil;

    XCTAssertTrue([formatter getObjectValue:&object
                                  forString:@"0012AB"
                           errorDescription:nil]);
    XCTAssertEqualObjects(object, @(0x12AB));

    object = nil;
    XCTAssertFalse([formatter getObjectValue:&object
                                   forString:@"not-hex"
                            errorDescription:nil]);
    XCTAssertNil(object);
}

@end
