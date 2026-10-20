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
#include <QVariantList>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"

#include "../../../ijumpbarcontroller.h"

namespace mu::dorico {
//! Backs JumpBar.qml (spec 46): mirrors IJumpBarController and keeps the result list and its highlight.
class JumpBarModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(bool isOpen READ isOpen NOTIFY changed)
    Q_PROPERTY(int mode READ mode NOTIFY changed) // 0 Commands, 1 Go To
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY changed)
    Q_PROPERTY(QVariantList rows READ rows NOTIFY changed) // { title, detail }: the key command and alias
    Q_PROPERTY(int currentRow READ currentRow NOTIFY changed) // -1 = none
    Q_PROPERTY(bool invalid READ invalid NOTIFY changed) // Go To: Return found no such place

    QML_ELEMENT

    muse::ContextInject<IJumpBarController> controller = { this };

public:
    explicit JumpBarModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();
    Q_INVOKABLE void accept(bool optionKey);
    Q_INVOKABLE void close();
    Q_INVOKABLE void setMode(int mode);
    Q_INVOKABLE void moveRow(int delta);

    bool isOpen() const;
    int mode() const;
    QString text() const { return m_text; }
    void setText(const QString& text);
    QVariantList rows() const { return m_rows; }
    int currentRow() const { return m_currentRow; }
    bool invalid() const { return m_invalid; }

signals:
    void changed();
    void opened(); // the bar was opened, or switched mode: take keyboard focus, show the new text

private:
    void refresh();

    QString m_text;
    QVariantList m_rows;
    std::vector<std::string> m_uris; // of the rows
    int m_currentRow = -1;
    bool m_showMostUsed = false;
    bool m_invalid = false;
};
}
