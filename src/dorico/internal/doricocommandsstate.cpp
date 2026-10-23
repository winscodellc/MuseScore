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

#include <algorithm>

#include <QClipboard>
#include <QGuiApplication>

#include "dockwindow/idockwindow.h"
#include "notation/inotation.h"
#include "notation/inotationelements.h"
#include "notation/inotationstyle.h"
#include "engraving/dom/score.h"

#include "../doricocommands.h"
#include "../video/videodata.h"
#include "filtersettings.h"
#include "modepolicy.h"
#include "pastespecial.h"
#include "doricoperf.h"
#include "doricoshell.h"

#include "log.h"

using namespace muse;
using namespace muse::rcommand;
using namespace mu::dorico;
using mu::engraving::SymId;

static const std::map<Command, std::vector<SymId> > ARTICULATION_SYMS = {
    { DORICO_TOGGLE_STRESSED_COMMAND, { SymId::articStressAbove } },
    { DORICO_TOGGLE_UNSTRESSED_COMMAND, { SymId::articUnstressAbove } },
    { DORICO_TOGGLE_STACCATISSIMO_COMMAND, { SymId::articStaccatissimoAbove } },
    { DORICO_TOGGLE_TENUTO_STACCATO_COMMAND, { SymId::articTenutoAbove, SymId::articStaccatoAbove } },
};

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
    // Stave sharing (Players "…" menu) is a score style
    notationController()->notationStyleChanged().onNotify(this, [this]() {
        updateCommandStates();
    });
    // Notes panel articulation highlights follow the selected notes, also outside note input.
    notationController()->selectionChanged().onNotify(this, [this]() {
        updateCommandStates();
    });

    // Explode / Reduce are enabled by what is on the clipboard
    if (QGuiApplication::clipboard()) {
        m_clipboardConnection = QObject::connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, QGuiApplication::clipboard(), [this]() {
            m_clip = pasteClipFromClipboard();
            updateCommandStates();
        });
        m_clip = pasteClipFromClipboard();
    }

    // Window > Show Toolbar / Show ... Zone are checked while shown
    auto listenToDocks = [this]() {
        if (muse::dock::IDockWindow* window = dockWindowProvider()->window()) {
            window->docksOpenStatusChanged().onReceive(this, [this](const QStringList&) { updateCommandStates(); });
        }
    };
    if (dockWindowProvider()->window()) {
        listenToDocks();
    } else {
        dockWindowProvider()->windowChanged().onNotify(this, listenToDocks);
    }

    // Edit > Filter > Select Only / Deselect Only show the current mode as checked
    muse::settings()->valueChanged(FILTER_MODE_KEY).onReceive(this, [this](const Val&) {
        updateCommandStates();
    });

    updateCommandStates();
}

void DoricoCommandsState::deinit()
{
    QObject::disconnect(m_clipboardConnection);
    muse::settings()->valueChanged(FILTER_MODE_KEY).disconnect(this);
    dockWindowProvider()->windowChanged().disconnect(this);
    if (muse::dock::IDockWindow* window = dockWindowProvider()->window()) {
        window->docksOpenStatusChanged().disconnect(this);
    }
    modes()->modeChanged().disconnect(this);
    modes()->modesChanged().disconnect(this);
    notationController()->noteInputStateChanged().disconnect(this);
    notationController()->selectionChanged().disconnect(this);
    notationController()->notationStyleChanged().disconnect(this);
    globalContext()->currentProjectChanged().disconnect(this);
    notationController()->textEditingChanged().disconnect(this);
}

void DoricoCommandsState::updateCommandStates()
{
    IF_ASSERT_FAILED(m_moduleRegister) {
        return;
    }

    PerfTimer perf("commandStates");
    const SelectionFacts facts = selectionFacts();
    for (const auto& command : m_moduleRegister->commandList()) {
        CommandState newState = commandState(command, facts);
        if (m_commandStates[command] != newState) {
            m_commandStates[command] = newState;
            m_commandStateChanged.send(command, newState);
        }
    }
}

DoricoCommandsState::SelectionFacts DoricoCommandsState::selectionFacts() const
{
    const notation::INotationPtr notation = globalContext()->currentNotation();
    const engraving::Score* score = notation ? notation->elements()->msScore() : nullptr;
    return { score && !score->selection().noteList().empty(), notationController()->currentArticulations() };
}

bool DoricoCommandsState::anyDockOpen(const std::vector<QString>& names) const
{
    const muse::dock::IDockWindow* window = dockWindowProvider()->window();
    return window && std::any_of(names.begin(), names.end(), [window](const QString& name) { return window->isDockOpen(name); });
}

CommandState DoricoCommandsState::commandState(const Command& command) const
{
    return commandState(command, selectionFacts());
}

CommandState DoricoCommandsState::commandState(const Command& command, const SelectionFacts& facts) const
{
    const bool editingText = notationController()->isTextEditing() || notationController()->isLyricsEditing();
    // The Hub and Preferences open without a project too (the Hub is where projects are opened)
    const bool lyricsCommand = command == DORICO_LYRICS_CLOSE_COMMAND || command == DORICO_LYRICS_NEXT_LINE_COMMAND
                               || command == DORICO_LYRICS_ABOVE_COMMAND
                               || command == DORICO_LYRICS_BELOW_COMMAND;
    const bool enabled = lyricsCommand ? notationController()->isLyricsEditing()
                         : (globalContext()->currentProject() != nullptr || command == DORICO_OPEN_HUB_COMMAND || command == DORICO_PREFERENCES_COMMAND)
                           && !editingText;

    const notation::INotationPtr notation = globalContext()->currentNotation();
    const engraving::Score* score = notation ? notation->elements()->msScore() : nullptr;
    bool pasteSpecialEnabled = true;
    if (command == DORICO_DUPLICATE_TO_STAFF_ABOVE_COMMAND || command == DORICO_MOVE_TO_STAFF_ABOVE_COMMAND) {
        pasteSpecialEnabled = score && pasteSpecialCanMove(score, -1);
    } else if (command == DORICO_DUPLICATE_TO_STAFF_BELOW_COMMAND || command == DORICO_MOVE_TO_STAFF_BELOW_COMMAND) {
        pasteSpecialEnabled = score && pasteSpecialCanMove(score, 1);
    } else if (command == DORICO_SWAP_STAVES_COMMAND) {
        pasteSpecialEnabled = score && pasteSpecialCanSwap(score);
    } else if (command == DORICO_EXPLODE_COMMAND) {
        pasteSpecialEnabled = score && pasteSpecialCanExplode(m_clip, pasteSpecialRangeStaves(score));
    } else if (command == DORICO_REDUCE_COMMAND) {
        pasteSpecialEnabled = score && pasteSpecialCanReduce(m_clip, pasteSpecialRangeStaves(score));
    } else if (std::find(DORICO_TRANSFORM_COMMANDS.begin(), DORICO_TRANSFORM_COMMANDS.end(), command) != DORICO_TRANSFORM_COMMANDS.end()
               || command.toString().starts_with("command://dorico/stem-")
               || command.toString().starts_with("command://dorico/notehead-")) { // a rest has no stem or notehead
        pasteSpecialEnabled = facts.hasNotes;
    } else if (command == DORICO_PASTE_INTO_NEW_VOICE_COMMAND || command == DORICO_PASTE_ARTICULATIONS_COMMAND
               || std::find(DORICO_PASTE_INTO_VOICE_COMMANDS.begin(), DORICO_PASTE_INTO_VOICE_COMMANDS.end(), command)
               != DORICO_PASTE_INTO_VOICE_COMMANDS.end()) {
        pasteSpecialEnabled = score && pasteSpecialHasTarget(score);
    }

#ifdef DORICO_HAS_VIDEO
    // Window > Video and F4 do nothing without an attached video (Dorico 6.2)
    const bool videoEnabled = command != DORICO_TOGGLE_VIDEO_WINDOW_COMMAND || (score && readVideoData(score).hasVideo());
#else
    // Built without Qt Multimedia: no video playback
    const bool videoEnabled = command != DORICO_VIDEO_ATTACH_COMMAND && command != DORICO_TOGGLE_VIDEO_WINDOW_COMMAND;
#endif

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
    } else if (command == DORICO_FILTER_SELECT_ONLY_COMMAND || command == DORICO_FILTER_DESELECT_ONLY_COMMAND) {
        checked = filterIsDeselectOnly() == (command == DORICO_FILTER_DESELECT_ONLY_COMMAND);
    } else if (command == DORICO_TOGGLE_TOOLBAR_COMMAND) {
        checked = anyDockOpen(toolbarDocks());
    } else if (const auto zone = std::find(DORICO_TOGGLE_ZONE_COMMANDS.begin(), DORICO_TOGGLE_ZONE_COMMANDS.end(), command);
               zone != DORICO_TOGGLE_ZONE_COMMANDS.end()) {
        checked = anyDockOpen(zoneDocks(modes()->mode(), static_cast<Zone>(zone - DORICO_TOGGLE_ZONE_COMMANDS.begin())));
    } else if (command == DORICO_TOGGLE_STAVE_SHARING_COMMAND) {
        const notation::INotationPtr notation = globalContext()->currentNotation();
        checked = notation && notation->style()->styleValue(engraving::Sid::enableStaveSharing).toBool();
    } else if (command == DORICO_TOGGLE_VIDEO_WINDOW_COMMAND) {
        checked = modes()->isVideoWindowVisible();
    } else if (const auto it = ARTICULATION_SYMS.find(command); it != ARTICULATION_SYMS.end()) {
        // Same rule as MuseScore's toggles: the input articulations, or those on the selected notes. Combined
        // articulations come back split (tenuto-staccato = tenuto + staccato), so every part must be there.
        checked = std::all_of(it->second.begin(), it->second.end(), [&](SymId s) { return facts.articulations.count(s) > 0; });
    }

    return CommandState(enabled && pasteSpecialEnabled && videoEnabled && isCommandAllowed(modes()->mode(), command.toString()), checked);
}

async::Channel<Command, CommandState> DoricoCommandsState::commandStateChanged() const
{
    return m_commandStateChanged;
}
