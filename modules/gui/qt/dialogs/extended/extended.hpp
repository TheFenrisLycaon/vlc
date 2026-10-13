/*****************************************************************************
 * extended.hpp : Extended controls - Undocked
 ****************************************************************************
 * Copyright (C) 2006 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef QHEIMDALL_EXTENDED_DIALOG_H_
#define QHEIMDALL_EXTENDED_DIALOG_H_ 1

#include "widgets/native/qheimdallframe.hpp"

#include "extended_panels.hpp"
#include "player/player_controller.hpp"

class QTabWidget;

class ExtendedDialog : public QHeimdallDialog
{
    Q_OBJECT
public:
    enum
    {
        AUDIO_TAB = 0,
        VIDEO_TAB,
        SYNCHRO_TAB,
        V4L2_TAB
    };
    void showTab( int i );
    int currentTab();

public:
    ExtendedDialog( qt_intf_t * );
    virtual ~ExtendedDialog();

private:
    SyncControls *syncW;
    ExtVideo *videoEffect;
    Equalizer *equal;
    QTabWidget *mainTabW;
    QPushButton *m_applyButton;
    QHash<QString, QVariant> m_hashConfigs[2];
private slots:
    void changedItem(PlayerController::PlayingState );
    void currentTabChanged( int );
    void saveConfig();
    void putAudioConfig( const QString& name, const QVariant value );
    void putVideoConfig( const QString& name, const QVariant value );
};

#endif
