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
#include <vector>

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

//! Explode / Reduce allocation (spec 47). Pure.
//! Sizes of the groups when `nSources` staves are shared among `nDest` staves, top-down, larger groups first: 5 onto 3 is {2,2,1}.
std::vector<size_t> reduceGroups(size_t nSources, size_t nDest);
//! For each of `nStaves` destination staves (top first), the notes it keeps, as indices counted from the top note of a chord of
//! `nNotes`. More notes than staves: contiguous runs, the extras go to the upper staves (4 onto 3 is {0,1},{2},{3}). Fewer
//! notes than staves: a note is shared by several staves (2 onto 3 is {0},{1},{1}).
std::vector<std::vector<size_t> > explodeNotes(size_t nNotes, size_t nStaves);
//! A one-staff clip made of the `index`-th staff of a staff-list payload (its notes and the spanners that start on it);
//! empty when there is no such staff.
muse::ByteArray staffClipOf(const muse::ByteArray& data, size_t index);

//! `dir` is -1 for the staff above, +1 for the staff below. Enabled when the selection has somewhere to go.
bool pasteSpecialCanMove(const mu::engraving::Score* score, int dir);
bool pasteSpecialCanSwap(const mu::engraving::Score* score);
//! Paste Into Voice and Paste Articulations need something selected to say where to paste
bool pasteSpecialHasTarget(const mu::engraving::Score* score);
//! Explode needs a one-staff clip and a range over two or more staves; Reduce a clip with more staves than the range has
bool pasteSpecialCanExplode(const mu::engraving::Score* score);
bool pasteSpecialCanReduce(const mu::engraving::Score* score);

//! Each returns true when it changed the score (one undo step). The system clipboard is only read, never written.
bool pasteSpecialDuplicate(const mu::notation::INotationPtr& notation, int dir);
bool pasteSpecialMove(const mu::notation::INotationPtr& notation, int dir);
//! `voice` 0-3, or -1 for the first voice that is empty where the clipboard lands
bool pasteSpecialIntoVoice(const mu::notation::INotationPtr& notation, int voice);
bool pasteSpecialSwap(const mu::notation::INotationPtr& notation);
bool pasteSpecialArticulations(const mu::notation::INotationPtr& notation);
//! Spec 47. The copied music goes to every selected staff and each keeps its share of every chord / the copied staves are
//! merged onto the selected staves, top-down.
bool pasteSpecialExplode(const mu::notation::INotationPtr& notation);
bool pasteSpecialReduce(const mu::notation::INotationPtr& notation);
}
