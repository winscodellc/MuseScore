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

#include <functional>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "rcommand/commandable.h"
#include "rcommand/icommanddispatcher.h"
#include "context/iglobalcontext.h"

#include "../idoricocaret.h"

namespace mu::dorico {
//! Duration edits and rhythmic moves on the selection (spec 13): lengthen / shorten by the grid, double / halve,
//! move left / right, scissors.
class DoricoDurationsController : public muse::async::Asyncable, public muse::Contextable, public muse::rcommand::Commandable
{
    muse::ContextInject<muse::rcommand::ICommandDispatcher> dispatcher = { this };
    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<IDoricoCaret> caret = { this };

public:
    DoricoDurationsController(const muse::modularity::ContextPtr& iocCtx)
        : muse::Contextable(iocCtx) {}

    void init();

private:
    void reg(const muse::rcommand::Command& command, std::function<void()> handler);

    //! byGrid: Shift-Opt-Left/Right; otherwise Cmd-Shift-Opt (double / halve).
    void onResize(bool longer, bool byGrid);
    void onMove(bool right);
    void onScissors();
};
}
