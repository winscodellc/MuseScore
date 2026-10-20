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

#include "global/io/fileinfo.h"
#include "global/io/path.h"
#include "translation.h"

#include "engraving/dom/mscore.h"
#include "engraving/dom/score.h"
#include "notation/imasternotation.h"
#include "notation/iexcerptnotation.h"
#include "notation/inotationelements.h"
#include "notation/inotationpainting.h"
#include "notation/inotationstyle.h"
#include "project/inotationproject.h"
#include "project/internal/exporttype.h"

#include "doricoprinter.h"
#include "../../../internal/doricoshell.h"

using namespace mu::dorico;
using namespace mu::dorico::printjob;
using namespace mu::notation;

namespace {
// The settings live in a metaTag of the layout's own score, saved with the project. Score::metaTag falls back to the full
// score's tags, so the full score uses another key and a part never inherits its settings.
const muse::String PM_TAG_SCORE = u"doricoPrintSettingsScore";
const muse::String PM_TAG_PART = u"doricoPrintSettings";
}

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
    std::map<INotationPtr, PrintSettings> kept; // a refresh keeps edits of parts that have no score to save them in yet
    for (const Layout& layout : m_layouts) {
        if (layout.checked) {
            checked.insert(layout.notation);
        }
        kept[layout.notation] = layout.settings;
    }

    beginResetModel();
    m_layouts.clear();

    IMasterNotationPtr master = globalContext()->currentMasterNotation();
    if (master) {
        const INotationPtr current = globalContext()->currentNotation();
        auto isChecked = [&](const INotationPtr& notation) {
            return keepChecks ? checked.count(notation) > 0 : notation == current;
        };

        m_layouts.push_back({ muse::qtrc("dorico", "Full score"), master->notation(), nullptr, isChecked(master->notation()), PrintSettings() });

        // Every layout, as in Dorico and the Setup Layouts panel: also the parts that were never opened (MuseScore's
        // potential excerpts). checkedNotations() creates those before printing.
        ExcerptNotationList excerpts = master->excerpts();
        const ExcerptNotationList potential = master->potentialExcerpts();
        excerpts.insert(excerpts.end(), potential.begin(), potential.end());
        master->sortExcerpts(excerpts);
        for (const IExcerptNotationPtr& excerpt : excerpts) {
            m_layouts.push_back({ excerpt->name(), excerpt->notation(), excerpt, isChecked(excerpt->notation()), PrintSettings() });
        }

        for (Layout& layout : m_layouts) {
            const auto it = kept.find(layout.notation);
            layout.settings = keepChecks && it != kept.end() ? it->second : readSettings(layout);
        }

        master->excerptsChanged().onNotify(this, [this]() {
            reload(/*keepChecks*/ true);
        }, Asyncable::Mode::SetReplace);
    }

    endResetModel();
    emit checkedChanged();
    emit settingsChanged();
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
    emit settingsChanged();
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

// ---- Print Options settings (spec 62) --------------------------------------------------------------------------------

PrintSettings PrintModel::readSettings(const Layout& layout) const
{
    // A part that was never opened has no score yet; its settings start at the defaults.
    if (layout.excerpt && !layout.excerpt->isInited()) {
        return PrintSettings();
    }

    const mu::engraving::Score* score = layout.notation->elements()->msScore();
    if (!score) {
        return PrintSettings();
    }

    const QByteArray json = score->metaTag(layout.excerpt ? PM_TAG_PART : PM_TAG_SCORE).toQString().toUtf8();
    return PrintSettings::fromJson(json);
}

void PrintModel::writeSettings(const Layout& layout) const
{
    if (layout.excerpt && !layout.excerpt->isInited()) {
        return;
    }

    mu::engraving::Score* score = layout.notation->elements()->msScore();
    if (!score) {
        return;
    }

    score->setMetaTag(layout.excerpt ? PM_TAG_PART : PM_TAG_SCORE, muse::String::fromUtf8(layout.settings.toJson().constData()));

    // A tag change is not an undoable edit, so the project is told directly that it has something to save.
    if (project::INotationProjectPtr project = globalContext()->currentProject()) {
        project->markAsUnsaved();
    }
}

const PrintModel::Layout* PrintModel::firstChecked() const
{
    const auto it = std::find_if(m_layouts.begin(), m_layouts.end(), [](const Layout& l) { return l.checked; });
    return it != m_layouts.end() ? &*it : nullptr;
}

QVariantMap PrintModel::settings() const
{
    QVariantMap merged = PrintSettings().toMap();
    const Layout* first = firstChecked();
    if (!first) {
        return merged;
    }

    merged = first->settings.toMap();
    for (const Layout& layout : m_layouts) {
        if (!layout.checked) {
            continue;
        }
        const QVariantMap other = layout.settings.toMap();
        for (auto it = other.constBegin(); it != other.constEnd(); ++it) {
            if (merged.contains(it.key()) && merged.value(it.key()) != it.value()) {
                merged.remove(it.key());
            }
        }
    }
    return merged;
}

void PrintModel::setSetting(const QString& key, const QVariant& value)
{
    for (Layout& layout : m_layouts) {
        if (!layout.checked) {
            continue;
        }

        if (!layout.settings.set(key, value)) {
            LOGW() << "Unknown print setting " << key;
            return;
        }

        // A paper size the new printer does not offer falls back to the layout's own.
        if (key == "printerName" && !layout.settings.paperSize.isEmpty()) {
            const QVariantList sizes = DoricoPrinter::paperSizes(layout.settings.printerName);
            const bool offered = std::any_of(sizes.begin(), sizes.end(), [&](const QVariant& s) {
                return s.toMap().value("key").toString() == layout.settings.paperSize;
            });
            if (!offered) {
                layout.settings.paperSize.clear();
            }
        }

        writeSettings(layout);
    }

    emit settingsChanged();
}

QStringList PrintModel::printers() const
{
    return DoricoPrinter::printerNames();
}

QString PrintModel::defaultPrinter() const
{
    return DoricoPrinter::defaultPrinterName();
}

QVariantList PrintModel::paperSizes() const
{
    const Layout* first = firstChecked();
    return DoricoPrinter::paperSizes(first ? first->settings.printerName : QString());
}

bool PrintModel::autoDuplexAvailable() const
{
    const Layout* first = firstChecked();
    return DoricoPrinter::autoDuplexAvailable(first ? first->settings.printerName : QString());
}

bool PrintModel::landscapeLayout() const
{
    const Layout* first = firstChecked();
    if (!first || (first->excerpt && !first->excerpt->isInited())) {
        return false;
    }

    const muse::SizeF size = first->notation->painting()->pageSizeInch();
    return size.width() > size.height();
}

bool PrintModel::anyPrinter() const
{
    return std::any_of(m_layouts.begin(), m_layouts.end(), [](const Layout& l) { return l.checked && !l.settings.graphics; })
           || !hasChecked();
}

bool PrintModel::anyGraphics() const
{
    return std::any_of(m_layouts.begin(), m_layouts.end(), [](const Layout& l) { return l.checked && l.settings.graphics; });
}

bool PrintModel::pagesValid() const
{
    return std::all_of(m_layouts.begin(), m_layouts.end(), [](const Layout& l) {
        if (!l.checked || l.settings.range != RangeKind::Pages) {
            return true;
        }
        // A part that was never laid out reports no pages: accept any number, it is checked again when it prints.
        const int pages = l.excerpt && !l.excerpt->isInited() ? 0 : l.notation->painting()->pageCount();
        return parsePageRanges(l.settings.pagesText, pages > 0 ? pages : 9999).ok;
    });
}

QString PrintModel::buttonLabel() const
{
    std::vector<bool> graphics;
    for (const Layout& layout : m_layouts) {
        if (layout.checked) {
            graphics.push_back(layout.settings.graphics);
        }
    }

    switch (buttonKind(graphics)) {
    case 1: return muse::qtrc("dorico", "Export");
    case 2: return muse::qtrc("dorico", "Print and Export");
    default: return muse::qtrc("dorico", "Print");
    }
}

std::vector<PrintModel::Job> PrintModel::checkedJobs()
{
    std::vector<Job> jobs;
    for (const Layout& layout : m_layouts) {
        if (layout.checked) {
            jobs.push_back({ layout.notation, layout.settings });
        }
    }

    // Creates the parts that were never opened and reloads the list (the settings of every layout are kept).
    checkedNotations();

    // Now they have a score to keep their settings in.
    for (const Layout& layout : m_layouts) {
        if (layout.checked) {
            writeSettings(layout);
        }
    }

    return jobs;
}

void PrintModel::showError(const QString& text) const
{
    interactive()->errorSync(muse::trc("dorico", "Print"), text.toStdString());
}

void PrintModel::runPrinterJob(const Job& job)
{
    using Pass = DoricoPrinter::Pass;

    if (job.settings.printOn != PrintOn::BothManual) {
        const muse::Ret ret = DoricoPrinter::print(job.notation, job.settings, Pass::All);
        if (!ret) {
            showError(QString::fromStdString(ret.text()));
        }
        return;
    }

    // Manual duplex: the printer cannot flip, so the front sides print, the user turns the stack, the back sides print.
    muse::Ret ret = DoricoPrinter::print(job.notation, job.settings, Pass::Outward);
    if (ret) {
        interactive()->infoSync(muse::trc("dorico", "Both sides manually"),
                                muse::trc("dorico", "Turn the stack over and put it back in the printer, then click OK to print the other side."));
        ret = DoricoPrinter::print(job.notation, job.settings, Pass::Inward);
    }
    if (!ret) {
        showError(QString::fromStdString(ret.text()));
    }
}

void PrintModel::runPdfJobs(const std::vector<Job>& jobs)
{
    INotationPtrList notations;
    for (const Job& job : jobs) {
        notations.push_back(job.notation);
    }

    // Several layouts go in one file only when none of them asks for a file of its own.
    const bool combined = jobs.size() > 1 && std::none_of(jobs.begin(), jobs.end(), [](const Job& j) {
        return j.settings.filePerLayout;
    });
    const auto unitType = combined ? project::INotationWriter::UnitType::MULTI_PART : project::INotationWriter::UnitType::PER_PART;

    const project::ExportType type = project::ExportType::makeWithSuffixes({ "pdf" }, "PDF", "PDF");
    const muse::RetVal<muse::io::path_t> path = exportScenario()->askExportPath(notations, type, unitType);
    if (!path.ret) {
        return;
    }

    auto pagesOf = [](const Job& job) {
        return pageGroups(job.settings, job.notation->painting()->pageCount());
    };

    if (combined) {
        std::vector<DoricoPrinter::PdfPart> parts;
        for (const Job& job : jobs) {
            std::vector<int> pages;
            for (const std::vector<int>& group : pagesOf(job)) {
                pages.insert(pages.end(), group.begin(), group.end());
            }
            parts.push_back({ job.notation, job.settings, pages });
        }
        const muse::Ret ret = DoricoPrinter::writePdf(path.val.toQString(), parts);
        if (!ret) {
            showError(QString::fromStdString(ret.text()));
        }
        return;
    }

    // One file per layout, named like MuseScore's export; a Pages entry with several ranges adds a file per range.
    struct File {
        QString path;
        DoricoPrinter::PdfPart part;
    };
    std::vector<File> files;
    const IMasterNotationPtr master = globalContext()->currentMasterNotation();
    for (const Job& job : jobs) {
        const auto groups = pagesOf(job);
        for (const std::vector<int>& group : groups) {
            if (group.empty()) {
                continue;
            }

            QString file = path.val.toQString();
            if (jobs.size() > 1 || groups.size() > 1) {
                muse::io::path_t base = muse::io::dirpath(path.val) + "/" + muse::io::completeBasename(path.val);
                if (jobs.size() > 1 && master && job.notation != master->notation()) {
                    base += "-" + muse::io::escapeFileName(job.notation->name()).toStdString();
                }
                if (groups.size() > 1) {
                    base += "-" + std::to_string(group.front() + 1);
                    if (group.back() != group.front()) {
                        base += "-" + std::to_string(group.back() + 1);
                    }
                }
                file = (base + ".pdf").toQString();
            }
            files.push_back({ file, { job.notation, job.settings, group } });
        }
    }

    // Only a single file was confirmed by the save dialog.
    if (files.size() > 1) {
        const bool exists = std::any_of(files.begin(), files.end(), [](const File& f) { return muse::io::FileInfo::exists(f.path); });
        if (exists && interactive()->questionSync(muse::trc("dorico", "Export"),
                                                   muse::trc("dorico", "Some of the files already exist. Replace them?"),
                                                   { muse::IInteractive::Button::Cancel, muse::IInteractive::Button::Yes },
                                                   muse::IInteractive::Button::Yes).standardButton() != muse::IInteractive::Button::Yes) {
            return;
        }
    }

    for (const File& file : files) {
        const muse::Ret ret = DoricoPrinter::writePdf(file.path, { file.part });
        if (!ret) {
            showError(QString::fromStdString(ret.text()));
            return;
        }
    }

    // Same as the other export formats.
    platformInteractive()->revealInFileBrowser(path.val.toQString());
}

void PrintModel::runImageJobs(const QString& suffix, const std::vector<Job>& jobs)
{
    INotationPtrList notations;
    for (const Job& job : jobs) {
        notations.push_back(job.notation);
    }

    const project::ExportType type = project::ExportType::makeWithSuffixes({ suffix }, suffix.toUpper(), suffix.toUpper());
    const auto unitType = exportUnitType(suffix, true);

    const muse::RetVal<muse::io::path_t> path = exportScenario()->askExportPath(notations, type, unitType);
    if (!path.ret) {
        return;
    }

    exportScenario()->exportScores(notations, path.val, unitType, /*openDestinationFolderOnExport*/ true);
}

void PrintModel::run()
{
    const std::vector<Job> jobs = checkedJobs();
    if (jobs.empty()) {
        return;
    }

    for (const Job& job : jobs) {
        if (!job.settings.graphics) {
            runPrinterJob(job);
        }
    }

    for (const QString& format : { QString("pdf"), QString("png"), QString("svg") }) {
        std::vector<Job> group;
        std::copy_if(jobs.begin(), jobs.end(), std::back_inserter(group), [&](const Job& j) {
            return j.settings.graphics && j.settings.graphicsFormat == format;
        });
        if (group.empty()) {
            continue;
        }
        format == "pdf" ? runPdfJobs(group) : runImageJobs(format, group);
    }
}
