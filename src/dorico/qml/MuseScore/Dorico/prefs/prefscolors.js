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

.pragma library

// Preferences dialog colors (spec 100), measured on Dorico Pro 6 screenshots (2026-10-22). The Dark values are
// Dorico's; the Light ones keep the same relationships.
function pane(isDark) { return isDark ? "#1f2429" : "#e9ebee" }
function card(isDark) { return isDark ? "#171b1f" : "#ffffff" }
function band(isDark) { return isDark ? "#2b2f35" : "#d3d7dc" }
function pageBand(isDark) { return isDark ? "#343940" : "#c4c9cf" }
function sidebar(isDark) { return isDark ? "#131415" : "#f4f5f6" }
function sidebarRow(isDark) { return isDark ? "#282d33" : "#e9ebee" }
function sidebarSelected(isDark) { return isDark ? "#454a4f" : "#cfd4da" }
function accent() { return "#40acff" }
function text(isDark) { return isDark ? "#e4e6e8" : "#1d2024" }
function dimText(isDark) { return isDark ? "#9aa0a6" : "#5d646b" }
function control(isDark) { return isDark ? "#36414d" : "#e2e5e9" }
function highlight() { return "#5c4b00" }
function currentHighlight() { return "#a07f00" }
