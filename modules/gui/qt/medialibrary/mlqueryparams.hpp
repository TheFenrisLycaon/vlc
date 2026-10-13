/*****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
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

#ifndef MLQUERYPARAMS_HPP
#define MLQUERYPARAMS_HPP

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <QByteArray>
#include <heimdall_media_library.h>

/**
 * C++ owned version of heimdall_ml_query_params_t, so that it can be moved or
 * copied.
 */
class MLQueryParams
{
public:
    MLQueryParams(QByteArray searchPatternUtf8, heimdall_ml_sorting_criteria_t sort,
                  bool desc, size_t index, size_t count)
        : searchPatternUtf8(std::move(searchPatternUtf8))
        , nbResults(count)
        , offset(index)
        , sort(sort)
        , desc(desc)
    {
    }

    MLQueryParams(QByteArray patternUtf8, heimdall_ml_sorting_criteria_t sort,
                  bool desc)
        : MLQueryParams(std::move(patternUtf8), sort, desc, 0, 0)
    {
    }

    /**
     * Expose the MLQueryParams content to a heimdall_ml_query_params_t.
     *
     * The returned value is valid as long as the MLQueryParams instance is
     * alive.
     */
    heimdall_ml_query_params_t toCQueryParams() const;

private:
    QByteArray searchPatternUtf8;
    uint32_t nbResults;
    uint32_t offset;
    heimdall_ml_sorting_criteria_t sort;
    bool desc;
};

#endif
