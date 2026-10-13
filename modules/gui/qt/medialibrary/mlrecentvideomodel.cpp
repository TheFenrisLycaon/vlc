/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
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

#include "mlrecentvideomodel.hpp"
#include "mlhelper.hpp"

// Ctor / dtor

MLRecentVideoModel::MLRecentVideoModel(QObject * parent) : MLVideoModel(parent) {}

// Protected MLBaseModel implementation

std::unique_ptr<MLListCacheLoader>
MLRecentVideoModel::createMLLoader() const
/* override */
{
    return std::make_unique<MLListCacheLoader>(m_mediaLib, std::make_shared<MLRecentVideoModel::Loader>(*this));
}

// Private MLVideoModel reimplementation

void MLRecentVideoModel::onHeimdallMlEvent(const MLEvent & event) /* override */
{
    switch (event.i_type)
    {
        case HEIMDALL_ML_EVENT_HISTORY_CHANGED:
            emit resetRequested();
            return;
        default:
            break;
    }

    MLVideoModel::onHeimdallMlEvent(event);
}

// Loader

size_t MLRecentVideoModel::Loader::count(heimdall_medialibrary_t* ml, const heimdall_ml_query_params_t* queryParams) const /* override */
{
    return heimdall_ml_count_video_history(ml, queryParams);
}

std::vector<std::unique_ptr<MLItem>>
MLRecentVideoModel::Loader::load(heimdall_medialibrary_t* ml, const heimdall_ml_query_params_t* queryParams) const /* override */
{
    ml_unique_ptr<heimdall_ml_media_list_t> media_list {
        heimdall_ml_list_video_history(ml, queryParams)
    };

    if (media_list == nullptr)
        return {};

    std::vector<std::unique_ptr<MLItem>> res;

    for (const heimdall_ml_media_t & media : ml_range_iterate<heimdall_ml_media_t>(media_list))
    {
        res.emplace_back( std::make_unique<MLVideo>(&media));
    }

    return res;
}

std::unique_ptr<MLItem>
MLRecentVideoModel::Loader::loadItemById(heimdall_medialibrary_t* ml, MLItemId itemId) const
{
    assert(itemId.type == HEIMDALL_ML_PARENT_UNKNOWN);
    ml_unique_ptr<heimdall_ml_media_t> media(heimdall_ml_get_media(ml, itemId.id));
    if (!media)
        return nullptr;
    return std::make_unique<MLVideo>(media.get());
}
