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


#include "selectionsummary.h"

#include <algorithm>
#include <cmath>

#include <QStringList>

using namespace mu::dorico;

static QString clock(double seconds)
{
    const int total = static_cast<int>(std::lround(std::max(0.0, seconds)));
    return QString("%1:%2:%3")
           .arg(total / 3600, 2, 10, QChar('0'))
           .arg(total / 60 % 60, 2, 10, QChar('0'))
           .arg(total % 60, 2, 10, QChar('0'));
}

static QString barText(int first, int last)
{
    return first == last ? QString("Bar %1").arg(first) : QString("Bars %1-%2").arg(first).arg(last);
}

QString mu::dorico::summarise(const SelectionInfo& info)
{
    constexpr const char* SEP = "   ";

    switch (info.kind) {
    case SelectionInfo::Kind::None:
        return QStringLiteral("No selection");
    case SelectionInfo::Kind::Multiple:
        return QStringLiteral("Multiple selection");
    case SelectionInfo::Kind::Single: {
        QStringList parts { barText(info.firstBar, info.firstBar) };
        if (!info.pitch.isEmpty()) {
            parts << info.pitch;
        }
        if (info.voice > 0) {
            parts << QString("V%1").arg(info.voice);
        }
        return parts.join(SEP);
    }
    case SelectionInfo::Kind::Range: {
        const double length = std::max(0.0, info.endSeconds - info.startSeconds);
        return QString("%1%2%3-%4 (%5\")")
               .arg(barText(info.firstBar, info.lastBar), SEP, clock(info.startSeconds), clock(info.endSeconds))
               .arg(QString::number(length, 'g', 3));
    }
    }
    return {};
}
