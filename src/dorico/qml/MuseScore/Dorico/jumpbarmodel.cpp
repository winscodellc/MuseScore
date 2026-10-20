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

#include "jumpbarmodel.h"

#include <algorithm>

using namespace mu::dorico;

JumpBarModel::JumpBarModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void JumpBarModel::load()
{
    controller()->stateChanged().onNotify(this, [this]() {
        m_text = QString::fromStdString(controller()->initialText());
        m_showMostUsed = false;
        m_invalid = false;
        refresh();
        if (isOpen()) {
            emit opened();
        }
    });
}

bool JumpBarModel::isOpen() const
{
    return controller()->isOpen();
}

int JumpBarModel::mode() const
{
    return static_cast<int>(controller()->mode());
}

void JumpBarModel::setText(const QString& text)
{
    if (m_text != text) {
        m_text = text;
        m_showMostUsed = false;
        m_invalid = false;
        refresh();
    }
}

void JumpBarModel::refresh()
{
    std::vector<CatalogEntry> entries;
    if (isOpen() && controller()->mode() == JumpBarMode::Commands) {
        entries = m_text.isEmpty() && m_showMostUsed ? controller()->mostUsed() : controller()->search(m_text.toStdString());
    }

    m_rows.clear();
    m_uris.clear();
    for (const CatalogEntry& e : entries) {
        QStringList detail;
        for (const std::string& s : { e.keys, e.alias }) {
            if (!s.empty()) {
                detail << QString::fromStdString(s);
            }
        }
        m_rows << QVariantMap { { "title", QString::fromStdString(e.title) }, { "detail", detail.join("  ·  ") } };
        m_uris.push_back(e.uri);
    }
    m_currentRow = m_rows.isEmpty() ? -1 : 0;
    emit changed();
}

void JumpBarModel::moveRow(int delta)
{
    // Down on an empty field lists the most used commands
    if (m_rows.isEmpty() && m_text.isEmpty() && delta > 0 && mode() == 0) {
        m_showMostUsed = true;
        refresh();
        return;
    }
    const int next = std::clamp(m_currentRow + delta, std::min(0, static_cast<int>(m_rows.size()) - 1),
                                static_cast<int>(m_rows.size()) - 1);
    if (next != m_currentRow) {
        m_currentRow = next;
        emit changed();
    }
}

void JumpBarModel::setMode(int mode)
{
    controller()->setMode(static_cast<JumpBarMode>(mode));
}

void JumpBarModel::accept(bool optionKey)
{
    const std::string uri = m_currentRow >= 0 ? m_uris.at(m_currentRow) : std::string();
    if (!controller()->accept(m_text.toStdString(), uri, optionKey) && mode() == 1) {
        m_invalid = true;
        emit changed();
    }
}

void JumpBarModel::close()
{
    controller()->close();
}
