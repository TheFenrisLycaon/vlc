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

#include "mlartistmodel.hpp"

MLArtistModel::MLArtistModel(QObject *parent)
    : MLBaseModel(parent)
{
}

QVariant MLArtistModel::itemRoleData(const MLItem *item, const int role) const
{
    const MLArtist* ml_artist = static_cast<const MLArtist *>(item);
    assert( ml_artist );

    switch (role)
    {
    case ARTIST_ID :
        return QVariant::fromValue( ml_artist->getId() );
    case ARTIST_NAME :
        return QVariant::fromValue( ml_artist->getName() );
    case ARTIST_SHORT_BIO :
        return QVariant::fromValue( ml_artist->getShortBio() );
    case ARTIST_COVER :
        return QVariant::fromValue( ml_artist->getCover() );
    case ARTIST_NB_ALBUMS :
        return QVariant::fromValue( ml_artist->getNbAlbums() );
    case ARTIST_NB_TRACKS :
        return QVariant::fromValue( ml_artist->getNbTracks() );
    default :
        return QVariant();
    }
}

QHash<int, QByteArray> MLArtistModel::roleNames() const
{
    return {
        { ARTIST_ID, QByteArrayLiteral("id") },
        { ARTIST_NAME, QByteArrayLiteral("name") },
        { ARTIST_SHORT_BIO, QByteArrayLiteral("short_bio") },
        { ARTIST_COVER, QByteArrayLiteral("cover") },
        { ARTIST_NB_ALBUMS, QByteArrayLiteral("nb_albums") },
        { ARTIST_NB_TRACKS, QByteArrayLiteral("nb_tracks") }
    };
}

heimdall_ml_sorting_criteria_t MLArtistModel::nameToCriteria(QByteArray name) const
{
    return QHash<QByteArray, heimdall_ml_sorting_criteria_t> {
        {QByteArrayLiteral("name"), HEIMDALL_ML_SORTING_ALPHA},
        {QByteArrayLiteral("nb_tracks"), HEIMDALL_ML_SORTING_TRACKNUMBER},
    }.value(name, HEIMDALL_ML_SORTING_DEFAULT);
}

void MLArtistModel::onHeimdallMlEvent(const MLEvent &event)
{
    switch (event.i_type)
    {
        case HEIMDALL_ML_EVENT_ARTIST_ADDED:
            emit resetRequested();
            return;
        case HEIMDALL_ML_EVENT_ARTIST_UPDATED:
        {
            MLItemId itemId(event.modification.i_entity_id, HEIMDALL_ML_PARENT_UNKNOWN);
            updateItemInCache(itemId);
            return;
        }
        case HEIMDALL_ML_EVENT_ARTIST_DELETED:
        {
            MLItemId itemId(event.deletion.i_entity_id, HEIMDALL_ML_PARENT_UNKNOWN);
            deleteItemInCache(itemId);
            return;
        }
        case HEIMDALL_ML_EVENT_GENRE_DELETED:
            if ( m_parent.id != 0 && m_parent.type == HEIMDALL_ML_PARENT_GENRE &&
                 m_parent.id == event.deletion.i_entity_id )
                emit resetRequested();
            return;
    }

    MLBaseModel::onHeimdallMlEvent(event);
}

std::unique_ptr<MLListCacheLoader>
MLArtistModel::createMLLoader() const
{
    return std::make_unique<MLListCacheLoader>(m_mediaLib, std::make_shared<MLArtistModel::Loader>(*this));
}

size_t MLArtistModel::Loader::count(heimdall_medialibrary_t* ml, const heimdall_ml_query_params_t* queryParams) const
{
    if ( m_parent.id <= 0 )
        return heimdall_ml_count_artists(ml, queryParams, false);
    return heimdall_ml_count_artists_of(ml, queryParams, m_parent.type, m_parent.id );
}

std::vector<std::unique_ptr<MLItem>>
MLArtistModel::Loader::load(heimdall_medialibrary_t* ml, const heimdall_ml_query_params_t* queryParams) const
{
    ml_unique_ptr<heimdall_ml_artist_list_t> artist_list;
    if ( m_parent.id <= 0 )
        artist_list.reset( heimdall_ml_list_artists(ml, queryParams, false) );
    else
        artist_list.reset( heimdall_ml_list_artist_of(ml, queryParams, m_parent.type, m_parent.id) );
    if ( artist_list == nullptr )
        return {};
    std::vector<std::unique_ptr<MLItem>> res;
    for( const heimdall_ml_artist_t& artist: ml_range_iterate<heimdall_ml_artist_t>( artist_list ) )
        res.emplace_back( std::make_unique<MLArtist>( &artist ) );
    return res;
}

std::unique_ptr<MLItem>
MLArtistModel::Loader::loadItemById(heimdall_medialibrary_t* ml, MLItemId itemId) const
{
    assert(itemId.type == HEIMDALL_ML_PARENT_ARTIST);
    ml_unique_ptr<heimdall_ml_artist_t> artist(heimdall_ml_get_artist(ml, itemId.id));
    if (!artist)
        return nullptr;
    return std::make_unique<MLArtist>(artist.get());
}

