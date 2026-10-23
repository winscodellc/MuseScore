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

#include <string>
#include <utility>
#include <vector>

#include "timecode.h"

namespace mu::engraving {
class Score;
}

//! The score's video settings and markers (spec 91), stored as JSON in the master score's metaTag "doricoVideo".
//! A build without the fork carries the tag along untouched.
namespace mu::dorico {
inline constexpr const char16_t* VIDEO_TAG = u"doricoVideo";

//! A marker is locked to time, not to a beat
struct VideoMarker {
    int id = 0;
    double time = 0.0;  // timecode, in seconds
    std::string text;
    bool important = false;

    bool operator==(const VideoMarker&) const = default;
};

struct VideoData {
    FrameRate rate = FrameRate::Fps25;
    bool useVideoRate = true;
    double timecodeStart = 0.0;
    int attachBeats = 0;
    int attachUnit = 480;       // ticks of the beat unit (a quarter)
    double startOffset = 0.0;
    int nextId = 1;
    std::vector<VideoMarker> markers; // sorted by time

    TimeSettings timeSettings() const { return { timecodeStart, attachBeats * attachUnit, startOffset }; }
    bool operator==(const VideoData&) const = default;
};

VideoData videoDataFromJson(const std::string& json);
std::string videoDataToJson(const VideoData& data);

VideoData readVideoData(const engraving::Score* score);
//! The first-pass tempo timeline, which page and lane positions use
TimeMap timeMap(const engraving::Score* score, const VideoData& data);

//! Each marker's raw tick and text, for markers inside the flow
std::vector<std::pair<int, std::string> > markerTicks(const engraving::Score* score);
}
