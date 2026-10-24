# Use Piano Roll to compose for another instrument

The VST3 already sends note-on, note-off, velocity and channel data to the host, including its typing keyboard input. The internal speaker/monitor switch only controls the preview synth; it does not mute MIDI output.

## Ableton Live: audible MIDI routing

1. Create a MIDI track named Piano Roll and load our VST3 there.
2. Create a second MIDI track named Synth and load an instrument such as Wavetable or your synth VST3 there.
3. On Synth, choose Piano Roll (the track) in MIDI From, then Piano Roll (the plugin) in the lower chooser. Choose the plugin output, rather than the track's original MIDI input.
4. Set Synth's Monitor to In.
5. In our plugin, disable the speaker icon (preview synth) and enable the chain icon (Host sync). Start Live's transport to play the score through Synth.
6. Enable the keyboard icon or Ctrl+T with the editor focused to play Synth using our typing layout. This works without starting transport.

If silent, first test that Synth makes sound from its own keyboard, then check that its input chooses our plugin and monitoring is In. Avoid playing an existing clip on Synth simultaneously with the same generated part.

## Record a Live clip

Arm the Synth track and record into an empty Session slot or Arrangement. You can capture both the sequenced score and focused typing. After recording, stop/disable the plugin's sequencer (or remove its routing), switch the receiving track to normal clip monitoring, and play the recorded clip through Synth. Keep one source playing the part to avoid doubled notes.

An alternative is Export MIDI from our editor, then import that file into Live. Neither recording nor export creates continuous two-way synchronization with the source score.

Save the Live Set with both tracks, or save a reusable grouped-track setup, to retain the routing. Live merges MIDI channels during internal track routing, so the current channel selector cannot split one plugin instance among 16 Live synth tracks. Use a separate Piano Roll instance per part for now.

## Replacing the native piano-roll workflow

Current: compose and edit inside our VST3, route it to a synth, and optionally record/export a Live clip. The score is stored in the plugin instance's state in the Live Set.

Not implemented: selecting an existing Live clip and having our editor load and continuously update that clip. The VST3 does not replace Live's built-in Clip View panel or access its clip database by itself.

Original integration plan (now implemented as the first companion in `companion/README.md`): a Max for Live companion can obtain the target clip via Live Object Model, read its note dictionaries, and exchange edits with the shared editor core. It would need note-ID reconciliation, undo grouping, clip-switch/conflict handling, and preservation of probability, velocity deviations, release velocities and per-note expression that our current score format does not represent. In clip mode Live would play the clip through its existing instrument; our sequencer would be disabled to prevent duplicate playback. The 0.6.0 companion implements the clip-authoring path with automatic sync and transport/audition commands; native verification is still required. The core remains independent of Ableton.

Hosting third-party synths inside this VST3 is another possible architecture, but it is not implemented and is unnecessary for normal Live track routing. Native Live instruments cannot simply be loaded into a VST3 host.

Sources:
- https://help.ableton.com/hc/en-us/articles/209070189-Accessing-the-MIDI-output-of-a-VST-plug-in
- https://docs.cycling74.com/apiref/lom/clip/
