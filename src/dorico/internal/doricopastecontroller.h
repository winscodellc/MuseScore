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

#include <vector>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "rcommand/commandable.h"
#include "rcommand/icommanddispatcher.h"
#include "context/iglobalcontext.h"

#include "../idoricomodestate.h"
#include "../idoricopaste.h"
#include "pasteplanner.h"

namespace mu::dorico {
//! Cmd-V / Cmd-X with Dorico's destination rules (spec 40), and R on a range (spec 42). Hooks `notation/paste` and `notation/cut` through preDispatch,
//! so the menus behave the same as the keys; whatever MuseScore already does the same way is left to its own handler.
class DoricoPasteController : public IDoricoPaste, public muse::async::Asyncable, public muse::Contextable,
    public muse::rcommand::Commandable
{
    muse::ContextInject<muse::rcommand::ICommandDispatcher> dispatcher = { this };
    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<IDoricoModeState> modes = { this };

public:
    DoricoPasteController(const muse::modularity::ContextPtr& iocCtx)
        : muse::Contextable(iocCtx) {}

    void init();

    //! IDoricoPaste (Opt-click and Opt-drag, spec 42)
    bool pasteStaff(mu::engraving::Transaction& tx, mu::engraving::Score* score, const muse::ByteArray& data,
                    mu::engraving::Segment* dst, mu::engraving::staff_idx_t staff) override;

private:
    //! Both return true when they did the job and MuseScore's own handler must not run.
    bool tryPaste();
    bool tryCut();
    //! R on a range: the copy goes after the selection with the same rules as Paste
    bool tryRepeat();
};
}
