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
#include "notation/inotation.h"
#include "engraving/editing/editscoreproperties.h"
#include "notation/inotationundostack.h"
#include "project/inotationproject.h"
#include "project/iprojectaudiosettings.h"
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
    // Undo / redo of a rename changes the tag
    auto listenToUndo = [this]() {
        if (const notation::IMasterNotationPtr master = globalContext()->currentMasterNotation()) {
            master->notation()->undoStack()->stackChanged().onNotify(this, [this]() { readNames(); });
        }
    };
    globalContext()->currentMasterNotationChanged().onNotify(this, [this, listenToUndo]() {
        listenToUndo();
        readNames();
    });
    listenToUndo();
    mixerNamesChanged().onNotify(this, [this]() { readNames(); });
    readNames();
}

void DoricoMixerModel::readNames()
{
    const notation::IMasterNotationPtr master = globalContext()->currentMasterNotation();
    const engraving::MasterScore* score = master ? master->masterScore() : nullptr;
    const QByteArray json = score ? score->metaTag(MIXER_NAMES_TAG).toQString().toUtf8() : QByteArray();

    const QVariantMap names = QJsonDocument::fromJson(json).object().toVariantMap();
    if (names == m_names) { // most undo-stack changes are other edits
        return;
    }
    m_names = names;
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

    // An undoable edit, so the project knows it has something to save even after other edits are undone
    const QByteArray json = QJsonDocument(QJsonObject::fromVariantMap(names)).toJson(QJsonDocument::Compact);
    std::map<muse::String, muse::String> tags = score->metaTags();
    tags[MIXER_NAMES_TAG] = muse::String::fromUtf8(json.constData());
    const notation::INotationUndoStackPtr undoStack = master->notation()->undoStack();
    undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Rename mixer channel"));
    score->undo(new engraving::ChangeMetaTags(score, tags));
    undoStack->commitChanges();

    mixerNamesChanged().notify();
}

// MuseScore's aux visibility is display only, so a removed channel would still be heard: it is muted instead (Dorico
// deletes it). Through the project's audio settings, which the playback controller applies to the aux track.
void DoricoMixerModel::setFxChannelMuted(aux_channel_idx_t index, bool muted)
{
    const project::INotationProjectPtr project = globalContext()->currentProject();
    if (!project) {
        return;
    }
    notation::INotationSoloMuteState::SoloMuteState state = project->audioSettings()->auxSoloMuteState(index);
    state.mute = muted;
    state.solo = false;
    project->audioSettings()->setAuxSoloMuteState(index, state);
}

void DoricoMixerModel::addFxChannel()
{
    for (aux_channel_idx_t i = 0; i < playback::AUX_CHANNEL_NUM; ++i) {
        if (!playbackConfiguration()->isAuxChannelVisible(i)) {
            playbackConfiguration()->setAuxChannelVisible(i, true);
            playbackConfiguration()->setAuxSendVisible(i, true);
            setFxChannelMuted(i, false);
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
            setFxChannelMuted(i, true);
            return;
        }
    }
}
