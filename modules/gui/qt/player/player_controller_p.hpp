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

#ifndef QHEIMDALL_INPUT_MANAGER_P_H_
#define QHEIMDALL_INPUT_MANAGER_P_H_

#include "player_controller.hpp"
#include "util/variables.hpp"
#include "input_models.hpp"
#include "util/varchoicemodel.hpp"
#include "util/shared_input_item.hpp"
#include "util/renderer_manager.hpp"

#include <QTimer>
#include <QUrl>
#include <QByteArray>
#include <QPair>

#include <optional>

#ifndef QT_HAS_LIBATOMIC
#warning "libatomic is not available. Read write lock is going to be used instead."
#include <QReadWriteLock>
#endif

typedef struct heimdall_preparser_t heimdall_preparser_t;

class PlayerControllerPrivate {
    Q_DISABLE_COPY(PlayerControllerPrivate)
public:
    Q_DECLARE_PUBLIC(PlayerController)
    PlayerController * const q_ptr;

public:
    PlayerControllerPrivate(PlayerController* playercontroller, qt_intf_t* p_intf);
    PlayerControllerPrivate() = delete;
    ~PlayerControllerPrivate();

    void UpdateName( input_item_t *p_item );
    void UpdateArt( input_item_t *p_item );
    void UpdateMeta( input_item_t *p_item );
    void UpdateInfo( input_item_t *p_item );
    void UpdateStats( const input_stats_t& stats );
    void UpdateProgram(heimdall_player_list_action action, const heimdall_player_program *prgm);
    void UpdateVouts(vout_thread_t **vouts, size_t i_vouts, heimdall_player_vout_action action);
    void UpdateTrackSelection(heimdall_es_id_t *trackid, bool selected);
    void UpdateSpuOrder(heimdall_es_id_t *es_id, enum heimdall_vout_order spu_order);
    int interpolateTime(heimdall_tick_t system_now);
    bool isCurrentItemSynced();

    // Lazily reads m_syltRaw (the raw sylt-data buffer captured during
    // UpdateMeta) into a list of TimedText entries. The cache is
    // invalidated when m_syltRaw changes.
    QList<TimedText> syltLyrics() const;
    void onArtFetchEnded(input_item_t *, bool fetched);

    // SMPTE Timer
    void addSMPTETimer();
    void removeSMPTETimer();

    ///call function @a fun on object thread
    template <typename Fun>
    void callAsync(Fun&& fun)
    {
        Q_Q(PlayerController);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
        // NOTE: Starting with Qt 6.7.0, lambda expression here without a return value
        //       causes compilation issues with some compilers.
        // TODO: Find out if a more recent Qt version does not behave that way.
        QMetaObject::invokeMethod(q, [fun = std::forward<Fun>(fun)]() -> std::monostate { fun(); return std::monostate{}; }, Qt::QueuedConnection);
#else
        QMetaObject::invokeMethod(q, std::forward<Fun>(fun), Qt::QueuedConnection, nullptr);
#endif
    }

public:
    qt_intf_t           *p_intf;
    heimdall_player_t            *m_player;
    heimdall_preparser_t         *m_preparser = nullptr;

    //callbacks
    heimdall_player_listener_id* m_player_listener = nullptr;
    heimdall_player_aout_listener_id* m_player_aout_listener = nullptr;
    heimdall_player_vout_listener_id* m_player_vout_listener = nullptr;

    //playback
    PlayerController::PlayingState m_playing_status = PlayerController::PLAYING_STATE_STOPPED;
    QString         m_name;
    float           m_buffering = 0.f;
    float           m_rate = 1.f;

    HeimdallTime      m_time;
    HeimdallDuration  m_remainingTime;
    double       m_position = 0.f;
    HeimdallDuration  m_length;

#ifdef QT_HAS_LIBATOMIC
    std::atomic<heimdall_player_timer_smpte_timecode> m_highResolutionTime {{}};
#else
    heimdall_player_timer_smpte_timecode m_highResolutionTime {};
    mutable QReadWriteLock m_highResolutionTimeLock;
#endif
    mutable QPair<heimdall_player_timer_smpte_timecode, QString> m_highResolutionTimeSample;

    unsigned m_smpteTimerRequestCount = 0;

    SharedInputItem    m_currentItem;
    bool            m_canRestorePlayback = false;

    int             m_capabilities = 0;

    //tracks
    TrackListModel m_videoTracks;
    TrackListModel m_audioTracks;
    TrackListModel m_subtitleTracks;

    ::heimdall::heimdall_shared_data_ptr<heimdall_es_id_t, &heimdall_es_id_Hold, &heimdall_es_id_Release> m_secondarySpuEsId;

    HeimdallDuration  m_audioDelay;
    HeimdallDuration  m_subtitleDelay;
    HeimdallDuration  m_secondarySubtitleDelay;
    float        m_subtitleFPS = 1.0;

    //timer
    heimdall_player_timer_id* m_player_timer = nullptr;
    heimdall_player_timer_id* m_player_timer_smpte = nullptr;
    struct heimdall_player_timer_point m_player_time;
    bool seeking = false;
    QTimer m_position_timer;
    QTimer m_time_timer;

    //title/chapters/menu
    TitleListModel m_titleList;
    ChapterListModel m_chapterList;
    bool m_hasTitles = false;
    bool m_hasChapters = false;
    bool m_hasMenu = false;
    bool m_isMenu = false;
    bool m_isInteractive = false;

    //programs
    ProgramListModel m_programList;
    bool m_hasPrograms = false;
    bool m_encrypted = false;

    //teletext
    bool m_teletextEnabled = false;
    bool m_teletextAvailable = false;
    int m_teletextPage = false;
    bool m_teletextTransparent = false;

    //vout properties
    HeimdallVarChoiceModel m_videoStereoMode;
    HeimdallVarChoiceModel m_zoom;
    HeimdallVarChoiceModel m_aspectRatio;
    HeimdallVarChoiceModel m_crop;
    HeimdallVarChoiceModel m_fit;
    HeimdallVarChoiceModel m_deinterlace;
    HeimdallVarChoiceModel m_deinterlaceMode;
    QHeimdallBool m_autoscale;
    bool            m_hasVideo = false;
    bool            m_fullscreen = false;
    bool            m_wallpaperMode = false;
    int m_vout_ref = 0;

    //aout properties
    HeimdallVarChoiceModel m_audioStereoMode;
    HeimdallVarChoiceModel m_audioMixMode;
    float           m_volume = 0.f;
    bool            m_muted = false;
    AudioDeviceModel m_audioDeviceList;
    HeimdallVarChoiceModel m_audioVisualization;

    //misc
    bool            m_recording = false;
    PlayerController::ABLoopState m_ABLoopState = PlayerController::ABLOOP_STATE_NONE;
    HeimdallTime m_ABLoopA;
    HeimdallTime m_ABLoopB;

    //others
    QString         m_artUrl;
    struct input_stats_t m_stats;

    RendererManager m_rendererManager;

    // meta
    QString m_title;
    QString m_artist;
    QString m_album;
    QUrl m_artwork;
    QUrl m_url;

    mutable std::optional<QPair<QByteArray, size_t>> m_syltRaw;
    mutable std::optional<QList<TimedText>> m_syltLyrics;
    int m_currentLyricIndex = -1;
};

#endif /* QHEIMDALL_INPUT_MANAGER_P_H_ */
