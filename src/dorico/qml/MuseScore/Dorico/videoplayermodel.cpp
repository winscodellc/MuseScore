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

#include "videoplayermodel.h"

#include <algorithm>
#include <cmath>
#include <thread>

#include <QFileInfo>
#include <QGuiApplication>
#include <QPointer>
#include <QVideoSink>

#include "engraving/dom/masterscore.h"
#include "notation/inotation.h"
#include "notation/inotationundostack.h"
#include "project/inotationproject.h"
#include "settings.h"

#include "log.h"

using namespace mu::dorico;
using namespace muse::audio;

static const muse::Settings::Key VIDEO_WINDOW_GEOMETRY("dorico", "videoWindowGeometry");

//! Frames decoded ahead of the shown one: after a move of the playhead, and while playing (Dorico's preload counts)
static constexpr int AHEAD_STOPPED = 3;
static constexpr int AHEAD_PLAYING = 5;
//! The clock runs on from the last position the engine sent for at most this long (a count-in, a stalled engine)
static constexpr double MAX_EXTRAPOLATION = 0.1;

VideoPlayerModel::VideoPlayerModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

VideoPlayerModel::~VideoPlayerModel() = default;

void VideoPlayerModel::load()
{
    modes()->modesChanged().onNotify(this, [this]() { emit windowVisibleChanged(); });
    globalContext()->currentMasterNotationChanged().onNotify(this, [this]() { listenToScore(); });

    const context::IPlaybackStatePtr state = globalContext()->playbackState();
    state->playbackPositionChanged().onReceive(this, [this](secs_t position) { anchor(position); });
    state->playbackStatusChanged().onReceive(this, [this, state](PlaybackStatus status) {
        m_running = status == PlaybackStatus::Running;
        anchor(state->playbackPosition());
    });
    m_running = state->isPlaying();
    anchor(state->playbackPosition());

    // Setting up playback (another score, say) removes every engine track: add the soundtrack again
    playbackController()->playbackInitedChanged().onReceive(this, [this](bool inited) {
        ++m_generation;
        m_adding = false;
        m_track = INVALID_TRACK_ID;
        if (inited) {
            addSoundtrack();
        }
    });

    listenToScore();
}

void VideoPlayerModel::anchor(double position)
{
    m_anchor = position;
    m_sinceAnchor.restart();
}

void VideoPlayerModel::listenToScore()
{
    if (m_master) {
        m_master->notation()->undoStack()->stackChanged().disconnect(this);
    }
    m_master = globalContext()->currentMasterNotation();
    if (m_master) {
        m_master->notation()->undoStack()->stackChanged().onNotify(this, [this]() { readData(); });
    }
    readData();
}

void VideoPlayerModel::readData()
{
    m_data = m_master ? readVideoData(m_master->masterScore()) : VideoData {};
    m_offset = m_master ? m_data.videoSeconds(timeMap(m_master->masterScore(), m_data).flowStart()) : 0.0;

    const project::INotationProjectPtr project = globalContext()->currentProject();
    const std::string dir = project ? QFileInfo(project->path().toQString()).absolutePath().toStdString() : std::string();
    const QString file = QString::fromStdString(resolveVideoPath(m_data, dir));
    if (file != m_file) {
        m_file = file;
        m_shown = -1;
        if (file.isEmpty() || !m_decoder.open(file.toStdString())) {
            m_decoder.close();
        }
        loadSoundtrack();
        emit videoChanged();
        emit windowVisibleChanged();
    } else if (m_track != INVALID_TRACK_ID) {
        // The offset, level or mute may have changed
        const TrackParams params = soundtrackParams();
        playback()->setSourceParams(m_track, params.source);
        playback()->setControlParams(m_track, params.control);
    }
}

void VideoPlayerModel::tick(double refreshRate)
{
    if (!m_sink || !m_decoder.isOpen()) {
        setInPicture(false);
        return;
    }

    // While playing, the time is the one at which this frame reaches the screen, a refresh from now
    double time = m_anchor;
    if (m_running) {
        time += std::min(m_sinceAnchor.nsecsElapsed() / 1e9, MAX_EXTRAPOLATION) + (refreshRate > 0.0 ? 1.0 / refreshRate : 0.0);
    }
    const double videoSecs = m_offset + time;
    if (videoSecs < 0.0 || videoSecs >= m_decoder.duration()) {
        m_shown = -1;
        setInPicture(false);
        return;
    }

    const long long index = videoFrameAt(videoSecs, m_decoder.fps());
    const QVideoFrame frame = m_decoder.frame(index, m_running ? AHEAD_PLAYING : AHEAD_STOPPED);
    if (frame.isValid() && index != m_shown) {
        m_sink->setVideoFrame(frame);
        m_shown = index;
    }
    // Until the first frame is decoded the window stays black; after that the last frame stays up until the next
    setInPicture(m_shown >= 0);
}

void VideoPlayerModel::loadSoundtrack()
{
    removeSoundtrack();
    m_wav.clear();
    m_wavDevice.reset();
    if (m_file.isEmpty()) {
        return;
    }
    // Read on a worker thread: a few seconds for a long video
    QPointer<VideoPlayerModel> self(this);
    std::thread([self, file = m_file]() {
        QByteArray wav = extractSoundtrackWav(file.toStdString());
        QMetaObject::invokeMethod(qApp, [self, file, wav = std::move(wav)]() {
            if (!self || self->m_file != file) {
                return;
            }
            self->m_wav = wav;
            self->addSoundtrack();
        });
    }).detach();
}

void VideoPlayerModel::addSoundtrack()
{
    if (m_wav.isEmpty() || m_track != INVALID_TRACK_ID || m_adding || !playbackController()->isPlaybackInited()) {
        return;
    }
    m_wavBytes = muse::ByteArray::fromRawData(reinterpret_cast<const uint8_t*>(m_wav.constData()), size_t(m_wav.size()));
    m_wavDevice = std::make_unique<muse::io::Buffer>(&m_wavBytes);
    m_adding = true;
    const int generation = m_generation;
    playback()->addTrack("Video", m_wavDevice.get(), soundtrackParams())
    .onResolve(this, [this, generation](TrackId track, const TrackParams&) {
        if (generation != m_generation) {
            return; // removed meanwhile, or playback was set up again (which removed it)
        }
        m_adding = false;
        m_track = track;
        const TrackParams params = soundtrackParams(); // the score may have changed meanwhile
        playback()->setSourceParams(m_track, params.source);
        playback()->setControlParams(m_track, params.control);
    })
    .onReject(this, [this, generation](int code, const std::string& msg) {
        if (generation == m_generation) {
            m_adding = false;
        }
        LOGE() << "can't add the video's soundtrack, code: [" << code << "] " << msg;
    });
}

void VideoPlayerModel::removeSoundtrack()
{
    ++m_generation;
    m_adding = false;
    if (m_track != INVALID_TRACK_ID) {
        playback()->removeTrack(m_track);
        m_track = INVALID_TRACK_ID;
    }
}

TrackParams VideoPlayerModel::soundtrackParams() const
{
    TrackParams params;
    params.source.configuration["offsetUs"] = std::to_string(std::llround(m_offset * 1e6));
    params.control.volume = volume_db_t(m_data.videoVolume > 0.001 ? muse::linear_to_db(float(m_data.videoVolume)) : -60.f);
    params.control.muted = m_data.videoMute;
    return params;
}

void VideoPlayerModel::setInPicture(bool in)
{
    if (m_inPicture != in) {
        m_inPicture = in;
        emit inPictureChanged();
    }
}

bool VideoPlayerModel::windowVisible() const
{
    // Shown only while there is a video to show (after undoing Attach, say)
    return modes()->isVideoWindowVisible() && !m_file.isEmpty();
}

void VideoPlayerModel::setWindowVisible(bool visible)
{
    modes()->setVideoWindowVisible(visible);
}

double VideoPlayerModel::aspectRatio() const
{
    const QSize size = m_decoder.size();
    return size.isEmpty() ? 16.0 / 9.0 : double(size.width()) / size.height();
}

void VideoPlayerModel::setVideoSink(QObject* sink)
{
    m_sink = qobject_cast<QVideoSink*>(sink);
    m_shown = -1;
}

QRect VideoPlayerModel::savedGeometry() const
{
    const QStringList parts = QString::fromStdString(muse::settings()->value(VIDEO_WINDOW_GEOMETRY).toString()).split(',');
    return parts.size() == 4 ? QRect(parts[0].toInt(), parts[1].toInt(), parts[2].toInt(), parts[3].toInt()) : QRect();
}

void VideoPlayerModel::saveGeometry(int x, int y, int width, int height)
{
    muse::settings()->setSharedValue(VIDEO_WINDOW_GEOMETRY, muse::Val(QString("%1,%2,%3,%4").arg(x).arg(y).arg(width).arg(height)
                                                                       .toStdString()));
}

bool VideoPlayerModel::shiftHeld() const
{
    return QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
}
