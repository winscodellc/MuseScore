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

#include "doricomixermodel.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "engraving/dom/masterscore.h"
#include "notation/imasternotation.h"
#include "project/inotationproject.h"
#include "playback/playbacktypes.h"

using namespace mu::dorico;
using mu::playback::MixerChannelItem;
using muse::audio::aux_channel_idx_t;

namespace {
const muse::String MIXER_NAMES_TAG = u"doricoMixerNames";

// Tells every DoricoMixerModel (one per channel strip) that the names changed.
muse::async::Notification& mixerNamesChanged()
{
    static muse::async::Notification notification;
    return notification;
}
}

DoricoMixerModel::DoricoMixerModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

int DoricoMixerModel::fxChannelMax() const
{
    return static_cast<int>(playback::AUX_CHANNEL_NUM);
}

void DoricoMixerModel::load()
{
    globalContext()->currentMasterNotationChanged().onNotify(this, [this]() { readNames(); });
    mixerNamesChanged().onNotify(this, [this]() { readNames(); });
    readNames();
}

void DoricoMixerModel::readNames()
{
    const notation::IMasterNotationPtr master = globalContext()->currentMasterNotation();
    const engraving::MasterScore* score = master ? master->masterScore() : nullptr;
    const QByteArray json = score ? score->metaTag(MIXER_NAMES_TAG).toQString().toUtf8() : QByteArray();

    m_names = QJsonDocument::fromJson(json).object().toVariantMap();
    emit namesChanged();
}

QString DoricoMixerModel::keyOf(MixerChannelItem* channel) const
{
    if (!channel) {
        return QString();
    }

    switch (channel->type()) {
    case MixerChannelItem::Type::PrimaryInstrument:
    case MixerChannelItem::Type::SecondaryInstrument: {
        const engraving::InstrumentTrackId& id = channel->instrumentTrackId();
        return QString::number(id.partId.toUint64()) + u'/' + id.instrumentId.toQString();
    }
    case MixerChannelItem::Type::Metronome:
        return QStringLiteral("metronome");
    case MixerChannelItem::Type::Aux:
        for (const auto& [index, trackId] : playbackController()->auxTrackIdMap()) {
            if (trackId == channel->trackId()) {
                return QStringLiteral("aux:%1").arg(index);
            }
        }
        return QString();
    default:
        return QString();
    }
}

void DoricoMixerModel::setName(const QString& key, const QString& name)
{
    const notation::IMasterNotationPtr master = globalContext()->currentMasterNotation();
    engraving::MasterScore* score = master ? master->masterScore() : nullptr;
    if (key.isEmpty() || !score) {
        return;
    }

    QVariantMap names = m_names;
    if (name.trimmed().isEmpty()) {
        names.remove(key);
    } else {
        names.insert(key, name.trimmed());
    }
    if (names == m_names) {
        return;
    }

    const QByteArray json = QJsonDocument(QJsonObject::fromVariantMap(names)).toJson(QJsonDocument::Compact);
    score->setMetaTag(MIXER_NAMES_TAG, muse::String::fromUtf8(json.constData()));

    // A tag change is not an undoable edit, so the project is told directly that it has something to save.
    if (project::INotationProjectPtr project = globalContext()->currentProject()) {
        project->markAsUnsaved();
    }

    mixerNamesChanged().notify();
}

void DoricoMixerModel::addFxChannel()
{
    for (aux_channel_idx_t i = 0; i < playback::AUX_CHANNEL_NUM; ++i) {
        if (!playbackConfiguration()->isAuxChannelVisible(i)) {
            playbackConfiguration()->setAuxChannelVisible(i, true);
            playbackConfiguration()->setAuxSendVisible(i, true);
            return;
        }
    }
}

void DoricoMixerModel::removeFxChannel()
{
    // Dorico removes the rightmost FX channel
    for (aux_channel_idx_t i = playback::AUX_CHANNEL_NUM; i-- > 0;) {
        if (playbackConfiguration()->isAuxChannelVisible(i)) {
            playbackConfiguration()->setAuxChannelVisible(i, false);
            playbackConfiguration()->setAuxSendVisible(i, false);
            return;
        }
    }
}
