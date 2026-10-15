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


#include "doricotopbarmodel.h"

#include <QTextDocumentFragment>

#include "containers.h"
#include "log.h"
#include "translation.h"

#include "notation/imasternotation.h"
#include "notation/iexcerptnotation.h"
#include "notation/inotationstyle.h"
#include "notation/inotationundostack.h"
#include "engraving/dom/masterscore.h"
#include "engraving/editing/editscoreproperties.h"
#include "engraving/dom/mscore.h"
#include "project/inotationproject.h"

#include "../../../doricocommands.h"
#include "../../../internal/doricoshell.h"
#include "../../../internal/partlayouts.h"

using namespace mu::dorico;
using namespace mu::notation;

DoricoTopBarModel::DoricoTopBarModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void DoricoTopBarModel::load()
{
    modes()->modeChanged().onNotify(this, [this]() {
        emit currentModeChanged();
    });

    globalContext()->currentNotationChanged().onNotify(this, [this]() {
        updateLayouts();
    });
    globalContext()->currentProjectChanged().onNotify(this, [this]() {
        updateLayouts();
    });

    updateLayouts();
}

int DoricoTopBarModel::currentMode() const
{
    return static_cast<int>(modes()->mode());
}

QStringList DoricoTopBarModel::layoutTitles() const
{
    return m_titles;
}

int DoricoTopBarModel::currentLayoutIndex() const
{
    return m_currentLayout;
}

QStringList DoricoTopBarModel::layoutInfos() const
{
    return m_infos;
}

QString DoricoTopBarModel::projectTitle() const
{
    const auto project = globalContext()->currentProject();
    // An unsaved / imported project is named after its work title, which can be rich text ("<font face=...>").
    return project ? QTextDocumentFragment::fromHtml(project->displayName()).toPlainText() : QString();
}

void DoricoTopBarModel::selectMode(int mode)
{
    if (mode < 0 || mode >= MODE_COUNT) {
        return;
    }
    dispatch(QString::fromStdString(DORICO_MODE_COMMANDS[mode].toString()));
}

void DoricoTopBarModel::dispatch(const QString& command)
{
    dispatcher()->dispatch(muse::rcommand::Command(command.toStdString()));
}

void DoricoTopBarModel::openPage(const QString& uri)
{
    interactive()->open(uri.toStdString());
}

void DoricoTopBarModel::selectLayout(int index)
{
    IMasterNotationPtr master = globalContext()->currentMasterNotation();
    if (!master || index < 0 || index >= static_cast<int>(m_layouts.size())) {
        return;
    }

    // Same sequence as PartListModel::openExcerpts: a part has to be in the master's excerpt list and open
    // before it can be shown.
    const IExcerptNotationPtr& excerpt = m_excerpts[index];
    if (excerpt && !excerpt->notation()->isOpen()) {
        ExcerptNotationList excerpts = master->excerpts();
        size_t idx = muse::indexOf(excerpts, excerpt);
        if (idx == muse::nidx) {
            excerpts.push_back(excerpt);
        } else {
            muse::moveItem(excerpts, idx, excerpts.size());
        }
        master->setExcerpts(excerpts);
        master->setExcerptIsOpen(excerpt->notation(), true);
    }

    globalContext()->setCurrentNotation(m_layouts[index]);
}

void DoricoTopBarModel::deleteLayout(int index)
{
    if (index <= 0 || index >= static_cast<int>(m_excerpts.size())) {
        return;
    }

    // Copied, not referenced: the list is rebuilt while the question is open if the score changes.
    const IExcerptNotationPtr excerpt = m_excerpts[index];
    if (excerpt->isEmpty()) {
        doDeleteLayout(excerpt);
        return;
    }

    interactive()->question("", muse::trc("notation", "Are you sure you want to delete this part?"), {
        muse::IInteractive::Button::Yes, muse::IInteractive::Button::No
    })
    .onResolve(this, [this, excerpt](const muse::IInteractive::Result& res) {
        if (res.isButton(muse::IInteractive::Button::Yes)) {
            doDeleteLayout(excerpt);
        }
    });
}

void DoricoTopBarModel::doDeleteLayout(const IExcerptNotationPtr& excerpt)
{
    IMasterNotationPtr master = globalContext()->currentMasterNotation();
    if (!master) {
        return;
    }

    const muse::ID partId = excerpt->initialPartId();
    INotationUndoStackPtr undoStack = master->notation()->undoStack();
    const size_t startState = undoStack->currentStateIndex();

    if (globalContext()->currentNotation() == excerpt->notation()) {
        globalContext()->setCurrentNotation(master->notation());
    }

    // Removes an opened part (an unopened instrument part is not in the list yet). Same as PartListModel::doRemovePart.
    ExcerptNotationList excerpts = master->excerpts();
    if (muse::remove(excerpts, excerpt)) {
        master->setExcerpts(excerpts);
    }

    if (!partId.isValid()) {
        return; // a custom part: removing it was enough
    }

    // MuseScore regenerates a part for every instrument without one, so remember that this one was deleted.
    mu::engraving::MasterScore* score = master->masterScore();
    std::map<muse::String, muse::String> tags = score->metaTags();
    tags[DELETED_PART_LAYOUTS_TAG] = addDeletedPartLayout(score->metaTag(DELETED_PART_LAYOUTS_TAG),
                                                          muse::String::fromStdString(partId.toStdString()));

    undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Delete part layout"));
    score->undo(new mu::engraving::ChangeMetaTags(score, tags));
    undoStack->commitChanges();

    // One undo step brings back an opened part together with its content.
    if (undoStack->currentStateIndex() > startState + 1) {
        undoStack->mergeTransactions(startState);
    }

    // No excerpt changed when the part was never opened, so nothing else triggers a refresh.
    updateLayouts();
}

void DoricoTopBarModel::updateLayouts()
{
    m_layouts.clear();
    m_excerpts.clear();
    m_titles.clear();
    m_infos.clear();
    m_currentLayout = 0;

    IMasterNotationPtr master = globalContext()->currentMasterNotation();
    if (master) {
        m_layouts.push_back(master->notation());
        m_excerpts.push_back(nullptr);
        m_titles << QString("Score");

        ExcerptNotationList excerpts = master->excerpts();
        const ExcerptNotationList potential = master->potentialExcerpts();
        excerpts.insert(excerpts.end(), potential.begin(), potential.end());
        master->sortExcerpts(excerpts);

        for (const IExcerptNotationPtr& excerpt : excerpts) {
            m_layouts.push_back(excerpt->notation());
            m_excerpts.push_back(excerpt);
            m_titles << excerpt->name();
        }

        const INotationPtr current = globalContext()->currentNotation();
        for (size_t i = 0; i < m_layouts.size(); ++i) {
            if (m_layouts[i] == current) {
                m_currentLayout = static_cast<int>(i);
                break;
            }
        }

        for (size_t i = 0; i < m_layouts.size(); ++i) {
            // A part that was never opened has no score yet, so it has no style to read.
            const bool hasScore = !m_excerpts[i] || m_excerpts[i]->isInited();
            const INotationStylePtr style = hasScore ? m_layouts[i]->style() : nullptr;
            if (!style) {
                m_infos << QString();
                continue;
            }
            m_infos << layoutPageInfo(style->styleValue(mu::engraving::Sid::pageWidth).toDouble(),
                                      style->styleValue(mu::engraving::Sid::pageHeight).toDouble(),
                                      style->styleValue(mu::engraving::Sid::spatium).toDouble() / mu::engraving::DPI * 72.0);
            style->styleChanged().onNotify(this, [this]() {
                updateLayouts();
            }, Asyncable::Mode::SetReplace);
        }

        // Undoing a part layout deletion only changes a meta tag (see doDeleteLayout).
        master->notation()->undoStack()->undoRedoNotification().onNotify(this, [this]() {
            updateLayouts();
        }, Asyncable::Mode::SetReplace);

        // Parts are added, removed and renamed from the Setup Layouts panel and the Parts dialog.
        master->excerptsChanged().onNotify(this, [this]() {
            updateLayouts();
        }, Asyncable::Mode::SetReplace);
        for (const IExcerptNotationPtr& excerpt : excerpts) {
            excerpt->nameChanged().onNotify(this, [this]() {
                updateLayouts();
            }, Asyncable::Mode::SetReplace);
        }
    }

    emit layoutsChanged();
}
