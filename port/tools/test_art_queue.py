"""Offline tests: local bare remote, two independent clones, no network."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import tempfile
import threading
import unittest

import art_queue as aq


def args(command, **kwargs):
    return argparse.Namespace(command=command, **kwargs)


class QueueTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.remote = self.root / "remote.git"
        self.remote.mkdir()
        aq.git(self.remote, "init", "--bare")
        self.q = aq.Queue(str(self.remote), self.root / "seed")
        self.q.read()
        state = dict(version=1, tasks={}, integrator=None)
        aq.mutate(state, args("add", id="scene-054", piece="Scene 054", approval="user-approved", reference="review"))
        self.q.publish(state)

    def tearDown(self):
        self.tmp.cleanup()

    def race(self, operation):
        clones = [self.root / name for name in ("claude.git", "codex.git")]
        for clone in clones:
            aq.git(self.root, "clone", "--bare", str(self.remote), str(clone))
        barrier = threading.Barrier(2)

        def compete(index):
            queue = aq.Queue(str(self.remote), clones[index])
            state = queue.read()
            operation(state, "agent-" + str(index))
            barrier.wait(timeout=15)
            try:
                queue.publish(state)
                return True, state
            except ValueError:
                return False, state

        with ThreadPoolExecutor(max_workers=2) as executor:
            results = list(executor.map(compete, range(2)))
        self.assertEqual(sum(success for success, _ in results), 1)
        winning_state = next(state for success, state in results if success)
        self.assertEqual(self.q.read(), winning_state)
        return winning_state

    def test_two_clones_claim_exactly_one_wins(self):
        state = self.race(lambda state, owner: aq.mutate(state, args("claim", id="scene-054", owner=owner)))
        with self.assertRaisesRegex(ValueError, "already claimed"):
            aq.mutate(state, args("claim", id="scene-054", owner="other"))
        snapshot = aq.git(self.q.cache, "show", self.q.sha + ":PENDIENTES.md").stdout
        self.assertIn("en_curso", snapshot)
        self.assertIn(state["tasks"]["scene-054"]["owner"], snapshot)
        self.assertEqual(aq.git(self.q.cache, "ls-tree", "--name-only", self.q.sha).stdout.splitlines(),
                         ["PENDIENTES.md", "queue.json"])

    def test_explicit_recovery_checks_commit_and_retains_audit(self):
        import subprocess
        import sys
        state = self.q.read()
        aq.mutate(state, args("claim", id="scene-054", owner="stopped-agent"))
        self.q.publish(state)
        command = [sys.executable, str(Path(aq.__file__).resolve()), "--remote", str(self.remote),
                   "recover-task", "scene-054", "--from-owner", "stopped-agent", "--to-owner", "new-agent",
                   "--actor", "coordinator", "--evidence", "Session stopped confirmed; explicit handoff",
                   "--expected-commit"]
        rejected = subprocess.run(command + ["outdated"], capture_output=True, encoding="utf-8")
        self.assertEqual(rejected.returncode, 1)
        self.assertEqual(self.q.read(), state)
        accepted = subprocess.run(command + [self.q.sha], capture_output=True, encoding="utf-8")
        self.assertEqual(accepted.returncode, 0, accepted.stdout)
        recovered = self.q.read()
        self.assertEqual(recovered["tasks"]["scene-054"]["owner"], "new-agent")
        self.assertEqual(recovered["recoveries"][0]["from_owner"], "stopped-agent")

    def test_two_clones_integrator_exactly_one_wins(self):
        state = self.race(lambda state, owner: aq.mutate(state, args("integrator-acquire", owner=owner)))
        lock = state["integrator"].copy()
        for owner, token in [("intruder", lock["token"]), (lock["owner"], "wrong-token")]:
            with self.assertRaisesRegex(ValueError, "mismatch"):
                aq.mutate(state, args("integrator-release", owner=owner, token=token))
            self.assertEqual(state["integrator"], lock)
        aq.mutate(state, args("integrator-touch", owner=lock["owner"], token=lock["token"]))
        aq.mutate(state, args("integrator-release", owner=lock["owner"], token=lock["token"]))
        self.assertIsNone(state["integrator"])

    def test_merge_requires_pr_approval_lock_evidence(self):
        state = self.q.read()
        aq.mutate(state, args("claim", id="scene-054", owner="claude"))
        update = dict(id="scene-054", owner="claude", branch="art/054", pr=None,
                      approval=None, reference=None, evidence=None, token=None, integrator_owner="codex")
        for status in ["listo_pr", "pr_abierto", "integrado"]:
            if status != "listo_pr":
                with self.assertRaises(ValueError):
                    aq.mutate(state, args("update", status=status, **update))
            if status == "pr_abierto":
                update["pr"] = "https://example.test/pull/1"
                with self.assertRaises(ValueError):  # image must reach the app before the PR is announced
                    aq.mutate(state, args("update", status=status, **update))
                update["sent"] = "imgid123"
            if status == "integrado":
                aq.mutate(state, args("integrator-acquire", owner="codex"))
                update["token"] = state["integrator"]["token"]
                update["evidence"] = "merged commit abc123"
            aq.mutate(state, args("update", status=status, **update))
        self.assertEqual(state["tasks"]["scene-054"]["status"], "integrado")

    def test_owner_can_update_metadata_without_changing_state(self):
        state = self.q.read()
        aq.mutate(state, args("claim", id="scene-054", owner="claude"))
        update = dict(id="scene-054", owner="claude", status="en_curso", branch="art/054",
                      reference="review/054", pr=None, approval=None, evidence=None,
                      token=None, integrator_owner=None)
        aq.mutate(state, args("update", **update))
        task = state["tasks"]["scene-054"]
        self.assertEqual((task["branch"], task["reference"], task["status"]),
                         ("art/054", "review/054", "en_curso"))
        with self.assertRaisesRegex(ValueError, "already claimed"):
            aq.mutate(state, args("claim", id="scene-054", owner="codex"))
        update["owner"] = "codex"
        with self.assertRaisesRegex(ValueError, "Only the task owner"):
            aq.mutate(state, args("update", **update))

    def test_cli_preserves_worktree(self):
        import subprocess
        import sys
        work = self.root / "work"
        aq.git(self.root, "clone", str(self.remote), str(work))
        (work / "untracked.txt").write_text("keep me", encoding="utf-8")
        before = aq.git(work, "status", "--porcelain").stdout
        result = subprocess.run([sys.executable, str(Path(aq.__file__).resolve()), "--repo", str(work),
                                 "--snapshot", str(self.root / "PENDIENTES.md"), "list"],
                                capture_output=True, text=True, encoding="utf-8")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue(json.loads(result.stdout)["ok"])
        self.assertEqual(aq.git(work, "status", "--porcelain").stdout, before)
        self.assertTrue((self.root / "PENDIENTES.md").exists())


if __name__ == "__main__":
    unittest.main()
