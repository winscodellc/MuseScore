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

#include <QElapsedTimer>
#include <QLoggingCategory>

namespace mu::dorico {
// Timings of the hot paths (spec 96). Off by default: QT_LOGGING_RULES="dorico.perf.info=true".
// Info, not debug: the app's Qt message handler drops debug lines unless the log level is Debug.
inline const QLoggingCategory& doricoPerf()
{
    static const QLoggingCategory category("dorico.perf", QtWarningMsg);
    return category;
}

// Logs how long the enclosing scope took, in microseconds.
class PerfTimer
{
public:
    explicit PerfTimer(const char* name)
        : m_name(name) { if (doricoPerf().isInfoEnabled()) { m_timer.start(); } }
    ~PerfTimer()
    {
        if (m_timer.isValid()) {
            qCInfo(doricoPerf) << m_name << m_timer.nsecsElapsed() / 1000 << "us";
        }
    }

private:
    const char* m_name;
    QElapsedTimer m_timer;
};
}
