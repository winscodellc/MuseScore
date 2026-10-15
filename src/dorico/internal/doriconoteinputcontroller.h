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
#include <optional>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "rcommand/commandable.h"
#include "rcommand/icommanddispatcher.h"
#include "toast/itoastservice.h"
#include "context/iglobalcontext.h"
#include "notation/inotationconfiguration.h"
#include "notationscene/inotationcommandscontroller.h"

#include "../idoricomodestate.h"
#include "noteinpututils.h"

namespace mu::dorico {
//! Handlers for the Dorico-style note input keys (spec 11).
//! Think of it as a Spring @Component that owns the key handlers: each command id is bound to a small method here,
//! and most methods just decide which existing notation/* command to dispatch.
class DoricoNoteInputController : public muse::async::Asyncable, public muse::Contextable, public muse::rcommand::Commandable
{
    muse::ContextInject<muse::rcommand::ICommandDispatcher> dispatcher = { this };
    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<notation::INotationCommandsController> notationController = { this };
    muse::ContextInject<IDoricoModeState> modes = { this };
    muse::GlobalInject<notation::INotationConfiguration> notationConfiguration;
    muse::GlobalInject<muse::toast::IToastService> toastService;

public:
    DoricoNoteInputController(const muse::modularity::ContextPtr& iocCtx)
        : muse::Contextable(iocCtx) {}

    void init();

private:
    void reg(const muse::rcommand::Command& command, std::function<void()> handler);

    notation::INotationPtr currentNotation() const;
    notation::INotationNoteInputPtr noteInput() const;
    bool inNoteInput() const;

    void onReturn();
    void onSpace();
    void onPitch(int note);
    void onForcedOctave(int note, bool above);
    void onRest();
    void onToggleRestInput();
    void onToggleChordMode();
    void onToggleInsertMode();
    void onSetDuration(int digit);
    void onCycleDots();
    void onNextVoice();
    void onNewVoice();
    void onToggleArticulation(mu::engraving::SymId sym);

    std::optional<int> referenceStep(bool above) const;
    bool voiceHasContent(size_t staffIdx, size_t voice) const;
    void switchToVoice(size_t voice);

    void onNoteAdded();
    void holdCaretOnChord();
    void closeChord();
    void rebarAfterInsert();

    void onNotationChanged();
    void syncInsertModeFromInput();

    DotTap m_dotTap;
    // true after a chord-mode letter was entered: the next letter stacks onto that chord instead of starting a new one,
    // and the caret is held on the chord. Space / duration key / mode change move the caret past it.
    // ponytail: not cleared by a mouse click elsewhere.
    bool m_chordPending = false;
    notation::INotationNoteInputPtr m_subscribedInput;
};
}
