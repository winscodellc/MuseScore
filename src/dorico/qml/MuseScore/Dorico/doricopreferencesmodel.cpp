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

#include "doricopreferencesmodel.h"

#include <cmath>

#include <QGuiApplication>

#include "translation.h"

using namespace mu::dorico;

static const char* prefsTypeName(PrefType type)
{
    switch (type) {
    case PrefType::Check: return "check";
    case PrefType::Dropdown: return "dropdown";
    case PrefType::Segmented: return "segmented";
    case PrefType::Spinner: return "spinner";
    case PrefType::Text: return "text";
    case PrefType::Path: return "path";
    case PrefType::Hex: return "hex";
    case PrefType::Slider: return "slider";
    case PrefType::Button: return "button";
    }
    return "";
}

static QString prefsQs(const std::string& s)
{
    return QString::fromStdString(s);
}

static const PrefRow* prefsFindRow(const PrefTable& table, const QString& rowId)
{
    return findPrefRow(table, rowId.toStdString());
}

DoricoPreferencesModel::DoricoPreferencesModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

DoricoPreferencesModel::~DoricoPreferencesModel()
{
    discard(); // never leave the settings transaction open
}

void DoricoPreferencesModel::load()
{
    PrefBackends backends;
    backends.notation = notationConfiguration();
    backends.project = projectConfiguration();
    backends.playback = playbackConfiguration();
    backends.musicXml = musicXmlConfiguration();
    backends.ui = uiConfiguration();
    backends.languagesConfig = languagesConfiguration();
    backends.languages = languagesService();
    m_table = buildPrefTable(backends);

    startEditing();
    emit categoriesChanged();
    emit loaded();
}

void DoricoPreferencesModel::startEditing()
{
    if (!m_editing) {
        appShellConfiguration()->startEditSettings();
        m_editing = true;
    }
    m_snapshot.clear();
    for (const auto& [key, item] : muse::settings()->items()) {
        m_snapshot[key] = item.value;
    }
    setDirty(false);
}

void DoricoPreferencesModel::setDirty(bool dirty)
{
    if (m_dirty != dirty) {
        m_dirty = dirty;
        emit dirtyChanged();
    }
}

void DoricoPreferencesModel::bump()
{
    ++m_revision;
    emit revisionChanged();
}

void DoricoPreferencesModel::refreshDirty()
{
    bool changed = false;
    for (const auto& [key, item] : muse::settings()->items()) {
        auto it = m_snapshot.find(key);
        if (it == m_snapshot.end() || !(it->second == item.value)) {
            changed = true;
            break;
        }
    }
    setDirty(changed);
}

void DoricoPreferencesModel::apply()
{
    if (m_editing) {
        appShellConfiguration()->applySettings();
        m_editing = false;
    }
    startEditing();
}

void DoricoPreferencesModel::discard()
{
    if (m_editing) {
        appShellConfiguration()->rollbackSettings();
        m_editing = false;
    }
    setDirty(false);
}

void DoricoPreferencesModel::requestClose()
{
    refreshDirty();
    if (!m_dirty) {
        emit closeRequested(false);
        return;
    }

    using Btn = muse::IInteractive::Button;
    muse::IInteractive::ButtonData discardBtn(Btn::Discard, muse::trc("dorico", "Discard Changes"));
    muse::IInteractive::ButtonData cancelBtn(Btn::Cancel, muse::trc("dorico", "Cancel"));
    muse::IInteractive::ButtonData applyBtn(Btn::Apply, muse::trc("dorico", "Apply Changes"), true);

    interactive()->question(muse::trc("dorico", "You have made changes to the Preferences."),
                            muse::trc("dorico", "Do you want to apply them now?"),
                            { discardBtn, cancelBtn, applyBtn }, applyBtn.btn, { muse::IInteractive::Option::WithIcon },
                            muse::trc("dorico", "Preferences"))
    .onResolve(this, [this](const muse::IInteractive::Result& res) {
        if (res.isButton(Btn::Discard)) {
            emit closeRequested(false);
        } else if (res.isButton(Btn::Apply)) {
            emit closeRequested(true);
        }
    });
}

void DoricoPreferencesModel::askForReset()
{
    using Btn = muse::IInteractive::Button;
    muse::IInteractive::ButtonData cancelBtn = interactive()->buttonData(Btn::Cancel);
    muse::IInteractive::ButtonData resetBtn = interactive()->buttonData(Btn::Reset);
    cancelBtn.accent = true;

    interactive()->warning(muse::trc("dorico", "Reset all preferences to their factory settings?"),
                           muse::trc("dorico", "This resets every option in this dialog, and deletes all custom shortcuts. "
                                               "It does not delete any of your scores.\n\nThis action cannot be undone."),
                           { cancelBtn, resetBtn }, cancelBtn.btn, { muse::IInteractive::Option::WithIcon },
                           muse::trc("dorico", "Reset to Factory"))
    .onResolve(this, [this](const muse::IInteractive::Result& res) {
        if (res.isButton(Btn::Reset)) {
            emit resetConfirmed();
        }
    });
}

void DoricoPreferencesModel::resetToFactory()
{
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    QGuiApplication::processEvents();
    appShellConfiguration()->revertToFactorySettings(true);
    // as MuseScore's dialog does: the first-launch wizard must not come back
    appShellConfiguration()->setHasCompletedFirstLaunchSetup(true);
    appShellConfiguration()->startEditSettings();
    m_editing = true;
    QGuiApplication::restoreOverrideCursor();

    load();
    bump();
}

QVariantMap DoricoPreferencesModel::rowToMap(const PrefRow& row, const QString& anchor) const
{
    QVariantMap m;
    m["id"] = prefsQs(row.id);
    m["anchor"] = anchor;
    m["label"] = prefsQs(row.label);
    m["help"] = prefsQs(row.help);
    m["type"] = prefsTypeName(row.type);
    m["min"] = row.min;
    m["max"] = row.max;
    m["unit"] = prefsQs(row.unit);
    m["backed"] = row.backed();
    m["caption"] = row.type == PrefType::Button ? prefsQs(row.fallback.toString()) : QString();
    QVariantList options;
    for (const PrefOption& o : row.options) {
        options << QVariantMap { { "text", prefsQs(o.text) }, { "value", o.value.toQVariant() } };
    }
    m["options"] = options;
    if (row.extra) {
        m["extra"] = rowToMap(*row.extra, QString());
    }
    return m;
}

namespace {
struct PrefsAnchors {
    static QString page(const PrefCategory& c, const PrefPage& p) { return prefsQs(c.id + "/" + p.id); }
    static QString section(const PrefCategory& c, const PrefPage& p, size_t i) { return page(c, p) + "/s" + QString::number(i); }
};
}

QVariantList DoricoPreferencesModel::categories() const
{
    const QString filter = m_categorySearch.trimmed();
    auto has = [&filter](const std::string& title) { return filter.isEmpty() || prefsQs(title).contains(filter, Qt::CaseInsensitive); };

    QVariantList result;
    for (const PrefCategory& c : m_table) {
        // sub-rows: the pages when there are several, else the section titles
        QVariantList children;
        bool anyChildMatches = false;
        auto addChild = [&](const std::string& title, const QString& anchor) {
            const bool matches = has(title);
            anyChildMatches = anyChildMatches || matches;
            children << QVariantMap { { "title", prefsQs(title) }, { "anchor", anchor }, { "matches", matches } };
        };
        if (c.pages.size() > 1) {
            for (const PrefPage& p : c.pages) {
                addChild(p.title, PrefsAnchors::page(c, p));
            }
        } else {
            for (size_t i = 0; i < c.pages.front().sections.size(); ++i) {
                const PrefSection& s = c.pages.front().sections[i];
                if (!s.title.empty() && s.title != "MuseScore") {
                    addChild(s.title, PrefsAnchors::section(c, c.pages.front(), i));
                }
            }
        }
        if (!has(c.title) && !anyChildMatches) {
            continue;
        }
        QVariantList shown;
        for (const QVariant& child : children) {
            if (has(c.title) || child.toMap().value("matches").toBool()) {
                shown << child;
            }
        }
        result << QVariantMap { { "id", prefsQs(c.id) }, { "title", prefsQs(c.title) }, { "children", shown } };
    }
    return result;
}

void DoricoPreferencesModel::setCategorySearch(const QString& text)
{
    if (m_categorySearch != text) {
        m_categorySearch = text;
        emit categoriesChanged();
    }
}

QVariantList DoricoPreferencesModel::content(const QString& categoryId) const
{
    QVariantList pages;
    for (const PrefCategory& c : m_table) {
        if (prefsQs(c.id) != categoryId) {
            continue;
        }
        for (const PrefPage& p : c.pages) {
            QVariantList sections;
            for (size_t i = 0; i < p.sections.size(); ++i) {
                const PrefSection& s = p.sections[i];
                QVariantList rows;
                for (const PrefRow& r : s.rows) {
                    rows << rowToMap(r, QString());
                }
                QVariantList embedded;
                for (const PrefEmbedded& e : s.embedded) {
                    embedded << QVariantMap { { "page", prefsQs(e.page) }, { "title", prefsQs(e.title) } };
                }
                sections << QVariantMap { { "title", prefsQs(s.title) }, { "anchor", PrefsAnchors::section(c, p, i) },
                                          { "rows", rows }, { "embedded", embedded } };
            }
            // a one-page category shows its own name as the page title
            pages << QVariantMap { { "title", prefsQs(c.pages.size() > 1 ? p.title : c.title) }, { "anchor", PrefsAnchors::page(c, p) },
                                   { "sections", sections } };
        }
    }
    return pages;
}

QStringList DoricoPreferencesModel::matches(const QString& categoryId, const QString& text) const
{
    QStringList ids;
    if (text.trimmed().isEmpty()) {
        return ids;
    }
    auto has = [&text](const std::string& s) { return prefsQs(s).contains(text.trimmed(), Qt::CaseInsensitive); };
    for (const PrefCategory& c : m_table) {
        if (prefsQs(c.id) != categoryId) {
            continue;
        }
        for (const PrefPage& p : c.pages) {
            for (size_t i = 0; i < p.sections.size(); ++i) {
                const PrefSection& s = p.sections[i];
                if (!s.title.empty() && has(s.title)) {
                    ids << PrefsAnchors::section(c, p, i);
                }
                for (const PrefRow& r : s.rows) {
                    if (has(r.label) || has(r.help) || (r.extra && has(r.extra->label))) {
                        ids << prefsQs(r.id);
                    }
                }
            }
        }
    }
    return ids;
}

QVariant DoricoPreferencesModel::value(const QString& rowId) const
{
    const PrefRow* row = prefsFindRow(m_table, rowId);
    if (!row) {
        return QVariant();
    }
    return row->backed() ? row->get().toQVariant() : row->fallback.toQVariant();
}

void DoricoPreferencesModel::setValue(const QString& rowId, const QVariant& value)
{
    const PrefRow* row = prefsFindRow(m_table, rowId);
    if (!row || !row->backed()) {
        return;
    }
    // QML numbers arrive as doubles; whole ones are ints for the setters that take an int
    muse::Val val = muse::Val::fromQVariant(value);
    if (value.typeId() == QMetaType::Double && std::floor(value.toDouble()) == value.toDouble() && row->type == PrefType::Spinner) {
        val = muse::Val(static_cast<int>(value.toDouble()));
    }
    row->set(val);
    bump();
    refreshDirty();
}
