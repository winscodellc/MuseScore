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
#include "notation/inotationconfiguration.h"
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
#include "internal/formatcontroller.h"
#include "internal/doricoselectioncontroller.h"
#include "internal/jumpbar/jumpbarcontroller.h"
#include "internal/modepolicy.h"
#include "internal/popovercontroller.h"
#include "internal/transformcontroller.h"

#include "log.h"

using namespace mu::dorico;
using namespace muse;
using namespace muse::modularity;

static const std::string mname("dorico");
static const Settings::Key KEYMAP_APPLIED_KEY(mname, "dorico/keymapApplied");
static const Settings::Key APPEARANCE_VERSION_KEY(mname, "dorico/appearanceVersion");
static const Settings::Key CANVAS_ORIENTATION_APPLIED_KEY(mname, "dorico/canvasOrientationApplied");

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
    // The modes all live in musescore://notation (registered by appshell, spec 95); these are our dialogs.
    auto ir = globalIoc()->resolve<muse::interactive::IInteractiveUriRegister>(mname);
    if (ir) {
        ir->registerQmlUri(muse::Uri("musescore://dorico/filternotesbypitch"), "MuseScore.Dorico", "FilterNotesByPitchDialog"); // spec 45
        ir->registerQmlUri(muse::Uri("musescore://dorico/transforminvert"), "MuseScore.Dorico", "InvertPitchesDialog"); // spec 65
        ir->registerQmlUri(muse::Uri("musescore://dorico/transformrotate"), "MuseScore.Dorico", "RotateDialog");
        ir->registerQmlUri(muse::Uri("musescore://dorico/preferences"), "MuseScore.Dorico", "DoricoPreferencesDialog"); // spec 100
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

    // Spec 101 (M2): Dorico lays its pages out in a vertical column. Once, so Preferences can still change it. Not part
    // of the appearance version: bumping that would overwrite the colors the user has set since.
    if (!settings()->value(CANVAS_ORIENTATION_APPLIED_KEY).toBool()) {
        auto notationConfig = globalIoc()->resolve<mu::notation::INotationConfiguration>(mname);
        if (notationConfig) {
            notationConfig->setCanvasOrientation(muse::Orientation::Vertical);
        }
        settings()->setSharedValue(CANVAS_ORIENTATION_APPLIED_KEY, Val(true));
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
    m_formatController = std::make_shared<DoricoFormatController>(iocContext());
    m_pasteController = std::make_shared<DoricoPasteController>(iocContext());
    m_selectionController = std::make_shared<DoricoSelectionController>(iocContext());
    m_transformController = std::make_shared<DoricoTransformController>(iocContext());
    m_popoverController = std::make_shared<PopoverController>(iocContext());
    m_jumpBarController = std::make_shared<JumpBarController>(iocContext());

    ioc()->registerExport<IDoricoModeState>(mname, m_modeState);
    m_modeGate = std::make_shared<ModeCommandsGate>(iocContext());
    ioc()->registerExport<mu::notation::INotationCommandsGate>(mname, m_modeGate);
    ioc()->registerExport<IDoricoCaret>(mname, m_caret);
    ioc()->registerExport<IDoricoPaste>(mname, m_pasteController);
    ioc()->registerExport<IPopoverController>(mname, m_popoverController);
    ioc()->registerExport<IJumpBarController>(mname, m_jumpBarController);
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
    m_modeGate->init(); // first: its preDispatch veto must run before the other controllers' hooks
    m_actionsController->init();
    m_caret->init();
    m_durationsController->init();
    m_noteInputController->init();
    m_formatController->init();
    m_pasteController->init();
    m_selectionController->init();
    m_transformController->init();
    m_popoverController->init();
    m_jumpBarController->init();
}
