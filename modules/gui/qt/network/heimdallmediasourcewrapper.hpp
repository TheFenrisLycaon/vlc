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
#ifndef HEIMDALLMEDIASOURCEWRAPPER_HPP
#define HEIMDALLMEDIASOURCEWRAPPER_HPP

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <heimdall_media_source.h>
#include <heimdall_cxx_helpers.hpp>

using MediaSourcePtr = ::heimdall::heimdall_shared_data_ptr<heimdall_media_source_t,
                                                  &heimdall_media_source_Hold,
                                                  &heimdall_media_source_Release>;

using MediaTreePtr = ::heimdall::heimdall_shared_data_ptr<heimdall_media_tree_t,
                                                &heimdall_media_tree_Hold,
                                                &heimdall_media_tree_Release>;

class MediaTreeLocker
{
public:
    MediaTreeLocker(MediaTreePtr& tree)
        : m_tree(tree)
    {
        heimdall_media_tree_Lock(m_tree.get());
    }

    ~MediaTreeLocker() {
        heimdall_media_tree_Unlock(m_tree.get());
    }

    MediaTreeLocker( const MediaTreeLocker& ) = delete;
    MediaTreeLocker( MediaTreeLocker&& ) = delete;

    MediaTreeLocker& operator=( const MediaTreeLocker& ) = delete;
    MediaTreeLocker& operator=( MediaTreeLocker&& ) = delete;

private:
    MediaTreePtr& m_tree;
};

#endif // HEIMDALLMEDIASOURCEWRAPPER_HPP
