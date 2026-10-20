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
#include "ui/iuiconfiguration.h"

#include "internal/doricoactionscontroller.h"
#include "internal/doricocaret.h"
#include "internal/doricocommandsregister.h"
#include "internal/doricocommandsstate.h"
#include "internal/doricodurationscontroller.h"
#include "internal/doricomenuarranger.h"
#include "internal/doricomodestate.h"
#include "internal/doricoshell.h"
#include "internal/doriconoteinputcontroller.h"
#include "internal/doricopastecontroller.h"
#include "internal/doricoselectioncontroller.h"
#include "internal/popovercontroller.h"

#include "log.h"

using namespace mu::dorico;
using namespace muse;
using namespace muse::modularity;

static const std::string mname("dorico");
static const Settings::Key KEYMAP_APPLIED_KEY(mname, "dorico/keymapApplied");
static const Settings::Key APPEARANCE_VERSION_KEY(mname, "dorico/appearanceVersion");

// Spec 60: bump to re-apply after the colors below are re-tuned.
static constexpr int APPEARANCE_VERSION = 2;

//! Dorico's music-area colors (sampled from the Dorico 6.2 manual screenshots), written over the notation and
//! engraving settings once, so Preferences can still change them afterwards. The UI colors are in app/configs/*.cfg.
static void applyDoricoAppearance()
{
    const QColor background("#2C4D6F"); // middle of Dorico 6's slate Write-mode gradient (#203F62 .. #375B7B)
    const QColor page("#FFFFFF");
    const QColor selection("#FF7D00"); // Dorico selects in orange whatever the voice

    for (const char* key : { "ui/canvas/background/darkTheme_score_background_color",
                             "ui/canvas/background/lightTheme_score_background_color" }) {
        settings()->setSharedValue(Settings::Key("notation", key), Val(background));
    }
    settings()->setSharedValue(Settings::Key("notation", "ui/canvas/background/useColor"), Val(true));
    settings()->setSharedValue(Settings::Key("notation", "ui/canvas/foreground/useColor"), Val(true));
    settings()->setSharedValue(Settings::Key("notation", "ui/canvas/foreground/color"), Val(page));

    for (const char* key : { "engraving/colors/voice1", "engraving/colors/voice2", "engraving/colors/voice3",
                             "engraving/colors/voice4", "engraving/colors/allVoicesColor" }) {
        settings()->setSharedValue(Settings::Key("engraving", key), Val(selection));
    }

    // Dorico's default theme is Dark.
    auto uiConfig = globalIoc()->resolve<muse::ui::IUiConfiguration>(mname);
    if (uiConfig) {
        uiConfig->setCurrentTheme(muse::ui::DARK_THEME_CODE);
    }
}

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

    settings()->setDefaultValue(APPEARANCE_VERSION_KEY, Val(0));
    if (settings()->value(APPEARANCE_VERSION_KEY).toInt() < APPEARANCE_VERSION) {
        applyDoricoAppearance();
        settings()->setSharedValue(APPEARANCE_VERSION_KEY, Val(APPEARANCE_VERSION));
    }
}

IContextSetup* DoricoModule::newContext(const muse::modularity::ContextPtr& ctx) const
{
    return new DoricoContext(ctx);
}

void DoricoContext::registerExports()
{
    m_actionsController = std::make_shared<DoricoActionsController>(iocContext());
    m_caret = std::make_shared<DoricoCaret>(iocContext());
    m_durationsController = std::make_shared<DoricoDurationsController>(iocContext());
    m_modeState = std::make_shared<DoricoModeState>();
    m_noteInputController = std::make_shared<DoricoNoteInputController>(iocContext());
    m_pasteController = std::make_shared<DoricoPasteController>(iocContext());
    m_selectionController = std::make_shared<DoricoSelectionController>(iocContext());
    m_popoverController = std::make_shared<PopoverController>(iocContext());

    ioc()->registerExport<IDoricoModeState>(mname, m_modeState);
    ioc()->registerExport<IDoricoCaret>(mname, m_caret);
    ioc()->registerExport<IDoricoPaste>(mname, m_pasteController);
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
    m_caret->init();
    m_durationsController->init();
    m_noteInputController->init();
    m_pasteController->init();
    m_selectionController->init();
    m_popoverController->init();
}
