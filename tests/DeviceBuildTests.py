import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('device_builder', Path(__file__).resolve().parents[1] / 'companion' / 'build_device.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)

class DevicePaths(unittest.TestCase):
    def test_installed_paths_handle_spaces(self):
        with tempfile.TemporaryDirectory(prefix='piano roll device ') as folder:
            destination = Path(folder)
            builder.build(destination, True)
            patch = json.loads((destination / 'Piano Roll Live.maxpat').read_text())['patcher']
            boxes = {item['box']['id']: item['box'] for item in patch['boxes']}
            self.assertEqual(boxes['api']['text'], 'js "' + (destination / 'bridge-max.js').as_posix() + '" "' + (destination / 'clip-protocol.js').as_posix() + '"')
            self.assertIn('"' + (destination / 'bridge-node.js').as_posix() + '"', boxes['node']['text'])
            self.assertTrue(all(item['bootpath'] == destination.as_posix() for item in patch['dependency_cache']))

    def test_source_patch_remains_portable(self):
        boxes = {item['box']['id']: item['box'] for item in builder.patch()['patcher']['boxes']}
        self.assertEqual(boxes['api']['text'], 'js bridge-max.js clip-protocol.js')
        self.assertEqual(boxes['node']['text'], 'node.script bridge-node.js @autostart 1 @defer 1')

if __name__ == '__main__':
    unittest.main()
