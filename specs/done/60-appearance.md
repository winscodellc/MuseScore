<!-- markdownlint-disable -->

# 60 - Appearance: colors, score canvas, icons

Status: done - Depends on: 50 - Dorico refs: `[M]user_interface/...` "Changing the window color theme" (API topic `iKF9BFI04pB8PG0opfbKpA`), "Project window in Write mode" (`gON2B7CLULjilph3Un5LwQ`), "Music area" (`bzfupzzVtDRcB8Dj1S612g`), "Changing the background color" (`H43HNanVmUSH0q2_mzu~6g`), "Changing music area colors" (`K2no0q0PuuZIF7iqOhsSIw`), "Changing the page color" (`hAV1f0AOkEajNr8cJlgtbw`)

## Goal

Make the fork look like Dorico Pro 6: Dorico's Dark and Light window colors, its music-area colors, and Dorico-style toolbox icons. No Dorico artwork is used: colors are measured values (facts). Icons are SMuFL glyphs from Bravura (OFL, shipped in `fonts/bravura`) or Lucide icons (ISC).

## Dorico behaviour

| Item                       | Behaviour (manual)                                                                                                                                   |
| -------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------- |
| Theme                      | Preferences > General > Window > Theme: **Dark** (default), Light, Auto                                                                              |
| Music-area background      | A gradient of up to four colors, or a single color. Write mode and Engrave mode can each have their own (Setup uses Write's). Default: blue gradient |
| Page color                 | Per layout type. Default: full score and custom score layouts white, part layouts cream. Display only, not exported                                  |
| Voice and selection colors | Preferences > Colors: the first eight voice colors, plus "Other colors" (selection, ...). Selected items are orange                                  |

### Measured palette

Sources: the screenshots embedded in the manual pages above, read through `https://www.steinberg.help/api/khub/maps/BYbh2hvBDwUCjSijIhHcqA/topics/<id>/content` (`write_mode_project_window_pro.png` 850x500, `theme_dark_pro.png` / `theme_light_pro.png` 370x221, `music_area.png`). Values are the dominant pixel color of each region.

| Role                                           | Dark                                                    | Light                  | MuseScore theme key                               |
| ---------------------------------------------- | ------------------------------------------------------- | ---------------------- | ------------------------------------------------- |
| Panels, lower zone, status bar                 | `#1F2429`                                               | `#F5F5F5`              | `background_primary_color`                        |
| Toolbar, toolboxes                             | `#272D33`                                               | `#EFF1F2`              | `background_secondary_color`                      |
| Panel section headers                          | `#2C333B`                                               | `#FFFFFF`              | `popup_background_color`                          |
| Text fields                                    | `#272F38`                                               | `#FFFFFF`              | `text_field_color`                                |
| Tab bar, separators                            | `#171B1F`                                               | `#D0D0D0`              | `stroke_color`                                    |
| Raised buttons (mode tabs, status-bar buttons) | `#36414D`                                               | `#E0E0E0`              | `button_color` (opacity 1.0)                      |
| Checked icon tool button                       | `#40ACFF`, black glyph                                  | `#0068AD`, white glyph | hard-coded in `DoricoButton` (icon-only buttons)  |
| Checked text button / tab                      | `#295B82`                                               | `#84CBE6`              | `accent_color` (accent opacities 0.8 / 0.6 / 1.0) |
| Text                                           | `#D0D5DB` (sampled peak `#C1C6CC`, raised for contrast) | `#1F2429`              | `font_primary_color`                              |
| Secondary text                                 | `#959BA2`                                               | (unchanged)            | `font_secondary_color`                            |
| Music-area gradient                            | top `#02629A`, middle `#0388C5`, bottom `#16A8E1`       | same                   | notation settings                                 |
| Page                                           | `#FFFFFF`                                               | same                   | `ui/canvas/foreground/color`                      |
| Selection                                      | `#FF7D00` (anti-aliased `#FFA346`)                      | same                   | engraving voice colors                            |

### Contrast (WCAG 2 ratios, checked after the first visual review)

Every readable text/background pair is at least AA (4.5:1). Rule: **text never sits on the bright tool blue**; it is only used behind icons, with a black (Dark) or white (Light) icon.

| Pair                                  | Dark                     | Light                                        |
| ------------------------------------- | ------------------------ | -------------------------------------------- |
| Text on panel / toolbar / button      | 10.6 / 9.4 / 7.0         | 14.3 / 14.3 / 11.8                           |
| Text on checked text button           | 4.9                      | 8.7                                          |
| Icon on checked tool button           | 8.6 (black on `#40ACFF`) | 5.9 (white on `#0068AD`)                     |
| Secondary text on panel               | 5.6                      | (MuseScore's)                                |
| Disabled item (opacity 0.45, was 0.3) | 3.3                      | 2.7 (disabled controls are exempt from WCAG) |

The first build used `#40ACFF` as the theme accent: checked text buttons (status bar "Transposed", "Page", and MuseScore dialog buttons) had grey text on bright blue, 1.5:1.

Glyph icons use `Text.QtRendering` (grayscale antialiasing): native LCD rendering gave thin stems and barlines blue/orange fringes. Bars and barlines uses `barlineFinal` (U+E032), which reads better small than the thin double barline.

## MuseScore today

- UI themes are `src/app/configs/{light,dark,high_contrast_*}.cfg`, loaded by `UiConfiguration::makeStandardTheme` (`muse/framework/ui/internal/uiconfiguration.cpp:285`). Default theme: Light.
- Canvas colors are settings: `notationconfiguration.cpp:40-50` (per-theme background, use-color, paper) and `engravingconfiguration.cpp:48-140` (voice colors = selection colors). `Settings::setDefaultValue` does not change a stored value (`settings.cpp:251`), so a new default would not apply.
- `FlatButton` draws icons from MuseScore's icon font only, but it accepts a `contentItem` component (`FlatButton.qml:102`). `Muse.GraphicalEffects` has `EffectColorOverlay` for tinting images.
- Bravura and Bravura Text are registered with `QFontDatabase` by the engraving module (`engravingmodule.cpp:182`).

## Design

- **Theme colors (upstream edit, values only):** `src/app/configs/dark.cfg` and `light.cfg` take the table above. Button opacity normal/hover/hit = 1.0/0.8/0.6, so buttons are opaque like Dorico's. Keys stay the same, so Preferences' theme reset gives back the Dorico colors.
- **Canvas colors and default theme (fork only):** `DoricoModule::onInit` (`src/dorico/doricomodule.cpp`) writes the music-area background (both themes), white paper and orange selection (voices 1-4 and "all voices") with `settings()->setSharedValue`, then switches to the Dark theme. It runs once per `APPEARANCE_VERSION`; bump the constant to re-apply after re-tuning. Same pattern as the one-time keymap switch next to it.
- **Icons:** `DoricoButton.doricoIcon` takes an `IconCode`, a SMuFL glyph string (drawn in Bravura Text, ink-centered and fitted to the icon box (see Icon sizes), tiny glyphs capped at a 100 px font) or `"svg:<name>"` (a Lucide SVG from `qml/MuseScore/Dorico/icons/`, tinted with `EffectColorOverlay`). Checked icon-only buttons use Dorico's bright tool blue with a black (Dark) or white (Light) glyph, as in Dorico; text buttons keep the muted theme accent. `ToolSection.icons` accepts the same values.
- Lucide SVGs come from `https://cdn.jsdelivr.net/npm/lucide-static@0.460.0/icons/<name>.svg`. Only the icons in use are committed, with `icons/LICENSE-lucide.txt`. They are added to the QML module with a `file(GLOB CONFIGURE_DEPENDS icons/*.svg)` in its `CMakeLists.txt` (re-globbed when icons are added or removed).

### Icon sizes

Measured against Dorico 6 in the manual's Write-mode screenshot, using UI text height as the common scale: Dorico's toolbox icons are ~1.4x and its Notes-panel notation 2-3x (durations 2.2x, accidentals 2.5x, articulations 3x) bigger than our first build.

| Where                                                               | Icon box (pt) | Button                         |
| ------------------------------------------------------------------- | ------------- | ------------------------------ |
| Toolboxes (Notes, Notations, Engrave)                               | 28            | 38, dock thickness 48 (was 44) |
| Notation in the Notes panel (durations, accidentals, articulations) | 34            | 44                             |
| Action icons in panels (`ToolSection` default)                      | 24            | 34                             |
| Top bar, status bar, other buttons (`DoricoButton` default)         | 22            | 32                             |

Glyphs fill 85% of the box (capped at a 100 px font, so dots stay dot-sized); SVGs are rendered at the box size.

### Icon mapping

Rule: the icon shows **what the button does in Dorico** (manual text), not just a related thing. Notation is drawn as notation (Bravura glyph or our own SVG of notes), actions as Lucide pictograms. Every icon button keeps its words as the tooltip and accessible name. Candidates were rendered side by side offscreen and checked before use.
Own SVGs (original, GPL, Lucide's 24 px / 2 px stroke style): `chord` (three stacked noteheads on one stem), `voice` (stem-up and stem-down notes), `tempo` (quarter note "="), `cue` (small note in brackets), `slur` (arc), `rehearsal-mark` (boxed A), `chord-symbol` (C⁷).

| Where                   | Button                                                               | Icon                                                                                                       |
| ----------------------- | -------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------- |
| Notes toolbox           | Show Left Zone / Select (disable mouse input) / Start Note Input     | `svg:panel-left` / `svg:mouse-pointer-2` / `svg:pencil`                                                    |
|                         | Pitch Before Duration                                                | U+E0A4 U+EB62 U+ECA5 (notehead, arrow, note: pitch then duration)                                          |
|                         | Chords / Insert                                                      | `svg:chord` / `svg:text-cursor-input` (insertion caret)                                                    |
|                         | Lock Duration / Force Duration                                       | `svg:lock` / `svg:pin` (pins the exact duration; distinct from Lock)                                       |
|                         | Create Voice                                                         | `svg:voice`                                                                                                |
|                         | Dotted notes / Rests / Tuplets / Grace notes                         | U+ECA5 U+ECB7 / U+E4E5 / U+E883 U+E88A U+E882 ("3:2", as Dorico) / U+E560                                  |
|                         | Tie / Scissors                                                       | U+E1F0 U+E1FD U+E1F0 (two notes tied) / `svg:scissors`                                                     |
| Notes panel             | Durations breve ... 128th                                            | U+ECA0, ECA2, ECA3, ECA5, ECA7, ECA9, ECAB, ECAD, ECAF                                                     |
|                         | Flat / Natural / Sharp / Slur                                        | U+E260 / E261 / E262 / `svg:slur`                                                                          |
|                         | Accent / Marcato / Stressed / Unstressed                             | U+E4A0 / E4AC / E4B6 / E4B8                                                                                |
|                         | Staccato / Tenuto / Staccatissimo / Tenuto-staccato                  | U+E4A2 / E4A4 / E4A6 / E4B2                                                                                |
| Notations toolbox       | Clefs / Key signatures / Time signatures                             | U+E050 / U+E262 U+E262 / U+E08A                                                                            |
|                         | Tempo / Dynamics / Ornaments                                         | `svg:tempo` / U+E522 / U+E566                                                                              |
|                         | Repeat structures / Bars and barlines / Holds and pauses             | U+E040 / U+E032 / U+E4C0                                                                                   |
|                         | Playing techniques / Lines                                           | U+E610 / `svg:move-up-right`                                                                               |
|                         | Cues / Video / Comments / Proofreading                               | `svg:cue` / `svg:film` / `svg:message-square` / `svg:check-check`                                          |
|                         | Rehearsal marks / Text / Lyrics / Chord symbols                      | `svg:rehearsal-mark` / `svg:type` / `svg:mic-vocal` / `svg:chord-symbol`                                   |
|                         | Note tools / Fingering / Figured bass / MIDI trigger                 | U+E0A4 / `svg:hand` / U+EA5B / `svg:keyboard-music`                                                        |
|                         | Popovers / panels switch                                             | `svg:square-pen` / `svg:layout-grid`                                                                       |
| Top bar                 | Hide/Restore Zones / Mixer / Help                                    | `svg:panels-left-bottom` / `svg:sliders-vertical` / `svg:circle-help`                                      |
| Status bar              | Follow playhead / Marquee tool / Page arrangement                    | `svg:locate-fixed` / `svg:square-dashed` / `svg:book-open`                                                 |
| Key editor (lower zone) | Advance Caret / Delete Left / Set Duration / Slur                    | `svg:arrow-right` / `svg:delete` / U+ECA5 / `svg:slur`                                                     |
| Engrave toolbox         | Show Left Zone / Graphic Editing / Frames                            | `svg:panel-left` / `svg:pen-tool` / `svg:frame`                                                            |
|                         | Staff Spacing / Note Spacing / Graphic Slices                        | `svg:move-vertical` / `svg:move-horizontal` / `svg:crop`                                                   |
| Engrave Formatting      | Move to Previous / Next System                                       | `svg:corner-left-up` / `svg:corner-right-down`                                                             |
|                         | Insert System Break / Lock System / Make into System                 | `svg:wrap-text` / `svg:lock` / `svg:fold-horizontal`                                                       |
|                         | Insert Frame Break / Lock Frame / Make into Frame                    | `svg:file-down` / `svg:lock` / `svg:fold-vertical`                                                         |
|                         | Lock Layout / Reset Layout / Coordination Line / Barline Joins       | `svg:lock` / `svg:rotate-ccw` / `svg:align-center-vertical` / `svg:link-2`                                 |
|                         | Insert Frames (vertical / text / horizontal) / Lock to Page Margins  | MuseScore's frame icons / `svg:scan`                                                                       |
| Setup: Players          | Add Single / Section Player, Add Ensemble, Add Group, Sort, Settings | `svg:user-plus`, `svg:users`, `svg:boxes`, `svg:folder-plus`, `svg:arrow-down-wide-narrow`, `svg:settings` |
| Setup: Flows            | Add Flow, Notation Options, Flow Settings, Delete Flow               | `svg:plus`, `svg:sliders-horizontal`, `svg:settings`, `svg:trash-2`                                        |
| Setup: Layouts          | Add Full Score / Part / Custom Score Layout                          | `svg:file-music` / `svg:file-user` / `svg:file-pen`                                                        |
|                         | Sort, Options, Settings, Delete                                      | `svg:arrow-down-wide-narrow`, `svg:sliders-horizontal`, `svg:settings`, `svg:trash-2`                      |

Still words, on purpose (Dorico shows words there too): status bar Grid value, Concert / Transposed Pitch, Order, Page / Galley / Fill; top-bar mode tabs, layout selector and View; Play mode's Track Inspector / VST and MIDI tabs; Print mode's options.

## Deferred / unsupported

| Item                                                                                           | Reason                                                                                                                            |
| ---------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------- |
| Gradient music-area background; separate Write / Engrave backgrounds                           | MuseScore has one color (or wallpaper) per theme; we use the gradient's middle color `#0388C5`                                    |
| Cream pages for part layouts                                                                   | MuseScore has one paper color for every layout                                                                                    |
| Voice colors shown only with Dorico's "Voice colors" view option                               | MuseScore tints selection by voice; every voice's selection color is orange instead                                               |
| Theme "Auto"                                                                                   | MuseScore's "Follow system theme" preference does the same                                                                        |
| Dorico's UI typeface                                                                           | Not documented; MuseScore's UI font stays                                                                                         |
| Icons outside the Dorico shell (MuseScore dialogs, palettes, Home page, status-bar zone arrow) | Keep MuseScore's icons                                                                                                            |
| Pixel-exact colors                                                                             | Measured from small, scaled screenshots; re-tune against a running Dorico if needed (bump `APPEARANCE_VERSION` for canvas colors) |

## Acceptance criteria

- [x] `dark.cfg` / `light.cfg` hold the measured Dorico colors; high-contrast themes unchanged.
- [x] On first start after this change the app switches to Dark, the music area is Dorico blue, pages are white, and selections are orange. A second start changes nothing (version key).
- [x] Notes toolbox, Notations toolbox (panels and popovers), Notes panel durations / accidentals / articulations, Engrave toolbox, Setup / Layouts action bars and the top bar use Bravura glyphs or Lucide icons; checked buttons show the glyph in the contrast color.
- [x] Lucide licence committed next to the icons; every `svg:` name used in QML has a file (checked by script).
- [x] Build and qmllint clean.
- [x] Contrast review from a screenshot of the running app (Dark): fixed accent text contrast, glyph fringes, disabled opacity.
- [x] Visual check in the running app, Dark and Light, next to Dorico 6 (manual: needs a GUI session).

## Tests

Manual: `cmake -P build.cmake run`; compare the Write, Engrave and Setup pages with Dorico 6 in Dark, then switch to Light in Preferences > Appearance. Every toolbox button must show an icon (an empty button means a missing glyph or SVG). Select a note in voices 1 and 2: both turn orange.
Check script (all `svg:` names exist): `cd src/dorico/qml/MuseScore/Dorico && grep -oh 'svg:[a-z0-9-]\+' *.qml | sort -u | while read s; do [ -f icons/${s#svg:}.svg ] || echo "MISSING $s"; done`
