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


#include "doricotoolsmodel.h"

#include "translation.h"

using namespace mu::dorico;
using namespace muse::uicomponents;

DoricoToolsModel::DoricoToolsModel(QObject* parent)
    : AbstractMenuModel(parent)
{
}

void DoricoToolsModel::setCodes(const QStringList& codes)
{
    if (m_codes == codes) {
        return;
    }
    m_codes = codes;
    emit codesChanged();

    if (!items().empty()) {
        load();
    }
}

void DoricoToolsModel::load()
{
    AbstractMenuModel::load();

    MenuItemList list;
    for (const QString& code : m_codes) {
        MenuItem* item = nullptr;
        if (code == QStringLiteral("-")) {
            item = makeSeparator();
        } else {
            item = makeMenuItem(muse::actions::ActionCode(code.toStdString()));
            if (!item) {
                item = new MenuItem(this);
                item->setId(code);
                item->setTitle(muse::TranslatableString::untranslatable(muse::String::fromQString(code.section('/', -1))));
                item->setDescription(muse::TranslatableString("dorico", "Not available yet"));
                item->setEnabled(false);
            }
        }
        list << item;
    }

    setItems(list);
}
