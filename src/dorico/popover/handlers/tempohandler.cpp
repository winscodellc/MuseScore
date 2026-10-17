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

#include "tempohandler.h"

#include <cmath>
#include <sstream>

#include "engraving/dom/chord.h"
#include "engraving/dom/chordrest.h"
#include "engraving/dom/durationtype.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/gradualtempochange.h"
#include "engraving/dom/input.h"
#include "engraving/dom/note.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/select.h"
#include "engraving/dom/tempotext.h"

#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationundostack.h"

#include "../parsers/tempoparser.h"

using namespace mu::dorico;
using namespace mu::engraving;
using muse::String;

// Named, not anonymous: the unity build compiles the handlers together, and they share names (Target, prefill).
namespace tempopopover {
using Kind = TempoEntry::Kind;

const Fraction QUARTER(1, 4);

// Where an entry goes. Tempo is global: it sits on the top staff, MuseScore copies it to the other system staves.
struct Target {
    Fraction start;
    Fraction end;
};

// SMuFL metronome note glyphs, as the Tempo palette writes them.
const std::pair<DurationType, const char*> NOTE_SYMS[] = {
    { DurationType::V_BREVE, "metNoteDoubleWhole" }, { DurationType::V_WHOLE, "metNoteWhole" },
    { DurationType::V_HALF, "metNoteHalfUp" }, { DurationType::V_QUARTER, "metNoteQuarterUp" },
    { DurationType::V_EIGHTH, "metNote8thUp" }, { DurationType::V_16TH, "metNote16thUp" },
    { DurationType::V_32ND, "metNote32ndUp" }, { DurationType::V_64TH, "metNote64thUp" },
};

std::string number(double v)
{
    std::ostringstream s;
    s << std::round(v * 100) / 100;
    return s.str();
}

double quarters(const BeatUnit& unit)
{
    TDuration d(unit.type);
    d.setDots(unit.dots);
    return d.fraction().toDouble() * 4;
}

// "<sym>metNoteQuarterUp</sym> = 120", with <sym>space</sym> and a dot glyph per dot, like the palette.
String metronomeXml(const TempoEntry& e)
{
    String xml;
    for (const auto& [type, sym] : NOTE_SYMS) {
        if (type == e.unit->type) {
            xml = u"<sym>" + String::fromAscii(sym) + u"</sym>";
        }
    }
    if (e.unit->dots) {
        xml += u"<sym>space</sym>";
    }
    for (int i = 0; i < e.unit->dots; ++i) {
        xml += u"<sym>metAugmentationDot</sym>";
    }
    xml += u" = " + String::fromStdString(number(e.bpm));
    return e.parenthesised ? u"(" + xml + u")" : xml;
}

TempoText* createTempoText(Score* score, const TempoEntry& e)
{
    // A word typed in the popover is plain text: escape <, & so it is not read as markup.
    String xml = String::fromStdString(e.text).toXmlEscaped();
    if (e.unit && !e.markHidden) {
        xml = xml.isEmpty() ? metronomeXml(e) : xml + u" " + metronomeXml(e);
    }

    TempoText* tt = Factory::createTempoText(score->dummy());
    tt->setXmlText(xml);
    if (e.unit) {
        tt->setTempo(e.bpm * quarters(*e.unit) / 60.0);
        tt->setFollowText(!e.markHidden); // editing the printed mark changes the tempo, as for palette marks
    } else if (e.wordBpm > 0) {
        tt->setTempo(e.wordBpm / 60.0);
    } else if (e.kind == Kind::ATempo) {
        tt->setATempo();
        tt->setFollowText(true);
    } else if (e.kind == Kind::TempoPrimo) {
        tt->setTempoPrimo();
        tt->setFollowText(true);
    } else {
        tt->setPlayTempoText(false); // Mosso, Rubato, free text: printed only
    }
    tt->setVisible(!e.hidden);
    return tt;
}

// Adds the tempo mark at tick, replacing any there (one tempo per position).
TempoText* addTempoText(Score* score, TempoText* tt, const Fraction& tick)
{
    Segment* seg = score->tick2segment(tick, true, SegmentType::ChordRest);
    if (!seg) {
        delete tt;
        return nullptr;
    }
    // Removing one removes its copies on the other system staves too, so search again each time.
    for (bool found = true; found;) {
        found = false;
        for (EngravingItem* a : seg->annotations()) {
            if (a->isTempoText()) {
                score->undoRemoveElement(a);
                found = true;
                break;
            }
        }
    }
    tt->setTrack(0);
    tt->setOwnershipParent(seg);
    score->undoAddElement(tt);
    return tt;
}

GradualTempoChange* addGradual(Score* score, const TempoEntry& e, const Target& t)
{
    if (t.end <= t.start) {
        return nullptr;
    }
    GradualTempoChange* g = Factory::createGradualTempoChange(score->dummy());
    const String text = String::fromStdString(e.text);
    g->setTempoChangeType(e.gradual);
    g->setBeginText(text);
    g->setContinueText(u"(" + text + u")");
    g->setLineVisible(e.line);
    g->setVisible(!e.hidden);
    g->setTrack(0);
    g->setTrack2(0);
    g->setTick(t.start);
    g->setTick2(t.end);
    score->undoAddElement(g);
    return g;
}

// The caret (gradual changes last a quarter note there), the range, or the earliest selected item (to its end).
// A selected gradual change is removed and its span reused: the entry replaces it.
bool target(Score* score, bool caret, Target& t)
{
    if (caret) {
        const InputState& is = score->inputState();
        if (!is.segment() || is.beyondScore()) {
            return false;
        }
        t = { is.tick(), is.tick() + QUARTER };
        return true;
    }

    const Selection& sel = score->selection();
    if (sel.isRange()) {
        t = { sel.tickStart(), sel.tickEnd() };
        return true;
    }

    const EngravingItem* first = nullptr;
    for (const EngravingItem* e : sel.elements()) {
        if (!first || e->tick() < first->tick()) {
            first = e;
        }
    }
    if (!first) {
        return false;
    }
    if (first->isGradualTempoChangeSegment()) {
        Spanner* g = toSpannerSegment(first)->spanner();
        t = { g->tick(), g->tick2() };
        score->undoRemoveElement(g);
        return true;
    }

    const EngravingItem* e = first->isNote() ? toNote(first)->chord() : first;
    const ChordRest* cr = e->isChordRest() ? toChordRest(e) : score->findCR(e->tick(), e->track());
    if (cr && cr->tick() != e->tick()) {
        cr = nullptr;
    }
    const Fraction end = cr ? (cr->isChord() ? toChord(cr)->endTickIncludingTied() : cr->endTick()) : e->tick() + QUARTER;
    t = { e->tick(), end };
    return true;
}

std::string prefill(const EngravingItem* item)
{
    if (item->isGradualTempoChangeSegment()) {
        const GradualTempoChange* g = toGradualTempoChange(toSpannerSegment(item)->spanner());
        const std::string word = g->beginText().toStdString();
        const std::string text = g->lineVisible() ? word + "..." : word; // rit.... = rit. + continuation line
        return g->visible() ? text : "[" + text + "]";
    }
    if (!item->isTempoText()) {
        return std::string();
    }

    const TempoText* tt = toTempoText(item);
    const String plain = tt->plainText();
    int len = 0;
    TDuration dur;
    const int pos = tt->isRelative() ? -1 : TempoText::findTempoDuration(plain, len, dur);

    std::string words = (pos < 0 ? plain : plain.left(pos)).trimmed().toStdString();
    std::string mark;
    if (pos >= 0) {
        const BeatUnit unit { dur.type(), dur.dots() };
        mark = beatUnitText(unit) + "=" + number(tt->tempo().val * 60.0 / quarters(unit));
        if (!words.empty() && words.back() == '(') {
            words.pop_back();
            mark = "(" + mark + ")";
            while (!words.empty() && words.back() == ' ') {
                words.pop_back();
            }
        }
    } else if (tt->playTempoText() && tt->isNormal() && !tt->isRelative() && tt->visible()) {
        // Allegro [q=96]: a speed the words do not give is a hidden metronome mark.
        const double bpm = tt->tempo().val * 60.0;
        if (std::abs(parseTempo(words).entry.wordBpm - bpm) > 0.5) {
            mark = "[q=" + number(bpm) + "]";
        }
    }

    const std::string text = words.empty() || mark.empty() ? words + mark : words + " " + mark;
    return tt->visible() ? text : "[" + text + "]";
}
}

PopoverDef mu::dorico::tempoPopover()
{
    PopoverDef def;
    def.id = "tempo";
    def.title = "Tempo";
    def.icon = muse::ui::IconCode::Code::TEMPO_CHANGE;
    def.itemTypes = { ElementType::TEMPO_TEXT, ElementType::GRADUAL_TEMPO_CHANGE_SEGMENT };
    def.validate = [](const std::string& text) { return parseTempo(text).status; };
    def.suggest = tempoSuggestions;
    def.prefill = tempopopover::prefill;

    def.apply = [](const std::string& text, ApplyScope, mu::notation::INotationPtr notation) {
        const TempoParse parsed = parseTempo(text);
        if (parsed.status != ParseStatus::Ok) {
            return false;
        }
        const TempoEntry& entry = parsed.entry;
        Score* score = notation->elements()->msScore();
        const bool caret = notation->interaction()->noteInput()->isNoteInputMode();

        mu::notation::INotationUndoStackPtr undoStack = notation->undoStack();
        undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Add tempo marking"));

        tempopopover::Target t;
        EngravingItem* created = nullptr;
        if (tempopopover::target(score, caret, t)) {
            if (entry.kind == TempoEntry::Kind::Gradual) {
                created = tempopopover::addGradual(score, entry, t);
            } else {
                created = tempopopover::addTempoText(score, tempopopover::createTempoText(score, entry), t.start);
            }
        }
        if (!created) {
            undoStack->rollbackChanges();
            return false;
        }
        undoStack->commitChanges();

        // Outside note input the new item is selected, so Return reopens it for editing.
        if (!caret) {
            if (created->isGradualTempoChange()) {
                const auto& segments = toSpanner(created)->spannerSegments();
                created = segments.empty() ? nullptr : segments.front();
            }
            if (created) {
                notation->interaction()->select({ created });
            }
        }
        return true;
    };
    return def;
}
