# Verification

This folder separates binary integration checks from FL Studio behavior comparisons. A passing custom-host test does not establish Ableton compatibility or FL feature parity.

## Compiled VST3 host probe

`tests/VST3HostProbe.cpp` is an independent executable that loads the actual VST3 bundle. It links JUCE's host implementation, not this project's processor/editor objects. It therefore exercises the DLL, VST3 wrapper, event buses, state streams, and editor factory. Both plugin and host use JUCE, so this is not an independent cross-framework validator.

Build the probe using the existing local JUCE checkout:

```powershell
cmake -S . -B build -DPIANOROLL_VERIFY_BINARY=ON -DJUCE_SOURCE_DIR=vendor/JUCE
cmake --build build --config Release --target PianoRollHostProbe pianoroll_integration_tests --parallel 4
```

Then run `./verification/run-checks.ps1 -BundlePath 'FULL PATH TO Piano Roll.vst3'`. Omitting the bundle argument tests the current build. To verify a release, extract the release ZIP and point at that extracted bundle.

The probe checks 24 combinations of 44.1/48/96 kHz, 64/257/1024/8192-sample blocks, and real-time/offline flags. It checks exact note sample positions, loop crossings, note channels, passthrough MIDI, finite preview audio, channel releases on stop, continuous-PPQ tempo changes, seek/chase, component-state recall into a fresh instance, instance-state isolation, and repeated editor creation. Audio is rendered as fast as possible without an audio device. The real-time flag checks do not measure deadline compliance, latency, jitter, or hardware audio.

`binary-host-report.json`, `binary-host-log.txt`, and `verification-manifest.json` record the results and tested DLL hash. Editor factory creation is tested; visible native window attachment, clicks, focus, and host-specific resizing remain unverified.

## FL command comparison

`fl-fixtures/` contains five separate input MIDI files and the prototype's expected output for each command. Expected files define this prototype's behavior; they are not FL-generated ground truth. The verification runner first checks that the prototype's actual import/command/export path matches them.

For an actual FL comparison:

1. Use an empty FL project with one instrument and a fresh pattern. Import the case's `.input.mid` into its piano roll.
2. Select all notes. Set **piano-roll snap to 1/4 beat** (a 16th note), then invoke the case's shortcut below. Use a fresh input for each case.
3. Export only that pattern's note data as MIDI to `fl-fixtures/CASE.FL.mid`.
4. Tell the agent the export is ready, or rerun `run-checks.ps1`. It detects and compares real `.FL.mid` files automatically.

| Case | Shortcut | Save FL output as |
| --- | --- | --- |
| quick-quantize | Ctrl+Q | `quick-quantize.FL.mid` |
| quantize-starts | Shift+Q | `quantize-starts.FL.mid` |
| quick-chop | Ctrl+U | `quick-chop.FL.mid` |
| glue | Ctrl+G | `glue.FL.mid` |
| legato | Ctrl+L | `legato.FL.mid` |

The comparator checks note count, pitch, channel, onset, duration, and velocity. Tempo/meta events and track organization are ignored. Timing tolerance is one tick at the coarser of the two PPQ resolutions; velocity tolerance is one MIDI value. An optional `--ignore-velocity` switch helps diagnose whether a difference is purely velocity-related. Fixture times use 10-tick multiples at 960 PPQ, so they are also exactly representable at 96 PPQ. Preserve the exported file unchanged so its resolution can be considered when diagnosing differences.

Quick chop matches the user's actual FL exports at **1/4 step** (47 notes) and **1/4 beat** (13 notes). `confirmed-FL-settings.json` records the two supplied files/settings, and the runner processes the prototype at the corresponding resolution before comparing each file. That initial comparison used mismatched settings and is retained as a diagnostic rather than counted as a code defect.

Quick quantize exposed a real endpoint-versus-duration mismatch, corrected in 0.3.1; all four notes now match the actual FL export. Start-only quantize also matches all four FL notes exactly, including preservation of the original durations. Glue matches the two-note FL result exactly, including retention of the first merged note's velocity. Legato matches all four FL notes exactly. The initial five-command fixture set is complete; broader edge cases remain unverified.

## Ableton Live check

Live 12 is installed on this PC. Its actual session/scan/routing test is not completed by the custom host probe.

1. Make the complete VST3 bundle available in a folder Live scans, then rescan and load Piano Roll on MIDI track A.
2. Check preview audio with local playback; then enable Host sync and use Live's transport. Test stop, seek, loop, and tempo changes.
3. Create MIDI track B, choose track A under **MIDI From**, choose **Piano Roll** in the lower input chooser, and set B's monitor to **In**. Put an instrument on B. Disable Piano Roll's preview synth to hear the destination instrument.
4. Record a MIDI clip on B and inspect its note timing; stop playback and check for held notes. Save and reopen the Live set to check recall.

These routing steps follow [Ableton's MIDI-output instructions](https://help.ableton.com/hc/en-us/articles/209070189-Accessing-the-MIDI-output-of-a-VST-plug-in). Live merges channels during internal track-to-track MIDI routing, so preservation of separate output channels in the custom host does not imply separate channels will remain on track B.

## Third-party validation status

No installed pluginval executable was found in the searched user project/download locations. Downloading pluginval 1.0.4 from its official GitHub release failed because this agent session could not reach the server. Consequently no pluginval certification/pass is claimed. Its [official instructions](https://github.com/Tracktion/pluginval) describe a strictness-level-5 run as a host-compatibility baseline; it remains an additional check to perform when the executable is available.
