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
#include <string>
#include <vector>

#include "engraving/types/types.h"
#include "notation/inotation.h"
#include "ui/view/iconcodes.h"

namespace mu::engraving {
class EngravingItem;
}

namespace mu::dorico {
enum class ParseStatus {
    Ok,
    Incomplete,  // valid prefix: neutral border, Return does nothing
    Unsupported, // valid Dorico syntax we deliberately do not implement: red border + "not supported yet"
    Invalid      // red border, Return does nothing
};

enum class ApplyScope {
    Default, // Return
    Local    // Opt-Return
};

//! One popover (spec 20). A plain record of callbacks, like a TS object literal; specs 21-36 each provide one.
//! `apply` runs inside the popover's undo step and acts on the current selection or note-input caret.
//! When the popover edits an existing item (Return on it, or opened on a note that already has one), that item is
//! selected before `apply`, so applying replaces it instead of adding a second one.
struct PopoverDef {
    std::string id;    // URI tail: command://dorico/popover/<id>
    std::string title;
    muse::ui::IconCode::Code icon = muse::ui::IconCode::Code::NONE; // MuseScore icon, never a Dorico asset
    bool supportsLocal = false;                                     // Opt-Return means something
    std::vector<engraving::ElementType> itemTypes;                  // selected item types whose Return reopens this popover

    std::function<ParseStatus(const std::string&)> validate;
    std::function<bool(const std::string&, ApplyScope, notation::INotationPtr)> apply; // true = applied
    std::function<std::string(const engraving::EngravingItem*)> prefill;              // entry text for Return-to-edit
    std::function<void(notation::INotationPtr)> noteInputEnded; // optional: close open-ended items (p<) when input stops
};
}
