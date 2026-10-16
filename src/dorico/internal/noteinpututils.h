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

#include <cstdint>
#include <optional>
#include <vector>

#include "engraving/dom/pitchspelling.h"

namespace mu::dorico {
// Step = octave * 7 + diatonic step (C = 0 ... B = 6), the same convention as engraving's NoteInputParams::step.
// Both helpers assume ref >= 0, which always holds for real pitches.

//! Nearest step with the given letter that is strictly above ref.
inline int stepAbove(int ref, int note)
{
    int s = (ref / 7) * 7 + note;
    return s <= ref ? s + 7 : s;
}

//! Nearest step with the given letter that is strictly below ref.
inline int stepBelow(int ref, int note)
{
    int s = (ref / 7) * 7 + note;
    return s >= ref ? s - 7 : s;
}

//! Caret step to the right (spec 12), in ticks: the next grid position (counted from the bar start) or the end of the
//! chord/rest the caret is in, whichever comes first.
inline int nextCaretTick(int cur, int barStart, int grid, int crEnd)
{
    const int next = barStart + ((cur - barStart) / grid + 1) * grid;
    return next < crEnd ? next : crEnd;
}

//! Caret step to the left: the previous grid position or the start of the chord/rest that holds it, whichever is
//! later. Needs cur > crStart >= barStart.
inline int prevCaretTick(int cur, int barStart, int grid, int crStart)
{
    const int prev = barStart + ((cur - barStart - 1) / grid) * grid;
    return prev > crStart ? prev : crStart;
}

//! Up/Down target staff (spec 12): the nearest visible staff above / below `from`, or the top / bottom visible one when
//! toEnd. std::nullopt = nowhere to go.
inline std::optional<size_t> verticalStaff(const std::vector<bool>& visible, size_t from, bool up, bool toEnd)
{
    std::optional<size_t> found;
    if (up) {
        for (size_t i = from; i-- > 0;) {
            if (visible[i]) {
                found = i;
                if (!toEnd) {
                    break;
                }
            }
        }
    } else {
        for (size_t i = from + 1; i < visible.size(); ++i) {
            if (visible[i]) {
                found = i;
                if (!toEnd) {
                    break;
                }
            }
        }
    }
    return found;
}

//! Respell (spec 12, Opt-= / Opt--): the tpc that spells the same pitch with the note name one step above / below,
//! or tpc itself when that would need more than a triple accidental.
inline int respellTpc(int tpc, bool above)
{
    static constexpr int NATURAL_PITCH[7] = { 0, 2, 4, 5, 7, 9, 11 }; // C D E F G A B
    const int step = engraving::tpc2step(tpc);
    const int newStep = (step + (above ? 1 : 6)) % 7;
    int delta = (NATURAL_PITCH[newStep] - NATURAL_PITCH[step] + 12) % 12; // semitones between the two letters
    if (delta > 6) {
        delta -= 12;
    }
    const int alter = static_cast<int>(engraving::tpc2alter(tpc)) - delta;
    if (alter < -3 || alter > 3) {
        return tpc;
    }
    return engraving::step2tpc(newStep, static_cast<engraving::AccidentalVal>(alter));
}

//! Double-tap detector for duration digits (6 6 = dotted quarter).
//! The clock is injected by the caller so tests do not sleep.
class DotTap
{
public:
    //! true = this press is the second tap and should toggle a dot instead of setting the duration.
    bool press(int digit, int64_t nowMs, int windowMs)
    {
        const bool dot = m_digit == digit && (nowMs - m_atMs) < windowMs;
        m_digit = dot ? -1 : digit; // a third quick tap starts over
        m_atMs = nowMs;
        return dot;
    }

    void reset() { m_digit = -1; }

private:
    int m_digit = -1;
    int64_t m_atMs = 0;
};
}
