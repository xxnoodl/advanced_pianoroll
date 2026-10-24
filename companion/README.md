# Piano Roll Live

A first clip bridge for the shared piano-roll editor. Use one companion per Live Set. It follows the MIDI clip shown in Live's Detail View while the editor has no unapplied changes. Editing stays in the shared C++ core; Live plays the actual clip through the instrument already on that track.

## Try it

1. Drag **Piano Roll Live.amxd** from the installed companion folder onto an empty MIDI helper track in Live Suite.
2. Click **Open editor** on the device. The editor opens without initializing an audio device.
3. Select a MIDI clip on any synth track in Live. Its active loop appears in the floating editor; the clip name is shown in the toolbar.
4. Edit a note, then click the **Apply edits to Live clip** arrow icon. Play the clip in Live to hear the existing synth. There is no two-track MIDI routing for clip playback in this mode.
5. Select another clip to follow it. Unapplied changes stay in the editor. Return to the original clip to apply them, or use **Reload selected Live clip** to discard them and switch.

Keep the AMXD, JS files and bin folder together. The installer resolves script paths for the destination folder; run the installer again after moving the installation. The device is not frozen. Save your Live Set to retain the companion; unsent editor changes are not stored in that Set.

## This version

- Edits supported fields of existing notes by their Live note IDs, retaining other returned properties. Adds new notes and removes notes you deleted from the editable loop.
- Boundary-crossing notes and notes outside the active loop are left out of the editable score and preserved in Live.
- Refuses stale snapshots, another selected clip, missing connections, invalid notes, and overlapping Apply requests.
- Supports up to 2048 editable notes and a loop up to 128 beats. Loop resizing is not implemented. Shorter-than-one-bar loops use a minimum four-beat editor viewport; added notes must stay within the real clip loop.
- Automatic writes, live typing audition, editor Space-to-Live transport, and per-note expression editing are not connected in this first bridge. The normal VST3 typing/MIDI workflow is unchanged. Use Live's transport to hear clip playback.
- Existing advanced properties are retained by modifying existing IDs; new notes use Live's defaults. Native Live verification of expression retention remains pending.
- Apply can involve several Live API calls. This API does not expose a transaction covering those calls. A reported partial Apply must be inspected in Live and undone there if needed, then reloaded.

## Build

Run `build.ps1` at the project root, then `companion/install.ps1`. This stages the complete companion and GUI executable in `host-test/live-companion`. No npm packages are downloaded. Python builds the original Max patch/container; Node and CTest exercise the protocol, adapter and local file exchange. Runtime traffic stays in a unique per-device temporary directory.

## Validation

The device container is structurally checked against its generated patch. Tests cover real C++ file exchange, source IDs, loop offsets, conflict handling, acknowledgements and no duplicate preview/sequencer output. JavaScript adapter tests use a mocked Live API; they do not establish native Max parsing or API behavior. Loading the generated AMXD, launching its editor from Live, and applying actual clip edits require the first manual Live test.

API references: [clip notes](https://docs.cycling74.com/apiref/lom/clip/), [selected clip](https://docs.cycling74.com/apiref/lom/song_view/), [Node for Max](https://docs.cycling74.com/reference/node.script).
