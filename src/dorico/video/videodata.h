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
#include "notation/imasternotation.h"

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

    // The attached video (phase B): none while the path is empty
    std::string videoPath;          // absolute
    std::string videoRelPath;       // relative to the score file, tried when the absolute path is missing
    double videoFps = 0.0;          // read from the file
    double videoVolume = 0.5;       // linear, 0..1, from the Mixer's Video strip (-6.02 dB, Dorico's default)
    bool videoMute = false;

    bool hasVideo() const { return !videoPath.empty(); }
    //! Where the video is at a timecode: 0 s is the video's first frame
    double videoSeconds(double timecodeSeconds) const { return timecodeSeconds - timecodeStart; }

    TimeSettings timeSettings() const { return { timecodeStart, attachBeats * attachUnit, startOffset }; }
    bool operator==(const VideoData&) const = default;
};

VideoData videoDataFromJson(const std::string& json);
std::string videoDataToJson(const VideoData& data);

VideoData readVideoData(const engraving::Score* score);
//! The attached video's file: the absolute path, else the relative one from the score's folder; empty if neither exists
std::string resolveVideoPath(const VideoData& data, const std::string& scoreDir);

//! One undo step; markers are sorted by time first. Does nothing if the data is unchanged.
void writeVideoData(const notation::IMasterNotationPtr& master, const VideoData& data, const char* undoTitle);
//! The first-pass tempo timeline, which page and lane positions use
TimeMap timeMap(const engraving::Score* score, const VideoData& data);

//! Each marker's raw tick and text, for markers inside the flow
std::vector<std::pair<int, std::string> > markerTicks(const engraving::Score* score);
}
