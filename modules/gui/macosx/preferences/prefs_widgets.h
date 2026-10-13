/*****************************************************************************
 * prefs_widgets.h: Preferences controls
 *****************************************************************************
 * Copyright (C) 2002-2012 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Derk-Jan Hartman <hartman at videolan.org>
 *          Felix Paul Kühne <fkuehne at videolan.org>
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

static NSMenu *o_keys_menu = nil;

@interface HeimdallConfigControl : NSView

@property (readonly) module_config_t *p_item;

@property (readwrite) NSTextField *label;

@property (readonly) NSString *name;
@property (readonly) int type;
@property (readwrite) int viewType;
@property (readonly) int intValue;
@property (readonly) float floatValue;
@property (readonly) char *stringValue;
@property (readonly) int labelSize;


+ (HeimdallConfigControl *)newControl:(module_config_t *)_p_item
                        withView:(NSView *)o_parent_view;
+ (int)calcVerticalMargin:(int)i_curItem lastItem:(int)i_lastItem;

- (id)initWithFrame:(NSRect)frame
               item:(module_config_t *)p_item;

- (void)setYPos:(int)i_yPos;
- (void)alignWithXPosition:(int)i_xPos;

- (void)applyChanges;
- (void)resetValues;

@end

@interface StringConfigControl : HeimdallConfigControl

- (id)initWithItem:(module_config_t *)_p_item
          withView:(NSView *)o_parent_view;

@end

@interface StringListConfigControl : HeimdallConfigControl

- (id)initWithItem:(module_config_t *)_p_item
          withView:(NSView *)o_parent_view;

@end

@interface FileConfigControl : HeimdallConfigControl

- (id)initWithItem:(module_config_t *)_p_item
          withView:(NSView *)o_parent_view;

- (IBAction)openFileDialog:(id)sender;

@end

@interface ModuleConfigControl : HeimdallConfigControl

- (id)initWithItem:(module_config_t *)_p_item
          withView:(NSView *)o_parent_view;

@end

@interface IntegerConfigControl : HeimdallConfigControl

- (id)initWithItem:(module_config_t *)_p_item
          withView:(NSView *)o_parent_view;

- (IBAction)stepperChanged:(id)sender;

@end

@interface IntegerListConfigControl : HeimdallConfigControl

- (id)initWithItem:(module_config_t *)_p_item
          withView:(NSView *)o_parent_view;

@end

@interface RangedIntegerConfigControl : HeimdallConfigControl

- (id)initWithItem:(module_config_t *)_p_item
          withView:(NSView *)o_parent_view;

- (IBAction)sliderChanged:(id)sender;

@end

@interface BoolConfigControl : HeimdallConfigControl

- (id)initWithItem:(module_config_t *)_p_item
          withView:(NSView *)o_parent_view;

@end

@interface FloatConfigControl : HeimdallConfigControl

- (id)initWithItem:(module_config_t *)_p_item
          withView:(NSView *)o_parent_view;

- (IBAction)stepperChanged:(id)sender;

@end

@interface RangedFloatConfigControl : HeimdallConfigControl

- (id)initWithItem:(module_config_t *)_p_item
          withView:(NSView *)o_parent_view;

- (IBAction)sliderChanged:(id)sender;

@end

@interface ModuleListConfigControl : HeimdallConfigControl

- (id)initWithItem:(module_config_t *)_p_item
          withView:(NSView *)o_parent_view;

@end

@interface SectionControl : HeimdallConfigControl

- (id)initWithItem:(module_config_t *)_p_item
          withView:(NSView *)o_parent_view;

@end
