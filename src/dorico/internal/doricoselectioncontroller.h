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
#include "interactive/iinteractive.h"

#include "filters.h"

namespace mu::engraving {
class EngravingItem;
}

namespace mu::dorico {
//! Selection keys of Dorico (spec 44): extend to the bar edge, Select More, Select None, Tab / Shift-Tab through the items at one
//! rhythmic position, and Select Same Pitch Onwards. Also Edit > Filter (spec 45), which reduces the selection.
class DoricoSelectionController : public muse::async::Asyncable, public muse::Contextable, public muse::rcommand::Commandable
{
    muse::ContextInject<muse::rcommand::ICommandDispatcher> dispatcher = { this };
    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<muse::IInteractive> interactive = { this };

public:
    DoricoSelectionController(const muse::modularity::ContextPtr& iocCtx)
        : muse::Contextable(iocCtx) {}

    void init();

private:
    void extendToBar(bool end);
    void selectMore();
    void cycleItems(bool forward);
    void selectSamePitchOnwards();
    void applyCategoryFilter(const FilterPredicate& matches);
    void filterNotesByPitch();

    //! What the last Select More press selected; the next press carries on from it only while the selection is still exactly that
    std::vector<const mu::engraving::EngravingItem*> m_lastItems;
    int m_lastStage = 0;
};
}
