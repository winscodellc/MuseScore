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

#include "timecode.h"

#include <cmath>
#include <cstdio>

#include "engraving/dom/tempotimeline.h"

namespace mu::dorico {
const std::vector<FrameRateInfo>& frameRates()
{
    static const std::vector<FrameRateInfo> rates = {
        { FrameRate::Fps23_976,    24000, 1001, 24, 0, "23.98 fps",  "23.976" },
        { FrameRate::Fps24,        24,    1,    24, 0, "24 fps",     "24" },
        { FrameRate::Fps24_975,    25000, 1001, 25, 0, "24.98 fps",  "24.975" },
        { FrameRate::Fps25,        25,    1,    25, 0, "25 fps",     "25" },
        { FrameRate::Fps29_97,     30000, 1001, 30, 0, "29.97 fps",  "29.97" },
        { FrameRate::Fps29_97Drop, 30000, 1001, 30, 2, "29.97 dfps", "29.97df" },
        { FrameRate::Fps30,        30,    1,    30, 0, "30 fps",     "30" },
        { FrameRate::Fps50,        50,    1,    50, 0, "50 fps",     "50" },
        { FrameRate::Fps59_94,     60000, 1001, 60, 0, "59.94 fps",  "59.94" },
        { FrameRate::Fps59_94Drop, 60000, 1001, 60, 4, "59.94 dfps", "59.94df" },
        { FrameRate::Fps60,        60,    1,    60, 0, "60 fps",     "60" },
    };
    return rates;
}

const FrameRateInfo& frameRateInfo(FrameRate rate)
{
    return frameRates()[static_cast<size_t>(rate)];
}

std::optional<FrameRate> frameRateFromKey(const std::string& key)
{
    for (const FrameRateInfo& info : frameRates()) {
        if (key == info.key) {
            return info.rate;
        }
    }
    return std::nullopt;
}

long long secondsToFrames(double seconds, FrameRate rate, bool nearest)
{
    const FrameRateInfo& r = frameRateInfo(rate);
    const double frames = seconds * r.num / r.den;
    return nearest ? std::llround(frames) : static_cast<long long>(std::floor(frames + 1e-6));
}

double framesToSeconds(long long frames, FrameRate rate)
{
    const FrameRateInfo& r = frameRateInfo(rate);
    return double(frames) * r.den / r.num;
}

// Drop frame skips the frame numbers 00 and 01 (00-03 at 59.94) at the start of every minute except every tenth one,
// so that the timecode keeps up with real time.
Timecode framesToTimecode(long long frames, FrameRate rate)
{
    const FrameRateInfo& r = frameRateInfo(rate);
    Timecode tc;
    tc.negative = frames < 0;
    long long n = tc.negative ? -frames : frames;

    if (r.drop) {
        const long long perMinute = 60LL * r.nominal - r.drop;
        const long long perTenMinutes = 10 * perMinute + r.drop;
        const long long tens = n / perTenMinutes;
        const long long rest = n % perTenMinutes;
        n += 9LL * r.drop * tens;
        if (rest > r.drop) {
            n += r.drop * ((rest - r.drop) / perMinute);
        }
    }

    tc.frames = int(n % r.nominal);
    n /= r.nominal;
    tc.seconds = int(n % 60);
    n /= 60;
    tc.minutes = int(n % 60);
    tc.hours = int(n / 60);
    return tc;
}

long long timecodeToFrames(const Timecode& tc, FrameRate rate)
{
    const FrameRateInfo& r = frameRateInfo(rate);
    const long long totalMinutes = 60LL * tc.hours + tc.minutes;
    long long n = ((totalMinutes * 60) + tc.seconds) * r.nominal + tc.frames;
    n -= r.drop * (totalMinutes - totalMinutes / 10);
    return tc.negative ? -n : n;
}

std::string formatTimecode(long long frames, FrameRate rate)
{
    const Timecode tc = framesToTimecode(frames, rate);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%s%02d:%02d:%02d%c%02d", tc.negative ? "-" : "", tc.hours, tc.minutes, tc.seconds,
                  frameRateInfo(rate).drop ? ';' : ':', tc.frames);
    return buf;
}

std::string formatTimecodeSeconds(double seconds, FrameRate rate, bool nearest)
{
    return formatTimecode(secondsToFrames(seconds, rate, nearest), rate);
}

std::optional<long long> parseTimecode(const std::string& text, FrameRate rate)
{
    std::vector<int> fields;
    bool negative = false;
    bool inNumber = false;
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c >= '0' && c <= '9') {
            if (!inNumber) {
                fields.push_back(0);
                inNumber = true;
            }
            if (fields.back() > 100000) {
                return std::nullopt;
            }
            fields.back() = fields.back() * 10 + (c - '0');
        } else if (c == ':' || c == ';' || c == '.') {
            if (!inNumber) {
                return std::nullopt;
            }
            inNumber = false;
        } else if (c == '-' && fields.empty() && !negative) {
            negative = true;
        } else if (c != ' ') {
            return std::nullopt;
        }
    }
    if (fields.empty() || fields.size() > 4 || !inNumber) {
        return std::nullopt;
    }

    fields.insert(fields.begin(), 4 - fields.size(), 0);
    Timecode tc { negative, fields[0], fields[1], fields[2], fields[3] };
    if (tc.minutes > 59 || tc.seconds > 59 || tc.frames >= frameRateInfo(rate).nominal) {
        return std::nullopt;
    }
    return timecodeToFrames(tc, rate);
}

std::optional<double> parseTimecodeSeconds(const std::string& text, FrameRate rate)
{
    const std::optional<long long> frames = parseTimecode(text, rate);
    if (!frames) {
        return std::nullopt;
    }
    return framesToSeconds(*frames, rate);
}

TimeMap::TimeMap(const TimeSettings& settings, const engraving::TempoTimeline& timeline)
    : m_settings(settings), m_timeline(timeline)
{
}

double TimeMap::flowStart() const
{
    return m_settings.timecodeStart + m_settings.startOffset - musicSeconds(m_settings.attachTicks);
}

double TimeMap::musicSeconds(int tick) const
{
    return m_timeline.utick2utime(tick) * m_timeline.tempoMultiplier().val;
}

int TimeMap::tickAt(double timecodeSeconds) const
{
    return m_timeline.utime2utick((timecodeSeconds - flowStart()) / m_timeline.tempoMultiplier().val);
}
}
