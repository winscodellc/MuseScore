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

#include "global/async/notification.h"
#include "modularity/imoduleinterface.h"

namespace mu::dorico {
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
};
}
