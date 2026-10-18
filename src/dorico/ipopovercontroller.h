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

#include "global/async/notification.h"
#include "draw/types/geometry.h"
#include "modularity/imoduleinterface.h"

#include "popover/popovertypes.h"

namespace mu::dorico {
//! The open popover, if any (spec 20). One per project window; PopoverModel (QML) shows it.
class IPopoverController : MODULE_CONTEXT_INTERFACE
{
    INTERFACE_ID(IPopoverController)

public:
    virtual ~IPopoverController() = default;

    virtual const PopoverDef* current() const = 0; // nullptr = closed
    virtual std::string initialText() const = 0;
    virtual muse::RectF anchorRect() const = 0;    // canvas coordinates: the box goes above it
    virtual muse::async::Notification stateChanged() const = 0;

    //! Return on a single selected item owned by a popover reopens that popover with the item's entry text.
    virtual bool openForSelection() = 0;

    virtual ParseStatus validate(const std::string& text) const = 0;
    virtual bool accept(const std::string& text, ApplyScope scope) = 0; // false = still open
    //! Navigating popovers (spec 32): enter the text, if any, and move the popover on. False = text invalid or no move.
    virtual bool move(const std::string& text, PopoverMove move) = 0;
    virtual void cancel() = 0;
};
}
