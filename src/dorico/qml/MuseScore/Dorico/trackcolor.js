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

// Play mode's instrument colours (spec 57), shared by the track overview, the Mixer and the Track Inspector.
// Measured on Dorico 6: each instrument moves about 12 degrees round the colour circle from red (Flute 1 red,
// Flute 2 orange, Piccolo yellow, ... Violoncello magenta), bright and saturated; it wraps after ~30 instruments.
// `count` is kept for callers; the step does not depend on it.
function trackColor(index, count) {
    return Qt.hsla((0.985 + 0.034 * index) % 1.0, 0.8, 0.6, 1.0)
}
