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

#include <map>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "context/iglobalcontext.h"
#include "print/iprintprovider.h"
#include "project/iexportprojectscenario.h"
#include "notation/inotation.h"
#include "notation/iexcerptnotation.h"
#include "interactive/iinteractive.h"
#include "interactive/iplatforminteractive.h"

#include "../../../internal/printjob.h"

namespace mu::dorico {
//! Backs Print mode (spec 58): the layouts (full score + each part) with a "print this" check box, and the
//! Print / Export actions for the checked ones. Think of a QAbstractListModel as a read-only list adapter that
//! QML's ListView/Repeater observe (like a RecyclerView adapter); the roles are the per-row fields.
class PrintModel : public QAbstractListModel, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(bool hasChecked READ hasChecked NOTIFY checkedChanged)

    //! The Print Options values the checked layouts share (spec 62). A key whose layouts disagree is left out, which QML
    //! reads as `undefined` (a blank field); editing it sets it on all of them.
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY settingsChanged)
    Q_PROPERTY(QStringList printers READ printers CONSTANT)
    Q_PROPERTY(QString defaultPrinter READ defaultPrinter CONSTANT)
    //! `[{ key, name }]` for the printer of the first checked layout.
    Q_PROPERTY(QVariantList paperSizes READ paperSizes NOTIFY settingsChanged)
    Q_PROPERTY(bool autoDuplexAvailable READ autoDuplexAvailable NOTIFY settingsChanged)
    //! The first checked layout's pages are wider than high (what Auto orientation follows for the Normal job type).
    Q_PROPERTY(bool landscapeLayout READ landscapeLayout NOTIFY settingsChanged)
    Q_PROPERTY(bool anyPrinter READ anyPrinter NOTIFY settingsChanged)
    Q_PROPERTY(bool anyGraphics READ anyGraphics NOTIFY settingsChanged)
    //! False while a checked layout has a Pages range that does not parse.
    Q_PROPERTY(bool pagesValid READ pagesValid NOTIFY settingsChanged)
    //! Print, Export or Print and Export, by the checked layouts' destinations.
    Q_PROPERTY(QString buttonLabel READ buttonLabel NOTIFY settingsChanged)

    QML_ELEMENT

    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<print::IPrintProvider> printProvider = { this };
    muse::ContextInject<project::IExportProjectScenario> exportScenario = { this };
    muse::ContextInject<muse::IInteractive> interactive = { this };
    muse::GlobalInject<muse::IPlatformInteractive> platformInteractive;

public:
    explicit PrintModel(QObject* parent = nullptr);

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool hasChecked() const;

    QVariantMap settings() const;
    QStringList printers() const;
    QString defaultPrinter() const;
    QVariantList paperSizes() const;
    bool autoDuplexAvailable() const;
    bool landscapeLayout() const;
    bool anyPrinter() const;
    bool anyGraphics() const;
    bool pagesValid() const;
    QString buttonLabel() const;

    Q_INVOKABLE void load();
    Q_INVOKABLE void setChecked(int row, bool checked);

    //! Sets one Print Options value on every checked layout and saves it in the project.
    Q_INVOKABLE void setSetting(const QString& key, const QVariant& value);
    //! Print / Export with the panel's settings, each checked layout with its own (no system dialog).
    Q_INVOKABLE void run();

    //! One system print dialog per checked layout (MuseScore prints one notation at a time).
    Q_INVOKABLE void print();

signals:
    void checkedChanged();
    void settingsChanged();

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
        printjob::PrintSettings settings;
    };

    struct Job {
        notation::INotationPtr notation;
        printjob::PrintSettings settings;
    };

    //! `keepChecks`: keep the checked layouts (a refresh) instead of checking only the current one (page shown).
    void reload(bool keepChecks);
    //! Creates the checked parts that were never opened (they have no score to print yet), then returns them all.
    notation::INotationPtrList checkedNotations();
    //! checkedNotations() with each layout's settings.
    std::vector<Job> checkedJobs();

    printjob::PrintSettings readSettings(const Layout& layout) const;
    void writeSettings(const Layout& layout) const;
    const Layout* firstChecked() const;

    void runPrinterJob(const Job& job);
    void runPdfJobs(const std::vector<Job>& jobs);
    //! PNG and SVG keep the spec 58 export (one file per page, no range or job type).
    void runImageJobs(const QString& suffix, const std::vector<Job>& jobs);
    void showError(const QString& text) const;

    std::vector<Layout> m_layouts;
};
}
