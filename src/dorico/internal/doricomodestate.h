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

#pragma once

#include "../idoricomodestate.h"

namespace mu::dorico {
class DoricoModeState : public IDoricoModeState
{
public:
    bool isChordMode() const override { return m_chord; }
    bool isInsertMode() const override { return m_insert; }
    void setChordMode(bool on) override;
    void setInsertMode(bool on) override;
    muse::async::Notification modesChanged() const override { return m_changed; }

    Mode mode() const override { return m_mode; }
    void setMode(Mode mode) override;
    muse::async::Notification modeChanged() const override { return m_modeChanged; }
    muse::async::Notification modeAboutToChange() const override { return m_modeAboutToChange; }
    muse::async::Notification zonesReset() const override { return m_zonesReset; }

    QString lowerZoneTab() const override { return m_lowerZoneTab; }
    void setLowerZoneTab(const QString& tab) override;
    muse::async::Notification lowerZoneTabChanged() const override { return m_lowerZoneTabChanged; }

    bool isVideoWindowVisible() const override { return m_videoWindow; }
    void setVideoWindowVisible(bool visible) override;

private:
    bool m_chord = false;
    bool m_insert = false;
    bool m_videoWindow = false;
    muse::async::Notification m_changed;
    Mode m_mode = Mode::Write;
    muse::async::Notification m_modeChanged;
    muse::async::Notification m_modeAboutToChange;
    muse::async::Notification m_zonesReset;
    QString m_lowerZoneTab = QStringLiteral("properties");
    muse::async::Notification m_lowerZoneTabChanged;
};
}
