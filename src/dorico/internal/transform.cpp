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

#include "transform.h"

#include <algorithm>

#include "engraving/dom/pitchspelling.h"

using namespace mu::engraving;

namespace mu::dorico::transform {
int stepOf(int pitch, int key, int& alter)
{
    const Key k = static_cast<Key>(key);
    return pitch2absStepByKey(pitch, pitch2tpc(pitch, k, Prefer::NEAREST), k, alter);
}

int invertPitch(int pitch, const InvertContext& ctx)
{
    if (!ctx.diatonic) {
        return ctx.twiceCentre - pitch;
    }
    int alter = 0;
    const int step = ctx.stepSum - stepOf(pitch, ctx.key, alter);
    if (step < MIN_STEP || step > MAX_STEP) {
        return -1;
    }
    return absStep2pitchByKey(step, static_cast<Key>(ctx.key)) - alter;
}

std::vector<size_t> reverseOrder(size_t n)
{
    std::vector<size_t> order(n);
    for (size_t i = 0; i < n; ++i) {
        order[i] = n - 1 - i;
    }
    return order;
}

std::vector<size_t> rotateOrder(size_t n, int steps)
{
    std::vector<size_t> order(n);
    if (n == 0) {
        return order;
    }
    const long long shift = ((steps % static_cast<long long>(n)) + static_cast<long long>(n)) % static_cast<long long>(n);
    for (size_t i = 0; i < n; ++i) {
        order[(i + static_cast<size_t>(shift)) % n] = i;
    }
    return order;
}

namespace {
std::vector<size_t> orderFor(const TransformSpec& spec, size_t n)
{
    switch (spec.op) {
    case TransformOp::Reverse: return reverseOrder(n);
    case TransformOp::Rotate: return rotateOrder(n, spec.steps);
    case TransformOp::Invert: break;
    }
    std::vector<size_t> identity(n);
    for (size_t i = 0; i < n; ++i) {
        identity[i] = i;
    }
    return identity;
}
}

std::vector<Event> apply(const std::vector<Event>& events, const TransformSpec& spec, const InvertContext& ctx)
{
    std::vector<Event> out = events;
    if (spec.op != TransformOp::Invert) {
        if (spec.pitches && spec.rhythms) {
            const std::vector<size_t> order = orderFor(spec, events.size());
            for (size_t i = 0; i < events.size(); ++i) {
                out[i] = events[order[i]];
            }
        } else if (spec.rhythms) {
            const std::vector<size_t> order = orderFor(spec, events.size());
            for (size_t i = 0; i < events.size(); ++i) {
                out[i].len = events[order[i]].len;
            }
        } else if (spec.pitches) {
            std::vector<size_t> notes; // positions of the note events
            for (size_t i = 0; i < events.size(); ++i) {
                if (!events[i].isRest()) {
                    notes.push_back(i);
                }
            }
            const std::vector<size_t> order = orderFor(spec, notes.size());
            for (size_t i = 0; i < notes.size(); ++i) {
                out[notes[i]].notes = events[notes[order[i]]].notes;
            }
        }
    }

    if (spec.invert) {
        for (Event& e : out) {
            for (NoteVal& n : e.notes) {
                n.pitch = invertPitch(n.pitch, ctx);
                n.tpc1 = Tpc::TPC_INVALID; // respelled from the key
                n.tpc2 = Tpc::TPC_INVALID;
            }
            std::sort(e.notes.begin(), e.notes.end(), [](const NoteVal& a, const NoteVal& b) { return a.pitch < b.pitch; });
        }
    }
    return out;
}
}
