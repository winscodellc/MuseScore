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

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "context/iglobalcontext.h"
#include "notation/inotationconfiguration.h"
#include "engraving/types/fraction.h"

#include "../idoricocaret.h"

namespace mu::engraving {
class Segment;
}

namespace mu::dorico {
class DoricoCaret : public IDoricoCaret, public muse::Contextable, public muse::async::Asyncable
{
    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::GlobalInject<notation::INotationConfiguration> notationConfiguration;

public:
    DoricoCaret(const muse::modularity::ContextPtr& iocCtx)
        : muse::Contextable(iocCtx) {}

    void init();

    bool step(bool right) override;
    bool materialize() override;
    void finerGrid() override;
    void coarserGrid() override;
    bool paint(muse::draw::Painter* painter) override;

private:
    //! The caret's tick: m_tick while it is inside the chord/rest at MuseScore's input position, else that position.
    engraving::Fraction caretTick() const;
    bool insideChordRest() const;
    void setGrid(int denominator);

    int m_gridDenominator = 8; // 8 = eighth-note grid
    // Where step() left the caret. Stale as soon as MuseScore's input position moves (note entered, click, ...).
    const engraving::Segment* m_anchor = nullptr;
    engraving::Fraction m_tick;
};
}
