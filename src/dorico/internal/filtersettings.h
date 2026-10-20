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

#include <string>

#include "settings.h"

//! Filter settings (spec 45): whether Edit > Filter keeps or drops its category, and the last Notes by Pitch list.
namespace mu::dorico {
inline const muse::Settings::Key FILTER_MODE_KEY("dorico", "dorico/filter/mode"); // "select" (default) | "deselect"
inline const muse::Settings::Key FILTER_PITCHES_KEY("dorico", "dorico/filter/pitches");
inline const muse::Settings::Key FILTER_PITCHES_SOUNDING_KEY("dorico", "dorico/filter/pitchesSounding");

inline bool filterIsDeselectOnly()
{
    return muse::settings()->value(FILTER_MODE_KEY).toString() == "deselect";
}
}
