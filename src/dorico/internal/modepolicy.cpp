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

#include "modepolicy.h"

#include <array>
#include <string_view>

#include "async/async.h"
#include "notationscene/notationcommands.h"
#include "notationscene/notationscenetypes.h"

namespace mu::dorico {
// Command URIs after "command://". An entry ending in '-' or '/' is a prefix; the others match exactly.
static constexpr std::array MODE_EVERYWHERE = {
    // view, layout of the window, library dialogs
    "notation/view/", "notation/diagnostic/", "notation/piano-keyboard", "notation/piano-keyboard/", "notation/show-", "notation/toggle-concert-pitch",
    "notation/toggle-mmrest", "notation/toggle-automation", "notation/open-edit-style", "notation/load-style",
    "notation/save-style", "notation/open-page-settings", "notation/open-parts", "notation/get-location",
    "notation/cancel", "notation/undo", "notation/redo",
    "dorico/mode-", "dorico/toggle-zone-", "dorico/toggle-all-zones", "dorico/toggle-toolbar", "dorico/open-hub", "dorico/export-", "dorico/notation-options",
    "dorico/open-publish", "dorico/preferences", "dorico/noop", "dorico/toggle-stave-sharing", "dorico/jump-bar", "dorico/jump-bar-", "dorico/go-to-bar",
    "dorico/grid-", "dorico/video-properties",
};

// Engrave (Dorico 6, spec 97 audit): select and copy, edit text, change how the music looks, and Alt-arrow (which still
// changes the pitch of selected notes). Delete is allowed here and resets the items' engraving changes instead.
static constexpr std::array MODE_ENGRAVE = {
    "notation/select", "notation/select-", "notation/add-to-selection-", "notation/goto-", "notation/move-left",
    "notation/move-right", "notation/move-left-quickly", "notation/move-right-quickly", "notation/copy", "notation/flip",
    "notation/delete", "notation/reset-shapes-and-positions", "notation/reset-text-style-overrides", "notation/reset-to-default-layout",
    "notation/toggle-autoplace", "notation/autoplace-enabled", "notation/freeze-current-placement",
    "notation/screen-edit-element", "notation/screen-edit-text", "notation/edittext-", "notation/editlyrics-",
    "notation/insert-hbox", "notation/insert-vbox", "notation/insert-textframe", "notation/insert-fretframe",
    "notation/append-hbox", "notation/append-vbox", "notation/append-textframe", "notation/append-fretframe",
    "notation/toggle-visible", "notation/set-visible", "notation/unset-visible",
    "dorico/select-", "dorico/deselect-all", "dorico/filter-", "dorico/filter/", "dorico/next-item-at-position",
    "dorico/prev-item-at-position", "dorico/caret-left", "dorico/caret-right", "dorico/lyrics-",
};

// Off in Write: the graphical nudges (they share Write's Alt-arrow keys, rhythmic moves there) and system / frame
// formatting, which Dorico 6 offers only in Engrave's toolbox.
static constexpr std::array MODE_ENGRAVE_ONLY = {
    "dorico/nudge-", "notation/toggle-system-break", "notation/toggle-page-break", "notation/toggle-section-break",
    "notation/make-into-", "notation/move-measure-to-", "notation/move-system-to-", "notation/apply-system-lock",
    "notation/apply-page-lock", "notation/toggle-system-lock", "notation/toggle-page-lock", "notation/toggle-score-lock",
    "notation/stretch-", "notation/open-breaks",
};

template<size_t N>
static bool modePolicyListed(const std::array<const char*, N>& list, std::string_view path)
{
    for (std::string_view entry : list) {
        const bool prefix = entry.back() == '-' || entry.back() == '/';
        if (prefix ? path.substr(0, entry.size()) == entry : path == entry) {
            return true;
        }
    }
    return false;
}

bool isCommandAllowed(Mode mode, const std::string& command)
{
    static constexpr std::string_view scheme = "command://";
    std::string_view path(command);
    if (path.substr(0, scheme.size()) != scheme) {
        return true;
    }
    path.remove_prefix(scheme.size());
    if (path.substr(0, 9) != "notation/" && path.substr(0, 7) != "dorico/") {
        return true;
    }

    switch (mode) {
    case Mode::Write: return !modePolicyListed(MODE_ENGRAVE_ONLY, path);
    case Mode::Engrave: return modePolicyListed(MODE_EVERYWHERE, path) || modePolicyListed(MODE_ENGRAVE, path)
               || modePolicyListed(MODE_ENGRAVE_ONLY, path);
    case Mode::Setup:
    case Mode::Play:
    case Mode::Print: return modePolicyListed(MODE_EVERYWHERE, path);
    }
    return true;
}

int interactionCaps(Mode mode)
{
    switch (mode) {
    case Mode::Write: return (notation::INTERACTION_ALL & ~(notation::INTERACTION_DRAG_ITEMS | notation::INTERACTION_GRIPS))
               | notation::INTERACTION_DORICO_RIGHT_CLICK;
    case Mode::Engrave: return notation::INTERACTION_SELECT | notation::INTERACTION_DRAG_NOTES | notation::INTERACTION_DRAG_ITEMS
               | notation::INTERACTION_GRIPS | notation::INTERACTION_EDIT_TEXT | notation::INTERACTION_DORICO_RIGHT_CLICK;
    case Mode::Setup:
    case Mode::Play:
    case Mode::Print: return 0;
    }
    return notation::INTERACTION_ALL;
}

void ModeCommandsGate::init()
{
    dispatcher()->preDispatch().onReceive(this, [this](const muse::rcommand::Command& command, bool* allowDispatch) {
        if (!isCommandAllowed(command)) {
            *allowDispatch = false;
        } else if (command == notation::DELETE_COMMAND && modes()->mode() == mu::dorico::Mode::Engrave) {
            // Dorico 6: Delete in Engrave removes nothing, it resets the selected items' engraving changes
            *allowDispatch = false;
            muse::async::Async::call(this, [this]() { dispatcher()->dispatch(notation::RESET_SHAPES_AND_POSITIONS_COMMAND); });
        }
    });
}

bool ModeCommandsGate::isCommandAllowed(const muse::rcommand::Command& command) const
{
    return mu::dorico::isCommandAllowed(modes()->mode(), command.toString());
}

muse::async::Notification ModeCommandsGate::changed() const
{
    return modes()->modeChanged();
}
}
