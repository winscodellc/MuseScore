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

#include "printmodel.h"

#include <set>

#include "engraving/dom/mscore.h"
#include "notation/imasternotation.h"
#include "notation/iexcerptnotation.h"
#include "notation/inotationpainting.h"
#include "notation/inotationstyle.h"
#include "project/internal/exporttype.h"

#include "../../../internal/doricoshell.h"

using namespace mu::dorico;
using namespace mu::notation;

PrintModel::PrintModel(QObject* parent)
    : QAbstractListModel(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void PrintModel::load()
{
    reload(/*keepChecks*/ false);
}

void PrintModel::reload(bool keepChecks)
{
    std::set<INotationPtr> checked;
    for (const Layout& layout : m_layouts) {
        if (layout.checked) {
            checked.insert(layout.notation);
        }
    }

    beginResetModel();
    m_layouts.clear();

    IMasterNotationPtr master = globalContext()->currentMasterNotation();
    if (master) {
        const INotationPtr current = globalContext()->currentNotation();
        auto isChecked = [&](const INotationPtr& notation) {
            return keepChecks ? checked.count(notation) > 0 : notation == current;
        };

        m_layouts.push_back({ muse::qtrc("dorico", "Full score"), master->notation(), nullptr, isChecked(master->notation()) });

        // Every layout, as in Dorico and the Setup Layouts panel: also the parts that were never opened (MuseScore's
        // potential excerpts). checkedNotations() creates those before printing.
        ExcerptNotationList excerpts = master->excerpts();
        const ExcerptNotationList potential = master->potentialExcerpts();
        excerpts.insert(excerpts.end(), potential.begin(), potential.end());
        master->sortExcerpts(excerpts);
        for (const IExcerptNotationPtr& excerpt : excerpts) {
            m_layouts.push_back({ excerpt->name(), excerpt->notation(), excerpt, isChecked(excerpt->notation()) });
        }

        master->excerptsChanged().onNotify(this, [this]() {
            reload(/*keepChecks*/ true);
        }, Asyncable::Mode::SetReplace);
    }

    endResetModel();
    emit checkedChanged();
}

QVariant PrintModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(m_layouts.size())) {
        return QVariant();
    }

    const Layout& layout = m_layouts[index.row()];
    switch (role) {
    case TitleRole:
        return layout.title;
    case CheckedRole:
        return layout.checked;
    case InfoRole: {
        // A part that was never opened has no score yet, so it has no style to read.
        if (layout.excerpt && !layout.excerpt->isInited()) {
            return QString();
        }
        const INotationStylePtr style = layout.notation->style();
        const int pages = layout.notation->painting()->pageCount();
        const QString size = layoutPageInfo(style->styleValue(engraving::Sid::pageWidth).toDouble(),
                                            style->styleValue(engraving::Sid::pageHeight).toDouble(),
                                            style->styleValue(engraving::Sid::spatium).toDouble() / engraving::DPI * 72.0);
        // A part that has not been laid out yet reports 0 pages: show only its size.
        return pages > 0 ? muse::qtrc("dorico", "%n page(s)", nullptr, pages) + ", " + size : size;
    }
    }

    return QVariant();
}

int PrintModel::rowCount(const QModelIndex&) const
{
    return static_cast<int>(m_layouts.size());
}

QHash<int, QByteArray> PrintModel::roleNames() const
{
    return {
        { TitleRole, "title" },
        { InfoRole, "info" },
        { CheckedRole, "checked" }
    };
}

bool PrintModel::hasChecked() const
{
    return std::any_of(m_layouts.begin(), m_layouts.end(), [](const Layout& l) { return l.checked; });
}

void PrintModel::setChecked(int row, bool checked)
{
    if (row < 0 || row >= static_cast<int>(m_layouts.size()) || m_layouts[row].checked == checked) {
        return;
    }

    m_layouts[row].checked = checked;
    emit dataChanged(index(row), index(row), { CheckedRole });
    emit checkedChanged();
}

INotationPtrList PrintModel::checkedNotations()
{
    INotationPtrList result;
    ExcerptNotationList toCreate;
    for (const Layout& layout : m_layouts) {
        if (!layout.checked) {
            continue;
        }
        result.push_back(layout.notation);
        if (layout.excerpt && !layout.excerpt->isInited()) {
            toCreate.push_back(layout.excerpt);
        }
    }

    // Same as opening the part from the layout selector (DoricoTopBarModel::selectLayout), without showing it.
    // setExcerpts triggers excerptsChanged and so a reload, which is why the result is collected first.
    IMasterNotationPtr master = globalContext()->currentMasterNotation();
    if (master && !toCreate.empty()) {
        ExcerptNotationList excerpts = master->excerpts();
        excerpts.insert(excerpts.end(), toCreate.begin(), toCreate.end());
        master->setExcerpts(excerpts);
    }

    return result;
}

void PrintModel::print()
{
    for (const INotationPtr& notation : checkedNotations()) {
        printProvider()->printNotation(notation);
    }
}

void PrintModel::exportTo(const QString& suffix, bool filePerLayout)
{
    const INotationPtrList notations = checkedNotations();
    if (notations.empty()) {
        return;
    }

    const project::ExportType type = project::ExportType::makeWithSuffixes({ suffix }, suffix.toUpper(), suffix.toUpper());
    const auto unitType = exportUnitType(suffix, filePerLayout);

    const muse::RetVal<muse::io::path_t> path = exportScenario()->askExportPath(notations, type, unitType);
    if (!path.ret) {
        return;
    }

    exportScenario()->exportScores(notations, path.val, unitType, /*openDestinationFolderOnExport*/ true);
}
