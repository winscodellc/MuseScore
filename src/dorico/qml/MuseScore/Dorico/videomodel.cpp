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

#include "videomodel.h"

#include <algorithm>
#include <cmath>

#include "engraving/dom/masterscore.h"
#include "engraving/dom/tempotimeline.h"
#include "engraving/dom/sig.h"
#include "engraving/editing/editscoreproperties.h"
#include "engraving/types/constants.h"
#include "notation/inotation.h"
#include "notation/inotationplayback.h"
#include "settings.h"

#include "dorico/internal/prefstable.h"
#include "notation/inotationundostack.h"

using namespace mu::dorico;

VideoModel::VideoModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void VideoModel::load()
{
    globalContext()->currentMasterNotationChanged().onNotify(this, [this]() { listenToScore(); });
    globalContext()->playbackState()->playbackPositionChanged().onReceive(this, [this](muse::audio::secs_t) {
        emit playheadChanged();
    });
    listenToScore();
}

void VideoModel::listenToScore()
{
    // Disconnect from the previous project's stack first (doricostatusmodel.cpp does the same)
    if (m_master) {
        m_master->notation()->undoStack()->stackChanged().disconnect(this);
    }
    m_master = globalContext()->currentMasterNotation();
    if (m_master) {
        // Every edit here, and its undo / redo, goes through the stack
        m_master->notation()->undoStack()->stackChanged().onNotify(this, [this]() { read(); });
    }
    read();
}

void VideoModel::read()
{
    m_data = m_master ? readVideoData(m_master->masterScore()) : VideoData {};
    emit changed();
    emit playheadChanged();
}

void VideoModel::write(const VideoData& data, const char* undoTitle)
{
    if (!m_master || data == m_data) {
        return;
    }
    VideoData sorted = data;
    std::stable_sort(sorted.markers.begin(), sorted.markers.end(), [](const VideoMarker& a, const VideoMarker& b) {
        return a.time < b.time;
    });

    engraving::MasterScore* score = m_master->masterScore();
    std::map<muse::String, muse::String> tags = score->metaTags();
    tags[VIDEO_TAG] = muse::String::fromStdString(videoDataToJson(sorted));
    const notation::INotationUndoStackPtr undoStack = m_master->notation()->undoStack();
    undoStack->prepareChanges(muse::TranslatableString("undoableAction", undoTitle));
    score->undo(new engraving::ChangeMetaTags(score, tags));
    undoStack->commitChanges();
}

QVariantList VideoModel::markers() const
{
    QVariantList list;
    for (const VideoMarker& m : m_data.markers) {
        list << QVariantMap { { "id", m.id }, { "timecode", format(m.time) }, { "text", QString::fromStdString(m.text) },
            { "important", m.important } };
    }
    return list;
}

bool VideoModel::hasImportant() const
{
    return std::any_of(m_data.markers.begin(), m_data.markers.end(), [](const VideoMarker& m) { return m.important; });
}

QStringList VideoModel::frameRates() const
{
    QStringList list;
    for (const FrameRateInfo& r : mu::dorico::frameRates()) {
        list << r.label;
    }
    return list;
}

QString VideoModel::format(double seconds) const
{
    return QString::fromStdString(formatTimecodeSeconds(seconds, m_data.rate, /*nearest*/ true));
}

QString VideoModel::startOffset() const
{
    return format(m_data.startOffset);
}

QString VideoModel::timecodeStart() const
{
    return format(m_data.timecodeStart);
}

double VideoModel::flowStart() const
{
    return m_master ? timeMap(m_master->masterScore(), m_data).flowStart() : m_data.timecodeStart;
}

double VideoModel::flowEnd() const
{
    if (!m_master) {
        return flowStart();
    }
    const engraving::MasterScore* score = m_master->masterScore();
    const TimeMap map = timeMap(score, m_data);
    return map.flowStart() + map.musicSeconds(score->endTick().ticks());
}

QString VideoModel::flowStartTimecode() const
{
    return format(flowStart());
}

// Markers cannot go in negative time (before 00:00:00:00)
QString VideoModel::rangeStart() const
{
    return format(std::max(0.0, flowStart()));
}

QString VideoModel::rangeEnd() const
{
    return format(flowEnd());
}

QString VideoModel::playheadTimecode() const
{
    if (!m_master) {
        return format(0.0);
    }
    // Elapsed playback time is real time from the start of the flow, repeats included
    const double secs = globalContext()->playbackState()->playbackPosition();
    const double multiplier = m_master->masterScore()->tempoTimeline(false).tempoMultiplier().val;
    // The frame showing, so rounded down (format() rounds to the nearest)
    return QString::fromStdString(formatTimecodeSeconds(flowStart() + secs * multiplier, m_data.rate));
}

QString VideoModel::playheadBars() const
{
    if (!m_master) {
        return "1.1.1.000";
    }
    const double secs = globalContext()->playbackState()->playbackPosition();
    const int tick = m_master->playback()->secToTick(secs);
    int bar = 0;
    int beat = 0;
    int ticks = 0;
    m_master->masterScore()->sigmap()->tickValues(tick, &bar, &beat, &ticks);
    constexpr int SIXTEENTH = engraving::Constants::DIVISION / 4;
    return QString("%1.%2.%3.%4").arg(bar + 1).arg(beat + 1).arg(ticks / SIXTEENTH + 1).arg(ticks % SIXTEENTH, 3, 10, QChar('0'));
}

QString VideoModel::playheadElapsed() const
{
    const long long ms = std::llround(globalContext()->playbackState()->playbackPosition() * 1000.0);
    return QString("%1:%2:%3.%4").arg(ms / 3600000, 2, 10, QChar('0')).arg(ms / 60000 % 60, 2, 10, QChar('0'))
           .arg(ms / 1000 % 60, 2, 10, QChar('0')).arg(ms % 1000, 3, 10, QChar('0'));
}

int VideoModel::defaultTimeDisplay() const
{
    const std::string v = muse::settings()->value(muse::Settings::Key("dorico", TRANSPORT_DISPLAY_SETTING)).toString();
    return v == "Elapsed Time" ? 1 : (v == "Timecode" ? 2 : 0);
}

QString VideoModel::markerTime(const QString& timecode, double& time) const
{
    const std::optional<long long> frames = parseTimecode(timecode.toStdString(), m_data.rate);
    if (!frames) {
        return tr("Invalid timecode");
    }
    const long long first = secondsToFrames(std::max(0.0, flowStart()), m_data.rate, true);
    const long long last = secondsToFrames(flowEnd(), m_data.rate, true);
    time = framesToSeconds(std::clamp(*frames, first, std::max(first, last)), m_data.rate);
    return {};
}

QString VideoModel::addMarker(const QString& text, const QString& timecode)
{
    double time = 0.0;
    const QString error = markerTime(timecode, time);
    if (!error.isEmpty()) {
        return error;
    }
    VideoData data = m_data;
    const QString trimmed = text.trimmed();
    data.markers.push_back({ data.nextId++, time, (trimmed.isEmpty() ? QString("Marker") : trimmed).toStdString(), false });
    write(data, "Add marker");
    return {};
}

QString VideoModel::setMarkerTimecode(int id, const QString& timecode)
{
    double time = 0.0;
    const QString error = markerTime(timecode, time);
    if (!error.isEmpty()) {
        return error;
    }
    VideoData data = m_data;
    for (VideoMarker& m : data.markers) {
        if (m.id == id) {
            m.time = time;
        }
    }
    write(data, "Change marker timecode");
    return {};
}

void VideoModel::setMarkerText(int id, const QString& text)
{
    VideoData data = m_data;
    for (VideoMarker& m : data.markers) {
        if (m.id == id) {
            m.text = text.toStdString();
        }
    }
    write(data, "Change marker text");
}

void VideoModel::setMarkerImportant(int id, bool important)
{
    VideoData data = m_data;
    for (VideoMarker& m : data.markers) {
        if (m.id == id) {
            m.important = important;
        }
    }
    write(data, "Change marker importance");
}

void VideoModel::deleteMarker(int id)
{
    VideoData data = m_data;
    std::erase_if(data.markers, [id](const VideoMarker& m) { return m.id == id; });
    write(data, "Delete marker");
}

QString VideoModel::applyProperties(int rateIndex, int attachBeats, int attachUnit, const QString& startOffset,
                                    const QString& timecodeStart)
{
    if (rateIndex < 0 || rateIndex >= int(mu::dorico::frameRates().size()) || attachBeats < 0 || attachUnit <= 0) {
        return tr("Invalid value");
    }
    const FrameRate rate = static_cast<FrameRate>(rateIndex);
    const std::optional<double> offset = parseTimecodeSeconds(startOffset.toStdString(), rate);
    const std::optional<double> start = parseTimecodeSeconds(timecodeStart.toStdString(), rate);
    if (!offset || *offset < 0 || !start || *start < 0) {
        return tr("Invalid timecode");
    }
    VideoData data = m_data;
    data.rate = rate;
    data.attachBeats = attachBeats;
    data.attachUnit = attachUnit;
    data.startOffset = *offset;
    data.timecodeStart = *start;
    for (VideoMarker& m : data.markers) {
        m.time += *start - m_data.timecodeStart;
    }
    write(data, "Change video properties");
    return {};
}

void VideoModel::openAddMarker()
{
    if (m_master) {
        interactive()->open("musescore://dorico/addmarker");
    }
}

void VideoModel::openProperties()
{
    if (m_master) {
        interactive()->open("musescore://dorico/videoproperties");
    }
}
