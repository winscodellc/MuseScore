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

#include "trackinspectormodel.h"

#include "engraving/dom/engravingitem.h"
#include "engraving/dom/part.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationselection.h"

using namespace mu::dorico;
using namespace mu::notation;
using mu::playback::MixerChannelItem;

TrackInspectorModel::TrackInspectorModel(QObject* parent)
    : QAbstractListModel(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void TrackInspectorModel::setSourceModel(QAbstractItemModel* model)
{
    if (m_source == model) {
        return;
    }
    if (m_source) {
        m_source->disconnect(this);
    }
    m_source = model;
    if (m_source) {
        // Channels are created and removed as instruments change; re-pick ours each time.
        connect(m_source, &QAbstractItemModel::modelReset, this, &TrackInspectorModel::update);
        connect(m_source, &QAbstractItemModel::rowsInserted, this, &TrackInspectorModel::update);
        connect(m_source, &QAbstractItemModel::rowsRemoved, this, &TrackInspectorModel::update);
    }
    emit sourceModelChanged();
    update();
}

void TrackInspectorModel::load()
{
    globalContext()->currentNotationChanged().onNotify(this, [this]() {
        listenToSelection();
    });
    listenToSelection();
}

void TrackInspectorModel::listenToSelection()
{
    // Disconnect from the previous score first: SetReplace only replaces a subscription on the same channel.
    if (m_notation) {
        m_notation->interaction()->selectionChanged().disconnect(this);
    }
    m_notation = globalContext()->currentNotation();
    if (m_notation) {
        m_notation->interaction()->selectionChanged().onNotify(this, [this]() {
            onSelectionChanged();
        });
    }
    onSelectionChanged();
}

void TrackInspectorModel::onSelectionChanged()
{
    if (m_notation) {
        for (const engraving::EngravingItem* item : m_notation->interaction()->selection()->elements()) {
            if (item && item->part()) {
                m_partId = item->part()->id();
                break;
            }
        }
    }
    update();
}

void TrackInspectorModel::setPartId(const QString& partId)
{
    m_partId = muse::ID(partId);
    update();
}

QString TrackInspectorModel::partId() const
{
    return m_channel ? m_channel->instrumentTrackId().partId.toQString() : QString();
}

void TrackInspectorModel::update()
{
    MixerChannelItem* found = nullptr;
    int foundIndex = 0;
    int primaries = 0;
    bool exact = false;
    for (int row = 0; m_source && row < m_source->rowCount(); ++row) {
        const QVariant value = m_source->data(m_source->index(row, 0), ChannelItemRole);
        MixerChannelItem* channel = value.value<MixerChannelItem*>();
        if (!channel || channel->type() != MixerChannelItem::Type::PrimaryInstrument) {
            continue;
        }
        // With nothing selected yet, show the first instrument rather than an empty inspector.
        if (!exact && (!found || channel->instrumentTrackId().partId == m_partId)) {
            found = channel;
            foundIndex = primaries;
            exact = channel->instrumentTrackId().partId == m_partId;
        }
        ++primaries; // count them all: the colour depends on how many instruments there are
    }

    if (found == m_channel && foundIndex == m_colorIndex && primaries == m_colorCount) {
        return;
    }
    m_colorIndex = foundIndex;
    m_colorCount = primaries;

    beginResetModel();
    if (m_channel) {
        m_channel->disconnect(this);
    }
    m_channel = found;
    if (m_channel) {
        // The mixer fills in channel titles asynchronously
        connect(m_channel, &MixerChannelItem::titleChanged, this, &TrackInspectorModel::channelChanged);
    }
    endResetModel();
    emit channelChanged();
}

QString TrackInspectorModel::trackTitle() const
{
    return m_channel ? m_channel->title() : QString();
}

QVariant TrackInspectorModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() != 0 || !m_channel || role != ChannelItemRole) {
        return QVariant();
    }
    return QVariant::fromValue(m_channel.data());
}

int TrackInspectorModel::rowCount(const QModelIndex&) const
{
    return m_channel ? 1 : 0;
}

QHash<int, QByteArray> TrackInspectorModel::roleNames() const
{
    return { { ChannelItemRole, "channelItem" } };
}
