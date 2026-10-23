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

#include <optional>
#include <string>
#include <vector>

namespace mu::engraving {
class TempoTimeline;
}

//! Timecodes and the score <-> picture time map (spec 91). No UI and no score, so it can be unit tested.
//! Every time here is real seconds: a frame lasts den / num seconds, and timecode 00:00:00:00 is 0 s.
namespace mu::dorico {
//! Dorico's 11 project frame rates, in its menu order
enum class FrameRate {
    Fps23_976, Fps24, Fps24_975, Fps25, Fps29_97, Fps29_97Drop, Fps30, Fps50, Fps59_94, Fps59_94Drop, Fps60
};

struct FrameRateInfo {
    FrameRate rate;
    int num;            // frames per second = num / den
    int den;
    int nominal;        // frames counted per timecode second (30 for 29.97)
    int drop;           // frame numbers skipped per minute, 0 for non-drop
    const char* label;  // as Dorico shows it, e.g. "29.97 dfps"
    const char* key;    // stored in the score
};

const std::vector<FrameRateInfo>& frameRates();
const FrameRateInfo& frameRateInfo(FrameRate rate);
std::optional<FrameRate> frameRateFromKey(const std::string& key);

//! The frame that is showing at `seconds` (rounds down, with a little slack for float error), or with `nearest` the
//! closest frame (for marker times, which are seconds and rarely fall on a frame at 29.97 / 59.94)
long long secondsToFrames(double seconds, FrameRate rate, bool nearest = false);
double framesToSeconds(long long frames, FrameRate rate);

struct Timecode {
    bool negative = false;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    int frames = 0;
};

Timecode framesToTimecode(long long frames, FrameRate rate);
long long timecodeToFrames(const Timecode& tc, FrameRate rate);

//! "hh:mm:ss:ff", with ';' before the frames for drop frame and a leading '-' before 00:00:00:00
std::string formatTimecode(long long frames, FrameRate rate);
std::string formatTimecodeSeconds(double seconds, FrameRate rate, bool nearest = false);

//! Accepts ':' or ';' (or '.'), unpadded fields and fewer than four fields (right-aligned: "10:12" is 10 s 12 frames).
//! Returns frames, or nothing if a field is out of range.
std::optional<long long> parseTimecode(const std::string& text, FrameRate rate);
std::optional<double> parseTimecodeSeconds(const std::string& text, FrameRate rate);

//! Where the flow sits against the timecode (Video Properties)
struct TimeSettings {
    double timecodeStart = 0.0;   // the timecode at the start of the video, in seconds
    int attachTicks = 0;          // flow attachment position (beat count x beat unit), in raw ticks from the flow start
    double startOffset = 0.0;     // position in the video that lands on the attachment position
};

//! Music time comes from the first-pass tempo timeline, at nominal tempo (the playback tempo multiplier is ignored)
class TimeMap
{
public:
    TimeMap(const TimeSettings& settings, const engraving::TempoTimeline& timeline);

    //! The timecode at the start of the flow: timecode start + start offset - music time of the attachment position
    double flowStart() const;
    double musicSeconds(int tick) const;
    int tickAt(double timecodeSeconds) const;

private:
    TimeSettings m_settings;
    const engraving::TempoTimeline& m_timeline;
};
}
