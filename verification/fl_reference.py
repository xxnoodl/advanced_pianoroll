"""Fixed MIDI fixtures and a note-level comparator for manual FL Studio checks.

Expected files encode this prototype's documented behavior, not an FL oracle.
Only an actual FL export can establish matching behavior.
"""
import argparse
import json
import struct
from collections import defaultdict, deque
from pathlib import Path

PPQ = 960
STEP = 240

def vlq(value):
    output = [value & 127]
    while value >> 7:
        value >>= 7
        output.insert(0, (value & 127) | 128)
    return bytes(output)

def write_midi(path, notes):
    events = [(0, -1, b"\xff\x51\x03\x07\xa1\x20")]
    for start, length, pitch, velocity, channel in notes:
        events += [(start, 1, bytes([0x90 | (channel - 1), pitch, velocity])),
                   (start + length, 0, bytes([0x80 | (channel - 1), pitch, 0]))]
    events.sort(key=lambda e: (e[0], e[1], e[2]))
    data = bytearray()
    previous = 0
    for tick, _, event in events:
        data += vlq(tick - previous) + event
        previous = tick
    data += b"\x00\xff\x2f\x00"
    Path(path).write_bytes(b"MThd" + struct.pack(">IHHH", 6, 0, 1, PPQ) + b"MTrk" + struct.pack(">I", len(data)) + data)

def read_midi(path):
    raw = Path(path).read_bytes()
    if raw[:4] != b"MThd":
        raise ValueError("Not a MIDI file")
    header_length = struct.unpack_from(">I", raw, 4)[0]
    fmt, tracks, ppq = struct.unpack_from(">HHH", raw, 8)
    if fmt > 1 or ppq == 0 or ppq & 0x8000:
        raise ValueError("Use a type-0/1 PPQ MIDI file")
    pos = 8 + header_length
    notes = []
    for _ in range(tracks):
        if raw[pos:pos+4] != b"MTrk":
            raise ValueError("Missing MIDI track")
        length = struct.unpack_from(">I", raw, pos + 4)[0]
        track = raw[pos+8:pos+8+length]
        pos += 8 + length
        i = tick = 0
        running = None
        active = defaultdict(deque)
        def number():
            nonlocal i
            value = 0
            for _ in range(4):
                byte = track[i]; i += 1
                value = (value << 7) | (byte & 127)
                if not byte & 128:
                    return value
            raise ValueError("Invalid variable-length value")
        while i < len(track):
            tick += number()
            status = track[i]
            if status & 128:
                i += 1
                if status < 0xF0:
                    running = status
            elif running is not None:
                status = running
            else:
                raise ValueError("Invalid running status")
            if status == 0xFF:
                kind = track[i]; i += 1
                size = number(); i += size
                if kind == 0x2F:
                    break
                continue
            if status in (0xF0, 0xF7):
                size = number(); i += size; running = None
                continue
            if status >= 0xF0:
                raise ValueError("Unsupported MIDI system event")
            kind = status & 0xF0
            channel = (status & 15) + 1
            first = track[i]; i += 1
            second = 0
            if kind not in (0xC0, 0xD0):
                second = track[i]; i += 1
            key = channel, first
            if kind == 0x90 and second:
                active[key].append((tick, second))
            elif kind == 0x80 or (kind == 0x90 and not second):
                if active[key]:
                    start, velocity = active[key].popleft()
                    notes.append(dict(start=start/ppq, length=(tick-start)/ppq, pitch=first, velocity=velocity, channel=channel))
        if any(active.values()):
            raise ValueError("MIDI contains unmatched note-ons")
    return ppq, sorted(notes, key=lambda n: (n["start"], n["pitch"], n["channel"]))

def generate(folder):
    folder.mkdir(parents=True, exist_ok=True)
    # Multiples of 10 ticks at 960 PPQ are exactly representable at 96 PPQ.
    # Keep the starts off the 240-tick snap grid without introducing import-rounding ties.
    source = [(130, 610, 60, 80, 1), (1070, 390, 64, 96, 1), (2010, 730, 67, 110, 1), (2880, 960, 48, 90, 1)]
    rounding = lambda tick: ((tick + STEP//2)//STEP)*STEP
    quantized = [(rounding(t), max(1, rounding(t+d)-rounding(t)), p, v, c) for t,d,p,v,c in source]
    starts = [(rounding(t), d, p, v, c) for t,d,p,v,c in source]
    chopped = []
    for t,d,p,v,c in source:
        for offset in range(0, d, STEP):
            chopped.append((t + offset, min(STEP, d-offset), p, v, c))
    glue_in = [(0,240,60,80,1), (240,480,60,96,1), (720,240,60,110,1), (960,240,64,100,1)]
    legato_in = [(0,240,60,80,1), (960,240,64,96,1), (1920,240,67,110,1), (2880,960,72,100,1)]
    pairs = {
        "quick-quantize": (source, quantized, "Ctrl+Q"),
        "quantize-starts": (source, starts, "Shift+Q"),
        "quick-chop": (source, chopped, "Ctrl+U"),
        "glue": (glue_in, [(0,960,60,80,1), (960,240,64,100,1)], "Ctrl+G"),
        "legato": (legato_in, [(t,960,p,v,c) for t,d,p,v,c in legato_in], "Ctrl+L"),
    }
    manifest = {}
    for name, (before, after, shortcut) in pairs.items():
        write_midi(folder / f"{name}.input.mid", before)
        write_midi(folder / f"{name}.expected.mid", after)
        manifest[name] = dict(shortcut=shortcut, snap="1/4 beat (16th note)", inputNotes=len(before), expectedNotes=len(after))
        # Verify the writer/parser agree on every note in each generated file.
        _, parsed = read_midi(folder / f"{name}.expected.mid")
        assert len(parsed) == len(after)
    (folder / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"Generated five input/expected MIDI pairs in {folder}")

def compare(expected_path, actual_path, ignore_velocity=False):
    expected_ppq, expected = read_midi(expected_path)
    actual_ppq, actual = read_midi(actual_path)
    tolerance = max(1/expected_ppq, 1/actual_ppq) + 1e-9
    unmatched = list(actual)
    missing = []
    for note in expected:
        found = next((i for i, candidate in enumerate(unmatched)
                      if candidate["pitch"] == note["pitch"] and candidate["channel"] == note["channel"]
                      and abs(candidate["start"] - note["start"]) <= tolerance
                      and abs(candidate["length"] - note["length"]) <= tolerance
                      and (ignore_velocity or abs(candidate["velocity"] - note["velocity"]) <= 1)), None)
        if found is None:
            missing.append(note)
        else:
            unmatched.pop(found)
    return dict(passed=not missing and not unmatched, expected=str(expected_path), actual=str(actual_path),
                expectedNotes=len(expected), actualNotes=len(actual), toleranceBeats=tolerance,
                missingExpectedNotes=missing, unexpectedActualNotes=unmatched)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="action", required=True)
    g = sub.add_parser("generate"); g.add_argument("folder", type=Path)
    c = sub.add_parser("compare"); c.add_argument("expected", type=Path); c.add_argument("actual", type=Path)
    c.add_argument("--ignore-velocity", action="store_true"); c.add_argument("--report", type=Path)
    args = parser.parse_args()
    try:
        if args.action == "generate":
            generate(args.folder)
        else:
            result = compare(args.expected, args.actual, args.ignore_velocity)
            output = json.dumps(result, indent=2)
            print(output)
            if args.report:
                args.report.write_text(output, encoding="utf-8")
            raise SystemExit(0 if result["passed"] else 1)
    except (ValueError, OSError, IndexError, struct.error) as error:
        parser.exit(2, f"Error: {error}\n")
