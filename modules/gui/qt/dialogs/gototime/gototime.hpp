/*****************************************************************************
 * gototime.hpp : GotoTime dialogs
 ****************************************************************************
 * Copyright (C) 2007 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Jean-Baptiste Kempf <jb (at) videolan.org>
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

#ifndef QHEIMDALL_GOTOTIME_DIALOG_H_
#define QHEIMDALL_GOTOTIME_DIALOG_H_ 1

#include "widgets/native/qheimdallframe.hpp"

class QTimeEdit;

class GotoTimeDialog : public QHeimdallDialog
{
    Q_OBJECT
public:
    GotoTimeDialog( qt_intf_t * );
    virtual ~GotoTimeDialog();

    void toggleVisible();

public slots:
    void accept() override;
    void reject() override;

private slots:
    void reset();

private:
    QTimeEdit *timeEdit = nullptr;
};

#endif
