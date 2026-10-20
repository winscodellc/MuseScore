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

#include "transformcontroller.h"

#include <sstream>
#include <vector>

#include "settings.h"

#include "notation/inotation.h"

#include "../doricocommands.h"
#include "../popover/parsers/notetoolsparser.h"
#include "transformapply.h"

#include "log.h"

using namespace mu::dorico;

// Named, not anonymous: the unity build compiles the internal files together.
namespace transformcontroller {
enum class Dialog {
    None,
    Invert,
    Rotate
};

struct Item {
    const char* prefix; // the entry up to what the dialog adds
    Dialog dialog;
    const char* title;
};

// In the order of DORICO_TRANSFORM_COMMANDS
const Item ITEMS[9] = {
    { "", Dialog::Invert, "Invert Pitches" },
    { "rev pt", Dialog::None, "" },
    { "rev pt", Dialog::Invert, "Reverse and Invert Pitches" },
    { "rot pit", Dialog::Rotate, "Rotate Pitches" },
    { "rev rm", Dialog::None, "" },
    { "rot rm", Dialog::Rotate, "Rotate Rhythms" },
    { "rev", Dialog::None, "" },
    { "rev", Dialog::Invert, "Reverse and Invert Pitches and Reverse Rhythms" },
    { "rot", Dialog::Rotate, "Rotate Pitches and Rhythms" },
};

const muse::Settings::Key INVERT_KEY("dorico", "dorico/transform/invert");
const muse::Settings::Key ROTATE_KEY("dorico", "dorico/transform/rotate");

std::vector<std::string> split(const std::string& s)
{
    std::vector<std::string> out;
    std::istringstream in(s);
    for (std::string part; std::getline(in, part, ',');) {
        out.push_back(part);
    }
    return out;
}

// The Invert dialog answers "mode,type,letter,accidental,octave,chord" (InvertPitchesDialog.qml); this is its entry words.
std::string invertWords(const std::string& csv)
{
    const std::vector<std::string> f = split(csv);
    if (f.size() < 6) {
        return "inv";
    }
    const std::string& mode = f[0];
    if (mode == "outer") {
        return "inv";
    }
    std::string words = "inv ";
    if (mode == "pitch") {
        words += std::string(1, "cdefgab"[std::min<size_t>(6, std::stoul(f[2]))]) + f[3] + f[4];
    } else if (mode == "root") {
        words += "root";
    } else {
        words += f[5] == "bottom" ? "bottom" : "top";
    }
    return words + " " + f[1];
}

// The Rotate dialog answers "steps,fw" or "steps,bw"
std::string rotateWords(const std::string& csv)
{
    const std::vector<std::string> f = split(csv);
    return f.size() < 2 ? "1" : (f[1] == "bw" ? "bw " : "") + f[0];
}
}

void DoricoTransformController::init()
{
    using namespace transformcontroller;
    muse::settings()->setDefaultValue(INVERT_KEY, muse::Val(""));
    muse::settings()->setDefaultValue(ROTATE_KEY, muse::Val(""));
    for (size_t i = 0; i < DORICO_TRANSFORM_COMMANDS.size(); ++i) {
        dispatcher()->onRequest(this, DORICO_TRANSFORM_COMMANDS[i], [this, i]() {
            if (globalContext()->currentNotation()) {
                run(i);
            }
            return muse::make_ok();
        });
    }
}

void DoricoTransformController::run(size_t index)
{
    using namespace transformcontroller;
    const Item& item = ITEMS[index];
    const std::string prefix = item.prefix;
    if (item.dialog == Dialog::None) {
        apply(prefix);
        return;
    }

    const bool invert = item.dialog == Dialog::Invert;
    const muse::Settings::Key& key = invert ? INVERT_KEY : ROTATE_KEY;
    muse::UriQuery query(invert ? "musescore://dorico/transforminvert" : "musescore://dorico/transformrotate");
    query.addParam("title", muse::Val(std::string(item.title)));
    query.addParam("last", muse::Val(muse::settings()->value(key).toString()));
    interactive()->open(query).onResolve(this, [this, key, invert, prefix](const muse::Val& result) {
        const std::string csv = result.toString();
        if (csv.empty() || !globalContext()->currentNotation()) {
            return;
        }
        muse::settings()->setSharedValue(key, muse::Val(csv));
        const std::string words = invert ? invertWords(csv) : rotateWords(csv);
        apply(prefix.empty() ? words : prefix + " " + words);
    });
}

void DoricoTransformController::apply(const std::string& entry)
{
    const NoteToolsParse parsed = parseNoteTools(entry);
    if (parsed.status != ParseStatus::Ok || !parsed.transform) {
        LOGW() << "Transform: not a valid entry: " << entry;
        return;
    }
    applyTransform(globalContext()->currentNotation(), *parsed.transform);
}
