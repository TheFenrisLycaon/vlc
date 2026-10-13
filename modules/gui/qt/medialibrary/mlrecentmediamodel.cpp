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

#include "mlrecentmediamodel.hpp"

MLRecentMediaModel::MLRecentMediaModel( QObject* parent )
    : MLMediaModel( parent )
{
}

QVariant MLRecentMediaModel::itemRoleData(const MLItem *item , int role ) const
{
    const MLMedia* media = static_cast<const MLMedia *>(item);
    if ( !media )
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
    case RECENT_MEDIA_URL:
        return QVariant::fromValue( QUrl::fromEncoded( media->mrl().toUtf8() ).toString( QUrl::PreferLocalFile | QUrl::RemovePassword ) );
    default :
        return MLMediaModel::itemRoleData(item, role);
    }
}

QHash<int, QByteArray> MLRecentMediaModel::roleNames() const
{
    QHash<int, QByteArray> hash = MLMediaModel::roleNames();

    hash.insert({
        { RECENT_MEDIA_URL, QByteArrayLiteral("url") },
    });

    return hash;
}

void MLRecentMediaModel::clearHistory()
{
    m_mediaLib->runOnMLThread(this,
    //ML thread
    [](heimdall_medialibrary_t* ml){
        heimdall_ml_clear_history(ml, HEIMDALL_ML_HISTORY_TYPE_GLOBAL);
    });
}

void MLRecentMediaModel::onHeimdallMlEvent( const MLEvent &event )
{
    switch ( event.i_type )
    {
        case HEIMDALL_ML_EVENT_HISTORY_CHANGED:
        {
            emit resetRequested();
            return;
        }
        default:
            break;
    }
    MLBaseModel::onHeimdallMlEvent( event );
}

std::unique_ptr<MLListCacheLoader>
MLRecentMediaModel::createMLLoader() const
{
    return std::make_unique<MLListCacheLoader>(m_mediaLib, std::make_shared<MLRecentMediaModel::Loader>(*this));
}

size_t MLRecentMediaModel::Loader::count(heimdall_medialibrary_t* ml, const heimdall_ml_query_params_t* queryParams) const
{
    return heimdall_ml_count_history( ml, queryParams, HEIMDALL_ML_HISTORY_TYPE_GLOBAL );
}

std::vector<std::unique_ptr<MLItem>>
MLRecentMediaModel::Loader::load(heimdall_medialibrary_t* ml, const heimdall_ml_query_params_t* queryParams) const
{
    ml_unique_ptr<heimdall_ml_media_list_t> media_list{ heimdall_ml_list_history(
                ml, queryParams, HEIMDALL_ML_HISTORY_TYPE_GLOBAL ) };
    if ( media_list == nullptr )
        return {};

    std::vector<std::unique_ptr<MLItem>> res;

    for( heimdall_ml_media_t &media: ml_range_iterate<heimdall_ml_media_t>( media_list ) )
        if ( media.i_type != HEIMDALL_ML_MEDIA_TYPE_AUDIO )
            res.emplace_back( std::make_unique<MLVideo>( &media ) );
        else if ( media.i_type == HEIMDALL_ML_MEDIA_TYPE_AUDIO )
            res.emplace_back( std::make_unique<MLAudio>( ml, &media ) );
        else
            Q_UNREACHABLE();

    return res;
}

// TODO: can we keep this as virtual and not pure virtual?
//       it's same in models derived from another models.
std::unique_ptr<MLItem>
MLRecentMediaModel::Loader::loadItemById(heimdall_medialibrary_t* ml, MLItemId itemId) const
{
    assert(itemId.type == HEIMDALL_ML_PARENT_UNKNOWN);
    ml_unique_ptr<heimdall_ml_media_t> media(heimdall_ml_get_media(ml, itemId.id));

    if (media == nullptr)
        return nullptr;

    if (media->i_type != HEIMDALL_ML_MEDIA_TYPE_AUDIO)
        return std::make_unique<MLVideo>(media.get());
    else if (media->i_type == HEIMDALL_ML_MEDIA_TYPE_AUDIO)
        return std::make_unique<MLAudio>(ml, media.get());
    else
        Q_UNREACHABLE();
    return nullptr;
}
