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

#include <memory>

#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QRect>
#include <QVideoFrame>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "audio/main/iplayback.h"
#include "context/iglobalcontext.h"
#include "global/io/buffer.h"
#include "notation/imasternotation.h"
#include "playback/iplaybackcontroller.h"

#include "dorico/idoricomodestate.h"
#include "dorico/video/videodata.h"
#include "dorico/video/videodecoder.h"

class QVideoSink;

namespace mu::dorico {
//! The Video window's player (spec 91 phase B), built the way Dorico's video engine works (REA, 2026-10-23):
//! - The picture is pulled: on every display refresh tick() asks for the frame under the playhead's time, which is
//!   the last playback position the engine sent plus the time since then, sampled for when the frame reaches the
//!   screen. There is no drift correction, the picture cannot drift. Frame k shows on [k / fps, (k + 1) / fps).
//! - A decoder thread decodes the wanted frame and preloads the next ones (3 when stopped, 5 while playing).
//! - The video plays at 1.0x, whatever the tempo. Outside the video the window is black.
//! - The video's sound is a track in the audio engine, so it stays with the music.
class VideoPlayerModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(bool windowVisible READ windowVisible WRITE setWindowVisible NOTIFY windowVisibleChanged)
    //! Attached and the file was found
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY videoChanged)
    //! The playhead is inside the video and its frame is shown
    Q_PROPERTY(bool inPicture READ inPicture NOTIFY inPictureChanged)
    //! Width / height of the picture, 16 / 9 until a video is loaded
    Q_PROPERTY(double aspectRatio READ aspectRatio NOTIFY videoChanged)

    QML_ELEMENT

    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<IDoricoModeState> modes = { this };
    muse::ContextInject<muse::audio::IPlayback> playback = { this };
    muse::ContextInject<playback::IPlaybackController> playbackController = { this };

public:
    explicit VideoPlayerModel(QObject* parent = nullptr);
    ~VideoPlayerModel() override;

    bool windowVisible() const;
    void setWindowVisible(bool visible);
    bool hasVideo() const { return m_decoder.isOpen(); }
    bool inPicture() const { return m_inPicture; }
    double aspectRatio() const;

    Q_INVOKABLE void load();
    //! The window's VideoOutput.videoSink
    Q_INVOKABLE void setVideoSink(QObject* sink);
    //! Once per display refresh (the window's FrameAnimation); `refreshRate` is the screen's, in Hz
    Q_INVOKABLE void tick(double refreshRate);

    //! The window's size and place, kept for every project (empty until the window was first moved)
    Q_INVOKABLE QRect savedGeometry() const;
    Q_INVOKABLE void saveGeometry(int x, int y, int width, int height);
    //! Shift-drag on an edge keeps the picture's shape
    Q_INVOKABLE bool shiftHeld() const;

signals:
    void windowVisibleChanged();
    void videoChanged();
    void inPictureChanged();

private:
    void listenToScore();
    void readData();
    void anchor(double position);
    void setInPicture(bool in);

    void loadSoundtrack();
    void addSoundtrack();
    void removeSoundtrack();
    muse::audio::TrackParams soundtrackParams() const;

    VideoDecoder m_decoder;
    QVideoSink* m_sink = nullptr;
    long long m_shown = -1;     // the frame in the sink, -1 for none
    notation::IMasterNotationPtr m_master;
    VideoData m_data;
    QString m_file;             // the resolved file, empty if missing
    double m_offset = 0.0;      // the video's time at playhead time 0
    bool m_inPicture = false;

    // The clock: the playhead's position when the engine last sent it, and the time since
    double m_anchor = 0.0;
    QElapsedTimer m_sinceAnchor;
    bool m_running = false;

    QByteArray m_wav;           // the soundtrack, read once per file
    muse::ByteArray m_wavBytes; // m_wav's bytes, for m_wavDevice
    std::unique_ptr<muse::io::Buffer> m_wavDevice;
    muse::audio::TrackId m_track = muse::audio::INVALID_TRACK_ID;
    bool m_adding = false;
    int m_generation = 0;       // drops a soundtrack read for a file that is no longer attached
};
}
