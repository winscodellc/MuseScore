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

#include <vector>

#include <QByteArray>
#include <QString>
#include <QVariantMap>

//! The maths and the per-layout settings of spec 62 (Print Options panel), on plain values so they can be unit tested
//! without a score or a printer.
namespace mu::dorico::printjob {
//! The enums are stored as ints in the project file: append new values, never reorder.
enum class JobType { Normal = 0, Spreads, TwoUp, Booklet };
enum class RangeKind { All = 0, Pages, Flows };
enum class PrintOn { OneSide = 0, BothManual, BothAuto };
enum class Flip { Auto = 0, LongSide, ShortSide };
enum class Orientation { Auto = 0, Portrait, Landscape }; //!< Auto: by job type, then the layout's own

//! Everything the Print Options panel holds for one layout (Dorico stores it per layout, in the project).
struct PrintSettings {
    bool graphics = false; //!< destination: false = Printer, true = Graphics
    QString printerName;   //!< empty = the system default
    int copies = 1;
    bool collate = true;

    JobType jobType = JobType::Normal;
    RangeKind range = RangeKind::All;
    QString pagesText;

    QString paperSize;     //!< QPageSize name ("A4"); empty = the layout's own page size
    Orientation orientation = Orientation::Auto;
    bool fitToPaper = true;
    double scalePercent = 100.0;
    bool oddFinalOn = false; //!< Spreads / 2-up: a separate paper for the unpaired last page
    QString oddFinalSize;
    Orientation oddFinalOrientation = Orientation::Auto;

    PrintOn printOn = PrintOn::OneSide;
    Flip flipPortrait = Flip::Auto;
    Flip flipLandscape = Flip::Auto;
    bool reverseOutward = false;

    bool cropMarks = false;
    bool border = false;
    bool dateTime = false;
    bool watermark = false;
    QString watermarkText;

    QString graphicsFormat = "pdf"; //!< pdf, png, svg
    bool colour = true;             //!< Color / Mono
    bool filePerLayout = true;

    //! Keys as the panel uses them (the member names); values are QML-friendly (enums as int).
    QVariantMap toMap() const;
    //! Reads the keys present, ignores the unknown ones (so an older or newer file still loads).
    void applyMap(const QVariantMap& map);
    //! One key, as the panel sets it. False for an unknown key.
    bool set(const QString& key, const QVariant& value);

    QByteArray toJson() const;
    static PrintSettings fromJson(const QByteArray& json);
};

//! 1-based, inclusive.
struct PageRange {
    int first = 0;
    int last = 0;
};

struct RangeParse {
    bool ok = false;
    std::vector<PageRange> ranges;
};

//! `1-4`, `3-` (to the last page), `1,3,5-8`. Spaces are ignored. Not ok when empty, malformed, reversed or outside
//! 1..pageCount.
RangeParse parsePageRanges(const QString& text, int pageCount);

//! The pages (0-based) a settings range selects, in order. A bad Pages text selects nothing; Booklet ignores the range
//! (full range only).
std::vector<int> selectedPages(const PrintSettings& settings, int pageCount);

//! The page groups one graphics export writes as separate files: each Pages range on its own, else everything in one.
//! (A printer prints the flattened `selectedPages` instead.)
std::vector<std::vector<int>> pageGroups(const PrintSettings& settings, int pageCount);

//! One side of a sheet: -1 = blank. Normal puts its page in `left` and leaves `right` unused (`twoUp` false).
struct Sheet {
    int left = -1;
    int right = -1;
    bool twoUp = false;
};

//! Lays `pages` (0-based indexes) onto sheets.
//! Normal: one page each. Spreads: even page numbers left, odd right (page 1 alone on the right). 2-up: first page left,
//! then pairs. Booklet: padded with blanks to a multiple of 4 and imposed so the folded sheets read in order (front
//! p4|p1, back p2|p3 for 4 pages).
std::vector<Sheet> imposeSheets(const std::vector<int>& pages, JobType jobType);

//! Sheet indexes in print order. Even sheets are the outward (front) sides, odd the inward (back) sides.
struct DuplexPasses {
    std::vector<int> outward;
    std::vector<int> inward;
};
DuplexPasses manualDuplexPasses(int sheetCount, bool reverseOutward);

struct RectIn {
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;
};

//! Where a page of `pageW` x `pageH` inches lands in `slot`: scaled to fit (enlarging if needed) or by a percentage, and
//! centred (a percentage that is too big overflows the slot equally on every side).
RectIn placePage(double pageW, double pageH, const RectIn& slot, bool fitToPaper, double scalePercent);

//! The half of a paper sheet a page goes on: the whole sheet for Normal / a single page, else left or right half.
RectIn slotRect(double paperW, double paperH, bool twoUp, bool right);

//! Spreads, 2-up and Booklet are landscape unless the user chose otherwise; Normal follows the layout.
bool isLandscape(Orientation chosen, JobType jobType, bool layoutLandscape);

//! Crop marks and border are printed only when the page is smaller than the paper (inches, small tolerance).
bool pageSmallerThanPaper(const RectIn& page, double paperW, double paperH);

//! Sheet order for `copies` copies when the printer cannot make them itself: whole set repeated (collated) or each
//! sheet repeated (not collated).
std::vector<int> copyOrder(const std::vector<int>& order, int copies, bool collate);

//! 0 = Print, 1 = Export, 2 = Print and Export, from each selected layout's destination (true = Graphics).
int buttonKind(const std::vector<bool>& graphicsPerLayout);
}
