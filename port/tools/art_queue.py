#!/usr/bin/env python3
"""Online art queue. All output is JSON; shared state lives on one dedicated Git ref."""
import argparse
import datetime as dt
import json
import os
from pathlib import Path
import secrets
import subprocess
import sys
import tempfile

REF = "refs/heads/coordination/art-queue"
STATES = ("pendiente", "en_curso", "en_revision", "listo_pr", "pr_abierto", "integrado", "bloqueado")
TRANSITIONS = {
    "en_curso": {"en_revision", "listo_pr", "bloqueado"},
    "en_revision": {"en_curso", "listo_pr", "bloqueado"},
    "listo_pr": {"en_curso", "pr_abierto", "bloqueado"},
    "pr_abierto": {"en_curso", "integrado", "bloqueado"},
    "bloqueado": {"en_curso"},
}


def now():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def git(repo, *args, data=None, check=True):
    # Binary pipes prevent Windows CRLF conversion from changing mktree names.
    p = subprocess.run(["git", "-C", str(repo), *args], input=data.encode("utf-8") if data is not None else None,
                       capture_output=True, env={**os.environ, "GIT_TERMINAL_PROMPT": "0"})
    p.stdout = p.stdout.decode("utf-8", errors="replace")
    p.stderr = p.stderr.decode("utf-8", errors="replace")
    if check and p.returncode:
        raise ValueError(p.stderr.strip() or "Git operation failed")
    return p


def render(state):
    def cell(v):
        return str(v or "—").replace("|", "\\|").replace("\n", " ").replace("\r", " ")
    lines = ["# PENDIENTES — cola online", "", "Estado autoritativo: coordination/art-queue (queue.json).",
             "Tomar una pieza con claim antes de trabajar. Todos los cambios entran por PR.", "",
             "| ID | Pieza | Estado | Responsable | Rama / PR | Aprobación | Actualizado |",
             "| --- | --- | --- | --- | --- | --- | --- |"]
    for t in state["tasks"].values():
        lines.append("| " + " | ".join(cell(v) for v in [t["id"], t["piece"], t["status"],
                     t["owner"], t.get("pr") or t.get("branch"), t.get("approval"), t["updated_at"]]) + " |")
    lock = state["integrator"]
    lines += ["", "Integrador: " + (cell(lock["owner"]) + " (ocupado; " + lock["updated_at"] + ")" if lock else "libre"), ""]
    return "\n".join(lines)


class Queue:
    def __init__(self, remote, cache):
        self.remote, self.cache = remote, Path(cache)
        self.cache.mkdir(parents=True, exist_ok=True)
        if not (self.cache / "HEAD").exists():
            git(self.cache, "init", "--bare")
        if git(self.cache, "rev-parse", "--is-bare-repository").stdout.strip() != "true":
            raise ValueError("--state-dir must be a dedicated bare repository")
        self.sha = None

    def read(self):
        refs = git(self.cache, "ls-remote", self.remote, REF).stdout.strip()
        if not refs:
            self.sha = None
            return None
        git(self.cache, "fetch", "--no-tags", self.remote, REF)
        self.sha = git(self.cache, "rev-parse", "FETCH_HEAD").stdout.strip()
        return json.loads(git(self.cache, "show", self.sha + ":queue.json").stdout)

    def publish(self, state):
        state["updated_at"] = now()
        entries = []
        for name, content in [("queue.json", json.dumps(state, ensure_ascii=False, indent=2) + "\n"),
                              ("PENDIENTES.md", render(state))]:
            blob = git(self.cache, "hash-object", "-w", "--stdin", data=content).stdout.strip()
            entries.append("100644 blob " + blob + "\t" + name + "\n")
        tree = git(self.cache, "mktree", data="".join(entries)).stdout.strip()
        args = ["-c", "user.name=Art Queue", "-c", "user.email=art-queue@localhost", "commit-tree", tree]
        if self.sha:
            args += ["-p", self.sha]
        commit = git(self.cache, *args, data="Update shared art queue\n").stdout.strip()
        result = git(self.cache, "push", "--porcelain", "--force-with-lease=" + REF + ":" + (self.sha or ""),
                     self.remote, commit + ":" + REF, check=False)
        if result.returncode:
            raise ValueError("Publish rejected; state unchanged by this operation. Refresh and retry. " + result.stderr.strip())
        self.sha = commit


def require(condition, message):
    if not condition:
        raise ValueError(message)


def mutate(state, a):
    stamp = now()
    if a.command in ("recover-task", "recover-integrator"):
        require(a.evidence.strip(), "Explicit recovery evidence required")
        if a.command == "recover-task":
            require(a.id in state["tasks"], "Unknown task")
            task = state["tasks"][a.id]
            require(task["owner"] == a.from_owner, "Previous owner mismatch")
            require(task["status"] not in ("pendiente", "integrado"), "Task is not recoverable")
            task.update(owner=a.to_owner, status="en_curso", updated_at=stamp)
            task["evidence"].append(dict(at=stamp, owner=a.actor, detail=a.evidence))
        else:
            lock = state["integrator"]
            require(lock and lock["owner"] == a.from_owner, "Previous integrator mismatch")
            state["integrator"] = None
        state.setdefault("recoveries", []).append(dict(at=stamp, actor=a.actor, operation=a.command,
            task=getattr(a, "id", None), from_owner=a.from_owner, to_owner=getattr(a, "to_owner", None),
            expected_commit=a.expected_commit, evidence=a.evidence))
        return
    if a.command == "add":
        require(a.id not in state["tasks"], "Task already exists")
        state["tasks"][a.id] = dict(id=a.id, piece=a.piece, status="pendiente", owner=None,
                                    created_at=stamp, updated_at=stamp, branch=None, pr=None,
                                    approval=a.approval, reference=a.reference, evidence=[])
        return
    if a.command.startswith("integrator-"):
        lock = state["integrator"]
        if a.command == "integrator-acquire":
            require(lock is None, "Integrator already occupied; never steal based on elapsed time")
            state["integrator"] = dict(owner=a.owner, token=secrets.token_hex(24), created_at=stamp, updated_at=stamp)
        else:
            require(lock and lock["owner"] == a.owner and secrets.compare_digest(lock["token"], a.token),
                    "Integrator owner/token mismatch")
            if a.command == "integrator-release":
                state["integrator"] = None
            else:
                lock["updated_at"] = stamp
        return
    require(a.id in state["tasks"], "Unknown task")
    task = state["tasks"][a.id]
    if a.command == "claim":
        require(task["status"] == "pendiente" and task["owner"] is None, "Task already claimed")
        task.update(owner=a.owner, status="en_curso", claimed_at=stamp)
    else:
        require(task["owner"] == a.owner, "Only the task owner can update it")
        status = a.status
        require(status == task["status"] or status in TRANSITIONS.get(task["status"], set()),
                "Invalid state transition")
        if status in ("listo_pr", "pr_abierto", "integrado"):
            require(a.approval or task["approval"], "Approval reference required")
        if status in ("pr_abierto", "integrado"):
            require(a.pr or task["pr"], "PR reference required")
        if status == "integrado":
            lock = state["integrator"]
            require(lock and lock["owner"] == (a.integrator_owner or a.owner)
                    and secrets.compare_digest(lock["token"], a.token or ""), "Integrator lock required")
            require(a.evidence, "Integration evidence required (merge commit / PR URL)")
            task["completed_at"] = stamp
        for name in ("branch", "pr", "approval", "reference"):
            if getattr(a, name):
                task[name] = getattr(a, name)
        task["status"] = status
        if a.evidence:
            task["evidence"].append(dict(at=stamp, owner=a.owner, detail=a.evidence))
    task["updated_at"] = stamp


def parser():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--repo", default=".", help="Working repository (only reads its remote URL)")
    p.add_argument("--remote", default="origin", help="Remote name, URL or absolute local bare repository")
    p.add_argument("--state-dir", help="Dedicated bare cache; default is a temporary repository per invocation")
    p.add_argument("--snapshot", help="Also write Markdown snapshot to this local file after success")
    sub = p.add_subparsers(dest="command", required=True)
    sub.add_parser("init", help="Create empty coordination branch; fails if it already exists")
    sub.add_parser("list", help="Read shared queue and render snapshot")
    add = sub.add_parser("add", help="Add pending piece")
    add.add_argument("id")
    add.add_argument("--piece", required=True)
    add.add_argument("--approval")
    add.add_argument("--reference")
    for command in ("claim", "update"):
        s = sub.add_parser(command)
        s.add_argument("id")
        s.add_argument("--owner", required=True)
        if command == "update":
            s.add_argument("--status", choices=STATES, required=True)
            for name in ("branch", "pr", "approval", "reference", "evidence", "token", "integrator-owner"):
                s.add_argument("--" + name)
    for command in ("integrator-acquire", "integrator-touch", "integrator-release"):
        s = sub.add_parser(command)
        s.add_argument("--owner", required=True)
        if command != "integrator-acquire":
            s.add_argument("--token", required=True)
    for command in ("recover-task", "recover-integrator"):
        s = sub.add_parser(command, help="Explicit recovery after checking abandoned work")
        if command == "recover-task":
            s.add_argument("id")
            s.add_argument("--to-owner", required=True)
        for name in ("from-owner", "actor", "evidence", "expected-commit"):
            s.add_argument("--" + name, required=True)
    return p


def main():
    a = parser().parse_args()
    try:
        remote_result = git(a.repo, "remote", "get-url", a.remote, check=False)
        remote = remote_result.stdout.strip() if remote_result.returncode == 0 else a.remote
        # Local remote URLs are relative to the working repo, not to the bare cache.
        if "://" not in remote and not (":" in remote and not Path(remote).is_absolute()):
            remote = str((Path(a.repo) / remote).resolve())
        with tempfile.TemporaryDirectory(prefix="art-queue-") as temporary:
            queue = Queue(remote, a.state_dir or temporary)
            state = queue.read()
            if a.command == "init":
                require(state is None, "Queue already initialized")
                state = dict(version=1, tasks={}, integrator=None, updated_at=now())
            else:
                require(state is not None, "Queue does not exist; run init first")
                if a.command != "list":
                    if a.command.startswith("recover-"):
                        require(a.expected_commit == queue.sha, "Recovery commit changed; review current state first")
                    mutate(state, a)
            if a.command != "list":
                queue.publish(state)
            result = dict(ok=True, commit=queue.sha, state=state)
            if a.snapshot:
                try:
                    Path(a.snapshot).write_text(render(state), encoding="utf-8")
                except OSError as error:
                    # Remote publication already succeeded; do not imply it rolled back.
                    result["snapshot_error"] = str(error)
            print(json.dumps(result, ensure_ascii=False))
        return 0
    except (ValueError, OSError, KeyError) as error:
        print(json.dumps(dict(ok=False, error=str(error)), ensure_ascii=False))
        return 1


if __name__ == "__main__":
    sys.exit(main())
