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

#include <map>

#include <QObject>
#include <QString>
#include <QVariantList>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "interactive/iinteractive.h"
#include "midiremote/imidiremote.h"
#include "shortcuts/ishortcutsregister.h"
#include "settings.h"
#include "ui/iuiconfiguration.h"
#include "languages/ilanguagesconfiguration.h"
#include "languages/ilanguagesservice.h"
#include "notation/inotationconfiguration.h"
#include "project/iprojectconfiguration.h"
#include "playback/iplaybackconfiguration.h"
#include "importexport/musicxml/imusicxmlconfiguration.h"
#include "appshell/iappshellconfiguration.h"

#include "../../../internal/prefstable.h"

namespace mu::dorico {
//! Backs DoricoPreferencesDialog.qml (spec 100). Rows read and write MuseScore's own configuration through the table in
//! internal/prefstable.cpp. Edits are staged by the settings transaction MuseScore's dialog already uses
//! (IAppShellConfiguration::startEditSettings): Apply commits it, closing the dialog rolls it back.
class DoricoPreferencesModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(QVariantList categories READ categories NOTIFY categoriesChanged)
    Q_PROPERTY(QString categorySearch READ categorySearch WRITE setCategorySearch NOTIFY categoriesChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged) // bumps when values change, so bindings re-read them
    Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged)

    QML_ELEMENT

    muse::GlobalInject<appshell::IAppShellConfiguration> appShellConfiguration;
    muse::GlobalInject<notation::INotationConfiguration> notationConfiguration;
    muse::GlobalInject<project::IProjectConfiguration> projectConfiguration;
    muse::GlobalInject<playback::IPlaybackConfiguration> playbackConfiguration;
    muse::GlobalInject<iex::musicxml::IMusicXmlConfiguration> musicXmlConfiguration;
    muse::GlobalInject<muse::ui::IUiConfiguration> uiConfiguration;
    muse::GlobalInject<muse::languages::ILanguagesConfiguration> languagesConfiguration;
    muse::GlobalInject<muse::languages::ILanguagesService> languagesService;
    muse::ContextInject<muse::IInteractive> interactive = { this };
    muse::ContextInject<muse::shortcuts::IShortcutsRegister> shortcutsRegister = { this };
    muse::ContextInject<muse::midiremote::IMidiRemote> midiRemote = { this };

public:
    explicit DoricoPreferencesModel(QObject* parent = nullptr);
    ~DoricoPreferencesModel() override;

    Q_INVOKABLE void load();

    QVariantList categories() const;
    QString categorySearch() const { return m_categorySearch; }
    void setCategorySearch(const QString& text);
    int revision() const { return m_revision; }
    bool dirty() const { return m_dirty; }

    //! The category's pages, each with its sections and rows, as plain maps for the QML repeaters
    Q_INVOKABLE QVariantList content(const QString& categoryId) const;
    //! Ids of the rows and section anchors on the category's page that contain `text`, in page order
    Q_INVOKABLE QStringList matches(const QString& categoryId, const QString& text) const;

    Q_INVOKABLE QVariant value(const QString& rowId) const;
    Q_INVOKABLE void setValue(const QString& rowId, const QVariant& value);

    //! Compares the settings with those at the last load / Apply (embedded MuseScore pages write them directly)
    Q_INVOKABLE void refreshDirty();
    Q_INVOKABLE void apply();
    Q_INVOKABLE void discard();
    //! Close / Escape: with unapplied changes asks "apply them now?" (Discard Changes, Cancel, Apply Changes), then closeRequested
    //! embeddedPending: an embedded MuseScore page may hold edits the settings diff can't see
    Q_INVOKABLE void requestClose(bool embeddedPending = false);
    Q_INVOKABLE void askForReset();
    Q_INVOKABLE void resetToFactory();

signals:
    void loaded(); // the table was (re)built: pages must be re-read
    void categoriesChanged();
    void revisionChanged();
    void dirtyChanged();
    void resetConfirmed();
    void closeRequested(bool apply);

private:
    void startEditing();
    void setDirty(bool dirty);
    void bump();

    QVariantMap rowToMap(const PrefRow& row, const QString& anchor) const;

    PrefTable m_table;
    QString m_categorySearch;
    int m_revision = 0;
    bool m_dirty = false;
    bool m_editing = false;
    std::map<muse::Settings::Key, muse::Val> m_snapshot;
    std::map<muse::Settings::Key, muse::Val> m_lastPoll; // to see an embedded MuseScore page change a setting
};
}
