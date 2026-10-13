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

// Safety net for the control bar layouts: the control ids saved in users'
// settings, the built-in profiles, and how profiles are saved and loaded.

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "../player/control_list_model.hpp"
#include "../player/player_controlbar_model.hpp"
#include "../dialogs/toolbar/controlbar_profile.hpp"
#include "../dialogs/toolbar/controlbar_profile_model.hpp"

#include <QCoreApplication>
#include <QTest>
#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QSettings>
#include <QTemporaryDir>
#include <QMetaEnum>

#include <array>
#include <memory>

using C = ControlListModel;
using Layout = std::array<QVector<int>, 3>;

namespace {

struct DefaultProfile
{
    ControlbarProfileModel::Style style;
    const char* name;
    QVector<QPair<int, Layout>> layouts;
};

// Snapshot of m_defaults in controlbar_profile_model.cpp (2026-10-12).
// A change here means users get different default control bars.
const QVector<DefaultProfile> defaultProfiles {
    { ControlbarProfileModel::Style::EMPTY_STYLE, "Empty Style", {
        { PlayerControlbarModel::Videoplayer, {{ {}, {}, {} }} },
        { PlayerControlbarModel::Audioplayer, {{ {}, {}, {} }} },
        { PlayerControlbarModel::Miniplayer, {{ {}, {}, {} }} },
    } },
    { ControlbarProfileModel::Style::DEFAULT_STYLE, "Default Style", {
        { PlayerControlbarModel::Videoplayer, {{ { C::LANG_BUTTON, C::BOOKMARK_BUTTON, C::EXTENDED_BUTTON, C::NAVIGATION_BUTTONS }, { C::SKIP_BACK_BUTTON, C::PREVIOUS_BUTTON, C::PLAY_BUTTON, C::NEXT_BUTTON, C::SKIP_FW_BUTTON }, { C::VOLUME, C::RENDERER_BUTTON, C::FULLSCREEN_BUTTON } }} },
        { PlayerControlbarModel::Audioplayer, {{ { C::LANG_BUTTON, C::BOOKMARK_BUTTON, C::EXTENDED_BUTTON }, { C::RANDOM_BUTTON, C::PREVIOUS_BUTTON, C::PLAY_BUTTON, C::NEXT_BUTTON, C::LOOP_BUTTON }, { C::VOLUME, C::RENDERER_BUTTON, C::FULLSCREEN_BUTTON } }} },
        { PlayerControlbarModel::Miniplayer, {{ { C::ARTWORK_INFO }, { C::RANDOM_BUTTON, C::PREVIOUS_BUTTON, C::PLAY_BUTTON, C::NEXT_BUTTON, C::LOOP_BUTTON }, { C::VOLUME, C::PLAYER_SWITCH_BUTTON } }} },
    } },
    { ControlbarProfileModel::Style::MINIMALIST_STYLE, "Minimalist Style", {
        { PlayerControlbarModel::Videoplayer, {{ { C::PLAY_BUTTON, C::WIDGET_SPACER, C::PREVIOUS_BUTTON, C::STOP_BUTTON, C::NEXT_BUTTON, C::WIDGET_SPACER, C::RECORD_BUTTON, C::WIDGET_SPACER, C::NAVIGATION_BUTTONS, C::WIDGET_SPACER, C::PLAYLIST_BUTTON }, {}, { C::VOLUME } }} },
        { PlayerControlbarModel::Audioplayer, {{ { C::PLAY_BUTTON, C::WIDGET_SPACER, C::PREVIOUS_BUTTON, C::STOP_BUTTON, C::NEXT_BUTTON, C::WIDGET_SPACER, C::RECORD_BUTTON, C::WIDGET_SPACER, C::NAVIGATION_BUTTONS, C::WIDGET_SPACER, C::PLAYLIST_BUTTON }, {}, { C::VOLUME } }} },
        { PlayerControlbarModel::Miniplayer, {{ { C::ARTWORK_INFO }, { C::PREVIOUS_BUTTON, C::PLAY_BUTTON, C::STOP_BUTTON, C::NEXT_BUTTON }, {} }} },
    } },
    { ControlbarProfileModel::Style::ONE_LINER_STYLE, "One-liner Style", {
        { PlayerControlbarModel::Videoplayer, {{ { C::PLAY_BUTTON, C::WIDGET_SPACER, C::PREVIOUS_BUTTON, C::STOP_BUTTON, C::NEXT_BUTTON, C::WIDGET_SPACER, C::FULLSCREEN_BUTTON, C::PLAYLIST_BUTTON, C::EXTENDED_BUTTON, C::WIDGET_SPACER, C::RECORD_BUTTON, C::SNAPSHOT_BUTTON, C::ATOB_BUTTON, C::FRAME_NEXT_BUTTON }, {}, { C::VOLUME } }} },
        { PlayerControlbarModel::Audioplayer, {{ { C::PLAY_BUTTON, C::WIDGET_SPACER, C::PREVIOUS_BUTTON, C::STOP_BUTTON, C::NEXT_BUTTON, C::WIDGET_SPACER, C::FULLSCREEN_BUTTON, C::PLAYLIST_BUTTON, C::EXTENDED_BUTTON, C::WIDGET_SPACER, C::RECORD_BUTTON, C::SNAPSHOT_BUTTON, C::ATOB_BUTTON, C::FRAME_NEXT_BUTTON }, {}, { C::VOLUME } }} },
        { PlayerControlbarModel::Miniplayer, {{ { C::ARTWORK_INFO }, { C::RANDOM_BUTTON, C::PREVIOUS_BUTTON, C::PLAY_BUTTON, C::STOP_BUTTON, C::NEXT_BUTTON, C::LOOP_BUTTON }, {} }} },
    } },
    { ControlbarProfileModel::Style::SIMPLEST_STYLE, "Simplest Style", {
        { PlayerControlbarModel::Videoplayer, {{ { C::VOLUME }, { C::PLAY_BUTTON, C::NEXT_BUTTON, C::STOP_BUTTON }, { C::FULLSCREEN_BUTTON } }} },
        { PlayerControlbarModel::Audioplayer, {{ { C::VOLUME }, { C::PLAY_BUTTON, C::NEXT_BUTTON, C::STOP_BUTTON }, { C::FULLSCREEN_BUTTON } }} },
        { PlayerControlbarModel::Miniplayer, {{ { C::ARTWORK_INFO }, { C::PREVIOUS_BUTTON, C::PLAY_BUTTON, C::NEXT_BUTTON }, {} }} },
    } },
    { ControlbarProfileModel::Style::CLASSIC_STYLE, "Classic Style", {
        { PlayerControlbarModel::Videoplayer, {{ { C::PLAY_BUTTON, C::WIDGET_SPACER, C::PREVIOUS_BUTTON, C::STOP_BUTTON, C::NEXT_BUTTON, C::WIDGET_SPACER, C::FULLSCREEN_BUTTON, C::EXTENDED_BUTTON, C::WIDGET_SPACER, C::PLAYLIST_BUTTON, C::LOOP_BUTTON, C::RANDOM_BUTTON }, {}, { C::VOLUME } }} },
        { PlayerControlbarModel::Audioplayer, {{ { C::PLAY_BUTTON, C::WIDGET_SPACER, C::PREVIOUS_BUTTON, C::STOP_BUTTON, C::NEXT_BUTTON, C::WIDGET_SPACER, C::FULLSCREEN_BUTTON, C::EXTENDED_BUTTON, C::WIDGET_SPACER, C::PLAYLIST_BUTTON, C::LOOP_BUTTON, C::RANDOM_BUTTON }, {}, { C::VOLUME } }} },
        { PlayerControlbarModel::Miniplayer, {{ { C::ARTWORK_INFO }, { C::PLAY_BUTTON, C::WIDGET_SPACER, C::PREVIOUS_BUTTON, C::STOP_BUTTON, C::NEXT_BUTTON, C::WIDGET_SPACER, C::FULLSCREEN_BUTTON, C::EXTENDED_BUTTON, C::WIDGET_SPACER, C::PLAYLIST_BUTTON, C::LOOP_BUTTON, C::RANDOM_BUTTON }, { C::VOLUME } }} },
    } },
};

QStringList profileNames(const ControlbarProfileModel& model)
{
    QStringList names;
    for (int i = 0; i < model.rowCount(); ++i)
        names << model.getProfile(i)->name();
    return names;
}

} // namespace

class TestControlbarLayout : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_dir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_dir->isValid());
        m_settings = std::make_unique<QSettings>(m_dir->filePath("vlc-qt-interface.ini"), QSettings::IniFormat);
    }

    void cleanup()
    {
        m_settings.reset();
        m_dir.reset();
    }

    // Saved layouts store these numbers. Renumbering breaks every saved layout.
    void controlTypeValues_data()
    {
        QTest::addColumn<QByteArray>("key");
        QTest::addColumn<int>("value");

        const QVector<QPair<const char*, int>> pinned {
            {"PLAY_BUTTON", 0}, {"STOP_BUTTON", 1}, {"OPEN_BUTTON", 2}, {"PREVIOUS_BUTTON", 3},
            {"NEXT_BUTTON", 4}, {"SLOWER_BUTTON", 5}, {"FASTER_BUTTON", 6}, {"FULLSCREEN_BUTTON", 7},
            {"EXTENDED_BUTTON", 8}, {"PLAYLIST_BUTTON", 9}, {"SNAPSHOT_BUTTON", 10}, {"RECORD_BUTTON", 11},
            {"ATOB_BUTTON", 12}, {"FRAME_NEXT_BUTTON", 13}, {"SKIP_BACK_BUTTON", 14}, {"SKIP_FW_BUTTON", 15},
            {"QUIT_BUTTON", 16}, {"RANDOM_BUTTON", 17}, {"LOOP_BUTTON", 18}, {"INFO_BUTTON", 19},
            {"LANG_BUTTON", 20}, {"MENU_BUTTON", 21}, {"BACK_BUTTON", 22}, {"CHAPTER_PREVIOUS_BUTTON", 23},
            {"CHAPTER_NEXT_BUTTON", 24}, {"BUTTON_MAX", 25}, {"PLAYER_SWITCH_BUTTON", 26}, {"ARTWORK_INFO", 27},
            {"PLAYBACK_SPEED_BUTTON", 28}, {"HIGH_RESOLUTION_TIME_WIDGET", 29}, {"FRAME_PREV_BUTTON", 30},
            {"SPLITTER", 0x20}, {"VOLUME", 0x21}, {"TELETEXT_BUTTONS", 0x22}, {"ASPECT_RATIO_COMBOBOX", 0x23},
            {"DVD_MENUS_BUTTON", 0x24}, {"REVERSE_BUTTON", 0x25}, {"BOOKMARK_BUTTON", 0x26},
            {"RENDERER_BUTTON", 0x27}, {"NAVIGATION_BUTTONS", 0x28}, {"PROGRAM_BUTTON", 0x29},
            {"NAVIGATION_BOX", 0x2A}, {"WIDGET_SPACER", 0x40}, {"WIDGET_SPACER_EXTEND", 0x41},
            {"WIDGET_MAX", 0x42},
        };
        for (const auto& [key, value] : pinned)
            QTest::newRow(key) << QByteArray(key) << value;
    }

    void controlTypeValues()
    {
        QFETCH(QByteArray, key);
        QFETCH(int, value);

        bool ok = false;
        const int actual = QMetaEnum::fromType<C::ControlType>().keyToValue(key.constData(), &ok);
        QVERIFY2(ok, "control type was renamed or removed");
        QCOMPARE(actual, value);
    }

    void playerIdentifierValues()
    {
        QCOMPARE(int(PlayerControlbarModel::Videoplayer), 0);
        QCOMPARE(int(PlayerControlbarModel::Audioplayer), 1);
        QCOMPARE(int(PlayerControlbarModel::Miniplayer), 2);
    }

    void controlListModelEditing()
    {
        ControlListModel model;
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
        QSignalSpy countSpy(&model, &ControlListModel::countChanged);

        model.setControls({ C::PLAY_BUTTON, C::STOP_BUTTON, C::VOLUME });
        QCOMPARE(model.getControls(), QVector<int>({ C::PLAY_BUTTON, C::STOP_BUTTON, C::VOLUME }));
        QCOMPARE(model.data(model.index(2), C::ID_ROLE).toInt(), int(C::VOLUME));
        QCOMPARE(model.roleNames().value(C::ID_ROLE), QByteArray("id"));

        model.insert(1, {{ "id", int(C::NEXT_BUTTON) }});
        QCOMPARE(model.getControls(), QVector<int>({ C::PLAY_BUTTON, C::NEXT_BUTTON, C::STOP_BUTTON, C::VOLUME }));

        model.move(0, 3);
        QCOMPARE(model.getControls(), QVector<int>({ C::NEXT_BUTTON, C::STOP_BUTTON, C::VOLUME, C::PLAY_BUTTON }));

        model.move(3, 0);
        QCOMPARE(model.getControls(), QVector<int>({ C::PLAY_BUTTON, C::NEXT_BUTTON, C::STOP_BUTTON, C::VOLUME }));

        model.remove(1);
        QCOMPARE(model.getControls(), QVector<int>({ C::PLAY_BUTTON, C::STOP_BUTTON, C::VOLUME }));

        QVERIFY(model.setData(model.index(1), int(C::RECORD_BUTTON), C::ID_ROLE));
        QVERIFY(!model.setData(model.index(1), int(C::RECORD_BUTTON), C::ID_ROLE)); // same value: no change
        QCOMPARE(model.getControls(), QVector<int>({ C::PLAY_BUTTON, C::RECORD_BUTTON, C::VOLUME }));

        model.clear();
        QCOMPARE(model.rowCount(), 0);
        QVERIFY(countSpy.count() >= 5);
    }

    void playerControlbarModelDirtyAndEmpty()
    {
        PlayerControlbarModel model;
        QVERIFY(model.empty());
        QVERIFY(!model.dirty());

        const Layout layout {{ { C::LANG_BUTTON }, { C::PLAY_BUTTON }, { C::VOLUME } }};
        model.loadModels(layout);
        QVERIFY(!model.empty());
        QVERIFY(model.dirty());
        QCOMPARE(model.serializeModels(), layout);

        model.setDirty(false);
        model.center()->insert(0, {{ "id", int(C::PREVIOUS_BUTTON) }});
        QVERIFY(model.dirty());

        PlayerControlbarModel other;
        other.loadModels(model.serializeModels());
        QVERIFY(model == other);
        other.right()->remove(0);
        QVERIFY(model != other);

        model.loadModels({});
        QVERIFY(model.empty());
    }

    void builtInProfiles_data()
    {
        QTest::addColumn<int>("index");
        for (int i = 0; i < defaultProfiles.size(); ++i)
            QTest::newRow(defaultProfiles[i].name) << i;
    }

    void builtInProfiles()
    {
        QFETCH(int, index);
        const DefaultProfile& expected = defaultProfiles[index];

        std::unique_ptr<ControlbarProfile> profile { ControlbarProfileModel::generateProfileFromStyle(expected.style) };
        QVERIFY(profile);
        QCOMPARE(profile->name(), QString(expected.name));
        QVERIFY(!profile->dirty());

        for (const auto& [identifier, layout] : expected.layouts)
            QCOMPARE(profile->getModelData(identifier), layout);
    }

    void firstStartLoadsDefaults()
    {
        ControlbarProfileModel model(m_settings.get());
        // the model loads from settings on the next event loop turn
        QTRY_COMPARE(model.rowCount(), defaultProfiles.size() - 1);

        // every built-in style except "Empty", in order
        QStringList expected;
        for (const auto& p : defaultProfiles)
            if (p.style != ControlbarProfileModel::Style::EMPTY_STYLE)
                expected << p.name;
        QCOMPARE(profileNames(model), expected);
        QCOMPARE(model.selectedProfile(), 0);
    }

    void saveThenReload()
    {
        {
            ControlbarProfileModel model(m_settings.get());
            QTRY_COMPARE(model.rowCount(), defaultProfiles.size() - 1);

            const auto custom = model.newProfile("Heimdall");
            QVERIFY(custom);
            custom->setModelData(PlayerControlbarModel::Videoplayer, {{ { C::PLAY_BUTTON }, {}, { C::VOLUME, C::FULLSCREEN_BUTTON } }});
            QVERIFY(model.setSelectedProfile(model.rowCount() - 1));
            model.save();
        }

        ControlbarProfileModel reloaded(m_settings.get());
        QTRY_COMPARE(reloaded.rowCount(), defaultProfiles.size());
        QCOMPARE(reloaded.selectedProfile(), reloaded.rowCount() - 1);

        const auto custom = reloaded.currentModel();
        QCOMPARE(custom->name(), QStringLiteral("Heimdall"));
        QCOMPARE(custom->getModelData(PlayerControlbarModel::Videoplayer),
                 Layout({{ { C::PLAY_BUTTON }, {}, { C::VOLUME, C::FULLSCREEN_BUTTON } }}));
        QVERIFY(!custom->dirty());

        // the defaults survive the round trip unchanged
        std::unique_ptr<ControlbarProfile> def { ControlbarProfileModel::generateProfileFromStyle(ControlbarProfileModel::Style::DEFAULT_STYLE) };
        for (int id : { PlayerControlbarModel::Videoplayer, PlayerControlbarModel::Audioplayer, PlayerControlbarModel::Miniplayer })
            QCOMPARE(reloaded.getProfile(0)->getModelData(id), def->getModelData(id));
    }

    // The settings format: "$<player id>|<left ids>|<center ids>|<right ids>", repeated.
    void readsHandWrittenSettings()
    {
        m_settings->beginGroup("ControlbarProfileModel");
        m_settings->setValue("SelectedProfile", 1);
        m_settings->beginWriteArray("Profiles");
        m_settings->setArrayIndex(0);
        m_settings->setValue("Name", "Good");
        m_settings->setValue("Model", "$0|20,38|3,0,4|33,7$2|27||33");
        m_settings->setArrayIndex(1);
        m_settings->setValue("Name", "Partly broken");
        // a bad player id and a part with too few fields are skipped; junk ids are dropped
        m_settings->setValue("Model", "$x|0|0|0$1|0|0$0|0,junk,4||");
        m_settings->setArrayIndex(2);
        m_settings->setValue("Name", "Empty model");
        m_settings->setValue("Model", "");
        m_settings->endArray();
        m_settings->endGroup();

        ControlbarProfileModel model(m_settings.get());
        QTRY_COMPARE(model.rowCount(), 2); // the profile with an empty model is skipped
        QCOMPARE(profileNames(model), QStringList({ "Good", "Partly broken" }));
        QCOMPARE(model.selectedProfile(), 1);

        const auto good = model.getProfile(0);
        QCOMPARE(good->getModelData(PlayerControlbarModel::Videoplayer),
                 Layout({{ { C::LANG_BUTTON, C::BOOKMARK_BUTTON }, { C::PREVIOUS_BUTTON, C::PLAY_BUTTON, C::NEXT_BUTTON }, { C::VOLUME, C::FULLSCREEN_BUTTON } }}));
        QCOMPARE(good->getModelData(PlayerControlbarModel::Miniplayer),
                 Layout({{ { C::ARTWORK_INFO }, {}, { C::VOLUME } }}));

        const auto partly = model.getProfile(1);
        QCOMPARE(partly->getModelData(PlayerControlbarModel::Videoplayer),
                 Layout({{ { C::PLAY_BUTTON, C::NEXT_BUTTON }, {}, {} }}));
        QVERIFY(!partly->getModel(PlayerControlbarModel::Audioplayer));
    }

    void newCloneAndDeleteProfiles()
    {
        ControlbarProfileModel model(m_settings.get());
        QTRY_COMPARE(model.rowCount(), defaultProfiles.size() - 1);
        const int defaults = model.rowCount();

        // names stay unique
        QVERIFY(!model.newProfile(QString()));
        QCOMPARE(model.newProfile("Default Style")->name(), QStringLiteral("Default Style (1)"));
        QCOMPARE(model.newProfile("Default Style")->name(), QStringLiteral("Default Style (2)"));
        QCOMPARE(model.rowCount(), defaults + 2);

        // cloning copies the selected layouts under a unique name
        QVERIFY(model.setSelectedProfile(1));
        model.cloneSelectedProfile("Minimal copy");
        const auto clone = model.getProfile(model.rowCount() - 1);
        QCOMPARE(clone->name(), QStringLiteral("Minimal copy"));
        QVERIFY(*clone == *model.getProfile(1));

        model.cloneSelectedProfile(QString());
        QCOMPARE(model.getProfile(model.rowCount() - 1)->name(), model.getProfile(1)->name() + " (1)");

        // deleting selects the profile before it
        const QString removed = model.getProfile(2)->name();
        QVERIFY(model.setSelectedProfile(2));
        const int before = model.rowCount();
        model.deleteSelectedProfile();
        QCOMPARE(model.rowCount(), before - 1);
        QVERIFY(!profileNames(model).contains(removed));
        QCOMPARE(model.selectedProfile(), 1);

        // deleting every profile brings the defaults back
        for (int n = model.rowCount(); n > 0; --n)
        {
            model.setSelectedProfile(0);
            model.deleteSelectedProfile();
        }
        QCOMPARE(model.rowCount(), defaults);
        QCOMPARE(profileNames(model).first(), QStringLiteral("Default Style"));
    }

    void removeRowsRemovesTheRequestedRows()
    {
        ControlbarProfileModel model(m_settings.get());
        QTRY_COMPARE(model.rowCount(), defaultProfiles.size() - 1);
        const int before = model.rowCount();

        QVERIFY(model.removeRows(0, 1));
        // BUG (today): removeRows() erases [row, row + count - 1), one row too few,
        // while telling views that `count` rows went away. Nothing calls it yet.
        QEXPECT_FAIL("", "ControlbarProfileModel::removeRows removes count - 1 rows", Continue);
        QCOMPARE(model.rowCount(), before - 1);
    }

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<QSettings> m_settings;
};

QTEST_GUILESS_MAIN(TestControlbarLayout)
#include "test_controlbar_layout.moc"
