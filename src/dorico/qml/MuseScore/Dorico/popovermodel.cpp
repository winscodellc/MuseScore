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

#include "popovermodel.h"

#include "../../../internal/doricoperf.h"
#include "../../../doricocommands.h"

#include "engraving/dom/lyrics.h"
#include "notation/inotationinteraction.h"
#include "notationscene/notationcommands.h"

#include <algorithm>

using namespace mu::dorico;
using namespace mu::notation;

PopoverModel::PopoverModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
    connect(this, &PopoverModel::changed, this, &PopoverModel::geometryChanged);
}

void PopoverModel::load()
{
    controller()->stateChanged().onNotify(this, [this]() {
        m_text = QString::fromStdString(controller()->initialText());
        m_currentSuggestion = -1;
        emit changed();
        if (isOpen()) {
            emit opened();
        }
    });

    globalContext()->currentNotationChanged().onNotify(this, [this]() { subscribeLyrics(); });
    subscribeLyrics();
}

void PopoverModel::subscribeLyrics()
{
    if (INotationPtr notation = globalContext()->currentNotation()) {
        INotationInteractionPtr interaction = notation->interaction();
        interaction->textEditingStarted().onNotify(this, [this]() { updateLyrics(); }, Asyncable::Mode::SetReplace);
        interaction->textEditingEnded().onReceive(this, [this](engraving::TextBase*) { updateLyrics(); },
                                                  Asyncable::Mode::SetReplace);
        // Every keystroke and placement change re-lays out the score
        notation->notationChanged().onReceive(this, [this](const muse::RectF&) { updateLyrics(); }, Asyncable::Mode::SetReplace);
    }
    updateLyrics();
}

void PopoverModel::updateLyrics()
{
    INotationPtr notation = globalContext()->currentNotation();
    const engraving::TextBase* text = notation && notation->interaction()->isTextEditingStarted()
                                      ? notation->interaction()->editedText() : nullptr;
    const bool open = text && text->isLyrics();
    if (!open && !m_lyricsOpen) {
        return;
    }
    m_lyricsOpen = open;
    if (open) {
        const engraving::Lyrics* lyric = engraving::toLyrics(text);
        m_lyricsLine = static_cast<int>(lyric->verse()) + 1;
        m_lyricsAbove = lyric->placeAbove();
        m_lyricsText = lyric->plainText().toQString();
        m_lyricsRect = lyric->canvasBoundingRect().toQRectF();
    }
    emit lyricsChanged();
    emit geometryChanged();
}

QRectF PopoverModel::lyricsAnchor() const
{
    return m_viewMatrix.value<QTransform>().mapRect(m_lyricsRect);
}

void PopoverModel::lyricsLineStep(int delta)
{
    dispatcher()->dispatch(delta < 0 ? EDITLYRIC_PREV_VERSE_COMMAND : DORICO_LYRICS_NEXT_LINE_COMMAND);
}

void PopoverModel::toggleLyricsPlacement()
{
    dispatcher()->dispatch(m_lyricsAbove ? DORICO_LYRICS_BELOW_COMMAND : DORICO_LYRICS_ABOVE_COMMAND);
}

bool PopoverModel::isOpen() const
{
    return controller()->current() != nullptr;
}

QString PopoverModel::glyph() const
{
    return isOpen() ? QString::fromStdString(controller()->current()->glyph) : QString();
}

bool PopoverModel::scopeButton() const
{
    return isOpen() && controller()->current()->scopeButton;
}

int PopoverModel::fieldWidth() const
{
    return isOpen() ? controller()->current()->fieldWidth : 0;
}

QString PopoverModel::title() const
{
    return isOpen() ? QString::fromStdString(controller()->current()->title) : QString();
}

bool PopoverModel::supportsLocal() const
{
    return isOpen() && controller()->current()->supportsLocal;
}

bool PopoverModel::navigates() const
{
    return isOpen() && controller()->current()->applyAt != nullptr;
}

void PopoverModel::setText(const QString& text)
{
    if (m_text != text) {
        m_text = text;
        m_currentSuggestion = -1;
        emit changed();
    }
}

int PopoverModel::status() const
{
    static int calls = 0;
    qCInfo(doricoPerf) << "popover status" << ++calls;
    return static_cast<int>(controller()->validate(m_text.toStdString()));
}

QStringList PopoverModel::suggestions() const
{
    QStringList list;
    const PopoverDef* def = controller()->current();
    if (def && def->suggest) {
        for (const std::string& s : def->suggest(m_text.toStdString())) {
            list << QString::fromStdString(s);
        }
    }
    return list;
}

void PopoverModel::moveSuggestion(int delta)
{
    const int count = static_cast<int>(suggestions().size());
    const int next = std::clamp(m_currentSuggestion + delta, -1, count - 1);
    if (next != m_currentSuggestion) {
        m_currentSuggestion = next;
        emit changed();
    }
}

void PopoverModel::setViewMatrix(const QVariant& matrix)
{
    if (m_viewMatrix == matrix) {
        return;
    }
    m_viewMatrix = matrix;
    emit geometryChanged();
}

QRectF PopoverModel::anchorRect() const
{
    return m_viewMatrix.value<QTransform>().mapRect(controller()->anchorRect().toQRectF());
}

void PopoverModel::accept(bool local)
{
    const QStringList list = suggestions();
    const QString text = m_currentSuggestion >= 0 && m_currentSuggestion < list.size() ? list.at(m_currentSuggestion) : m_text;
    controller()->accept(text.toStdString(), local ? ApplyScope::Local : ApplyScope::Default);
}

void PopoverModel::move(int popoverMove)
{
    controller()->move(m_text.toStdString(), static_cast<PopoverMove>(popoverMove));
}

void PopoverModel::cancel()
{
    controller()->cancel();
}
