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

#include <string>

#include "global/async/asyncable.h"
#include "global/modularity/ioc.h"
#include "rcommand/icommanddispatcher.h"
#include "notationscene/inotationcommandsgate.h"

#include "../idoricomodestate.h"

namespace mu::dorico {
//! What each mode lets you edit (spec 97). Only notation and Dorico commands are gated; a command of either that is
//! not listed for a mode works in Write only.
bool isCommandAllowed(Mode mode, const std::string& command);

//! notation::InteractionCaps flags for the score view in a mode.
int interactionCaps(Mode mode);

//! Feeds the policy to the notation commands' enabled state, and stops blocked commands that code dispatches directly
//! (the dispatcher does not check the enabled state; e.g. double-clicking a bar starts note input).
class ModeCommandsGate : public notation::INotationCommandsGate, public muse::Contextable, public muse::async::Asyncable
{
    muse::ContextInject<IDoricoModeState> modes = { this };
    muse::ContextInject<muse::rcommand::ICommandDispatcher> dispatcher = { this };

public:
    ModeCommandsGate(const muse::modularity::ContextPtr& ctx)
        : muse::Contextable(ctx) {}

    void init();

    bool isCommandAllowed(const muse::rcommand::Command& command) const override;
    muse::async::Notification changed() const override;
};
}
