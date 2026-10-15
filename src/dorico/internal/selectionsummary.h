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

#include <QString>

//! Short selection text for the Dorico status bar (spec 51). Pure: no score, no services, unit-tested.
namespace mu::dorico {
struct SelectionInfo {
    enum class Kind {
        None,
        Single,
        Range,
        Multiple
    };

    Kind kind = Kind::None;
    int firstBar = 0;          // 1-based displayed bar numbers
    int lastBar = 0;
    double startSeconds = 0.0;  // Range only
    double endSeconds = 0.0;
    QString pitch;             // Single only; empty for non-note items
    int voice = 0;             // 1-based, 0 = not shown
};

QString summarise(const SelectionInfo& info);
}
