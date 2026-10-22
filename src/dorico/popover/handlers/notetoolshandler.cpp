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

#include "notetoolshandler.h"

#include <algorithm>

#include "engraving/dom/chord.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/interval.h"
#include "engraving/dom/note.h"
#include "engraving/dom/score.h"
#include "engraving/dom/select.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/tie.h"

#include "notation/inotationelements.h"
#include "notation/inotationundostack.h"

#include "../../internal/transformapply.h"
#include "../parsers/notetoolsparser.h"

using namespace mu::dorico;
using namespace mu::engraving;

// Named, not anonymous: the unity build compiles the handlers together.
namespace notetoolspopover {
// The notes the entry works on, one per tie chain: the selected notes, or with top / bottom only each chord's highest /
// lowest. A chain is represented by its first note.
std::vector<Note*> targetChains(Score* score, NoteToolsScope scope)
{
    std::vector<Note*> chains;
    for (Note* note : score->selection().noteList()) {
        const Chord* chord = note->chord();
        if (note->staff()->isDrumStaff(note->tick())
            || (scope == NoteToolsScope::Top && note != chord->upNote())
            || (scope == NoteToolsScope::Bottom && note != chord->downNote())) {
            continue;
        }
        Note* head = note->firstTiedNote();
        if (std::find(chains.begin(), chains.end(), head) == chains.end()) {
            chains.push_back(head);
        }
    }
    return chains;
}

// Moves the note by the interval; false if that leaves the pitch range.
bool moveNote(Note* note, const IntervalSpec& spec)
{
    const IntervalSteps steps = toSteps(spec);
    if (spec.followsKey()) {
        // A third in the key, but an octave keeps its accidental, as in Score::addInterval.
        return note->transposeDiatonic(steps.diatonic, steps.diatonic % 7 == 0, false);
    }
    return note->transpose(Interval(steps.diatonic, steps.semitones), false);
}

bool transposeChain(Note* head, const IntervalSpec& spec)
{
    for (Note* note : head->tiedNotes()) {
        if (!moveNote(note, spec)) {
            return false;
        }
    }
    return true;
}

// Adds a note `spec` away from each note of the tie chain, tied the same way. A pitch the chord already has adds nothing.
bool addToChain(Score* score, Note* head, const IntervalSpec& spec)
{
    std::vector<Note*> added;
    for (Note* source : head->tiedNotes()) {
        Chord* chord = source->chord();
        Note* note = Factory::createNote(chord);
        note->setTrack(chord->track());
        note->setNval(source->noteVal());
        score->undoAddElement(note);
        added.push_back(note);
        if (!moveNote(note, spec)) {
            return false;
        }
    }

    const Note* first = added.front();
    const std::vector<Note*>& chordNotes = first->chord()->notes();
    const bool duplicate = std::any_of(chordNotes.begin(), chordNotes.end(), [&](const Note* n) {
        return n != first && n->pitch() == first->pitch();
    });
    if (duplicate) {
        for (Note* note : added) {
            score->undoRemoveElement(note);
        }
        return true;
    }

    for (size_t i = 0; i + 1 < added.size(); ++i) {
        Note* from = added[i];
        Note* to = added[i + 1];
        Tie* tie = Factory::createTie(from);
        tie->setStartNote(from);
        tie->setTick(from->tick());
        tie->setTrack(from->track());
        tie->setEndNote(to);
        tie->setTick2(to->tick());
        from->setTieFor(tie);
        to->setTieBack(tie);
        score->undoAddElement(tie);
    }
    return true;
}
}

PopoverDef mu::dorico::noteToolsPopover()
{
    PopoverDef def;
    def.id = "notetools";
    def.title = "Note Tools";
    def.glyph = "\uE1F0\uE1F7\uE1F0";
    def.validate = [](const std::string& text) { return parseNoteTools(text).status; };
    def.prefill = [](const EngravingItem*) { return std::string(); }; // nothing to reopen
    def.apply = [](const std::string& text, ApplyScope, mu::notation::INotationPtr notation) {
        const NoteToolsParse parsed = parseNoteTools(text);
        if (parsed.status != ParseStatus::Ok) {
            return false;
        }
        if (parsed.transform) {
            return applyTransform(notation, *parsed.transform);
        }
        Score* score = notation->elements()->msScore();
        const std::vector<Note*> chains = notetoolspopover::targetChains(score, parsed.scope);
        if (chains.empty()) {
            return false;
        }

        mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
        undoStack->prepareChanges(parsed.transpose ? muse::TranslatableString("undoableAction", "Transpose")
                                  : muse::TranslatableString("undoableAction", "Add interval"));
        bool ok = true;
        for (Note* head : chains) {
            if (parsed.transpose) {
                ok = notetoolspopover::transposeChain(head, parsed.intervals.front());
            } else {
                // Every interval is measured from the selected note: `3,6` is a third and a sixth, not a sixth above the third.
                for (const IntervalSpec& spec : parsed.intervals) {
                    ok = ok && notetoolspopover::addToChain(score, head, spec);
                }
            }
            if (!ok) {
                break;
            }
        }
        if (!ok) { // out of pitch range: leave the score as it was
            undoStack->rollbackChanges();
            return false;
        }
        undoStack->commitChanges();
        return true;
    };
    return def;
}
