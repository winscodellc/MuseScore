# 60 - Appearance: colors, score canvas, icons
Status: pending review  -  Depends on: 50  -  Dorico refs: `[M]user_interface/...` "Changing the window color theme" (API topic `iKF9BFI04pB8PG0opfbKpA`), "Project window in Write mode" (`gON2B7CLULjilph3Un5LwQ`), "Music area" (`bzfupzzVtDRcB8Dj1S612g`), "Changing the background color" (`H43HNanVmUSH0q2_mzu~6g`), "Changing music area colors" (`K2no0q0PuuZIF7iqOhsSIw`), "Changing the page color" (`hAV1f0AOkEajNr8cJlgtbw`)

## Goal
Make the fork look like Dorico Pro 6: Dorico's Dark and Light window colors, its music-area colors, and Dorico-style toolbox icons. No Dorico artwork is used: colors are measured values (facts). Icons are SMuFL glyphs from Bravura (OFL, shipped in `fonts/bravura`) or Lucide icons (ISC).

## Dorico behaviour
| Item | Behaviour (manual) |
|---|---|
| Theme | Preferences > General > Window > Theme: **Dark** (default), Light, Auto |
| Music-area background | A gradient of up to four colors, or a single color. Write mode and Engrave mode can each have their own (Setup uses Write's). Default: blue gradient |
| Page color | Per layout type. Default: full score and custom score layouts white, part layouts cream. Display only, not exported |
| Voice and selection colors | Preferences > Colors: the first eight voice colors, plus "Other colors" (selection, ...). Selected items are orange |

### Measured palette
Sources: the screenshots embedded in the manual pages above, read through `https://www.steinberg.help/api/khub/maps/BYbh2hvBDwUCjSijIhHcqA/topics/<id>/content` (`write_mode_project_window_pro.png` 850x500, `theme_dark_pro.png` / `theme_light_pro.png` 370x221, `music_area.png`). Values are the dominant pixel color of each region.

| Role | Dark | Light | MuseScore theme key |
|---|---|---|---|
| Panels, lower zone, status bar | `#1F2429` | `#F5F5F5` | `background_primary_color` |
| Toolbar, toolboxes | `#272D33` | `#EFF1F2` | `background_secondary_color` |
| Panel section headers | `#2C333B` | `#FFFFFF` | `popup_background_color` |
| Text fields | `#272F38` | `#FFFFFF` | `text_field_color` |
| Tab bar, separators | `#171B1F` | `#D0D0D0` | `stroke_color` |
| Raised buttons (mode tabs, status-bar buttons) | `#36414D` | `#E0E0E0` | `button_color` (opacity 1.0) |
| Checked icon tool button | `#40ACFF`, black glyph | `#0068AD`, white glyph | hard-coded in `DoricoButton` (icon-only buttons) |
| Checked text button / tab | `#295B82` | `#84CBE6` | `accent_color` (accent opacities 0.8 / 0.6 / 1.0) |
| Text | `#D0D5DB` (sampled peak `#C1C6CC`, raised for contrast) | `#1F2429` | `font_primary_color` |
| Secondary text | `#959BA2` | (unchanged) | `font_secondary_color` |
| Music-area gradient | top `#02629A`, middle `#0388C5`, bottom `#16A8E1` | same | notation settings |
| Page | `#FFFFFF` | same | `ui/canvas/foreground/color` |
| Selection | `#FF7D00` (anti-aliased `#FFA346`) | same | engraving voice colors |

### Contrast (WCAG 2 ratios, checked after the first visual review)
Every readable text/background pair is at least AA (4.5:1). Rule: **text never sits on the bright tool blue**; it is only used behind icons, with a black (Dark) or white (Light) icon.

| Pair | Dark | Light |
|---|---|---|
| Text on panel / toolbar / button | 10.6 / 9.4 / 7.0 | 14.3 / 14.3 / 11.8 |
| Text on checked text button | 4.9 | 8.7 |
| Icon on checked tool button | 8.6 (black on `#40ACFF`) | 5.9 (white on `#0068AD`) |
| Secondary text on panel | 5.6 | (MuseScore's) |
| Disabled item (opacity 0.45, was 0.3) | 3.3 | 2.7 (disabled controls are exempt from WCAG) |

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
- **Icons:** `DoricoButton.doricoIcon` takes an `IconCode`, a SMuFL glyph string (drawn in Bravura Text, ink-centered and fitted to a 20 px box, tiny glyphs capped at 60 px) or `"svg:<name>"` (a Lucide SVG from `qml/MuseScore/Dorico/icons/`, tinted with `EffectColorOverlay`). Checked icon-only buttons use Dorico's bright tool blue with a black (Dark) or white (Light) glyph, as in Dorico; text buttons keep the muted theme accent. `ToolSection.icons` accepts the same values.
- Lucide SVGs come from `https://cdn.jsdelivr.net/npm/lucide-static@0.460.0/icons/<name>.svg`. Only the icons in use are committed, with `icons/LICENSE-lucide.txt`. They are added to the QML module with a `file(GLOB icons/*.svg)` in its `CMakeLists.txt`.

### Icon mapping
| Button | Icon |
|---|---|
| Show Left Zone | `svg:panel-left` |
| Select / Start Note Input / Pitch Before Duration | `svg:mouse-pointer-2` / `svg:pencil` / `svg:pencil-line` |
| Chords / Insert / Lock Duration / Force Duration | `svg:layers` / `svg:between-vertical-start` / `svg:lock` / `svg:lock-keyhole` |
| Create Voice / Scissors | `svg:split` / `svg:scissors` |
| Dotted notes / Rests / Tuplets / Grace notes | U+ECA5 U+ECB7 (quarter + dot) / U+E4E5 restQuarter / U+E883 tuplet3 / U+E560 acciaccatura |
| Durations breve ... 128th (Notes panel) | U+ECA0, ECA2, ECA3, ECA5, ECA7, ECA9, ECAB, ECAD, ECAF (metNote*) |
| Flat / natural / sharp | U+E260 / E261 / E262 |
| Accent / marcato / staccato / tenuto | U+E4A0 / E4AC / E4A2 / E4A4 |
| Clefs / key signatures / time signatures | U+E050 gClef / U+E262 U+E262 / U+E08A timeSigCommon |
| Tempo / dynamics / ornaments | `svg:gauge` / U+E522 dynamicForte / U+E566 ornamentTrill |
| Repeat structures / bars and barlines / holds and pauses | U+E040 repeatLeft / U+E032 barlineFinal / U+E4C0 fermataAbove |
| Playing techniques / lines | U+E610 stringsDownBow / `svg:spline` |
| Note tools / figured bass | U+E0A4 noteheadBlack / U+EA5B figbass6 |
| Cues, video, comments, proofreading | `svg:mic`, `svg:film`, `svg:message-square`, `svg:check-check` |
| Rehearsal marks, text, lyrics, fingering, MIDI trigger | `svg:flag`, `svg:type`, `svg:text`, `svg:hand`, `svg:keyboard-music` |
| Notations toolbox: popovers / panels switch | `svg:square-pen` / `svg:layout-grid` |
| Top bar: Hide/Restore Zones, Mixer, Help | `svg:eye`, `svg:sliders-vertical`, `svg:circle-help` |
| Engrave toolbox | `svg:panel-left`, `svg:pen-tool`, `svg:frame`, `svg:move-vertical`, `svg:move-horizontal`, `svg:camera` |
| Setup / Layouts action bars | `svg:plus`, `svg:user-plus`, `svg:music`, `svg:list`, `svg:settings`, `svg:sliders-horizontal`, `svg:pencil`, `svg:trash-2` |
| Tie, slur, chord symbols, bracket | MuseScore icons (`NOTE_TIE`, `NOTE_SLUR`, `CHORD_SYMBOL`, `BRACKET`): no fitting glyph or Lucide icon |

## Deferred / unsupported
| Item | Reason |
|---|---|
| Gradient music-area background; separate Write / Engrave backgrounds | MuseScore has one color (or wallpaper) per theme; we use the gradient's middle color `#0388C5` |
| Cream pages for part layouts | MuseScore has one paper color for every layout |
| Voice colors shown only with Dorico's "Voice colors" view option | MuseScore tints selection by voice; every voice's selection color is orange instead |
| Theme "Auto" | MuseScore's "Follow system theme" preference does the same |
| Dorico's UI typeface | Not documented; MuseScore's UI font stays |
| Icons outside the Dorico shell (MuseScore dialogs, palettes, Home page, status bar arrows) | Keep MuseScore's icons |
| Pixel-exact colors | Measured from small, scaled screenshots; re-tune against a running Dorico if needed (bump `APPEARANCE_VERSION` for canvas colors) |

## Acceptance criteria
- [x] `dark.cfg` / `light.cfg` hold the measured Dorico colors; high-contrast themes unchanged.
- [x] On first start after this change the app switches to Dark, the music area is Dorico blue, pages are white, and selections are orange. A second start changes nothing (version key).
- [x] Notes toolbox, Notations toolbox (panels and popovers), Notes panel durations / accidentals / articulations, Engrave toolbox, Setup / Layouts action bars and the top bar use Bravura glyphs or Lucide icons; checked buttons show the glyph in the contrast color.
- [x] Lucide licence committed next to the icons; every `svg:` name used in QML has a file (checked by script).
- [x] Build and qmllint clean.
- [x] Contrast review from a screenshot of the running app (Dark): fixed accent text contrast, glyph fringes, disabled opacity.
- [ ] Visual check in the running app, Dark and Light, next to Dorico 6 (manual: needs a GUI session).

## Tests
Manual: `cmake -P build.cmake run`; compare the Write, Engrave and Setup pages with Dorico 6 in Dark, then switch to Light in Preferences > Appearance. Every toolbox button must show an icon (an empty button means a missing glyph or SVG). Select a note in voices 1 and 2: both turn orange.
Check script (all `svg:` names exist): `cd src/dorico/qml/MuseScore/Dorico && grep -oh 'svg:[a-z0-9-]\+' *.qml | sort -u | while read s; do [ -f icons/${s#svg:}.svg ] || echo "MISSING $s"; done`
