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
#include "context/iglobalcontext.h"

namespace mu::dorico {
//! Backs the selection summary in DoricoStatusBar.qml. Fills a SelectionInfo from the current selection and
//! hands it to summarise() (the pure, tested formatter).
class DoricoStatusModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(QString summary READ summary NOTIFY summaryChanged)

    QML_ELEMENT

    muse::ContextInject<context::IGlobalContext> globalContext = { this };

public:
    explicit DoricoStatusModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();

    QString summary() const { return m_summary; }

signals:
    void summaryChanged();

private:
    void subscribeToSelection();
    void update();

    QString m_summary;
    notation::INotationPtr m_notation;
};
}
