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

#include <QObject>
#include <QString>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"

#include "../../../idoricomodestate.h"

namespace mu::dorico {
//! Selected tab of the Write-mode lower zone (spec 54). The tab lives in IDoricoModeState so the key/menu redirect
//! in DoricoActionsController can change it; this model mirrors it for QML and remembers it across restarts.
class LowerZoneModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(QString currentTab READ currentTab WRITE setCurrentTab NOTIFY currentTabChanged)

    QML_ELEMENT

    muse::ContextInject<IDoricoModeState> modes = { this };

public:
    explicit LowerZoneModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();

    QString currentTab() const;
    void setCurrentTab(const QString& tab);

signals:
    void currentTabChanged();
};
}
