/****************************************************************************
 * HeimdallTimeFormatterTest.m: HeimdallTimeFormatter native tests
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

#import "views/HeimdallTimeFormatter.h"

@interface HeimdallTimeFormatterTest : XCTestCase
@end

@implementation HeimdallTimeFormatterTest

- (void)testStringForObjectValue
{
    HeimdallTimeFormatter * const formatter = [HeimdallTimeFormatter new];

    XCTAssertEqualObjects([formatter stringForObjectValue:@83], @"01:23");
    XCTAssertEqualObjects([formatter stringForObjectValue:@3601], @"1:00:01");
    XCTAssertEqualObjects([formatter stringForObjectValue:@0], @"--:--");
}

- (void)testGetObjectValueFromMinutesAndSeconds
{
    HeimdallTimeFormatter * const formatter = [HeimdallTimeFormatter new];
    id object = nil;
    NSString *error = nil;

    XCTAssertTrue([formatter getObjectValue:&object
                                  forString:@"01:23"
                           errorDescription:&error]);
    XCTAssertEqualObjects(object, @83000);
    XCTAssertNil(error);
}

- (void)testGetObjectValueFromSeconds
{
    HeimdallTimeFormatter * const formatter = [HeimdallTimeFormatter new];
    id object = nil;

    XCTAssertTrue([formatter getObjectValue:&object
                                  forString:@"83"
                           errorDescription:nil]);
    XCTAssertEqualObjects(object, @83000);
}

- (void)testGetObjectValueFromHoursMinutesAndSeconds
{
    HeimdallTimeFormatter * const formatter = [HeimdallTimeFormatter new];
    id object = nil;

    XCTAssertTrue([formatter getObjectValue:&object
                                  forString:@"1:02:03"
                           errorDescription:nil]);
    XCTAssertEqualObjects(object, @3723000);
}

- (void)testGetObjectValueRejectsInvalidComponentCount
{
    HeimdallTimeFormatter * const formatter = [HeimdallTimeFormatter new];
    id object = nil;
    NSString *error = nil;

    XCTAssertFalse([formatter getObjectValue:&object
                                   forString:@"1:2:3:4"
                            errorDescription:&error]);
    XCTAssertNil(object);
    XCTAssertNotNil(error);

    object = nil;
    error = nil;
    XCTAssertFalse([formatter getObjectValue:&object
                                   forString:@""
                            errorDescription:&error]);
    XCTAssertNil(object);
    XCTAssertNotNil(error);
}

@end
