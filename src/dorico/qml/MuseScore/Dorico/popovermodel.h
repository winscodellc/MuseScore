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

#include <QObject>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QTransform>
#include <QVariant>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "context/iglobalcontext.h"
#include "rcommand/icommanddispatcher.h"

#include "../../../ipopovercontroller.h"

namespace mu::dorico {
//! Backs Popover.qml (spec 20): mirrors IPopoverController and maps its canvas anchor into view pixels.
//! Also the lyrics popover look (spec 99): the lyric is typed in MuseScore's in-score editor (spec 33), and the box shows
//! its line, text and placement while that editor runs.
class PopoverModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(bool isOpen READ isOpen NOTIFY changed)
    Q_PROPERTY(QString glyph READ glyph NOTIFY changed) // PopoverDef::glyph
    Q_PROPERTY(bool scopeButton READ scopeButton NOTIFY changed)
    Q_PROPERTY(int fieldWidth READ fieldWidth NOTIFY changed)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(bool supportsLocal READ supportsLocal NOTIFY changed)
    Q_PROPERTY(bool navigates READ navigates NOTIFY changed) // Space / Tab / Cmd-arrows move it along the score (spec 32)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY changed)
    Q_PROPERTY(int status READ status NOTIFY changed) // ParseStatus: 0 Ok, 1 Incomplete, 2 Unsupported, 3 Invalid
    // The suggestion menu (tempo, spec 26): Up/Down move the highlight, Return applies the highlighted entry.
    Q_PROPERTY(QStringList suggestions READ suggestions NOTIFY changed)
    Q_PROPERTY(int currentSuggestion READ currentSuggestion NOTIFY changed) // -1 = none, Return applies the text
    // The paint view's `matrix` (canvas -> view); bound from QML so scroll and zoom move the popover. Its own signal, so
    // scrolling does not re-run status() and suggestions() (spec 96).
    Q_PROPERTY(QVariant viewMatrix READ viewMatrix WRITE setViewMatrix NOTIFY geometryChanged)
    Q_PROPERTY(QRectF anchorRect READ anchorRect NOTIFY geometryChanged)

    Q_PROPERTY(bool lyricsOpen READ lyricsOpen NOTIFY lyricsChanged)
    Q_PROPERTY(int lyricsLine READ lyricsLine NOTIFY lyricsChanged) // 1-based
    Q_PROPERTY(bool lyricsAbove READ lyricsAbove NOTIFY lyricsChanged)
    Q_PROPERTY(QString lyricsText READ lyricsText NOTIFY lyricsChanged)
    Q_PROPERTY(QRectF lyricsAnchor READ lyricsAnchor NOTIFY geometryChanged)

    QML_ELEMENT

    muse::ContextInject<IPopoverController> controller = { this };
    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<muse::rcommand::ICommandDispatcher> dispatcher = { this };

public:
    explicit PopoverModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();
    Q_INVOKABLE void accept(bool local);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void moveSuggestion(int delta);
    Q_INVOKABLE void move(int popoverMove); // PopoverMove: 0 next beat, 1 previous beat, 2 next bar, 3 previous bar, 4/5 next/previous item
    Q_INVOKABLE void lyricsLineStep(int delta); // -1 previous line, +1 next line (added when missing)
    Q_INVOKABLE void toggleLyricsPlacement();

    bool isOpen() const;
    QString glyph() const;
    bool scopeButton() const;
    int fieldWidth() const;
    QString title() const;
    bool supportsLocal() const;
    bool navigates() const;
    QString text() const { return m_text; }
    void setText(const QString& text);
    int status() const;
    QStringList suggestions() const;
    int currentSuggestion() const { return m_currentSuggestion; }
    QVariant viewMatrix() const { return m_viewMatrix; }
    void setViewMatrix(const QVariant& matrix);
    QRectF anchorRect() const;

    bool lyricsOpen() const { return m_lyricsOpen; }
    int lyricsLine() const { return m_lyricsLine; }
    bool lyricsAbove() const { return m_lyricsAbove; }
    QString lyricsText() const { return m_lyricsText; }
    QRectF lyricsAnchor() const;

signals:
    void changed();
    void geometryChanged(); // also sent with every changed()
    void opened(); // a popover was opened (or replaced): take keyboard focus
    void lyricsChanged();

private:
    void subscribeLyrics();
    void updateLyrics();

    QString m_text;
    int m_currentSuggestion = -1;
    QVariant m_viewMatrix;

    bool m_lyricsOpen = false;
    int m_lyricsLine = 1;
    bool m_lyricsAbove = false;
    QString m_lyricsText;
    QRectF m_lyricsRect; // canvas
};
}
