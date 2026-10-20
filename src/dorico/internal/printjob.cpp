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

#include "printjob.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

#include <QJsonDocument>
#include <QJsonObject>

using namespace mu::dorico::printjob;

namespace {
// Reads `key` from `map` into `field` when present. Enums travel as ints.
template<class T>
void pjRead(const QVariantMap& map, const char* key, T& field)
{
    const auto it = map.constFind(QString::fromLatin1(key));
    if (it == map.constEnd()) {
        return;
    }
    if constexpr (std::is_enum_v<T>) {
        field = static_cast<T>(it->toInt());
    } else if constexpr (std::is_same_v<T, bool>) {
        field = it->toBool();
    } else if constexpr (std::is_same_v<T, int>) {
        field = it->toInt();
    } else if constexpr (std::is_same_v<T, double>) {
        field = it->toDouble();
    } else {
        field = it->toString();
    }
}

template<class T>
QVariant pjWrite(const T& field)
{
    if constexpr (std::is_enum_v<T>) {
        return static_cast<int>(field);
    } else {
        return field;
    }
}

constexpr double PJ_EPS = 1e-6;
}

#define PJ_FIELDS(X) \
    X(graphics) X(printerName) X(copies) X(collate) X(jobType) X(range) X(pagesText) X(paperSize) X(orientation) \
    X(fitToPaper) X(scalePercent) X(oddFinalOn) X(oddFinalSize) X(oddFinalOrientation) X(printOn) X(flipPortrait) \
    X(flipLandscape) X(reverseOutward) X(cropMarks) X(border) X(dateTime) X(watermark) X(watermarkText) \
    X(graphicsFormat) X(colour) X(filePerLayout)

QVariantMap PrintSettings::toMap() const
{
    QVariantMap map;
#define PJ_PUT(name) map.insert(QStringLiteral(#name), pjWrite(name));
    PJ_FIELDS(PJ_PUT)
#undef PJ_PUT
    return map;
}

void PrintSettings::applyMap(const QVariantMap& map)
{
#define PJ_GET(name) pjRead(map, #name, name);
    PJ_FIELDS(PJ_GET)
#undef PJ_GET
    // A hand-edited or damaged file must not leave the printer with nonsense.
    copies = std::clamp(copies, 1, 999);
    scalePercent = std::clamp(scalePercent, 1.0, 1000.0);
}

bool PrintSettings::set(const QString& key, const QVariant& value)
{
    if (!toMap().contains(key)) {
        return false;
    }
    applyMap({ { key, value } });
    return true;
}

QByteArray PrintSettings::toJson() const
{
    return QJsonDocument(QJsonObject::fromVariantMap(toMap())).toJson(QJsonDocument::Compact);
}

PrintSettings PrintSettings::fromJson(const QByteArray& json)
{
    PrintSettings settings;
    settings.applyMap(QJsonDocument::fromJson(json).object().toVariantMap());
    return settings;
}

RangeParse mu::dorico::printjob::parsePageRanges(const QString& text, int pageCount)
{
    RangeParse result;
    const QString compact = QString(text).remove(' ');
    if (compact.isEmpty() || pageCount < 1) {
        return result;
    }

    for (const QString& part : compact.split(',')) {
        const int dash = part.indexOf('-');
        bool okFirst = false;
        bool okLast = true;
        PageRange range;
        if (dash < 0) {
            range.first = part.toInt(&okFirst);
            range.last = range.first;
        } else {
            range.first = part.left(dash).toInt(&okFirst);
            const QString tail = part.mid(dash + 1);
            if (tail.isEmpty()) {
                range.last = pageCount; // "3-"
            } else {
                range.last = tail.toInt(&okLast);
            }
        }

        if (!okFirst || !okLast || range.first < 1 || range.last < range.first || range.last > pageCount) {
            return {};
        }
        result.ranges.push_back(range);
    }

    result.ok = true;
    return result;
}

std::vector<int> mu::dorico::printjob::selectedPages(const PrintSettings& settings, int pageCount)
{
    std::vector<int> pages;
    if (settings.range == RangeKind::Pages && settings.jobType != JobType::Booklet) {
        for (const PageRange& r : parsePageRanges(settings.pagesText, pageCount).ranges) {
            for (int p = r.first; p <= r.last; ++p) {
                pages.push_back(p - 1);
            }
        }
        return pages;
    }

    for (int p = 0; p < pageCount; ++p) {
        pages.push_back(p);
    }
    return pages;
}

std::vector<std::vector<int>> mu::dorico::printjob::pageGroups(const PrintSettings& settings, int pageCount)
{
    if (settings.range == RangeKind::Pages && settings.jobType != JobType::Booklet) {
        std::vector<std::vector<int>> groups;
        for (const PageRange& r : parsePageRanges(settings.pagesText, pageCount).ranges) {
            std::vector<int> pages;
            for (int p = r.first; p <= r.last; ++p) {
                pages.push_back(p - 1);
            }
            groups.push_back(pages);
        }
        return groups;
    }

    return { selectedPages(settings, pageCount) };
}

std::vector<Sheet> mu::dorico::printjob::imposeSheets(const std::vector<int>& pages, JobType jobType)
{
    std::vector<Sheet> sheets;
    const int n = static_cast<int>(pages.size());

    switch (jobType) {
    case JobType::Normal:
        for (int p : pages) {
            sheets.push_back({ p, -1, false });
        }
        break;
    case JobType::TwoUp:
        for (int i = 0; i < n; i += 2) {
            sheets.push_back({ pages[i], i + 1 < n ? pages[i + 1] : -1, true });
        }
        break;
    case JobType::Spreads:
        // Even page numbers go on the left, odd on the right, so page 1 is alone on the right of the first sheet.
        for (int i = 0; i < n; ++i) {
            const bool right = (pages[i] + 1) % 2 == 1;
            if (!right) {
                sheets.push_back({ pages[i], -1, true });
            } else if (!sheets.empty() && sheets.back().right < 0 && sheets.back().left == pages[i] - 1) {
                sheets.back().right = pages[i];
            } else {
                sheets.push_back({ -1, pages[i], true });
            }
        }
        break;
    case JobType::Booklet: {
        std::vector<int> padded = pages;
        while (padded.size() % 4 != 0) {
            padded.push_back(-1);
        }
        const int total = static_cast<int>(padded.size());
        for (int k = 0; k < total / 4; ++k) {
            sheets.push_back({ padded[total - 1 - 2 * k], padded[2 * k], true });
            sheets.push_back({ padded[2 * k + 1], padded[total - 2 - 2 * k], true });
        }
        break;
    }
    }

    return sheets;
}

DuplexPasses mu::dorico::printjob::manualDuplexPasses(int sheetCount, bool reverseOutward)
{
    DuplexPasses passes;
    for (int i = 0; i < sheetCount; ++i) {
        (i % 2 == 0 ? passes.outward : passes.inward).push_back(i);
    }
    if (reverseOutward) {
        std::reverse(passes.outward.begin(), passes.outward.end());
    }
    return passes;
}

RectIn mu::dorico::printjob::placePage(double pageW, double pageH, const RectIn& slot, bool fitToPaper, double scalePercent)
{
    if (pageW <= 0.0 || pageH <= 0.0) {
        return slot;
    }

    const double scale = fitToPaper ? std::min(slot.w / pageW, slot.h / pageH) : scalePercent / 100.0;
    const double w = pageW * scale;
    const double h = pageH * scale;
    return { slot.x + (slot.w - w) / 2.0, slot.y + (slot.h - h) / 2.0, w, h };
}

RectIn mu::dorico::printjob::slotRect(double paperW, double paperH, bool twoUp, bool right)
{
    if (!twoUp) {
        return { 0.0, 0.0, paperW, paperH };
    }
    return { right ? paperW / 2.0 : 0.0, 0.0, paperW / 2.0, paperH };
}

bool mu::dorico::printjob::isLandscape(Orientation chosen, JobType jobType, bool layoutLandscape)
{
    if (chosen != Orientation::Auto) {
        return chosen == Orientation::Landscape;
    }
    return jobType != JobType::Normal || layoutLandscape;
}

bool mu::dorico::printjob::pageSmallerThanPaper(const RectIn& page, double paperW, double paperH)
{
    return page.w < paperW - PJ_EPS || page.h < paperH - PJ_EPS;
}

std::vector<int> mu::dorico::printjob::copyOrder(const std::vector<int>& order, int copies, bool collate)
{
    std::vector<int> result;
    for (int c = 0; c < std::max(copies, 1); ++c) {
        if (collate) {
            result.insert(result.end(), order.begin(), order.end());
        }
    }
    if (!collate) {
        for (int sheet : order) {
            result.insert(result.end(), std::max(copies, 1), sheet);
        }
    }
    return result;
}

int mu::dorico::printjob::buttonKind(const std::vector<bool>& graphicsPerLayout)
{
    const bool anyGraphics = std::any_of(graphicsPerLayout.begin(), graphicsPerLayout.end(), [](bool g) { return g; });
    const bool anyPrinter = std::any_of(graphicsPerLayout.begin(), graphicsPerLayout.end(), [](bool g) { return !g; });
    return anyGraphics && anyPrinter ? 2 : (anyGraphics ? 1 : 0);
}
