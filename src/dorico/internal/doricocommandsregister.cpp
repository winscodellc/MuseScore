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

#include "doricocommandsregister.h"

#include "../doricocommands.h"
#include "rcommand/commandtypes.h"

using namespace muse;
using namespace muse::rcommand;
using namespace mu::dorico;

static const std::vector<CommandInfo> s_commandInfos = {
    CommandInfo{
        DORICO_NOOP_COMMAND,
        TranslatableString("dorico", "Dorico no-op"),
        TranslatableString("dorico", "Does nothing; proves the dorico module is wired"),
        InputSchema(),
        Decoration()
    },
};

std::string DoricoCommandsRegister::moduleName() const
{
    return "dorico";
}

const std::vector<Command>& DoricoCommandsRegister::commandList() const
{
    static std::vector<Command> commands;
    if (commands.empty()) {
        commands.reserve(s_commandInfos.size());
        for (const auto& info : s_commandInfos) {
            commands.push_back(info.command);
        }
    }
    return commands;
}

const std::vector<CommandInfo>& DoricoCommandsRegister::commandInfoList() const
{
    return s_commandInfos;
}
