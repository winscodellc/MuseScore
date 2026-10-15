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

#include "doricomodestate.h"

using namespace mu::dorico;

void DoricoModeState::setChordMode(bool on)
{
    if (m_chord == on) {
        return;
    }
    m_chord = on;
    if (on) {
        m_insert = false;
    }
    m_changed.notify();
}

void DoricoModeState::setInsertMode(bool on)
{
    if (m_insert == on) {
        return;
    }
    m_insert = on;
    if (on) {
        m_chord = false;
    }
    m_changed.notify();
}

void DoricoModeState::setMode(Mode mode)
{
    if (m_mode == mode) {
        return;
    }
    m_mode = mode;
    m_modeChanged.notify();
}

void DoricoModeState::setLowerZoneTab(const QString& tab)
{
    if (m_lowerZoneTab == tab) {
        return;
    }
    m_lowerZoneTab = tab;
    m_lowerZoneTabChanged.notify();
}
