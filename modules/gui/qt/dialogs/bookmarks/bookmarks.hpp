/*****************************************************************************
 * bookmarks.hpp : bookmarks
 ****************************************************************************
 * Copyright (C) 2006 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Antoine Lejeune <phytos@via.ecp.fr>
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


#ifndef QHEIMDALL_BOOKMARKS_H_
#define QHEIMDALL_BOOKMARKS_H_ 1

#include "widgets/native/qheimdallframe.hpp"

class QPushButton;
class MLBookmarkModel;
class QTreeView;

class BookmarksDialog : public QHeimdallFrame
{
    Q_OBJECT

public:
    BookmarksDialog( qt_intf_t * );
    virtual ~BookmarksDialog();

public:
    void toggleVisible();

private:
    QTreeView *bookmarksList;
    QPushButton *clearButton;
    QPushButton *delButton;
    QPushButton *addButton;
    MLBookmarkModel* m_model;

private slots:
    void add();
    void del();
    void clear();
    void extract();
    void activateItem( const QModelIndex& index );
    void updateButtons();
};

#endif

