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

#include "dorico/internal/printjob.h"

using namespace mu::dorico::printjob;

namespace {
std::vector<int> pjPages(int n)
{
    std::vector<int> pages;
    for (int i = 0; i < n; ++i) {
        pages.push_back(i);
    }
    return pages;
}

// "L|R" per sheet, blank = _
std::string pjSheetsToString(const std::vector<Sheet>& sheets)
{
    std::string out;
    auto name = [](int p) { return p < 0 ? std::string("_") : std::to_string(p + 1); };
    for (const Sheet& s : sheets) {
        out += (out.empty() ? "" : " ") + name(s.left) + (s.twoUp ? "|" + name(s.right) : "");
    }
    return out;
}
}

TEST(Dorico_PrintJobTests, PageRangesAccept)
{
    RangeParse r = parsePageRanges("1-4", 10);
    ASSERT_TRUE(r.ok);
    ASSERT_EQ(r.ranges.size(), 1u);
    EXPECT_EQ(r.ranges[0].first, 1);
    EXPECT_EQ(r.ranges[0].last, 4);

    r = parsePageRanges("3-", 10);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.ranges[0].first, 3);
    EXPECT_EQ(r.ranges[0].last, 10);

    r = parsePageRanges(" 1, 3 ,5-8 ", 10);
    ASSERT_TRUE(r.ok);
    ASSERT_EQ(r.ranges.size(), 3u);
    EXPECT_EQ(r.ranges[1].first, 3);
    EXPECT_EQ(r.ranges[1].last, 3);
    EXPECT_EQ(r.ranges[2].last, 8);
}

TEST(Dorico_PrintJobTests, PageRangesReject)
{
    for (const char* bad : { "", "0", "5-2", "a", "1,,3", "-3", "11", "1-11", "1-2-3", "1;3" }) {
        EXPECT_FALSE(parsePageRanges(bad, 10).ok) << bad;
    }
    EXPECT_FALSE(parsePageRanges("1", 0).ok);
}

TEST(Dorico_PrintJobTests, SelectedPages)
{
    PrintSettings s;
    EXPECT_EQ(selectedPages(s, 4), (std::vector<int> { 0, 1, 2, 3 }));

    s.range = RangeKind::Pages;
    s.pagesText = "1,3-";
    EXPECT_EQ(selectedPages(s, 4), (std::vector<int> { 0, 2, 3 }));

    s.pagesText = "9";
    EXPECT_TRUE(selectedPages(s, 4).empty());

    // Booklet is full range only.
    s.pagesText = "1";
    s.jobType = JobType::Booklet;
    EXPECT_EQ(selectedPages(s, 4).size(), 4u);
}

TEST(Dorico_PrintJobTests, PageGroups)
{
    PrintSettings s;
    EXPECT_EQ(pageGroups(s, 3), (std::vector<std::vector<int>> { { 0, 1, 2 } }));

    // Separate ranges are separate files.
    s.range = RangeKind::Pages;
    s.pagesText = "1,3-";
    EXPECT_EQ(pageGroups(s, 4), (std::vector<std::vector<int>> { { 0 }, { 2, 3 } }));

    // A bad range selects nothing, Booklet ignores it.
    s.pagesText = "x";
    EXPECT_TRUE(pageGroups(s, 4).empty());
    s.jobType = JobType::Booklet;
    EXPECT_EQ(pageGroups(s, 4).size(), 1u);
}

TEST(Dorico_PrintJobTests, CopyOrder)
{
    EXPECT_EQ(copyOrder({ 0, 1 }, 2, true), (std::vector<int> { 0, 1, 0, 1 }));
    EXPECT_EQ(copyOrder({ 0, 1 }, 2, false), (std::vector<int> { 0, 0, 1, 1 }));
    EXPECT_EQ(copyOrder({ 0, 1 }, 1, true), (std::vector<int> { 0, 1 }));
}

TEST(Dorico_PrintJobTests, ImposeNormal)
{
    EXPECT_EQ(pjSheetsToString(imposeSheets(pjPages(3), JobType::Normal)), "1 2 3");
}

TEST(Dorico_PrintJobTests, ImposeTwoUp)
{
    EXPECT_EQ(pjSheetsToString(imposeSheets(pjPages(5), JobType::TwoUp)), "1|2 3|4 5|_");
}

TEST(Dorico_PrintJobTests, ImposeSpreads)
{
    // Page 1 alone on the right, then even left / odd right.
    EXPECT_EQ(pjSheetsToString(imposeSheets(pjPages(5), JobType::Spreads)), "_|1 2|3 4|5");
    EXPECT_EQ(pjSheetsToString(imposeSheets(pjPages(4), JobType::Spreads)), "_|1 2|3 4|_");
    // A range starting on page 3 (odd): right side, blank left.
    EXPECT_EQ(pjSheetsToString(imposeSheets({ 2, 3 }, JobType::Spreads)), "_|3 4|_");
}

TEST(Dorico_PrintJobTests, ImposeBooklet)
{
    EXPECT_EQ(pjSheetsToString(imposeSheets(pjPages(4), JobType::Booklet)), "4|1 2|3");
    // 6 pages print as 8, the two blanks at the end.
    EXPECT_EQ(pjSheetsToString(imposeSheets(pjPages(6), JobType::Booklet)), "_|1 2|_ 6|3 4|5");
}

TEST(Dorico_PrintJobTests, ManualDuplexPasses)
{
    DuplexPasses p = manualDuplexPasses(5, false);
    EXPECT_EQ(p.outward, (std::vector<int> { 0, 2, 4 }));
    EXPECT_EQ(p.inward, (std::vector<int> { 1, 3 }));

    p = manualDuplexPasses(5, true);
    EXPECT_EQ(p.outward, (std::vector<int> { 4, 2, 0 }));
    EXPECT_EQ(p.inward, (std::vector<int> { 1, 3 }));
}

TEST(Dorico_PrintJobTests, PlacePage)
{
    const RectIn a3 = slotRect(16.54, 11.69, false, false);

    // Fit enlarges an A4 portrait page to the height of A3 landscape and centres it.
    RectIn r = placePage(8.27, 11.69, a3, true, 100);
    EXPECT_NEAR(r.h, 11.69, 1e-9);
    EXPECT_NEAR(r.w, 8.27, 1e-9);
    EXPECT_NEAR(r.x, (16.54 - 8.27) / 2.0, 1e-9);

    // Custom 100 % keeps the size, 50 % halves it, both centred.
    r = placePage(8.27, 11.69, a3, false, 50);
    EXPECT_NEAR(r.w, 8.27 / 2.0, 1e-9);
    EXPECT_NEAR(r.y, (11.69 - 11.69 / 2.0) / 2.0, 1e-9);

    // Too big overflows equally on every side.
    r = placePage(11.69, 16.54, slotRect(8.27, 11.69, false, false), false, 100);
    EXPECT_LT(r.x, 0.0);
    EXPECT_NEAR(r.x, (8.27 - 11.69) / 2.0, 1e-9);

    // 2-up halves.
    const RectIn right = slotRect(16.54, 11.69, true, true);
    EXPECT_NEAR(right.x, 8.27, 1e-9);
    EXPECT_NEAR(right.w, 8.27, 1e-9);
}

TEST(Dorico_PrintJobTests, Orientation)
{
    EXPECT_TRUE(isLandscape(Orientation::Auto, JobType::Booklet, false));
    EXPECT_FALSE(isLandscape(Orientation::Auto, JobType::Normal, false));
    EXPECT_TRUE(isLandscape(Orientation::Auto, JobType::Normal, true));
    EXPECT_FALSE(isLandscape(Orientation::Portrait, JobType::TwoUp, true));
}

TEST(Dorico_PrintJobTests, AnnotationsOnlyWhenPageSmaller)
{
    EXPECT_TRUE(pageSmallerThanPaper({ 0, 0, 8.27, 11.69 }, 11.69, 16.54));
    EXPECT_FALSE(pageSmallerThanPaper({ 0, 0, 8.27, 11.69 }, 8.27, 11.69));
    EXPECT_FALSE(pageSmallerThanPaper({ 0, 0, 8.27, 11.69 }, 8.2700001, 11.69));
}

TEST(Dorico_PrintJobTests, ButtonKind)
{
    EXPECT_EQ(buttonKind({ false, false }), 0);
    EXPECT_EQ(buttonKind({ true, true }), 1);
    EXPECT_EQ(buttonKind({ false, true }), 2);
}

TEST(Dorico_PrintJobTests, SettingsJsonRoundTrip)
{
    PrintSettings s;
    s.graphics = true;
    s.printerName = "Office";
    s.copies = 3;
    s.collate = true;
    s.jobType = JobType::Booklet;
    s.range = RangeKind::Pages;
    s.pagesText = "1,3-";
    s.orientation = Orientation::Landscape;
    s.fitToPaper = false;
    s.scalePercent = 85.5;
    s.printOn = PrintOn::BothManual;
    s.flipLandscape = Flip::ShortSide;
    s.watermark = true;
    s.watermarkText = "DRAFT";
    s.colour = false;

    const PrintSettings back = PrintSettings::fromJson(s.toJson());
    EXPECT_EQ(back.toMap(), s.toMap());
    EXPECT_EQ(back.jobType, JobType::Booklet);
    EXPECT_EQ(back.flipLandscape, Flip::ShortSide);
    EXPECT_DOUBLE_EQ(back.scalePercent, 85.5);
}

TEST(Dorico_PrintJobTests, SettingsDefaultsAndRobustness)
{
    // An empty object (a project from before the panel) gives the defaults.
    EXPECT_EQ(PrintSettings::fromJson("{}").toMap(), PrintSettings().toMap());
    EXPECT_EQ(PrintSettings::fromJson("not json").toMap(), PrintSettings().toMap());

    PrintSettings s;
    EXPECT_FALSE(s.set("nonsense", 1));
    EXPECT_TRUE(s.set("copies", 0));
    EXPECT_EQ(s.copies, 1); // clamped
    EXPECT_TRUE(s.set("jobType", 2));
    EXPECT_EQ(s.jobType, JobType::TwoUp);
}
