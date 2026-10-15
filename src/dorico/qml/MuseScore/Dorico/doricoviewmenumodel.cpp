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


#include "doricoviewmenumodel.h"

#include "notationscene/notationcommands.h"

using namespace mu::dorico;
using namespace mu::notation;
using namespace muse::uicomponents;

DoricoViewMenuModel::DoricoViewMenuModel(QObject* parent)
    : AbstractMenuModel(parent)
{
}

void DoricoViewMenuModel::load()
{
    AbstractMenuModel::load();

    setItems({
        makeMenuItem(SHOW_INVISIBLE_COMMAND),
        makeMenuItem(SHOW_UNPRINTABLE_COMMAND),
        makeMenuItem(SHOW_FRAMES_COMMAND),
        makeMenuItem(SHOW_PAGEBORDERS_COMMAND),
        makeMenuItem(SHOW_SOUNDFLAGS_COMMAND),
        makeMenuItem(SHOW_IRREGULAR_COMMAND),
    });
}
