# FL Studio reference investigation

Target supplied by the user: `C:\Program Files\Image-Line\FL Studio 2025`.

## Observed shipped artifact

REA opened `FLEngine_x64.dll` as a native x86-64 PE shared library. SHA-256: `919c9e47e180f3480dac27b42d18b6fbd8dd2f9af1599ab451c6d32ac64806c8`. Size: 57,367,032 bytes.

REA `inspect_pe_resources` Evidence ID: `ev_8c4924e1051940a2a795f8c48adf3902457240d146308850873dee28c3b1ba6e`. The complete bounded inventory examined 656 directory entries and recorded 323 resources. The evidence is exported to `rea-evidence.json`.

That inventory identifies resource locations and digests. It does not decode the piano-roll UI, recover source code, establish interaction behavior, or observe execution. Icon relationships are static candidates. No proprietary payloads were copied into the plugin.

The REA session reported no configured native deep-analysis provider. Ghidra, IDA, and Kuna were unavailable. No native pseudocode, function tracing, or runtime UI observation was obtained. A restart discarded the first session's transient ledger; the resource inspection was repeated to preserve the original artifact evidence in the exported bundle.

## Observed documentation via ordinary filesystem read

Read `System\Config\Piano roll scripts\Piano roll script reference.txt` from the supplied installation. This was a shell read, not REA Evidence. It describes Python scripting with `flpianoroll`, MIDI-numbered notes, tick-based timing and durations, PPQ, selection, groups, channels/colours, velocity, and additional expressive note properties. It also documents score markers and timeline selections.

This establishes documented data-model concepts only. It does not establish native editing algorithms, shortcut behavior, synthesis semantics, binary layouts, or plugin-host interoperability.

## Reconstruction decisions and open questions

The new model uses beats internally, note IDs, 16 MIDI channels, note velocity/mute state, and a fixed capacity. The grid, colour palette, gestures, toolbar, preview oscillator, history, serialization, and scheduling code are independent design choices. No claim of measured FL interaction parity is made.

Answered: installed target identity; resource inventory; documented basic note/score fields; viable independent VST3/standalone project structure.

Partially answered: piano-roll feature scope, based on the shipped API rather than runtime observations.

Unresolved: exact native FL gesture semantics, selection edge cases, zoom/scroll behavior, note-property synthesis, advanced tools, and visual parity. These require a configured native provider and/or authorized runtime observations. The implementation remains an initial prototype until full compilation and host/UI verification succeed.

## Subsequent recording reference

The user subsequently supplied a real FL Studio recording. `VIDEO_REFERENCE.md` documents its identity, visual sampling boundary, timestamped observations, and the 0.2.0 implementation pass. This adds recorded runtime visual evidence; it does not turn the earlier resource inventory into decompilation evidence. Full 0.1.0 builds and automated tests succeeded after this initial investigation; manual host comparison remains outstanding.
