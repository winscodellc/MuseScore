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

#include <QAbstractListModel>
#include <QPointer>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "context/iglobalcontext.h"
#include "notation/inotation.h"

#include "mixerchannelitem.h"

namespace mu::dorico {
//! Play mode's Track Inspector (spec 57): a one-row view of a MixerPanelModel holding just the mixer channel of the
//! selected instrument. MuseScore's mixer section components (sound, audio FX, fader, mute/solo) take any list
//! model with a "channelItem" role, so they render this one channel unchanged. With nothing selected it keeps the
//! last instrument, like Dorico's inspector.
class TrackInspectorModel : public QAbstractListModel, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel * sourceModel READ sourceModel WRITE setSourceModel NOTIFY sourceModelChanged)
    Q_PROPERTY(QString trackTitle READ trackTitle NOTIFY channelChanged)

    QML_ELEMENT

    muse::ContextInject<context::IGlobalContext> globalContext = { this };

public:
    explicit TrackInspectorModel(QObject* parent = nullptr);

    QAbstractItemModel* sourceModel() const { return m_source; }
    void setSourceModel(QAbstractItemModel* model);

    QString trackTitle() const;

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void load();

signals:
    void sourceModelChanged();
    void channelChanged();

private:
    static constexpr int ChannelItemRole = Qt::UserRole + 1;

    void listenToSelection();
    void update();

    QPointer<QAbstractItemModel> m_source;
    QPointer<playback::MixerChannelItem> m_channel;
    notation::INotationPtr m_notation;
    muse::ID m_partId;
};
}
