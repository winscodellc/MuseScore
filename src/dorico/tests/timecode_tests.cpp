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

#include <gtest/gtest.h>

#include "dorico/video/timecode.h"
#include "dorico/video/videodata.h"
#include "engraving/dom/tempotimeline.h"

using namespace mu::dorico;
using mu::engraving::TempoTimeline;

TEST(DoricoTimecode, EveryRateRoundTripsAnHourOfFrames)
{
    for (const FrameRateInfo& r : frameRates()) {
        const long long hour = secondsToFrames(3600.0, r.rate);
        for (long long f = 0; f < hour; f += 7) {
            const std::optional<long long> back = parseTimecode(formatTimecode(f, r.rate), r.rate);
            ASSERT_TRUE(back.has_value()) << r.label << " " << f;
            ASSERT_EQ(*back, f) << r.label << " " << formatTimecode(f, r.rate);
            ASSERT_EQ(secondsToFrames(framesToSeconds(f, r.rate), r.rate), f) << r.label;
        }
    }
}

// A marker 1 s after a 29.97 dfps start lands 29.97 frames in: it shows as the next whole second, the playhead as ;29
TEST(DoricoTimecode, MarkerTimesRoundToTheNearestFrame)
{
    const double start = framesToSeconds(*parseTimecode("01:00:00;00", FrameRate::Fps29_97Drop), FrameRate::Fps29_97Drop);
    EXPECT_EQ(formatTimecodeSeconds(start + 1.0, FrameRate::Fps29_97Drop, true), "01:00:01;00");
    EXPECT_EQ(formatTimecodeSeconds(start + 1.0, FrameRate::Fps29_97Drop), "01:00:00;29");
}

TEST(DoricoTimecode, DropFrameSkipsFramesEveryMinuteButTheTenth)
{
    EXPECT_EQ(formatTimecode(1799, FrameRate::Fps29_97Drop), "00:00:59;29");
    EXPECT_EQ(formatTimecode(1800, FrameRate::Fps29_97Drop), "00:01:00;02");
    EXPECT_EQ(formatTimecode(17981, FrameRate::Fps29_97Drop), "00:09:59;29");
    EXPECT_EQ(formatTimecode(17982, FrameRate::Fps29_97Drop), "00:10:00;00");
    // 59.94 drops four
    EXPECT_EQ(formatTimecode(3600, FrameRate::Fps59_94Drop), "00:01:00;04");
    EXPECT_EQ(formatTimecode(35964, FrameRate::Fps59_94Drop), "00:10:00;00");
    // An hour of drop-frame timecode is an hour of real time (to a frame)
    EXPECT_NEAR(framesToSeconds(*parseTimecode("01:00:00;00", FrameRate::Fps29_97Drop), FrameRate::Fps29_97Drop), 3600.0, 0.04);
    // Non-drop 29.97 runs slow
    EXPECT_EQ(formatTimecode(1800, FrameRate::Fps29_97), "00:01:00:00");
}

TEST(DoricoTimecode, ParsingAcceptsSeparatorsShortAndNegativeInput)
{
    const FrameRate r = FrameRate::Fps25;
    EXPECT_EQ(parseTimecode("1:00:10:12", r), parseTimecode("01:00:10:12", r));
    EXPECT_EQ(parseTimecode("01;00;10;12", r), parseTimecode("01:00:10:12", r));
    EXPECT_EQ(parseTimecode("10:12", r), 10 * 25 + 12);
    EXPECT_EQ(parseTimecode("-00:00:08:00", r), -200);
    EXPECT_EQ(formatTimecode(-200, r), "-00:00:08:00");
    EXPECT_FALSE(parseTimecode("00:00:00:25", r));
    EXPECT_FALSE(parseTimecode("00:60:00:00", r));
    EXPECT_FALSE(parseTimecode("1:2:3:4:5", r));
    EXPECT_FALSE(parseTimecode("abc", r));
    EXPECT_FALSE(parseTimecode("", r));
    EXPECT_FALSE(parseTimecode("10:", r));
}

TEST(DoricoTimecode, ManualExampleFlowStart)
{
    // Timecode start 02:00:00:00, attached at 8 quarters (bar 3 in 4/4), 60 bpm: the flow starts at 01:59:52:00
    TempoTimeline tl;
    tl.rebuild(mu::engraving::TempoValues { { 0, 1.0 } }, {});
    const TimeSettings s { *parseTimecodeSeconds("02:00:00:00", FrameRate::Fps25), 8 * 480, 0.0 };
    const TimeMap map(s, tl);
    EXPECT_EQ(formatTimecodeSeconds(map.flowStart(), FrameRate::Fps25), "01:59:52:00");
    // The video's first frame is on beat 8
    EXPECT_EQ(map.tickAt(s.timecodeStart), 8 * 480);

    // A 5 s start offset puts the video's fifth second on beat 8
    const TimeMap offset({ s.timecodeStart, 8 * 480, 5.0 }, tl);
    EXPECT_EQ(offset.tickAt(s.timecodeStart + 5.0), 8 * 480);
}

TEST(DoricoTimecode, MarkersAreLockedToTimeNotBeats)
{
    TempoTimeline slow;
    slow.rebuild(mu::engraving::TempoValues { { 0, 1.0 } }, {});   // 60 bpm
    TempoTimeline fast;
    fast.rebuild(mu::engraving::TempoValues { { 0, 2.0 } }, {});   // 120 bpm
    const TimeSettings s { 3600.0, 0, 0.0 };
    // A marker 4 s into the flow is on beat 4 at 60 bpm and on beat 8 at 120 bpm
    EXPECT_EQ(TimeMap(s, slow).tickAt(3604.0), 4 * 480);
    EXPECT_EQ(TimeMap(s, fast).tickAt(3604.0), 8 * 480);

    // The playback tempo multiplier does not move it
    fast.setTempoMultiplier(0.5);
    EXPECT_EQ(TimeMap(s, fast).tickAt(3604.0), 8 * 480);
}

TEST(DoricoVideoData, JsonRoundTrip)
{
    VideoData d;
    d.rate = FrameRate::Fps29_97Drop;
    d.useVideoRate = false;
    d.timecodeStart = 3600.0;
    d.attachBeats = 6;
    d.attachUnit = 720;
    d.startOffset = 1.5;
    d.markers = { { 1, 3601.0, "Door", true }, { 2, 3605.5, "Marker", false } };
    d.nextId = 3;
    EXPECT_EQ(videoDataFromJson(videoDataToJson(d)), d);

    EXPECT_EQ(videoDataFromJson(""), VideoData {});
    EXPECT_EQ(videoDataFromJson("not json"), VideoData {});
}
