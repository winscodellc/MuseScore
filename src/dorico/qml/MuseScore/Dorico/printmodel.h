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

#include <QAbstractListModel>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "context/iglobalcontext.h"
#include "print/iprintprovider.h"
#include "project/iexportprojectscenario.h"
#include "notation/inotation.h"
#include "notation/iexcerptnotation.h"

namespace mu::dorico {
//! Backs Print mode (spec 58): the layouts (full score + each part) with a "print this" check box, and the
//! Print / Export actions for the checked ones. Think of a QAbstractListModel as a read-only list adapter that
//! QML's ListView/Repeater observe (like a RecyclerView adapter); the roles are the per-row fields.
class PrintModel : public QAbstractListModel, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(bool hasChecked READ hasChecked NOTIFY checkedChanged)

    QML_ELEMENT

    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<print::IPrintProvider> printProvider = { this };
    muse::ContextInject<project::IExportProjectScenario> exportScenario = { this };

public:
    explicit PrintModel(QObject* parent = nullptr);

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool hasChecked() const;

    Q_INVOKABLE void load();
    Q_INVOKABLE void setChecked(int row, bool checked);

    //! One system print dialog per checked layout (MuseScore prints one notation at a time).
    Q_INVOKABLE void print();
    //! `suffix` is "pdf", "png" or "svg"; asks for the file name, then writes the checked layouts.
    Q_INVOKABLE void exportTo(const QString& suffix, bool filePerLayout);

signals:
    void checkedChanged();

private:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        InfoRole,
        CheckedRole
    };

    struct Layout {
        QString title;
        notation::INotationPtr notation;
        notation::IExcerptNotationPtr excerpt; // null for the full score
        bool checked = false;
    };

    //! `keepChecks`: keep the checked layouts (a refresh) instead of checking only the current one (page shown).
    void reload(bool keepChecks);
    //! Creates the checked parts that were never opened (they have no score to print yet), then returns them all.
    notation::INotationPtrList checkedNotations();

    std::vector<Layout> m_layouts;
};
}
