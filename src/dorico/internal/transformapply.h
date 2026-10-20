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

#include "notation/inotation.h"

#include "../popover/parsers/notetoolsparser.h"

namespace mu::dorico {
//! Applies an invert / reverse / rotate (spec 65) to the selection, as one undo step. False, with the score untouched,
//! if there is nothing to transform, a result leaves the MIDI range, or a rhythm operation meets tuplets or grace notes.
bool applyTransform(const notation::INotationPtr& notation, const TransformSpec& spec);
}
