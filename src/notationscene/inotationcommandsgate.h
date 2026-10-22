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

#include "modularity/imoduleinterface.h"

#include "global/async/notification.h"

#include "rcommand/commandtypes.h"

namespace mu::notation {
//! Dorico fork (spec 97): lets another module disable notation commands, e.g. per editing mode. Optional: with no
//! gate registered every command keeps its usual state.
class INotationCommandsGate : MODULE_CONTEXT_INTERFACE
{
    INTERFACE_ID(INotationCommandsGate)
public:
    virtual ~INotationCommandsGate() = default;

    virtual bool isCommandAllowed(const muse::rcommand::Command& command) const = 0;
    virtual muse::async::Notification changed() const = 0;
};
}
