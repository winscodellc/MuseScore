<!-- markdownlint-disable -->

# 100 - Preferences dialog

Status: todo - Depends on: 94, 60 - Dorico refs: `reference/manual/topics/preferences/preferences-dialog.md`, images `preferences_dialog.png`, `preferences_dialog_key_commands_page.png`

Ground truth: `reference/dorico-shots/prefs/notes.md`. It has the Dorico Pro 6 chrome, every category with every option label and control type, our 15 pages, and a Dorico → ours mapping table. Captured 2026-10-21.

## Goal

Cmd-, opens a Dorico-style Preferences dialog. It has Dorico's categories in Dorico's order, the same search and layout, Dorico's control style and Dorico's bottom buttons. Every Dorico option that MuseScore can back is wired to the MuseScore setting. The rest are shown disabled, labelled "Not available yet". MuseScore-only settings that matter (Audio & MIDI devices, Folders, Advanced, ...) stay reachable: they are folded into the nearest Dorico category, under a separator titled "MuseScore".

## Dorico behaviour

| Part | Dorico Pro 6 |
|---|---|
| Window | About 1130 x 708 pt, resizable, title "Preferences". |
| Left column | About 203 pt, flat black. The top has a **"Search categories..."** field (auto-focused, Cmd-L, blue 2 px focus ring). Below it is one list: **Colors** (4 sub-rows), **General** (General, Window, View, Files), **Key Commands**, **MusicXML** (Import, Export), **Note Input and Editing** (Note Input, Editing), **Play**, **Proofreading** (Proofreading, Categories), **Remote**, **VST Plug-ins**. Rows are 27 pt, text 13 px, no icons. The selected row is lighter, with a 3 px blue left bar. |
| Right pane | The page title band (lighter, 16 px). Section titles are dark full-width bands (bold 13 px). Each section is a **rounded dark card** (radius about 12, `#16191c` on pane `#1e2125`) holding label + control rows, with italic grey help text and blue "?" help icons. A group (for example General) scrolls as one page, and its sub-rows are anchors. |
| Controls | Grey dropdowns with a chevron. **Segmented toggles**: the selected one is bright blue `#2da0f5` with black bold text. Square checkboxes with a blue tick. Spinners. "Choose..." buttons. Colour rows: hex field + swatch + reset. |
| Page search | **Cmd-F** opens a "Search page..." bar with previous and next buttons, a match count and a close X. |
| Bottom bar | **Reset to Factory** (left), **Apply** (disabled until something changes), **Close**. There is no OK or Cancel. |
| Colours | Header band `#2b2f35`, accent `#2da0f5`, text white-grey. |

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
   - Apply semantics: edits are staged; **Apply** commits them; **Close** asks nothing and discards anything not yet applied (verify in Dorico whether Close discards or keeps staged changes, and match). **Reset to Factory** resets the shown page, after confirmation (verify the scope in Dorico).
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

All are shown disabled:
- Proofreading (spec 79);
- Remote;
- VST Plug-ins (MuseScore's VST scanning may back part of it; check);
- MIDI recording options (77);
- the Note Input "Lock Duration" / "Force Duration" behaviours, if not backed by spec 11.

## Acceptance criteria

- [ ] Cmd-, opens the new dialog at about 1130 x 708. It is resizable, and its categories and sub-rows are in Dorico's order.
- [ ] Category search (Cmd-L) filters the list. Cmd-F finds labels on the page, with a working previous / next and count.
- [ ] Every page shows Dorico's sections and options in order (diff against `prefs/notes.md`). Backed options read and write the real MuseScore setting (restart persists it). Unbacked ones are disabled, with a tooltip.
- [ ] Apply, Close and Reset to Factory behave as verified in Dorico. Escape closes the dialog.
- [ ] Side-by-side crops (category list, one card, one segmented control, bottom bar) match the Dorico values in the table above, within 2 pt and ±4 per channel.
- [ ] No MuseScore setting that ours exposes today becomes unreachable (checklist from `prefs/notes.md`, "Ours").

## Tests

- GoogleTest `DoricoPreferencesModel*`: every backed row round-trips its value. No duplicate row ids.
- Manual, run with the `ui-tester` agent: open the dialog, then check search, one change per page, Apply, and the value after a restart.
