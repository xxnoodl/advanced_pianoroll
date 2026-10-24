# Ableton test - Piano Roll 0.3.1

Prepared dedicated VST3 scan folder:

`C:\Users\zihn\Documents\PianoRollVST3\host-test\VST3`

The complete bundle in this folder matches the tested 0.3.1 DLL hash:
`3E281A4222A716A483729C8B7A57C207DB415E4FB494F6C5DDF70EB56F24185C`.

## Stage 1: scan, load, and preview audio

1. Open Live and use an empty test set. Open Settings/Preferences > Plug-Ins.
2. Enable the VST3 custom folder and browse to the folder above. Rescan.
3. Search Live's browser for Piano Roll and load it onto a new MIDI track.
4. Open its editor, enable Host sync, leave Preview synth enabled, and start Live's transport. Confirm that the four-bar demo pattern makes sound and stops when the transport stops (allowing the short release tail).

The scan/load outcome can be corroborated in Ableton's local logs. Audible hardware output needs the user's observation.

## Stage 2: MIDI routing and recording

1. Create another MIDI track with an instrument.
2. Choose the Piano Roll track in MIDI From, choose the Piano Roll plug-in in the lower input chooser, and set monitoring to In.
3. Disable Piano Roll's preview synth. Start transport and confirm the destination instrument plays.
4. Record the generated MIDI onto the destination track. Inspect/export the recorded clip to check that it contains notes.

Live merges channels in internal track-to-track MIDI routing; this is distinct from the multi-channel preservation verified in the custom host.

## Stage 3: recall and transport

1. Make a recognizable edit to the notes, save the test set here as `PianoRoll-Ableton-Test.als`, close it, and reopen it.
2. Verify notes, loop length, Host sync, and Preview synth settings survive.
3. Exercise stop/start, seek, looping, tempo changes, and editor close/reopen. Report any held notes, missing MIDI, resets, or UI problems.

References: [Ableton's Windows VST setup](https://help.ableton.com/hc/en-us/articles/209071729-Using-VST-plug-ins-on-Windows), [MIDI output routing](https://help.ableton.com/hc/en-us/articles/209070189-Accessing-the-MIDI-output-of-a-VST-plug-in).

Current status: Live's scanner found Piano Roll in the prepared custom folder, and Log.txt confirms that the 0.3.1 processor loaded successfully. The user reports the plug-in appears, loads, and plays, but reports poor quality. Whether this means the basic preview timbre, audio glitches, or editor quality is being clarified. MIDI routing, recording, and session recall remain pending; audio quality is not signed off.
