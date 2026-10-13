/*****************************************************************************
 * Keyring.hpp
 *****************************************************************************
 * Copyright (C) 2019 VideoLabs, VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifndef KEYRING_H
#define KEYRING_H

#include <heimdall_common.h>
#include <heimdall_threads.h>

#include <map>
#include <list>
#include <vector>
#include <string>

namespace adaptive
{
    class SharedResources;

    namespace encryption
    {
        using KeyringKey = std::vector<unsigned char>;

        class Keyring
        {
            public:
                Keyring(heimdall_object_t *);
                ~Keyring();
                KeyringKey getKey(SharedResources *, const std::string &);

            private:
                static const int MAX_KEYS = 50;
                std::map<std::string, KeyringKey> keys;
                std::list<std::string> lru;
                heimdall_object_t *obj;
                heimdall_mutex_t lock;
        };
    }
}

#endif
