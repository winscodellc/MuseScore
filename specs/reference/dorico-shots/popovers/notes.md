<!-- markdownlint-disable -->
# Popover audit: Dorico 6 vs our build

Shots: `dorico-pop-<name>[-invalid].png` vs `ours-pop-<name>[-invalid].png` (700x400 crops, 2026-10-21). Both taken with one note selected (not note input): Dorico on a flute chord at a larger zoom, ours on the first oboe note of the "Dawn" score (`audit.mscx`) at about 65% of Dorico's size. Compare in staff heights.

Staff height (5 lines, top to bottom line): **Dorico ~38.5 px (space 9.6)**, **ours ~25 px (space 6.2)**. Ratio ours/Dorico = 0.65. Pixel values below are measured by eye on the crops (+-2 px); "sh" = staff heights.

## Shared styling

| Item | Dorico | Ours |
|---|---|---|
| Box, dynamics | 247 x 45 px = 6.4 x 1.17 sh | 269 x 30 px = 10.8 x 1.2 sh (wider by 1.7x relative to staff) |
| Box, icon-only popovers (clefs, holds, ornaments, techniques, repeats, intervals) | ~175 x 34 px (4.5 x 0.9 sh) | ~226 x 30 px (9 x 1.2 sh) |
| Field | separate inset rounded rectangle, 170 x 29 px (4.4 x 0.75 sh) in dynamics; darker navy than the box; **1 px blue focus border** | no inset field: the text sits directly on the box; no border while valid |
| Radius | box ~8 px (0.2 sh), field ~4 px | box ~3 px (0.12 sh), no field |
| Icon | popover-specific music glyph, 16-20 px (0.45 sh), white/grey, left of the field | glyph ~12-14 px (0.5 sh), mostly the same idea but a different glyph on some (see below) |
| Handle | 6-dot (2x3) grip at the far left, grey | solid 4 px dark vertical bar at the far left (not a grip) |
| Local/Global control | person icon (button) at the far right of dynamics, timesig, keysig, bars, chords, figbass popovers; absent on clefs, tempo, holds, ornaments, techniques, repeats, tuplets, intervals | text label "Global" (grey, no button look) at the far right of dynamics, timesig, keysig and bars only |
| Colours | box mid grey ~#4a4d53 with slight transparency; field ~#262b36; blue border ~#2f7bd6; text white | box dark ~#2c3138 (almost black-blue), text grey-white; no border; invalid = 1 px **red** border |
| Font | UI sans, ~13 px typed text (0.34 sh), icon glyph bold | UI sans ~11 px typed text (0.44 sh), smaller and lighter |
| Position | above the selection, box left edge ~60 px (1.5 sh) **left** of the note, bottom ~0.3 sh above the staff top; caret in field | above the note, box left edge ~ at the note x (0 to 0.4 sh left), sits **right of** the note, bottom ~0.1 sh above the staff top, so it covers the neighbouring staff line above |
| Prefill | field empty and focused | dynamics prefilled "pp" (selected), tuplets "2:3" (selected), tempo "accel." (not selected, unrelated to the item); others empty |
| Invalid state (typed `zzz`) | **no visual change**: text shown, blue border stays, no message | red border, text kept, no message |
| Suggestions / list | none visible in any of the 32 shots | none |

## Per-popover differences

| Popover (key) | Icon Dorico / ours | Other differences |
|---|---|---|
| dynamics (Shift-D) | italic f / italic f (same) | ours prefilled "pp" selected; person icon vs "Global" text |
| timesig (Shift-M) | stacked "3/4" / diagonal "4/4" | person vs Global |
| keysig (Shift-K) | sharp+natural pair / same | person vs Global |
| clefs (Shift-C) | treble clef / treble clef (same) | Dorico box 175 wide, no person; ours same icon-only layout but wider and no handle grip |
| bars (Shift-B) | staff-with-barlines glyph / two thin bars (red+blue tint) | person vs Global |
| tempo (Shift-T) | crotchet "=72" icon, blue-border field / none (text "accel." prefilled, no icon, no border) | biggest mismatch: our tempo popover has no icon, no field and a prefill that is not selected, so typing `zzz` gives "accel.zzz" |
| holds (Shift-H) | fermata / fermata (same) | no person in either |
| ornaments (Shift-O) | "tr" / infinity-like turn glyph | glyph differs |
| techniques (Shift-P) | down/up-bow glyph / ">" accent glyph | glyph differs |
| repeats (Shift-R) | "1." volta ending glyph / `‖:` repeat barline | glyph differs |
| tuplets (`;`) | beamed 3-note glyph with small "3" / same, larger | Dorico prefilled "3:2" (selected), ours "2:3" (selected) |
| chords (Shift-Q) | big bold "C7" glyph, field, stepper "1" (chord count), person | ours: small "Cm7" label, no field, no stepper, no person; it opens on the selected note's measure (orange measure box) in a different state than Dorico |
| lyrics (Shift-L) | "v1." label, field, stepper "1" (verse), "down arrow" (placement) | ours: **no popover**; Shift-L starts in-score lyric editing (small boxed caret under the note), typed text becomes the lyric |
| fingering (Shift-F) | **not comparable**: the Dorico shot shows no popover (zoomed score, view changed); needs a re-take | ours: fretboard-grid icon, empty field |
| intervals (Shift-I) | fan-like glyph, no person / ours: filled notehead ellipse | glyph differs |
| figbass (Shift-G) | stacked "6/3" figure + person / ours: beamed note glyph | wrong glyph in ours; Dorico has person |

## Priorities for our build
1. Add the inset field, 6-dot grip, rounded radius ~0.2 sh and blue focus border; drop the solid handle bar.
2. Replace "Global" text with a person (Local/Global) button and show it on the same popovers as Dorico (chords, figbass too).
3. Invalid input: Dorico shows nothing; ours shows red. Decide whether to keep red (spec choice).
4. Position the box left of the note (box left ~1.5 sh left of the note), not right of it; fix size (narrower) relative to staff.
5. Fix glyphs: timesig stack, bars, tempo (add icon, drop "accel." prefill), ornaments "tr", techniques bow, repeats volta, intervals, figbass "6/3".
6. Lyrics popover (v1. field, verse stepper, placement arrow) and chord popover stepper are missing.
