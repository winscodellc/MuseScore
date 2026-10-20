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

#include <optional>
#include <set>

#include "global/types/bytearray.h"

#include "engraving/types/fraction.h"

#include "notation/inotation.h"

namespace mu::engraving {
class Score;
class Segment;
}

//! Dorico's Edit > Paste Special family (spec 41), plus the clipboard helpers shared with Paste (spec 40).
namespace mu::dorico {
//! The staff-list payload MuseScore puts on the system clipboard
struct PasteClip {
    mu::engraving::Fraction len;
    size_t staves = 0;
    muse::ByteArray data;
};

//! nullopt when `data` is not a staff-list payload
std::optional<PasteClip> pasteClipFrom(const muse::ByteArray& data);
std::optional<PasteClip> pasteClipFromClipboard();

mu::engraving::Segment* pasteSegmentAt(mu::engraving::Score* score, const mu::engraving::Fraction& tick);

//! The voices (0-3) that hold a chord in a staff-list payload. Pure: reads the XML only. A clip stores a voice change as
//! a `<location><voices>` step relative to the previous one, directly under `<Staff>`.
std::set<int> pasteClipVoices(const muse::ByteArray& data);

//! `dir` is -1 for the staff above, +1 for the staff below. Enabled when the selection has somewhere to go.
bool pasteSpecialCanMove(const mu::engraving::Score* score, int dir);
bool pasteSpecialCanSwap(const mu::engraving::Score* score);
//! Paste Into Voice and Paste Articulations need something selected to say where to paste
bool pasteSpecialHasTarget(const mu::engraving::Score* score);

//! Each returns true when it changed the score (one undo step). The system clipboard is only read, never written.
bool pasteSpecialDuplicate(const mu::notation::INotationPtr& notation, int dir);
bool pasteSpecialMove(const mu::notation::INotationPtr& notation, int dir);
//! `voice` 0-3, or -1 for the first voice that is empty where the clipboard lands
bool pasteSpecialIntoVoice(const mu::notation::INotationPtr& notation, int voice);
bool pasteSpecialSwap(const mu::notation::INotationPtr& notation);
bool pasteSpecialArticulations(const mu::notation::INotationPtr& notation);
}
