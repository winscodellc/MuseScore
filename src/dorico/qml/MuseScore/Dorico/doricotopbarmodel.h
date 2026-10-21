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
#include <QStringList>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "interactive/iinteractive.h"
#include "rcommand/icommanddispatcher.h"
#include "context/iglobalcontext.h"
#include "notation/inotation.h"

#include "../../../idoricomodestate.h"

namespace mu::dorico {
//! Backs DoricoTopBar.qml: active mode, the layout (score / part) selector, and two dispatch helpers.
//! A Q_PROPERTY with a NOTIFY signal is the observer pattern: QML re-reads the value whenever the signal fires.
class DoricoTopBarModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(int currentMode READ currentMode NOTIFY currentModeChanged)
    Q_PROPERTY(QStringList layoutTitles READ layoutTitles NOTIFY layoutsChanged)
    Q_PROPERTY(int currentLayoutIndex READ currentLayoutIndex NOTIFY layoutsChanged)
    // Parallel to layoutTitles: page size / space size per layout, empty for parts that were never opened (spec 55).
    Q_PROPERTY(QStringList layoutInfos READ layoutInfos NOTIFY layoutsChanged)
    Q_PROPERTY(QString projectTitle READ projectTitle NOTIFY layoutsChanged)

    QML_ELEMENT

    muse::ContextInject<muse::IInteractive> interactive = { this };
    muse::ContextInject<muse::rcommand::ICommandDispatcher> dispatcher = { this };
    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<IDoricoModeState> modes = { this };

public:
    explicit DoricoTopBarModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();

    int currentMode() const;
    QStringList layoutTitles() const;
    int currentLayoutIndex() const;
    QStringList layoutInfos() const;
    QString projectTitle() const;

    Q_INVOKABLE void selectMode(int mode);
    Q_INVOKABLE void selectLayout(int index);
    //! Deletes a part layout, including an instrument's part (which the Parts dialog cannot delete). Undoable.
    Q_INVOKABLE void deleteLayout(int index);
    Q_INVOKABLE void dispatch(const QString& command);
    Q_INVOKABLE void openPage(const QString& uri);

    // A mode's zone size (spec 95): width of the left/right zone, height of the lower zone (zone: 0 left, 1 lower,
    // 2 right). The page saves it when a mode is left and puts it back when the mode is shown.
    Q_INVOKABLE int zoneSize(int mode, int zone) const;
    Q_INVOKABLE void setZoneSize(int mode, int zone, int size);

signals:
    void currentModeChanged();
    void modeAboutToChange();
    void layoutsChanged();

private:
    void updateLayouts();
    void doDeleteLayout(const notation::IExcerptNotationPtr& excerpt);

    // Index 0 is the full score, the rest are the parts (opened or not), in the Parts dialog order.
    std::vector<notation::INotationPtr> m_layouts;
    // Non-null for entries that are parts: selecting one must make sure the part is open first.
    std::vector<notation::IExcerptNotationPtr> m_excerpts;
    QStringList m_titles;
    QStringList m_infos;
    int m_currentLayout = 0;
};
}
