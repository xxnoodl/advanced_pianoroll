# Verification results - Piano Roll 0.6.0

Built and checked on 2026-10-24 on `feat/live-clip-bridge`. All nine CTest suites pass. The compiled VST3 passes the independent 24-configuration host probe; the existing six actual FL comparison records and five Python MIDI-comparator tests still pass.

New tests exercise empty Session/Arrangement drafts, no creation merely from selecting a track, overlap refusal, first-edit clip creation, original-target binding after selection changes, note-ID mappings, local edits retained across asynchronous acknowledgements, order-independent dirty checks, automatic editor save, queued playback after clip creation, transport commands, and balanced audition events. The mocked Live workflow checks routing restoration, user routing changes, reference-counted held notes, target-switch release, watchdog release, record-arm preservation and ambiguous-name rejection. The routing dictionary wrapper matches the Max Live API's property/dictionary/list format; native verification is still required. Inspected the clip-mode preview.

The user confirmed that 0.5.0 opens from the companion after resolving script paths. That proves native launch, not the new 0.6.0 workflow. Empty-track creation, actual automatic note writes, native transport, playhead feedback and audible synth audition remain pending the fresh Live pass. Typing auditions notes; it does not record into the score. Per-note expression editing and native multi-call Undo integration are not complete.

VST3 SHA256: `B6EF4F2317A394C3E01935DEA24F471508971913975F01FD9DEFECAAA73B5C96`.

Clip editor SHA256: `B3090213AEB286F590F165C02C8E392C81CFFCBA9785E6A65D6ED458A62B4639`.

Historical results follow.

---

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

# Verification results - Piano Roll 0.3.1

Verified on 2026-10-24. The current subject is the compiled 0.3.1 VST3 loaded by a separate host, not a processor object linked into the test executable. The earlier 0.3.0 DLL was also tested after extraction from its delivered ZIP. Version 0.3.1 corrects a quantization difference found against a real FL export.

Current tested DLL SHA-256: `3E281A4222A716A483729C8B7A57C207DB415E4FB494F6C5DDF70EB56F24185C`.

## Passed

| Check | Result |
| --- | --- |
| VST3 scan and instance creation | One Piano Roll 0.3.1 instrument; stereo output, MIDI input/output. |
| Host configuration matrix | 24/24: 44.1/48/96 kHz Ãƒâ€” 64/257/1024/8192 samples Ãƒâ€” real-time/offline flags. |
| Note output | Exact sample offsets, channels, onset/off pairs, and loop crossing; no unexpected duplicate note events in the fixture. |
| MIDI passthrough | Input note/channel/sample position preserved through the DLL. |
| Preview audio | Nonzero finite output within the tested amplitude bounds. |
| Transport stop | Release messages on all 16 MIDI channels. |
| Tempo change | Held-note duration follows a continuous-PPQ change from 120 to 240 BPM. |
| Seek | Release messages plus one chased held note at sample zero. |
| State | Recall into a fresh DLL instance, monitor-off MIDI output, and independent instance settings. |
| Editor factory | Three creation/destruction cycles through the VST3 wrapper. |
| Existing CTest suites | 3/3: sequencing, editing, processor/editor integration. |
| MIDI comparator tests | 5/5: known running-status input, exact match, and detection of pitch/count/timing/channel/velocity differences. |
| Prototype fixture processing | 5/5: actual import Ã¢â€ â€™ shortcut command Ã¢â€ â€™ export matched the expected quantize/chop/glue/legato fixtures. |

Detailed machine-readable results: `binary-host-report.json`, `verification-manifest.json`, and `fl-fixtures/*.prototype-comparison.json`. The host test source is `../tests/VST3HostProbe.cpp`; it uses JUCE hosting but does not link this project's processor/editor code.

## FL quick-chop comparison - passed on the supplied fixture

The user exported the same input from FL twice and confirmed the snap labels:

| FL file | Confirmed snap | Notes | Comparison with prototype import/command/export |
| --- | --- | --- | --- |
| `quick-chop.FL.mid` | 1/4 step = 1/16 beat | 47 | Exact normalized note-data match. |
| `quick-chop.FL2.mid` | 1/4 beat | 13 | Exact normalized note-data match. |

Pitch, onset, duration, channel, and velocity all match after converting the 960-PPQ prototype and 96-PPQ FL exports to common beat units. No tolerance was needed for the normalized note-data equality check. Partial final slices are preserved in both outputs. See `fl-fixtures/quick-chop.verification-summary.json` for file hashes and `confirmed-FL-settings.json` for the user-confirmed snap settings.

The initial 13-versus-47 mismatch compared different snap settings; it was not a demonstrated chopping-algorithm defect. That diagnostic report is retained with a superseded/different-settings annotation. Current like-for-like reports are `quick-chop.FL.mid.comparison.json` and `quick-chop.FL2.mid.comparison.json`.

Scope: one off-grid, mixed-duration fixture at two snap values. Version 0.3.1 now exposes 1/4 step and 1/4 beat in the snap dropdown. UI integration tests select each label and invoke Ctrl+U, obtaining the same 47/13 note counts. This does not establish complete snap-menu parity or all chop behavior.

## FL quick-quantize comparison - corrected and passed in 0.3.1

The actual `quick-quantize.FL.mid` exposed a difference in 0.3.0: the prototype rounded note duration, whereas FL's result snapped original note endpoints. For the first note, FL produces onset 0.25, end 0.75, duration 0.5; the earlier prototype produced duration 0.75.

The endpoint-enabled quantizer was corrected, leaving start-only quantizing's duration-preservation behavior intact. All four notes from the prototype's actual import/Ctrl+Q/export path now match FL exactly after PPQ normalization, including pitch, channel, velocity, onset, and duration. Source/regression tests and the real snap-selector/Ctrl+Q UI path cover the first-note discrepancy.

See `fl-fixtures/quick-quantize.verification-summary.json` and `quick-quantize.observation.json`. Pre-fix MIDI and the failed comparison are retained under `.before-0.3.1` suffixes. The full compiled-DLL matrix and all automated suites pass again on 0.3.1. Matching is established for this fixture and settings; advanced modes, ties, and very short collapsed notes require further FL checks.

## FL start-only quantize comparison - passed

The supplied `quantize-starts.FL.mid` matches the 0.3.1 prototype's actual Shift+Q export exactly in normalized note data: all four pitches, channels, velocities, start times, and durations. Durations also match the original input unchanged, independently confirming start-only behavior on this fixture. See `fl-fixtures/quantize-starts.verification-summary.json` for the reference hash and PPQ details.

## FL glue comparison - passed

The supplied `glue.FL.mid` matches the prototype's Ctrl+G export exactly after PPQ normalization. Both produce two notes: the three contiguous C notes merge into one beat-long note retaining the first note's velocity (80); the separate E note remains unchanged. See `fl-fixtures/glue.verification-summary.json` for the note data and reference hash. This establishes the touching-note case on this fixture, not all overlapping/group/channel edge cases.

## FL legato comparison - passed

The supplied `legato.FL.mid` matches the prototype's Ctrl+L export exactly after PPQ normalization: all four notes have matching pitch, onset, duration, velocity, and channel. The first three extend to the following onset and the final note retains its fixture endpoint. See `fl-fixtures/legato.verification-summary.json` for the note data and reference hash.

## Five-command reference set - complete

All five supplied command fixtures now match actual FL exports in normalized note data: quick chop (at two snap resolutions), quick quantize, start-only quantize, glue, and legato. The quick-quantize difference discovered in 0.3.0 was corrected in 0.3.1. These results establish agreement for these fixtures/settings, not every edge case or complete FL piano-roll parity.

## Still pending

- Additional FL edge-case coverage: partial selections, overlaps, polyphonic legato, quantizer ties/collapsed notes, channel boundaries, and advanced processing tools.
- Ableton's actual scanner, transport, session recall, MIDI routing, and audible hardware output.
- Visible native editor attachment, window focus, mouse interactions, host resizing, and modal tool behavior inside a DAW.
- Real-time deadline/latency/stress measurement. The host probe renders offline even when exercising the real-time processing flag.
- A third-party validator such as pluginval. Its download was unavailable from this session; no pluginval result is claimed.
- Full FL preset/algorithm and visual/interaction parity.

The custom host and plugin both use JUCE, so these tests provide useful DLL/protocol coverage but cannot substitute for cross-vendor host testing.

## Next integration stage

The initial FL command comparison set is complete. The next distinct check is loading the delivered VST3 in Ableton, confirming audible preview and generated-MIDI routing, then saving/reopening the session. Steps are in this folder's `README.md` under Ableton Live check.

Further FL behavior comparisons should use additional fixtures targeting the specific edge cases above.
