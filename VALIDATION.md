# Verification results - Piano Roll 0.5.0

Native load follow-up: Live Log.txt at 22:40:59 reports `js: can't find file bridge-max.js`, followed by missing functions. The companion installer now resolves both Max/Node entry paths and the protocol include path for its actual destination. Two device-path tests (including spaces), adapter include-path checks and the transport tests pass. The install can relink a moved packaged companion. A fresh native load is still required to confirm the path fix.

Launcher startup fix: Max imports the Node entry point, so it must initialize without relying on `require.main` or an environment flag. The entry point is now unconditional, the test imports it the same way, and launcher sessions/exits are logged to `runtime.log`. Transport and adapter tests pass after this fix; the staged companion was updated. Native launch confirmation remains pending.

Built and checked on 2026-10-24 on `feat/live-clip-bridge`. Seven CTest suites pass: sequencing, editing, processor integration, clip protocol, C++ Live bridge, mocked Max API adapter, and local bridge transport. The compiled VST3 also passes the separate 24-case host matrix and existing tempo/seek/MIDI/state checks. All six supplied FL comparison records and five Python comparator tests continue to pass.

The clip bridge tests exercise source note IDs, absolute/loop-relative timing, preserved outside/boundary notes, fractional velocity conversion, dirty-editor retention, stale/invalid sessions, rejected and accepted acknowledgements, and disabled duplicate preview/sequencer output. UI tests render the target clip and check Apply/undo enablement. Inspected `build/pianoroll-live-bridge-preview.png`. The staged GUI executable and scripts match the built/source files.

Native Live integration is pending: structural AMXD checks and a mocked Live API do not prove that Max loads the generated device or accepts its note API calls. The next manual test is device load, Open editor, clip selection and a small Apply. Typing audition, Space-to-Live transport, automatic writes and atomic multi-call undo are not implemented in this first clip mode. The normal VST3 remains independent.

VST3 SHA256: `91816F34460F46209D49D4575F8D44AA08D3FB4399D732DE727D75DFD4E543B4`.

Clip editor SHA256: `1D7B0B8DA17A5B2C85B6507301E44B19D312DF92AB2C3CD614B499B5D15B3F5C`.

Historical results follow.

---

# Verification results - Piano Roll 0.4.2

Built and verified on 2026-10-24. All three CTest suites pass. The compiled VST3 passes the separate 24-case audio host matrix, MIDI, tempo/seek, state and editor lifecycle checks. All six supplied FL comparison records and five Python comparator tests continue to pass.

New regression coverage: Alt+wheel up reduces pitch row height; Alt+wheel down increases it. Keyboard/grid/unselected-note zoom preserves note data. Selected-note velocity edits, fine mode, clamping and undo retain their previous behavior. Actual processor host tempo reporting is tested separately from local tempo. UI tests verify editable local tempo, read-only host tempo, and restoration of local BPM when sync is disabled. The MIDI routing help icon is exposed. Default/minimum window layout at 100/125/150% UI size and direct rendering at 250% device scale remain covered. Inspected the new editor preview.

The user reports 0.4.1 graphics look quite okay in Live. Live playback of another synth through plugin MIDI output and recording into a Live clip still need manual verification. The routing help and SYNTH-ROUTING.md describe this supported host routing workflow; no automatic track creation or native clip integration is claimed. The proposed optional Max for Live companion is not implemented.

Tested/staged DLL SHA256: `7AC51A9B9566A6481185DFDCB1C453BE4A2FD76E5A807FE8DA8D307C6E4D3DDC`.

Historical results follow.

---

# Verification results - Piano Roll 0.4.1

Release VST3 and standalone built on 2026-10-24. Three CTest suites pass, including the new icon tool selection callbacks and accessible names/tooltips. The final compiled VST3 passes the 24-case host matrix, tempo/seek, MIDI, state and editor lifecycle checks. All six existing FL comparison records and all five Python comparator tests pass.

Inspected the new compact icon toolbar at 1x and directly rendered 250% device scale; 100/125/150% UI sizing bounds remain checked at default and minimum window sizes. User's screenshot demonstrates 0.4.0 native Live output was softer than the FL reference; the user confirmed Live's Auto-Scale Plugin Window was enabled. The effect of disabling it and 0.4.1 native output are still pending user verification. The new runtime display report records peer/component/desktop scale only when attached to a native window. No claim that native blur is resolved is made.

Final tested/staged DLL SHA256: `A028F07FA538904C4D29C90FCA9A2D451B3DCFE7594FF88B0000B6543FC4DBBE`.

Historical results follow.

---

# Verification results - Piano Roll 0.4.0

Verified on 2026-10-24. Release VST3 and standalone built successfully. All three CTest suites pass; the final compiled VST3 passes the separate 24-case host matrix, tempo/seek, MIDI, state, and editor lifecycle checks. All six supplied FL comparison records still pass (five commands, two chop resolutions). Five Python comparator tests pass.

New tests cover all 32 semitones of the default typing layout, both key rows through actual processor MIDI output, selected MIDI channel, auto-repeat suppression, overlapping keys, octave offsets, focus-loss/mode-disable release, and restored editor shortcuts. Wheel tests cover selected velocity edits, clamping, undo, fine edits, and keyboard/empty-grid/unselected-note zoom without note mutation.

Inspected editor previews at 100% and directly rendered 250% device scale. UI sizes 100/125/150% are checked for control bounds at default and minimum window size. These checks do not establish native host DPI behavior or OS keyboard-event delivery. Live 0.3.1 loading/playback was user-confirmed earlier; 0.4.0 is staged separately for a fresh live-window test. No pluginval claim is made.

Final tested/staged DLL SHA256: `C315D17C95B4D6FBA6B3A2D0A1FE7CFAFC29CCBD645118CDEE018B0BA21352E6`.

Typing preferences and UI size are currently editor-local; internal pattern recording is not implemented. The QWERTY layout is based on the installed Piano layout key/note mapping, with independently written handling. No FL code or preset files are distributed.

Historical 0.3.1 results follow.

---

# Validation - 2026-10-24 (0.3.1)

Environment: Windows x64, Visual Studio Build Tools 2026, MSVC 19.51.36244, CMake 4.2, JUCE 8.0.12.

## Successful checks

- `build.ps1` configured and built the Release VST3 and standalone targets.
- JUCE's VST3 manifest helper loaded the plugin and generated its factory/component metadata. This verifies wrapper loading for metadata extraction; it is not a DAW compatibility test.
- CTest: 3/3 suites passed on the latest 0.3.1 source.
- Sequencing suite: block-start onset, next-block note-off, loop crossing, off-before-on retrigger, mute exclusion, quantization, negative-time wrapping, input validation, capacity bounds, and timing invariance across 257-sample partitions at 60/123/300 BPM.
- Processor/editor suite: generated MIDI channel/sample offsets, audible preview signal, channel release on stop, passthrough MIDI with preview muted, state/settings recall, pattern-file round trip and corrupt-file rejection, MIDI note round trip, host-position note chase and stop release, editor quantize, undo/redo, duplication, selected mute, and offscreen editor painting/PNG output.
- Inspected the offscreen editor image for toolbar, grid, piano keys, notes, bar numbers, and velocity lane layout.
- Baseline 0.1.0 standalone hidden-process smoke check: process stayed alive and reported Responding=true. WaitForInputIdle returned false and no visible main-window title was obtained; this check does not establish completed device initialization or interactive usability. The 0.2.0 live standalone window has not been manually exercised.
- Inspected standalone PE dependencies. They include Windows and Microsoft Visual C++ runtime libraries; no FL Studio DLL dependencies are present.
- PowerShell build/package scripts parsed successfully.

## Recording-driven 0.2.0 checks

- Rebuilt both Windows x64 targets following visual/navigation changes based on the user's recording.
- Both automated test suites pass with the revised editor and processor.
- Added tests for active-key feedback at onset/release, transport indication, snapped forward/backward ruler ranges, independent note/time selection, ruler-range clearing, and velocity-lane resizing without modifying the score.
- Rendered normal and selected editor previews with the software renderer; the normal preview was visually compared against the reference's grid density, keyboard labels, overview, note shape, and velocity lane. Selected-preview inspection is recorded separately in the work session.
- The ruler range remains visual/editor-local; playback looping and tool scope are not claimed as matched.

## Not established

No real DAW scan, host MIDI routing, hardware audio output, live mouse interaction, offline-render comparison, or long-running real-time stress test has been completed. There has been visual comparison against the user's recorded FL session, but no controlled live side-by-side behavior test. Advanced FL-specific functionality is listed as unimplemented in README.md.

The binary/source package is intended for first-round personal testing. The task's near-identical interaction/feature goal remains broader than this initial implementation.

## Stamp and processing 0.3.0 checks

- Release VST3 and standalone targets build successfully. The latest full CTest run passed all three suites in 0.75 seconds.
- Added the dependency-free editing suite: start-only/full quantize and selection isolation, chop remainder/properties/selection, chop/glue round trip, channel isolation, atomic capacity rejection, stamp pitches/selection/overflow rejection, catalog interval checks, seeded velocity randomization, scaling bounds, LFO phase/cycle values, and strum ordering/slope.
- Integration tests exercise actual mouse-based one-shot Major stamping and undo, Ctrl+U quick chop, Ctrl+G glue, non-compounding preview changes, Cancel restoration, Accept followed by one-step Undo, external-state replacement protection, and closing an LFO panel without committing.
- Added a regression test verifying that a very short note can be dragged without accidentally resizing it.
- Rendered and inspected the updated editor and LFO controls. Native popup placement, dialog keyboard handling, and all host-specific window behavior still require manual testing.
- Menu/shortcut references are the user's screenshots and official Image-Line documentation, linked in README.md. Algorithms and interval voicings are original implementations; no native FL processing code was recovered.

## Subsequent delivered-binary verification

The actual DLL from the delivered 0.3.0 ZIP passed 24 configurations in a separate VST3-host executable, plus continuous-PPQ tempo-change, seek/chase, state recall/isolation, and editor-factory checks. Five prototype MIDI fixture comparisons and five comparator tests also passed. See `verification/RESULTS.md` and its linked JSON records for the tested binary hash and coverage boundaries. Actual FL exports and an Ableton session test are still pending; custom-host success is not full FL/Ableton verification.

Subsequently, actual user-generated FL quick-chop exports matched the prototype's import/chop/export path exactly in normalized note data at 1/4-step and 1/4-beat snap (47 and 13 notes respectively). Other FL commands and the Ableton session test remain pending. Details and user-confirmed settings are in `verification/RESULTS.md`.

The next actual FL quick-quantize export exposed a 0.3.0 defect: duration rounding differed from FL's endpoint snapping. Corrected in 0.3.1 and verified against all four exported notes with exact normalized note-data equality. Added endpoint/partial-strength regression tests and UI tests exercising the labelled snap selector and Ctrl+Q/Ctrl+U paths. The 24-case DLL probe, fixture comparisons, comparator tests, and three CTest suites all pass on 0.3.1. No actual Ableton-session or full FL-parity claim is made.

The user subsequently supplied start-only quantize, glue, and legato exports. All match exactly after PPQ normalization. The initial five-command FL fixture set is now complete (quick chop at two resolutions); further edge-case, advanced-tool, UI, and real-host testing remains outstanding.
