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
MenuSpec item(const char* id, const char* title) { return { id, title, {} }; }
MenuSpec sep() { return { "-", {}, {} }; }
// A disabled placeholder; the id is made unique here so the table needs no slugs.
MenuSpec stub(const char* title)
{
    static int count = 0;
    return { STUB_ID_PREFIX + QString::number(++count), title, {} };
}
// "-" in the list is a separator
std::vector<MenuSpec> stubs(const std::vector<const char*>& titles)
{
    std::vector<MenuSpec> out;
    for (const char* title : titles) {
        out.push_back(std::string(title) == "-" ? sep() : stub(title));
    }
    return out;
}
MenuSpec stubMenu(const char* title, const std::vector<const char*>& titles)
{
    static int count = 0;
    return { STUB_ID_PREFIX + "menu-" + QString::number(++count), title, stubs(titles) };
}
// Create Text and Create System Text list the same paragraph styles
const std::vector<const char*> TEXT_STYLES = {
    "Bar numbers (parts)", "Bar numbers (score)", "Composer", "Copyright", "Cue Labels", "Default Text", "Figured Bass Tasto Solo",
    "Flow Title", "Galley View Margin Labels", "Harp Pedal Settings", "Header", "Instrument Change Labels", "Instrument Change Warnings",
    "Layout name", "Lyricist", "Lyrics", "Lyrics (Chorus Translation)", "Lyrics (Chorus)", "Lyrics (Translation)",
    "Lyrics (Verse Numbers)", "MIDI Trigger Region Labels", "Page Number", "Percussion Legend", "Player Group Labels", "Player Labels",
    "Rehearsal Marks", "Repeat Count", "Repeat Marker Jumps", "Repeat Marker Sections", "Staff Labels", "Staff Labels (Inner)",
    "Staff Labels (Percussion Grid)", "Tacet", "Tempo (Gradual)", "Tempo (Immediate)", "Tempo (Metronome Mark)", "Title" };
MenuSpec menu(const char* id, const char* title, std::vector<MenuSpec> children) { return { id, title, std::move(children) }; }
MenuSpec titled(const std::string& id, const std::string& title) { return { QString::fromStdString(id), QString::fromStdString(title), {} }; }
}

//! Edit > Filter (spec 45): the two modes, a submenu per group of filters, then the single-category filters
MenuSpec mu::dorico::filterMenu()
{
    std::vector<MenuSpec> children = { item("command://dorico/filter-select-only", "Select Only"), item("command://dorico/filter-deselect-only", "Deselect Only"), sep() };
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
            entries.push_back(titled("command://dorico/filter-notes-by-pitch", "Notes by Pitch..."));
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

// ponytail: ids are MuseScore's command URIs as AppMenuModel builds them (appshell/.../appmenumodel.cpp). An id that
// is neither in MuseScore's menus nor a registered command is skipped with a LOGW by DoricoMenuArranger; any
// MuseScore item not named here lands in Window > More MuseScore Commands. A title replaces the command's own.
static MenuSpec fileMenu()
{
    return menu("dorico-menu-file", "File", {
        item("command://project/new", "New"),
        stub("New From Project Template"),
        item("command://project/open", "Open..."),
        item("menu-file-open", "Open Recent"),
        sep(),
        item("command://project/close", "Close Project"),
        item("command://project/save", "Save"),
        item("command://project/save-as", "Save As..."),
        stub("Revert"),
        sep(),
        stub("Close All"),
        stub("Save All"),
        stub("Save As Project Template..."),
        sep(),
        stubMenu("Import", { "Flows...", "MIDI...", "MusicXML...", "Tempo Track..." }),
        menu("dorico-menu-export", "Export", {
            stub("Flows..."),
            item("command://dorico/export-audio", "Audio..."),
            item("command://dorico/export-midi", "MIDI..."),
            item("command://dorico/export-musicxml", "MusicXML..."),
            stub("Tempo Track..."),
            stub("Lyrics..."),
            stub("Text..."),
        }),
        sep(),
        item("command://project/print", "Print..."),
        sep(),
        item("command://project/properties", "Project Info..."),
        stub("Project Statistics..."),
        sep(),
        stub("Export User Settings..."),
        stub("Import User Settings..."),
        sep(),
        item("command://app/quit"),
    });
}

static MenuSpec editMenu()
{
    return menu("dorico-menu-edit", "Edit", {
        item("command://notation/undo"),
        item("command://notation/redo"),
        item("command://app/dock/toggle-undo-history", "History..."),
        sep(),
        item("command://notation/cut", "Cut"),
        item("command://notation/copy", "Copy"),
        item("command://notation/paste", "Paste"),
        menu("dorico-menu-paste-special", "Paste Special", {
            menu("dorico-menu-paste-into-voice", "Paste Into Voice", {
                item("command://dorico/paste-into-voice-1", "Up-stem Voice 1"),
                item("command://dorico/paste-into-voice-2", "Up-stem Voice 2"),
                item("command://dorico/paste-into-voice-3", "Up-stem Voice 3"),
                item("command://dorico/paste-into-voice-4", "Up-stem Voice 4"),
                sep(),
                item("command://dorico/paste-into-new-voice", "New Up-stem Voice"),
                stub("New Down-stem Voice"),
                sep(),
                stub("New Slashed Up-stem Voice"),
                stub("New Slashed Down-stem Voice"),
                stub("New Slashed Stemless Voice"),
            }),
            sep(),
            item("command://dorico/reduce", "Reduce"),
            item("command://dorico/explode", "Explode"),
            sep(),
            item("command://dorico/move-to-staff-above", "Move to Staff Above"),
            item("command://dorico/move-to-staff-below", "Move to Staff Below"),
            sep(),
            item("command://dorico/duplicate-to-staff-above", "Duplicate to Staff Above"),
            item("command://dorico/duplicate-to-staff-below", "Duplicate to Staff Below"),
            sep(),
            item("command://dorico/swap-staves", "Swap"),
            sep(),
            item("command://dorico/paste-articulations", "Paste Articulations"),
        }),
        item("command://notation/delete", "Delete"),
        item("command://dorico/delete-bars", "Delete Bars"),
        sep(),
        item("command://dorico/remove-rests", "Remove Rests"),
        sep(),
        stub("Requantize..."),
        sep(),
        item("command://notation/select-all", "Select All"),
        item("command://dorico/select-more", "Select More"),
        item("command://notation/add-to-selection-begin-system", "Select To Start Of System"),
        item("command://notation/add-to-selection-end-system", "Select To End Of System"),
        item("command://notation/add-to-selection-begin-score", "Select To Start Of Flow"),
        item("command://notation/add-to-selection-end-score", "Select To End Of Flow"),
        item("command://dorico/deselect-all", "Select None"),
        filterMenu(),
        sep(),
        menu("dorico-menu-go-to", "Go To", {
            item("command://dorico/go-to-bar", "Go To Bar..."),
            stub("Go To Page..."),
            stub("Go To Rehearsal Mark..."),
            sep(),
            stub("Go To Previous Flow"),
            stub("Go To Next Flow"),
        }),
        sep(),
        item("command://notation/toggle-concert-pitch", "Concert Pitch"),
        stub("Transposed Pitch"),
        sep(),
        stub("Condensing"),
        sep(),
        item("command://notation/flip", "Flip"),
        notationsMenu(),
        sep(),
        item("command://notation/reset-text-style-overrides", "Reset Appearance"),
        item("command://notation/reset-shapes-and-positions", "Reset Position"),
        stub("Propagate Properties"),
        sep(),
        stub("Device Setup..."),
        sep(),
        item("command://dorico/preferences", "Preferences..."),
    });
}

static MenuSpec viewMenu()
{
    return menu("dorico-menu-view", "View", {
        item("command://notation/view/mode-page", "Page View"),
        item("command://notation/view/mode-continuous", "Galley View"),
        stub("Fill View"),
        sep(),
        stubMenu("Pages", { "Spreads Horizontally", "Spreads Vertically", "Single Pages Horizontally", "Single Pages Vertically" }),
        sep(),
        stub("System Track"),
        stub("Instrument Filter Overlay"),
        stub("Cycle Locators"),
        sep(),
        stubMenu("Signposts", { "Hide Signposts", "-", "Accidentals", "Bracket and Barline Changes", "Chord Symbols", "Clefs",
                                "Condensing Changes", "Cues", "Cutaways", "Divisi", "Dynamics", "Figured Bass", "Frame Breaks",
                                "Harp Pedals", "Instrument Changes", "Key Signatures", "Note Spacing Changes", "Octave Lines",
                                "Pedal Lines", "Percussion Legends", "Player Labels", "Playing Techniques", "Repeat Markers",
                                "Rhythmic Feel", "Split Multi-bar Rest", "Staff Count Changes", "System Breaks", "Tempo", "Text",
                                "Time Signatures", "Trills", "Trill Accidentals", "Tuplets", "-", "Set All Signposts Shown",
                                "Set All Signposts Hidden" }),
        stubMenu("Note And Rest Colors", { "None", "Voice Colors", "Notes Out Of Range", "-", "Implicit Rests", "-", "Condensed Music",
                                           "Cues", "Divisi Unison Ranges", "Suppressed Playback", "Hidden Noteheads", "-",
                                           "Cautionary Accidentals", "Contradictory Accidentals", "Forced Accidentals" }),
        stubMenu("Bar Numbers", { "Page View", "Galley View", "Fill View" }),
        stub("Page Numbers"),
        stub("Attachment Lines"),
        sep(),
        stub("Comments"),
        stub("Cutaway Staff Lines"),
        stub("Token Placeholders"),
        sep(),
        stub("Highlight Bar Repeat Regions"),
        stub("Highlight Chord Symbol Regions"),
        stub("Highlight MIDI Trigger Regions"),
        stub("Highlight Cues"),
        stub("Highlight Slash Regions"),
        sep(),
        menu("dorico-menu-zoom", "Zoom", {
            item("command://notation/view/zoom-in", "Zoom In"),
            item("command://notation/view/zoom-out", "Zoom Out"),
            sep(),
            stub("25%"), stub("50%"), stub("75%"),
            item("command://notation/view/zoom-to-100", "100%"),
            stub("150%"), stub("200%"), stub("300%"), stub("400%"), stub("800%"), stub("1600%"),
            sep(),
            item("command://notation/view/zoom-to-whole-page", "Whole Page"),
            item("command://notation/view/zoom-to-two-pages", "Two Pages"),
            item("command://notation/view/zoom-to-page-width", "Page Width"),
            stub("Page Height"),
            sep(),
            stub("Custom Zoom..."),
        }),
        item("command://app/fullscreen", "Full Screen"),
        sep(),
        stub("Marquee Select Tool"),
        stub("Hand Tool"),
        sep(),
        stub("View Options..."),
    });
}

// The popover that creates each kind of notation (Write > Create ...); the rest have no popover yet
static std::vector<MenuSpec> createItems()
{
    return {
        item("command://dorico/popover/bars", "Create Bar or Barline"),
        item("command://dorico/popover/chordsymbols", "Create Chord Symbol"),
        item("command://dorico/popover/clef", "Create Clef"),
        stub("Create Cue"),
        item("command://dorico/popover/dynamics", "Create Dynamic"),
        item("command://dorico/popover/figuredbass", "Create Figured Bass"),
        item("command://dorico/popover/fingering", "Create Fingering"),
        item("command://dorico/popover/holds", "Create Hold or Pause"),
        item("command://dorico/popover/keysig", "Create Key Signature"),
        item("command://dorico/popover/lyrics", "Create Lyrics"),
        stub("Create Marker"),
        stub("Create MIDI Trigger Region"),
        item("command://dorico/popover/ornaments", "Create Ornament"),
        item("command://dorico/popover/techniques", "Create Playing Technique"),
        stub("Create Rehearsal Mark"),
        item("command://dorico/popover/repeats", "Create Repeat"),
        stub("Create Rhythmic Feel"),
        stubMenu("Create System Text", TEXT_STYLES),
        item("command://dorico/popover/tempo", "Create Tempo"),
        item("command://dorico/popover/timesig", "Create Time Signature"),
        item("command://dorico/popover/tuplets", "Create Tuplet"),
        stubMenu("Create Text", TEXT_STYLES),
        stub("Calculate Harp Pedals"),
    };
}

static MenuSpec writeMenu()
{
    std::vector<MenuSpec> items = {
        stubMenu("Instrument Visibility", { "Create Preset from Selection", "-", "Edit Instrument Filter Presets..." }),
        sep(),
        item("command://notation/toggle-note-input", "Note Input"),
        item("command://dorico/toggle-insert-mode", "Insert"),
        stubMenu("Insert Scope", { "Voice", "Player", "Global", "Global Adjustment Of Current Bar", "-", "Set Edit Stop Position" }),
        menu("dorico-menu-input-pitch", "Input Pitch", {
            item("command://playback/midi-input-written-pitch", "Written Pitch"),
            item("command://playback/midi-input-sounding-pitch", "Sounding Pitch"),
        }),
        sep(),
        menu("dorico-menu-rhythmic-grid", "Rhythmic Grid", {
            stub("Whole Note (Semibreve)"), stub("Dotted Half Note (Dotted Minim)"), stub("Half Note (Minim)"),
            stub("Dotted Quarter Note (Dotted Crotchet)"), stub("Quarter Note (Crotchet)"), stub("Dotted Eighth Note (Dotted Quaver)"),
            stub("Eighth Note (Quaver)"), stub("Dotted 16th Note (Semiquaver)"), stub("16th Note (Semiquaver)"),
            stub("32nd Note (Demisemiquaver)"),
            sep(),
            item("command://dorico/grid-coarser", "Decrease Rhythmic Grid Resolution"),
            item("command://dorico/grid-finer", "Increase Rhythmic Grid Resolution"),
        }),
        menu("dorico-menu-respell", "Respell", {
            item("command://dorico/respell-above", "Respell Using Note Name Above"),
            item("command://dorico/respell-below", "Respell Using Note Name Below"),
            sep(),
            stub("Propagate Note Spellings"),
            stub("Reset Note Spellings in Current Layout"),
            stub("Reset Note Spellings in All Layouts"),
            stub("Respell Notes Automatically..."),
        }),
        sep(),
        item("command://dorico/new-voice", "Create Voice"),
        stub("Create Slash Voice"),
        item("command://dorico/next-voice", "Next Voice"),
        sep(),
        item("command://notation/open-transpose", "Transpose..."),
        stub("Add Notes Above or Below..."),
        item("command://dorico/popover/notetools", "Add Intervals"),
        menu("dorico-menu-transform", "Transform", {
            menu("dorico-menu-transform-pitches", "Pitches", {
                item("command://dorico/transform/invert-pitches", "Invert Pitches..."),
                item("command://dorico/transform/reverse-pitches", "Reverse Pitches"),
                item("command://dorico/transform/reverse-invert-pitches", "Reverse and Invert Pitches..."),
                item("command://dorico/transform/rotate-pitches", "Rotate Pitches..."),
                stub("Map Pitches..."),
                stub("Map Scale..."),
                stub("Repeat Pitches..."),
            }),
            menu("dorico-menu-transform-rhythm", "Rhythm", {
                item("command://dorico/transform/reverse-rhythms", "Reverse Rhythms"),
                item("command://dorico/transform/rotate-rhythms", "Rotate Rhythms..."),
            }),
            menu("dorico-menu-transform-all", "All", {
                item("command://dorico/transform/reverse-all", "Reverse Pitches and Rhythms"),
                item("command://dorico/transform/reverse-invert-all", "Reverse and Invert Pitches and Reverse Rhythms..."),
                item("command://dorico/transform/rotate-all", "Rotate Pitches and Rhythms..."),
            }),
        }),
        sep(),
    };
    for (MenuSpec& create : createItems()) {
        items.push_back(std::move(create));
    }
    items.insert(items.end(), {
        sep(),
        stub("Create Slash Region"),
        stub("Create Bar Repeat Region"),
        stub("Create Numbered Bar Region"),
        stub("Create Chord Symbol Region"),
        sep(),
        stub("Create Comment"),
        stub("Reply to Comment"),
        sep(),
        menu("dorico-menu-edit-duration", "Edit Duration", {
            item("command://dorico/lengthen-by-grid", "Lengthen Duration by Grid Value"),
            item("command://dorico/shorten-by-grid", "Shorten Duration by Grid Value"),
            stub("Lengthen Duration"),
            stub("Shorten Duration"),
            sep(),
            item("command://dorico/double-duration", "Double Note Duration"),
            item("command://dorico/halve-duration", "Halve Note Duration"),
            sep(),
            stub("Split by Rhythmic Grid Resolution"),
            stubMenu("Split by Note Duration", { "32nd Note (Demisemiquaver)", "16th Note (Semiquaver)", "Eighth Note (Quaver)",
                                                 "Quarter Note (Crotchet)", "Half Note (Minim)", "Whole Note (Semibreve)" }),
            sep(),
            item("command://notation/extend-to-next-note", "Extend to Next Note"),
            stub("Extend to End of Selection"),
            stub("Shorten to Next Note"),
        }),
        sep(),
        stub("Split Flow"),
        stub("Trim Flow"),
    });
    return menu("dorico-menu-write", "Write", std::move(items));
}

// Setup and Engrave as captured from Dorico Pro 6 (reference/dorico-shots/menus/dorico-menubar-{setup,engrave}.txt)
static MenuSpec setupMenu()
{
    return menu("dorico-menu-setup", "Setup", {
        item("command://instruments/select", "New Single Player..."),
        stub("New Section Player..."),
        stub("Add Instrument to Player..."),
        stub("Add Ensemble..."),
        sep(),
        stub("Delete Player"),
        stub("Delete Empty Players..."),
        sep(),
        stub("Renumber Instruments"),
        sep(),
        stub("New Player Group"),
        stub("Delete Player Group"),
        sep(),
        stub("New Flow"),
        stub("Delete Flow"),
        sep(),
        stub("New Layout"),
        stub("Delete Layout"),
        stub("Propagate Part Formatting..."),
        stub("Duplicate Layout"),
        stub("Renumber Layouts"),
        stub("Clef and Transposition Overrides..."),
        sep(),
        stub("Create Default Part Layouts"),
        stub("Reset Instrument Names"),
    });
}

static MenuSpec engraveMenu()
{
    const std::vector<const char*> resets = { "Reset Layout", "Reset Selected Frames", "Reset Selected Systems" };
    return menu("dorico-menu-engrave", "Engrave", {
        stub("Rulers"),
        stub("Grid"),
        stub("Grid Options..."),
        stubMenu("Show Handles", { "Only for Selected Items", "Always", "-", "Arpeggio Signs", "Barlines", "Beams", "Brackets and Braces",
                                   "Figured Bass", "Gradual Tempo", "Guitar Bends", "Hairpins", "Horizontal Lines", "Lyrics",
                                   "Octave Lines", "Pedal Lines", "Repeat Endings", "Slurs", "Ties", "Trills", "Tuplets",
                                   "Unbeamed Stems", "Vertical Lines" }),
        stubMenu("Crosshairs", { "Show When Dragging", "Show When Selected" }),
        sep(),
        menu("dorico-menu-format-frames", "Format Music Frames", {
            item("command://notation/toggle-page-break", "Frame Break"),
            item("command://notation/make-into-page", "Make Into Frame"),
            item("command://notation/toggle-page-lock", "Lock Frame"),
            sep(),
            stub("Lock Layout"),
            item("command://notation/reset-to-default-layout", "Reset Layout"),
        }),
        menu("dorico-menu-format-systems", "Format Systems", {
            item("command://notation/toggle-system-break", "System Break"),
            item("command://notation/make-into-system", "Make Into System"),
            stub("Lock System"),
            sep(),
            item("command://notation/move-measure-to-next-system", "Move Bars to Next System"),
            item("command://notation/move-measure-to-prev-system", "Move Bars to Previous System"),
        }),
        stub("Note Spacing Change..."),
        stub("Condensing Change..."),
        stub("Split Multi-bar Rest"),
        stub("Reset Bracketing"),
        sep(),
        stubMenu("Staff Spacing", { "Reset Layout", "Reset Selected Frames", "Reset Selected Systems", "-", "Copy Staff Spacing..." }),
        stubMenu("Note Spacing", resets),
        stubMenu("Lyric Offsets", resets),
    });
}

static MenuSpec playMenu()
{
    return menu("dorico-menu-play", "Play", {
        stub("Record"),
        stub("Retrospective Record"),
        stub("Fixed Tempo Mode"),
        sep(),
        item("command://playback/play", "Play From Playhead Position"),
        item("command://playback/play-selection", "Play From Selection"),
        stub("Play From Start Of Flow"),
        stub("Play From Start Of Project"),
        stub("Play From Last Start Position"),
        sep(),
        item("command://playback/loop-toggle", "Cycle"),
        menu("dorico-menu-locators", "Locators", {
            stub("Set Cycle Locators From Selection"),
            item("command://playback/loop-in", "Set Left Cycle Locator"),
            item("command://playback/loop-out", "Set Right Cycle Locator"),
            sep(),
            stub("Clear Cycle Locators"),
        }),
        sep(),
        stub("Scrub Playback"),
        sep(),
        stub("Solo Selected Instruments"),
        stub("Deactivate All Solo States"),
        stub("Deactivate All Mute States"),
        sep(),
        stub("Increase Key Editor Grid Resolution"),
        stub("Decrease Key Editor Grid Resolution"),
        sep(),
        stub("Load Sounds For Unassigned Instruments"),
        item("command://playback/open-playback-setup", "Playback Template..."),
        stub("Stage Template..."),
        stub("Space Template..."),
        sep(),
        stubMenu("Automation", { "Delete All Automation For Selection", "Delete All Automation For Instruments In Selection",
                                 "Delete All Automation" }),
        sep(),
        stub("Reset Playback Overrides"),
        stub("Apply Default Output Level"),
        sep(),
        stubMenu("Tracks", { "Time Track", "Chord Track", "Marker Track" }),
    });
}

static MenuSpec libraryMenu()
{
    return menu("dorico-menu-library", "Library", {
        stub("Music Fonts..."), stub("Font Styles..."), stub("Paragraph Styles..."), stub("Character Styles..."),
        sep(),
        stub("Page Sizes..."),
        sep(),
        stub("Chord Diagrams..."), stub("Chord Symbols..."), stub("Chord Symbol Kerning Pairs..."), stub("Music Symbols..."),
        stub("Notehead Sets..."), stub("Playing Techniques..."),
        sep(),
        stub("Lines..."), stub("Line Bodies..."), stub("Line Annotations..."), stub("Repeatable Symbols..."),
        sep(),
        stub("Expression Maps..."), stub("Percussion Maps..."), stub("Playback Techniques..."),
        sep(),
        item("command://notation/open-edit-style", "Engraving Options..."),
        item("command://notation/open-page-settings", "Layout Options..."),
        item("command://dorico/notation-options", "Notation Options..."),
        stub("Note Input Options..."),
        stub("Playback Options..."),
        sep(),
        stub("Instruments..."), stub("Instrument Families..."), stub("Instrument Score Orders..."),
        sep(),
        stub("Export Library..."),
        stub("Library Manager..."),
    });
}

static MenuSpec windowMenu()
{
    return menu("dorico-menu-window", "Window", {
        stub("New Window"),
        item("command://app/dock/toggle-mixer", "Mixer"),
        item("command://app/dock/toggle-playback", "Transport"),
        item("command://dorico/toggle-video-window", "Video"),
        item("command://dorico/open-hub", "Hub"),
        sep(),
        item("command://dorico/mode-setup", "Setup"),
        item("command://dorico/mode-write", "Write"),
        item("command://dorico/mode-engrave", "Engrave"),
        item("command://dorico/mode-play", "Play"),
        item("command://dorico/mode-print", "Print"),
        sep(),
        stub("Previous Mode"),
        sep(),
        stub("Show Tabs"),
        sep(),
        stub("Close Tab"), stub("New Tab"), stub("Next Tab"), stub("Move Tab To New Window"), stub("Previous Tab"),
        stub("Close Other Tabs"), stub("Merge All Windows"),
        sep(),
        stub("No Split"), stub("Vertical Split"), stub("Horizontal Split"),
        sep(),
        item("command://dorico/toggle-toolbar", "Show Toolbar"),
        item("command://dorico/toggle-zone-left", "Show Left Zone"),
        item("command://dorico/toggle-zone-lower", "Show Lower Zone"),
        item("command://dorico/toggle-zone-right", "Show Right Zone"),
        item("command://dorico/toggle-all-zones", "Hide/Restore Zones"),
        sep(),
        stub("Next Layout"), stub("Previous Layout"), stub("Counterpart Layout"),
    });
}

static MenuSpec helpMenu()
{
    return menu("dorico-menu-help", "Help", {
        item("command://app/online-handbook", "Show Help"),
        sep(),
        stub("Dorico Help"),
        stub("Guided Tour"),
        stub("Key Commands"),
        sep(),
        stubMenu("Support", { "Steinberg Service and Support" }),
        stubMenu("Community", { "Dorico Blog", "Facebook", "YouTube", "Instagram", "Dorico Forum" }),
        sep(),
        stub("Create Diagnostic Report"),
        item("command://update/check", "Check For Updates"),
    });
}

static MenuSpec scriptMenu()
{
    return menu("dorico-menu-script", "Script", {
        stub("Run Script..."), stub("Run Last Script"),
        sep(),
        stub("Start Recording Macro"), stub("End Recording Macro"),
        sep(),
        stub("Console"),
        sep(),
        stub("Attach Debugger"),
    });
}

const std::vector<MenuSpec>& mu::dorico::doricoMenus()
{
    static const std::vector<MenuSpec> menus = {
        fileMenu(),
        editMenu(),
        viewMenu(),
        { MODE_MENU_ID, "Write", {} },   // the slot: see modeMenu()
        playMenu(),
        libraryMenu(),
        windowMenu(),
        helpMenu(),
        scriptMenu(),
    };
    return menus;
}

const MenuSpec* mu::dorico::modeMenu(Mode mode)
{
    static const MenuSpec setup = setupMenu();
    static const MenuSpec write = writeMenu();
    static const MenuSpec engrave = engraveMenu();
    switch (mode) {
    case Mode::Setup: return &setup;
    case Mode::Write: return &write;
    case Mode::Engrave: return &engrave;
    case Mode::Play:
    case Mode::Print: break;
    }
    return nullptr;
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
