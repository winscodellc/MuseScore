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

#include <QString>
#include <QVariantList>

#include "global/types/ret.h"
#include "notation/inotation.h"

#include "../../../internal/printjob.h"

namespace mu::dorico {
//! Spec 62: paints a layout onto printer or PDF sheets without the system dialog. Each `page` of the score is placed on a
//! sheet by `printjob::placePage` (job type, paper size, scale), then the annotations are drawn over the sheet.
//! `Qt::PrintSupport` is only linked here, the maths is in `internal/printjob.h`.
class DoricoPrinter
{
public:
    //! Which sheets of the job: Normal prints them all; manual duplex prints the front sides first, then the back sides.
    enum class Pass { All, Outward, Inward };

    //! Prints to `settings.printerName` (empty = system default). Copies and collate go to the printer when it can do
    //! them, else the sheets are repeated here.
    static muse::Ret print(const notation::INotationPtr& notation, const printjob::PrintSettings& settings, Pass pass);

    //! One layout of a PDF file: the notation and the pages (0-based) to put in it; empty `pages` = what the settings select.
    struct PdfPart {
        notation::INotationPtr notation;
        printjob::PrintSettings settings;
        std::vector<int> pages;
    };
    //! Writes `parts` one after the other into one PDF.
    static muse::Ret writePdf(const QString& path, const std::vector<PdfPart>& parts);

    //! The sheets a job of `settings` prints for a layout with `pageCount` pages.
    static std::vector<printjob::Sheet> sheetsFor(const printjob::PrintSettings& settings, int pageCount);

    static QStringList printerNames();
    static QString defaultPrinterName();
    //! `[{ key, name }]` for the printer's paper sizes (the standard ones when `printerName` is unknown).
    static QVariantList paperSizes(const QString& printerName);
    static bool autoDuplexAvailable(const QString& printerName);
};
}
