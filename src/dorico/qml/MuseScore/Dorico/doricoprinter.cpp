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

#include "doricoprinter.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include <QDateTime>
#include <QFile>
#include <QLocale>
#include <QPdfWriter>
#include <QPrinter>
#include <QPrinterInfo>

#include "draw/painter.h"

#include "engraving/dom/mscore.h"
#include "engraving/dom/score.h"
#include "notation/inotationelements.h"
#include "notation/inotationpainting.h"

#include "log.h"

using namespace mu::dorico;
using namespace mu::dorico::printjob;
using namespace mu::notation;
using namespace muse::draw;

namespace {
//! The paper a sheet goes on, as the printer or PDF gets it, and its size in inches in the printed orientation.
struct DpPaper {
    QPageSize size;
    bool landscape = false;
    double w = 0.0;
    double h = 0.0;

    //! Full-page mode: a QPrinter silently ignores a zero-margin layout in standard mode (it falls outside the
    //! printable area), so the job would keep the printer's default paper.
    QPageLayout layout() const
    {
        QPageLayout result(size, landscape ? QPageLayout::Landscape : QPageLayout::Portrait, QMarginsF());
        result.setMode(QPageLayout::FullPageMode);
        return result;
    }
};

QPageSize dpPageSizeFromKey(const QString& key, const QString& printerName)
{
    if (key.isEmpty()) {
        return QPageSize();
    }

    const QPrinterInfo info = printerName.isEmpty() ? QPrinterInfo::defaultPrinter() : QPrinterInfo::printerInfo(printerName);
    for (const QPageSize& size : info.supportedPageSizes()) {
        if (size.key() == key) {
            return size;
        }
    }

    for (int id = 0; id <= QPageSize::LastPageSize; ++id) {
        const QPageSize size(static_cast<QPageSize::PageSizeId>(id));
        if (size.isValid() && size.key() == key) {
            return size;
        }
    }

    return QPageSize();
}

DpPaper dpPaperFor(const QString& key, const QString& printerName, Orientation orientation, JobType jobType, const QSizeF& layoutIn)
{
    const bool layoutLandscape = layoutIn.width() > layoutIn.height();

    QPageSize size = dpPageSizeFromKey(key, printerName);
    if (!size.isValid()) {
        // The layout's own size; a fuzzy match turns A4-ish sizes into the standard A4.
        const QSizeF portrait(std::min(layoutIn.width(), layoutIn.height()), std::max(layoutIn.width(), layoutIn.height()));
        size = QPageSize(portrait, QPageSize::Inch, QString(), QPageSize::FuzzyMatch);
    }

    const QSizeF natural = size.size(QPageSize::Inch);
    DpPaper paper;
    paper.size = size;
    paper.landscape = isLandscape(orientation, jobType, layoutLandscape);
    paper.w = paper.landscape ? std::max(natural.width(), natural.height()) : std::min(natural.width(), natural.height());
    paper.h = paper.landscape ? std::min(natural.width(), natural.height()) : std::max(natural.width(), natural.height());
    return paper;
}

//! Printing needs the PAGE view (a score open in continuous view has no pages) and a laid out score.
class DpPageModeGuard
{
public:
    explicit DpPageModeGuard(const INotationPtr& notation)
        : m_notation(notation), m_viewMode(notation->painting()->viewMode())
    {
        mu::engraving::Score* score = notation->elements()->msScore();
        if (!score->autoLayoutEnabled()) {
            score->doLayout();
        }
        notation->painting()->setViewMode(mu::engraving::LayoutMode::PAGE);
    }

    ~DpPageModeGuard() { m_notation->painting()->setViewMode(m_viewMode); }

private:
    INotationPtr m_notation;
    mu::engraving::LayoutMode m_viewMode;
};

// ---- annotations --------------------------------------------------------------------------------------------------

constexpr double DP_CROP_GAP_IN = 0.05;
constexpr double DP_CROP_LEN_IN = 0.2;

Font dpFont(double pointSize)
{
    Font font(Font::FontFamily(u"Edwin"), Font::Type::Text);
    font.setPointSizeF(pointSize);
    return font;
}

//! Draws over the finished sheet. The window is in inches * mu::engraving::DPI, the unit Painter::drawText assumes for
//! its font scaling, so font sizes are plain points.
void dpPaintAnnotations(Painter& painter, double dpi, const DpPaper& paper, const std::vector<RectIn>& pages,
                        const RectIn& slotOfPages, const PrintSettings& settings)
{
    if (!settings.cropMarks && !settings.border && !settings.dateTime && !settings.watermark) {
        return;
    }

    constexpr double U = mu::engraving::DPI;

    painter.save();
    painter.setViewport(muse::RectF(0.0, 0.0, paper.w * dpi, paper.h * dpi));
    painter.setWindow(muse::RectF(0.0, 0.0, paper.w * U, paper.h * U));

    for (const RectIn& page : pages) {
        const bool smaller = pageSmallerThanPaper(page, slotOfPages.w, slotOfPages.h);

        if (settings.border && smaller) {
            painter.setPen(Pen(Color::BLACK, 0.5 / 72.0 * U));
            painter.setBrush(Brush(BrushStyle::NoBrush));
            painter.drawRect(page.x * U, page.y * U, page.w * U, page.h * U);
        }

        if (settings.cropMarks && smaller) {
            painter.setPen(Pen(Color::BLACK, 0.5 / 72.0 * U));
            const double g = DP_CROP_GAP_IN * U;
            const double l = (DP_CROP_GAP_IN + DP_CROP_LEN_IN) * U;
            const double x0 = page.x * U;
            const double x1 = (page.x + page.w) * U;
            const double y0 = page.y * U;
            const double y1 = (page.y + page.h) * U;
            for (const double x : { x0, x1 }) {
                const double dir = x == x0 ? -1.0 : 1.0;
                painter.drawLine(x + dir * g, y0, x + dir * l, y0);
                painter.drawLine(x + dir * g, y1, x + dir * l, y1);
            }
            for (const double y : { y0, y1 }) {
                const double dir = y == y0 ? -1.0 : 1.0;
                painter.drawLine(x0, y + dir * g, x0, y + dir * l);
                painter.drawLine(x1, y + dir * g, x1, y + dir * l);
            }
        }

        if (settings.dateTime) {
            painter.setFont(dpFont(8.0));
            painter.setPen(Pen(Color::BLACK));
            const QString text = QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat);
            painter.drawText(muse::RectF(page.x * U, (page.y + page.h - 0.45) * U, page.w * U, 0.3 * U),
                             Alignment(AlignHCenter | AlignVCenter), TextFlags(TextDontClip | TextSingleLine),
                             muse::String::fromQString(text));
        }

        // Mono graphics keep no translucent elements.
        if (settings.watermark && !settings.watermarkText.isEmpty() && !(settings.graphics && !settings.colour)) {
            // Roughly 0.55 em per glyph: size the text to 80 % of the page diagonal.
            const double diagonalPt = std::hypot(page.w, page.h) * 72.0;
            const double pt = std::clamp(0.8 * diagonalPt / (0.55 * settings.watermarkText.size()), 12.0, 400.0);
            painter.save();
            painter.translate((page.x + page.w / 2.0) * U, (page.y + page.h / 2.0) * U);
            painter.rotate(-std::atan2(page.h, page.w) * 180.0 / M_PI);
            painter.setFont(dpFont(pt));
            painter.setPen(Pen(Color(128, 128, 128, 70)));
            painter.drawText(muse::RectF(-page.w * U, -page.h * U / 4.0, 2.0 * page.w * U, page.h * U / 2.0), Alignment(AlignCenter),
                             TextFlags(TextDontClip | TextSingleLine), muse::String::fromQString(settings.watermarkText));
            painter.restore();
        }
    }

    painter.restore();
}

// ---- sheets -------------------------------------------------------------------------------------------------------

//! One page of the score, scaled and moved to `placed` (inches on the sheet). paintPrint resets the viewport and window
//! to the page at the sheet origin on every call; the world transform is separate from them, and in window units
//! (mu::engraving::DPI per inch), so the move and the scale survive.
void dpPaintPage(Painter& painter, INotationPainting& painting, int page, const RectIn& placed, double pageWidthIn, double dpi)
{
    constexpr double U = mu::engraving::DPI;

    painter.save();
    painter.translate(placed.x * U, placed.y * U);
    const double scale = placed.w / pageWidthIn;
    painter.scale(scale, scale);

    INotationPainting::Options opt;
    opt.fromPage = page;
    opt.toPage = page;
    opt.deviceDpi = static_cast<int>(dpi);
    painting.paintPrint(&painter, opt);

    painter.restore();
}

bool dpIsOddFinal(const PrintSettings& settings, const std::vector<Sheet>& sheets, int index)
{
    if (!settings.oddFinalOn || (settings.jobType != JobType::Spreads && settings.jobType != JobType::TwoUp)
        || index != static_cast<int>(sheets.size()) - 1) {
        return false;
    }
    return (sheets[index].left >= 0) != (sheets[index].right >= 0);
}

//! Paints the sheets `order` (indexes into `sheets`) of one layout. The device already has a page open when this is called
//! (starting the painter opens the first one, a following layout of a PDF gets one from the caller), so the first sheet goes
//! on it and only the next ones start a new page.
void dpPaintSheets(QPagedPaintDevice& device, Painter& painter, const INotationPtr& notation, const PrintSettings& settings,
                   const QString& printerName, const std::vector<Sheet>& sheets, const std::vector<int>& order)
{
    INotationPainting& painting = *notation->painting();
    const muse::SizeF layoutSize = painting.pageSizeInch();
    const QSizeF layoutIn(layoutSize.width(), layoutSize.height());
    const double dpi = device.logicalDpiX();

    // A graphics file keeps the layout's own page size, only the orientation can change.
    const QString key = settings.graphics ? QString() : settings.paperSize;
    const DpPaper mainPaper = dpPaperFor(key, printerName, settings.orientation, settings.jobType, layoutIn);
    const DpPaper oddPaper = dpPaperFor(settings.oddFinalSize.isEmpty() ? key : settings.oddFinalSize, printerName,
                                        settings.oddFinalOrientation, JobType::Normal, layoutIn);

    bool first = true;
    for (const int index : order) {
        const Sheet& sheet = sheets[index];
        const bool odd = dpIsOddFinal(settings, sheets, index);
        const DpPaper& paper = odd ? oddPaper : mainPaper;

        if (first) {
            first = false;
        } else {
            device.setPageLayout(paper.layout());
            device.newPage();
        }

        const bool twoUp = sheet.twoUp && !odd;
        std::vector<RectIn> placedPages;
        auto put = [&](int page, bool right) {
            if (page < 0) {
                return;
            }
            const RectIn slot = slotRect(paper.w, paper.h, twoUp, right);
            const RectIn placed = placePage(layoutIn.width(), layoutIn.height(), slot, settings.fitToPaper, settings.scalePercent);
            dpPaintPage(painter, painting, page, placed, layoutIn.width(), dpi);
            placedPages.push_back(placed);
        };

        if (twoUp) {
            put(sheet.left, false);
            put(sheet.right, true);
        } else {
            put(sheet.left >= 0 ? sheet.left : sheet.right, false);
        }

        dpPaintAnnotations(painter, dpi, paper, placedPages, slotRect(paper.w, paper.h, twoUp, false), settings);
    }
}

QPrinter::DuplexMode dpDuplexMode(const PrintSettings& settings, bool landscape)
{
    if (settings.printOn != PrintOn::BothAuto) {
        return QPrinter::DuplexNone;
    }

    switch (landscape ? settings.flipLandscape : settings.flipPortrait) {
    case Flip::Auto: return QPrinter::DuplexAuto;
    case Flip::LongSide: return QPrinter::DuplexLongSide;
    case Flip::ShortSide: return QPrinter::DuplexShortSide;
    }
    return QPrinter::DuplexAuto;
}
}

std::vector<Sheet> DoricoPrinter::sheetsFor(const PrintSettings& settings, int pageCount)
{
    return imposeSheets(selectedPages(settings, pageCount), settings.jobType);
}

muse::Ret DoricoPrinter::print(const INotationPtr& notation, const PrintSettings& settings, Pass pass)
{
    IF_ASSERT_FAILED(notation) {
        return muse::make_ret(muse::Ret::Code::InternalError);
    }

    DpPageModeGuard guard(notation);

    const std::vector<Sheet> sheets = sheetsFor(settings, notation->painting()->pageCount());
    if (sheets.empty()) {
        return muse::make_ret(muse::Ret::Code::InternalError, std::string("No pages to print"));
    }

    std::vector<int> order;
    if (pass == Pass::All) {
        for (int i = 0; i < static_cast<int>(sheets.size()); ++i) {
            order.push_back(i);
        }
    } else {
        const DuplexPasses passes = manualDuplexPasses(static_cast<int>(sheets.size()), settings.reverseOutward);
        order = pass == Pass::Outward ? passes.outward : passes.inward;
    }
    if (order.empty()) {
        return muse::make_ok();
    }

    const muse::SizeF layoutSize = notation->painting()->pageSizeInch();
    const QSizeF layoutIn(layoutSize.width(), layoutSize.height());
    const DpPaper paper = dpPaperFor(settings.paperSize, settings.printerName, settings.orientation, settings.jobType, layoutIn);

    QPrinter printer(QPrinter::HighResolution);
    if (!settings.printerName.isEmpty()) {
        printer.setPrinterName(settings.printerName);
    }
    if (!printer.isValid()) {
        return muse::make_ret(muse::Ret::Code::InternalError, std::string("The printer is not available"));
    }

    printer.setPageLayout(paper.layout());
    printer.setFullPage(true);
    printer.setDocName(notation->projectWorkTitleAndPartName());
    printer.setOutputFormat(QPrinter::NativeFormat);
    printer.setDuplex(dpDuplexMode(settings, paper.landscape));

    if (printer.supportsMultipleCopies()) {
        printer.setCopyCount(settings.copies);
        printer.setCollateCopies(settings.collate);
    } else {
        order = copyOrder(order, settings.copies, settings.collate);
    }

    Painter painter(&printer, "print");
    if (!painter.isActive()) {
        return muse::make_ret(muse::Ret::Code::InternalError, std::string("Cannot start printing"));
    }

    dpPaintSheets(printer, painter, notation, settings, settings.printerName, sheets, order);
    painter.endDraw();

    return muse::make_ok();
}

muse::Ret DoricoPrinter::writePdf(const QString& path, const std::vector<PdfPart>& parts)
{
    IF_ASSERT_FAILED(!parts.empty()) {
        return muse::make_ret(muse::Ret::Code::InternalError);
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return muse::make_ret(muse::Ret::Code::InternalError, std::string("Cannot write ") + path.toStdString());
    }

    const PrintSettings& firstSettings = parts.front().settings;

    QPdfWriter pdf(&file);
    pdf.setResolution(static_cast<int>(mu::engraving::DPI));
    pdf.setTitle(parts.front().notation->projectWorkTitleAndPartName());
    pdf.setPageMargins(QMarginsF());
    pdf.setColorModel(firstSettings.colour ? QPdfWriter::ColorModel::Auto : QPdfWriter::ColorModel::Grayscale);

    // Page mode and layout first: the page sizes depend on them. The first layout's paper must be in place before the
    // painter starts the first page (PdfWriter::writeList does the same).
    std::vector<std::unique_ptr<DpPageModeGuard> > guards;
    for (const PdfPart& part : parts) {
        guards.push_back(std::make_unique<DpPageModeGuard>(part.notation));
    }

    auto paperOf = [](const PdfPart& part) {
        const muse::SizeF size = part.notation->painting()->pageSizeInch();
        return dpPaperFor(QString(), QString(), part.settings.orientation, part.settings.jobType, QSizeF(size.width(), size.height()));
    };

    pdf.setPageLayout(paperOf(parts.front()).layout());
    Painter painter(&pdf, "pdfwriter");
    if (!painter.isActive()) {
        return muse::make_ret(muse::Ret::Code::InternalError, std::string("Cannot start the PDF"));
    }

    bool firstPart = true;
    for (const PdfPart& part : parts) {
        const int pageCount = part.notation->painting()->pageCount();
        const std::vector<int> pages = part.pages.empty() ? selectedPages(part.settings, pageCount) : part.pages;
        const std::vector<Sheet> sheets = imposeSheets(pages, part.settings.jobType);
        if (sheets.empty()) {
            continue;
        }

        std::vector<int> order;
        for (int i = 0; i < static_cast<int>(sheets.size()); ++i) {
            order.push_back(i);
        }

        if (!firstPart) {
            pdf.setPageLayout(paperOf(part).layout());
            pdf.newPage();
        }

        dpPaintSheets(pdf, painter, part.notation, part.settings, QString(), sheets, order);
        firstPart = false;
    }

    painter.endDraw();
    return muse::make_ok();
}

QStringList DoricoPrinter::printerNames()
{
    return QPrinterInfo::availablePrinterNames();
}

QString DoricoPrinter::defaultPrinterName()
{
    return QPrinterInfo::defaultPrinterName();
}

QVariantList DoricoPrinter::paperSizes(const QString& printerName)
{
    QList<QPageSize> sizes;
    if (!printerName.isEmpty() || !QPrinterInfo::defaultPrinterName().isEmpty()) {
        const QPrinterInfo info = printerName.isEmpty() ? QPrinterInfo::defaultPrinter() : QPrinterInfo::printerInfo(printerName);
        sizes = info.supportedPageSizes();
    }
    if (sizes.isEmpty()) {
        for (const auto id : { QPageSize::A3, QPageSize::A4, QPageSize::A5, QPageSize::Letter, QPageSize::Legal, QPageSize::Tabloid }) {
            sizes.append(QPageSize(id));
        }
    }

    QVariantList result;
    for (const QPageSize& size : sizes) {
        result.append(QVariantMap { { "key", size.key() }, { "name", size.name() } });
    }
    return result;
}

bool DoricoPrinter::autoDuplexAvailable(const QString& printerName)
{
    const QPrinterInfo info = printerName.isEmpty() ? QPrinterInfo::defaultPrinter() : QPrinterInfo::printerInfo(printerName);
    return info.supportedDuplexModes().contains(QPrinter::DuplexLongSide);
}
