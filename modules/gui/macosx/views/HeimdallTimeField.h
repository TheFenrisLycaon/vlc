/*****************************************************************************
 * HeimdallTimeField.h: NSButton subclass for playback time fields
 *****************************************************************************
 * Copyright (C) 2003-2017 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Jon Lech Johansen <jon-vl@nanocrew.net>
 *          Felix Paul Kühne <fkuehne at lycaria dot org>
 *          Marvin Scholz <epirat07 at gmail dot com>
 *          Serhii Bykov <esphynox@gmail.com>
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

#import <Cocoa/Cocoa.h>

/*****************************************************************************
 * HeimdallTimeField interface
 *****************************************************************************
 * we need the implementation to catch our click-event in the controller window
 *****************************************************************************/

extern NSString * const HeimdallTimeFieldDisplayTimeAsElapsed;
extern NSString * const HeimdallTimeFieldDisplayTimeAsRemaining;

@interface HeimdallTimeField : NSButton

@property (readwrite, assign, nonatomic) NSString *preferencesIdentifier;
@property (readwrite, assign, nonatomic) BOOL isTimeRemaining;

- (void)setTime:(NSString *)time withRemainingTime:(NSString *)remainingTime;

@end
