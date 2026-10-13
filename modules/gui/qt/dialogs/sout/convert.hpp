/*****************************************************************************
 * convert.hpp : GotoTime dialogs
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

#ifndef QHEIMDALL_CONVERT_DIALOG_H_
#define QHEIMDALL_CONVERT_DIALOG_H_ 1

#include <QUrl>

#include "widgets/native/qheimdallframe.hpp"

class QLineEdit;
class QCheckBox;
class QRadioButton;
class QPushButton;
class HeimdallProfileSelector;

class ConvertDialog : public QHeimdallDialog
{
    Q_OBJECT
public:
    ConvertDialog(QWindow *, qt_intf_t *, const QStringList& );
    virtual ~ConvertDialog(){}

    QStringList getMrls() {return mrls;}

private:
    QLineEdit *fileLine;

    QCheckBox *displayBox, *deinterBox, *appendBox;
    QRadioButton *dumpRadio;
    QPushButton *okButton;
    HeimdallProfileSelector *profile;

    const QStringList *incomingMRLs;
    const bool singleFileSelected;
    QUrl outgoingMRL;
    QStringList mrls;

public slots:
    void accept() override;

private slots:
    void fileBrowse();
    void setDestinationFileExtension();
    void validate();
};

#endif
