/****************************************************************************
 * HeimdallLogMessageTest.m: Heimdall log message native tests
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

#import "windows/logging/HeimdallLogMessage.h"
#import <heimdall_messages.h>

@interface HeimdallLogMessageTest : XCTestCase
@end

@implementation HeimdallLogMessageTest

- (heimdall_log_t)logInfo
{
    heimdall_log_t info = { 0 };
    info.psz_module = "core";
    info.file = "input.c";
    info.line = 42;
    info.func = "OpenInput";
    return info;
}

- (void)testMessageProperties
{
    heimdall_log_t info = [self logInfo];
    char message[] = "Could not open media";

    HeimdallLogMessage * const logMessage =
        [[HeimdallLogMessage alloc] initWithMessage:message
                                           type:HEIMDALL_MSG_ERR
                                           info:&info];

    XCTAssertNotNil(logMessage);
    XCTAssertEqual(logMessage.type, HEIMDALL_MSG_ERR);
    XCTAssertEqualObjects(logMessage.message, @"Could not open media");
    XCTAssertEqualObjects(logMessage.component, @"core");
    XCTAssertEqualObjects(logMessage.function, @"OpenInput");
    XCTAssertEqualObjects(logMessage.location, @"input.c:42");
    XCTAssertEqualObjects(logMessage.typeName, @"error");
    XCTAssertEqualObjects(logMessage.fullMessage, @"core error: Could not open media");
}

- (void)testFactoryMethod
{
    heimdall_log_t info = [self logInfo];
    char message[] = "Started";

    HeimdallLogMessage * const logMessage = [HeimdallLogMessage logMessage:message
                                                              type:HEIMDALL_MSG_INFO
                                                              info:&info];

    XCTAssertNotNil(logMessage);
    XCTAssertEqualObjects(logMessage.fullMessage, @"core info: Started");
}

- (void)testTypeNames
{
    const struct {
        int type;
        NSString *name;
    } cases[] = {
        { HEIMDALL_MSG_INFO, @"info" },
        { HEIMDALL_MSG_ERR, @"error" },
        { HEIMDALL_MSG_WARN, @"warning" },
        { HEIMDALL_MSG_DBG, @"debug" },
        { 99, @"unknown" },
    };

    for (NSUInteger index = 0; index < sizeof(cases) / sizeof(cases[0]); index++) {
        heimdall_log_t info = [self logInfo];
        char message[] = "Message";
        HeimdallLogMessage * const logMessage =
            [[HeimdallLogMessage alloc] initWithMessage:message
                                               type:cases[index].type
                                               info:&info];

        XCTAssertEqualObjects(logMessage.typeName, cases[index].name);
    }
}

- (void)testEmptyMessage
{
    heimdall_log_t info = [self logInfo];
    char message[] = "";

    HeimdallLogMessage * const logMessage =
        [[HeimdallLogMessage alloc] initWithMessage:message
                                           type:HEIMDALL_MSG_INFO
                                           info:&info];

    XCTAssertNotNil(logMessage);
    XCTAssertEqualObjects(logMessage.message, @"");
    XCTAssertEqualObjects(logMessage.fullMessage, @"core info: ");
}

- (void)testRejectsMissingMessageOrLogInfo
{
    heimdall_log_t info = [self logInfo];

    XCTAssertNil([[HeimdallLogMessage alloc] initWithMessage:NULL
                                                   type:HEIMDALL_MSG_INFO
                                                   info:&info]);
    XCTAssertNil([[HeimdallLogMessage alloc] initWithMessage:(char *)"Message"
                                                   type:HEIMDALL_MSG_INFO
                                                   info:NULL]);

    info.psz_module = NULL;
    XCTAssertNil([[HeimdallLogMessage alloc] initWithMessage:(char *)"Message"
                                                   type:HEIMDALL_MSG_INFO
                                                   info:&info]);
}

@end
