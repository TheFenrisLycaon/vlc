/*****************************************************************************
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

// Safety net for the color scheme picker. The chosen row index is saved in
// the user's settings (MainWindow/color-scheme-index), so the order matters.

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "../util/color_scheme_model.hpp"

#include <QCoreApplication>
#include <QTest>
#include <QAbstractItemModelTester>
#include <QSignalSpy>

class TestColorSchemeModel : public QObject
{
    Q_OBJECT

private slots:
    void schemesInSavedOrder()
    {
        ColorSchemeModel model;
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);

        QCOMPARE(model.rowCount({}), 3);
        const auto schemes = model.getSchemes();
        QCOMPARE(schemes[0].scheme, ColorSchemeModel::System);
        QCOMPARE(schemes[1].scheme, ColorSchemeModel::Day);
        QCOMPARE(schemes[2].scheme, ColorSchemeModel::Night);

        QCOMPARE(model.data(model.index(0), Qt::DisplayRole).toString(), QStringLiteral("System"));
        QCOMPARE(model.data(model.index(1), Qt::DisplayRole).toString(), QStringLiteral("Day"));
        QCOMPARE(model.data(model.index(2), Qt::DisplayRole).toString(), QStringLiteral("Night"));

        QCOMPARE(int(ColorSchemeModel::System), 0);
        QCOMPARE(int(ColorSchemeModel::Day), 1);
        QCOMPARE(int(ColorSchemeModel::Night), 2);
    }

    void startsOnSystem()
    {
        ColorSchemeModel model;
        QCOMPARE(model.currentIndex(), 0);
        QCOMPARE(model.currentScheme(), ColorSchemeModel::System);
        QCOMPARE(model.currentText(), QStringLiteral("System"));
        QCOMPARE(model.data(model.index(0), Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Checked);
        QCOMPARE(model.data(model.index(2), Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Unchecked);
        QVERIFY(model.flags(model.index(1)) & Qt::ItemIsUserCheckable);
    }

    void pickingAScheme()
    {
        ColorSchemeModel model;
        QSignalSpy changed(&model, &ColorSchemeModel::currentChanged);

        model.setCurrentIndex(2);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(model.currentScheme(), ColorSchemeModel::Night);
        QCOMPARE(model.data(model.index(2), Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Checked);
        QCOMPARE(model.data(model.index(0), Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Unchecked);

        model.setCurrentIndex(2); // same index: no signal
        QCOMPARE(changed.count(), 1);

        model.setCurrentIndex(7); // out of range falls back to System
        QCOMPARE(model.currentIndex(), 0);
        QCOMPARE(changed.count(), 2);
    }

    void pickingThroughTheCheckBox()
    {
        ColorSchemeModel model;

        QVERIFY(model.setData(model.index(1), true, Qt::CheckStateRole));
        QCOMPARE(model.currentScheme(), ColorSchemeModel::Day);

        QVERIFY(!model.setData(model.index(1), true, Qt::CheckStateRole));  // already current
        QVERIFY(!model.setData(model.index(2), false, Qt::CheckStateRole)); // unchecking does nothing
        QVERIFY(!model.setData(model.index(2), true, Qt::DisplayRole));     // only the check state is editable
        QCOMPARE(model.currentScheme(), ColorSchemeModel::Day);
    }
};

QTEST_GUILESS_MAIN(TestColorSchemeModel)
#include "test_color_scheme_model.moc"
