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
#include "rcommand/icommandsregister.h"
#include "rcommand/icommandsstate.h"

#include "internal/doricoactionscontroller.h"
#include "internal/doricocommandsregister.h"
#include "internal/doricocommandsstate.h"

#include "log.h"

using namespace mu::dorico;
using namespace muse;
using namespace muse::modularity;

static const std::string mname("dorico");

std::string DoricoModule::moduleName() const
{
    return mname;
}

void DoricoModule::resolveImports()
{
    auto cr = globalIoc()->resolve<muse::rcommand::ICommandsRegister>(mname);
    if (cr) {
        cr->reg(std::make_shared<DoricoCommandsRegister>());
    }
}

void DoricoModule::onInit(const IApplication::RunMode&)
{
    LOGI() << "dorico module init";
}

IContextSetup* DoricoModule::newContext(const muse::modularity::ContextPtr& ctx) const
{
    return new DoricoContext(ctx);
}

void DoricoContext::registerExports()
{
    m_actionsController = std::make_shared<DoricoActionsController>(iocContext());
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
}
