/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <set>

#include <QColor>

#include "dorico/internal/prefstable.h"

#include "notation/tests/mocks/notationconfigurationmock.h"
#include "notation/types/zoomtype.h"
#include "project/tests/mocks/projectconfigurationmock.h"
#include "playback/tests/mocks/playbackconfigurationmock.h"
#include "ui/tests/mocks/uiconfigurationmock.h"

using namespace mu::dorico;
using namespace testing;
using muse::Val;

namespace {
//! Mocks that remember what was set, so a row's getter can be checked against its setter
class DoricoPrefTableTests : public Test
{
public:
    void SetUp() override
    {
        notation = std::make_shared<NiceMock<mu::notation::NotationConfigurationMock> >();
        project = std::make_shared<NiceMock<mu::project::ProjectConfigurationMock> >();
        playback = std::make_shared<NiceMock<mu::playback::PlaybackConfigurationMock> >();
        ui = std::make_shared<NiceMock<muse::ui::UiConfigurationMock> >();

        ON_CALL(*notation, isAutomaticallyPanEnabled()).WillByDefault([this]() { return pan; });
        ON_CALL(*notation, setIsAutomaticallyPanEnabled(_)).WillByDefault([this](bool v) { pan = v; });
        ON_CALL(*notation, isMidiInputEnabled()).WillByDefault([this]() { return midiInput; });
        ON_CALL(*notation, setIsMidiInputEnabled(_)).WillByDefault([this](bool v) { midiInput = v; });
        ON_CALL(*notation, addAccidentalDotsArticulationsToNextNoteEntered()).WillByDefault([this]() { return nextNote; });
        ON_CALL(*notation, setAddAccidentalDotsArticulationsToNextNoteEntered(_)).WillByDefault([this](bool v) { nextNote = v; });
        ON_CALL(*notation, scoreInversionEnabled()).WillByDefault([this]() { return invert; });
        ON_CALL(*notation, setScoreInversionEnabled(_)).WillByDefault([this](bool v) { invert = v; });
        ON_CALL(*notation, foregroundColor()).WillByDefault([this]() { return foreground; });
        ON_CALL(*notation, setForegroundColor(_)).WillByDefault([this](const QColor& c) { foreground = c; });
        ON_CALL(*notation, defaultZoom()).WillByDefault([this]() { return zoom; });
        ON_CALL(*notation, setDefaultZoom(_)).WillByDefault([this](int v) { zoom = v; });
        ON_CALL(*notation, defaultZoomType()).WillByDefault([this]() { return zoomType; });
        ON_CALL(*notation, setDefaultZoomType(_)).WillByDefault([this](mu::notation::ZoomType v) { zoomType = v; });

        ON_CALL(*project, userProjectsPath()).WillByDefault([this]() { return folder; });
        ON_CALL(*project, setUserProjectsPath(_)).WillByDefault([this](const muse::io::path_t& v) { folder = v; });
        ON_CALL(*project, isAutoSaveEnabled()).WillByDefault([this]() { return autoSave; });
        ON_CALL(*project, setAutoSaveEnabled(_)).WillByDefault([this](bool v) { autoSave = v; });
        ON_CALL(*project, autoSaveIntervalMinutes()).WillByDefault([this]() { return minutes; });
        ON_CALL(*project, setAutoSaveInterval(_)).WillByDefault([this](int v) { minutes = v; });

        ON_CALL(*playback, playNotesWhenEditing()).WillByDefault([this]() { return playNotes; });
        ON_CALL(*playback, setPlayNotesWhenEditing(_)).WillByDefault([this](bool v) { playNotes = v; });
        ON_CALL(*playback, playChordWhenEditing()).WillByDefault([this]() { return playChord; });
        ON_CALL(*playback, setPlayChordWhenEditing(_)).WillByDefault([this](bool v) { playChord = v; });

        theme.codeKey = "light";
        ON_CALL(*ui, currentTheme()).WillByDefault(ReturnRef(theme));
        ON_CALL(*ui, setCurrentTheme(_)).WillByDefault([this](const muse::ui::ThemeCode& c) { theme.codeKey = c; });
    }

    PrefTable table() const
    {
        PrefBackends b;
        b.notation = notation;
        b.project = project;
        b.playback = playback;
        b.ui = ui;
        return buildPrefTable(b);
    }

    std::shared_ptr<NiceMock<mu::notation::NotationConfigurationMock> > notation;
    std::shared_ptr<NiceMock<mu::project::ProjectConfigurationMock> > project;
    std::shared_ptr<NiceMock<mu::playback::PlaybackConfigurationMock> > playback;
    std::shared_ptr<NiceMock<muse::ui::UiConfigurationMock> > ui;

    bool pan = false, midiInput = false, nextNote = false, invert = false, autoSave = false, playNotes = false, playChord = false;
    QColor foreground = QColor("#ffffff");
    int zoom = 100, minutes = 2;
    mu::notation::ZoomType zoomType = mu::notation::ZoomType::Percentage;
    muse::io::path_t folder = "/tmp/a";
    muse::ui::ThemeInfo theme;
};

//! A value different from the row's current one
Val changed(const PrefRow& row, const Val& current)
{
    switch (row.type) {
    case PrefType::Check: return Val(!current.toBool());
    case PrefType::Spinner: return Val(current.toInt() + 3);
    case PrefType::Path: return Val("/tmp/b");
    case PrefType::Hex: return Val("#123456");
    default: break;
    }
    for (const PrefOption& o : row.options) {
        if (!(o.value == current)) {
            return o.value;
        }
    }
    return Val("dark"); // the theme list is empty under the mock
}
}

TEST(DoricoPrefTable, CategoriesInDoricoOrder)
{
    const PrefTable table = buildPrefTable({});
    std::vector<std::string> titles;
    for (const PrefCategory& c : table) {
        titles.push_back(c.title);
    }
    EXPECT_EQ(titles, (std::vector<std::string> { "Colors", "General", "Key Commands", "MusicXML", "Note Input and Editing", "Play",
                                                  "Proofreading", "Remote", "VST Plug-ins" }));

    std::vector<std::string> general;
    for (const PrefPage& p : table[1].pages) {
        general.push_back(p.title);
    }
    EXPECT_EQ(general, (std::vector<std::string> { "General", "Window", "View", "Files" }));
}

TEST(DoricoPrefTable, RowIdsAreUniqueAndNamed)
{
    const PrefTable table = buildPrefTable({});
    std::set<std::string> seen;
    for (const std::string& id : prefRowIds(table)) {
        EXPECT_FALSE(id.empty());
        EXPECT_TRUE(seen.insert(id).second) << "duplicate row id " << id;
    }
    EXPECT_GT(seen.size(), 200u);
}

TEST(DoricoPrefTable, NothingIsBackedWithoutConfigurations)
{
    const PrefTable table = buildPrefTable({});
    for (const std::string& id : prefRowIds(table)) {
        EXPECT_FALSE(findPrefRow(table, id)->backed()) << id;
    }
}

TEST_F(DoricoPrefTableTests, BackedRowsAreTheExpectedOnes)
{
    const PrefTable t = table();
    std::set<std::string> backed;
    for (const std::string& id : prefRowIds(t)) {
        if (findPrefRow(t, id)->backed()) {
            backed.insert(id);
        }
    }
    // language and the MusicXML option need configurations that have no mock
    EXPECT_EQ(backed, (std::set<std::string> {
        "general.window.theme", "general.view.defaultZoom", "general.files.projectFolder", "general.files.autoSave",
        "general.files.autoSaveMinutes", "colors.page.fullScore", "colors.page.invert", "note.input.accidentalTiming",
        "note.input.playNotes", "note.input.playChord", "play.playhead.follow", "play.recording.midiInput" }));
}

TEST_F(DoricoPrefTableTests, EveryBackedRowRoundTrips)
{
    const PrefTable t = table();
    int checked = 0;
    for (const std::string& id : prefRowIds(t)) {
        const PrefRow* row = findPrefRow(t, id);
        if (!row->backed()) {
            continue;
        }
        const Val next = changed(*row, row->get());
        row->set(next);
        EXPECT_EQ(row->get(), next) << id;
        ++checked;
    }
    EXPECT_EQ(checked, 12);
}

TEST_F(DoricoPrefTableTests, ZoomRowMapsToZoomTypeAndPercentage)
{
    const PrefTable t = table();
    const PrefRow* row = findPrefRow(t, "general.view.defaultZoom");
    row->set(Val("pagewidth"));
    EXPECT_EQ(zoomType, mu::notation::ZoomType::PageWidth);
    row->set(Val("pct:150"));
    EXPECT_EQ(zoomType, mu::notation::ZoomType::Percentage);
    EXPECT_EQ(zoom, 150);
    EXPECT_EQ(row->get().toString(), "pct:150");
}

TEST_F(DoricoPrefTableTests, AutoSaveMinutesIsAnExtraOfTheCheck)
{
    const PrefTable t = table();
    const PrefRow* row = findPrefRow(t, "general.files.autoSave");
    ASSERT_TRUE(row->extra);
    EXPECT_EQ(row->extra->id, "general.files.autoSaveMinutes");
    row->extra->set(Val(9));
    EXPECT_EQ(minutes, 9);
}
