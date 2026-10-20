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


#include "selectmore.h"

#include <algorithm>
#include <map>

using namespace mu::dorico;

namespace {
struct TickSpan {
    int min = 0;
    int max = 0;
};

SelectMoreRange expanded(const TickSpan& span, int stage, const SelectMoreContext& ctx)
{
    if (stage == 1) {
        // One tick (a lone item, or all notes of one chord): its bar. Two or more: everything between the first and the last.
        return span.min == span.max ? ctx.bar(span.min) : SelectMoreRange { span.min, span.max + 1 };
    }
    if (stage == 2 && !ctx.galley) {
        return { std::min(ctx.system(span.min).start, ctx.system(span.max).start), std::max(ctx.system(span.min).end,
                                                                                            ctx.system(span.max).end) };
    }
    return ctx.flow;
}
}

SelectMoreResult mu::dorico::selectMore(const std::vector<SelectMoreItem>& all, std::vector<size_t> selected, int stage,
                                        const SelectMoreContext& ctx)
{
    std::sort(selected.begin(), selected.end());

    std::map<SelectKey, TickSpan> spans;
    for (size_t i : selected) {
        const SelectMoreItem& item = all[i];
        auto [it, added] = spans.try_emplace(item.key, TickSpan { item.tick, item.tick });
        if (!added) {
            it->second.min = std::min(it->second.min, item.tick);
            it->second.max = std::max(it->second.max, item.tick);
        }
    }

    SelectMoreResult result { selected, 3 };
    for (int s = stage + 1; s <= 3; ++s) {
        std::vector<size_t> indices;
        for (size_t i = 0; i < all.size(); ++i) {
            const auto it = spans.find(all[i].key);
            if (it == spans.end()) {
                continue;
            }
            const SelectMoreRange range = expanded(it->second, s, ctx);
            if (all[i].tick >= range.start && all[i].tick < range.end) {
                indices.push_back(i);
            }
        }
        result = { indices, s };
        if (indices != selected) {
            break;
        }
    }
    return result;
}
