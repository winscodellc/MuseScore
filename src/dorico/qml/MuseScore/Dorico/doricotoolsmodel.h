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

#include <QStringList>
#include <qqmlintegration.h>

#include "uicomponents/qml/Muse/UiComponents/abstractmenumodel.h"

namespace mu::dorico {
//! A row/column of tool buttons for the Dorico toolboxes and panels. QML sets `codes` to a list of
//! "command://..." URIs or legacy action codes; each becomes a MenuItem whose `enabled`/`checked` follow the
//! command state live (the same mechanism NoteInputBarModel uses). A code that is not registered yet becomes a
//! disabled placeholder, so a button can be listed before the feature behind it exists.
class DoricoToolsModel : public muse::uicomponents::AbstractMenuModel
{
    Q_OBJECT

    Q_PROPERTY(QStringList codes READ codes WRITE setCodes NOTIFY codesChanged)

    QML_ELEMENT

public:
    explicit DoricoToolsModel(QObject* parent = nullptr);

    QStringList codes() const { return m_codes; }
    void setCodes(const QStringList& codes);

    Q_INVOKABLE void load() override;

signals:
    void codesChanged();

private:
    QStringList m_codes;
};
}
