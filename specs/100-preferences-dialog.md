<!-- markdownlint-disable -->

# 100 - Preferences dialog

Status: built, pending review - Depends on: 94, 60 - Dorico refs: `reference/manual/topics/preferences/preferences-dialog.md`, images `preferences_dialog.png`, `preferences_dialog_key_commands_page.png`

Ground truth: `reference/dorico-shots/prefs/notes.md`. It has the Dorico Pro 6 chrome, every category with every option label and control type, our 15 pages, and a Dorico → ours mapping table. Captured 2026-10-21.

## Goal

Cmd-, opens a Dorico-style Preferences dialog. It has Dorico's categories in Dorico's order, the same search and layout, Dorico's control style and Dorico's bottom buttons. Every Dorico option that MuseScore can back is wired to the MuseScore setting. The rest are shown disabled, labelled "Not available yet". MuseScore-only settings that matter (Audio & MIDI devices, Folders, Advanced, ...) stay reachable: they are folded into the nearest Dorico category, under a separator titled "MuseScore".

## Dorico behaviour

| Part | Dorico Pro 6 |
|---|---|
| Window | 1130 x 708 pt including the title bar (re-measured 2026-10-22; our content area is 677), resizable, title "Preferences". |
| Left column | About 203 pt, flat black. The top has a **"Search categories..."** field (auto-focused, Cmd-L, blue 2 px focus ring). Below it is one list: **Colors** (4 sub-rows), **General** (General, Window, View, Files), **Key Commands**, **MusicXML** (Import, Export), **Note Input and Editing** (Note Input, Editing), **Play**, **Proofreading** (Proofreading, Categories), **Remote**, **VST Plug-ins**. Rows have a 27 pt pitch (a 25 pt row, a 2 pt gap), text 13 px, no icons. Rows are `#282d33` on `#131415`; the selected row is `#454a4f`, with a 3 px `#40acff` left bar. |
| Right pane | The page title band (lighter, 16 px). Section titles are dark full-width bands (bold 13 px). Each section is a **rounded dark card** (radius about 12, `#171b1f` on pane `#1f2429`) holding label + control rows, with italic grey help text and blue "?" help icons. A group (for example General) scrolls as one page, and its sub-rows are anchors. |
| Controls | Grey dropdowns with a chevron. **Segmented toggles**: the selected one is bright blue `#40acff` with black bold text, the others `#36414d`. Square checkboxes with a blue tick. Spinners. "Choose..." buttons. Colour rows: hex field + swatch + reset. |
| Page search | **Cmd-F** opens a "Search page..." bar with previous and next buttons, a match count and a close X. |
| Bottom bar | **Reset to Factory** (left), **Apply** (disabled until something changes), **Close**. There is no OK or Cancel. Close or Escape with unapplied changes asks "You have made changes to the Preferences. Do you want to apply them now?" with **Discard Changes**, **Cancel** and **Apply Changes**. Escape closes the Cmd-F bar first, then the window. Verified in Dorico Pro 6 on 2026-10-22. |
| Colours | Header band `#2b2f35`, accent `#40acff` (the first audit, `prefs/notes.md`, has `#2da0f5`, `#1e2125` and `#16191c`: those were off; re-measured from screenshots on 2026-10-22, matching spec 60's tool blue), text white-grey. |

The per-page option lists are in `prefs/notes.md`, under "Dorico pages", and are the spec for each page's content. The mapping table in the same file says which MuseScore setting backs each option.

## MuseScore today

- The dialog is upstream code: `src/preferences/qml/MuseScore/Preferences/`. Its pages are listed in `preferencesmodel.cpp:158-200`.
- 880 x 672, fixed size, a 220 pt icon list of 15 pages, no search, Reset preferences / Cancel / OK.
- Escape does not close it (audit). Dorico's Close / Escape behaviour has to be matched.
- The fork has not changed it. Cmd-, is bound in `src/dorico/resources/dorico_keys.json:5` and `doricomenu.cpp:148`.

## Design

1. **A new dialog in the dorico module**:
   - `DoricoPreferencesDialog.qml`, registered as `musescore://dorico/preferences`.
   - Cmd-, and the app menu "Preferences..." open it instead of MuseScore's.
   - MuseScore's dialog stays reachable from Window > More MuseScore Commands, as a fallback during the transition.
   - No upstream edits.
2. **Data model**: `DoricoPreferencesModel` (C++), a table of categories → sections → option rows.
   - Each row has: label, control type, help text, and a getter / setter bound to the existing configuration interface (`INotationConfiguration`, `IAppShellConfiguration`, `IPlaybackConfiguration`, `IShortcutsConfiguration`, ...), or `unsupported`.
   - Generate the table from the `prefs/notes.md` mapping.
   - Apply semantics: edits are staged; **Apply** commits them; the staging is MuseScore's settings transaction (`startEditSettings` / `applySettings` / `rollbackSettings`), not a map of our own. **Close** and Escape with unapplied changes ask first (Discard Changes / Cancel / Apply Changes, as Dorico does); the window's own close box discards without asking. **Reset to Factory** resets **every** option (the manual: "all the options in the dialog"), and asks first. Dorico showed no confirmation in the test run, and the scope there was not established (it reset at least the General page); we keep the confirmation.
3. **Reusable controls**:
   - `DoricoCard`, `DoricoSectionBand`, `DoricoSegmented`, `DoricoColorRow` (hex + swatch + reset) and `DoricoHelpIcon`, in `src/dorico/qml/MuseScore/Dorico/prefs/`.
   - Spec 101 can reuse them for the Options dialogs.
4. **Search**: the category filter matches category and sub-row names. **Cmd-F** searches labels on the current page, highlighting matches with previous / next and a count.
5. **Key Commands page**: build it on the existing shortcuts model (`src/shortcuts`): a per-command list with a key editor, the keyboard language shown read-only. Print Summary and Jump Bar Alias are stubs (spec 68 for aliases).
6. **MuseScore-only settings**:
   - Audio / MIDI device → Play.
   - Folders, Save, Autosave → General > Files.
   - Theme / UI font → Colors or General > Window.
   - Braille and Advanced → a "MuseScore" section at the end of General.
   - Each sits under a "MuseScore" band, so it is clear they are not Dorico's.

## Deferred / unsupported

Not done in this spec:
- Most Dorico rows have no MuseScore setting: only 12 are backed (Language, Theme, Default zoom, Default project folder, Auto-save + minutes, Full score page colour, Invert colors, accidental timing, Play notes / chord, Follow playhead, MIDI input; Import breaks when MusicXML is configured). `prefs/notes.md` "Mapping" lists which could be added.
- "Dorico" and "Steinberg" in row labels read "MuseScore".
- The Key Commands page is MuseScore's shortcut table (and MIDI mappings) in a framed area, not Dorico's per-command tree, keyboard language, Print Summary or Jump Bar Alias (stubs, spec 68).
- Colors: only the full-score page colour and Invert are backed; voice colours 1-8, other colours and the background gradients are disabled (MuseScore's own live under General > Window > Appearance and General > Files > Advanced).
- The "MuseScore" bands embed MuseScore's own pages, collapsed, in a fixed-height frame: Appearance, Canvas, General, Save & publish, Folders, Score, Video, Braille, Advanced, Import, Note input, Percussion, Audio & MIDI, shortcuts and MIDI mappings. They keep MuseScore's look, and Cmd-F does not search inside them.
- Cmd-Q does not quit while the dialog is open (the dialog claims every key but Cmd-Q, and the framework still swallows it); close the dialog first.
- The window's own close box discards without the "apply them now?" prompt.
- The window size is not remembered between openings.
- Round-trip tests use mocks for the notation, project, playback and UI configurations only; Language and the MusicXML option have none and are covered by the manual test.

Shown disabled:
- Proofreading (spec 79);
- Remote;
- VST Plug-ins (MuseScore's VST scanning may back part of it; check);
- MIDI recording options (77);
- the Note Input "Lock Duration" / "Force Duration" behaviours, if not backed by spec 11.

## Acceptance criteria

- [x] Cmd-, opens the new dialog at about 1130 x 708. It is resizable, and its categories and sub-rows are in Dorico's order.
- [x] Category search (Cmd-L) filters the list. Cmd-F finds labels on the page, with a working previous / next and count. (Checked with Cmd-G / Cmd-Shift-G; the arrow buttons call the same function and were not clicked.)
- [ ] Every page shows Dorico's sections and options in order (diff against `prefs/notes.md`). Backed options read and write the real MuseScore setting (restart persists it). Unbacked ones are disabled, with a tooltip. *Not ticked:* the table was written from `notes.md` and each page was looked at, but no row-by-row diff was run. A restart was checked for Auto-save and Follow playhead only, and Language, Theme, Default zoom, Default project folder, Invert colors, the page colour and MusicXML Import breaks were not changed in the app.
- [ ] Apply, Close and Reset to Factory behave as verified in Dorico. Escape closes the dialog. *Not ticked:* Apply, Close (the "apply them now?" prompt) and Escape match Dorico Pro 6 (checked in both). Reset to Factory differs on purpose (see Design 2): ours confirms; Dorico showed no confirmation and its scope was not established.
- [ ] Side-by-side crops (category list, one card, one segmented control, bottom bar) match the Dorico values in the table above, within 2 pt and ±4 per channel. *Not ticked:* the category list, card, pane, accent and segmented control match the re-measured Dorico values (table above); the bottom bar and the section / page bands were not cropped against Dorico.
- [ ] No MuseScore setting that ours exposes today becomes unreachable (checklist from `prefs/notes.md`, "Ours"). *Not ticked:* all 15 MuseScore pages plus shortcuts and MIDI mappings are embedded; Folders, Advanced and the shortcut table were opened and render, the others were not expanded in the app.

## Tests

- GoogleTest `DoricoPreferencesModel*`: every backed row round-trips its value. No duplicate row ids.
- Manual, run with the `ui-tester` agent: open the dialog, then check search, one change per page, Apply, and the value after a restart.
