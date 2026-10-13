/*****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
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

#ifndef HEIMDALL_QT_MESSAGE_HANDLER_HPP
#define HEIMDALL_QT_MESSAGE_HANDLER_HPP

extern "C" {
typedef struct heimdall_object_t heimdall_object_t ;
}

void setupHeimdallQtMessageHandler(heimdall_object_t* p_intf);
void cleanupHeimdallQtMessageHandler();

//(un)register qt message handler using RAII
class HeimdallQtMessageHandlerRegisterer
{
public:
    inline HeimdallQtMessageHandlerRegisterer(heimdall_object_t* p_intf) {
        setupHeimdallQtMessageHandler(p_intf);
    }

    inline ~HeimdallQtMessageHandlerRegisterer() {
        cleanupHeimdallQtMessageHandler();
    }
};

#endif /* HEIMDALL_QT_MESSAGE_HANDLER_HPP */
