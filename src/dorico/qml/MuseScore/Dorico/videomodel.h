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
#include <QStringList>
#include <QVariantList>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "context/iglobalcontext.h"
#include "interactive/iinteractive.h"
#include "notation/imasternotation.h"

#include "dorico/video/videodata.h"

namespace mu::dorico {
//! Markers, timecodes and the video settings of the current project (spec 91), for the Video panel, the Add Marker and
//! Video Properties dialogs, the flow card and the time display. Every edit is one undo step on the score.
//! Timecodes go in and out as text in the project frame rate; the edits that take one return an error, or "" if done.
//! A marker timecode outside the flow is clamped into it, as in Dorico.
class VideoModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    //! Sorted by time: { id, timecode, text, important }
    Q_PROPERTY(QVariantList markers READ markers NOTIFY changed)
    Q_PROPERTY(bool hasImportant READ hasImportant NOTIFY changed)
    Q_PROPERTY(bool hasProject READ hasProject NOTIFY changed)

    Q_PROPERTY(QStringList frameRates READ frameRates CONSTANT)
    Q_PROPERTY(int rateIndex READ rateIndex NOTIFY changed)
    Q_PROPERTY(bool dropFrame READ dropFrame NOTIFY changed)
    Q_PROPERTY(int framesPerSecond READ framesPerSecond NOTIFY changed)
    Q_PROPERTY(bool useVideoRate READ useVideoRate NOTIFY changed)
    Q_PROPERTY(int attachBeats READ attachBeats NOTIFY changed)
    Q_PROPERTY(int attachUnit READ attachUnit NOTIFY changed)
    Q_PROPERTY(QString startOffset READ startOffset NOTIFY changed)
    Q_PROPERTY(QString timecodeStart READ timecodeStart NOTIFY changed)

    Q_PROPERTY(QString flowStartTimecode READ flowStartTimecode NOTIFY changed)
    Q_PROPERTY(QString rangeStart READ rangeStart NOTIFY changed)
    Q_PROPERTY(QString rangeEnd READ rangeEnd NOTIFY changed)
    Q_PROPERTY(QString playheadTimecode READ playheadTimecode NOTIFY playheadChanged)
    //! The other two time display formats: "2.2.1.009" (bar.beat.sixteenth.tick) and "00:00:02.541"
    Q_PROPERTY(QString playheadBars READ playheadBars NOTIFY playheadChanged)
    Q_PROPERTY(QString playheadElapsed READ playheadElapsed NOTIFY playheadChanged)

    QML_ELEMENT

    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<muse::IInteractive> interactive = { this };

public:
    explicit VideoModel(QObject* parent = nullptr);

    QVariantList markers() const;
    bool hasImportant() const;
    bool hasProject() const { return m_master != nullptr; }

    QStringList frameRates() const;
    int rateIndex() const { return static_cast<int>(m_data.rate); }
    bool dropFrame() const { return frameRateInfo(m_data.rate).drop > 0; }
    int framesPerSecond() const { return frameRateInfo(m_data.rate).nominal; }
    bool useVideoRate() const { return m_data.useVideoRate; }
    int attachBeats() const { return m_data.attachBeats; }
    int attachUnit() const { return m_data.attachUnit; }
    QString startOffset() const;
    QString timecodeStart() const;

    QString flowStartTimecode() const;
    QString rangeStart() const;
    QString rangeEnd() const;
    QString playheadTimecode() const;
    QString playheadBars() const;
    QString playheadElapsed() const;

    Q_INVOKABLE void load();

    Q_INVOKABLE QString addMarker(const QString& text, const QString& timecode);
    Q_INVOKABLE QString setMarkerTimecode(int id, const QString& timecode);
    Q_INVOKABLE void setMarkerText(int id, const QString& text);
    Q_INVOKABLE void setMarkerImportant(int id, bool important);
    Q_INVOKABLE void deleteMarker(int id);

    //! Timecodes are read in the new frame rate, so they keep what the user typed. A new timecode start moves every
    //! marker by the same amount, so the markers stay on the music (as Dorico 6.2 does)
    Q_INVOKABLE QString applyProperties(int rateIndex, int attachBeats, int attachUnit, const QString& startOffset,
                                        const QString& timecodeStart);

    //! The time display's first format, from Preferences > Play: 0 bars and beats, 1 elapsed time, 2 timecode
    Q_INVOKABLE int defaultTimeDisplay() const;

    Q_INVOKABLE void openAddMarker();
    Q_INVOKABLE void openProperties();

signals:
    void changed();
    void playheadChanged();

private:
    void listenToScore();
    void read();
    void write(const VideoData& data, const char* undoTitle);
    //! The error for an invalid timecode, or "" and the time, clamped into the flow
    QString markerTime(const QString& timecode, double& time) const;
    double flowStart() const;
    double flowEnd() const;
    QString format(double seconds) const;

    notation::IMasterNotationPtr m_master;
    VideoData m_data;
};
}
