# Recorded FL Studio piano-roll reference

Source supplied by the user: `C:\Users\zihn\Videos\Captures\FL Studio 2025 2026-10-23 20-46-11.mp4`.

SHA-256: `BFC1B980BAB660015589E8E2AC3B25DD9730576AE6CA666CF3C0195D433C5132`.

Duration 219.050333 seconds; 2304×1440; approximately 59.94 frames/second; video plus audio. Inspection used FFmpeg/FFprobe and ordinary image viewing, **not REA decompilation or live app control**. Sampled 438 frames at 0.5-second intervals, inspected overview/contact sheets, and inspected native-resolution frames at 01:12 and 02:46. This is sampled visual evidence, not examination of every original frame. Audio was not transcribed.

The local `reference/` directory contains the extracted frames and contact sheets. The 442 MB source video is not copied into the source package or plugin.

## Timestamped observations

| Approximate time | Visible evidence | What remains unknown |
| --- | --- | --- |
| 00:00–00:30 | Empty score with intermittent orange highlights on the piano keyboard. | Input source and pressed keys are not shown. |
| 00:33–00:36 | Long upper notes/chords and separate lower notes; the top strip shows a compressed score outline. | How this score was entered or restored. |
| 00:39–00:51 | Dense short upper notes with longer lower notes; matching velocity stems; playback cursor crosses the grid. | Which transformation, import, or command created the change. |
| 00:54–01:09 | Notes and velocity stems are pink; a reddish time span appears in the ruler and grid. | Exact selection gesture/modifiers. |
| 01:12–01:30 | Smooth wave-shaped velocity profiles change depth and repetition while the score remains visible. | Tool identity, parameter values, preview/commit behavior, and shortcuts. |
| 01:36–01:48 | Velocity profiles become irregular; the score layout is largely retained. | Randomization method, seed, amount, selection scope, and whether another tool is involved. |
| 01:57–02:15 | Green notes and irregular green stems; playback highlights keys. | Exact audition/selection commands. |
| 02:27–02:35 | A red time span remains while notes are green; notes appear as the playhead advances. | Recording input path and whether the time span controls looping. |
| 02:42–02:48 | Some lower notes are subdivided into adjacent repeated notes. A selected row is pink while other rows remain green. | Exact slicing/chopping command, count control, and modifier keys. |
| 02:46 native frame | Compact rows, labels on black and white keys, nearly square note edges, note labels when space allows, and a separate velocity divider are clearly visible. | Measured drag rules and minimum/maximum divider positions. |
| 03:09–03:18 | The main pitch viewport changes while the overview continues to display a broader score outline. | Exact scrolling/zooming gesture. |
| 03:21–03:36 | A selected pink vertical note stack appears alongside unselected short notes. | Whether it was entered by a stamp, MIDI input, paste, or another operation. |

No tool dialog or readable key overlay establishes the hidden transformation commands in these sections. Those commands must be confirmed rather than guessed.

## First implementation pass: 0.2.0

- Added a full-pattern note overview and a visible viewport indicator. Clicking/dragging it navigates time.
- Added horizontal and vertical scrollbars and vertical pitch zoom. The specific Ctrl+Alt+wheel mapping is an implementation choice, not a shortcut recovered from the video.
- Increased default pitch density; added compact black/white key labels and rectangular notes with subtle shading.
- Changed selected notes and matching velocity stems to pink; added orange audition/playback key feedback and a red playback cursor.
- Added a separately draggable ruler time range. It shades the ruler, score, and velocity lane without changing the note selection. Right-clicking the ruler clears it.
- Added a draggable velocity-lane divider. Its exact drag limits are an implementation choice.

**Scope boundary:** the time range is editor-local selection shading in this pass. It does not yet change playback-loop bounds, persist in plugin state, or define the scope of processing tools. It must not be described as completed FL timeline-selection parity.

The video establishes the visible outcomes above. It does not establish the exact modifier mapping, hidden tool algorithms, MIDI input/recording behavior, or internal FL implementation. The existing native resource inventory remains a separate evidence source described in INVESTIGATION.md.

## Next behavior questions

1. Identify the tools and shortcuts used for velocity waves and irregular velocity changes around 01:10–01:45.
2. Identify the operation creating the short upper-note pattern near 00:37–00:39 and the repeated lower-note slices around 02:42–02:48.
3. Establish how time selection affects playback looping and processing scope.
4. Record or describe the exact mouse modifiers for selection, duplication, resizing, zoom, and scrolling.

These are gaps in the reference, not reasons to claim the corresponding FL behavior has been reproduced.

## Second recording — 2026-10-24

Source: `C:\Users\zihn\Videos\Captures\FL Studio 2025 2026-10-24 06-37-05.mp4`. Duration 68.458533 seconds, 2304×1440 at approximately 59.94 fps. Inspected a four-second overview, full-resolution frames at 00:24/00:36/00:46/00:54, and lower-left control-lane crops sampled every 0.5 seconds. Derived images are in `reference/velocity-20261024/`. No source code or plugin behavior was changed during this inspection.

Confirmed visible property-lane transitions: Velocity through approximately 00:36, Pan around 00:37–00:42, Release around 00:42.5–00:45, and Fine pitch from approximately 00:45.5 onward. Pan and fine pitch have a centre/zero baseline; velocity and release show stems from the lower baseline. A selected note and its stem are pink. Fine-pitch stem dragging changes the displayed value; at 00:54 the hint reads `Fine pitch - +540 cents`. At 00:36 the hint reads `Velocity - 92%`.

At 00:24, the upper-left hint reads `Randomize all steps`. This is a useful tool/action clue, but the associated menu/dialog and parameter controls are not visible. Clicking/hovering the lower-left Control selector is visible, while its popup menu is not present in the inspected frames. A capture that omits separate popup windows is a possible explanation, not an established fact.

This is enough reference to identify the multi-property Control lane and observe direct per-note editing. It still does not expose the wave-shaping/randomization dialog or its parameter values from the first recording.

## Menu screenshots and third recording — 2026-10-24

User-supplied screenshots `Screenshot 2026-10-24 064706.png`, `064657.png`, and `064639.png` directly expose menu content missing from the recordings. The first two establish a stamp/preset menu with basic and advanced chords, many scale presets, percussion fills/flams/rolls, a slide-up preset, Automatic chord, and an Only one option. Preset interval/rhythm definitions and the exact Only one/Automatic chord behaviors are not established by their names alone.

The third screenshot establishes the Tools menu and displayed shortcut mappings, including Quick legato (Ctrl+L), Quick quantize (Ctrl+Q), Quick quantize start times (Shift+Q), Quantize (Alt+Q), Quick chop (Ctrl+U), Chop (Alt+U), Glue (Ctrl+G), Arpeggiate (Alt+A), Strum (Alt+S), Flam (Alt+F), Claw machine (Alt+W), Limit (Alt+K), Flip (Alt+Y), Randomize (Alt+R), Scale levels (Alt+X), and LFO (Alt+O). It also exposes Riff machine, chord-progression generation, and script entries including Arpeggiator, Euclidean, Humanize, Note repeater sprinkler, and Select by velocity and more. This confirms tool availability and shortcuts, not that any particular tool was invoked in the earlier clips or that its algorithm has been recovered.

Third video: `C:\Users\zihn\Videos\Captures\FL Studio 2025 2026-10-24 06-45-23.mp4`, 66.490600 seconds, 2304×1440 at approximately 59.94 fps. Inspected a three-second overview and a native-resolution frame at 00:48. It shows a change to a selected pink note pattern and subsequent changes to the selected velocity distribution. No readable processing-tool dialog appears in the inspected frames. Derived images are in `reference/tools-20261024/`.

Implementation consequence: the prototype needs a real stamp/preset system and a processing-tools menu, along with corrected/distinct quick-action shortcuts. Its current plain Q/L commands do not constitute FL shortcut parity. The menu inventory is now confirmed; tool parameter behavior and preset definitions remain separate work. No plugin code was changed in this reference-inspection turn.

## Subsequent implementation - 0.3.0

The next authorized build pass implements a subset of this menu inventory: basic/advanced chord and common-scale stamps; one-shot/repeated placement; snap-based quick chop; contiguous-note glue; separate quick quantize/start-only quantize actions; and adjustable Quantize, Chop, Strum, Randomize velocity, Scale velocity, and Velocity LFO panels with preview/Accept/Cancel.

Verified shortcut mappings were applied to these commands, including Ctrl+L/Q/U/G, Shift+Q, and Alt+Q/U/S/R/X/O. Tool selection uses P/B/C/E/T; octave transposition uses Ctrl+Up/Down; selected mute/unmute uses Alt+M/Alt+Shift+M. Plain Q/L/M no longer invoke the earlier prototype-specific editing actions.

The official piano-roll menu documentation also confirms one-shot stamping and snap-based chopping: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/pianoroll_menu.htm . Quantizer mode descriptions were consulted at https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/pianoroll_qnt.htm . The implemented quick quantize snaps starts and durations; Shift+Q preserves durations. These choices do not establish matching internal FL algorithms.

No unsupported menu item was added as a placeholder. Automatic chords, exotic/percussion presets, the full arpeggiator and other processing tools, and pan/release/fine-pitch property lanes remain unfinished. Root-position stamp voicings and the velocity-processing parameters are defined by the new implementation, not extracted from FL's preset files or native code.

## MIDI export reference - quantize correction in 0.3.1

The user subsequently exported the fixed quick-quantize fixture from FL at 1/4-beat snap. Source file: `verification/fl-fixtures/quick-quantize.FL.mid`, SHA-256 `21632bf725138e42f222fc16ef92575dd5c58d04c01fb046b47427e56d392e0c`.

This exposed a real difference in 0.3.0. The first note originally started at 130/960 beats and ended at 740/960 beats. FL moved its start to 0.25 and its end to 0.75, giving duration 0.5. The prototype rounded the original duration to 0.75 instead. All four exported notes support snapping original endpoints on this fixture.

Version 0.3.1 changes the endpoint-enabled quantizer accordingly; start-only quantizing still preserves duration. This establishes agreement for the supplied fixture and settings, not all advanced FL quantizer modes or collapse/rounding edge cases. The original failed output/report were retained with `.before-0.3.1` suffixes for comparison.
