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

#include "keysighandler.h"

#include <set>
#include <vector>

#include "engraving/dom/input.h"
#include "engraving/dom/keysig.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/select.h"
#include "engraving/dom/staff.h"
#include "engraving/editing/editkeysig.h"
#include "engraving/editing/transaction/transaction.h"

#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationundostack.h"

#include "../parsers/keysigparser.h"

using namespace mu::dorico;
using namespace mu::engraving;

namespace {
KeySigEvent keySigEvent(const KeySigEntry& entry)
{
    KeySigEvent event;
    if (entry.kind == KeySigEntry::Kind::Open) { // as MuseScore's "Open/Atonal" palette cell
        event.setConcertKey(Key::C);
        event.setCustom(true);
        event.setMode(KeyMode::NONE);
    } else {
        event.setConcertKey(Key(entry.fifths)); // concert key; undoChangeKeySig writes transposing staves' keys
        event.setMode(entry.minor ? KeyMode::MINOR : KeyMode::MAJOR);
    }
    return event;
}

// Where the key goes: the caret (even mid-bar, as Dorico) or the earliest selected item; and the staves Opt-Return uses.
bool keySigTarget(Score* score, bool caret, Fraction& tick, std::set<staff_idx_t>& staves)
{
    if (caret) {
        const InputState& is = score->inputState();
        if (!is.segment() || is.beyondScore()) {
            return false;
        }
        tick = is.tick();
        staves.insert(track2staff(is.track()));
        return true;
    }

    const Selection& sel = score->selection();
    if (sel.isRange()) {
        if (!sel.startSegment()) {
            return false;
        }
        tick = sel.startSegment()->tick();
        for (staff_idx_t s = sel.staffStart(); s < sel.staffEnd(); ++s) {
            staves.insert(s);
        }
        return true;
    }

    const EngravingItem* first = nullptr;
    for (const EngravingItem* e : sel.elements()) {
        staves.insert(e->staffIdx());
        if (!first || e->tick() < first->tick()) {
            first = e;
        }
    }
    if (!first || first->tick() >= score->endTick()) { // the final barline has nothing after it
        return false;
    }
    tick = first->tick();
    return true;
}

std::string keySigPrefill(const EngravingItem* item)
{
    const KeySigEvent& event = toKeySig(item)->keySigEvent();
    if (event.isAtonal()) {
        return keySigText({ KeySigEntry::Kind::Open, 0, false });
    }
    if (event.custom()) {
        return std::string(); // custom keys are panel-only
    }
    return keySigText({ KeySigEntry::Kind::Tonal, int(event.concertKey()), event.mode() == KeyMode::MINOR });
}
}

PopoverDef mu::dorico::keySigPopover()
{
    PopoverDef def;
    def.id = "keysig";
    def.title = "Key Signatures";
    def.glyph = "\uE262\uE262";
    def.scopeButton = true;
    def.fieldWidth = 171;
    def.supportsLocal = true;
    def.itemTypes = { ElementType::KEYSIG };
    def.validate = [](const std::string& text) { return parseKeySig(text).status; };
    def.prefill = keySigPrefill;

    def.apply = [](const std::string& text, ApplyScope scope, mu::notation::INotationPtr notation) {
        const KeySigParse parsed = parseKeySig(text);
        if (parsed.status != ParseStatus::Ok) {
            return false;
        }

        Score* score = notation->elements()->msScore();
        const bool caret = notation->interaction()->noteInput()->isNoteInputMode();
        Fraction tick;
        std::set<staff_idx_t> selected;
        if (!keySigTarget(score, caret, tick, selected) || selected.empty()) {
            return false;
        }

        // Return: every staff of the whole score, also from a part. Each staff's linked copies follow it.
        std::vector<Staff*> staves;
        if (scope == ApplyScope::Local) {
            for (staff_idx_t s : selected) {
                staves.push_back(score->staff(s));
            }
        } else {
            staves = score->masterScore()->staves();
        }
        std::erase_if(staves, [&](const Staff* s) { return !s || s->isDrumStaff(tick); });
        if (staves.empty()) {
            return false;
        }

        mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
        undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Add key signature"));
        Transaction& tx = score->transactionManager()->currentOrDummyTransaction();
        // Replaces a key signature already at that tick.
        const KeySigEvent event = keySigEvent(parsed.entry);
        for (Staff* staff : staves) {
            EditKeySig::undoChangeKeySig(tx, staff->score(), staff, tick, event);
        }
        undoStack->commitChanges();

        // Outside note input the new key signature is selected, so Return reopens it for editing.
        if (!caret) {
            const Measure* m = score->tick2measure(tick);
            const Segment* seg = m ? m->findSegment(SegmentType::KeySig, tick) : nullptr;
            if (EngravingItem* ks = seg ? seg->element(staff2track(*selected.begin())) : nullptr) {
                notation->interaction()->select({ ks });
            }
        }
        return true;
    };
    return def;
}
