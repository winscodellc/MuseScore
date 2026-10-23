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

#include "videodecoder.h"

#include <cmath>
#include <cstring>
#include <vector>

#include <QVideoFrameFormat>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include "log.h"

using namespace mu::dorico;

static long long framePts(const AVFrame* f)
{
    return f->best_effort_timestamp != AV_NOPTS_VALUE ? f->best_effort_timestamp : f->pts;
}

static AVCodecContext* openDecoder(AVFormatContext* fmt, AVMediaType type, int* stream)
{
    const AVCodec* codec = nullptr;
    *stream = av_find_best_stream(fmt, type, -1, -1, &codec, 0);
    if (*stream < 0 || !codec) {
        return nullptr;
    }
    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(ctx, fmt->streams[*stream]->codecpar);
    ctx->thread_count = 0; // as many as the machine has
    if (avcodec_open2(ctx, codec, nullptr) < 0) {
        avcodec_free_context(&ctx);
        return nullptr;
    }
    return ctx;
}

VideoDecoder::~VideoDecoder()
{
    close();
}

bool VideoDecoder::open(const std::string& path)
{
    close();
    if (avformat_open_input(&m_fmt, path.c_str(), nullptr, nullptr) < 0) {
        return false;
    }
    avformat_find_stream_info(m_fmt, nullptr);
    m_codec = openDecoder(m_fmt, AVMEDIA_TYPE_VIDEO, &m_stream);
    if (!m_codec) {
        LOGW() << "no video stream in " << path;
        close();
        return false;
    }

    const AVStream* st = m_fmt->streams[m_stream];
    m_timeBase = av_q2d(st->time_base);
    m_startPts = st->start_time != AV_NOPTS_VALUE ? st->start_time : 0;
    m_fps = av_q2d(av_guess_frame_rate(m_fmt, const_cast<AVStream*>(st), nullptr));
    m_duration = st->duration != AV_NOPTS_VALUE ? st->duration * m_timeBase : double(m_fmt->duration) / AV_TIME_BASE;
    m_size = QSize(m_codec->width, m_codec->height);
    m_next = -1;

    m_stop = false;
    m_want = -1;
    m_thread = std::thread([this]() { run(); });
    return true;
}

void VideoDecoder::close()
{
    if (m_thread.joinable()) {
        {
            std::lock_guard lock(m_mutex);
            m_stop = true;
        }
        m_wake.notify_all();
        m_thread.join();
    }
    m_cache.clear();
    sws_freeContext(m_sws);
    m_sws = nullptr;
    avcodec_free_context(&m_codec);
    avformat_close_input(&m_fmt);
    m_fps = 0.0;
    m_duration = 0.0;
}

QVideoFrame VideoDecoder::frame(long long index, int ahead)
{
    std::lock_guard lock(m_mutex);
    if (index != m_want || ahead != m_ahead) {
        m_want = index;
        m_ahead = ahead;
        // Keep the frames of the new request only (Dorico's caches hold 4 to 10 frames)
        for (auto it = m_cache.begin(); it != m_cache.end();) {
            it = it->first < index || it->first > index + ahead ? m_cache.erase(it) : std::next(it);
        }
        m_wake.notify_all();
    }
    const auto it = m_cache.find(index);
    return it == m_cache.end() ? QVideoFrame() : it->second;
}

void VideoDecoder::run()
{
    const long long frameCount = std::llround(m_duration * m_fps);
    long long failed = -1;
    std::unique_lock lock(m_mutex);
    while (!m_stop) {
        // The first frame of the request not decoded yet: the asked-for one first, then the ones after it
        long long target = -1;
        for (long long k = m_want; k >= 0 && k <= m_want + m_ahead && k < frameCount; ++k) {
            if (!m_cache.count(k) && k != failed) {
                target = k;
                break;
            }
        }
        if (target < 0) {
            m_wake.wait(lock);
            continue;
        }
        lock.unlock();
        decodeTo(target);
        lock.lock();
        // A frame the file does not give (past its last frame, a damaged stream) is not asked for again in a loop
        const bool stillWanted = target >= m_want && target <= m_want + m_ahead;
        if (stillWanted && !m_cache.count(target)) {
            failed = target;
        }
    }
}

void VideoDecoder::decodeTo(long long target)
{
    // Seek to the keyframe before the target when it is behind the decoder or more than a second ahead of it
    if (m_next < 0 || target < m_next || target > m_next + std::llround(m_fps)) {
        const long long ts = m_startPts + std::llround(double(target) / m_fps / m_timeBase);
        av_seek_frame(m_fmt, m_stream, ts, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(m_codec);
        m_next = -1;
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* f = av_frame_alloc();
    bool draining = false;
    for (;;) {
        const int r = avcodec_receive_frame(m_codec, f);
        if (r == 0) {
            const long long index = std::llround((framePts(f) - m_startPts) * m_timeBase * m_fps);
            m_next = index + 1;
            bool wanted;
            {
                std::lock_guard lock(m_mutex);
                wanted = index >= m_want && index <= m_want + m_ahead;
                if (target < m_want || target > m_want + m_ahead) {
                    av_frame_unref(f);
                    break; // the request moved on
                }
            }
            if (wanted) {
                store(index, f);
            }
            if (index > target) {
                store(target, f); // the stream skipped the target (a gap, an inexact seek): the next frame stands in
            }
            av_frame_unref(f);
            if (index >= target) {
                break;
            }
            continue;
        }
        if (r == AVERROR_EOF || (draining && r == AVERROR(EAGAIN))) {
            m_next = -1;
            break;
        }
        if (av_read_frame(m_fmt, packet) < 0) {
            avcodec_send_packet(m_codec, nullptr);
            draining = true;
            continue;
        }
        if (packet->stream_index == m_stream) {
            avcodec_send_packet(m_codec, packet);
        }
        av_packet_unref(packet);
    }
    av_frame_free(&f);
    av_packet_free(&packet);
}

void VideoDecoder::store(long long index, const AVFrame* f)
{
    QVideoFrame frame(QVideoFrameFormat(QSize(f->width, f->height), QVideoFrameFormat::Format_YUV420P));
    if (!frame.map(QVideoFrame::WriteOnly)) {
        return;
    }
    m_sws = sws_getCachedContext(m_sws, f->width, f->height, AVPixelFormat(f->format), f->width, f->height,
                                 AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr);
    uint8_t* dst[4] = { frame.bits(0), frame.bits(1), frame.bits(2), nullptr };
    const int dstStride[4] = { frame.bytesPerLine(0), frame.bytesPerLine(1), frame.bytesPerLine(2), 0 };
    sws_scale(m_sws, f->data, f->linesize, 0, f->height, dst, dstStride);
    frame.unmap();
    frame.setStartTime(std::llround(index / m_fps * 1e6));

    std::lock_guard lock(m_mutex);
    if (index >= m_want && index <= m_want + m_ahead) {
        m_cache[index] = frame;
    }
}

QByteArray mu::dorico::extractSoundtrackWav(const std::string& path)
{
    constexpr int RATE = 48000;

    AVFormatContext* fmt = nullptr;
    if (avformat_open_input(&fmt, path.c_str(), nullptr, nullptr) < 0) {
        return {};
    }
    avformat_find_stream_info(fmt, nullptr);
    int audio = -1;
    AVCodecContext* codec = openDecoder(fmt, AVMEDIA_TYPE_AUDIO, &audio);
    if (!codec) {
        avformat_close_input(&fmt);
        return {};
    }

    // 0 s is the first video frame
    const int video = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    const AVStream* vst = video >= 0 ? fmt->streams[video] : nullptr;
    const double videoStart = vst && vst->start_time != AV_NOPTS_VALUE ? vst->start_time * av_q2d(vst->time_base) : 0.0;
    const double audioTimeBase = av_q2d(fmt->streams[audio]->time_base);

    std::vector<int16_t> pcm; // interleaved stereo
    long long skip = 0;       // samples (per channel) to drop when the sound starts before the picture
    SwrContext* swr = nullptr;
    std::vector<int16_t> buf;

    auto convert = [&](const AVFrame* f) {
        const int max = swr_get_out_samples(swr, f ? f->nb_samples : 0);
        if (max <= 0) {
            return;
        }
        buf.resize(size_t(max) * 2);
        uint8_t* out = reinterpret_cast<uint8_t*>(buf.data());
        const int n = swr_convert(swr, &out, max, f ? const_cast<const uint8_t**>(f->extended_data) : nullptr,
                                  f ? f->nb_samples : 0);
        const long long dropped = std::min<long long>(skip, std::max(n, 0));
        skip -= dropped;
        pcm.insert(pcm.end(), buf.begin() + dropped * 2, buf.begin() + std::max(n, 0) * 2);
    };

    AVPacket* packet = av_packet_alloc();
    AVFrame* f = av_frame_alloc();
    bool draining = false;
    for (;;) {
        const int r = avcodec_receive_frame(codec, f);
        if (r == 0) {
            if (!swr) {
                // Place the sound against the picture by the first frame's time
                const long long start = std::llround((framePts(f) * audioTimeBase - videoStart) * RATE);
                if (start > 0) {
                    pcm.assign(size_t(start) * 2, 0);
                } else {
                    skip = -start;
                }
                AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
                swr_alloc_set_opts2(&swr, &stereo, AV_SAMPLE_FMT_S16, RATE, &f->ch_layout, AVSampleFormat(f->format),
                                    f->sample_rate, 0, nullptr);
                if (!swr || swr_init(swr) < 0) {
                    break;
                }
            }
            convert(f);
            av_frame_unref(f);
            continue;
        }
        if (r == AVERROR_EOF || (draining && r == AVERROR(EAGAIN))) {
            break;
        }
        if (av_read_frame(fmt, packet) < 0) {
            avcodec_send_packet(codec, nullptr);
            draining = true;
            continue;
        }
        if (packet->stream_index == audio) {
            avcodec_send_packet(codec, packet);
        }
        av_packet_unref(packet);
    }
    if (swr) {
        convert(nullptr); // what the resampler still holds
    }
    swr_free(&swr);
    av_frame_free(&f);
    av_packet_free(&packet);
    avcodec_free_context(&codec);
    avformat_close_input(&fmt);

    if (pcm.empty()) {
        return {};
    }

    // RIFF / WAVE, PCM 16 bit stereo
    const uint32_t dataBytes = uint32_t(pcm.size() * 2);
    QByteArray wav;
    wav.reserve(44 + dataBytes);
    auto put = [&wav](uint32_t v, int bytes) {
        for (int i = 0; i < bytes; ++i) {
            wav.append(char((v >> (8 * i)) & 0xff));
        }
    };
    wav.append("RIFF");
    put(36 + dataBytes, 4);
    wav.append("WAVEfmt ");
    put(16, 4);
    put(1, 2);             // PCM
    put(2, 2);             // channels
    put(RATE, 4);
    put(RATE * 4, 4);      // bytes per second
    put(4, 2);             // block align
    put(16, 2);            // bits
    wav.append("data");
    put(dataBytes, 4);
    wav.append(reinterpret_cast<const char*>(pcm.data()), dataBytes);
    return wav;
}
