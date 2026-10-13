/****************************************************************************
 * NSStringHelpersTest.m: NSString helper native tests
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
#import <heimdall_tick.h>

#import <string.h>

#import "extensions/NSString+Helpers.h"
#import <heimdall_actions.h>

@interface NSStringHelpersTest : XCTestCase
@end

@implementation NSStringHelpersTest

- (void)testNilSafeStringComparison
{
    XCTAssertTrue([NSString equalOrBothNil:nil to:nil]);
    XCTAssertFalse([NSString equalOrBothNil:nil to:@"value"]);
    XCTAssertFalse([NSString equalOrBothNil:@"value" to:nil]);
    XCTAssertTrue([NSString equalOrBothNil:@"value" to:@"value"]);
    XCTAssertFalse([NSString equalOrBothNil:@"value" to:@"other"]);
}

- (void)assertHeimdallKey:(const char *)key equalsFunctionKey:(unichar)functionKey
{
    NSString * const expected = [NSString stringWithFormat:@"%C", functionKey];
    XCTAssertEqualObjects(HeimdallKeyToString((char *)key), expected);
}

- (void)testTimeInSecondsFromStringWithColons
{
    XCTAssertEqual([NSString timeInSecondsFromStringWithColons:@"1:02:03"], 3723);
    XCTAssertEqual([NSString timeInSecondsFromStringWithColons:@"02:03"], 123);
    XCTAssertEqual([NSString timeInSecondsFromStringWithColons:@"83"], 83);
    XCTAssertEqual([NSString timeInSecondsFromStringWithColons:@"00:00:00"], 0);
    XCTAssertEqual([NSString timeInSecondsFromStringWithColons:@"1:"], 60);
    XCTAssertEqual([NSString timeInSecondsFromStringWithColons:@":1"], 1);
    XCTAssertEqual([NSString timeInSecondsFromStringWithColons:@":"], 0);
    XCTAssertEqual([NSString timeInSecondsFromStringWithColons:@" 2 : 03"], 123);
    XCTAssertEqual([NSString timeInSecondsFromStringWithColons:@"12seconds"], 12);
    XCTAssertEqual([NSString timeInSecondsFromStringWithColons:@"1:2:3:4"], 1);
    XCTAssertEqual([NSString timeInSecondsFromStringWithColons:@""], 0);
    XCTAssertEqual([NSString timeInSecondsFromStringWithColons:nil], 0);
}

- (void)testTimeFormatting
{
    XCTAssertEqualObjects([NSString stringWithTime:0], @"--:--");
    XCTAssertEqualObjects([NSString stringWithTime:-1], @"--:--");
    XCTAssertEqualObjects([NSString stringWithTime:1], @"00:01");
    XCTAssertEqualObjects([NSString stringWithTime:59], @"00:59");
    XCTAssertEqualObjects([NSString stringWithTime:60], @"01:00");
    XCTAssertEqualObjects([NSString stringWithTime:3599], @"59:59");
    XCTAssertEqualObjects([NSString stringWithTime:3600], @"1:00:00");
    XCTAssertEqualObjects([NSString stringWithTime:86400], @"24:00:00");

    XCTAssertEqualObjects([NSString stringWithTimeFromTicks:HEIMDALL_TICK_0], @"00:00");
    XCTAssertEqualObjects([NSString stringWithTimeFromTicks:HEIMDALL_TICK_FROM_SEC(1)], @"00:01");
    XCTAssertEqualObjects([NSString stringWithTimeFromTicks:HEIMDALL_TICK_FROM_SEC(60)], @"01:00");
    XCTAssertEqualObjects([NSString stringWithTimeFromTicks:HEIMDALL_TICK_FROM_SEC(3600)], @"1:00:00");
    XCTAssertEqualObjects([NSString stringWithTimeFromTicks:-HEIMDALL_TICK_FROM_SEC(1)], @"-00:01");

    XCTAssertEqualObjects([NSString stringWithTimeFromMilliseconds:-1], @"--:--");
    XCTAssertEqualObjects([NSString stringWithTimeFromMilliseconds:0], @"--:--");
    XCTAssertEqualObjects([NSString stringWithTimeFromMilliseconds:1], @"00:00");
    XCTAssertEqualObjects([NSString stringWithTimeFromMilliseconds:999], @"00:00");
    XCTAssertEqualObjects([NSString stringWithTimeFromMilliseconds:1000], @"00:01");
    XCTAssertEqualObjects([NSString stringWithTimeFromMilliseconds:60000], @"01:00");
    XCTAssertEqualObjects([NSString stringWithTimeFromMilliseconds:3600000], @"1:00:00");
}

- (void)testDurationFormatting
{
    XCTAssertEqualObjects([NSString stringWithDuration:HEIMDALL_TICK_FROM_SEC(3600)
                                            currentTime:HEIMDALL_TICK_FROM_SEC(60)
                                               negative:YES], @"-59:00");
    XCTAssertEqualObjects([NSString stringWithDuration:HEIMDALL_TICK_FROM_SEC(3600)
                                            currentTime:HEIMDALL_TICK_FROM_SEC(60)
                                               negative:NO], @"01:00");
    XCTAssertEqualObjects([NSString stringWithDuration:HEIMDALL_TICK_FROM_SEC(60)
                                            currentTime:HEIMDALL_TICK_FROM_SEC(60)
                                               negative:YES], @"-00:00");
    XCTAssertEqualObjects([NSString stringWithDuration:HEIMDALL_TICK_FROM_SEC(60)
                                            currentTime:HEIMDALL_TICK_FROM_SEC(90)
                                               negative:YES], @"-00:00");
    XCTAssertEqualObjects([NSString stringWithDuration:0
                                            currentTime:HEIMDALL_TICK_FROM_SEC(60)
                                               negative:YES], @"01:00");
}

- (void)testExtensionsArrayFromHeimdallStyleString
{
    XCTAssertEqualObjects([NSString extensionsArrayFromHeimdallStyleString:"*.mp4;*.mkv"],
                          (@[ @"mp4", @"mkv" ]));
    XCTAssertEqualObjects([NSString extensionsArrayFromHeimdallStyleString:"srt;ass"],
                          (@[ @"srt", @"ass" ]));
    XCTAssertEqualObjects([NSString extensionsArrayFromHeimdallStyleString:NULL], @[]);
    XCTAssertEqualObjects([NSString extensionsArrayFromHeimdallStyleString:""], (@[ @"" ]));
    XCTAssertEqualObjects([NSString extensionsArrayFromHeimdallStyleString:"mp4;"], (@[ @"mp4", @"" ]));
    XCTAssertEqualObjects([NSString extensionsArrayFromHeimdallStyleString:";mp4"], (@[ @"", @"mp4" ]));
    XCTAssertEqualObjects([NSString extensionsArrayFromHeimdallStyleString:"mp4;;mkv"],
                          (@[ @"mp4", @"", @"mkv" ]));
    XCTAssertEqualObjects([NSString extensionsArrayFromHeimdallStyleString:"*.tar.gz;*.M4V"],
                          (@[ @"tar.gz", @"M4V" ]));

    const char invalidUTF8[] = { (char)0xff, '\0' };
    XCTAssertNil([NSString extensionsArrayFromHeimdallStyleString:invalidUTF8]);
}

- (void)testStringWithIncrementedTrailingNumber
{
    XCTAssertEqualObjects([@"Duplicate" stringWithIncrementedTrailingNumber], @"Duplicate (2)");
    XCTAssertEqualObjects([@"Duplicate (2)" stringWithIncrementedTrailingNumber], @"Duplicate (3)");
    XCTAssertEqualObjects([@"Duplicate (09)" stringWithIncrementedTrailingNumber],
                          @"Duplicate (10)");
    XCTAssertEqualObjects([@"" stringWithIncrementedTrailingNumber], @" (2)");
    XCTAssertEqualObjects([@"Duplicate(2)" stringWithIncrementedTrailingNumber], @"Duplicate(3)");
    XCTAssertEqualObjects([@"Duplicate (0)" stringWithIncrementedTrailingNumber], @"Duplicate (1)");
    XCTAssertEqualObjects([@"Duplicate (00)" stringWithIncrementedTrailingNumber],
                          @"Duplicate (1)");
    XCTAssertEqualObjects([@"Duplicate (2) (3)" stringWithIncrementedTrailingNumber],
                          @"Duplicate (2) (4)");
    XCTAssertEqualObjects([@"Duplicate (2) " stringWithIncrementedTrailingNumber],
                          @"Duplicate (2)  (2)");
    XCTAssertEqualObjects([@"Duplicate (x)" stringWithIncrementedTrailingNumber],
                          @"Duplicate (x) (2)");
}

- (void)testFlagEmojiStringForCountryCode
{
    XCTAssertEqualObjects(flagEmojiStringForCountryCode(@"us"), @"🇺🇸");
    XCTAssertEqualObjects(flagEmojiStringForCountryCode(@"HK"), @"🇭🇰");
    XCTAssertEqualObjects(flagEmojiStringForCountryCode(@"aA"), @"🇦🇦");
    XCTAssertEqualObjects(flagEmojiStringForCountryCode(@"ZZ"), @"🇿🇿");
    XCTAssertNil(flagEmojiStringForCountryCode(@"U1"));
    XCTAssertNil(flagEmojiStringForCountryCode(@""));
    XCTAssertNil(flagEmojiStringForCountryCode(@"U"));
    XCTAssertNil(flagEmojiStringForCountryCode(@"USA"));
    XCTAssertNil(flagEmojiStringForCountryCode(@"U-") );
    XCTAssertNil(flagEmojiStringForCountryCode(@"éé"));
    XCTAssertNil(flagEmojiStringForCountryCode(nil));

    for (unichar first = 'A'; first <= 'Z'; first++) {
        for (unichar second = 'A'; second <= 'Z'; second++) {
            NSString * const code = [NSString stringWithFormat:@"%C%C", first, second];
            NSString * const flag = flagEmojiStringForCountryCode(code);
            XCTAssertNotNil(flag, @"Expected a flag for %@", code);
            XCTAssertEqual(flag.length, (NSUInteger)4, @"Unexpected UTF-16 length for %@", code);
        }
    }
}

- (void)testBase64EncodingAndDecoding
{
    XCTAssertNil([NSString base64StringWithCString:NULL]);
    XCTAssertEqualObjects([NSString base64StringWithCString:""], @"");
    XCTAssertEqualObjects([NSString base64StringWithCString:"Heimdall"], @"VkxD");
    XCTAssertEqualObjects([@"Heimdall" base64EncodedString], @"VkxD");
    XCTAssertEqualObjects([@"" base64EncodedString], @"");
    XCTAssertEqualObjects([@"😀" base64EncodedString], @"8J+YgA==");

    XCTAssertEqualObjects([@"" base64DecodedString], @"");
    XCTAssertEqualObjects([@"VkxD" base64DecodedString], @"Heimdall");
    XCTAssertEqualObjects([@"8J+YgA==" base64DecodedString], @"😀");
    XCTAssertEqualObjects([@"!" base64DecodedString], @"");
    XCTAssertEqualObjects([@"YQ$=" base64DecodedString], @"a");

    const char * const invalidUTF8Base64 = "/w==";
    XCTAssertNil([@(invalidUTF8Base64) base64DecodedString]);
    XCTAssertEqualObjects(B64DecNSStr(@"!"), @"");

    char * const input = strdup("Heimdall");
    XCTAssertEqualObjects(B64EncAndFree(input), @"VkxD");
}

- (void)testStringWrapping
{
    XCTAssertEqualObjects([@"" stringWrappedToWidth:40], @"");

    NSString * const shortString = @"Short text";
    XCTAssertEqualObjects([shortString stringWrappedToWidth:1000], shortString);
    XCTAssertEqualObjects(shortString, @"Short text");

    NSString * const longString =
        @"This is a long sentence that should wrap across multiple lines.";
    NSString * const wrappedString = [longString stringWrappedToWidth:40];
    XCTAssertTrue([wrappedString containsString:@"\n"]);
    XCTAssertEqualObjects([wrappedString stringByReplacingOccurrencesOfString:@"\n" withString:@""],
                          longString);

    NSString * const multilineString = @"first\nsecond";
    XCTAssertTrue([[multilineString stringWrappedToWidth:1000] containsString:@"\n"]);
}

- (void)testCStringConversion
{
    XCTAssertEqualObjects(toNSStr(NULL), @"");
    XCTAssertEqualObjects(toNSStr("Heimdall"), @"Heimdall");
    XCTAssertEqualObjects(toNSStr("caf\xc3\xa9"), @"café");

    const char invalidUTF8[] = { (char)0xff, '\0' };
    XCTAssertNil(toNSStr(invalidUTF8));
}

- (void)testCocoaKeyToHeimdall
{
    XCTAssertEqual(CocoaKeyToHeimdall(NSUpArrowFunctionKey), (unsigned int)KEY_UP);
    XCTAssertEqual(CocoaKeyToHeimdall(NSDownArrowFunctionKey), (unsigned int)KEY_DOWN);
    XCTAssertEqual(CocoaKeyToHeimdall(NSLeftArrowFunctionKey), (unsigned int)KEY_LEFT);
    XCTAssertEqual(CocoaKeyToHeimdall(NSRightArrowFunctionKey), (unsigned int)KEY_RIGHT);
    XCTAssertEqual(CocoaKeyToHeimdall(NSInsertFunctionKey), (unsigned int)KEY_INSERT);
    XCTAssertEqual(CocoaKeyToHeimdall(NSHomeFunctionKey), (unsigned int)KEY_HOME);
    XCTAssertEqual(CocoaKeyToHeimdall(NSEndFunctionKey), (unsigned int)KEY_END);
    XCTAssertEqual(CocoaKeyToHeimdall(NSPageUpFunctionKey), (unsigned int)KEY_PAGEUP);
    XCTAssertEqual(CocoaKeyToHeimdall(NSPageDownFunctionKey), (unsigned int)KEY_PAGEDOWN);
    XCTAssertEqual(CocoaKeyToHeimdall(NSMenuFunctionKey), (unsigned int)KEY_MENU);
    XCTAssertEqual(CocoaKeyToHeimdall(NSTabCharacter), (unsigned int)KEY_TAB);
    XCTAssertEqual(CocoaKeyToHeimdall(NSCarriageReturnCharacter), (unsigned int)KEY_ENTER);
    XCTAssertEqual(CocoaKeyToHeimdall(NSEnterCharacter), (unsigned int)KEY_ENTER);
    XCTAssertEqual(CocoaKeyToHeimdall(NSBackspaceCharacter), (unsigned int)KEY_BACKSPACE);
    XCTAssertEqual(CocoaKeyToHeimdall(NSDeleteCharacter), (unsigned int)KEY_DELETE);

    NSArray<NSNumber *> * const functionKeys = @[
        @(NSF1FunctionKey), @(NSF2FunctionKey), @(NSF3FunctionKey), @(NSF4FunctionKey),
        @(NSF5FunctionKey), @(NSF6FunctionKey), @(NSF7FunctionKey), @(NSF8FunctionKey),
        @(NSF9FunctionKey), @(NSF10FunctionKey), @(NSF11FunctionKey), @(NSF12FunctionKey)
    ];
    for (NSUInteger index = 0; index < functionKeys.count; index++) {
        XCTAssertEqual(CocoaKeyToHeimdall(functionKeys[index].unsignedShortValue),
                       (unsigned int)(KEY_F1 + (index << 16)));
    }

    XCTAssertEqual(CocoaKeyToHeimdall('A'), (unsigned int)'A');
    XCTAssertEqual(CocoaKeyToHeimdall(0), 0U);
}

- (void)testOSXStringKeyToString
{
    XCTAssertEqualObjects(OSXStringKeyToString(@"a"), @"A");
    XCTAssertEqualObjects(OSXStringKeyToString(@"Command+Shift+P"), @"⌘⇧P");
    XCTAssertEqualObjects(OSXStringKeyToString(@"Command+"), @"⌘+");
    XCTAssertEqualObjects(OSXStringKeyToString(@"Ctrl-"), @"⌃-");
    XCTAssertEqualObjects(OSXStringKeyToString(@"Right"), @"→");
    XCTAssertEqualObjects(OSXStringKeyToString(@"Left"), @"←");
    XCTAssertEqualObjects(OSXStringKeyToString(@"Page Up"), @"⇞");
    XCTAssertEqualObjects(OSXStringKeyToString(@"Page Down"), @"⇟");
    XCTAssertEqualObjects(OSXStringKeyToString(@"Up"), @"↑");
    XCTAssertEqualObjects(OSXStringKeyToString(@"Down"), @"↓");
    XCTAssertEqualObjects(OSXStringKeyToString(@"Enter"), @"↵");
    XCTAssertEqualObjects(OSXStringKeyToString(@"Tab"), @"⇥");
    XCTAssertEqualObjects(OSXStringKeyToString(@"Delete"), @"⌫");
    XCTAssertTrue([OSXStringKeyToString(@"") length] > 0);
}

- (void)testHeimdallKeyToString
{
    XCTAssertEqualObjects(HeimdallKeyToString(NULL), @"");
    XCTAssertEqualObjects(HeimdallKeyToString((char *)""), @"");
    XCTAssertEqualObjects(HeimdallKeyToString((char *)"a"), @"a");
    XCTAssertEqualObjects(HeimdallKeyToString((char *)"Command+Shift+a"), @"a");
    XCTAssertEqualObjects(HeimdallKeyToString((char *)"Command+"), @"+");
    [self assertHeimdallKey:"Page Up" equalsFunctionKey:NSPageUpFunctionKey];
    [self assertHeimdallKey:"Page Down" equalsFunctionKey:NSPageDownFunctionKey];
    [self assertHeimdallKey:"Up" equalsFunctionKey:NSUpArrowFunctionKey];
    [self assertHeimdallKey:"Down" equalsFunctionKey:NSDownArrowFunctionKey];
    [self assertHeimdallKey:"Right" equalsFunctionKey:NSRightArrowFunctionKey];
    [self assertHeimdallKey:"Left" equalsFunctionKey:NSLeftArrowFunctionKey];
    [self assertHeimdallKey:"Enter" equalsFunctionKey:NSEnterCharacter];
    [self assertHeimdallKey:"Insert" equalsFunctionKey:NSInsertFunctionKey];
    [self assertHeimdallKey:"Home" equalsFunctionKey:NSHomeFunctionKey];
    [self assertHeimdallKey:"End" equalsFunctionKey:NSEndFunctionKey];
    [self assertHeimdallKey:"Menu" equalsFunctionKey:NSMenuFunctionKey];
    [self assertHeimdallKey:"Tab" equalsFunctionKey:NSTabCharacter];
    [self assertHeimdallKey:"Backspace" equalsFunctionKey:NSBackspaceCharacter];
    [self assertHeimdallKey:"Delete" equalsFunctionKey:NSDeleteCharacter];
    [self assertHeimdallKey:"F12" equalsFunctionKey:NSF12FunctionKey];
    [self assertHeimdallKey:"F1" equalsFunctionKey:NSF1FunctionKey];
    for (NSUInteger functionNumber = 2; functionNumber <= 11; functionNumber++) {
        NSString * const key = [NSString stringWithFormat:@"F%lu", functionNumber];
        NSString * const expected = [NSString stringWithFormat:@"%C",
                                      (unichar)(NSF1FunctionKey + functionNumber - 1)];
        XCTAssertEqualObjects(HeimdallKeyToString((char *)key.UTF8String), expected);
    }
    XCTAssertEqualObjects(HeimdallKeyToString((char *)"Space"), @" ");
    XCTAssertEqualObjects(HeimdallKeyToString((char *)"Escape"), @"Escape");
}

- (void)testHeimdallModifiersToCocoa
{
    XCTAssertEqual(HeimdallModifiersToCocoa((char *)""), 0U);
    XCTAssertEqual(HeimdallModifiersToCocoa((char *)"Command"),
                   (unsigned int)NSEventModifierFlagCommand);
    XCTAssertEqual(HeimdallModifiersToCocoa((char *)"Alt"), (unsigned int)NSEventModifierFlagOption);
    XCTAssertEqual(HeimdallModifiersToCocoa((char *)"Shift"), (unsigned int)NSEventModifierFlagShift);
    XCTAssertEqual(HeimdallModifiersToCocoa((char *)"Ctrl"), (unsigned int)NSEventModifierFlagControl);
    XCTAssertEqual(HeimdallModifiersToCocoa((char *)"CommandAltShiftCtrl"),
                   (unsigned int)(NSEventModifierFlagCommand |
                                  NSEventModifierFlagOption |
                                  NSEventModifierFlagShift |
                                  NSEventModifierFlagControl));
    XCTAssertEqual(HeimdallModifiersToCocoa((char *)"Unknown"), 0U);
}

- (void)testVolumeHelpersForTemporaryDirectories
{
    NSString * const uuid = [NSUUID UUID].UUIDString;
    NSString * const path = [NSTemporaryDirectory() stringByAppendingPathComponent:uuid];
    NSFileManager * const fileManager = [[NSFileManager alloc] init];
    XCTAssertTrue([fileManager createDirectoryAtPath:path
                         withIntermediateDirectories:NO
                                          attributes:nil
                                               error:nil]);

    XCTAssertEqualObjects(getVolumeTypeFromMountPath(path), kHeimdallMediaVideoTSFolder);

    NSString * const vcdPath = [path stringByAppendingPathComponent:kHeimdallMediaVCD];
    XCTAssertTrue([fileManager createDirectoryAtPath:vcdPath
                         withIntermediateDirectories:NO
                                          attributes:nil
                                               error:nil]);
    XCTAssertEqualObjects(getVolumeTypeFromMountPath(path), kHeimdallMediaVCD);

    [fileManager removeItemAtPath:vcdPath error:nil];
    NSString * const bdmvPath = [path stringByAppendingPathComponent:kHeimdallMediaBDMVFolder];
    XCTAssertTrue([fileManager createDirectoryAtPath:bdmvPath
                         withIntermediateDirectories:NO
                                          attributes:nil
                                               error:nil]);
    XCTAssertEqualObjects(getVolumeTypeFromMountPath(path), kHeimdallMediaBDMVFolder);

    NSString * const missingPath = [path stringByAppendingPathComponent:@"missing"];
    XCTAssertEqualObjects(getVolumeTypeFromMountPath(nil), @"");
    XCTAssertEqualObjects(getVolumeTypeFromMountPath(missingPath), @"");
    XCTAssertEqualObjects(getBSDNodeFromMountPath(nil), @"");
    XCTAssertEqualObjects(getBSDNodeFromMountPath(missingPath), @"");

    [fileManager removeItemAtPath:path error:nil];
}

@end
