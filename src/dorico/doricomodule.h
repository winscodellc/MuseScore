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

#include <memory>

#include "modularity/imodulesetup.h"

namespace mu::dorico {
class DoricoActionsController;
class DoricoCaret;
class DoricoDurationsController;
class DoricoModeState;
class DoricoNoteInputController;
class DoricoFormatController;
class DoricoPasteController;
class DoricoSelectionController;
class DoricoTransformController;
class JumpBarController;
class ModeCommandsGate;
class PopoverController;

class DoricoModule : public muse::modularity::IModuleSetup
{
public:
    std::string moduleName() const override;

    void resolveImports() override;
    void onInit(const muse::IApplication::RunMode& mode) override;

    muse::modularity::IContextSetup* newContext(const muse::modularity::ContextPtr& ctx) const override;

    static void applyFirstRunDefaults();
};

class DoricoContext : public muse::modularity::IContextSetup
{
public:
    DoricoContext(const muse::modularity::ContextPtr& ctx)
        : muse::modularity::IContextSetup(ctx) {}

    void registerExports() override;
    void resolveImports() override;
    void onInit(const muse::IApplication::RunMode& mode) override;

private:
    std::shared_ptr<DoricoActionsController> m_actionsController;
    std::shared_ptr<DoricoCaret> m_caret;
    std::shared_ptr<DoricoDurationsController> m_durationsController;
    std::shared_ptr<DoricoModeState> m_modeState;
    std::shared_ptr<DoricoNoteInputController> m_noteInputController;
    std::shared_ptr<DoricoFormatController> m_formatController;
    std::shared_ptr<DoricoPasteController> m_pasteController;
    std::shared_ptr<DoricoSelectionController> m_selectionController;
    std::shared_ptr<DoricoTransformController> m_transformController;
    std::shared_ptr<JumpBarController> m_jumpBarController;
    std::shared_ptr<ModeCommandsGate> m_modeGate;
    std::shared_ptr<PopoverController> m_popoverController;
};
}
