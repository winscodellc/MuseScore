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

#include "timesighandler.h"

#include <set>
#include <string>

#include "engraving/dom/chordrest.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/input.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/select.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/timesig.h"
#include "engraving/editing/edittimesig.h"
#include "engraving/editing/transaction/transaction.h"

#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationundostack.h"

#include "../parsers/timesigparser.h"

using namespace mu::dorico;
using namespace mu::engraving;
using muse::String;

namespace {
// Beam groups are stored in 32nd notes: [2,3,2]/8 breaks beams after the 2nd and 5th eighth.
Groups beamGroups(const TimeSigEntry& e)
{
    Groups groups;
    if (e.groups.size() < 2 || 32 % e.denominator != 0) {
        return groups;
    }
    int units = 0;
    for (size_t i = 0; i + 1 < e.groups.size(); ++i) {
        units += e.groups[i];
        groups.addNode({ units * 32 / e.denominator, 0x111 }); // new beam at every level
    }
    return groups;
}

// "2+3+2" from beam groups, or empty when they are not whole denominator units.
std::string groupsText(const TimeSig* ts)
{
    std::string text;
    int last = 0;
    for (const GroupNode& n : ts->groups().nodes()) {
        if ((n.action & 0xf) != 1) {
            continue;
        }
        if (n.pos * ts->denominator() % 32 != 0) {
            return std::string();
        }
        const int units = n.pos * ts->denominator() / 32;
        text += std::to_string(units - last) + "+";
        last = units;
    }
    if (text.empty() || last >= ts->numerator()) {
        return std::string();
    }
    return text + std::to_string(ts->numerator() - last);
}

std::string decimal(double v)
{
    std::string s = std::to_string(v);
    s.erase(s.find_last_not_of('0') + 1);
    if (s.back() == '.') {
        s.pop_back();
    }
    return s;
}

TimeSig* createTimeSig(Score* score, const TimeSigEntry& e)
{
    TimeSig* ts = Factory::createTimeSig(score->dummy());
    switch (e.kind) {
    case TimeSigEntry::Kind::Common: ts->setSig(e.sig(), TimeSigType::FOUR_FOUR);
        break;
    case TimeSigEntry::Kind::Cut: ts->setSig(e.sig(), TimeSigType::ALLA_BREVE);
        break;
    case TimeSigEntry::Kind::Normal:
        ts->setSig(e.sig());
        if (e.groups.size() > 1 && e.groupingShown) {
            std::string numerator;
            for (int g : e.groups) {
                numerator += (numerator.empty() ? "" : "+") + std::to_string(g);
            }
            ts->setNumeratorString(String::fromStdString(numerator));
            ts->setDenominatorString(String::number(e.denominator)); // a custom numerator hides the default denominator
        }
        break;
    }
    ts->setGroups(beamGroups(e));
    return ts;
}

// Shortening a bar to a pick-up deletes what lies past it, where Dorico re-bars that music instead.
// ponytail: we refuse when the cut-off part holds notes or attached items; re-barring is deferred.
bool hasMusicFrom(const Measure* m, const Fraction& tick)
{
    for (const Segment* s = m->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
        if (s->tick() >= tick && !s->annotations().empty()) {
            return true;
        }
        for (const EngravingItem* e : s->elist()) {
            if (e && e->isChord() && toChordRest(e)->endTick() > tick) {
                return true;
            }
        }
    }
    return false;
}

// The bar the entry goes in (start of the caret's bar or the earliest selected item's bar) and the staves Opt-Return uses.
Measure* target(Score* score, bool caret, std::set<staff_idx_t>& staves)
{
    if (caret) {
        const InputState& is = score->inputState();
        if (!is.segment() || is.beyondScore()) {
            return nullptr;
        }
        staves.insert(track2staff(is.track()));
        return is.segment()->measure();
    }

    const Selection& sel = score->selection();
    if (sel.isRange()) {
        for (staff_idx_t s = sel.staffStart(); s < sel.staffEnd(); ++s) {
            staves.insert(s);
        }
        return sel.startSegment() ? sel.startSegment()->measure() : nullptr;
    }

    const EngravingItem* first = nullptr;
    for (const EngravingItem* e : sel.elements()) {
        staves.insert(e->staffIdx());
        if (!first || e->tick() < first->tick()) {
            first = e;
        }
    }
    // A time signature at the end of a bar (before a system break) belongs to the next bar; tick2measure finds that one.
    return first ? score->tick2measure(first->tick()) : nullptr;
}

std::string timeSigPrefill(const EngravingItem* item)
{
    const TimeSig* ts = toTimeSig(item);
    const std::string den = std::to_string(ts->denominator());
    std::string text;
    switch (ts->timeSigType()) {
    case TimeSigType::FOUR_FOUR: text = "c";
        break;
    case TimeSigType::ALLA_BREVE: text = "cut";
        break;
    default: {
        const std::string hidden = groupsText(ts);
        if (!ts->numeratorString().isEmpty()) {
            text = ts->numeratorString().toStdString() + "/" + den;
        } else if (!hidden.empty()) {
            text = "[" + hidden + "]/" + den;
        } else {
            text = std::to_string(ts->numerator()) + "/" + den;
        }
    }
    }

    const Measure* m = ts->score()->tick2measure(ts->tick());
    if (m && m->excludeFromNumbering() && m->ticks() < m->timesig()) {
        text += "," + decimal((m->ticks() * Fraction(ts->denominator(), 1)).toDouble());
    }
    return text;
}
}

PopoverDef mu::dorico::timeSigPopover()
{
    PopoverDef def;
    def.id = "timesig";
    def.title = "Time Signatures";
    def.glyph = "3\n4"; // bold UI digits: Bravura Text has no plain timeSig digits
    def.scopeButton = true;
    def.fieldWidth = 171;
    def.supportsLocal = true;
    def.itemTypes = { ElementType::TIMESIG };
    def.validate = [](const std::string& text) { return parseTimeSig(text).status; };
    def.prefill = timeSigPrefill;

    def.apply = [](const std::string& text, ApplyScope scope, mu::notation::INotationPtr notation) {
        const TimeSigParse parsed = parseTimeSig(text);
        if (parsed.status != ParseStatus::Ok) {
            return false;
        }
        const TimeSigEntry& entry = parsed.entry;
        const bool local = scope == ApplyScope::Local;
        if (local && !entry.pickup.isZero()) {
            return false; // MuseScore bars span every staff, so a pick-up cannot be local
        }

        Score* score = notation->elements()->msScore();
        const bool caret = notation->interaction()->noteInput()->isNoteInputMode();
        std::set<staff_idx_t> staves;
        Measure* measure = target(score, caret, staves);
        if (!measure || staves.empty()) {
            return false;
        }
        const Fraction tick = measure->tick();
        const Fraction caretTick = caret ? score->inputState().tick() : tick;

        mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
        undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Add time signature"));
        Transaction& tx = score->transactionManager()->currentOrDummyTransaction();

        // EditTimeSig replaces a time signature already at that bar and re-bars the music up to the next one.
        TimeSig* ts = createTimeSig(score, entry);
        if (local) {
            for (staff_idx_t staff : staves) {
                EditTimeSig::addTimeSig(tx, score, measure, staff, Factory::copyTimeSig(*ts), true);
            }
            delete ts;
        } else {
            EditTimeSig::addTimeSig(tx, score, measure, 0, ts, false);
        }

        // addTimeSig unwinds without telling us when it cannot re-bar (e.g. a local change over notes).
        const TimeSig* added = score->staff(*staves.begin())->timeSig(tick);
        if (!added || added->tick() != tick || !added->sig().identical(entry.sig())) {
            undoStack->rollbackChanges();
            return false;
        }

        if (!entry.pickup.isZero()) {
            Measure* m = score->masterScore()->tick2measure(tick);
            if (hasMusicFrom(m, tick + entry.pickup)) {
                undoStack->rollbackChanges();
                return false;
            }
            m->undoChangeProperty(Pid::EXCLUDE_FROM_NUMBERING, true);
            m->adjustToLen(entry.pickup);
        }

        if (caret) {
            // The caret's segment may have been rewritten or cut off with the pick-up.
            Segment* seg = score->tick2segment(caretTick, true, SegmentType::ChordRest);
            score->inputState().setSegment(seg ? seg : score->tick2measure(tick)->first(SegmentType::ChordRest));
        }

        undoStack->commitChanges();

        // Outside note input the new time signature is selected, so Return reopens it for editing.
        if (!caret) {
            if (TimeSig* sel = score->staff(*staves.begin())->timeSig(tick)) {
                notation->interaction()->select({ sel });
            }
        }
        return true;
    };
    return def;
}
