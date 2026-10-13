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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "playlist_controller.hpp"
#include "playlist_controller_p.hpp"
#include <heimdall_player.h>
#include <heimdall_url.h>
#include "util/shared_input_item.hpp"
#include <algorithm>
#include <QVariant>
#include <QDesktopServices>

namespace heimdall {
  namespace playlist {

static QVector<PlaylistItem> toVec(heimdall_playlist_item_t *const items[],
                                   size_t len)
{
    QVector<PlaylistItem> vec;
    for (size_t i = 0; i < len; ++i)
        vec.push_back(items[i]);
    return vec;
}

template <typename RAW, typename WRAPPER>
static QVector<RAW> toRaw(const QVector<WRAPPER> &items)
{
    QVector<RAW> vec;
    int count = items.size();
    vec.reserve(count);
    for (int i = 0; i < count; ++i)
        vec.push_back(items[i].raw());
    return vec;
}

QVector<Media> toMediaList(const QVariantList &sources)
{
    QVector<Media> mediaList;
    std::transform(sources.begin(), sources.end(),
                   std::back_inserter(mediaList), [](const QVariant& value) {
        const auto typeId = value.typeId();
        if ((typeId == QMetaType::QUrl) || (typeId == QMetaType::QString))
        {
            std::optional<QString> input;
            QUrl mrl;
            if (typeId == QMetaType::QString)
            {
                input = value.value<QString>().trimmed(); // akin to `UrlValidator::fixup()`
                mrl = *input; // plain text to QUrl, tolerant mode
            }
            else if (typeId == QMetaType::QUrl)
            {
                mrl = value.value<QUrl>(); // use the provided QUrl as is
            }
            else
                Q_UNREACHABLE();

            if (!mrl.isValid())
                return Media(); // the input is broken (beyond repair of tolerant mode if plain text)

            if ((typeId == QMetaType::QString) && mrl.scheme().isEmpty())
            {
                // Assume input is a path (only if the input is plain text):
                assert(input);
                mrl = QString::fromUtf8(heimdall::wrap_cptr(heimdall_path2uri(input->toUtf8().cbegin(), nullptr)).get()); // tolerant mode

                if (!mrl.isValid())
                    return Media();
            }

            if (mrl.scheme().isEmpty()) // akin to `UrlValidator::validate()`
                return Media();

            return Media(mrl.toString(QUrl::FullyEncoded), mrl.fileName());
        }
        else if (value.canConvert<SharedInputItem>())
        {
            return Media(value.value<SharedInputItem>().get());
        }
        else if (value.canConvert<PlaylistItem>())
        {
            return Media(value.value<PlaylistItem>().inputItem());
        }
        return Media{};
    });

    // Clear invalid media:
    mediaList.removeAll(Media());

    return mediaList;
}


extern "C" { // for C callbacks

static void
on_playlist_items_reset(heimdall_playlist_t *playlist,
                        heimdall_playlist_item_t *const items[],
                        size_t len, void *userdata)
{
    PlaylistControllerPrivate *that = static_cast<PlaylistControllerPrivate *>(userdata);
    auto vec = toVec(items, len);
    size_t totalCount = heimdall_playlist_Count(playlist);

    that->callAsync([=](){
        if (that->m_playlist != playlist)
            return;
        PlaylistController* q = that->q_func();
        bool empty = vec.size() == 0;
        if (that->m_empty != empty)
        {
            that->m_empty = empty;
            emit q->isEmptyChanged(empty);
        }
        emit q->itemsReset(vec);

        if (totalCount != that->m_count)
        {
            that->m_count = totalCount;
            emit q->countChanged(totalCount);
        }
    });
}

static void
on_playlist_items_added(heimdall_playlist_t *playlist, size_t index,
                        heimdall_playlist_item_t *const items[], size_t len,
                        void *userdata)
{
    PlaylistControllerPrivate *that = static_cast<PlaylistControllerPrivate *>(userdata);
    auto vec = toVec(items, len);
    size_t totalCount = heimdall_playlist_Count(playlist);
    that->callAsync([=](){
        if (that->m_playlist != playlist)
            return;
        PlaylistController* q = that->q_func();
        if (that->m_empty && vec.size() > 0)
        {
            that->m_empty = false;
            emit q->isEmptyChanged(that->m_empty);
        }
        emit q->itemsAdded(index, vec);

        if (totalCount != that->m_count)
        {
            that->m_count = totalCount;
            emit q->countChanged(totalCount);
        }
    });
}

static void
on_playlist_items_moved(heimdall_playlist_t *playlist, size_t index, size_t count,
                        size_t target, void *userdata)
{
    PlaylistControllerPrivate *that = static_cast<PlaylistControllerPrivate *>(userdata);
    that->callAsync([=](){
        if (that->m_playlist != playlist)
            return;
        emit that->q_func()->itemsMoved(index, count, target);
    });
}

static void
on_playlist_items_removed(heimdall_playlist_t *playlist, size_t index, size_t count,
                          void *userdata)
{
    PlaylistControllerPrivate *that = static_cast<PlaylistControllerPrivate *>(userdata);
    size_t totalCount = heimdall_playlist_Count(playlist);
    bool empty = (totalCount == 0);
    that->callAsync([=](){
        if (that->m_playlist != playlist)
            return;
        PlaylistController* q = that->q_func();
        if (that->m_empty != empty)
        {
            that->m_empty = empty;
            emit q->isEmptyChanged(empty);
        }
        emit q->itemsRemoved(index, count);
        if (totalCount != that->m_count)
        {
            that->m_count = totalCount;
            emit q->countChanged(totalCount);
        }
    });
}

static void
on_playlist_items_updated(heimdall_playlist_t *playlist, size_t index,
                          heimdall_playlist_item_t *const items[], size_t len,
                          void *userdata)
{
    PlaylistControllerPrivate *that = static_cast<PlaylistControllerPrivate *>(userdata);
    auto vec = toVec(items, len);
    that->callAsync([=](){
        if (that->m_playlist != playlist)
            return;
        emit that->q_func()->itemsUpdated(index, vec);
        if (that->m_currentIndex != -1) {
            size_t currentIndex = static_cast<size_t>(that->m_currentIndex);
            if (currentIndex >= index && currentIndex < index + len)
            {
                const auto newCurrentItem = vec[currentIndex - index];
                const auto currentItem = that->m_currentItem.raw();
                // Even if heimdall_playlist_item_t is the same, assign unconditionally to
                // update the data:
                that->m_currentItem = newCurrentItem;
                if (currentItem != newCurrentItem.raw())
                    emit that->q_func()->currentItemChanged();
            }
        }
    });
}

static void
on_playlist_playback_repeat_changed(heimdall_playlist_t *playlist,
                                    enum heimdall_playlist_playback_repeat repeat,
                                    void *userdata)
{
    PlaylistControllerPrivate *that = static_cast<PlaylistControllerPrivate *>(userdata);
    that->callAsync([=](){
        if (that->m_playlist != playlist)
            return;
        PlaylistController::PlaybackRepeat repeatMode = static_cast<PlaylistController::PlaybackRepeat>(repeat);
        if (that->m_repeat != repeatMode )
        {
            that->m_repeat = repeatMode;
            emit that->q_func()->repeatModeChanged(repeatMode);
        }
    });
}

static void
on_playlist_playback_order_changed(heimdall_playlist_t *playlist,
                                   enum heimdall_playlist_playback_order order,
                                   void *userdata)
{
    PlaylistControllerPrivate *that = static_cast<PlaylistControllerPrivate *>(userdata);
    that->callAsync([=](){
        if (that->m_playlist != playlist)
            return;
        bool isRandom = order == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM;
        if (that->m_random != isRandom)
        {
            that->m_random = isRandom;
            emit that->q_func()->randomChanged(isRandom);
        }
    });
}

static void
on_playlist_current_index_changed(heimdall_playlist_t *playlist, ssize_t index,
                                 void *userdata)
{
    PlaylistControllerPrivate *that = static_cast<PlaylistControllerPrivate *>(userdata);


    heimdall_playlist_item_t* playlistItem = nullptr;
    if (index >= 0)
        playlistItem = heimdall_playlist_Get(playlist, index);
    PlaylistItem newItem{ playlistItem };

    that->callAsync([=](){
        PlaylistController* q = that->q_func();

        if (that->m_playlist != playlist)
            return;
        if (that->m_currentIndex != index)
        {
            that->m_currentIndex = index;
            emit q->currentIndexChanged(that->m_currentIndex);
        }

        // Unlike items updated callback, we should not need to unconditionally
        // set the current item to update data because this is only a current
        // index changed callback.
        if (that->m_currentItem.raw() != newItem.raw())
        {
            that->m_currentItem = newItem;
            emit q->currentItemChanged();
        }
    });
}

static void
on_playlist_has_prev_changed(heimdall_playlist_t *playlist, bool has_prev,
                             void *userdata)
{
    PlaylistControllerPrivate *that = static_cast<PlaylistControllerPrivate *>(userdata);
    that->callAsync([=](){
        if (that->m_playlist != playlist)
            return;
        if (that->m_hasPrev != has_prev)
        {
            that->m_hasPrev = has_prev;
            emit that->q_func()->hasPrevChanged(has_prev);
        }
    });
}

static void
on_playlist_has_next_changed(heimdall_playlist_t *playlist, bool has_next,
                             void *userdata)
{
    PlaylistControllerPrivate *that = static_cast<PlaylistControllerPrivate *>(userdata);
    that->callAsync([=](){
        if (that->m_playlist != playlist)
            return;
        if (that->m_hasNext != has_next)
        {
            that->m_hasNext = has_next;
            emit that->q_func()->hasNextChanged(has_next);
        }
    });
}

static void
on_media_stopped_action_changed(heimdall_playlist_t *playlist,
                          enum heimdall_playlist_media_stopped_action new_action,
                          void *userdata)
{
    PlaylistControllerPrivate *that = static_cast<PlaylistControllerPrivate *>(userdata);
    that->callAsync([=](){
        auto action = static_cast<PlaylistController::MediaStopAction>(new_action);
        if (that->m_playlist != playlist)
            return;
        if (that->m_mediaStopAction == action)
            return;
        that->m_mediaStopAction = action;
        emit that->q_func()->mediaStopActionChanged(action);
    });
}

} // extern "C"

static const struct heimdall_playlist_callbacks playlist_callbacks = []{
    struct heimdall_playlist_callbacks cbs {};
    cbs.on_items_reset = on_playlist_items_reset;
    cbs.on_items_added = on_playlist_items_added;
    cbs.on_items_moved = on_playlist_items_moved;
    cbs.on_items_removed = on_playlist_items_removed;
    cbs.on_items_updated = on_playlist_items_updated;
    cbs.on_playback_repeat_changed = on_playlist_playback_repeat_changed;
    cbs.on_playback_order_changed= on_playlist_playback_order_changed;
    cbs.on_current_index_changed = on_playlist_current_index_changed;
    cbs.on_has_next_changed = on_playlist_has_next_changed;
    cbs.on_has_prev_changed = on_playlist_has_prev_changed;
    cbs.on_media_stopped_action_changed = on_media_stopped_action_changed;
    return cbs;
}();


//private API

PlaylistControllerPrivate::PlaylistControllerPrivate(PlaylistController* playlistController, heimdall_playlist_t *playlist)
    : q_ptr(playlistController)
    , m_playlist(playlist)
{
    fillSortKeyTitleList();
}

PlaylistControllerPrivate::~PlaylistControllerPrivate()
{
    if (m_playlist && m_listener)
    {
        heimdall_playlist_locker lock(m_playlist);
        heimdall_playlist_RemoveListener(m_playlist, m_listener);
    }
}

//public API

PlaylistController::PlaylistController(heimdall_playlist_t *playlist, QObject *parent)
    : QObject(parent)
    , d_ptr( new PlaylistControllerPrivate(this, playlist) )
{
    Q_D(PlaylistController);
    assert(playlist);

    connect(this, &PlaylistController::itemsMoved, this, &PlaylistController::resetSortKey);
    connect(this, &PlaylistController::itemsAdded, this, &PlaylistController::resetSortKey);
    connect(this, &PlaylistController::isEmptyChanged, [this](bool isEmpty) {if (isEmpty) emit resetSortKey();});

    {
        heimdall_playlist_locker locker(d->m_playlist);
        d->m_listener = heimdall_playlist_AddListener(d->m_playlist, &playlist_callbacks, d, true);
    }
    /*
     * Queue a playlistInitialized to be sent after the initial state callbacks
     * heimdall_playlist_AddListener will synchronously call each callback in
     * playlist_callbacks, which will in turn queue an async call on the Qt
     * main thread
     */
    d->callAsync([this, d](){
        d->m_initialized = true;
        emit initializedChanged();
    });
}

PlaylistController::~PlaylistController()
{
}

PlaylistItem PlaylistController::getCurrentItem() const
{
    Q_D(const PlaylistController);
    return d->m_currentItem;
}

void PlaylistController::append(const QVariantList& sourceList, bool startPlaying)
{
    append(toMediaList(sourceList), startPlaying);
}

void PlaylistController::append(const QVariant &source, bool startPlaying)
{
    append(QVariantList{source}, startPlaying);
}

void PlaylistController::insert(unsigned index, const QVariantList& sourceList, bool startPlaying)
{
    insert(index, toMediaList(sourceList), startPlaying);
}

void
PlaylistController::append(const QVector<Media> &media, bool startPlaying)
{
    Q_D(PlaylistController);

    if (Q_UNLIKELY(media.isEmpty()))
        return;

    heimdall_playlist_locker locker(d->m_playlist);

    auto rawMedia = toRaw<input_item_t *>(media);
    /* We can't append an empty media. */
    assert(rawMedia.size() > 0);

    int ret = heimdall_playlist_Append(d->m_playlist,
                                  rawMedia.constData(), rawMedia.size());
    if (ret != HEIMDALL_SUCCESS)
        throw std::bad_alloc();
    if (startPlaying)
    {
        ssize_t playIndex = (ssize_t)heimdall_playlist_Count( d->m_playlist ) - rawMedia.size();
        ret = heimdall_playlist_GoTo(d->m_playlist, playIndex);
        if (ret != HEIMDALL_SUCCESS)
            return;
        heimdall_playlist_Start(d->m_playlist);
    }
}

void
PlaylistController::insert(size_t index, const QVector<Media> &media, bool startPlaying)
{
    Q_D(PlaylistController);

    if (Q_UNLIKELY(media.isEmpty()))
        return;

    heimdall_playlist_locker locker(d->m_playlist);

    auto rawMedia = toRaw<input_item_t *>(media);
    /* We can't insert an empty media. */
    assert(rawMedia.size() > 0);

    int ret = heimdall_playlist_RequestInsert(d->m_playlist, index,
                                         rawMedia.constData(), rawMedia.size());
    if (ret != HEIMDALL_SUCCESS)
        throw std::bad_alloc();
    if (startPlaying)
    {
        ret = heimdall_playlist_GoTo(d->m_playlist, index);
        if (ret != HEIMDALL_SUCCESS)
            return;
        heimdall_playlist_Start(d->m_playlist);
    }
}

void
PlaylistController::move(const QVector<PlaylistItem> &items, size_t target,
               ssize_t indexHint)
{
    Q_D(PlaylistController);
    heimdall_playlist_locker locker(d->m_playlist);

    auto rawItems = toRaw<heimdall_playlist_item_t *>(items);
    int ret = heimdall_playlist_RequestMove(d->m_playlist, rawItems.constData(),
                                       rawItems.size(), target, indexHint);
    if (ret != HEIMDALL_SUCCESS)
        throw std::bad_alloc();
}

void
PlaylistController::remove(const QVector<PlaylistItem> &items, ssize_t indexHint)
{
    Q_D(PlaylistController);
    heimdall_playlist_locker locker(d->m_playlist);
    auto rawItems = toRaw<heimdall_playlist_item_t *>(items);
    int ret = heimdall_playlist_RequestRemove(d->m_playlist, rawItems.constData(),
                                         rawItems.size(), indexHint);
    if (ret != HEIMDALL_SUCCESS)
        throw std::bad_alloc();
}

void
PlaylistController::shuffle()
{
    Q_D(PlaylistController);
    heimdall_playlist_locker locker(d->m_playlist);
    heimdall_playlist_Shuffle(d->m_playlist);
}

void
PlaylistController::sort(const QVector<heimdall_playlist_sort_criterion> &criteria)
{
    Q_D(PlaylistController);
    heimdall_playlist_locker locker(d->m_playlist);
    heimdall_playlist_Sort(d->m_playlist, criteria.constData(), criteria.size());
}

void PlaylistController::sort(PlaylistController::SortKey key, PlaylistController::SortOrder order)
{
    if(key != SortKey::SORT_KEY_NONE)
        setSortKey(key);
    setSortOrder(order);

    sort();
}

void PlaylistController::sort(PlaylistController::SortKey key)
{
    if (key == SortKey::SORT_KEY_NONE)
        return;

    if (getSortKey() != key)
    {
        setSortOrder(SortOrder::SORT_ORDER_ASC);
        setSortKey(key);
    }
    else
    {
        switchSortOrder();
    }

    sort();
}

void PlaylistController::sort(void)
{
    Q_D(PlaylistController);

    if (d->m_sortKey == SortKey::SORT_KEY_NONE)
        return;

    heimdall_playlist_sort_criterion crit = {
        static_cast<heimdall_playlist_sort_key>(d->m_sortKey),
        static_cast<heimdall_playlist_sort_order>(d->m_sortOrder)
    };
    QVector<heimdall_playlist_sort_criterion> criteria { crit };
    sort( criteria );
}

void PlaylistController::explore(const PlaylistItem& pItem)
{
    heimdall_playlist_item_t * const playlistItem = pItem.raw();
    if( playlistItem )
    {
        input_item_t * const p_input = heimdall_playlist_item_GetMedia(playlistItem);
        auto uri = heimdall::wrap_cptr( input_item_GetURI(p_input) );

        if( uri && uri.get()[0] != '\0')
        {
            auto path = heimdall::wrap_cptr( heimdall_uri2path( uri.get() ) );

            if( !path )
                return;

            QString containingDir = QFileInfo( path.get() ).absolutePath();
            if( !QFileInfo( containingDir ).isDir() )
                return;

            QUrl file = QUrl::fromLocalFile( containingDir );
            if( !file.isLocalFile() )
                return;

            QDesktopServices::openUrl( file );
        }
    }
}

int PlaylistController::serialize(const QString &fileName)
{
    Q_D(PlaylistController);
    heimdall_playlist_locker lock{d->m_playlist};
    return heimdall_playlist_Export(d->m_playlist, qtu(fileName), "export-m3u8");
}

int PlaylistController::currentIndex() const
{
    Q_D(const PlaylistController);
    return d->m_currentIndex;
}

void PlaylistController::play()
{
    Q_D(PlaylistController);
    heimdall_playlist_locker lock{ d->m_playlist };
    heimdall_playlist_Start( d->m_playlist );
}

void PlaylistController::pause()
{
    Q_D(PlaylistController);
    heimdall_playlist_locker lock{ d->m_playlist };
    heimdall_playlist_Pause( d->m_playlist );
}

void PlaylistController::stop()
{
    Q_D(PlaylistController);
    heimdall_playlist_locker lock{ d->m_playlist };
    heimdall_playlist_Stop( d->m_playlist );
}

void PlaylistController::next()
{
    Q_D(PlaylistController);
    heimdall_playlist_locker lock{ d->m_playlist };
    heimdall_playlist_Next( d->m_playlist );
}

void PlaylistController::prev()
{
    Q_D(PlaylistController);
    heimdall_playlist_locker lock{ d->m_playlist };
    heimdall_playlist_Prev( d->m_playlist );
}

void PlaylistController::prevOrReset()
{
    Q_D(PlaylistController);
    heimdall_playlist_locker lock{ d->m_playlist };
    bool seek = false;
    heimdall_player_t* player = heimdall_playlist_GetPlayer( d->m_playlist );
    Q_ASSERT(player);
    if( !heimdall_player_IsStarted(player) || heimdall_player_GetTime(player) < HEIMDALL_TICK_FROM_MS(10) )
    {
        int ret = heimdall_playlist_Prev( d->m_playlist );
        if (ret == HEIMDALL_SUCCESS)
            heimdall_playlist_Start( d->m_playlist );
    }
    else
        seek = true;
    if (seek)
        heimdall_player_JumpPos( player, 0.f );
}

void PlaylistController::togglePlayPause()
{
    Q_D(PlaylistController);
    heimdall_playlist_locker lock{ d->m_playlist };
    heimdall_player_t* player = heimdall_playlist_GetPlayer( d->m_playlist );
    if ( heimdall_player_IsStarted(player) )
        heimdall_player_TogglePause( player );
    else
        heimdall_playlist_Start( d->m_playlist );
}

void PlaylistController::toggleRandom()
{
    Q_D(PlaylistController);
    heimdall_playlist_playback_order new_order;
    heimdall_playlist_locker lock{ d->m_playlist };
    if ( d->m_random )
        new_order = HEIMDALL_PLAYLIST_PLAYBACK_ORDER_NORMAL;
    else
        new_order = HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM;
    heimdall_playlist_SetPlaybackOrder( d->m_playlist, new_order );
    config_PutInt( "random", new_order );
}

void PlaylistController::toggleRepeatMode()
{
    Q_D(PlaylistController);
    heimdall_playlist_playback_repeat new_repeat;
    /* Toggle Normal -> Loop -> Repeat -> Normal ... */
    switch ( d->m_repeat ) {
    case PlaybackRepeat::PLAYBACK_REPEAT_NONE:
        new_repeat = HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL;
        break;
    case PlaybackRepeat::PLAYBACK_REPEAT_ALL:
        new_repeat = HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_CURRENT;
        break;
    case PlaybackRepeat::PLAYBACK_REPEAT_CURRENT:
    default:
        new_repeat = HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_NONE;
        break;
    }
    {
        heimdall_playlist_locker lock{ d->m_playlist };
        heimdall_playlist_SetPlaybackRepeat( d->m_playlist, new_repeat );
    }
    config_PutInt( "repeat", new_repeat );
}

void PlaylistController::clear()
{
    Q_D(PlaylistController);
    heimdall_playlist_locker lock{ d->m_playlist };
    heimdall_playlist_Clear( d->m_playlist );
}

void PlaylistController::goTo(uint index, bool startPlaying)
{
    Q_D(PlaylistController);
    heimdall_playlist_locker lock{ d->m_playlist };
    size_t count = heimdall_playlist_Count( d->m_playlist );
    if (count == 0 || index > count-1)
        return;
    heimdall_playlist_GoTo( d->m_playlist, index );
    if (startPlaying)
        heimdall_playlist_Start( d->m_playlist );
}

bool PlaylistController::isRandom() const
{
    Q_D(const PlaylistController);
    return d->m_random;
}

void PlaylistController::setRandom(bool random)
{
    Q_D(const PlaylistController);
    heimdall_playlist_locker lock{ d->m_playlist };
    heimdall_playlist_SetPlaybackOrder( d->m_playlist, random ? HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM : HEIMDALL_PLAYLIST_PLAYBACK_ORDER_NORMAL );
}

Playlist PlaylistController::getPlaylist() const
{
    Q_D(const PlaylistController);
    return Playlist(d->m_playlist);
}

void PlaylistController::resetSortKey()
{
    Q_D(PlaylistController);
    d->m_sortKey = SortKey::SORT_KEY_NONE;
    emit sortKeyChanged();
}

PlaylistController::PlaybackRepeat PlaylistController::getRepeatMode() const
{
    Q_D(const PlaylistController);
    return d->m_repeat;
}

void PlaylistController::setRepeatMode(PlaylistController::PlaybackRepeat mode)
{
    Q_D(const PlaylistController);
    heimdall_playlist_locker lock{ d->m_playlist };
    heimdall_playlist_SetPlaybackRepeat( d->m_playlist, static_cast<heimdall_playlist_playback_repeat>(mode) );
}

PlaylistController::MediaStopAction PlaylistController::getMediaStopAction() const
{
    Q_D(const PlaylistController);
    return d->m_mediaStopAction;
}

void PlaylistController::setMediaStopAction(PlaylistController::MediaStopAction action)
{
    Q_D(PlaylistController);
    heimdall_playlist_locker lock{ d->m_playlist };
    heimdall_playlist_SetMediaStoppedAction(d->m_playlist, static_cast<heimdall_playlist_media_stopped_action>(action) );
}

bool PlaylistController::isEmpty() const
{
    Q_D(const PlaylistController);
    return d->m_empty;
}

int PlaylistController::count() const
{
    Q_D(const PlaylistController);
    assert(d->m_count <= (size_t)INT_MAX);
    return d->m_count;
}

void PlaylistController::setSortKey(SortKey sortKey)
{
    Q_D(PlaylistController);

    if (sortKey == d->m_sortKey)
        return;

    d->m_sortKey = sortKey;
    emit sortKeyChanged();
}

void PlaylistController::setSortOrder(SortOrder sortOrder)
{
    Q_D(PlaylistController);

    SortOrder order = d->m_sortOrder;
    if(sortOrder == order)
        return;

    d->m_sortOrder = sortOrder;
    emit sortOrderChanged();
}

void PlaylistController::switchSortOrder()
{
    Q_D(PlaylistController);

    SortOrder order = d->m_sortOrder;
    if (order == SortOrder::SORT_ORDER_ASC)
        order = SortOrder::SORT_ORDER_DESC;
    else if (order == SortOrder::SORT_ORDER_DESC)
        order = SortOrder::SORT_ORDER_ASC;
    else
        return;

    d->m_sortOrder = order;
    emit sortOrderChanged();
}

PlaylistController::SortKey PlaylistController::getSortKey() const
{
    Q_D(const PlaylistController);
    return d->m_sortKey;
}

PlaylistController::SortOrder PlaylistController::getSortOrder() const
{
    Q_D(const PlaylistController);
    return d->m_sortOrder;
}

bool PlaylistController::hasNext() const
{
    Q_D(const PlaylistController);
    return d->m_hasNext;
}

bool PlaylistController::hasPrev() const
{
    Q_D(const PlaylistController);
    return d->m_hasPrev;
}

QVariantList PlaylistController::getSortKeyTitleList() const
{
    Q_D(const PlaylistController);

    return d->sortKeyTitleList;
}

bool PlaylistController::isInitialized() const
{
    Q_D(const PlaylistController);
    return d->m_initialized;
}

  } // namespace playlist
} // namespace heimdall
