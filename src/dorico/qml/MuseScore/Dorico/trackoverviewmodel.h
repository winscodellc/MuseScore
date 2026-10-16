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
#include <QVariantList>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "context/iglobalcontext.h"
#include "notation/imasternotation.h"
#include "playback/iplaybackcontroller.h"

namespace mu::dorico {
//! Play mode's track overview (spec 57): one track per instrument of the whole project (not of the open part), with
//! the bars where it has notes drawn as blocks, a bar ruler and the playhead. Positions are in quarter notes.
class TrackOverviewModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    //! [{ partId, title, muted, solo, blocks: [{ start, end }] }]
    Q_PROPERTY(QVariantList tracks READ tracks NOTIFY contentChanged)
    //! [{ start, number }]
    Q_PROPERTY(QVariantList bars READ bars NOTIFY contentChanged)
    Q_PROPERTY(double length READ length NOTIFY contentChanged)
    Q_PROPERTY(double playheadPosition READ playheadPosition NOTIFY playheadPositionChanged)

    QML_ELEMENT

    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<playback::IPlaybackController> playbackController = { this };

public:
    explicit TrackOverviewModel(QObject* parent = nullptr);

    QVariantList tracks() const { return m_tracks; }
    QVariantList bars() const { return m_bars; }
    double length() const { return m_length; }
    double playheadPosition() const { return m_playheadPosition; }

    Q_INVOKABLE void load();
    //! Mute / solo every instrument track of the part, as the mixer does (muting un-solos and vice versa).
    Q_INVOKABLE void setMuted(const QString& partId, bool muted);
    Q_INVOKABLE void setSolo(const QString& partId, bool solo);

signals:
    void contentChanged();
    void playheadPositionChanged();

private:
    void listenToScore();
    void rebuild(const engraving::Score* score);
    void updatePlayhead(muse::audio::secs_t secs);
    void listenToSoloMute();
    void changeSoloMute(const QString& partId, bool mute, bool value);

    notation::IMasterNotationPtr m_master;
    notation::INotationPtr m_notation; // solo / mute live on the current notation (score or part), like the mixer
    QVariantList m_tracks;
    QVariantList m_bars;
    double m_length = 0;
    double m_playheadPosition = 0;
};
}
