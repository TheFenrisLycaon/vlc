/*****************************************************************************
 * HeimdallLibraryHomeViewActionsViewController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
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

#import "HeimdallLibraryHomeViewActionsView.h"

#import "extensions/NSString+Helpers.h"
#import "extensions/NSView+HeimdallAdditions.h"
#import "library/HeimdallLibraryWindow.h"
#import "main/HeimdallMain.h"
#import "menus/HeimdallMainMenu.h"

@implementation HeimdallLibraryHomeViewActionsView

- (void)awakeFromNib
{
    [super awakeFromNib];

    self.openFileButton.title = _NS("Open file");
    self.openDiscButton.title = _NS("Open disc");
    self.openUrlButton.title = _NS("Open stream");
    self.openCaptureDeviceButton.title = _NS("Open capture device");
    self.openBrowseButton.title = _NS("Browse");
}

- (IBAction)openFileAction:(id)sender
{
    [HeimdallMain.sharedInstance.mainMenu intfOpenFile:self];
}

- (IBAction)openDiscAction:(id)sender
{
    [HeimdallMain.sharedInstance.mainMenu intfOpenDisc:self];
}

- (IBAction)openUrlAction:(id)sender
{
    [HeimdallMain.sharedInstance.mainMenu intfOpenNet:self];
}

- (IBAction)openCaptureDeviceAction:(id)sender
{
    [HeimdallMain.sharedInstance.mainMenu intfOpenCapture:self];
}

- (IBAction)openBrowseAction:(id)sender
{
    [HeimdallMain.sharedInstance.libraryWindow goToBrowseSection:self];
}

@end
