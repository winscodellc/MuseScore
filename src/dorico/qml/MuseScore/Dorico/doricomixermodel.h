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
#include <QVariantMap>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "context/iglobalcontext.h"
#include "playback/iplaybackconfiguration.h"
#include "playback/iplaybackcontroller.h"

#include "mixerchannelitem.h"

namespace mu::dorico {
//! The Dorico Mixer's own data (spec 63): channel names typed by the user, kept in a metaTag of the master score so
//! they are saved with the project, and Add / Remove Effects Channel, which show / hide MuseScore's fixed aux
//! channels (an app-wide setting in MuseScore). Every instance sees the same names: renaming reloads them all.
class DoricoMixerModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    //! Name per channel key (see keyOf); channels without an entry use their MuseScore title.
    Q_PROPERTY(QVariantMap names READ names NOTIFY namesChanged)
    Q_PROPERTY(int fxChannelMax READ fxChannelMax CONSTANT)

    QML_ELEMENT

    muse::GlobalInject<playback::IPlaybackConfiguration> playbackConfiguration;
    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<playback::IPlaybackController> playbackController = { this };

public:
    explicit DoricoMixerModel(QObject* parent = nullptr);

    QVariantMap names() const { return m_names; }
    int fxChannelMax() const;

    Q_INVOKABLE void load();
    //! "partId/instrumentId", "metronome" or "aux:<n>"; empty for the output, which cannot be renamed.
    Q_INVOKABLE QString keyOf(mu::playback::MixerChannelItem* channel) const;
    //! An empty name removes the entry, so the channel shows its default name again.
    Q_INVOKABLE void setName(const QString& key, const QString& name);

    Q_INVOKABLE void addFxChannel();
    Q_INVOKABLE void removeFxChannel();

signals:
    void namesChanged();

private:
    void readNames();
    void setFxChannelMuted(muse::audio::aux_channel_idx_t index, bool muted);

    QVariantMap m_names;
};
}
