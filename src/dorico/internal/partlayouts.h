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

#include "global/types/string.h"

namespace mu::dorico {
//! Instrument part layouts deleted in Setup mode (spec 55). MuseScore regenerates a part for every instrument, so the
//! deleted ones are listed by part id in this score meta tag, which MasterNotation::updatePotentialExcerpts skips.
//! The key is repeated there because the notation module cannot include dorico headers.
inline const muse::String DELETED_PART_LAYOUTS_TAG(u"deletedPartLayouts");

//! The tag value is a comma-separated list of part ids.
inline muse::String addDeletedPartLayout(const muse::String& tag, const muse::String& partId)
{
    muse::StringList ids = tag.split(u',', muse::SkipEmptyParts);
    if (!ids.contains(partId)) {
        ids.push_back(partId);
    }
    return ids.join(u",");
}
}
