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
#include <vector>

#include "async/asyncable.h"
#include "types/translatablestring.h"

#include "engraving/types/fraction.h"
#include "engraving/types/types.h"
#include "notation/inotation.h"

namespace mu::engraving {
class ChordRest;
class Score;
}

namespace mu::dorico {
//! Items entered at the note-input caret that belong on a note (ornaments, bowing marks): Dorico adds them once a note
//! is input there or Space advances the caret. Shared by the ornaments (spec 28) and playing techniques (spec 29)
//! popovers. ponytail: one caret staff (the multi-staff caret is deferred), and the check runs on the next event loop
//! turn, after the note entry's own undo step.
class PendingAtCaret : public muse::async::Asyncable
{
public:
    //! Adds one item to the chord/rest input at the caret; `caret` is where the caret went (a trill spans to it).
    using AddItem = std::function<bool(engraving::Score*, engraving::ChordRest*, const engraving::Fraction& caret)>;

    explicit PendingAtCaret(const muse::TranslatableString& undoName);

    bool add(AddItem item, notation::INotationPtr notation); // false when the caret is nowhere
    void cancel();

private:
    void check();

    muse::TranslatableString m_undoName;
    notation::INotationPtr m_notation;
    std::vector<AddItem> m_items;
    engraving::Fraction m_tick;
    engraving::track_idx_t m_track = 0;
};
}
