<!-- markdownlint-disable -->
# 33 — Popover: lyrics (Shift-L)

Status: done · Depends on: [20-popover-framework](20-popover-framework.md) · Dorico refs:
- [P] `https://www.steinberg.help/r/dorico/doricopopovers/6.2/en/_shared/topics/dorico/dorico_popovers/dorico_popovers_lyrics_r.html`
- [M] `https://www.steinberg.help/r/dorico-pro/6.2/en/dorico/topics/write_mode/write_mode_notations_input/write_mode_lyrics_inputting_t.html`
- [M] `.../notation_reference/notation_reference_lyrics/notation_reference_lyrics_copying_pasting_t.html`

Reference: [popovers](../reference/dorico-popovers.md) (B1, B2 "Lyrics") · [editing](../reference/dorico-editing.md) (section 1.9) · [internals](../reference/musescore-internals.md) (sections 1 and 3).

Names such as `parse`, `Result` and `apply` follow the contract in spec 20; rename to match its final API.

## Goal
Shift-L opens a lyrics entry. Typing syllables and pressing Space or `-` places each one on a note and advances to the next note, exactly as Dorico does.

## Dorico behaviour

This popover is a key map more than a syntax: the text typed is the syllable, and the keys control advancing.

| Key | Action |
|---|---|
| Space | End the word and advance to the next note. A gap or extender line is shown. |
| `-` | End the syllable and advance. A hyphen is shown. |
| Space or `-` repeated | Skip notes (melisma). |
| Right / Left | Move the cursor one character; at the edge, move into the next/previous lyric. |
| Opt-Right / Opt-Left | Next / previous syllable. |
| Shift-Opt-Space | Non-breaking space. |
| Opt-`-` | Literal hyphen without advancing. |
| `_` | Elision slur. |
| Up / Down | Lyric line number. Up from line 1 gives the chorus line (`c`). |
| Shift-Up / Shift-Down | Lyrics above / below the staff. |
| Opt-Down | Translation line. |
| Return or Esc | Close. |

- The popover closes automatically at the last note in the voice.
- **Start**: select a note first (the first note of the voice), then Shift-L. The popover opens at that note's lyric position.
- **Opt-Return**: no variant.
- **Editing existing**: Return on a selected lyric reopens the popover at it.
- **Paste**: Cmd-V pastes one word or syllable at a time and advances like Space or `-` would (a hyphen in the clipboard text acts as `-`). Each pasted syllable is removed from the clipboard. Clipboard text may use only single spaces and single hyphens as separators.

## MuseScore today
- Lyric entry exists: `add-lyric` (Ctrl+L) calls `addTextToItem(LYRICS, …)` → `startEditText` on a `Lyrics` element (`notationinteraction.cpp:6204`, `edit.cpp:640`).
- While editing lyric text, MuseScore has its own commands for next syllable, hyphen, melisma extender and moving to the next/previous lyric. They are enabled only while editing text/lyrics (`notationcommandsstate.cpp`, research section 1) and are bound in the `NOTATION_TEXT_EDITING` scope of `shortcuts.json`. Look in `notationcommands.h` for the exact command ids.
- Verses are a lyric property (verse number). Placement above/below is a property of the lyric (confirm the property id).
- `copy-lyrics` exists (`NotationInteraction::copyLyrics`, `notationinteraction.cpp:5222`).
- Elision handling and the translation/chorus lines have no direct MuseScore equivalent.

## Design
- Strategy: reuse MuseScore's lyric text-editing session as the engine and change only the keys. Do not build a second lyric editor.
- **New files** in `src/dorico/internal/popovers/lyrics/`:
  - `lyricskeymap.h/.cpp`: the table that maps the keys above to MuseScore's lyric text-editing commands.
  - `lyricspopovermodel.h/.cpp`: thin wrapper; its `open()` starts the editing session.
- **Shift-L:** a `command://dorico/popover-lyrics` handler that calls `addTextToItem(LYRICS, selectedNote)` and activates the Dorico lyric key map while the lyric is being edited. The spec 20 framework draws no entry field in this case, because the lyric's own on-score text editing is the field. State this in the spec 20 contract: a popover may declare `usesInlineEditor = true`.
- **Key map** (scope `NOTATION_TEXT_EDITING` entries in `shortcuts_dorico.json`, added by spec 10 or this spec):
  - Space → MuseScore's "next syllable with gap".
  - `-` → "next syllable with hyphen".
  - `_` → melisma extender (this is MuseScore's meaning of `_`, not Dorico's elision; see Deferred).
  - Opt-Right / Opt-Left → previous/next syllable.
  - Up / Down → verse number.
  - Shift-Up / Shift-Down → set the lyric's placement above/below.
  - Return / Esc → end text editing (`endEditText`, `notationinteraction.cpp:4373`).
- **Auto-close at last note:** MuseScore ends lyric editing when there is no next chord. Verify and rely on it.
- **Cmd-V word-by-word:** read the clipboard string, take the first token, insert it into the current lyric and run the Space or `-` command depending on the separator, then write the remainder back to the clipboard. Mark as stretch if the clipboard write is awkward.
- **Upstream edits:** none beyond spec 20's overlay hook. Dorico's Space inside lyric editing replaces MuseScore's, which is the default already; check for a conflict.

## Deferred / unsupported
- Translation line (Opt-Down) and chorus line (`c`): MuseScore verses are numbered only.
- Elision slur (`_` in Dorico). MuseScore's `_` means a melisma extender; leave the default and note the difference.
- Non-breaking space (Shift-Opt-Space) and the literal hyphen (Opt-`-`), unless MuseScore's lyric editor has direct equivalents.
- Skipping notes with repeated Space/`-` exactly as Dorico draws them, if MuseScore draws extender lines differently.
- Placement above/below per lyric line beyond what MuseScore's lyric property supports.

## Acceptance criteria
- [x] Select the first note of a voice, press Shift-L, type `Hel`, press `-`, type `lo`, press Space: "Hel-" is on note 1, "lo" on note 2, and editing continues on note 3.
- [x] Pressing Space twice with no text skips a note.
- [x] Opt-Right / Opt-Left move between existing syllables.
- [x] Up / Down change the verse number; Shift-Up puts the lyric above the staff.
- [x] Return or Esc ends the entry; the popover also closes after the last note of the voice.
- [x] Return on a selected lyric reopens editing at it.
- [ ] (not UI-tested: no X clipboard tool on the test machine) Cmd-V with `one two-three` on the clipboard places three syllables (stretch).
- [x] One undo per word typed, or one for the whole entry session; choose one in spec 20 and apply it consistently.

## As built (corrections to the design above)
- No `lyricspopovermodel` or `lyricskeymap` C++ table. `PopoverDef::openInline` (new) makes Shift-L run `handlers/lyricshandler.cpp` instead of opening a box: it starts MuseScore's lyric editor on the selected note, editing its line 1 lyric if it has one (`add-lyrics` would add line 2). The key map is data in `shortcuts_dorico.json` (`NOTATION_TEXT_EDITING`, which wins over every other scope while the score has focus) and `check_keymap.py` covers it; `itemTypes` stays empty, so Return on a lyric is MuseScore's edit-text.
- Keys while editing a lyric: Space `edittext-next-word`, `-` `editlyrics-next-syllable`, `_` `editlyrics-add-melisma` (MuseScore's extender, not Dorico's elision), Right/Left at the text edge and Opt-Right/Opt-Left `edittext-next/prev-element`, Up `editlyrics-prev-verse`. New `dorico/*` commands, enabled **only** while a lyric is edited (`DoricoCommandsState`): Return `lyrics-close`, Down `lyrics-next-line` (MuseScore's next-verse only visits existing lines and its add-verse always adds one; this goes to the next line, adding it when missing), Shift-Up/Down `lyrics-above/below` (set `Pid::PLACEMENT` **unstyled**, or the next lyric, which copies the flag, snaps back to the style). Return no longer adds a verse (`editlyrics-add-verse` unbound).
- Upstream edit: `Lyrics::isEditAllowed` (`engraving/dom/lyrics.cpp`) releases Opt-Left/Right, otherwise the text editor moves by word and the shortcut never fires.
- Undo: MuseScore's, one step per syllable/action (decided: kept, not merged per session).
- Last note of the score: Space keeps editing the same lyric (MuseScore's behaviour); editing does not close by itself. Kept.
- Paste: MuseScore's lyric paste already does one syllable per Cmd-V, advancing and leaving the rest on the clipboard (`NotationInteraction::pasteSelection` → `extractSyllable`). Not UI-tested (no `xclip` on the Linux test machine).

## Tests
No pure parser exists for this popover. Test the key map as data:

GoogleTest, `src/dorico/tests/lyricskeymap_tests.cpp`:

| Key | Expected command |
|---|---|
| `Space` | next syllable (gap) |
| `-` | next syllable (hyphen) |
| `Alt+Right` | next existing syllable |
| `Alt+Left` | previous existing syllable |
| `Up` | verse number up |
| `Shift+Up` | placement above |
| `Return` | end editing |
| `Esc` | end editing |

If the clipboard splitter is implemented, test it as a pure function `splitLyricClipboard(string) -> vector<{text, separator}>`:

| Input | Output |
|---|---|
| `one two` | `[{one, space}, {two, end}]` |
| `one-two` | `[{one, hyphen}, {two, end}]` |
| `one  two` (double space) | `Invalid` |

Manual: the acceptance list on a one-staff score with eight quarter notes.
