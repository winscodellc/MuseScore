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

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "rcommand/commandable.h"
#include "rcommand/icommanddispatcher.h"
#include "context/iglobalcontext.h"
#include "notationscene/inotationcommandscontroller.h"

#include "../idoricomodestate.h"
#include "../ipopovercontroller.h"
#include "../popover/popoverregistry.h"

namespace mu::dorico {
//! Opens, applies and closes popovers (spec 20). Each Shift+letter command opens one; PopoverModel shows it.
class PopoverController : public IPopoverController, public muse::async::Asyncable, public muse::Contextable,
    public muse::rcommand::Commandable
{
    muse::ContextInject<muse::rcommand::ICommandDispatcher> dispatcher = { this };
    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<notation::INotationCommandsController> notationController = { this };
    muse::ContextInject<IDoricoModeState> modes = { this };

public:
    PopoverController(const muse::modularity::ContextPtr& iocCtx)
        : muse::Contextable(iocCtx) {}

    void init();

    const PopoverDef* current() const override;
    std::string initialText() const override;
    muse::RectF anchorRect() const override;
    muse::async::Notification stateChanged() const override;

    bool openForSelection() override;

    ParseStatus validate(const std::string& text) const override;
    bool accept(const std::string& text, ApplyScope scope) override;
    void cancel() override;

private:
    bool canOpen() const;
    void open(const PopoverDef* def, const std::string& text);
    muse::RectF computeAnchor() const;

    PopoverRegistry m_registry = makePopoverRegistry();
    const PopoverDef* m_current = nullptr;
    std::string m_initialText;
    muse::RectF m_anchor;
    muse::async::Notification m_stateChanged;
};
}
