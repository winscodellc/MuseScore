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
