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
#include <vector>

#include "global/async/notification.h"
#include "modularity/imoduleinterface.h"

#include "internal/jumpbar/commandcatalog.h"

namespace mu::dorico {
enum class JumpBarMode {
    Commands,
    GoTo
};

//! The jump bar (spec 46): type a command name to run it, or a place to go to. One per project window; JumpBarModel
//! (QML) shows it.
class IJumpBarController : MODULE_CONTEXT_INTERFACE
{
    INTERFACE_ID(IJumpBarController)

public:
    virtual ~IJumpBarController() = default;

    virtual bool isOpen() const = 0;
    virtual JumpBarMode mode() const = 0;
    virtual std::string initialText() const = 0;
    virtual muse::async::Notification stateChanged() const = 0;

    virtual void setMode(JumpBarMode mode) = 0;

    //! Commands mode: the enabled commands matching the text (before any `\=alias`); at most a few rows.
    virtual std::vector<CatalogEntry> search(const std::string& text) const = 0;
    //! What Down shows on an empty field
    virtual std::vector<CatalogEntry> mostUsed() const = 0;

    //! Return (`assignOnly` false) or Opt-Return. Commands mode: `uri` is the chosen row; `Name\=alias` in the text gives
    //! it that alias. Go To mode: goes to the place. False = nothing happened (no row chosen, or the place does not
    //! exist) and the bar stays open.
    virtual bool accept(const std::string& text, const std::string& uri, bool assignOnly) = 0;
    virtual void close() = 0;
};
}
