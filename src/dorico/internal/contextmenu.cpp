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

#include "contextmenu.h"

using namespace mu::dorico;

namespace {
MenuSpec ctxSep() { return { "-", {}, {} }; }
MenuSpec ctxCmd(const char* id, const char* title) { return { id, title, {} }; }
MenuSpec ctxStub(const char* slug, const char* title) { return { QString("dorico-ctx-") + slug, title, {} }; }
MenuSpec ctxSub(const char* slug, const char* title, std::vector<MenuSpec> children)
{
    return { QString("dorico-ctx-") + slug, title, std::move(children) };
}

MenuSpec ctxPasteSpecial()
{
    return ctxSub("paste-special", "Paste Special", {
        ctxSub("paste-into-voice", "Paste Into Voice", {
            ctxCmd("command://dorico/paste-into-voice-1", "Voice 1"),
            ctxCmd("command://dorico/paste-into-voice-2", "Voice 2"),
            ctxCmd("command://dorico/paste-into-voice-3", "Voice 3"),
            ctxCmd("command://dorico/paste-into-voice-4", "Voice 4"),
            ctxCmd("command://dorico/paste-into-new-voice", "New Voice"),
        }),
        ctxSep(),
        ctxCmd("command://dorico/move-to-staff-above", "Move to Staff Above"),
        ctxCmd("command://dorico/move-to-staff-below", "Move to Staff Below"),
        ctxSep(),
        ctxCmd("command://dorico/duplicate-to-staff-above", "Duplicate to Staff Above"),
        ctxCmd("command://dorico/duplicate-to-staff-below", "Duplicate to Staff Below"),
        ctxSep(),
        ctxCmd("command://dorico/swap-staves", "Swap"),
    });
}

// Engrave's Filter has only the two modes
MenuSpec ctxEngraveFilter()
{
    return ctxSub("filter", "Filter", { ctxCmd("command://dorico/filter-select-only", "Select Only"),
                                     ctxCmd("command://dorico/filter-deselect-only", "Deselect Only") });
}

MenuSpec ctxBeaming()
{
    return ctxSub("beaming", "Beaming", {
        ctxCmd("command://notation/add-beam-mid", "Beam Together"),
        ctxCmd("command://notation/add-beam-begin", "Split Beam"),
        ctxCmd("command://notation/add-beam-none", "Make Unbeamed"),
        ctxCmd("command://notation/add-beam-begin32", "Split Secondary Beam"),
        ctxStub("partial-beam", "Set Partial Beam Direction"),
        ctxStub("fanned-beam", "Create Fanned Beam"),
        ctxStub("stemlets", "Stemlets"),
        ctxCmd("command://notation/reset-beams", "Reset Beaming"),
    });
}

MenuSpec ctxChordSymbols()
{
    return ctxSub("chord-symbols", "Chord Symbols and Diagrams", {
        ctxStub("chord-line", "Line"),
        ctxSep(),
        ctxStub("generate-chord-symbols", "Generate Chord Symbols from Selection..."),
        ctxStub("generate-notes", "Generate Notes from Chord Symbols in Selection..."),
    });
}

MenuSpec ctxNotehead()
{
    return ctxSub("notehead", "Notehead", {
        ctxCmd("command://dorico/notehead-circled", "Circled Noteheads"),
        ctxCmd("command://dorico/notehead-default", "Default Noteheads"),
        ctxCmd("command://dorico/notehead-circled-large", "Large Circled Noteheads"),
        ctxStub("larger-noteheads", "Larger Noteheads"),
        ctxCmd("command://dorico/notehead-slashed1", "Slashed Noteheads (Bottom Left to Top Right)"),
        ctxCmd("command://dorico/notehead-slashed2", "Slashed Noteheads (Top Left to Bottom Right)"),
        ctxSep(),
        ctxCmd("command://dorico/notehead-cross", "Crosses"),
        ctxCmd("command://dorico/notehead-triangle", "Triangular"),
        ctxCmd("command://dorico/notehead-diamond", "Diamonds"),
        ctxCmd("command://dorico/notehead-slash", "Slashes"),
        ctxStub("round-square", "Round and Square"),
        ctxSep(),
        ctxStub("scale-degree", "Scale Degree"),
        ctxStub("pitched", "Pitched"),
        ctxSep(),
        ctxCmd("command://dorico/notehead-default", "Reset to Default Notehead"),
        ctxSep(),
        ctxCmd("command://notation/add-parentheses", "Toggle Round Brackets"),
        ctxCmd("command://notation/add-brackets", "Toggle Square Brackets"),
    });
}

MenuSpec ctxVoices()
{
    return ctxSub("voices", "Voices", {
        ctxStub("stems-up", "Default Stems Up"),
        ctxStub("stems-down", "Default Stems Down"),
        ctxSep(),
        ctxStub("rhythmic-slashes", "Rhythmic Slashes"),
        ctxSep(),
        ctxStub("swap-voice-contents", "Swap Voice Contents"),
        ctxSub("change-voice", "Change Voice", {
            ctxCmd("command://notation/use-voice-1", "Voice 1"),
            ctxCmd("command://notation/use-voice-2", "Voice 2"),
            ctxCmd("command://notation/use-voice-3", "Voice 3"),
            ctxCmd("command://notation/use-voice-4", "Voice 4"),
        }),
        ctxSep(),
        ctxStub("swap-voice-order", "Swap Voice Order"),
    });
}

MenuSpec ctxStem()
{
    return ctxSub("stem", "Stem", {
        ctxCmd("command://dorico/stem-up", "Force Stem Up"),
        ctxCmd("command://dorico/stem-down", "Force Stem Down"),
        ctxCmd("command://dorico/stem-auto", "Remove Forced Stem"),
        ctxSep(),
        ctxStub("force-centered-beam", "Force Centered Beam"),
        ctxStub("custom-centered-beam", "Custom Centered Beam..."),
        ctxStub("remove-centered-beam", "Remove Centered Beam"),
    });
}

MenuSpec ctxStaffWrite()
{
    return ctxSub("staff", "Staff", {
        ctxStub("add-staff-above", "Add Staff Above"),
        ctxStub("add-staff-below", "Add Staff Below"),
        ctxSep(),
        ctxStub("ossia-above", "Create Ossia Above"),
        ctxStub("ossia-below", "Create Ossia Below"),
        ctxSep(),
        ctxStub("change-divisi", "Change Divisi..."),
        ctxStub("restore-unison", "Restore Unison"),
        ctxSep(),
        ctxStub("cutaway-manual", "Create Manual Cutaway"),
        ctxStub("cutaway-start", "Create Start Cutaway"),
        ctxStub("cutaway-end", "Create End Cutaway"),
        ctxSep(),
        ctxStub("remove-staff", "Remove Staff"),
        ctxSep(),
        ctxStub("staff-visibility", "Manual Staff Visibility..."),
    });
}

MenuSpec ctxStaffEngrave()
{
    return ctxSub("staff", "Staff", {
        ctxStub("cutaway-manual", "Create Manual Cutaway"),
        ctxStub("cutaway-start", "Create Start Cutaway"),
        ctxStub("cutaway-end", "Create End Cutaway"),
        ctxSep(),
        ctxStub("coordination-line", "Create Coordination Line"),
        ctxSep(),
        ctxStub("staff-visibility", "Manual Staff Visibility..."),
    });
}

MenuSpec ctxStaffSize()
{
    return ctxSub("staff-size", "Staff Size", { ctxStub("staff-size-60", "60%"), ctxStub("staff-size-75", "75%"), ctxStub("staff-size-100", "100%"),
                                             ctxSep(), ctxStub("staff-size-custom", "Custom Staff Size...") });
}

MenuSpec ctxDynamics(Mode mode)
{
    if (mode == Mode::Engrave) {
        return ctxSub("dynamics", "Dynamics", { ctxStub("group-dynamics", "Group Dynamics"), ctxStub("ungroup-dynamics", "Ungroup Dynamics"),
                                             ctxStub("remove-from-group", "Remove from Group"), ctxSep(), ctxStub("link-dynamics", "Link"),
                                             ctxStub("unlink-dynamics", "Unlink"), ctxSep(), ctxStub("align-dynamics", "Align Dynamics") });
    }
    return ctxSub("dynamics", "Dynamics", { ctxStub("group-dynamics", "Group Dynamics"), ctxStub("ungroup-dynamics", "Ungroup Dynamics"),
                                         ctxStub("remove-from-group", "Remove from Group"), ctxSep(), ctxStub("link-dynamics", "Link"),
                                         ctxStub("unlink-dynamics", "Unlink"), ctxSep(), ctxStub("increase-intensity", "Increase Dynamic Intensity"),
                                         ctxStub("decrease-intensity", "Decrease Dynamic Intensity") });
}
}

//! Edit > Notations (spec 102): Dorico's alphabetical list, the context menu's submenus plus the ones it only shows on a
//! selection of that kind.
MenuSpec mu::dorico::notationsMenu()
{
    return ctxSub("notations", "Notations", {
        ctxSub("bar-numbers", "Bar Numbers", { ctxStub("bar-number-change", "Add Bar Number Change...") }),
        ctxBeaming(),
        ctxSub("chord-symbols", "Chord Symbols and Diagrams", {
            ctxStub("chord-line", "Line"), ctxStub("cycle-diagram", "Cycle Chord Diagram"),
            ctxStub("diagram-variants", "Show All Variants of Chord Diagram..."), ctxSep(),
            ctxStub("copy-shape", "Copy Shape to Matching Chord Symbols"), ctxSep(),
            ctxStub("reset-finger-numbering", "Reset Chord Diagram Finger Numbering"), ctxSep(),
            ctxStub("clear-played-pitches", "Clear Chord Symbol Played Pitches"), ctxSep(),
            ctxStub("kerning-pairs", "Chord Symbol Kerning Pairs..."), ctxStub("chord-appearance", "Chord Symbol Appearance..."), ctxSep(),
            ctxStub("generate-chord-symbols", "Generate Chord Symbols from Selection..."),
            ctxStub("generate-notes", "Generate Notes from Chord Symbols in Selection..."),
            ctxStub("paste-chords-as-notes", "Paste Chord Symbols as Notes") }),
        ctxSub("clef", "Clef", { ctxStub("clef-concert", "Concert Pitch"), ctxStub("clef-transposed", "Transposed Pitch"), ctxSep(),
                              ctxStub("clef-reset-position", "Reset Clef Position"), ctxStub("clef-after-grace", "After Grace Notes") }),
        ctxSub("cross-staff", "Cross Staff", { ctxStub("cross-above", "Cross to Staff Above"), ctxStub("cross-below", "Cross to Staff Below"),
                                            ctxStub("cross-reset", "Reset to Original Staff") }),
        ctxDynamics(Mode::Write),
        ctxSub("figured-bass", "Figured Bass", { ctxStub("reset-figured-bass", "Reset Figured Bass"),
                                              ctxStub("force-figured-bass", "Force Current Appearance") }),
        ctxSub("fingering", "Fingering", { ctxStub("reset-fingering", "Reset Fingering") }),
        ctxSub("lyrics", "Lyrics", { ctxStub("lyrics-line", "Line"), ctxStub("lyrics-translations", "Translations"),
                                  ctxStub("lyrics-placement", "Placement"), ctxSep(), ctxStub("edit-lyrics-line", "Edit Line of Lyrics..."),
                                  ctxStub("edit-lyric", "Edit Single Lyric...") }),
        ctxNotehead(),
        ctxSub("octave-line", "Octave Line", { ctxStub("octave-corner", "Add Corner"), ctxStub("octave-angled", "Make Angled"),
                                            ctxStub("octave-remove", "Remove Corner/Angle") }),
        ctxSub("pedal-lines", "Pedal Lines", { ctxStub("pedal-retake", "Add Retake"), ctxStub("pedal-quarter", "Add Change to One Quarter Depressed"),
                                            ctxStub("pedal-half", "Add Change to Half Depressed"),
                                            ctxStub("pedal-three-quarters", "Add Change to Three Quarters Depressed"),
                                            ctxStub("pedal-full", "Add Change to Fully Depressed"), ctxStub("pedal-remove-retake", "Remove Retake"),
                                            ctxSep(), ctxStub("pedal-split", "Split Pedal Line"), ctxStub("pedal-merge", "Merge Pedal Lines") }),
        ctxSub("percussion", "Percussion", { ctxStub("legend-all", "Legend for All Instruments"),
                                          ctxStub("legend-selected", "Legend for Selected Instruments"),
                                          ctxStub("legend-sounding", "Legend for Sounding Instruments"), ctxSep(),
                                          ctxStub("percussion-change-voice", "Change Voice") }),
        ctxSub("playing-techniques", "Playing Techniques", { ctxStub("group-techniques", "Group Playing Techniques"),
                                                          ctxStub("ungroup-techniques", "Ungroup Playing Techniques"),
                                                          ctxStub("remove-technique-from-group", "Remove Playing Technique from Group") }),
        ctxSub("slurs", "Slurs", { ctxStub("slur-link", "Link"), ctxStub("slur-unlink", "Unlink") }),
        ctxStaffSize(),
        ctxStaffWrite(),
        ctxStem(),
        ctxSub("text", "Text", { ctxStub("text-paragraph-style", "Change Paragraph Style..."),
                              ctxStub("text-to-system-text", "Change Text to System Text") }),
        ctxVoices(),
    });
}

std::vector<MenuSpec> mu::dorico::scoreContextMenu(const ContextMenuInput& in)
{
    if (in.mode != Mode::Write && in.mode != Mode::Engrave) {
        return {};
    }
    const bool write = in.mode == Mode::Write;

    std::vector<MenuSpec> out;
    if (write && in.hasSelection) {
        out.push_back(ctxCmd("command://notation/cut", "Cut"));
        out.push_back(ctxCmd("command://notation/copy", "Copy"));
        if (in.clipboardHasMusic) {
            out.push_back(ctxCmd("command://notation/paste", "Paste"));
        }
        out.push_back(ctxPasteSpecial());
        out.push_back(ctxSep());
    }
    out.push_back(write ? filterMenu() : ctxEngraveFilter());
    out.push_back(ctxSep());

    if (in.hasSelection && (in.text || in.dynamics)) {
        out.push_back(ctxStub("edit-font", "Edit Font..."));
        out.push_back(ctxSep());
    }

    // Dorico's alphabetical order. Notes and ranges get the long list, Engrave only the layout rows.
    const bool formatting = write && in.notes;
    out.push_back(ctxSub("bar-numbers", "Bar Numbers", { ctxStub("bar-number-change", "Add Bar Number Change...") }));
    if (formatting) {
        out.push_back(ctxBeaming());
        out.push_back(ctxChordSymbols());
    }
    if (in.hasSelection && in.dynamics) {
        out.push_back(ctxDynamics(in.mode));
    }
    if (formatting) {
        out.push_back(ctxSub("fingering", "Fingering", { ctxStub("reset-fingering", "Reset Fingering") }));
        out.push_back(ctxNotehead());
        out.push_back(ctxSub("octave-line", "Octave Line", { ctxStub("octave-corner", "Add Corner"), ctxStub("octave-angled", "Make Angled"),
                                                          ctxStub("octave-remove", "Remove Corner/Angle") }));
        out.push_back(ctxSub("pedal-lines", "Pedal Lines", { ctxStub("pedal-retake", "Add Retake"),
                                                          ctxStub("pedal-quarter", "Add Change to One Quarter Depressed"),
                                                          ctxStub("pedal-half", "Add Change to Half Depressed"),
                                                          ctxStub("pedal-three-quarters", "Add Change to Three Quarters Depressed"),
                                                          ctxStub("pedal-full", "Add Change to Fully Depressed"),
                                                          ctxStub("pedal-remove-retake", "Remove Retake"), ctxSep(),
                                                          ctxStub("pedal-split", "Split Pedal Line"), ctxStub("pedal-merge", "Merge Pedal Lines") }));
    }
    if (write && in.hasSelection && in.slurs) {
        out.push_back(ctxSub("slurs", "Slurs", { ctxStub("slur-link", "Link"), ctxStub("slur-unlink", "Unlink") }));
    }
    out.push_back(ctxStaffSize());
    if (in.hasSelection) {
        out.push_back(write ? ctxStaffWrite() : ctxStaffEngrave());
    }
    if (formatting) {
        out.push_back(ctxStem());
        out.push_back(ctxVoices());
    }
    return out;
}

// ponytail: the Setup menus are labels with no behaviour behind most rows; the lists wire the few that exist.
std::vector<MenuSpec> mu::dorico::playerContextMenu()
{
    return {
        ctxStub("soloist", "Soloist"), ctxSep(),
        ctxStub("rename-player", "Rename Player..."),
        ctxStub("add-instrument", "Add Instrument to Player..."),
        ctxStub("add-to-group", "Add Player to Group"), ctxSep(),
        ctxStub("player-flows", "Flows"), ctxStub("player-layouts", "Layouts"), ctxSep(),
        ctxStub("duplicate-player", "Duplicate Player"),
        ctxCmd("dorico-setup-delete", "Delete Player"), ctxSep(),
        // Not in Dorico: MuseScore's hidden instruments and shared parts have no other switch in the Setup shell
        ctxCmd("dorico-setup-hide", "Hide Instrument"), ctxCmd("dorico-setup-show", "Show Instrument"),
        ctxCmd("dorico-setup-disable-part", "Disable Part"), ctxCmd("dorico-setup-enable-part", "Enable Part"), ctxSep(),
        ctxStub("player-chord-symbols", "Chord Symbols"), ctxStub("player-chord-diagrams", "Chord Diagrams"), ctxSep(),
        ctxStub("combine-kit", "Combine Instruments Into Kit"), ctxStub("empty-kit", "Create Empty Kit"), ctxStub("import-kit", "Import Kit..."),
    };
}

std::vector<MenuSpec> mu::dorico::flowContextMenu()
{
    return {
        ctxStub("rename-flow", "Rename"),
        ctxCmd("command://notation/open-edit-style", "Notation Options..."),
        ctxCmd("command://project/properties", "Project Info..."), ctxSep(),
        ctxStub("flow-layouts", "Layouts"), ctxStub("flow-players", "Players"), ctxSep(),
        ctxSub("flow-video", "Video", { ctxStub("video-attach", "Attach..."), ctxStub("video-detach", "Detach"),
                                        ctxCmd("command://dorico/video-properties", "Properties...") }), ctxSep(),
        ctxStub("duplicate-flow", "Duplicate Flow"), ctxStub("delete-flow", "Delete Flow"),
    };
}

std::vector<MenuSpec> mu::dorico::layoutContextMenu()
{
    return {
        ctxCmd("dorico-setup-rename", "Rename"),
        ctxStub("renumber-layouts", "Renumber Layouts"),
        ctxCmd("command://notation/open-page-settings", "Layout Options..."),
        ctxStub("propagate-part-formatting", "Propagate Part Formatting..."),
        ctxStub("clef-transposition-overrides", "Clef and Transposition Overrides..."),
        ctxStub("flow-heading-overrides", "Flow Heading Overrides..."), ctxSep(),
        ctxStub("layout-flows", "Flows"), ctxStub("layout-players", "Players"), ctxSep(),
        ctxCmd("dorico-setup-delete", "Delete Layout"),
        ctxCmd("dorico-setup-duplicate", "Duplicate Layout"),
    };
}
