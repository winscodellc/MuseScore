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

#include "doricocommandsstate.h"

#include "../doricocommands.h"

#include "log.h"

using namespace muse;
using namespace muse::rcommand;
using namespace mu::dorico;

std::string DoricoCommandsState::moduleName() const
{
    return "dorico";
}

void DoricoCommandsState::init()
{
    m_moduleRegister = commandsRegister()->moduleRegister(moduleName());
    IF_ASSERT_FAILED(m_moduleRegister) {
        return;
    }

    globalContext()->currentProjectChanged().onNotify(this, [this]() {
        updateCommandStates();
    });

    // Keys like Space, Return and the letters must reach the text editor while text is being edited.
    notationController()->textEditingChanged().onReceive(this, [this](bool) {
        updateCommandStates();
    });

    // The active mode tab is shown as checked.
    modes()->modeChanged().onNotify(this, [this]() {
        updateCommandStates();
    });

    // Notes toolbox highlights (spec 52): note input, chord and insert mode.
    modes()->modesChanged().onNotify(this, [this]() {
        updateCommandStates();
    });
    notationController()->noteInputStateChanged().onNotify(this, [this]() {
        updateCommandStates();
    });

    updateCommandStates();
}

void DoricoCommandsState::deinit()
{
    modes()->modeChanged().disconnect(this);
    modes()->modesChanged().disconnect(this);
    notationController()->noteInputStateChanged().disconnect(this);
    globalContext()->currentProjectChanged().disconnect(this);
    notationController()->textEditingChanged().disconnect(this);
}

void DoricoCommandsState::updateCommandStates()
{
    IF_ASSERT_FAILED(m_moduleRegister) {
        return;
    }

    for (const auto& command : m_moduleRegister->commandList()) {
        CommandState newState = commandState(command);
        if (m_commandStates[command] != newState) {
            m_commandStates[command] = newState;
            m_commandStateChanged.send(command, newState);
        }
    }
}

CommandState DoricoCommandsState::commandState(const Command& command) const
{
    const bool editingText = notationController()->isTextEditing() || notationController()->isLyricsEditing();
    const bool enabled = globalContext()->currentProject() != nullptr && !editingText;

    bool checked = false;
    for (size_t i = 0; i < DORICO_MODE_COMMANDS.size(); ++i) {
        if (command == DORICO_MODE_COMMANDS[i]) {
            checked = modes()->mode() == static_cast<mu::dorico::Mode>(i);
        }
    }

    if (command == DORICO_RETURN_COMMAND) {
        checked = notationController()->isNoteInputMode();
    } else if (command == DORICO_TOGGLE_CHORD_MODE_COMMAND) {
        checked = modes()->isChordMode();
    } else if (command == DORICO_TOGGLE_INSERT_MODE_COMMAND) {
        checked = modes()->isInsertMode();
    }

    return CommandState(enabled, checked);
}

async::Channel<Command, CommandState> DoricoCommandsState::commandStateChanged() const
{
    return m_commandStateChanged;
}
