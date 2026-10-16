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

#include "dynamicshandler.h"

#include <memory>
#include <set>
#include <vector>

#include "engraving/dom/chord.h"
#include "engraving/dom/chordrest.h"
#include "engraving/dom/dynamic.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/hairpin.h"
#include "engraving/dom/input.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/part.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/select.h"
#include "engraving/dom/staff.h"
#include "engraving/types/typesconv.h"

#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationundostack.h"

#include "../parsers/dynamicsparser.h"

using namespace mu::dorico;
using namespace mu::engraving;
using muse::String;

namespace {
using Kind = DynamicItem::Kind;

const Fraction QUARTER(1, 4);

// An open-ended gradual dynamic entered at the caret (p<): created when stopped (?, the next dynamics entry, end of
// note input). ponytail: the hairpin appears when stopped, it does not grow while notes are entered.
struct PendingGradual {
    bool active = false;
    const Score* score = nullptr;
    Fraction start;
    track_idx_t track = 0;
    bool local = false;
    DynamicItem item;
};

// Where an entry goes: one staff (track), from `start`; gradual dynamics share [start, end).
struct Target {
    Fraction start;
    Fraction end;
    track_idx_t track = 0;
};

DynamicType dynamicType(const std::string& name)
{
    return TConv::fromXml(muse::AsciiStringView(name.c_str()), DynamicType::OTHER);
}

Dynamic* createDynamic(Score* score, const DynamicItem& item, bool parenthesised)
{
    const DynamicType type = dynamicType(item.dynamic);
    String xml = Dynamic::dynamicText(type);
    if (item.separator) {
        xml += String::fromStdString(std::string(1, item.separator)) + Dynamic::dynamicText(dynamicType(item.second));
    }
    if (!item.modifier.empty()) {
        const String mod = String::fromStdString(item.modifier);
        xml = item.modifierBefore ? mod + u" " + xml : xml + u" " + mod;
    }
    if (parenthesised) {
        xml = u"(" + xml + u")";
    }

    Dynamic* d = Factory::createDynamic(score->dummy());
    d->setDynamicType(type);
    d->setXmlText(xml);
    return d;
}

// The first chord/rest position on the track's staff at or after tick; tick itself when there is none.
Fraction snapForward(const Score* score, const Fraction& tick, track_idx_t track)
{
    const staff_idx_t staff = track2staff(track);
    for (const Measure* m = score->tick2measure(tick); m; m = m->nextMeasure()) {
        for (const Segment* s = m->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
            if (s->tick() < tick) {
                continue;
            }
            for (track_idx_t t = staff * VOICES; t < (staff + 1) * VOICES; ++t) {
                if (s->element(t)) {
                    return s->tick();
                }
            }
        }
    }
    return tick;
}

// Adds a dynamic at tick, or changes the one already there on that staff (Dorico replaces it).
Dynamic* addDynamic(Score* score, Dynamic* d, const Fraction& tick, track_idx_t track, bool local)
{
    Segment* seg = score->tick2segment(tick, true, SegmentType::ChordRest);
    if (!seg) {
        delete d;
        return nullptr;
    }
    for (EngravingItem* a : seg->annotations()) {
        if (a->isDynamic() && a->staffIdx() == track2staff(track)) {
            Dynamic* old = toDynamic(a);
            old->undoChangeProperty(Pid::DYNAMIC_TYPE, d->dynamicType());
            old->undoChangeProperty(Pid::TEXT, d->xmlText());
            delete d;
            return old;
        }
    }
    d->setInitialTrackAndVoiceAssignment(track, local);
    d->setOwnershipParent(seg);
    score->undoAddElement(d);
    return d;
}

Dynamic* addNiente(Score* score, const Fraction& tick, track_idx_t track, bool local)
{
    DynamicItem n;
    n.dynamic = "n";
    return addDynamic(score, createDynamic(score, n, false), tick, track, local);
}

Hairpin* addGradual(Score* score, const DynamicItem& item, const Fraction& from, const Fraction& to, track_idx_t track,
                    bool local)
{
    if (to <= from) {
        return nullptr;
    }
    Hairpin* h = Factory::createHairpin(score->dummy());
    if (item.kind == Kind::Hairpin) {
        h->setHairpinType(item.cresc ? HairpinType::CRESC_HAIRPIN : HairpinType::DIM_HAIRPIN);
        h->setHairpinCircledTip(item.niente == DynamicItem::Niente::Circle);
    } else {
        const String text = String::fromStdString(item.text);
        h->setHairpinType(item.cresc ? HairpinType::CRESC_LINE : HairpinType::DIM_LINE);
        h->setBeginText(text);
        h->setContinueText(u"(" + text + u")");
        h->setLineVisible(item.line);
    }
    h->setInitialTrackAndVoiceAssignment(track, local);
    h->setTrack2(h->track());
    h->setTick(from);
    h->setTick2(to);
    score->undoAddElement(h);

    // n< / >n: a niente dynamic at the quiet end, which the hairpin snaps to.
    if (item.niente == DynamicItem::Niente::Letter) {
        addNiente(score, item.cresc ? from : to, track, local);
    }
    return h;
}

void finishPending(PendingGradual& p, Score* score, const Fraction& end)
{
    if (p.active && p.score == score) {
        addGradual(score, p.item, p.start, end, p.track, p.local);
    }
    p.active = false;
}

// The selected item a list selection starts at, as a target. A selected hairpin is removed: the entry replaces it.
bool listTarget(Score* score, Target& target)
{
    const EngravingItem* first = nullptr;
    for (const EngravingItem* e : score->selection().elements()) {
        if (!first || e->tick() < first->tick() || (e->tick() == first->tick() && e->staffIdx() < first->staffIdx())) {
            first = e;
        }
    }
    if (!first) {
        return false;
    }

    if (first->isHairpinSegment()) {
        Hairpin* h = toHairpinSegment(first)->hairpin();
        target = { h->tick(), h->tick2(), h->track() };
        score->undoRemoveElement(h);
        return true;
    }

    const EngravingItem* e = first->isNote() ? toNote(first)->chord() : first;
    const ChordRest* cr = e->isChordRest() ? toChordRest(e) : score->findCR(e->tick(), e->track());
    if (cr && cr->tick() != e->tick()) {
        cr = nullptr;
    }
    const Fraction end = cr ? (cr->isChord() ? toChord(cr)->endTickIncludingTied() : cr->endTick()) : e->tick() + QUARTER;
    target = { e->tick(), end, e->track() };
    return true;
}

std::vector<Target> targets(Score* score, bool caret, bool local, size_t graduals)
{
    if (caret) {
        const InputState& is = score->inputState();
        if (!is.segment() || is.beyondScore()) {
            return {};
        }
        // During note input each gradual dynamic of a phrase lasts a quarter note.
        return { { is.tick(), is.tick() + QUARTER * static_cast<int>(std::max<size_t>(graduals, 1)), is.track() } };
    }

    const Selection& sel = score->selection();
    if (!sel.isRange()) {
        Target t;
        return listTarget(score, t) ? std::vector<Target> { t } : std::vector<Target> {};
    }

    // Range: each staff, or one staff per instrument when the dynamic applies to all its staves.
    std::vector<Target> result;
    std::set<const Part*> parts;
    for (staff_idx_t staff = sel.staffStart(); staff < sel.staffEnd(); ++staff) {
        if (!local && !parts.insert(score->staff(staff)->part()).second) {
            continue;
        }
        result.push_back({ sel.tickStart(), sel.tickEnd(), staff * VOICES });
    }
    return result;
}

std::string prefill(const EngravingItem* item)
{
    if (item->isDynamic()) {
        const Dynamic* d = toDynamic(item);
        if (d->dynamicType() == DynamicType::OTHER || d->hasCustomText()) {
            return d->plainText().toStdString();
        }
        const muse::AsciiStringView name = TConv::toXml(d->dynamicType());
        return std::string(name.ascii(), name.size());
    }
    if (item->isHairpinSegment()) {
        const Hairpin* h = toHairpinSegment(item)->hairpin();
        switch (h->hairpinType()) {
        case HairpinType::CRESC_HAIRPIN: return h->hairpinCircledTip() ? "o<" : "<";
        case HairpinType::DIM_HAIRPIN: return h->hairpinCircledTip() ? ">o" : ">";
        case HairpinType::CRESC_LINE:
        case HairpinType::DIM_LINE: {
            std::string word = h->beginText().toStdString();
            if (!word.empty() && word.back() == '.') {
                word.pop_back();
            }
            return h->lineVisible() ? word + "..." : word;
        }
        default: break;
        }
    }
    return std::string();
}
}

PopoverDef mu::dorico::dynamicsPopover()
{
    auto pending = std::make_shared<PendingGradual>();

    PopoverDef def;
    def.id = "dynamics";
    def.title = "Dynamics";
    def.icon = muse::ui::IconCode::Code::DYNAMIC_FORTE;
    def.supportsLocal = true;
    def.itemTypes = { ElementType::DYNAMIC, ElementType::HAIRPIN_SEGMENT };
    def.validate = [](const std::string& text) { return parseDynamics(text).status; };
    def.prefill = prefill;

    def.apply = [pending](const std::string& text, ApplyScope scope, mu::notation::INotationPtr notation) {
        const DynamicsParse parsed = parseDynamics(text);
        if (parsed.status != ParseStatus::Ok) {
            return false;
        }
        const DynamicEntry& entry = parsed.entry;
        Score* score = notation->elements()->msScore();
        const bool caret = notation->interaction()->noteInput()->isNoteInputMode();
        const bool local = scope == ApplyScope::Local;

        mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
        undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Add dynamics"));

        // Any entry at the caret stops an open-ended gradual dynamic there.
        if (caret) {
            finishPending(*pending, score, score->inputState().tick());
        }

        size_t graduals = 0;
        for (const DynamicItem& item : entry.items) {
            graduals += item.kind != Kind::Immediate;
        }

        const std::vector<Target> where = entry.stop ? std::vector<Target> {} : targets(score, caret, local, graduals);
        if (!entry.stop && where.empty()) {
            undoStack->rollbackChanges();
            return false;
        }

        EngravingItem* created = nullptr;
        for (const Target& t : where) {
            // Positions of the phrase: ticks[k] starts gradual k; the last one ends the phrase.
            std::vector<Fraction> ticks = { t.start };
            for (size_t k = 1; k <= graduals; ++k) {
                const int ideal = t.start.ticks() + (t.end.ticks() - t.start.ticks()) * int(k) / int(graduals);
                ticks.push_back(snapForward(score, Fraction::fromTicks(ideal), t.track));
            }

            size_t k = 0;
            for (size_t i = 0; i < entry.items.size(); ++i) {
                const DynamicItem& item = entry.items[i];
                EngravingItem* added = nullptr;
                if (item.kind == Kind::Immediate) {
                    added = addDynamic(score, createDynamic(score, item, entry.parenthesised), ticks[k], t.track, local);
                } else if (caret && i + 1 == entry.items.size()) {
                    *pending = { true, score, ticks[k], t.track, local, item }; // p<: open-ended at the caret
                } else {
                    added = addGradual(score, item, ticks[k], ticks[k + 1], t.track, local);
                    ++k;
                }
                if (!created) {
                    created = added;
                }
            }
        }

        undoStack->commitChanges();

        // Outside note input the new item is selected, so Return reopens it for editing.
        if (!caret && created) {
            EngravingItem* sel = created->isHairpin() ? toHairpin(created)->frontSegment() : created;
            if (sel) {
                notation->interaction()->select({ sel });
            }
        }
        return true;
    };

    def.noteInputEnded = [pending](mu::notation::INotationPtr notation) {
        if (!pending->active) {
            return;
        }
        Score* score = notation->elements()->msScore();
        mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
        undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Add dynamics"));
        finishPending(*pending, score, score->inputState().tick());
        undoStack->commitChanges();
    };
    return def;
}
