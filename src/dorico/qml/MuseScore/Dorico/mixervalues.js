// SPDX-License-Identifier: GPL-3.0-only
// MuseScore-Studio-CLA-applies
//
// MuseScore Studio
// Music Composition & Notation
//
// Copyright (C) 2026 MuseScore Limited and others
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

.pragma library

// Typed values in the Mixer (spec 63). Both return null when the text is not a value.

// Volume in dB: a number, "inf" for the maximum, "-inf" / "-oo" for the minimum; clamped to the fader range.
function parseVolume(text, min, max) {
    const t = text.trim().toLowerCase()
    if (t === "inf" || t === "+inf") {
        return max
    }
    if (t === "-inf" || t === "-oo") {
        return min
    }
    const value = Number(t)
    return t === "" || isNaN(value) ? null : Math.max(min, Math.min(max, value))
}

// Pan: -100..100, or C, L<n> (left = negative), R<n>; clamped.
function parsePan(text, min, max) {
    const t = text.trim().toUpperCase()
    if (t === "C") {
        return 0
    }
    const side = /^([LR])\s*(\d+)$/.exec(t)
    const value = side ? (side[1] === "L" ? -1 : 1) * Number(side[2]) : (t === "" ? NaN : Number(t))
    return isNaN(value) ? null : Math.max(min, Math.min(max, Math.round(value)))
}
