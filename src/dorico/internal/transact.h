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

#include <functional>

#include "global/types/translatablestring.h"
#include "global/log.h"

#include "engraving/dom/score.h"
#include "engraving/editing/transaction/transaction.h"

#include "notation/inotation.h"
#include "notation/inotationelements.h"
#include "notation/inotationundostack.h"

namespace mu::dorico {
//! Runs `body` as one undo step on the score of `notation`; if it returns false the score is rolled back.
inline bool transact(const notation::INotationPtr& notation, const muse::TranslatableString& title,
                     const std::function<bool(engraving::Score*, engraving::Transaction&)>& body)
{
    engraving::Score* score = notation->elements()->msScore();
    notation::INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(title);
    engraving::Transaction& tx = score->transactionManager()->currentOrDummyTransaction();
    if (!body(score, tx)) {
        undoStack->rollbackChanges();
        LOGW() << "Nothing to do, or no room: " << title.str.toStdString();
        return false;
    }
    undoStack->commitChanges();
    return true;
}
}
