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

#include "videodata.h"

#include <algorithm>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "engraving/dom/masterscore.h"

namespace mu::dorico {
VideoData videoDataFromJson(const std::string& json)
{
    VideoData d;
    const QJsonObject o = QJsonDocument::fromJson(QByteArray::fromStdString(json)).object();
    if (o.isEmpty()) {
        return d;
    }
    d.rate = frameRateFromKey(o.value("rate").toString().toStdString()).value_or(d.rate);
    d.useVideoRate = o.value("useVideoRate").toBool(d.useVideoRate);
    d.timecodeStart = o.value("tcStart").toDouble(d.timecodeStart);
    d.attachBeats = o.value("attachBeats").toInt(d.attachBeats);
    d.attachUnit = std::max(1, o.value("attachUnit").toInt(d.attachUnit));
    d.startOffset = o.value("offset").toDouble(d.startOffset);
    for (const QJsonValue& v : o.value("markers").toArray()) {
        const QJsonObject m = v.toObject();
        d.markers.push_back({ m.value("id").toInt(), m.value("t").toDouble(), m.value("text").toString().toStdString(),
                              m.value("imp").toBool() });
        d.nextId = std::max(d.nextId, d.markers.back().id + 1);
    }
    d.nextId = std::max(d.nextId, o.value("nextId").toInt(1));
    std::stable_sort(d.markers.begin(), d.markers.end(), [](const VideoMarker& a, const VideoMarker& b) { return a.time < b.time; });
    return d;
}

std::string videoDataToJson(const VideoData& d)
{
    QJsonArray markers;
    for (const VideoMarker& m : d.markers) {
        markers.append(QJsonObject { { "id", m.id }, { "t", m.time }, { "text", QString::fromStdString(m.text) }, { "imp", m.important } });
    }
    const QJsonObject o {
        { "v", 1 },
        { "rate", frameRateInfo(d.rate).key },
        { "useVideoRate", d.useVideoRate },
        { "tcStart", d.timecodeStart },
        { "attachBeats", d.attachBeats },
        { "attachUnit", d.attachUnit },
        { "offset", d.startOffset },
        { "nextId", d.nextId },
        { "markers", markers },
    };
    return QJsonDocument(o).toJson(QJsonDocument::Compact).toStdString();
}

VideoData readVideoData(const engraving::Score* score)
{
    if (!score) {
        return {};
    }
    return videoDataFromJson(score->masterScore()->metaTag(VIDEO_TAG).toStdString());
}

TimeMap timeMap(const engraving::Score* score, const VideoData& data)
{
    return TimeMap(data.timeSettings(), score->masterScore()->tempoTimeline(/*expandRepeats*/ false));
}

std::vector<std::pair<int, std::string> > markerTicks(const engraving::Score* score)
{
    std::vector<std::pair<int, std::string> > out;
    const VideoData data = readVideoData(score);
    if (data.markers.empty()) {
        return out;
    }
    const TimeMap map = timeMap(score, data);
    const int end = score->masterScore()->endTick().ticks();
    for (const VideoMarker& m : data.markers) {
        const int tick = map.tickAt(m.time);
        if (m.time >= map.flowStart() && tick < end) {
            out.emplace_back(tick, m.text);
        }
    }
    return out;
}
}
