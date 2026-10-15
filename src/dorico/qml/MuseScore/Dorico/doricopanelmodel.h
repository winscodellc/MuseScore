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

#include <QAbstractItemModel>
#include <QObject>
#include <QSize>
#include <QStringList>
#include <qqmlintegration.h>

#include "modularity/ioc.h"
#include "dockwindow/idockwindowprovider.h"

#include "palette/internal/palettemodel.h"
#include "palette/internal/paletteprovider.h"

namespace mu::dorico {
//! One Dorico Notations panel (spec 53): its sections as a small palette tree built from MuseScore's master palette
//! cells (table in internal/notationspanels.cpp). QML shows each section with MuseScore's own PaletteGridView, and a
//! click goes through UserPaletteController::applyPaletteElement, exactly like clicking the palette (undoable).
class DoricoPanelModel : public QObject, public muse::Contextable
{
    Q_OBJECT

    Q_PROPERTY(QString panelId READ panelId WRITE setPanelId NOTIFY panelChanged)
    Q_PROPERTY(QStringList sectionTitles READ sectionTitles NOTIFY panelChanged)
    Q_PROPERTY(QAbstractItemModel * paletteModel READ paletteModel NOTIFY panelChanged)
    Q_PROPERTY(mu::palette::AbstractPaletteController * paletteController READ paletteController NOTIFY panelChanged)

    QML_ELEMENT

    muse::ContextInject<muse::dock::IDockWindowProvider> dockWindowProvider = { this };

public:
    explicit DoricoPanelModel(QObject* parent = nullptr);

    QString panelId() const { return m_panelId; }
    void setPanelId(const QString& id);

    QStringList sectionTitles() const { return m_titles; }
    QAbstractItemModel* paletteModel() const { return m_model; }
    mu::palette::AbstractPaletteController* paletteController() const { return m_controller; }

    Q_INVOKABLE QModelIndex sectionIndex(int section) const;
    Q_INVOKABLE QSize cellSize(int section) const;
    Q_INVOKABLE bool drawGrid(int section) const;

    //! Opens or closes the right-zone dock that shows the panel.
    Q_INVOKABLE void setPanelOpen(bool open);
    Q_INVOKABLE bool isPanelOpen() const;

signals:
    void panelChanged();

private:
    void rebuild();

    QString m_panelId;
    QStringList m_titles;
    palette::PaletteTreePtr m_tree;
    palette::PaletteTreeModel* m_model = nullptr;
    palette::UserPaletteController* m_controller = nullptr;
};
}
