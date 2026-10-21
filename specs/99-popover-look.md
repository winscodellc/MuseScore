<!-- markdownlint-disable -->

# 99 - Popover look and placement

Status: todo - Depends on: 20-36, 94 - Dorico refs: `reference/dorico-popovers.md` (anatomy), manual images `popover_concept_example_with_music.png`, `popover_local_button.png`, `popovers_staff_relative_{above,below}.png`

Ground truth: `reference/dorico-shots/popovers/notes.md`, with 32 crop pairs `dorico-pop-<name>[-invalid].png` / `ours-pop-<name>[-invalid].png`. They come from Dorico Pro 6, 2026-10-21. The display is 1x, so px = pt.

## Goal

Every popover looks, sizes and sits like Dorico's: an inset field, a grip, the right glyph, a Local/Global button and Dorico's position. The entry syntax and behaviour (specs 20-36) are unchanged except where noted below. Today ours is a flat dark box with the text sitting directly on it, a solid handle bar and a "Global" text label. Ours is also about 1.7 times too wide for the staff and sits to the right of the note, and 8 of the 16 popovers show the wrong glyph.

## Dorico behaviour

### Anatomy (dynamics as the reference: `dorico-pop-dynamics.png`)

| Part | Dorico |
|---|---|
| Box | About 247 x 45 pt (dynamics) and about 175 x 34 (icon-only popovers). Radius about 8. Mid grey about `#4a4d53`, slightly translucent, no visible outer border. |
| Grip | A 2 x 3 dot grip at the far left, grey. Dragging it moves the popover. |
| Icon | A popover-specific glyph, 16-20 pt, white or grey, left of the field. |
| Field | A separate inset rounded rectangle of about 170 x 29 (dynamics), radius about 4, background about `#262b36`, **1 px blue focus border** about `#2f7bd6`. White text about 13 pt. |
| Local/Global | A person-icon button at the far right, on dynamics, time signatures, key signatures, bars, chord symbols and figured bass. It is absent on the others. |
| Position | Above the selection. The box's left edge is about 1.5 staff heights **left** of the note, and its bottom is about 0.3 staff height above the top staff line. "Below" popovers (`popovers_staff_relative_below.png`) go under the staff. |
| Invalid entry | **No visual change**: the blue border stays. Return is refused (nothing is created). |
| Prefill | The field is empty when creating. When editing an existing item, it holds that item's text, selected. Tuplets prefill `3:2`, selected. |
| Size versus zoom | **To verify:** Dorico's popover is a UI widget, so its size is probably fixed in points whatever the zoom. Take two Dorico shots at different zooms and record the result here. Use absolute points if the size is fixed, staff-relative if it scales. |

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
| Fingering ⇧F | **not captured** (re-take) | fretboard-grid icon | Capture first, then match |
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

- Icons per popover come from `iconCode`, set per popover type in `popovermodel.*` and the controller.
- Position is computed in `popovermodel.cpp` from the item or caret rect and the view matrix.

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
5. **Placement**: anchor the box's bottom-left at (note x − 1.5 staff heights, staff top − 0.3 staff heights), or the mirrored position for "below" popovers.
   - Clamp it inside the view.
   - Keep the drag-by-grip behaviour.
   - The size is fixed in points, or scaled with zoom, depending on the verification above.
6. **Prefills**: tempo and tuplets as in the table. Check every popover's prefill when it edits an existing item, against the Dorico Popover Reference.
7. **Chord symbols and lyrics**: chord symbols get the field, a count stepper and the person button. Lyrics get a proper popover (v1. label, verse stepper, placement toggle) that writes the lyric through the existing spec 33 handler, replacing `openInline`.

## Deferred / unsupported

- Dorico's exact icon artwork (hard constraint). Bravura and Lucide equivalents are used.
- The suggestion list style: Dorico showed no list in any shot. Keep ours, restyled.

## Acceptance criteria

- [ ] Re-take the Dorico fingering shot and the zoom-dependence check, and record the results in this spec.
- [ ] For each of the 16 popovers, a crop pair at equal zoom (or the documented equivalent) matches Dorico:
  - box and field size within 2 pt;
  - radius, grip, glyph and Local/Global button as in the tables;
  - colours within ±4 per channel.
- [ ] The popover sits above-left of the note as measured, and below the staff for "below" popovers.
- [ ] Typing an invalid entry shows no red border, and Return does nothing.
- [ ] Tempo has an icon and an empty field when creating. Tuplets prefill `3:2`.
- [ ] Lyrics (⇧L) opens a popover, and committing it creates the same lyric as today's inline path.
- [ ] All existing popover parser tests still pass (`dorico_tests --gtest_filter='Dorico*Parse*'`).

## Tests

- The existing GoogleTest parser suites stay green.
- Manual, run with the `ui-tester` agent: the 16-popover crop pass against `dorico-pop-*.png`. Launch Dorico Pro with the steps in `.claude/agents/ui-tester.md`. Don't zoom past 400%, and don't enter note input there: a high zoom triggered Dorico's toolbar loop once.
