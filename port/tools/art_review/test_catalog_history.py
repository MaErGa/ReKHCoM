import hashlib
import json
import tempfile
import unittest
from pathlib import Path

from PIL import Image
from catalog_history import run
from server import Store


class HistoryTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.data = self.root / 'desk'
        Image.new('RGB', (3, 2), 'red').save(self.root / 'shot.png')
        (self.root / 'log.txt').write_text('Build v1; capture frame 123', encoding='utf-8')
        sha = lambda path: hashlib.sha256((self.root / path).read_bytes()).hexdigest()
        self.entry = dict(key='history-menu', source='shot.png', source_sha256=sha('shot.png'),
            title='Menu', version='v1', status='pending', provenance='Test source',
            evidence_files=[dict(path='log.txt', sha256=sha('log.txt'))])
        self.manifest = dict(schema='rekhcom.art-history/1', entries=[self.entry])

    def test_plan_does_not_create_desk(self):
        result = run(self.manifest, self.root, self.data)
        self.assertEqual(result['counts'], {'import': 1})
        self.assertFalse(self.data.exists())

    def test_repeat_preserves_human_feedback(self):
        first = run(self.manifest, self.root, self.data, True)
        store = Store(self.data)
        item = store.get(first['entries'][0]['id'])
        item.update(status='honors', general='Keep this exact version')
        store.save(item)
        second = run(self.manifest, self.root, self.data, True)
        self.assertEqual(second['counts'], {'preserved': 1})
        self.assertEqual(len(store.all()), 1)
        self.assertEqual(store.get(item['id'])['general'], 'Keep this exact version')
        self.assertEqual(store.get(item['id'])['status'], 'honors')

    def test_hash_error_prevents_all_writes(self):
        other = dict(self.entry, key='other', source_sha256='0' * 64)
        self.manifest['entries'].append(other)
        with self.assertRaisesRegex(ValueError, 'Source hash'):
            run(self.manifest, self.root, self.data, True)
        self.assertFalse(self.data.exists())

    def test_generic_approval_cannot_promote_screenshot(self):
        self.entry.update(status='approved', approval_record=dict(quote='Menu approved'))
        with self.assertRaisesRegex(ValueError, 'exact screenshot'):
            run(self.manifest, self.root, self.data, True)
        self.assertFalse(self.data.exists())

    def test_duplicate_pixels_do_not_duplicate_family(self):
        store = Store(self.data)
        old = store.add((self.root / 'shot.png').read_bytes(), 'Existing', 'old', status='approved')
        result = run(self.manifest, self.root, self.data, True)
        self.assertEqual(result['counts'], {'already_catalogued': 1})
        self.assertEqual(result['entries'][0]['id'], old['id'])
        self.assertEqual(len(store.all()), 1)


if __name__ == '__main__':
    unittest.main()
