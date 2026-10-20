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

#include "doricomenu.h"

#include <algorithm>
#include <cctype>

#include "../doricocommands.h"
#include "filters.h"

using namespace mu::dorico;

namespace {
MenuSpec item(const char* id) { return { id, {}, {} }; }
MenuSpec sep() { return { "-", {}, {} }; }
MenuSpec menu(const char* id, const char* title, std::vector<MenuSpec> children) { return { id, title, std::move(children) }; }
MenuSpec titled(const std::string& id, const std::string& title) { return { QString::fromStdString(id), QString::fromStdString(title), {} }; }

//! Edit > Filter (spec 45): the two modes, a submenu per group of filters, then the single-category filters
MenuSpec filterMenu()
{
    std::vector<MenuSpec> children = { item("command://dorico/filter-select-only"), item("command://dorico/filter-deselect-only"), sep() };
    std::vector<std::string> groups;
    for (const FilterDef& def : filterDefs()) {
        if (!def.group.empty() && std::find(groups.begin(), groups.end(), def.group) == groups.end()) {
            groups.push_back(def.group);
        }
    }
    for (const std::string& group : groups) {
        std::string id = "dorico-menu-filter-" + group;
        std::transform(id.begin(), id.end(), id.begin(), [](unsigned char c) { return std::tolower(c); });
        std::vector<MenuSpec> entries;
        for (const FilterDef& def : filterDefs()) {
            if (def.group == group) {
                entries.push_back(titled(doricoFilterCommandUri(def.id), def.title));
            }
        }
        if (group == "Notes") {
            entries.push_back(titled("command://dorico/filter-notes-by-pitch", "Notes by Pitch…"));
        }
        children.push_back({ QString::fromStdString(id), QString::fromStdString(group), std::move(entries) });
    }
    for (const FilterDef& def : filterDefs()) {
        if (def.group.empty()) {
            children.push_back(titled(doricoFilterCommandUri(def.id), def.title));
        }
    }
    return { "dorico-menu-filter", "Filter", std::move(children) };
}
}

const std::vector<MenuSpec>& mu::dorico::doricoMenus()
{
    // ponytail: ids are MuseScore's command URIs as AppMenuModel builds them (appshell/.../appmenumodel.cpp). An id that
    // is neither in MuseScore's menus nor a registered command is skipped with a LOGW by DoricoMenuArranger; any
    // MuseScore item not named here lands in Window > More MuseScore Commands.
    static const std::vector<MenuSpec> menus = {
        menu("dorico-menu-file", "File", {
            item("command://project/new"),
            item("command://project/open"),
            item("menu-file-open"),
            item("command://project/close"),
            sep(),
            item("command://project/save"),
            item("command://project/save-as"),
            item("command://project/save-a-copy"),
            item("command://project/save-selection"),
            item("command://project/save-to-cloud"),
            sep(),
            item("command://project/properties"),
            item("command://project/export"),
            item("command://dorico/open-publish"),
            sep(),
            item("command://project/print"),
            sep(),
            item("command://app/quit"),
        }),
        menu("dorico-menu-edit", "Edit", {
            item("command://notation/undo"),
            item("command://notation/redo"),
            item("command://app/dock/toggle-undo-history"),
            sep(),
            item("command://notation/cut"),
            item("command://notation/copy"),
            item("command://notation/paste"),
            menu("dorico-menu-paste-special", "Paste Special", {
                item("command://notation/paste-half"),
                item("command://notation/paste-double"),
                item("command://notation/copy-paste-swap"),
                sep(),
                menu("dorico-menu-paste-into-voice", "Paste Into Voice", {
                    item("command://dorico/paste-into-voice-1"),
                    item("command://dorico/paste-into-voice-2"),
                    item("command://dorico/paste-into-voice-3"),
                    item("command://dorico/paste-into-voice-4"),
                    item("command://dorico/paste-into-new-voice"),
                }),
                item("command://dorico/duplicate-to-staff-above"),
                item("command://dorico/duplicate-to-staff-below"),
                item("command://dorico/move-to-staff-above"),
                item("command://dorico/move-to-staff-below"),
                item("command://dorico/swap-staves"),
                item("command://dorico/explode"),
                item("command://dorico/reduce"),
                item("command://dorico/paste-articulations"),
            }),
            item("command://notation/delete"),
            item("command://dorico/delete-bars"),
            item("command://dorico/remove-rests"),
            sep(),
            item("command://notation/select-all"),
            item("command://dorico/deselect-all"),
            item("command://dorico/select-more"),
            menu("dorico-menu-select-to", "Select To", {
                item("command://notation/add-to-selection-end-system"),
                item("command://notation/add-to-selection-begin-system"),
                item("command://notation/add-to-selection-end-score"),
                item("command://notation/add-to-selection-begin-score"),
            }),
            item("command://notation/select-section"),
            filterMenu(),
            item("command://app/dock/toggle-selection-filter"),
            item("command://notation/view/show-search"),
            sep(),
            item("command://notation/reset-shapes-and-positions"),
            item("command://notation/reset-text-style-overrides"),
            sep(),
            item("command://app/preferences"),
        }),
        menu("dorico-menu-setup", "Setup", {
            item("command://dorico/mode-setup"),
            sep(),
            item("command://instruments/select"),
            item("command://notation/open-parts"),
        }),
        menu("dorico-menu-write", "Write", {
            item("command://dorico/mode-write"),
            item("command://notation/toggle-note-input"),
            sep(),
            item("menu-notes"),
            item("menu-intervals"),
            item("menu-tuplets"),
            item("menu-measures"),
            item("menu-text"),
            item("menu-lines"),
            item("menu-chord-and-frets"),
            sep(),
            item("command://notation/open-transpose"),
            menu("dorico-menu-respell", "Respell", {
                item("command://notation/pitch-spell-sharps"),
                item("command://notation/pitch-spell-flats"),
                item("menu-enharmonic-spelling"),
            }),
            menu("dorico-menu-transform", "Transform", {
                menu("dorico-menu-transform-pitches", "Pitches", {
                    item("command://dorico/transform/invert-pitches"),
                    item("command://dorico/transform/reverse-pitches"),
                    item("command://dorico/transform/reverse-invert-pitches"),
                    item("command://dorico/transform/rotate-pitches"),
                }),
                menu("dorico-menu-transform-rhythm", "Rhythm", {
                    item("command://dorico/transform/reverse-rhythms"),
                    item("command://dorico/transform/rotate-rhythms"),
                }),
                menu("dorico-menu-transform-all", "All", {
                    item("command://dorico/transform/reverse-all"),
                    item("command://dorico/transform/reverse-invert-all"),
                    item("command://dorico/transform/rotate-all"),
                }),
            }),
            item("menu-voices"),
            item("menu-tools-measures"),
            sep(),
            item("command://notation/staff-implode"),
            item("command://notation/open-realizechordsymbols"),
            item("command://notation/slash-fill"),
            item("command://notation/slash-rhythm"),
            item("command://notation/regroup-rhythms"),
            item("command://notation/resequence-rehearsal-marks"),
            item("command://notation/copy-lyrics"),
            item("command://notation/remove-empty-trailing-measures"),
        }),
        menu("dorico-menu-engrave", "Engrave", {
            item("command://dorico/mode-engrave"),
            sep(),
            item("command://notation/open-page-settings"),
            item("command://notation/open-breaks"),
            item("menu-stretch"),
            item("menu-frames"),
            sep(),
            item("command://notation/reset-beams"),
            item("command://notation/reset-to-default-layout"),
        }),
        menu("dorico-menu-play", "Play", {
            item("command://dorico/mode-play"),
            sep(),
            item("command://playback/play"),
            item("command://playback/rewind"),
            item("command://playback/loop-toggle"),
            item("command://playback/metronome-toggle"),
            sep(),
            item("command://playback/open-playback-setup"),
            item("command://app/dock/toggle-mixer"),
            item("command://playback/reload-playback-cache"),
        }),
        menu("dorico-menu-library", "Library", {
            item("command://notation/open-edit-style"),
            item("command://notation/load-style"),
            item("command://notation/save-style"),
            sep(),
            item("command://palette/toggle-master-palette"),
            item("command://app/dock/toggle-palettes"),
        }),
        menu("dorico-menu-view", "View", {
            item("command://app/fullscreen"),
            sep(),
            item("menu-show"),
            sep(),
            item("command://app/dock/toggle-navigator"),
            item("command://app/dock/toggle-braille"),
        }),
        menu("dorico-menu-window", "Window", {
            item("command://dorico/open-hub"),
            sep(),
            item("command://dorico/mode-setup"),
            item("command://dorico/mode-write"),
            item("command://dorico/mode-engrave"),
            item("command://dorico/mode-play"),
            item("command://dorico/mode-print"),
            sep(),
            item("command://dorico/toggle-toolbar"),
            item("command://dorico/toggle-zone-left"),
            item("command://dorico/toggle-zone-right"),
            item("command://dorico/toggle-zone-lower"),
            item("command://dorico/toggle-all-zones"),
            sep(),
            item("command://app/dock/toggle-mixer"),
            item("command://app/dock/toggle-properties"),
            item("command://app/dock/toggle-timeline"),
            item("command://app/dock/toggle-piano-keyboard"),
            item("command://app/dock/toggle-percussion"),
            sep(),
            item("menu-workspaces"),
            item("command://dock/restore-default-layout"),
            sep(),
            item("menu-extensions"),
        }),
        menu("dorico-menu-help", "Help", {
            item("command://app/online-handbook"),
            item("command://app/ask-help"),
            item("command://update/check"),
            sep(),
            item("command://app/about-musescore"),
            item("command://app/about-qt"),
            item("command://app/about-musicxml"),
            item("command://app/accessibility-statement"),
            sep(),
            item("command://playback/clear-onlinesounds-cache"),
            item("command://app/revert-to-factory"),
            item("menu-diagnostic"),
        }),
    };
    return menus;
}

static void collectPlaced(const std::vector<MenuSpec>& specs, QStringList& out)
{
    for (const MenuSpec& spec : specs) {
        if (spec.isSeparator()) {
            continue;
        }
        if (spec.isSubmenu()) {
            collectPlaced(spec.children, out);
        } else if (!out.contains(spec.id)) {
            out << spec.id;
        }
    }
}

QStringList mu::dorico::placedItemIds(const std::vector<MenuSpec>& menus)
{
    QStringList ids;
    collectPlaced(menus, ids);
    return ids;
}
