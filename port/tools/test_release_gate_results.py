#!/usr/bin/env python3
"""Offline regressions for release gates: process failures cannot pass on partial evidence."""
import importlib.util
import json
import os
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

TOOLS = Path(os.environ.get('KHCOM_GATE_TEST_TOOLS', Path(__file__).parent))


def load(name):
    spec = importlib.util.spec_from_file_location('gate_' + name, TOOLS / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class GateResults(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.build = self.root / 'bin'
        self.build.mkdir()
        for name in ('khcom.exe', 'SDL3.dll'):
            (self.build / name).write_bytes(b'original')
        (self.root / 'roms').mkdir()
        (self.root / 'roms/B8CP.gba').write_bytes(b'own ROM fixture')

    def events(self, code=0, shots=True, selected=True):
        m = load('test_events')
        def run(exe, work, version, lid, index, *args):
            d = work / f'{lid}_{index:03d}'
            d.mkdir(parents=True, exist_ok=True)
            if shots:
                (d / 'last_shot.sha1').write_text('fixture')
            return code, False, []
        with patch.object(m, 'ROOT', self.root), patch.object(m, 'event_lists', return_value=[(0, 0, 'fixture')] if selected else []), \
                patch.object(m, 'run', side_effect=run), patch.object(sys, 'argv', ['test_events']):
            return m.main()

    def test_events_nonzero_fails(self):
        self.assertEqual(self.events(code=17), 1)

    def test_events_no_picture_fails(self):
        self.assertEqual(self.events(shots=False), 1)

    def test_events_empty_fails(self):
        self.assertEqual(self.events(selected=False), 1)

    def test_events_valid_pruned_evidence_passes(self):
        self.assertEqual(self.events(), 0)

    def frameskip(self, shots):
        m = load('test_frameskip')
        def run(args, **kwargs):
            d = Path(args[args.index('--work') + 1]) / '0_000'
            d.mkdir(parents=True, exist_ok=True)
            (d / 'result.json').write_text('[0, false, []]')
            if shots:
                for frame in (900, 1800, 2700, 3600):
                    (d / f'shot_{frame:05d}.bmp').write_bytes(b'identical pixels')
            kwargs['stdout'].close()
            return SimpleNamespace(returncode=0)
        with patch.object(m, 'ROOT', self.root), patch.object(m.subprocess, 'run', side_effect=run), \
                patch.object(sys, 'argv', ['test_frameskip']):
            with self.assertRaises(SystemExit) as exit:
                m.main()
        return exit.exception.code

    def test_frameskip_requires_four_pictures(self):
        self.assertEqual(self.frameskip(False), 1)

    def test_frameskip_valid_four_pictures_passes(self):
        self.assertEqual(self.frameskip(True), 0)

    def audio(self, name, code, crash=False):
        m = load(name)
        hd = self.root / 'hd'
        (hd / 'music').mkdir(parents=True)
        def run(args, **kwargs):
            (kwargs['cwd'] / 'kh_com.log').write_text('hd music: song 1\n' + ('CRASH\n' if crash else ''))
            return SimpleNamespace(returncode=code)
        with patch.object(m, 'ROOT', self.root), patch.object(m.subprocess, 'run', side_effect=run):
            if name == 'test_hd_audio':
                return m.run(self.build, 'title')
            with patch.object(m, 'analyse', return_value={'frames': 120}), patch.object(m, 'heard', return_value='0 jumps'):
                return m.run(self.build, hd, 'title')

    def test_audio_nonzero_with_valid_log_fails(self):
        self.assertTrue(self.audio('test_hd_audio', 17)[0])

    def test_audio_crash_log_with_zero_exit_fails(self):
        self.assertTrue(self.audio('test_hd_audio', 0, True)[0])

    def test_flow_nonzero_fails_before_partial_analysis(self):
        self.assertIn('error', self.audio('test_hd_audio_flow', 17))

    def test_audio_valid_log_and_exit_passes(self):
        bad, ran = self.audio('test_hd_audio', 0)
        self.assertTrue(ran)
        self.assertEqual(bad, [])

    def test_resume_rechecks_modified_inputs(self):
        m = load('test_events')
        work = self.root / 'events'
        def place(exe, work, d):
            for f in ('khcom.exe', 'SDL3.dll'):
                (d / f).write_bytes((exe / f).read_bytes())
        def run(args, **kwargs):
            (kwargs['cwd'] / 'shot_00100.bmp').write_bytes(b'fixture')
            return SimpleNamespace(returncode=0)
        with patch.object(m, 'ROOT', self.root), patch.object(m.case_bin, 'place', side_effect=place), \
                patch.object(m.subprocess, 'run', side_effect=run) as execute:
            def case(frames=100):
                return m.run(self.build, work, 'eu', 0, 0, frames, 20, resume=True)
            case()
            case()
            self.assertEqual(execute.call_count, 1)
            (self.build / 'khcom.exe').write_bytes(b'new executable')
            case()
            self.assertEqual(execute.call_count, 2)
            case(200)
            self.assertEqual(execute.call_count, 3)
            with patch.dict(os.environ, {'KHCOM_SKIPTEST': '1'}):
                case(200)
            self.assertEqual(execute.call_count, 4)
            (self.build / 'SDL3.dll').write_bytes(b'new SDL')
            case(200)
            self.assertEqual(execute.call_count, 5)
            (self.root / 'roms/B8CP.gba').write_bytes(b'new ROM')
            case(200)
            self.assertEqual(execute.call_count, 6)

    def test_stale_shared_copy_cannot_claim_new_fingerprint(self):
        m = load('test_events')
        def stale(exe, work, d):
            for f in ('khcom.exe', 'SDL3.dll'):
                (d / f).write_bytes(b'previous build')
        with patch.object(m, 'ROOT', self.root), patch.object(m.case_bin, 'place', side_effect=stale), \
                patch.object(m.subprocess, 'run') as execute:
            code, _, _ = m.run(self.build, self.root / 'events', 'eu', 0, 0, 100, 20)
        self.assertNotEqual(code, 0)
        execute.assert_not_called()


if __name__ == '__main__':
    unittest.main()
