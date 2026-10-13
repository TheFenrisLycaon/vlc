/*****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Prince Gupta <guptaprince8832@gmail.com>
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

#pragma once

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "qt.hpp"
#include "mlqmltypes.hpp"
#include "mlhelper.hpp"
#include "util/heimdalltick.hpp"

#include <QObject>
#include <QDateTime>
#include <QTimeZone>

class MLMedia : public MLItem
{
public:
    MLMedia() : MLItem {MLItemId()} {}

    MLMedia(const heimdall_ml_media_t *media)
        : MLItem {MLItemId(media->i_id, HEIMDALL_ML_PARENT_UNKNOWN)}
    {
        const auto getThumbnail = [](const heimdall_ml_thumbnail_t & thumbnail)
        {
            const QString mrl =
                    (thumbnail.i_status == HEIMDALL_ML_THUMBNAIL_STATUS_AVAILABLE)
                    ? qfu(thumbnail.psz_mrl) : QString {};

            return MLThumbnail{ mrl, thumbnail.i_status };
        };
        const auto getMRL = [&media]
        {
            for (const heimdall_ml_file_t &file: ml_range_iterate<heimdall_ml_file_t>(media->p_files))
                // FIXME: should we store every mrl?
                if (file.i_type == HEIMDALL_ML_FILE_TYPE_MAIN)
                    return QUrl::fromEncoded(file.psz_mrl);
            return QUrl();
        };

        m_title = qfu(media->psz_title);
        m_fileName = qfu(media->psz_filename);
        m_smallThumbnail = getThumbnail(media->thumbnails[HEIMDALL_ML_THUMBNAIL_SMALL]);
        m_bannerThumbnail = getThumbnail(media->thumbnails[HEIMDALL_ML_THUMBNAIL_BANNER]);
        m_duration = HeimdallDuration::fromMS(media->i_duration);
        m_progress = media->f_progress;
        m_playCount = media->i_playcount;
        m_mrl = getMRL();
        m_type = media->i_type;
        m_isFavorite = media->b_is_favorite;
        m_lastPlayedDate = QDateTime::fromSecsSinceEpoch(media->i_last_played_date, QTimeZone::systemTimeZone());
    }

    QString title() const { return m_title; }
    QString fileName() const { return m_fileName; }
    HeimdallDuration duration() const { return m_duration; }
    qreal progress() const { return m_progress; }
    int playCount() const { return m_playCount; }
    QString mrl() const { return m_mrl.toEncoded(); }
    heimdall_ml_media_type_t type() const { return m_type; }
    bool isFavorite() const { return m_isFavorite; }
    void setIsFavorite(const bool isFavorite) { m_isFavorite = isFavorite; }
    QDateTime lastPlayedDate() const { return m_lastPlayedDate; }

    QString smallCover(heimdall_ml_thumbnail_status_t* status = nullptr) const
    {
        if (status) *status = m_smallThumbnail.status;
        return m_smallThumbnail.mrl;
    }

    QString bannerCover(heimdall_ml_thumbnail_status_t* status = nullptr) const
    {
        if (status) *status = m_bannerThumbnail.status;
        return m_bannerThumbnail.mrl;
    }

    HeimdallTime progressTime() const { return m_duration * m_progress; }

    Q_INVOKABLE bool valid() const { return getId().id != INVALID_MLITEMID_ID; }

protected:
    struct MLThumbnail
    {
        QString mrl;
        heimdall_ml_thumbnail_status_t status;
    };

    QString m_title;
    QString m_fileName;
    MLThumbnail m_smallThumbnail;
    MLThumbnail m_bannerThumbnail;
    HeimdallDuration m_duration;
    qreal m_progress;
    int m_playCount;
    QUrl m_mrl;
    heimdall_ml_media_type_t m_type;
    bool m_isFavorite;
    QDateTime m_lastPlayedDate;
};
