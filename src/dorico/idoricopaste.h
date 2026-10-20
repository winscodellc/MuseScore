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

#include "global/types/bytearray.h"

#include "modularity/imoduleinterface.h"

#include "engraving/types/types.h"

namespace mu::engraving {
class Score;
class Segment;
class Transaction;
}

namespace mu::dorico {
//! Dorico's paste rules for the callers that paste on their own (spec 42): Opt-click and Opt-drag copy a range through
//! MuseScore's drop code. Optional: only the Dorico module registers it, the caller falls back to MuseScore's paste.
class IDoricoPaste : MODULE_CONTEXT_INTERFACE
{
    INTERFACE_ID(IDoricoPaste)

public:
    virtual ~IDoricoPaste() = default;

    //! Pastes a staff-list payload with its first chord at `dst` on `staff`, inside the caller's transaction, following
    //! the Chord and Insert modes. false = no room.
    virtual bool pasteStaff(mu::engraving::Transaction& tx, mu::engraving::Score* score, const muse::ByteArray& data,
                            mu::engraving::Segment* dst, mu::engraving::staff_idx_t staff) = 0;
};
}
