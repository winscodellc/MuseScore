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

#include "prefstable.h"

#include <algorithm>

#include <QColor>

#include "settings.h"
#include "notation/inotationconfiguration.h"
#include "notation/types/zoomtype.h"
#include "project/iprojectconfiguration.h"
#include "playback/iplaybackconfiguration.h"
#include "importexport/musicxml/imusicxmlconfiguration.h"
#include "ui/iuiconfiguration.h"
#include "languages/ilanguagesconfiguration.h"
#include "languages/ilanguagesservice.h"
#include "languages/languagestypes.h"

using namespace mu::dorico;
using muse::Val;

// Helper names are unique to this file: the module builds as a unity batch.
namespace mu::dorico::prefstable {
using Get = std::function<Val()>;
using Set = std::function<void(const Val&)>;

static PrefRow make(PrefType type, const char* id, const char* label, Val fallback, const char* help = "")
{
    PrefRow r;
    r.type = type;
    r.id = id;
    r.label = label;
    r.fallback = std::move(fallback);
    r.help = help;
    return r;
}

static PrefRow check(const char* id, const char* label, bool on = false, const char* help = "")
{
    return make(PrefType::Check, id, label, Val(on), help);
}

static std::vector<PrefOption> optionList(std::initializer_list<const char*> texts)
{
    std::vector<PrefOption> list;
    for (const char* t : texts) {
        list.push_back({ t, Val(t) });
    }
    return list;
}

static PrefRow choice(PrefType type, const char* id, const char* label, std::initializer_list<const char*> texts, const char* help = "")
{
    PrefRow r = make(type, id, label, Val(*texts.begin()), help);
    r.options = optionList(texts);
    return r;
}

static PrefRow dropdown(const char* id, const char* label, std::initializer_list<const char*> texts, const char* help = "")
{
    return choice(PrefType::Dropdown, id, label, texts, help);
}

static PrefRow segmented(const char* id, const char* label, std::initializer_list<const char*> texts, const char* help = "")
{
    return choice(PrefType::Segmented, id, label, texts, help);
}

static PrefRow spinner(const char* id, const char* label, double value, double min, double max, const char* unit = "")
{
    PrefRow r = make(PrefType::Spinner, id, label, Val(value));
    r.min = min;
    r.max = max;
    r.unit = unit;
    return r;
}

static PrefRow hex(const char* id, const char* label, const char* color)
{
    return make(PrefType::Hex, id, label, Val(color));
}

static PrefRow text(const char* id, const char* label, const char* value = "")
{
    return make(PrefType::Text, id, label, Val(value));
}

//! The button's own caption is `caption` when the row has a separate label (Dorico's "Filenames ... [Edit...]"), else the label
static PrefRow button(const char* id, const char* label, const char* help = "", const char* caption = "")
{
    return make(PrefType::Button, id, label, Val(caption), help);
}

static PrefRow withExtra(PrefRow row, PrefRow extra)
{
    row.extra = std::make_shared<PrefRow>(std::move(extra));
    return row;
}

static PrefRow bound(PrefRow row, Get get, Set set)
{
    row.get = std::move(get);
    row.set = std::move(set);
    return row;
}

static PrefSection section(const char* title, std::vector<PrefRow> rows)
{
    PrefSection s;
    s.title = title;
    s.rows = std::move(rows);
    return s;
}

//! "MuseScore" band holding MuseScore's own pages
static PrefSection museScore(std::vector<PrefEmbedded> pages)
{
    PrefSection s;
    s.title = "MuseScore";
    s.embedded = std::move(pages);
    return s;
}

static PrefPage page(const char* id, const char* title, std::vector<PrefSection> sections)
{
    return { id, title, std::move(sections) };
}

//! ThemeInfo::title is empty for the standard themes
static std::string themeTitle(const muse::ui::ThemeInfo& theme)
{
    if (theme.codeKey == muse::ui::LIGHT_THEME_CODE) {
        return "Light";
    }
    if (theme.codeKey == muse::ui::DARK_THEME_CODE) {
        return "Dark";
    }
    if (theme.codeKey == muse::ui::HIGH_CONTRAST_WHITE_THEME_CODE) {
        return "High contrast (white)";
    }
    if (theme.codeKey == muse::ui::HIGH_CONTRAST_BLACK_THEME_CODE) {
        return "High contrast (black)";
    }
    return theme.title.empty() ? theme.codeKey : theme.title;
}

static std::string hexOf(const QColor& c)
{
    return c.name().toStdString();
}

static std::string zoomValue(mu::notation::ZoomType type, int percent)
{
    switch (type) {
    case mu::notation::ZoomType::PageWidth: return "pagewidth";
    case mu::notation::ZoomType::WholePage: return "wholepage";
    case mu::notation::ZoomType::TwoPages: return "twopages";
    case mu::notation::ZoomType::Percentage: break;
    }
    return "pct:" + std::to_string(percent);
}

// ---------------------------------------------------------------------------------------------------------------------
// Colors

static PrefCategory colors(const PrefBackends& b)
{
    PrefRow fullScore = hex("colors.page.fullScore", "Full score layouts", "#ffffff");
    PrefRow invert = check("colors.page.invert", "Invert colors for music (white on black)", false,
                           "Draws the music in the page's inverted colors.");
    if (b.notation) {
        auto n = b.notation;
        // The page colour itself: NotationConfiguration::foregroundColor() switches to the inverted colour while
        // Invert is on, which is Dorico's separate (unbacked) "Inverted page background" row.
        static const muse::Settings::Key PAGE_COLOR_KEY("notation", "ui/canvas/foreground/color");
        fullScore = bound(fullScore, []() { return Val(hexOf(muse::settings()->value(PAGE_COLOR_KEY).toQColor())); },
                          [n](const Val& v) {
            muse::settings()->setSharedValue(PAGE_COLOR_KEY, Val(QColor(QString::fromStdString(v.toString()))));
            n->setForegroundUseColor(true); // Dorico has no page wallpaper
        });
        invert = bound(invert, [n]() { return Val(n->scoreInversionEnabled()); },
                       [n](const Val& v) { n->setScoreInversionEnabled(v.toBool()); });
    }

    PrefPage pageColors = page("page", "Page Colors", {
        section("", {
            fullScore,
            hex("colors.page.partLayouts", "Part layouts", "#fef2da"),
            hex("colors.page.customLayouts", "Custom score layouts", "#ffffff"),
            invert,
            hex("colors.page.invertedBackground", "Inverted page background", "#000000"),
        }),
    });

    auto backgroundMode = [](const char* prefix) {
        const std::string p = std::string("colors.background.") + prefix + ".";
        std::vector<PrefRow> rows;
        PrefRow use = segmented("", "Use", { "Gradient", "Single Color" });
        use.id = p + "use";
        PrefRow preset = dropdown("", "Preset", { "(Custom)" });
        preset.id = p + "preset";
        rows.push_back(use);
        rows.push_back(preset);
        static const char* const DEFAULTS[] = { "#4c5258", "#3a4046", "#2b3035", "#1e2125" };
        for (int i = 0; i < 4; ++i) {
            PrefRow stop = hex("", "", DEFAULTS[i]);
            stop.id = p + "stop" + std::to_string(i + 1);
            stop.label = "Stop " + std::to_string(i + 1);
            rows.push_back(stop);
        }
        return rows;
    };
    PrefPage pageBackground = page("background", "Background Colors", {
        section("Write mode", backgroundMode("write")),
        section("Engrave mode", backgroundMode("engrave")),
    });

    static const char* const VOICES[] = { "#008ae6", "#df0771", "#00a83b", "#8f0ae8", "#d64000", "#2a56ab", "#c72cba", "#009e90" };
    std::vector<PrefRow> voiceRows;
    for (int i = 0; i < 8; ++i) {
        PrefRow r = hex("", "", VOICES[i]);
        r.id = "colors.voice.voice" + std::to_string(i + 1);
        r.label = "Voice " + std::to_string(i + 1);
        voiceRows.push_back(r);
    }
    voiceRows.back().help = "The colors that notes and rests take when voice colors are shown.";
    PrefPage pageVoices = page("voice", "Voice Colors", { section("", voiceRows) });

    static const std::pair<const char*, const char*> OTHERS[] = {
        { "selected", "Selected items" }, { "linkedSelected", "Linked selected items" }, { "shadowNotehead", "Shadow notehead" },
        { "cautionaryAccidental", "Cautionary accidental" }, { "contradictoryAccidental", "Contradictory accidental" },
        { "forcedAccidental", "Forced accidental" }, { "outOfStandardRange", "Out of standard range" },
        { "outOfAdvancedRange", "Out of advanced range" }, { "caret", "Caret and rhythmic grid" }, { "playhead", "Playhead" },
        { "playheadRecording", "Playhead when recording" }, { "activeCycle", "Active cycle locators" },
        { "inactiveCycle", "Inactive cycle locators" }, { "unplayableTab", "Unplayable in tablature" },
        { "minorGrid", "Minor grid lines" }, { "majorGrid", "Major grid lines" },
    };
    std::vector<PrefRow> otherRows;
    for (const auto& [name, label] : OTHERS) {
        PrefRow r = hex("", label, "#2da0f5");
        r.id = std::string("colors.other.") + name;
        otherRows.push_back(r);
    }
    PrefPage pageOther = page("other", "Other Colors", {
        section("", otherRows),
        section("Crosshair", {
            hex("colors.other.crosshairColor", "Color", "#ff0000"),
            spinner("colors.other.crosshairThickness", "Thickness", 2, 1, 10),
        }),
    });

    return { "colors", "Colors", { pageColors, pageBackground, pageVoices, pageOther } };
}

// ---------------------------------------------------------------------------------------------------------------------
// General

static PrefCategory general(const PrefBackends& b)
{
    PrefRow language = dropdown("general.general.language", "Language", { "System default" });
    if (b.languagesConfig && b.languages) {
        auto cfg = b.languagesConfig;
        std::vector<PrefOption> options { { "System default", Val(muse::languages::SYSTEM_LANGUAGE_CODE.toStdString()) } };
        QList<muse::languages::Language> languages = b.languages->languages().values();
        std::sort(languages.begin(), languages.end(), [](const auto& l, const auto& r) { return l.code < r.code; });
        for (const auto& l : languages) {
            options.push_back({ l.name.toStdString(), Val(l.code.toStdString()) });
        }
        language.options = options;
        language = bound(language, [cfg]() { return Val(cfg->currentLanguageCode().val.toStdString()); },
                         [cfg](const Val& v) { cfg->setCurrentLanguageCode(QString::fromStdString(v.toString())); });
    }

    PrefPage pageGeneral = page("general", "General", {
        section("", {
            language,
            dropdown("general.general.unit", "Preferred unit of measurement", { "Millimeters (mm)", "Inches (in)" }),
            dropdown("general.general.paperSize", "Default paper size type", { "Automatic", "A4", "Letter" }),
            dropdown("general.general.textFont", "Default text font family", { "Academico" }),
            segmented("general.general.middleC", "Middle C (note 60)", { "C3", "C4", "C5" }),
            segmented("general.general.pianoRoll", "Default piano roll display", { "Played Durations", "Notated Durations" }),
        }),
        section("Instrument Names", {
            withExtra(check("general.general.instrumentNameLanguage", "Default language for instrument names"),
                      dropdown("general.general.instrumentNameLanguageValue", "", { "English" })),
        }),
        section("Comments", {
            segmented("general.general.commentAuthor", "Author name for comments", { "User Name", "Custom Name" },
                      "Which name is shown on the comments you add."),
            text("general.general.commentName", "Full name"),
            text("general.general.commentInitials", "Initials"),
        }),
        section("Crash Reporting", {
            check("general.general.crashReports", "Show prompt to send crash reports to MuseScore", false,
                  "Crash reports help to find and fix problems."),
            button("general.general.privacyPolicy", "Privacy Policy..."),
        }),
        section("Suppressed Warnings", {
            button("general.general.resetWarnings", "Reset Warnings"),
        }),
        museScore({ { "GeneralPreferencesPage", "General" } }),
    });

    PrefRow theme = dropdown("general.window.theme", "Theme", { "Dark" });
    if (b.ui) {
        auto ui = b.ui;
        std::vector<PrefOption> options;
        for (const auto& t : ui->themes()) {
            options.push_back({ themeTitle(t), Val(t.codeKey) });
        }
        theme.options = options;
        theme = bound(theme, [ui]() { return Val(ui->currentTheme().codeKey); },
                      [ui](const Val& v) {
            if (ui->isFollowSystemTheme().val) {
                ui->setFollowSystemTheme(false);
            }
            ui->setCurrentTheme(v.toString());
        });
    }
    PrefPage pageWindow = page("window", "Window", {
        section("", {
            theme,
            check("general.window.maximized", "Open new windows maximized"),
            dropdown("general.window.lastWindowClosed", "When the last window is closed", { "Open Hub window", "Quit" }),
            check("general.window.closeSidePanels", "Close side panels when lower panel is opened"),
            check("general.window.graphicalEditing", "When switching to Engrave mode, activate the Graphical Editing tool"),
        }),
        museScore({ { "AppearancePreferencesPage", "Appearance" } }),
    });

    PrefRow zoom = dropdown("general.view.defaultZoom", "Default zoom", { "100%" });
    if (b.notation) {
        auto n = b.notation;
        std::vector<PrefOption> options;
        QList<int> percents = n->possibleZoomPercentageList();
        const int current = n->defaultZoom();
        if (!percents.contains(current)) {
            percents.append(current);
            std::sort(percents.begin(), percents.end());
        }
        for (int p : percents) {
            options.push_back({ std::to_string(p) + "%", Val("pct:" + std::to_string(p)) });
        }
        options.push_back({ "Page width", Val("pagewidth") });
        options.push_back({ "Whole page", Val("wholepage") });
        options.push_back({ "Two pages", Val("twopages") });
        zoom.options = options;
        zoom = bound(zoom, [n]() { return Val(zoomValue(n->defaultZoomType(), n->defaultZoom())); },
                     [n](const Val& v) {
            const std::string s = v.toString();
            if (s == "pagewidth") {
                n->setDefaultZoomType(mu::notation::ZoomType::PageWidth);
            } else if (s == "wholepage") {
                n->setDefaultZoomType(mu::notation::ZoomType::WholePage);
            } else if (s == "twopages") {
                n->setDefaultZoomType(mu::notation::ZoomType::TwoPages);
            } else if (s.rfind("pct:", 0) == 0) {
                n->setDefaultZoomType(mu::notation::ZoomType::Percentage);
                n->setDefaultZoom(std::stoi(s.substr(4)));
            }
        });
    }
    PrefPage pageView = page("view", "View", {
        section("", {
            zoom,
            dropdown("general.view.viewType", "Default view type", { "Page View", "Galley View" }),
            dropdown("general.view.overscroll", "Overscroll amount", { "Small", "Medium", "Large" }),
            check("general.view.tabBar", "Show tab bar in new projects"),
            check("general.view.bringIntoViewTab", "When switching tab or window, bring selection into view"),
            check("general.view.bringIntoViewPlay", "When switching to Play mode, bring selection into view"),
        }),
        section("Ruler", {
            check("general.view.rulerSpreads", "Continue ruler across spreads"),
        }),
        museScore({ { "CanvasPreferencesPage", "Canvas" } }),
    });

    PrefRow projectFolder = make(PrefType::Path, "general.files.projectFolder", "Default project folder", Val(""));
    PrefRow autoSave = withExtra(check("general.files.autoSave", "Auto-save every", true,
                                       "Saves a recovery copy of each open project at this interval."),
                                 spinner("general.files.autoSaveMinutes", "minutes", 2, 1, 999, "minutes"));
    if (b.project) {
        auto p = b.project;
        projectFolder = bound(projectFolder, [p]() { return Val(p->userProjectsPath()); },
                              [p](const Val& v) { p->setUserProjectsPath(v.toPath()); });
        autoSave = bound(autoSave, [p]() { return Val(p->isAutoSaveEnabled()); },
                         [p](const Val& v) { p->setAutoSaveEnabled(v.toBool()); });
        autoSave.extra = std::make_shared<PrefRow>(bound(*autoSave.extra, [p]() { return Val(p->autoSaveIntervalMinutes()); },
                                                         [p](const Val& v) { p->setAutoSaveInterval(v.toInt()); }));
    }
    PrefPage pageFiles = page("files", "Files", {
        section("Saving Projects", {
            projectFolder,
            check("general.files.completePath", "Show complete path for recent files"),
            autoSave,
            make(PrefType::Path, "general.files.backupFolder", "Project backup folder", Val("")),
            spinner("general.files.backups", "Number of backups per project", 5, 0, 99),
            check("general.files.warnOutside", "Warn when open project is updated outside MuseScore, moved, or deleted", true),
        }),
        section("Opening Projects", {
            check("general.files.warnFonts", "Warn when projects use fonts not installed on this computer", true),
            check("general.files.warnVersion", "Warn when opening projects last saved in an earlier or later version", true),
            check("general.files.promptSaveVersion", "Prompt to save projects from an earlier or later version to a new location", true),
        }),
        section("Exporting Files", {
            button("general.files.graphicsNames", "Filenames for exporting graphics", "", "Edit..."),
            check("general.files.omitExcluded", "Omit excluded players and layouts when exporting flows", true),
            check("general.files.openCsv", "Open text CSV in default application after export"),
            withExtra(check("general.files.warnSuspect", "Warn when printing full or custom score layouts with suspect transposition", true,
                            "Warns before printing a layout whose transposition looks wrong."),
                      segmented("general.files.suspectTransposition", "Suspect transposition for score layouts",
                                { "Concert Pitch", "Transposed Pitch" })),
            dropdown("general.files.clipboardFormat", "Format for text items copied to clipboard", { "HTML", "Plain text" },
                     "The format text is copied in when you copy text items."),
        }),
        museScore({ { "SaveAndPublishPreferencesPage", "Save & publish" }, { "FoldersPreferencesPage", "Folders" },
                    { "ScorePreferencesPage", "Score" }, { "VideoPreferencesPage", "Video" },
                    { "BraillePreferencesPage", "Braille" }, { "AdvancedPreferencesPage", "Advanced" } }),
    });

    return { "general", "General", { pageGeneral, pageWindow, pageView, pageFiles } };
}

// ---------------------------------------------------------------------------------------------------------------------
// Key Commands, MusicXML

static PrefCategory keyCommands()
{
    PrefPage p = page("keycommands", "Key Commands", {
        section("", {
            dropdown("keycommands.keyboardLanguage", "Keyboard language", { "Default" }),
            button("keycommands.printSummary", "Print Summary"),
        }),
        museScore({ { "ShortcutsPreferencesPage", "Key commands" } }),
        section("MIDI Learn", {}),
        section("Jump Bar Alias", {
            text("keycommands.alias", "Alias"),
            button("keycommands.addAlias", "Add Alias"),
            button("keycommands.removeAlias", "Remove Alias"),
        }),
    });
    p.sections[2].embedded = { { "MidiDeviceMappingPreferencesPage", "MIDI mappings" } };
    return { "keycommands", "Key Commands", { p } };
}

static PrefCategory musicXml(const PrefBackends& b)
{
    PrefRow breaks = check("musicxml.import.breaks", "Import system and frame breaks", true);
    if (b.musicXml) {
        auto x = b.musicXml;
        breaks = bound(breaks, [x]() { return Val(x->importBreaks()); }, [x](const Val& v) { x->setImportBreaks(v.toBool()); });
    }
    PrefPage import = page("import", "Import", {
        section("Notes and Chords", {
            check("musicxml.import.durations", "Note durations", true),
            check("musicxml.import.stems", "Stem directions", true),
            check("musicxml.import.ties", "Tie placement and appearance", true),
            check("musicxml.import.beams", "Beam groups", true),
            check("musicxml.import.accidentals", "Accidental visibility", true),
            check("musicxml.import.articulations", "Articulation placement", true),
            check("musicxml.import.tuplets", "Tuplet placement and bracket visibility", true),
            button("musicxml.import.advancedNotes", "Advanced Options"),
        }),
        section("Rests", {
            check("musicxml.import.restDurations", "Rest durations", true),
            check("musicxml.import.restPositions", "Rest positions", true),
            check("musicxml.import.restVisibility", "Rest visibility", true),
        }),
        section("Other Notations", {
            check("musicxml.import.dynamics", "Dynamics placement", true),
            check("musicxml.import.glissandi", "Glissando line style and text", true),
            check("musicxml.import.slurs", "Slur direction", true),
            check("musicxml.import.text", "Text items", true),
            check("musicxml.import.fontStyle", "Import font style information", true),
            check("musicxml.import.laissezVibrer", "Explicit laissez vibrer ties only", true),
            check("musicxml.import.figuredBass", "Figured bass appearance", true),
            check("musicxml.import.otherTechnical", "Import other-technical playing techniques", true),
            check("musicxml.import.arpeggios", "Ambiguous arpeggio lines span all staves", true),
            breaks,
            check("musicxml.import.octaveTranspositions", "Include instrument octave transpositions at concert pitch", true),
            button("musicxml.import.advancedOther", "Advanced Options"),
        }),
        museScore({ { "ImportPreferencesPage", "Import" } }),
    });
    PrefPage exp = page("export", "Export", {
        section("", {
            check("musicxml.export.suppressedBreaks", "Include suppressed system and frame breaks"),
            withExtra(check("musicxml.export.endOfBar", "Items at the end of the bar naturally relative to barline", true),
                      spinner("musicxml.export.gap", "Gap after barline", 0.5, 0, 10, "spaces")),
            check("musicxml.export.splitItems", "Split ongoing items at system and frame breaks", true),
            check("musicxml.export.sameVertical", "Use same vertical position for all systems for lines crossing breaks"),
        }),
    });
    return { "musicxml", "MusicXML", { import, exp } };
}

// ---------------------------------------------------------------------------------------------------------------------
// Note Input and Editing

static PrefCategory noteInput(const PrefBackends& b)
{
    PrefRow accidentalTiming = segmented("note.input.accidentalTiming", "Specify accidental, rhythm dot and articulations",
                                         { "After inputting note", "Before inputting note" },
                                         "Whether these are applied to the note you have just input or to the next one.");
    PrefRow playNotes = check("note.input.playNotes", "Play notes during note input and selection", true);
    PrefRow playChord = check("note.input.playChord", "Play all notes in chord when any is selected", true);
    if (b.notation) {
        auto n = b.notation;
        accidentalTiming = bound(accidentalTiming,
                                 [n]() { return Val(n->addAccidentalDotsArticulationsToNextNoteEntered() ? "Before inputting note"
                                                                                                         : "After inputting note"); },
                                 [n](const Val& v) { n->setAddAccidentalDotsArticulationsToNextNoteEntered(v.toString() == "Before inputting note"); });
    }
    if (b.playback) {
        auto p = b.playback;
        playNotes = bound(playNotes, [p]() { return Val(p->playNotesWhenEditing()); },
                          [p](const Val& v) { p->setPlayNotesWhenEditing(v.toBool()); });
        playChord = bound(playChord, [p]() { return Val(p->playChordWhenEditing()); },
                          [p](const Val& v) { p->setPlayChordWhenEditing(v.toBool()); });
    }

    PrefPage input = page("input", "Note Input", {
        section("Pitch and Duration", {
            segmented("note.input.pitchTiming", "By default, specify pitch", { "Before Duration", "After Duration" }),
            accidentalTiming,
        }),
        section("Lock Duration", {
            withExtra(check("note.input.lockDuration", "Jump over rests to next note only if closer than", true),
                      spinner("note.input.lockDurationQuarters", "quarters", 16, 1, 99, "quarters")),
        }),
        section("Force Duration", {
            check("note.input.forceDuration", "Preserve Force Duration state when stopping and restarting note input"),
        }),
        section("Auditioning", {
            playNotes,
            check("note.input.playShadow", "Play changes of shadow notehead pitch", true),
            playChord,
            withExtra(check("note.input.fixedVolume", "Use fixed volume to play selected notes"),
                      spinner("note.input.fixedVolumeValue", "", 100, 1, 127)),
        }),
        section("Mouse Input", {
            check("note.input.mouseDefault", "By default, enable note input using the mouse", true),
            check("note.input.mouseShadow", "Play shadow notehead pitch", true),
            check("note.input.mouseEdit", "Enable note pitch and rhythmic position editing using the mouse", true),
            check("note.input.middleButton", "Middle mouse button changes selection"),
        }),
        section("Breaks", {
            check("note.input.snapBreaks", "Snap system and frame breaks to barlines when creating", true,
                  "New breaks are placed at the nearest barline."),
            check("note.input.lockSystems", "Lock source and destination systems when moving bars", true,
                  "Moving bars does not reflow the systems around them."),
            check("note.input.breaksOnLock", "Create system breaks when using Lock Layout", true,
                  "Locking the layout adds a break at the end of each system."),
        }),
        section("Percussion Input", {
            segmented("note.input.percussionInput", "Input onto kit or grid", { "Use percussion map", "Use staff position" }),
            segmented("note.input.percussionClef", "Interpret as", { "Treble G clef", "Bass F clef" }),
            spinner("note.input.percussionTechniqueKey", "Input techniques from MIDI key", 60, 0, 127),
            spinner("note.input.percussionTechniqueKeys", "Number of keys for techniques", 12, 0, 127),
            segmented("note.input.singleLine", "Input onto single-line staves", { "Use percussion map", "Use staff position" }),
            spinner("note.input.relativePitch", "Interpret pitches relative to", 60, 0, 127),
        }),
        section("Copying Music", {
            check("note.input.copyMidiCc", "Include MIDI CC, pitch bend and dynamics from Key Editor when copying music", true),
        }),
        section("Properties", {
            segmented("note.input.localProperties", "When starting a new project, set local properties", { "Locally", "Globally" },
                      "Whether property changes apply to the selection or to the whole project."),
            segmented("note.input.commonProperties", "When selecting multiple items of different types",
                      { "Show all properties", "Show only common properties" }),
        }),
        museScore({ { "NoteInputPreferencesPage", "Note input" }, { "PercussionPreferencesPage", "Percussion" } }),
    });

    PrefPage editing = page("editing", "Editing", {
        section("", {
            segmented("note.editing.selectionPriority", "Selection priority when clicking", { "Prioritize noteheads", "Prioritize beams and stems" }),
            segmented("note.editing.clickDrag", "Default click-drag behavior", { "Hand Tool", "Marquee Select Tool" }),
            make(PrefType::Slider, "note.editing.dragThreshold", "Drag threshold", Val(4.0)),
            check("note.editing.doubleClickWrite", "Double-click in Setup or Engrave mode switches to Write mode", true),
            segmented("note.editing.chordSymbolDoubleClick", "Double-click on a chord symbol in Engrave mode opens",
                      { "Kerning Pairs Editor", "Appearance Editor" }),
            check("note.editing.followUndo", "Follow selection changes on undo and redo", true),
            withExtra(check("note.editing.multipleItems", "Allow multiple items to be created with the mouse"),
                      segmented("note.editing.creatingItems", "Creating items with the mouse", { "Create item at selection", "Load pointer with item" })),
            withExtra(check("note.editing.firstNote", "Create items at position of first selected note", true),
                      check("note.editing.firstNoteClefs", "Create clefs at position of first selected note")),
            check("note.editing.separateTrills", "Create separate trills for each selected note"),
            check("note.editing.linkPaste", "Link dynamics and slurs to existing items when pasting", true),
            check("note.editing.gradualDynamics", "End gradual dynamics at start of last note in multiple selections", true),
            check("note.editing.cutaways", "Cutaways changes on top staves apply to entire instrument by default", true),
            segmented("note.editing.graceDirection", "Direction of rhythmic movement when converting to grace notes",
                      { "Automatic", "Forwards", "Backwards" }),
            check("note.editing.projectInfo", "Open Project Info dialog when editing text frames containing tokens", true),
        }),
    });
    return { "note", "Note Input and Editing", { input, editing } };
}

// ---------------------------------------------------------------------------------------------------------------------
// Play, Proofreading, Remote, VST

static PrefCategory play(const PrefBackends& b)
{
    PrefRow follow = check("play.playhead.follow", "Follow playhead during playback", true);
    PrefRow midiInput = check("play.recording.midiInput", "Enable MIDI Input", true);
    if (b.notation) {
        auto n = b.notation;
        follow = bound(follow, [n]() { return Val(n->isAutomaticallyPanEnabled()); },
                       [n](const Val& v) { n->setIsAutomaticallyPanEnabled(v.toBool()); });
        midiInput = bound(midiInput, [n]() { return Val(n->isMidiInputEnabled()); },
                          [n](const Val& v) { n->setIsMidiInputEnabled(v.toBool()); });
    }

    // Read by the time display (DoricoTimeDisplay.qml); Dorico 6 opens a new project on bars and beats
    PrefRow transportDisplay = segmented("play.transport.display", "Default mini transport display",
                                         { "Elapsed Time", "Timecode", "Bar.Beat.Ticks" });
    if (b.notation) {   // with the app's settings, like the page colour
        static const muse::Settings::Key TRANSPORT_KEY("dorico", TRANSPORT_DISPLAY_SETTING);
        transportDisplay = bound(transportDisplay, []() {
            const Val v = muse::settings()->value(TRANSPORT_KEY);
            return v.isNull() || v.toString().empty() ? Val("Bar.Beat.Ticks") : v;
        }, [](const Val& v) { muse::settings()->setSharedValue(TRANSPORT_KEY, v); });
    }

    static const std::initializer_list<const char*> UNITS = { "1/4", "1/8", "1/16", "1/32", "1/64" };
    PrefPage p = page("play", "Play", {
        section("Playback Template", {
            withExtra(check("play.template.auto", "Use 'Auto' playback template", true), text("play.template.default", "Default playback template")),
            segmented("play.template.halion", "Default HALion plug-in", { "Use best available", "Use preferred" }),
            text("play.template.halionPreferred", "Preferred HALion plug-in"),
            check("play.template.updateMaps", "Update to expression maps and percussion maps with higher versions", true),
            dropdown("play.template.stage", "Default stage template", { "Default" }),
            dropdown("play.template.space", "Space template override", { "None" }),
        }),
        section("Project Activation", {
            segmented("play.activation.single", "Activate project when only one project", { "Ask", "Always", "Never" }),
            segmented("play.activation.subsequent", "Activate subsequent project on load", { "Ask", "Always", "Never" }),
        }),
        section("Playhead", {
            follow,
            check("play.playhead.show", "Show playhead when stopped", true),
            check("play.playhead.selectedStaves", "Play from selection only plays selected staves"),
            check("play.playhead.miniTransport", "Play button in mini transport plays from selection, if any", true),
        }),
        section("Mixer", {
            withExtra(make(PrefType::Slider, "play.mixer.level", "Default output level", Val(0.0)),
                      spinner("play.mixer.levelDb", "", 0, -60, 12, "dB")),
        }),
        section("MIDI Input Devices", {
            dropdown("play.midi.devices", "Devices", { "All devices" }),
        }),
        section("Quantization", {
            segmented("play.quantization.unit", "Quantization unit", UNITS, "The shortest note value that recorded notes snap to."),
            check("play.quantization.tuplets", "Detect tuplets", true),
            segmented("play.quantization.tupletUnit", "Quantization unit for tuplets", UNITS),
            check("play.quantization.grace", "Detect grace notes"),
            check("play.quantization.fillGaps", "Fill gaps", true),
        }),
        section("Recording", {
            midiInput,
            check("play.recording.suspend", "Suspend when application is in background", true),
            check("play.recording.thru", "Enable MIDI thru", true),
            check("play.recording.filterControllers", "Filter out MIDI controllers"),
            check("play.recording.velocities", "Preserve note velocities", true),
            check("play.recording.positions", "Preserve note positions"),
            withExtra(check("play.recording.cc64", "Import CC64 as pedal lines", true),
                      check("play.recording.snapPedal", "Snap pedal lines to previous beat", true)),
            check("play.recording.slurs", "Detect slurs"),
            check("play.recording.tremolos", "Detect tremolos"),
            check("play.recording.trills", "Detect trills"),
            check("play.recording.articulations", "Add articulations to notes"),
            spinner("play.recording.latency", "MIDI input latency compensation", 0, 0, 500, "ms"),
        }),
        section("Transport Display", {
            transportDisplay,
        }),
        section("Controllers", {
            check("play.controllers.reset", "Reset controllers and send 'all notes off' when stopping playback", true),
        }),
        section("Audio Device", {
            button("play.audio.setup", "Audio Device Setup..."),
        }),
        museScore({ { "AudioMidiPreferencesPage", "Audio & MIDI" } }),
    });
    return { "play", "Play", { p } };
}

static PrefCategory proofreading()
{
    static const char* const NAMES[] = {
        "Clefs", "Octave clefs", "Key signatures", "Hidden time signatures", "Repeated time signatures", "Incorrect bar lengths",
        "Pick-up bars", "Repeat structures", "Dynamics", "Suggested dynamics", "Playing techniques", "Unplayable playing techniques",
        "Suggested playing techniques", "Instrument changes", "Stretch and double-stopping", "String harmonics", "Snap pizzicato",
        "String fingering", "Instrument ranges", "Empty percussion legends",
    };
    std::vector<PrefRow> categories;
    for (size_t i = 0; i < std::size(NAMES); ++i) {
        PrefRow r = check("", NAMES[i], true);
        r.id = "proofreading.categories.c" + std::to_string(i + 1);
        categories.push_back(r);
    }
    PrefPage main = page("proofreading", "Proofreading", {
        section("General", { check("proofreading.general.enable", "Enable proofreading", true) }),
        section("Stringed Instruments", {
            spinner("proofreading.strings.stretch", "Maximum stretch between tip of index and pinky fingers", 162, 50, 300, "millimeters"),
        }),
    });
    PrefPage cats = page("categories", "Categories", { section("Categories", categories) });
    return { "proofreading", "Proofreading", { main, cats } };
}

static PrefCategory remote()
{
    PrefPage p = page("remote", "Remote", {
        section("", {
            check("remote.enable", "Enable remote control"),
            spinner("remote.port", "Port", 4560, 1, 65535),
            button("remote.removeClients", "Remove selected clients"),
            button("remote.clearClients", "Clear all clients"),
        }),
    });
    return { "remote", "Remote", { p } };
}

static PrefCategory vst()
{
    PrefPage p = page("vst", "VST Plug-ins", {
        section("", { check("vst.openWindows", "Open VST plug-in windows when opening projects") }),
        section("Allowed VST2 Plug-ins", {
            button("vst.blockAll", "Block All"),
            button("vst.allowAll", "Allow All"),
        }),
        section("Audio Engine Data", {
            button("vst.clearCache", "Clear Audio Engine Cache", "Removes the cached plug-in scan results."),
            button("vst.resetData", "Reset Audio Engine Data", "Restart MuseScore for these changes to take effect."),
        }),
    });
    return { "vst", "VST Plug-ins", { p } };
}
}

PrefTable mu::dorico::buildPrefTable(const PrefBackends& b)
{
    using namespace mu::dorico::prefstable;
    return { colors(b), general(b), keyCommands(), musicXml(b), noteInput(b), play(b), proofreading(), remote(), vst() };
}

static void collectRowIds(const PrefRow& row, std::vector<std::string>& ids)
{
    ids.push_back(row.id);
    if (row.extra) {
        collectRowIds(*row.extra, ids);
    }
}

std::vector<std::string> mu::dorico::prefRowIds(const PrefTable& table)
{
    std::vector<std::string> ids;
    for (const PrefCategory& c : table) {
        for (const PrefPage& p : c.pages) {
            for (const PrefSection& s : p.sections) {
                for (const PrefRow& r : s.rows) {
                    collectRowIds(r, ids);
                }
            }
        }
    }
    return ids;
}

const PrefRow* mu::dorico::findPrefRow(const PrefTable& table, const std::string& id)
{
    for (const PrefCategory& c : table) {
        for (const PrefPage& p : c.pages) {
            for (const PrefSection& s : p.sections) {
                for (const PrefRow& r : s.rows) {
                    if (r.id == id) {
                        return &r;
                    }
                    if (r.extra && r.extra->id == id) {
                        return r.extra.get();
                    }
                }
            }
        }
    }
    return nullptr;
}
