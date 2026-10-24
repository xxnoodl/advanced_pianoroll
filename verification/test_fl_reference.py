import struct
import tempfile
import unittest
from pathlib import Path
import fl_reference as midi

class MidiComparisonTests(unittest.TestCase):
    def setUp(self):
        self.workspace = Path(__file__).resolve().parent
        self.temp = tempfile.TemporaryDirectory(prefix="midi-check-", dir=self.workspace)
        self.folder = Path(self.temp.name).resolve()
        self.assertTrue(self.folder.is_relative_to(self.workspace))
    def tearDown(self):
        self.assertTrue(self.folder.is_relative_to(self.workspace))
        self.temp.cleanup()
    def pair(self, actual):
        a, b = self.folder / "expected.mid", self.folder / "actual.mid"
        midi.write_midi(a, [(0, 960, 60, 90, 1)])
        midi.write_midi(b, actual)
        return a, b
    def test_known_running_status(self):
        data = b"\x00\x90\x3c\x64" + midi.vlq(240) + b"\x40\x64\x00\x80\x3c\x00" + midi.vlq(240) + b"\x40\x00\x00\xff\x2f\x00"
        file = self.folder / "running.mid"
        file.write_bytes(b"MThd" + struct.pack(">IHHH", 6, 0, 1, 960) + b"MTrk" + struct.pack(">I", len(data)) + data)
        ppq, notes = midi.read_midi(file)
        self.assertEqual(ppq, 960)
        self.assertEqual([(n["pitch"], n["start"], n["length"]) for n in notes], [(60, 0, 0.25), (64, 0.25, 0.25)])
    def test_exact_match(self):
        self.assertTrue(midi.compare(*self.pair([(0, 960, 60, 90, 1)]))["passed"])
    def test_pitch_and_count_mismatch(self):
        self.assertFalse(midi.compare(*self.pair([(0, 960, 61, 90, 1)]))["passed"])
        self.assertFalse(midi.compare(*self.pair([]))["passed"])
    def test_timing_mismatch(self):
        self.assertFalse(midi.compare(*self.pair([(4, 960, 60, 90, 1)]))["passed"])
        self.assertFalse(midi.compare(*self.pair([(0, 964, 60, 90, 1)]))["passed"])
    def test_velocity_and_channel_mismatch(self):
        pair = self.pair([(0, 960, 60, 70, 1)])
        self.assertFalse(midi.compare(*pair)["passed"])
        self.assertTrue(midi.compare(*pair, ignore_velocity=True)["passed"])
        self.assertFalse(midi.compare(*self.pair([(0, 960, 60, 90, 2)]), ignore_velocity=True)["passed"])

if __name__ == "__main__":
    unittest.main()
