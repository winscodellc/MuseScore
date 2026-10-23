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

#include <QString>

#include "global/async/notification.h"
#include "modularity/imoduleinterface.h"

namespace mu::dorico {
//! The five project window modes (spec 50). Write is the default when a project opens.
enum class Mode {
    Setup,
    Write,
    Engrave,
    Play,
    Print
};

//! Dorico's toolbox toggles for note input. Chord and Insert are mutually exclusive.
class IDoricoModeState : MODULE_CONTEXT_INTERFACE
{
    INTERFACE_ID(IDoricoModeState)

public:
    virtual ~IDoricoModeState() = default;

    virtual bool isChordMode() const = 0;
    virtual bool isInsertMode() const = 0;
    virtual void setChordMode(bool on) = 0;
    virtual void setInsertMode(bool on) = 0;
    virtual muse::async::Notification modesChanged() const = 0;

    virtual Mode mode() const = 0;
    virtual void setMode(Mode mode) = 0;
    virtual muse::async::Notification modeChanged() const = 0;
    //! Sent by setMode before the mode changes, so the page can save the leaving mode's zone sizes (spec 95).
    virtual muse::async::Notification modeAboutToChange() const = 0;
    //! Sent after Restore Default Layout put every mode's default zones back, so the page applies the default sizes.
    virtual muse::async::Notification zonesReset() const = 0;

    //! Selected tab of the Write-mode lower zone (spec 54): properties, keyboard, fretboard, drumPads, mixer, keyEditor, timeline.
    virtual QString lowerZoneTab() const = 0;
    virtual void setLowerZoneTab(const QString& tab) = 0;
    virtual muse::async::Notification lowerZoneTabChanged() const = 0;

    //! The Video window (spec 91), shown in every mode. Changes send modesChanged.
    virtual bool isVideoWindowVisible() const = 0;
    virtual void setVideoWindowVisible(bool visible) = 0;
};
}
