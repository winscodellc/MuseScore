<!-- markdownlint-disable -->

# 21 — Dynamics popover (Shift-D)

Status: done · Depends on: 20 · Dorico refs: [dynamics reference](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_dynamics_dynamics_r.html), [gradual](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_dynamics_gradual_r.html), [modifiers](https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_dynamics_modifiers_r.html), [procedure](https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_notations_input/write_mode_dynamics_inputting_popover_t.html)

Reference: `specs/reference/dorico-popovers.md` (Dynamics), `specs/reference/musescore-internals.md` (§3).

## Goal

Shift-D opens the popover; typing `p`, `mf`, `fp`, `<`, `p<f>p`, `cresc...` etc. and pressing Return inserts the matching dynamic/hairpin at the selection or caret.

## Dorico behaviour

### Entry syntax

Immediate and combined dynamics:

| Entry               | Result                      |
| ------------------- | --------------------------- |
| `pppppp` … `p`      | Soft dynamics (1–6 p)       |
| `mp`, `mf`          | Medium                      |
| `f` … `ffffff`      | Loud (1–6 f)                |
| `fp`                | Fortepiano, no separator    |
| `f-p`, `f:p`, `f/p` | Fortepiano with a separator |
| `sfz`               | Sforzando                   |
| `rfz`               | Rinforzando                 |
| `(f)`               | Parenthesised dynamic       |

Gradual dynamics:

| Entry                          | Result                                                 |
| ------------------------------ | ------------------------------------------------------ |
| `<`                            | Crescendo hairpin                                      |
| `cresc`, `crescendo`           | "cresc." text                                          |
| `cresc...`, `crescendo...`     | cresc. with continuation line                          |
| `>`                            | Diminuendo hairpin                                     |
| `dim`, `diminuendo`            | "dim." text                                            |
| `dim...`, `diminuendo...`      | dim. with continuation line                            |
| `decresc`, `decrescendo`       | "decresc." text                                        |
| `decresc...`, `decrescendo...` | decresc. with continuation line                        |
| `<>` / `><`                    | Messa di voce: one hairpin with a shared centre handle |
| `< >` / `> <`                  | Two separate grouped hairpins                          |
| `o<`, `>o`                     | Niente hairpin with a circle                           |
| `n<`, `>n`                     | Niente hairpin with the letter "n"                     |
| `?`                            | Stop a gradual dynamic during note input               |

Modifiers (combined with an immediate dynamic, separated by a space: `f molto`, `p espressivo`; free text allowed):

| Entry                        | Modifier  |
| ---------------------------- | --------- |
| `subito`, `sub`, `sub.`      | subito    |
| `possibile`, `poss`, `poss.` | possibile |
| `poco`                       | poco      |
| `molto`                      | molto     |
| `piu`, `più`                 | più       |
| `meno`                       | meno      |
| `mosso`                      | mosso     |

`molto` and similar are placed automatically **before** the dynamic.

Phrases: `p<f>p`, `f>` create a grouped phrase. During note input each hairpin in a phrase lasts a quarter note; an open-ended entry (`p<`) extends as the caret advances.

### Placement and variants

|               | Return                                  | Opt-Return                              |
| ------------- | --------------------------------------- | --------------------------------------- |
| Voices/staves | All voices and staves of the instrument | Only the caret's voice (voice-specific) |

- **Selection range:** immediate dynamic at the earliest selected item; hairpins/gradual text span the range.
- **Caret (note input):** at the caret on all staves the caret spans. Open-ended hairpins (`p<`) start at the caret and extend until stopped (`?`, another dynamic, end of input).
- **Existing dynamic selected → Return:** reopens the popover with its entry text; applying replaces it.
- Brackets: `( … )` = parenthesised.

## MuseScore today

- Ctrl-D (`add-dynamic`) → `toggleDynamicPopup` (`src/notation/internal/notationinteraction.cpp:5827`) → `addTextToItem(DYNAMICS, el)` (:6204) → `addText` (:6220) starts text edit; the existing `DynamicPopup.qml` offers choices. `Dynamic::parseDynamicText` (`src/engraving/dom/dynamic.cpp:243`) and `endEdit` (:353) map typed text to `DynamicType`.
- `INotationInteraction::addHairpinsToSelection(HairpinType)` (:5467); `EditHairpin::addHairpin` in `engraving/editing/`.
- `applyPaletteElement` (:2224) is the generic way to drop a `Dynamic`/`Hairpin` on a list or range.
- Dynamics have a voice-assignment property (`Pid::VOICE_ASSIGNMENT`, values for all-voices-in-instrument / all-in-staff / current voice only). **Verify** the exact enum and Pid in `src/engraving/dom/types` and `dynamic.cpp`; it implements the Return vs Opt-Return distinction.
- Gaps: no combined `fp`-style entry with separators, no modifier words, no niente hairpins by text, no phrase grouping.

## Design

- Parser: `src/dorico/popover/parsers/dynamicsparser.{h,cpp}`.
  ```cpp
  struct DynamicItem { enum class Kind { Immediate, Hairpin, GradualText, Niente, MessaDiVoce }; ... };
  struct DynamicEntry { std::vector<DynamicItem> items; bool openEnded; bool parenthesised; bool stop; };
  ParseResult<DynamicEntry> parseDynamics(std::string_view);
  ```
  Tokenise left to right: dynamic letters (greedy `p{1,6}`, `f{1,6}`, `mp`, `mf`, `fp`, `f[-:/]p`, `sfz`, `rfz`), `<`, `>`, `<>`, `><`, word forms, then optional modifier words. `p<f>p` = items `[P, Hairpin(cresc), F, Hairpin(dim), P]`.
- Handler: `handlers/dynamicshandler.{h,cpp}`.
  - Immediate: `Factory::createDynamic`, set `Dynamic::setDynamicType(type)`; for entries with no `DynamicType` (e.g. `f-p`, modifier + dynamic) use `DynamicType::OTHER` with `setXmlText` built from SMuFL dynamic symbols and the modifier text (**verify** markup form against how the palette's custom dynamics are stored). Apply via `applyPaletteElement`.
  - Hairpin: `addHairpinsToSelection(HairpinType::CRESC_HAIRPIN / DIM_HAIRPIN)`; niente variants set `Pid::HAIRPIN_CIRCLEDTIP` (o) or begin/end text "n" (**verify** pid names); `cresc`/`dim` text variants use `HairpinType::CRESC_LINE`/`DIM_LINE` with begin text, `...` selects the line form.
  - `<>`: two hairpins end to end at the range midpoint (**ponytail:** midpoint of the range or one quarter each at the caret; no shared centre handle).
  - Phrase: apply items sequentially at consecutive positions: dynamic at start, hairpin spans to the next dynamic's tick; each hairpin one quarter at the caret.
  - Local (Opt-Return): after creating each element set the voice-assignment property to current-voice-only; default sets all-voices-in-instrument.
  - Parenthesised: set `Pid::DYNAMICS_PARENTHESES`-style property if present (**verify**), else wrap xmlText with parentheses.
- Open-ended at caret: **build the shared `PendingSpanner` helper here** (moved from spec 20, since this is its first user; specs 24, 28, 29 reuse it). Design as first sketched in spec 20: `PendingSpanner { type, startTick, track, payload }` held while note input is active, set by an open-ended entry (`p<`), finalised from `startTick` to the input tick when the stop token is entered, another item of the family is entered, or note input ends (`INotationNoteInput::stateChanged()` / `noteInputEnded()`). `// ponytail:` the line is created when stopped, not live-extended.
- `prefill`: for a `Dynamic` return its `dynamicType` text (`TConv::toXml`) or plain text; for a `Hairpin` return `<` / `>`.
- `itemTypes` (spec 20 replaced `handlesItem` with a type list): `DYNAMIC`, `HAIRPIN_SEGMENT` (selected hairpins are segments; check `isNotLine` inside `prefill`/`apply`).
- Key binding: `Shift+D` → `command://dorico/popover/dynamics` is already bound (spec 20). Replace `temporaryDynamics()` in `src/dorico/popover/popoverregistry.cpp` with the real def; keep its tests' expectations or update them in `popoverregistry_tests.cpp`.

## As built (wins over the Design section where they differ)

- Parser `popover/parsers/dynamicsparser.{h,cpp}` (no score access) returns `DynamicsParse { status, DynamicEntry }`; items are `Immediate`, `Hairpin`, `GradualText`. `<>` / `><` and `< >` / `> <` all parse to two hairpins (no `MessaDiVoce` kind, no shared centre handle). Immediate names are MuseScore's dynamic names (also `sfp`, `sffz`, `fz`, `n`, ...), except the bare letters `m r s z`. Two immediates need a gradual between them (`p f` is Invalid). A modifier is the rest of the entry after a space, attached to the last immediate; `molto poco più meno` go before the dynamic, everything else (free text included) after. Decided: `f mo` = Ok, free text "mo".
- Handler `popover/handlers/dynamicshandler.{h,cpp}` (`dynamicsPopover()`, replaces `temporaryDynamics()`). It adds items directly in one transaction (no `applyPaletteElement`):
  - Immediate: `Dynamic` with the first dynamic's type; the text is built from SMuFL dynamic symbols (`Dynamic::dynamicText`) plus separator, second dynamic and modifier, and `( )` for parentheses (plain text, not `Pid::HAS_PARENTHESES`). A dynamic already at that position on the staff is changed instead of adding a second one.
  - `<` `>`: `CRESC_HAIRPIN` / `DIM_HAIRPIN`; `o<` `>o` set the circled tip; `n<` `>n` add a niente `n` dynamic at the quiet end. `cresc` / `dim` / `decresc`: `CRESC_LINE` / `DIM_LINE` with that begin text, line hidden unless `...`.
  - Positions: range selection = each staff (or one staff per instrument for Return) over the range; a list selection = the earliest item, spanning its chord/rest; a selected hairpin is replaced over its own span; note input = the caret with one quarter per gradual. A phrase's gradual dynamics split the span evenly, each boundary snapped forward to the next note/rest on the staff (the last one is the note after the selection).
  - Return / Opt-Return: `setInitialTrackAndVoiceAssignment(track, local)`, i.e. all voices of the instrument vs the caret's / selected voice only.
  - Open-ended at the caret (`p<`, `f cresc...`): the last gradual is held as a pending item and created up to the caret when the next dynamics entry (incl. `?`) is applied or note input ends. `PopoverDef::noteInputEnded` (new, optional) is called by `PopoverController` for that. Not live-extended.
  - Outside note input the first new item is selected (Return reopens it). `prefill`: dynamic name or its plain text; hairpins give `<` `>` `o<` `>o`, lines their word (+ `...`).

## Deferred / unsupported

- `?` stop token outside note input (no-op, status Ok).
- Messa di voce shared centre handle (becomes two hairpins).
- Modifier free text beyond the table is accepted but is placed after the dynamic.
- Grouped hairpin pair behaviour for `< >` (two independent hairpins).
- Modifiers on gradual dynamics (`poco cresc.`): a modifier needs an immediate dynamic.
- Caret inside a rest (spec 12's grid caret): dynamics go to MuseScore's input position, not the virtual caret tick.
- A phrase whose last dynamic falls after the end of the score drops that dynamic.
- Linking dynamics across staves (Dorico auto-links; MuseScore applies per voice assignment).
- `Unsupported`: `7+` p/f letters is **Invalid** (more than 6).

## Acceptance criteria

- [x] Shift-D on a selected note, `mf`, Return → one `mf` dynamic under the note; Cmd-Z removes it.
- [x] `fp`, `sfz`, `rfz`, `pppppp`, `ffffff` produce the matching MuseScore dynamic types.
- [x] `f-p`, `f:p`, `f/p` produce custom text dynamics showing "f" and "p" with the chosen separator.
- [x] `<` on a range selection creates a crescendo hairpin spanning the range; `>` a diminuendo.
- [x] `cresc...` creates a cresc. line spanning the range; `cresc` creates text only.
- [x] `p<f>p` at the caret/selection creates three dynamics joined by two hairpins in order.
- [x] In note input, `p<` creates the `p` and the hairpin ends when the caret has advanced and the stop token/next dynamic/end of input occurs.
- [x] Return inserts into all voices/staves of the instrument; Opt-Return only the caret voice (check voice-assignment property).
- [x] Select an existing dynamic, press Return → popover prefilled; change to `f`, Return → replaced (no second dynamic).
- [x] `ppppppp` (7 p) is red/Invalid; `foo` is red/Invalid. _(parser tests; red border is spec 20)_
- [x] All GoogleTest parser cases below pass (as adapted: `<>` = two hairpins, `f mo` = free text).

## Tests

`src/dorico/tests/dynamicsparser_tests.cpp`:

| Input                  | Expected                                            |
| ---------------------- | --------------------------------------------------- |
| `p`                    | Ok, 1 item Immediate P                              |
| `mf`                   | Ok, Immediate MF                                    |
| `ffffff`               | Ok, Immediate FFFFFF                                |
| `fffffff`              | Invalid                                             |
| `fp`                   | Ok, Immediate FP                                    |
| `f-p`, `f:p`, `f/p`    | Ok, Combined F,P with separator `-`,`:`,`/`         |
| `sfz`, `rfz`           | Ok                                                  |
| `(f)`                  | Ok, parenthesised F                                 |
| `<` / `>`              | Ok, Hairpin cresc / dim                             |
| `cresc`                | Ok, GradualText cresc, no line                      |
| `crescendo...`         | Ok, GradualText cresc with line                     |
| `decresc...`           | Ok, GradualText decresc with line                   |
| `<>`                   | Ok, MessaDiVoce cresc-dim                           |
| `< >`                  | Ok, two Hairpins                                    |
| `o<`, `>o`, `n<`, `>n` | Ok, Niente (circle/letter)                          |
| `f molto`              | Ok, F with modifier molto (placed before)           |
| `p espressivo`         | Ok, P with free-text modifier                       |
| `p<f>p`                | Ok, items [P, cresc, F, dim, P]                     |
| `p<`                   | Ok, openEnded                                       |
| `?`                    | Ok, stop                                            |
| `f mo`                 | Incomplete or modifier free text (decide, document) |
| `ppppppp`, `xyz`       | Invalid                                             |
| `` (empty)             | Incomplete                                          |
