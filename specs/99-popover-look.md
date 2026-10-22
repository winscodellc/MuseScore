<!-- markdownlint-disable -->

# 99 - Popover look and placement

Status: done - Depends on: 20-36, 94 - Dorico refs: `reference/dorico-popovers.md` (anatomy), manual images `popover_concept_example_with_music.png`, `popover_local_button.png`, `popovers_staff_relative_{above,below}.png`

Ground truth: `reference/dorico-shots/popovers/notes.md`, with 32 crop pairs `dorico-pop-<name>[-invalid].png` / `ours-pop-<name>[-invalid].png`. They come from Dorico Pro 6, 2026-10-21. The display is 1x, so px = pt.

## Goal

Every popover looks, sizes and sits like Dorico's: an inset field, a grip, the right glyph, a Local/Global button and Dorico's position. The entry syntax and behaviour (specs 20-36) are unchanged except where noted below. Today ours is a flat dark box with the text sitting directly on it, a solid handle bar and a "Global" text label. Ours is also about 1.7 times too wide for the staff and sits to the right of the note, and 8 of the 16 popovers show the wrong glyph.

## Dorico behaviour

### Anatomy (dynamics as the reference: `dorico-pop-dynamics.png`)

| Part | Dorico |
|---|---|
| Box | Measured on the crops (exact pixels): 247 x 49 with the person button, 180 x 39 without (fingering 204 x 39, tempo wider). Radius about 8. `#404347`, opaque, no outer border. |
| Grip | A 2 x 3 dot grip at the far left, grey. Dragging it moves the popover. |
| Icon | A popover-specific glyph, 16-20 pt, white or grey, left of the field. |
| Field | An inset rounded rectangle 29 high, starting 42 px into the box: 171 wide on the person-button popovers, 133 on the others, 157 on fingering, 296 on tempo. Radius about 4, `#252C36`, **1 px blue border** `#246DA6`. White text about 13 pt. Icon and person glyphs `#DBE0E6`, grip dots `#8E9196`. |
| Local/Global | A person-icon button at the far right, on dynamics, time signatures, key signatures, bars, chord symbols and figured bass. It is absent on the others. |
| Position | Above the selection. The box's left edge is about **55 px left** of the note centre at every zoom, and its bottom is about **0.5 staff height** above the top staff line (it scales with the score). No popover opens below the staff: `popovers_staff_relative_below.png` is only the lyric placement arrow. |
| Invalid entry | **No visual change**: the blue border stays. Return is refused (nothing is created). |
| Prefill | The field is empty when creating. When editing an existing item, it holds that item's text, selected. Tuplets prefill `3:2`, selected. |
| Size versus zoom | **Verified (2026-10-21)**: the box is 247 x 48.5 px at both 100% and 200% (staff 24.5 and 36 px), so the size is fixed in points. |

### Glyph per popover (Dorico → ours today)

| Popover | Dorico icon | Ours | Fix |
|---|---|---|---|
| Dynamics ⇧D | italic **f** | same | — |
| Time sig ⇧M | stacked **3/4** | diagonal 4/4 | Bravura `timeSig3` over `timeSig4` |
| Key sig ⇧K | sharp + natural pair | same | — |
| Clefs ⇧C | treble clef | same | — |
| Bars ⇧B | staff with barlines | two thin tinted bars | Bravura barline glyphs on a staff fragment |
| Tempo ⇧T | crotchet "= 72" | **no icon**, no field, prefilled "accel." (not selected) | Add the icon and the field. Fix the prefill: empty when creating; when editing, the existing text, selected. |
| Holds ⇧H | fermata | same | — |
| Ornaments ⇧O | **tr** | turn-like glyph | Bravura `ornamentTrill` |
| Playing techniques ⇧P | down/up bow | accent `>` | Bravura `stringsDownBow` / `stringsUpBow` |
| Repeats ⇧R | "1." volta | repeat barline `‖:` | Volta bracket "1." (drawn or glyph) |
| Tuplets ; | beamed 3 with a small 3 | same, larger | Size; prefill `3:2` instead of `2:3` |
| Chord symbols ⇧Q | large bold "C7" + field + count stepper + person | small "Cm7" label only | Field, stepper (chord-symbol region count), person button |
| Lyrics ⇧L | "v1." label + field + verse stepper + placement arrow | **no popover**: in-score editing (spec 33 `openInline`) | Spec choice: Dorico 6 shows a popover. Match it (a popover, with the text committed into the lyric). Re-check spec 33's syntax table. |
| Fingering ⇧F | "5" over a note, no person button (re-taken 2026-10-21) | fretboard-grid icon | `fingering5` over `noteQuarterUp` |
| Note tools ⇧I | fan-like glyph | filled notehead | Match the Dorico glyph from the Bravura set |
| Figured bass ⇧G | stacked "6/3" + person | beamed note | Bravura `figbass6` over `figbass3`, plus the person button |

## MuseScore today

All in `src/dorico/qml/MuseScore/Dorico/Popover.qml`:

| Part | Line(s) | What it does |
|---|---|---|
| Box | `:85-94` | Height 32, radius 4, 1 px border; the invalid colour is hard-coded `#E0453D` at `:46` |
| Handle | `:104-109` | A solid 6x20 bar |
| Icon | `:122-125` | A `StyledIconLabel` using MuseScore's icon font |
| Field | `:130-133` | A bare `TextInput`, 180 wide |
| Global | `:179-184` | Text at 0.7 opacity |
| Suggestions | `:197-224` | The suggestion list |
| View matrix | `:51` | Binds `viewMatrix`; spec 96 makes it bind only while the popover is open |

- Icons per popover come from `iconCode`, set per popover type in `PopoverDef::icon` (`popover/popovertypes.h`) and exposed by `qml/MuseScore/Dorico/popovermodel.cpp`.
- Position: `PopoverController::computeAnchor` (`internal/popovercontroller.cpp`) gives the item or caret rect; `popovermodel.cpp` maps it through the view matrix.

## Design

1. **Restyle `Popover.qml`**: an outer box, a `DotGrip` (a new small component), an icon, an inset field (a `Rectangle` holding the `TextInput`), an optional person button, and the suggestion list styled like the box.
   - Add theme constants to a `DoricoPopoverStyle` singleton (colours, radii, sizes) so the values live in one place.
2. **Glyphs**: switch the icon to a Bravura glyph per popover. Text in the Bravura font, with SMuFL code points, as the Notes panel already does.
   - Add `popoverGlyph` to the model, alongside or instead of `iconCode`.
   - The time signature and figured bass need two stacked glyphs, so make the icon a small component.
3. **Local/Global button**:
   - A person-icon toggle with the same behaviour as the "Global" text today.
   - It is shown on the six Dorico popovers.
   - Tooltip: "Local / Global". The icon is from Lucide (`user`), never Dorico's artwork.
4. **Invalid state**: match Dorico, so there is no red border and the blue focus border stays.
   - Return still refuses, as today.
   - **Changes current behaviour.** The user asked for an exact match, so remove the red border, but keep a `dorico/popoverShowInvalid` setting (default off) in case it is wanted back.
5. **Placement**: anchor the box's bottom-left at (note x − 50 px, staff top − 0.5 staff heights).
   - Clamp it inside the view.
   - Keep the drag-by-grip behaviour.
   - The size is fixed in points, or scaled with zoom, depending on the verification above.
6. **Prefills**: tempo and tuplets as in the table. Check every popover's prefill when it edits an existing item, against the Dorico Popover Reference.
7. **Chord symbols and lyrics**: chord symbols get the field, a count stepper and the person button. Lyrics get a proper popover (v1. label, verse stepper, placement toggle) that writes the lyric through the existing spec 33 handler, replacing `openInline`.

## Deferred / unsupported

- Dorico's exact icon artwork (hard constraint). Bravura and Lucide equivalents are used.
- The suggestion list style: Dorico showed no list in any shot. Keep ours, restyled.
- Chord symbols' region-count stepper: MuseScore has no chord-symbol regions/lines, so the box has no stepper and is 64 px narrower than Dorico's (field and person button match).
- The person button on chord symbols and figured bass is drawn (Dorico has it) but dimmed: those handlers have no local variant yet. Its tooltip says so.
- The `dorico/popoverShowInvalid` setting (design step 4) was not added: nothing asked for the red border back, and it is one line in `Popover.qml` if wanted.
- Lyrics (decided with the user, 2026-10-21): no real popover. The lyric is still typed in MuseScore's in-score editor (spec 33, all its keys kept), and a Dorico-style box (grip, "vN." label, a field mirroring the syllable, line stepper, placement arrow) is drawn while a lyric is edited. It sits under the lyric instead of above the staff, so it never covers the text being typed.

## Acceptance criteria

- [x] Re-take the Dorico fingering shot and the zoom-dependence check, and record the results in this spec.
- [ ] (15 of 16 match; chord symbols lack the region stepper and lyrics are a mirror box, see Deferred) For each of the 16 popovers, a crop pair at equal zoom (or the documented equivalent) matches Dorico:
  - box and field size within 2 pt;
  - radius, grip, glyph and Local/Global button as in the tables;
  - colours within ±4 per channel.
- [x] The popover sits above-left of the note as measured.
- [x] Typing an invalid entry shows no red border, and Return does nothing.
- [x] Tempo has an icon and an empty field when creating. Tuplets prefill `3:2`.
- [ ] (user decision: mirror box over the in-score editor, see Deferred) Lyrics (⇧L) opens a popover, and committing it creates the same lyric as today's inline path.
- [x] All existing popover parser tests still pass (`dorico_tests --gtest_filter='Dorico*Parse*'`).

## Tests

- The existing GoogleTest parser suites stay green.
- Manual, run with the `ui-tester` agent: the 16-popover crop pass against `dorico-pop-*.png`. Launch Dorico Pro with the steps in `.claude/agents/ui-tester.md`. Don't zoom past 400%, and don't enter note input there: a high zoom triggered Dorico's toolbar loop once.

## As built

- `PopoverDef` (`popover/popovertypes.h`): `icon` became `glyph` (SMuFL text in Bravura Text, `\n` stacks lines, or `svg:<name>`). New `scopeButton` (Dorico's person button) and `fieldWidth` (measured). Each handler sets them.
- `Popover.qml` holds everything: the colour and size constants, and inline `DotGrip` and `PopoverGlyph` components (a separate singleton or files were not needed). Plain-text icons (3/4, C7, v1.) are drawn bold in the UI font, because Bravura Text has no plain time-signature digits. Icons fill at most 85% of the slot width.
- The person button toggles `localScope`: Return then applies the local variant, as Opt-Return does. With both, Opt-Return gives the global variant.
- Placement: `PopoverController::computeAnchor` raises the anchor 2 spaces above the staff top (at the caret: exactly the staff top), and the QML puts the box's left edge 54 px left of the note centre (or of a wide range's left edge). The size is fixed.
- `bars` and `repeats` icons are own SVGs (`icons/popover-bars.svg`, `popover-repeats.svg`); the person icon is Lucide `user`.
- Tempo: `zzz` is valid free tempo text (spec 26), so Return creates it. That is correct, not an invalid case.
- Verified with the ui-tester on Dawn (staff 36 px): every size within 1 px, left edge 53 px, gap 0.5 staff heights (fingering 0.6: a stem-up chord reaches above the staff), same place in note input, all icons visible and centred.
