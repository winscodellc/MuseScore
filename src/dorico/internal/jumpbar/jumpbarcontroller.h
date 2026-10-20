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
#include "rcommand/icommandsregister.h"
#include "rcommand/icommandsstate.h"
#include "shortcuts_v2/icommandshortcutsregister.h"
#include "context/iglobalcontext.h"

#include "../../ijumpbarcontroller.h"

namespace mu::dorico {
class JumpBarController : public IJumpBarController, public muse::async::Asyncable, public muse::Contextable,
    public muse::rcommand::Commandable
{
    muse::GlobalInject<muse::rcommand::ICommandsRegister> commandsRegister;
    muse::GlobalInject<muse::shortcuts::ICommandShortcutsRegister> shortcutsRegister;
    muse::ContextInject<muse::rcommand::ICommandDispatcher> dispatcher = { this };
    muse::ContextInject<muse::rcommand::ICommandsState> commandsState = { this };
    muse::ContextInject<context::IGlobalContext> globalContext = { this };

public:
    JumpBarController(const muse::modularity::ContextPtr& iocCtx)
        : muse::Contextable(iocCtx) {}

    void init();

    bool isOpen() const override { return m_open; }
    JumpBarMode mode() const override { return m_mode; }
    std::string initialText() const override { return m_initialText; }
    muse::async::Notification stateChanged() const override { return m_stateChanged; }

    void setMode(JumpBarMode mode) override;
    std::vector<CatalogEntry> search(const std::string& text) const override;
    std::vector<CatalogEntry> mostUsed() const override;
    bool accept(const std::string& text, const std::string& uri, bool assignOnly) override;
    void close() override;

private:
    void open(JumpBarMode mode, const std::string& text);
    std::vector<CatalogEntry> catalog() const;
    std::string lastCommandTitle() const;
    void setAlias(const std::string& uri, const std::string& alias);
    void recordUse(const std::string& uri);
    bool goTo(const std::string& text);

    bool m_open = false;
    JumpBarMode m_mode = JumpBarMode::Commands;
    std::string m_initialText;
    muse::async::Notification m_stateChanged;
};
}
