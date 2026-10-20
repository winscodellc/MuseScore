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

#include "jumpbarcontroller.h"

#include <QString>

#include "settings.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/page.h"
#include "engraving/dom/score.h"

#include "notation/inotation.h"
#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationselection.h"

#include "../../doricocommands.h"
#include "gotoparser.h"

using namespace mu::dorico;
using namespace mu::engraving;
using namespace mu::notation;
using namespace muse;
using namespace muse::rcommand;

namespace {
const std::string mname("dorico");
const Settings::Key USAGE_KEY(mname, "dorico/jumpbar/usage");     // command uri -> times run
const Settings::Key ALIASES_KEY(mname, "dorico/jumpbar/aliases"); // alias -> command uri
const Settings::Key LAST_KEY(mname, "dorico/jumpbar/last");       // the command run last

constexpr size_t MAX_ROWS = 8;

UsageCounts usageCounts()
{
    UsageCounts usage;
    for (const auto& [uri, count] : settings()->value(USAGE_KEY).toMap()) {
        usage[uri] = count.toInt();
    }
    return usage;
}

//! The bar the status bar calls `n`: a pickup bar has no number, so the first full bar after it is 1.
Measure* barNumbered(const Score* score, int n)
{
    for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
        if (m->measureNumber() + 1 == n) {
            return m;
        }
    }
    return nullptr;
}

Measure* currentMeasure(const INotationSelectionPtr& selection)
{
    MeasureBase* mb = selection->startMeasureBase();
    return mb && mb->isMeasure() ? toMeasure(mb) : nullptr;
}
}

void JumpBarController::init()
{
    settings()->setDefaultValue(USAGE_KEY, Val(ValMap()));
    settings()->setDefaultValue(ALIASES_KEY, Val(ValMap()));
    settings()->setDefaultValue(LAST_KEY, Val(std::string()));

    dispatcher()->onRequest(this, DORICO_JUMP_BAR_COMMAND, [this]() {
        open(m_mode, m_mode == JumpBarMode::Commands ? lastCommandTitle() : std::string());
        return make_ok();
    });
    dispatcher()->onRequest(this, DORICO_JUMP_BAR_COMMANDS_COMMAND, [this]() {
        open(JumpBarMode::Commands, lastCommandTitle());
        return make_ok();
    });
    dispatcher()->onRequest(this, DORICO_JUMP_BAR_GOTO_COMMAND, [this]() {
        open(JumpBarMode::GoTo, std::string());
        return make_ok();
    });
    dispatcher()->onRequest(this, DORICO_GO_TO_BAR_COMMAND, [this]() {
        open(JumpBarMode::GoTo, "b");
        return make_ok();
    });
}

void JumpBarController::open(JumpBarMode mode, const std::string& text)
{
    if (!globalContext()->currentNotation()) {
        return;
    }
    m_open = true;
    m_mode = mode;
    m_initialText = text;
    m_stateChanged.notify();
}

void JumpBarController::close()
{
    if (m_open) {
        m_open = false;
        m_stateChanged.notify();
    }
}

void JumpBarController::setMode(JumpBarMode mode)
{
    if (m_open && mode != m_mode) {
        m_mode = mode;
        m_initialText.clear();
        m_stateChanged.notify();
    }
}

std::string JumpBarController::lastCommandTitle() const
{
    const std::string uri = settings()->value(LAST_KEY).toString();
    if (uri.empty()) {
        return {};
    }
    const CommandInfo& info = commandsRegister()->commandInfo(Command(uri));
    return info.isValid() ? info.title.qTranslatedWithoutMnemonic().toStdString() : std::string();
}

std::vector<CatalogEntry> JumpBarController::catalog() const
{
    std::map<std::string, std::string> aliasOf; // uri -> alias
    for (const auto& [alias, uri] : settings()->value(ALIASES_KEY).toMap()) {
        aliasOf[uri.toString()] = alias;
    }

    std::vector<CatalogEntry> entries;
    for (const CommandInfo& info : commandsRegister()->commandInfoList()) {
        // Commands with input parameters need their own dialog, which Opt-Return would open: not offered yet (spec 46)
        if (!info.availabilities.testFlag(Availability::Dispatch) || !info.inputSchema.args.empty()
            || !commandsState()->commandState(info.command).enabled) {
            continue;
        }
        const std::string uri = info.command.toString();
        const auto it = aliasOf.find(uri);
        entries.push_back({ uri,
                            info.title.qTranslatedWithoutMnemonic().toStdString(),
                            muse::shortcuts::sequencesToNativeText(shortcutsRegister()->shortcut(info.command).sequences).toStdString(),
                            it == aliasOf.end() ? std::string() : it->second });
    }
    return entries;
}

std::vector<CatalogEntry> JumpBarController::search(const std::string& text) const
{
    std::vector<CatalogEntry> rows = rankCommands(catalog(), splitAlias(text).name, usageCounts());
    rows.resize(std::min(rows.size(), MAX_ROWS)); // ponytail: no scrolling in the result list; the best rows are the first
    return rows;
}

std::vector<CatalogEntry> JumpBarController::mostUsed() const
{
    return topUsed(catalog(), usageCounts());
}

void JumpBarController::setAlias(const std::string& uri, const std::string& alias)
{
    // One alias per command, and an alias names one command
    ValMap aliases;
    for (const auto& [a, u] : settings()->value(ALIASES_KEY).toMap()) {
        if (a != alias && u.toString() != uri) {
            aliases[a] = u;
        }
    }
    aliases[alias] = Val(uri);
    settings()->setSharedValue(ALIASES_KEY, Val(aliases));
}

void JumpBarController::recordUse(const std::string& uri)
{
    ValMap usage = settings()->value(USAGE_KEY).toMap();
    usage[uri] = Val(usage[uri].toInt() + 1);
    settings()->setSharedValue(USAGE_KEY, Val(usage));
    settings()->setSharedValue(LAST_KEY, Val(uri));
}

bool JumpBarController::accept(const std::string& text, const std::string& uri, bool assignOnly)
{
    if (m_mode == JumpBarMode::GoTo) {
        if (!goTo(text)) {
            return false;
        }
        close();
        return true;
    }

    if (uri.empty()) {
        return false;
    }
    if (const auto alias = splitAlias(text).alias) {
        setAlias(uri, *alias);
    } else if (assignOnly) {
        return false;
    }
    close(); // first: a command may open a popover or take the keyboard focus
    if (!assignOnly) {
        recordUse(uri);
        dispatcher()->dispatch(Command(uri));
    }
    return true;
}

// Relative forms (`b+4`, `p+2`) count from the bar the selection starts in, or from the first bar: our reading,
// the manual does not explain `+`.
bool JumpBarController::goTo(const std::string& text)
{
    const GoToTarget t = parseGoTo(text);
    const INotationPtr notation = globalContext()->currentNotation();
    if (!t.valid || !notation) {
        return false;
    }
    const INotationInteractionPtr interaction = notation->interaction();
    const INotationElementsPtr elements = notation->elements();
    const Measure* current = currentMeasure(interaction->selection());

    Measure* target = nullptr;
    if (!t.rehearsal.empty()) {
        const std::vector<EngravingItem*> found = elements->search(QString::fromStdString("r" + t.rehearsal));
        target = found.empty() ? nullptr : found.front()->findMeasure();
    } else if (t.bar) {
        target = t.barRelative ? elements->measure((current ? current->measureIndex() : 0) + *t.bar)
                 : barNumbered(elements->msScore(), *t.bar);
    } else if (t.page) {
        const PageList& pages = elements->pages();
        const size_t from = t.pageRelative && current && current->page() ? current->page()->pageNumber() : 0;
        const size_t index = t.pageRelative ? from + *t.page : *t.page - 1;
        target = index < pages.size() ? pages[index]->firstMeasure() : nullptr;
    }
    if (!target) {
        return false;
    }

    // Selecting one measure makes a range selection of it on that staff, which also moves the playhead there
    interaction->select({ target }, SelectType::SINGLE, 0);
    interaction->showItem(target);
    return true;
}
