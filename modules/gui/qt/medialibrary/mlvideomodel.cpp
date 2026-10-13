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

#include "mlvideomodel.hpp"

MLVideoModel::MLVideoModel(QObject* parent)
    : MLMediaModel(parent)
{
}

// Interface

void MLVideoModel::setItemPlayed(const QModelIndex & index, bool played)
{
    MLVideo * video = static_cast<MLVideo *>(item(index.row()));

    if (video == nullptr)
        return;

    assert(m_mediaLib);

    int64_t id = video->getId().id;

    // ML thread
    m_mediaLib->runOnMLThread(this, [id, played] (heimdall_medialibrary_t * ml)
    {
        heimdall_ml_media_set_played(ml, id, played);
    });

    // NOTE: We want the change to be visible right away.
    video->setIsNew(played == false);

    emit dataChanged(index, index, { VIDEO_IS_NEW });
}

QVariant MLVideoModel::itemRoleData(const MLItem *item, int role) const
{
    const auto video = static_cast<const MLVideo *>(item);
    if ( video == nullptr )
        return {};

    switch (role)
    {
    case VIDEO_THUMBNAIL: // TODO: remove (a similar role already MEDIA_SMALL_COVER)
        return MLMediaModel::itemRoleData(item, MLMediaModel::MEDIA_SMALL_COVER);
    case VIDEO_IS_NEW:
        return QVariant::fromValue(video->isNew());
    case VIDEO_RESOLUTION:
        return QVariant::fromValue(video->getResolutionName());
    case VIDEO_CHANNEL:
        return QVariant::fromValue(video->getChannel());
    case VIDEO_DISPLAY_MRL:
        return QVariant::fromValue(video->getDisplayMRL());
    case VIDEO_VIDEO_TRACK:
        return getVariantList(video->getVideoDesc());
    case VIDEO_AUDIO_TRACK:
        return getVariantList(video->getAudioDesc());
    case VIDEO_SUBTITLE_TRACK:
        return getVariantList(video->getSubtitleDesc());
    default:
        return MLMediaModel::itemRoleData(item, role);
    }

    return {};
}

QHash<int, QByteArray> MLVideoModel::roleNames() const
{
    QHash<int, QByteArray> hash = MLMediaModel::roleNames();

    hash.insert({
        {VIDEO_THUMBNAIL, QByteArrayLiteral("thumbnail")}, // TODO: remove (a similar roleName already "small_cover")
        {VIDEO_IS_NEW, QByteArrayLiteral("isNew")},
        {VIDEO_RESOLUTION, QByteArrayLiteral("resolution_name")},
        {VIDEO_CHANNEL, QByteArrayLiteral("channel")},
        {VIDEO_DISPLAY_MRL, QByteArrayLiteral("display_mrl")},
        {VIDEO_VIDEO_TRACK, QByteArrayLiteral("videoDesc")},
        {VIDEO_AUDIO_TRACK, QByteArrayLiteral("audioDesc")},
        {VIDEO_SUBTITLE_TRACK, QByteArrayLiteral("subtitleDesc")},
    });

    return hash;
}

heimdall_ml_sorting_criteria_t MLVideoModel::nameToCriteria(QByteArray name) const
{
    return QHash<QByteArray, heimdall_ml_sorting_criteria_t> {
        {QByteArrayLiteral("title"), HEIMDALL_ML_SORTING_ALPHA},
        {QByteArrayLiteral("duration"), HEIMDALL_ML_SORTING_DURATION},
        {QByteArrayLiteral("playcount"), HEIMDALL_ML_SORTING_PLAYCOUNT},
        {QByteArrayLiteral("insertion"), HEIMDALL_ML_SORTING_INSERTIONDATE},
    }.value(name, HEIMDALL_ML_SORTING_DEFAULT);
}

// Protected MLBaseModel reimplementation

void MLVideoModel::onHeimdallMlEvent(const MLEvent &event)
{
    switch (event.i_type)
    {
        case HEIMDALL_ML_EVENT_MEDIA_ADDED:
        {
            if (event.creation.media.i_type == HEIMDALL_ML_MEDIA_TYPE_VIDEO)
            {
                emit resetRequested();

                return;
            }

            break;
        }
        case HEIMDALL_ML_EVENT_MEDIA_UPDATED:
        {
            MLItemId itemId(event.modification.i_entity_id, HEIMDALL_ML_PARENT_UNKNOWN);
            updateItemInCache(itemId);

            return;
        }
        case HEIMDALL_ML_EVENT_MEDIA_DELETED:
        {
            MLItemId itemId(event.deletion.i_entity_id, HEIMDALL_ML_PARENT_UNKNOWN);
            deleteItemInCache(itemId);

            return;
        }
        default:
            break;
    }

    MLBaseModel::onHeimdallMlEvent(event);
}

void MLVideoModel::thumbnailUpdated(const QModelIndex& idx, MLItem* mlitem, const QString& mrl, heimdall_ml_thumbnail_status_t status)
{
    auto videoItem = static_cast<MLVideo*>(mlitem);
    videoItem->setSmallCover(status, mrl);
    emit dataChanged(idx, idx, {VIDEO_THUMBNAIL});
}

void MLVideoModel::generateThumbnail(uint64_t id) const
{
    m_mediaLib->runOnMLThread(this,
    //ML thread
    [id](heimdall_medialibrary_t* ml){
        heimdall_ml_media_generate_thumbnail(ml, id, HEIMDALL_ML_THUMBNAIL_SMALL, 512, 320, .15 );
    });
}

std::unique_ptr<MLListCacheLoader>
MLVideoModel::createMLLoader() const
{
    return std::make_unique<MLListCacheLoader>(m_mediaLib, std::make_shared<MLVideoModel::Loader>(*this));
}

size_t MLVideoModel::Loader::count(heimdall_medialibrary_t* ml, const heimdall_ml_query_params_t* queryParams) const /* override */
{
    int64_t id = m_parent.id;

    if (id <= 0)
        return heimdall_ml_count_video_media(ml, queryParams);
    else
        return heimdall_ml_count_video_of(ml, queryParams, m_parent.type, id);
}

std::vector<std::unique_ptr<MLItem>>
MLVideoModel::Loader::load(heimdall_medialibrary_t* ml, const heimdall_ml_query_params_t* queryParams) const /* override */
{
    ml_unique_ptr<heimdall_ml_media_list_t> list;

    int64_t id = m_parent.id;

    if (id <= 0)
        list.reset(heimdall_ml_list_video_media(ml, queryParams));
    else
        list.reset(heimdall_ml_list_video_of(ml, queryParams, m_parent.type, id));

    if (list == nullptr)
        return {};

    std::vector<std::unique_ptr<MLItem>> result;

    for (const heimdall_ml_media_t & media : ml_range_iterate<heimdall_ml_media_t>(list))
    {
        result.emplace_back(std::make_unique<MLVideo>( &media));
    }

    return result;
}

std::unique_ptr<MLItem>
MLVideoModel::Loader::loadItemById(heimdall_medialibrary_t* ml, MLItemId itemId) const
{
    assert(itemId.type == HEIMDALL_ML_PARENT_UNKNOWN);
    ml_unique_ptr<heimdall_ml_media_t> media(heimdall_ml_get_media(ml, itemId.id));
    if (!media)
        return nullptr;
    return std::make_unique<MLVideo>(media.get());
}
