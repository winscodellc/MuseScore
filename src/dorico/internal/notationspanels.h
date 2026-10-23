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

#include <functional>
#include <vector>

#include <QString>
#include <QStringList>

#include "engraving/types/types.h"

//! Write mode's Notations panels (spec 53): which MuseScore palette cells each Dorico panel section shows.
//! Pure data + matching, no palette objects, so it is unit-tested; DoricoPanelModel feeds it the real cells.
namespace mu::dorico {
// The MuseScore master palettes a section can draw from (PaletteCreator::new<X>Palette).
enum class SourcePalette {
    Clefs,
    KeySignatures,
    TimeSignatures,
    Tempo,
    Accidentals,
    Dynamics,
    Articulations,
    Text,
    Keyboard,
    Repeats,
    BarLines,
    Ornaments,
    Breaths,
    Arpeggios,
    Tremolos,
    Harp,
    Guitar,
    Lines,
    Brackets
};

struct NotationsSection {
    QString panel;                              // panel id, e.g. "clefs"
    QString title;                              // section name as in the Dorico manual
    SourcePalette palette;
    std::vector<engraving::ElementType> types;  // empty: any element type
    QStringList names;                          // empty: any cell; see nameMatches() for the pattern syntax
    bool rest = false;                          // only the cells no other section of the same panel takes
};

struct PaletteCellInfo {
    engraving::ElementType type;
    QString name;                               // untranslated cell name (PaletteCell::name)
};

//! Every panel section, in manual order. Panels with no section are stubs (cues, comments, proofreading) or have their own QML (video).
//! "engraveBracketing" is not a Write panel: Engrave's Formatting panel shows it (spec 56).
const std::vector<NotationsSection>& notationsSections();
std::vector<NotationsSection> sectionsOfPanel(const QString& panel);

//! `pattern` matches `name` when equal; "abc*" is a prefix (dynamics: "sf (*" for "sf (sforzando)") and "*abc*" a
//! case-insensitive substring.
bool nameMatches(const QString& pattern, const QString& name);

//! For each section, the indices of its cells in its source palette (palette order). Name patterns that match no
//! cell are appended to `unknownNames` as "section: pattern", so a renamed MuseScore cell is noticed.
using CellsOf = std::function<const std::vector<PaletteCellInfo>& (SourcePalette)>;
std::vector<std::vector<size_t> > resolveSections(const std::vector<NotationsSection>& sections, const CellsOf& cellsOf,
                                                  QStringList* unknownNames = nullptr);
}
