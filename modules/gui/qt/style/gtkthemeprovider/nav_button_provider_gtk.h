/*****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

// original code from the Chromium project

// Copyright 2017 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 TheFenrisLycaon
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GTK_NAV_BUTTON_PROVIDER_GTK_H_
#define UI_GTK_NAV_BUTTON_PROVIDER_GTK_H_

#include "gtk_compat.h"

#include "../qtthemeprovider.hpp"

#include <map>

namespace gtk {

class NavButtonProviderGtk {
public:
    NavButtonProviderGtk();
    ~NavButtonProviderGtk();

    // views::NavButtonProvider:
    void RedrawImages(int top_area_height, bool maximized, bool active);
    HeimdallPicturePtr GetImage(heimdall_qt_theme_csd_button_type type,
                            heimdall_qt_theme_csd_button_state state) const;
    MyInset GetNavButtonMargin(
        heimdall_qt_theme_csd_button_type type) const;
    MyInset GetTopAreaSpacing() const;
    int GetInterNavButtonSpacing() const;

private:
    std::map<heimdall_qt_theme_csd_button_type,
             std::map<heimdall_qt_theme_csd_button_state, HeimdallPicturePtr>>
        button_images_;
    std::map<heimdall_qt_theme_csd_button_type, MyInset>
        button_margins_;
    MyInset top_area_spacing_;
    int inter_button_spacing_;
};

}  // namespace gtk

#endif  // UI_GTK_NAV_BUTTON_PROVIDER_GTK_H_
