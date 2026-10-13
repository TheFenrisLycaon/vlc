/* Copyright (C) 2003-2020 VLC authors and VideoLAN
/* Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Jon Lech Johansen <jon-vl@nanocrew.net>
 *          Felix Paul Kühne <fkuehne at lycaria dot org>
 *          Marvin Scholz <epirat07 at gmail dot com>
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
 */

#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudio/CoreAudio.h>

#import "HeimdallSystemVolume.h"
#import "main/HeimdallMain.h"

#define HeimdallSystemVolume_LogErr(...) \
    msg_Generic(getIntf(), HEIMDALL_MSG_ERR, "HeimdallSystemVolume: " __VA_ARGS__)

static const AudioObjectPropertyAddress virtualMasterPropertyAddress = {
    .mElement   = kAudioObjectPropertyElementMaster,
    .mScope     = kAudioObjectPropertyScopeOutput,
    .mSelector  = kAudioHardwareServiceDeviceProperty_VirtualMainVolume,
};

@implementation HeimdallSystemVolume

+ (AudioDeviceID)defaultOutputDevice
{
    const AudioObjectPropertyAddress defaultDevicePropertyAddress = {
        .mElement   = kAudioObjectPropertyElementMaster,
        .mScope     = kAudioObjectPropertyScopeOutput,
        .mSelector  = kAudioHardwarePropertyDefaultOutputDevice,
    };

    if (!AudioObjectHasProperty(kAudioObjectSystemObject, &defaultDevicePropertyAddress)) {
        return kAudioObjectUnknown;
    }

    AudioDeviceID deviceID;
    if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &defaultDevicePropertyAddress,
            0, NULL, &(UInt32) { sizeof(deviceID) }, &deviceID) != noErr)
    {
        return kAudioObjectUnknown;
    }
    
    return deviceID;
}

+ (BOOL)virtualMasterVolume:(Float32 *)volume forDevice:(AudioDeviceID)device
{
    NSAssert(volume != NULL, @"Argument volume may not be NULL!");

    if (!AudioObjectHasProperty(device, &virtualMasterPropertyAddress))
        return NO;

    if (AudioObjectGetPropertyData(device, &virtualMasterPropertyAddress,
            0, NULL, &(UInt32) { sizeof(*volume) }, volume) != noErr)
    {
        return NO;
    }
    return YES;
}

+ (BOOL)setVirtualMasterVolume:(Float32)volume forDevice:(AudioDeviceID)device
{
    OSStatus err;
    Boolean isSettable;

    err = AudioObjectIsPropertySettable(device, &virtualMasterPropertyAddress, &isSettable);
    if (err != noErr || !isSettable)
        return NO;

    err = AudioObjectSetPropertyData(device, &virtualMasterPropertyAddress,
            0, NULL, sizeof(volume), &volume);
    if (err != noErr )
        return NO;

    return YES;
}

+ (BOOL)changeSystemVolume:(Float32)amount
{
    AudioDeviceID deviceID = [self defaultOutputDevice];
    if (deviceID == kAudioObjectUnknown) {
        HeimdallSystemVolume_LogErr("Could not adjust system volume, failed to obtain default output device");
        return NO;
    }

    Float32 volume;
    if (![self virtualMasterVolume:&volume forDevice:deviceID]) {
        HeimdallSystemVolume_LogErr("Could not adjust system volume, failed to obtain current volume");
        return NO;
    }

    Float32 newVolume = volume + amount;
    newVolume = HEIMDALL_CLIP(newVolume, 0.0, 1.0);

    if (![self setVirtualMasterVolume:newVolume forDevice:deviceID]) {
        HeimdallSystemVolume_LogErr("Could not adjust system volume, failed to set new volume");
        return NO;
    }

    return YES;
}

@end
