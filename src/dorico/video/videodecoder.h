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

#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include <QByteArray>
#include <QSize>
#include <QVideoFrame>

struct AVCodecContext;
struct AVFormatContext;
struct AVFrame;
struct SwsContext;

namespace mu::dorico {
//! Decodes a video's frames by number on a worker thread, the way Dorico's video engine does (spec 91 phase B): the
//! caller asks for frame k plus n frames ahead, the worker decodes k first (seeking to the keyframe before it when k is
//! behind or far ahead), then the n after it, and keeps them in a small cache. Frame k covers [k / fps, (k + 1) / fps)
//! from the video's first frame.
class VideoDecoder
{
public:
    VideoDecoder() = default;
    ~VideoDecoder();

    bool open(const std::string& path);
    void close();

    bool isOpen() const { return m_fmt != nullptr; }
    double fps() const { return m_fps; }
    double duration() const { return m_duration; }
    QSize size() const { return m_size; }

    //! Ask for frame `index` and `ahead` frames after it; returns frame `index` if it is decoded, else an invalid frame
    QVideoFrame frame(long long index, int ahead);

private:
    void run();
    //! Decodes until frame `target` is in the cache, or the request moves on
    void decodeTo(long long target);
    void store(long long index, const AVFrame* frame);

    AVFormatContext* m_fmt = nullptr;
    AVCodecContext* m_codec = nullptr;
    SwsContext* m_sws = nullptr;
    int m_stream = -1;
    long long m_startPts = 0;
    double m_timeBase = 0.0;
    double m_fps = 0.0;
    double m_duration = 0.0;
    QSize m_size;
    long long m_next = -1;          // the frame the decoder gives next, -1 after a seek

    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_wake;
    bool m_stop = false;
    long long m_want = -1;
    int m_ahead = 0;
    std::map<long long, QVideoFrame> m_cache;
};

//! The video's sound as a 16-bit stereo 48 kHz WAV, 0 s being the first video frame; empty if it has no sound.
//! ponytail: the whole soundtrack in memory (11.5 MB a minute), a disk cache like Dorico's if long reels need it
QByteArray extractSoundtrackWav(const std::string& path);
}
