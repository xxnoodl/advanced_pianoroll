# Piano Roll

An independent C++ piano-roll sequencer targeting Windows x64 VST3 and standalone, with the editor and sequencing core shared between both. This is an initial implementation, not a verified near-identical FL Studio replacement. No FL Studio binaries, artwork, samples, presets, or recovered implementation code are included.

Version 0.2.0 uses the user's actual FL Studio screen recording for its first visual/navigation refinement. See `VIDEO_REFERENCE.md` for timestamped observations and the remaining behavior questions. This recording analysis uses ordinary media tools; it is separate from the limited REA binary/resource inspection.

Version 0.3.0 adds chord/scale stamping and a working Tools menu. These are independent implementations informed by the menu screenshots and documented behavior. Detailed FL algorithm/preset parity has not been established.

Version 0.3.1 fixes a mismatch found against the user's actual FL export: Ctrl+Q now snaps original note starts and ends, rather than rounding durations. Shift+Q retains the original durations. The snap dropdown now includes explicitly labelled 1/4-beat and 1/4-step settings, including 64th-note resolution.

Version 0.4.0 adds the default FL-style QWERTY typing piano, context-sensitive Alt+wheel, larger grid labels and keys, flat vector controls, a wrapping toolbar, and independent 100/125/150% UI sizing. The editor follows host DPI scaling; the UI sizing menu changes readability without overriding the host's scale factor.

Typing piano: click the button or focus the roll and press Ctrl+T. Default lower white keys are `Z X C V B N M , . /`; upper white keys are `Q W E R T Y U I O P [ ]`. Black keys are `S D G H J L ;` and `2 3 5 6 7 9 0 =`. This covers MIDI 48 through 79, with overlapping assignments as in the installed default Piano layout. Z is labelled C4 and Q C5 in FL's octave convention. Octave and velocity menus apply to typing; MIDI uses the selected channel. Key repeats are ignored, overlapping assignments share a held note, and notes release on focus loss, mode change, channel/octave change, or editor close. Letter tool shortcuts work with typing mode off. This is focused-editor input, not a global keyboard hook; the host must pass keys to the plugin. Non-QWERTY physical layouts have not been verified. Typing performs/routes MIDI and does not yet record notes into the internal pattern. UI sizing and typing preferences currently reset when the editor is recreated.

Alt+wheel over a selected note adjusts all selected velocities (four MIDI units per wheel event); Ctrl+Alt+wheel makes one-unit adjustments. Over an unselected note, empty grid, or keyboard it zooms vertically. This is the user's requested selection rule, rather than a claim of exact FL wheel parity. Velocity changes support undo. Ctrl+wheel zooms horizontally and Shift+wheel scrolls time.

For a blurry window in Live, compare the standalone and check Live's per-plugin **Auto-Scale Plugin Window** setting in the Browser. Freely scalable VST3 editors can be sharper with that option disabled. The actual 4K host result still needs visual confirmation; offscreen device-scale previews do not prove native host DPI behavior.

Version 0.4.1 replaces action labels and the tool selector with original vector icons, compact controls, hover tooltips and accessible names. Snap, channel, tempo and other numeric/musical values remain readable text. The oversized heading is removed and the toolbar is reduced to two compact rows at the default size. Grid labels use explicit regular Segoe UI fonts. UI actions, typing and wheel rules remain as in 0.4.0.

A one-time runtime display report is written when the editor is attached to a real window at `%APPDATA%/IndependentAudio/PianoRoll/display-diagnostics.json`. It records editor dimensions and the component/peer/desktop scale factors, to distinguish native rendering from host autoscaling. It contains no session or MIDI content. The screenshot comparison showed a soft Live window; the user confirmed Auto-Scale Plugin Window was enabled. Disabling that per-plugin option is the next native rendering check, not yet a verified resolution.

Version 0.4.2 refines the tempo field and dropdown/menu styling, adds a MIDI-routing help icon, and reverses only Alt+wheel vertical zoom: wheel up shrinks rows; wheel down enlarges them. Selected-note velocity edits keep the existing direction. In Host sync the BPM field displays the host's reported tempo and is read-only; disabling sync restores the stored local BPM. Active tempo text edits are protected from timer updates. See `host-test/SYNTH-ROUTING.md` for Live routing/recording and the distinction between a VST3 score editor and a future Live clip companion.

## Current status

- Windows x64 VST3 and standalone binaries compiled successfully with MSVC and JUCE 8.0.12.
- Three CTest suites pass: sequencing, editing, and processor/editor integration.
- Tests exercise preview audio, generated/passthrough MIDI, sample timing, host seek/stop handling, state recall, MIDI/pattern files, quantize, undo/redo, duplicate, and mute.
- The editor was rendered offscreen, inspected, and included as `build/pianoroll-preview.png`.
- The baseline 0.1.0 hidden standalone process launch remained running and responsive; audio-device initialization and visible-window interaction were not verified by that check. Version 0.2.0 still requires live-window testing.
- Version 0.3.1 loading and playback in Ableton were user-confirmed. Version 0.4.0 graphics, focused typing, live mouse interaction, MIDI routing, and recall still need the new manual host pass. This is not complete FL Studio feature parity.
- Version 0.3.1 passed a separate binary host probe across 24 configurations, including tempo/seek and state checks. Quick chop and quick quantize now match supplied actual FL exports on their fixtures. See `verification/RESULTS.md`; other FL command comparisons and an Ableton session test remain pending.

## Build

Requires Visual Studio C++ Build Tools, a Windows SDK, CMake, and JUCE 8.0.12. The installed Visual Studio tools on this PC include a suitable compiler and CMake. Build with:

```powershell
cd "$env:USERPROFILE\Documents\PianoRollVST3"
.\build.ps1
```

CMake downloads the pinned JUCE version when no local copy is available. For an offline agent session, run this outside Codex first:

```powershell
git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git "$env:USERPROFILE\Documents\PianoRollVST3\vendor\JUCE"
```

Alternatively pass an existing JUCE checkout:

```powershell
.\build.ps1 -JucePath 'D:\Libraries\JUCE'
```

Core-only tests need no download:

```powershell
.\build.ps1 -CoreOnly
```

Expected artifacts after a successful full build:

- `build/PianoRoll_artefacts/Release/Standalone/Piano Roll.exe`
- `build/PianoRoll_artefacts/Release/VST3/Piano Roll.vst3` (copy the entire bundle when installing)

The build does not install plugins or change DAW configuration. JUCE's plugin build API is documented at https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md . Dependency licensing is recorded in JUCE's own LICENSE.md in its checkout.

Create a transferable ZIP containing the standalone app, complete VST3 bundle, and source project with `./package.ps1`. Extract it before running. The build targets Windows x64 and links the Microsoft Visual C++ runtime dynamically. Add the extracted VST3 bundle to an appropriate VST3 scan location in your host; it is not installed automatically.

## Use

Open the standalone app to edit and audition without a DAW. In the VST3, turn on **Host sync** to follow the host transport and PPQ timing, or leave it off for the plugin's local Play/Stop and BPM controls. The preview synth produces audio for quick auditioning. Turn it off when routing the generated MIDI to another instrument. MIDI output availability and routing depend on the host; host integration has not been tested yet. The standalone app's JUCE audio/MIDI settings provide device selection.

The plugin saves notes, loop length, local tempo, sync mode, and preview setting in host session state. Save/Open also writes independent `.pianoroll` files. Standard PPQ `.mid` files import/export note timing, pitch, channel, and velocity. Import merges tracks and rejects over-limit files without modifying the pattern. Tempo maps and imported time-signature changes are not applied; this version uses a 4/4 grid. Muted notes are omitted from MIDI export.

## Editing

- Draw: click to create, drag to move, drag the right edge to resize.
- Right-click and drag to erase. Shift-drag existing notes to duplicate. Alt bypasses snapping.
- Select tool or Ctrl-drag creates a selection box. Shift adds to selection.
- Paint adds notes along a drag. Mute toggles a note. Slice splits at the clicked snapped position.
- Drag velocity stems; selected notes change together when a selected stem is edited.
- Wheel scrolls pitch. Shift-wheel scrolls time. Ctrl-wheel zooms time. Fit frames the pattern.
- Ctrl+Alt+wheel zooms pitch rows. The scrollbars navigate pitch/time, and the top note overview navigates the visible time window.
- Drag the divider above the velocity lane to resize it.
- Drag the ruler below the overview to create a separate time range; right-click the ruler to clear it. This range is currently visual/editor-local and does not change playback-loop bounds or tool scope.
- Ctrl+A/C/X/V selects/copies/cuts/pastes. Paste starts at the current playhead (zero when stopped).
- Ctrl+B duplicates the selected span immediately after its end, provided it fits in the loop.
- Ctrl+Z/Y undo/redo. Arrow keys move selected notes. Ctrl+Up/Down moves an octave. Ctrl+D deselects notes.
- Ctrl+Q quantizes starts and ends; Shift+Q quantizes starts while preserving duration. Ctrl+L extends notes to the next onset in the same channel.
- Ctrl+U chops notes at the current snap interval, retaining any final partial slice. Ctrl+G glues touching notes with matching pitch/channel/mute state. The first note's other properties are retained.
- Alt+M mutes selected notes; Alt+Shift+M unmutes them. Delete removes selected notes. Space controls local playback.
- P/B/C/E/T chooses Draw/Paint/Slice/Select/Mute. Shift+N disables the stamp preset.
- Channel sets the channel and colour for new notes; existing channels remain visible.
- Major/minor scale shading is visual guidance. It does not change pitches.

Initial limits: 2048 notes, one pattern per plugin instance, 1–32 bars in 4/4, 16 MIDI channels. Undo stores 64 editing steps within the open editor; loading state or changing the loop clears editor history. Loop shortening is rejected when it would cut off existing notes.

## Stamps and processing tools

**Stamp** offers basic chords, advanced chord voicings, and common scales. Choose a preset, then click a blank grid position to place its root. The inserted notes are selected together and can be moved/resized together. **Only one** returns to ordinary note entry after one successful stamp; turn it off for repeated placement. A stamp exceeding the MIDI pitch range or note capacity is rejected as a whole. The catalog uses explicit root-position semitone patterns; it does not copy or decode FL score presets. Automatic chord, exotic scale presets, percussion stamps, and Slide up are not included yet.

**Tools** contains quick commands plus six adjustable processing panels:

| Panel | Shortcut | Controls and behavior |
| --- | --- | --- |
| Quantize | Alt+Q | Step in beats, strength, optional note-end quantization. |
| Chop | Alt+U | Slice length in beats and gate percentage. |
| Strum | Alt+S | Spread in beats, velocity slope, low/high-note-first ordering for simultaneous notes in each channel. |
| Randomize velocity | Alt+R | Bounded velocity variation and a reproducible seed. |
| Scale velocity levels | Alt+X | Velocity gain and offset, clamped to 1–127. |
| Velocity LFO | Alt+O | Centre, amplitude, cycles across the selected note span, phase, and sine/triangle/saw/square shapes. |

Tools affect selected notes, or all notes if none are selected. Adjustments preview from the original score, so repeated slider changes do not compound. **Accept** records one undo step; **Cancel**, Escape, or closing the tool restores the original. Replacing the pattern externally invalidates the preview rather than allowing Cancel to overwrite the new score. Current live-preview values are audible and are present in the processor while previewing; finish the tool before saving a host session.

When snap is off, quick quantize/chop use a quarter-beat fallback. Time-range shading does not define processing scope yet. These panels expose the stated controls; they are not replicas of every option in FL's corresponding dialogs. In particular, Randomize/Scale/LFO currently process velocity only, and the quantizer does not load FL groove templates.

Reference sources: the user's screenshots/recordings documented in `VIDEO_REFERENCE.md`, [Image-Line's piano-roll menu documentation](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/pianoroll_menu.htm), and [quantizer documentation](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/pianoroll_qnt.htm). The screenshots confirm the displayed shortcuts; documentation clarifies snap-based chopping and one-shot stamping.

## Architecture

`Source/Core.h` contains a dependency-free fixed-capacity note model and sample-position MIDI scheduling. `Processor.*` integrates host/local transport, generated MIDI, preview audio, state persistence, and MIDI files. `Editor.*` contains the custom-drawn grid, tools, gestures, and editing history. `tests/CoreTests.cpp` verifies boundary timing, loop/retrigger order, muted notes, input bounds, capacity, quantization, and timing invariance across different block partitions and tempos.

The audio callback tries the pattern lock rather than waiting for the UI; when an edit is in progress it keeps the previous snapshot. Pattern changes, stop, and timeline jumps issue note-offs; active notes are chased on start/seek. The MIDI collector and buffer handling still need real-time profiling under dense editing and host stress.

## Manual validation still required

1. Open the standalone, select an audio device, play the demonstration pattern, and exercise every gesture and shortcut.
2. Scan the VST3 in available hosts. Confirm stereo audio, MIDI output routing, and local transport.
3. Enable Host sync; verify playback, stop, loop, seek, tempo changes, offline render, and no stuck notes in a real host.
4. Save/reopen a host session and compare notes and settings. File and in-memory round trips already pass automated tests.
5. Stress simultaneous UI edits, repeated same-pitch notes, maximum capacity, multiple instances, and editor open/close during playback.

Advanced FL features such as slide-note synthesis, portamento, per-note pan/filter/release/fine-pitch lanes, ghost-channel editing, full arpeggiation, automatic chords, percussion stamps, score markers, time-signature changes, and scripting remain unimplemented. Processing tools and chord/scale stamps now have independent implementations as described above; complete FL preset/algorithm parity remains unverified. See `INVESTIGATION.md` and `VIDEO_REFERENCE.md` for the evidence boundary.
