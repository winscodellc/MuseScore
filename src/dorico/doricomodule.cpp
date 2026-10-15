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

#include "doricomodule.h"

#include "modularity/ioc.h"
#include "settings.h"
#include "shortcuts_v2/ishortcutsconfiguration.h"
#include "rcommand/icommandsregister.h"
#include "rcommand/icommandsstate.h"

#include "interactive/iinteractiveuriregister.h"

#include "internal/doricoactionscontroller.h"
#include "internal/doricocommandsregister.h"
#include "internal/doricocommandsstate.h"
#include "internal/doricomenuarranger.h"
#include "internal/doricomodestate.h"
#include "internal/doricoshell.h"
#include "internal/doriconoteinputcontroller.h"
#include "internal/popovercontroller.h"

#include "log.h"

using namespace mu::dorico;
using namespace muse;
using namespace muse::modularity;

static const std::string mname("dorico");
static const Settings::Key KEYMAP_APPLIED_KEY(mname, "dorico/keymapApplied");

std::string DoricoModule::moduleName() const
{
    return mname;
}

void DoricoModule::resolveImports()
{
    // Write reuses musescore://notation (registered by appshell); the other modes are ours.
    auto ir = globalIoc()->resolve<muse::interactive::IInteractiveUriRegister>(mname);
    if (ir) {
        for (Mode mode : { Mode::Setup, Mode::Engrave, Mode::Play, Mode::Print }) {
            ir->registerPageUri(modeUri(mode));
        }
    }

    auto cr = globalIoc()->resolve<muse::rcommand::ICommandsRegister>(mname);
    if (cr) {
        cr->reg(std::make_shared<DoricoCommandsRegister>());
    }
}

void DoricoModule::onInit(const IApplication::RunMode&)
{
    LOGI() << "dorico module init";

    settings()->setDefaultValue(KEYMAP_APPLIED_KEY, Val(false));

    // ponytail: runs once; if the user later picks another preset we never touch it again
    if (!settings()->value(KEYMAP_APPLIED_KEY).toBool()) {
        auto shortcutsConfig = globalIoc()->resolve<muse::shortcuts::IShortcutsConfiguration>(mname);
        if (shortcutsConfig) {
            shortcutsConfig->setCurrentShortcutsPresetName("shortcuts_dorico");
        }
        settings()->setSharedValue(KEYMAP_APPLIED_KEY, Val(true));
    }
}

IContextSetup* DoricoModule::newContext(const muse::modularity::ContextPtr& ctx) const
{
    return new DoricoContext(ctx);
}

void DoricoContext::registerExports()
{
    m_actionsController = std::make_shared<DoricoActionsController>(iocContext());
    m_modeState = std::make_shared<DoricoModeState>();
    m_noteInputController = std::make_shared<DoricoNoteInputController>(iocContext());
    m_popoverController = std::make_shared<PopoverController>(iocContext());

    ioc()->registerExport<IDoricoModeState>(mname, m_modeState);
    ioc()->registerExport<IPopoverController>(mname, m_popoverController);
    ioc()->registerExport<IDoricoMenuArranger>(mname, std::make_shared<DoricoMenuArranger>(iocContext()));
}

void DoricoContext::resolveImports()
{
    auto cs = ioc()->resolve<muse::rcommand::ICommandsState>(mname);
    if (cs) {
        cs->reg(std::make_shared<DoricoCommandsState>(iocContext()));
    }
}

void DoricoContext::onInit(const IApplication::RunMode&)
{
    m_actionsController->init();
    m_noteInputController->init();
    m_popoverController->init();
}
