# Piano Roll Live

One shared piano-roll window for the selected Live MIDI track. The companion stays on a MIDI helper track; Live plays the actual clip through the synth already on the target track. The editor does not open an audio device.

## Use it

1. Load **Piano Roll Live.amxd** on an empty MIDI helper track and click **Open editor**.
2. Select a MIDI instrument track in Live. You can select an existing clip or an empty Session slot. In Arrangement View, an empty area at Live's current timeline position becomes a draft.
3. Draw notes in our editor. The first completed edit creates the clip if needed. Subsequent edits sync automatically after you finish the gesture; there is no normal Apply step.
4. Press **Space** or the play icon inside our window. Unsaved notes are sent first, then Live plays the target clip. Press again to stop Live's transport. The editor shows Live's tempo and playhead.
5. Enable the keyboard icon or **Ctrl+T** for FL-style typing audition. Clicking notes or piano keys also sends audition MIDI through the selected synth.

Typing auditions notes; it does not record them into the score without a recording workflow. Draw notes to populate the clip in this version.

## Synth audition

The helper automatically routes its MIDI output to the selected track. Give synth tracks distinct names and leave their normal MIDI monitoring on Auto or In. The companion uses the target's temporary implicit arm state for audition and restores it after note release. It does not change explicit recording-arm buttons. While Live is recording, an otherwise unarmed target must be armed by you before auditioning, to avoid starting unintended recording.

Held notes release on target changes, editor exit, or a missing-editor watchdog timeout. The helper's previous routing is restored when the editor closes or the target changes; a routing choice you changed yourself is retained. A companion placed before an instrument on that same track can audition directly, but a separate helper track supports following any synth.

## Saving and conflicts

Edits are sent after a short idle period (180 ms after the latest change and after finishing a gesture). You can keep editing while a save is pending; newer edits are retained and sent next. A save that began on one clip stays bound to that clip if you select another track. It never substitutes the newly selected clip as its destination.

If Live's notes changed externally, sync pauses and retains your local edits. A retry icon appears; Reload lets you discard local changes and load Live's current version. Existing notes are updated by ID. Notes outside or crossing the active loop are preserved outside the editable score. New notes use Live's defaults for properties our editor does not expose. Per-note expression editing, loop resizing, and full native clip Undo integration remain outside this version.

Session drafts use the chosen empty slot (or the first empty slot on the selected track). Arrangement drafts begin at Live's current timeline position, extend up to four bars, and stop before the next existing clip. Selecting a track alone does not create a clip. Writing notes or explicitly pressing Play materializes a draft. Existing clips are never overwritten to make a draft.

## Install or move

From the source project, run `build.ps1`, then `companion/install.ps1`. The complete local install is staged under `host-test/live-companion-0.6.0`. Keep the AMXD, scripts and bin folder together. When moving a packaged companion, run its `install.ps1` once to resolve script paths for the new folder. No npm download is needed.

Use one companion per Set and save your Live Set normally. Close an old editor window and remove its old device before loading another version.

## Validation

Unit/integration tests cover drafts, note-ID reconciliation, concurrent edits during saves, original-target binding, MIDI event ordering, routing restoration, held-note release, transport commands, conflicts, and installed paths. The Live API tests are mocked; they do not prove that the audible audition and native API behavior work on a particular host. The user confirmed native launch of 0.5.0; the 0.6.0 authoring workflow needs a fresh Live pass.

References: [clip notes](https://docs.cycling74.com/apiref/lom/clip/), [clip creation](https://docs.cycling74.com/apiref/lom/clipslot/), [track routing](https://docs.cycling74.com/apiref/lom/track/).
