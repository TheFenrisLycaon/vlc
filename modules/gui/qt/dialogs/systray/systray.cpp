/*****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
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

#include "systray.hpp"

#include <QImageReader>

#include "maininterface/mainctx.hpp"
#include "menus/menus.hpp"
#include "playlist/playlist_controller.hpp"
#include "player/player_controller.hpp"
#include "dialogs/dialogs_provider.hpp"
#include "widgets/native/qheimdallframe.hpp"

using namespace heimdall::playlist;

HeimdallSystray::HeimdallSystray(MainCtx* ctx, QObject* parent)
    : QSystemTrayIcon(parent)
    , m_ctx(ctx)
    , m_intf(ctx->getIntf())
{
    assert(ctx);
    assert(m_intf);

    m_notificationSetting = var_InheritInteger(m_intf, "qt-notification");

    QIcon iconHeimdall;
    if( m_ctx->useXmasCone() )
        iconHeimdall = QIcon::fromTheme( "heimdall-xmas", QIcon( ":/logo/heimdall128-xmas.png" ) );
    else
        iconHeimdall = QIcon::fromTheme( "heimdall", QIcon( ":/logo/heimdall256.png" ) );

    setIcon(iconHeimdall);
    setToolTip( qtr( "Heimdall media player" ));

    m_menu = std::make_unique<HeimdallMenu>( qtr( "Heimdall media player"), m_intf );
    m_menu->setIcon( iconHeimdall );
    setContextMenu(m_menu.get());
    update();
    show();

    connect( this, &QSystemTrayIcon::activated,
            this, &HeimdallSystray::handleClick );

    /* Connects on nameChanged() */
    connect( m_intf->p_mainPlayerController, &PlayerController::nameChanged,
            this, &HeimdallSystray::updateTooltipName );
    /* Connect PLAY_STATUS on the systray */
    connect( m_intf->p_mainPlayerController, &PlayerController::playingStateChanged,
            this, &HeimdallSystray::update );
}

HeimdallSystray::~HeimdallSystray()
{}

/**
 * Updates the HeimdallSystray Icon's menu and toggle the main interface
 */
void HeimdallSystray::toggleUpdateMenu()
{
    m_ctx->toggleWindowVisibility(true);
    update();
}

/* First Item of the systray menu */
void HeimdallSystray::showUpdateMenu()
{
    m_ctx->setInterfaceVisibible(true, true);
    update();
}

/* First Item of the systray menu */
void HeimdallSystray::hideUpdateMenu()
{
    m_ctx->setInterfaceVisibible(false);
    update();
}

/* Click on systray Icon */
void HeimdallSystray::handleClick(
    QSystemTrayIcon::ActivationReason reason )
{
    switch( reason )
    {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        toggleUpdateMenu();
        break;
    case QSystemTrayIcon::MiddleClick:
        if (PlaylistController* const playlistController = m_intf->p_mainPlaylistController)
            playlistController->togglePlayPause();
        break;
    default:
        break;
    }
}

/**
 * Updates the name of the systray Icon tooltip.
 * Doesn't check if the systray exists, check before you call it.
 **/
void HeimdallSystray::updateTooltipName( const QString& name )
{
    if( name.isEmpty() )
    {
        setToolTip( qtr( "Heimdall media player" ) );
    }
    else
    {
        setToolTip( name );
        const auto windowVisiblity = m_ctx->interfaceVisibility();
        if( ( m_notificationSetting == NOTIFICATION_ALWAYS ) ||
            ( m_notificationSetting == NOTIFICATION_MINIMIZED && (windowVisiblity == QWindow::Hidden || windowVisiblity == QWindow::Minimized)))
        {
            const auto showMessageTemplate = [this, &name](const auto &icon) {
                showMessage( qtr( "Heimdall media player" ), name, icon, 3000 );
            };

            assert(m_intf);
            assert(m_intf->p_mainPlayerController);
            const QUrl& art = m_intf->p_mainPlayerController->getArtwork();
            if (art.isValid())
            {
                // If there is artwork, use it but only when it is (almost) square:
                const QString& fileName = art.toLocalFile();
                const QImageReader imageReader(fileName);
                if (Q_LIKELY(imageReader.canRead()))
                {
                    const QSize& size = imageReader.size(); // this does not read the whole image
                    if (Q_LIKELY(!size.isEmpty()))
                    {
                        const double ratio = static_cast<double>(size.width()) / size.height();
                        if (Q_LIKELY(std::abs(ratio - 1.0) < 0.2))
                        {
                            showMessageTemplate(QIcon(fileName));
                            return;
                        }
                    }
                }
            }

            showMessageTemplate( QSystemTrayIcon::NoIcon );
        }
    }
    update();
}

void HeimdallSystray::update()
{
    // explictly delete submenus, see QTBUG-11070
    for (QAction *action : m_menu->actions()) {
        if (action->menu()) {
            delete action->menu();
        }
    }
    m_menu->clear();

#ifndef Q_OS_MAC
    /* Hide / Show Heimdall and cone */
    if( m_ctx->interfaceVisibility() != QWindow::Hidden )
    {
        m_menu->addAction(
            QIcon( ":/logo/heimdall16.png" ), qtr( "&Hide Heimdall media player in taskbar" ),
            this, &HeimdallSystray::hideUpdateMenu);
    }
    else
    {
        m_menu->addAction(
            QIcon( ":/logo/heimdall16.png" ), qtr( "Sho&w Heimdall media player" ),
            this, &HeimdallSystray::showUpdateMenu);
    }
    m_menu->addSeparator();
#endif

    HeimdallMenuBar::PopupMenuPlaylistEntries( m_menu.get(), m_intf );
    HeimdallMenuBar::PopupMenuControlEntries( m_menu.get(), m_intf, false );

    HeimdallMenuBar::VolumeEntries( m_intf, m_menu.get() );
    m_menu->addSeparator();
    m_menu->addAction(
        QIcon(":/menu/file.svg"), qtr( "&Open Media" ),
        THEDP, &DialogsProvider::openFileDialog);

    m_menu->addAction(
        QIcon(":/menu/exit.svg"), qtr( "&Quit" ),
        THEDP, &DialogsProvider::quit);

    /* Set the menu */
    setContextMenu( m_menu.get() );
}
