// SPDX-License-Identifier: LGPL-2.1-or-later
/*****************************************************************************
 * LazyPreparser.h: class to create a preparser at the first request
 *****************************************************************************
 * Copyright © 2025 Videolabs, VideoLAN and VLC authors
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Gabriel Lafond Thenaille <gabriel@videolabs.io>
 *****************************************************************************/

#ifndef LAZYPREPARSER_H
#define LAZYPREPARSER_H

#include <heimdall_common.h>
#include <heimdall_preparser.h>

class LazyPreparser
{
    public:
        LazyPreparser(heimdall_object_t *obj, const struct heimdall_preparser_cfg cfg)
            : m_obj(obj)
            , m_cfg(cfg)
            , m_preparser(nullptr, &heimdall_preparser_Delete)
        {
            assert(obj != NULL);
        }

        heimdall_preparser_t *instance()
        {
            heimdall::threads::mutex_locker locker(m_mutex);

            heimdall_preparser_t *preparser = m_preparser.get();
            if (preparser == nullptr) {
                m_preparser.reset(heimdall_preparser_New(m_obj, &m_cfg));
                preparser = m_preparser.get();
                if (preparser == nullptr) {
                    msg_Warn(m_obj, "LazyPreparser: Failed to instantiate a heimdall_preparser_t!");
                } else {
                    msg_Dbg(m_obj, "LazyPreparser: heimdall_preparser_t created!");
                }
            }
            return preparser;
        }

        heimdall_preparser_t *get()
        {
            heimdall::threads::mutex_locker locker(m_mutex);
            return m_preparser.get();
        }

    private:
        heimdall::threads::mutex m_mutex;
        heimdall_object_t *m_obj;
        const struct heimdall_preparser_cfg m_cfg;
        std::unique_ptr<heimdall_preparser_t, void(*)(heimdall_preparser_t*)> m_preparser;
};

#endif
