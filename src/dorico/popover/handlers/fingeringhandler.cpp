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

#include "fingeringhandler.h"

#include "engraving/dom/chord.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/fingering.h"
#include "engraving/dom/instrument.h"
#include "engraving/dom/note.h"
#include "engraving/dom/part.h"
#include "engraving/dom/score.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/stringdata.h"

#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationundostack.h"

#include "../parsers/fingeringparser.h"
#include "../popovernavigation.h"

using namespace mu::dorico;
using namespace mu::engraving;

// Named, not anonymous: the unity build compiles the handlers together.
namespace fingeringpopover {
std::vector<Fingering*> fingeringsOf(const Note* note)
{
    std::vector<Fingering*> result;
    for (EngravingItem* e : note->el()) {
        if (e->isFingering()) {
            result.push_back(toFingering(e));
        }
    }
    return result;
}

bool isFretted(const Chord* chord)
{
    const StringData* strings = chord->staff()->part()->instrument(chord->tick())->stringData();
    return strings && strings->frettedStrings() > 0;
}

// Entry text for the chord: one fingering per note, lowest first, up to the first note without one. A hand-styled digit
// keeps its letter (`L2`) so that reapplying it gives the same style.
std::string chordText(const Chord* chord)
{
    std::string text;
    for (const Note* note : chord->notes()) {
        const std::vector<Fingering*> fingerings = fingeringsOf(note);
        if (fingerings.empty()) {
            break;
        }
        const std::string s = fingerings.front()->plainText().toStdString();
        const TextStyleType style = fingerings.front()->textStyleType();
        const bool digit = !s.empty() && s[0] >= '0' && s[0] <= '9';
        text += (text.empty() ? "" : ",");
        if (digit && style == TextStyleType::LH_GUITAR_FINGERING) {
            text += "L";
        } else if (digit && style == TextStyleType::RH_GUITAR_FINGERING) {
            text += "R";
        }
        text += s;
    }
    return text;
}
}

PopoverDef mu::dorico::fingeringPopover()
{
    PopoverDef def;
    def.id = "fingering";
    def.title = "Fingering";
    def.icon = muse::ui::IconCode::Code::FRETBOARD_DIAGRAM;
    def.itemTypes = { ElementType::FINGERING };
    def.stepsByChord = true;
    def.validate = [](const std::string& text) { return parseFingering(text, false).status; };
    def.prefill = [](const EngravingItem* item) {
        const Chord* chord = item->isFingering() ? toFingering(item)->note()->chord() : (item->isChord() ? toChord(item) : nullptr);
        return chord ? fingeringpopover::chordText(chord) : std::string();
    };

    def.applyAt = [](const std::string& text, const PopoverPosition& pos, mu::notation::INotationPtr notation) {
        Score* score = notation->elements()->msScore();
        Chord* chord = chordAt(score, pos);
        if (!chord) {
            return false;
        }
        const FingeringParse parsed = parseFingering(text, fingeringpopover::isFretted(chord));
        if (parsed.status != ParseStatus::Ok || parsed.items.size() > chord->notes().size()) {
            return false;
        }

        mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
        undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Add fingering"));
        Fingering* top = nullptr;
        for (size_t i = 0; i < parsed.items.size(); ++i) {
            Note* note = chord->notes()[i];
            for (Fingering* old : fingeringpopover::fingeringsOf(note)) {
                score->undoRemoveElement(old);
            }
            // Like Note::drop: a TAB staff only shows fingering if its staff type says so.
            if (note->staff()->isTabStaff(note->tick()) && !note->staff()->staffTypeForElement(note)->showTabFingering()) {
                continue;
            }
            Fingering* f = Factory::createFingering(note, parsed.items[i].style);
            f->setTrack(note->track());
            f->setPlainText(muse::String::fromStdString(parsed.items[i].text));
            score->undoAddElement(f);
            top = f;
        }
        undoStack->commitChanges();
        if (top) {
            notation->interaction()->select({ top }); // Return on it reopens the popover
        }
        return true;
    };
    return def;
}
