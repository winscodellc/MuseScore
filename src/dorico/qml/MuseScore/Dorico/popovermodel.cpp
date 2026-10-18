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

#include "popovermodel.h"

#include <algorithm>

using namespace mu::dorico;

PopoverModel::PopoverModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void PopoverModel::load()
{
    controller()->stateChanged().onNotify(this, [this]() {
        m_text = QString::fromStdString(controller()->initialText());
        m_currentSuggestion = -1;
        emit changed();
        if (isOpen()) {
            emit opened();
        }
    });
}

bool PopoverModel::isOpen() const
{
    return controller()->current() != nullptr;
}

int PopoverModel::iconCode() const
{
    return isOpen() ? static_cast<int>(controller()->current()->icon) : 0;
}

QString PopoverModel::title() const
{
    return isOpen() ? QString::fromStdString(controller()->current()->title) : QString();
}

bool PopoverModel::supportsLocal() const
{
    return isOpen() && controller()->current()->supportsLocal;
}

bool PopoverModel::navigates() const
{
    return isOpen() && controller()->current()->applyAt != nullptr;
}

void PopoverModel::setText(const QString& text)
{
    if (m_text != text) {
        m_text = text;
        m_currentSuggestion = -1;
        emit changed();
    }
}

int PopoverModel::status() const
{
    return static_cast<int>(controller()->validate(m_text.toStdString()));
}

QStringList PopoverModel::suggestions() const
{
    QStringList list;
    const PopoverDef* def = controller()->current();
    if (def && def->suggest) {
        for (const std::string& s : def->suggest(m_text.toStdString())) {
            list << QString::fromStdString(s);
        }
    }
    return list;
}

void PopoverModel::moveSuggestion(int delta)
{
    const int count = static_cast<int>(suggestions().size());
    const int next = std::clamp(m_currentSuggestion + delta, -1, count - 1);
    if (next != m_currentSuggestion) {
        m_currentSuggestion = next;
        emit changed();
    }
}

void PopoverModel::setViewMatrix(const QVariant& matrix)
{
    m_viewMatrix = matrix;
    emit changed();
}

QRectF PopoverModel::anchorRect() const
{
    return m_viewMatrix.value<QTransform>().mapRect(controller()->anchorRect().toQRectF());
}

void PopoverModel::accept(bool local)
{
    const QStringList list = suggestions();
    const QString text = m_currentSuggestion >= 0 && m_currentSuggestion < list.size() ? list.at(m_currentSuggestion) : m_text;
    controller()->accept(text.toStdString(), local ? ApplyScope::Local : ApplyScope::Default);
}

void PopoverModel::move(int popoverMove)
{
    controller()->move(m_text.toStdString(), static_cast<PopoverMove>(popoverMove));
}

void PopoverModel::cancel()
{
    controller()->cancel();
}
