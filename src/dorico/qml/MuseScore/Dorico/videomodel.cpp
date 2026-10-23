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

#include <QDir>
#include <QFileInfo>
#ifdef DORICO_HAS_VIDEO
#include "dorico/video/videodecoder.h"
#include <QUrl>
#endif

#include "engraving/dom/masterscore.h"
#include "engraving/dom/tempotimeline.h"
#include "engraving/dom/sig.h"
#include "engraving/types/constants.h"
#include "notation/inotation.h"
#include "notation/inotationplayback.h"
#include "notation/inotationundostack.h"
#include "project/inotationproject.h"
#include "settings.h"

#include "dorico/internal/prefstable.h"

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
    writeVideoData(m_master, data, undoTitle);
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
    const bool attaching = !m_probePath.isEmpty() && m_probeError.isEmpty() && m_probeFps > 0.0;
    const FrameRate rate = static_cast<FrameRate>(rateIndex);
    const std::optional<double> offset = parseTimecodeSeconds(startOffset.toStdString(), rate);
    const std::optional<double> start = parseTimecodeSeconds(timecodeStart.toStdString(), rate);
    if (!offset || *offset < 0 || !start || *start < 0) {
        return tr("Invalid timecode");
    }
    VideoData data = m_data;
    if (attaching) {
        data.videoPath = QFileInfo(m_probePath).absoluteFilePath().toStdString();
        const std::string dir = scoreDir();
        data.videoRelPath = dir.empty() ? std::string()
                            : QDir(QString::fromStdString(dir)).relativeFilePath(m_probePath).toStdString();
        data.videoFps = m_probeFps;
    }
    data.rate = rate;
    data.attachBeats = attachBeats;
    data.attachUnit = attachUnit;
    data.startOffset = *offset;
    data.timecodeStart = *start;
    for (VideoMarker& m : data.markers) {
        m.time += *start - m_data.timecodeStart;
    }
    write(data, attaching ? "Attach video" : "Change video properties");
    if (attaching) {
        modes()->setVideoWindowVisible(true);
    }
    return {};
}

std::string VideoModel::scoreDir() const
{
    const project::INotationProjectPtr project = globalContext()->currentProject();
    // A new project has no file yet, so no folder to be relative to
    return project && !project->isNewlyCreated() ? QFileInfo(project->path().toQString()).absolutePath().toStdString()
           : std::string();
}

QString VideoModel::videoFile() const
{
    return QString::fromStdString(m_data.videoPath);
}

static QString rateLabel(double fps)
{
    const std::optional<FrameRate> rate = frameRateForFps(fps);
    return rate ? QString(frameRateInfo(*rate).label) : QString();
}

QString VideoModel::videoRate() const
{
    return rateLabel(m_data.videoFps);
}

int VideoModel::videoRateIndex() const
{
    const std::optional<FrameRate> rate = frameRateForFps(m_data.videoFps);
    return rate ? static_cast<int>(*rate) : -1;
}

bool VideoModel::videoMissing() const
{
    return m_data.hasVideo() && resolveVideoPath(m_data, scoreDir()).empty();
}

bool VideoModel::videoAvailable() const
{
#ifdef DORICO_HAS_VIDEO
    return true;
#else
    return false;
#endif
}

QString VideoModel::probeFile() const
{
    return m_probePath;
}

QString VideoModel::probeRate() const
{
    return rateLabel(m_probeFps);
}

int VideoModel::probeRateIndex() const
{
    const std::optional<FrameRate> rate = frameRateForFps(m_probeFps);
    return rate ? static_cast<int>(*rate) : -1;
}

void VideoModel::probeVideo(const QString& path)
{
    m_probePath = path;
    m_probeFps = 0.0;
    m_probeError.clear();
    emit probeChanged();
#ifdef DORICO_HAS_VIDEO
    // ponytail: FFmpeg's guess is the stream's nominal rate, so a variable-rate file whose rate is one of the 11 gets
    // through; reading every timestamp would catch it
    VideoDecoder decoder;
    const double fps = decoder.open(path.toStdString()) ? decoder.fps() : 0.0;
    if (fps <= 0.0) {
        m_probeError = tr("The file cannot be opened as a video.");
    } else if (!frameRateForFps(fps)) {
        m_probeError = tr("The video's frame rate (%1 fps) is not supported. Use a constant frame rate of 23.98, 24, "
                          "24.98, 25, 29.97, 30, 50, 59.94 or 60 fps.").arg(fps, 0, 'f', 3);
    } else {
        m_probeFps = fps;
    }
    emit probeChanged();
#else
    m_probeError = tr("This build cannot play video.");
    emit probeChanged();
#endif
}

void VideoModel::setVideoVolume(double volume)
{
    VideoData data = m_data;
    data.videoVolume = std::clamp(volume, 0.0, 1.0);
    write(data, "Change video volume");
}

void VideoModel::setVideoMute(bool mute)
{
    VideoData data = m_data;
    data.videoMute = mute;
    write(data, "Mute video");
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
