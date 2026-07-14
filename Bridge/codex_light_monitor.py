#!/usr/bin/env python3
"""Monitor local Codex logs and emit CodexLight states.

States:
  GREEN  - idle, completed, or no recent agent activity
  RED    - Codex is thinking, writing, or running tools
  YELLOW - Codex is waiting for approval or explicit user input

The script uses only Python's standard library by default. If --serial is used,
pyserial must be installed in the Python environment running this script.
"""

from __future__ import annotations

import argparse
import json
import os
import sqlite3
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Iterable, Optional


STATE_GREEN = "GREEN"
STATE_RED = "RED"
STATE_YELLOW = "YELLOW"


@dataclass
class MonitorState:
    state: str = STATE_GREEN
    reason: str = "startup"
    last_activity: float = field(default_factory=time.monotonic)
    last_emit: float = 0.0
    pending_calls: Dict[str, str] = field(default_factory=dict)
    pending_approvals: Dict[str, str] = field(default_factory=dict)
    active_turn_id: Optional[str] = None
    completed_hint_at: Optional[float] = None


class StateEmitter:
    def __init__(self, serial_port: Optional[str], baud: int, repeat: bool) -> None:
        self.repeat = repeat
        self.last_state: Optional[str] = None
        self.serial = None

        if serial_port:
            try:
                import serial  # type: ignore
            except ImportError as exc:
                raise SystemExit(
                    "pyserial is required for --serial. Install it with: pip install pyserial"
                ) from exc
            self.serial = serial.Serial(serial_port, baudrate=baud, timeout=1)

    def emit(self, state: str, reason: str) -> None:
        if not self.repeat and state == self.last_state:
            return

        self.last_state = state
        stamp = time.strftime("%Y-%m-%d %H:%M:%S")
        line = f"{stamp} {state:<6} {reason}"
        print(line, flush=True)

        if self.serial is not None:
            self.serial.write((state + "\n").encode("ascii"))
            self.serial.flush()


def default_sessions_root() -> Path:
    return Path.home() / ".codex" / "sessions"


def default_sqlite_path() -> Path:
    return Path.home() / ".codex" / "logs_2.sqlite"


def iter_jsonl_files(root: Path, max_age_days: int) -> Iterable[Path]:
    if not root.exists():
        return []

    cutoff = time.time() - max_age_days * 24 * 60 * 60
    files = []
    for path in root.rglob("*.jsonl"):
        try:
            if path.stat().st_mtime >= cutoff:
                files.append(path)
        except OSError:
            continue
    return sorted(files, key=lambda p: p.stat().st_mtime)


def read_new_lines(path: Path, offsets: Dict[Path, int], from_start: bool) -> Iterable[str]:
    try:
        size = path.stat().st_size
    except OSError:
        return []

    if path not in offsets:
        offsets[path] = 0 if from_start else size
        return []

    old_offset = offsets[path]
    if size < old_offset:
        old_offset = 0

    if size == old_offset:
        return []

    lines = []
    try:
        with path.open("r", encoding="utf-8", errors="replace") as handle:
            handle.seek(old_offset)
            lines = handle.readlines()
            offsets[path] = handle.tell()
    except OSError:
        return []
    return lines


def json_payload(line: str) -> Optional[dict]:
    try:
        value = json.loads(line)
    except json.JSONDecodeError:
        return None
    return value if isinstance(value, dict) else None


def contains_approval_request(payload: dict) -> bool:
    text = json.dumps(payload, ensure_ascii=False).lower()
    approval_markers = (
        "require_escalated",
        "sandbox_permissions",
        "request_user_input",
        "approval",
        "permission",
    )
    return any(marker in text for marker in approval_markers)


def set_state(ms: MonitorState, state: str, reason: str) -> None:
    ms.state = state
    ms.reason = reason
    ms.last_activity = time.monotonic()
    if state != STATE_GREEN:
        ms.completed_hint_at = None


def handle_session_event(ms: MonitorState, event: dict) -> None:
    event_type = event.get("type")
    payload = event.get("payload") if isinstance(event.get("payload"), dict) else {}
    payload_type = payload.get("type")

    if event_type == "event_msg" and payload_type == "task_started":
        ms.active_turn_id = payload.get("turn_id")
        set_state(ms, STATE_RED, "task_started")
        return

    if event_type == "event_msg" and payload_type == "turn_aborted":
        ms.pending_calls.clear()
        ms.pending_approvals.clear()
        ms.active_turn_id = None
        set_state(ms, STATE_GREEN, "turn_aborted")
        return

    if event_type == "event_msg" and payload_type == "user_message":
        set_state(ms, STATE_RED, "user_message")
        return

    if event_type == "event_msg" and payload_type in {"agent_message", "agent_reasoning", "token_count"}:
        if not ms.pending_approvals:
            set_state(ms, STATE_RED, payload_type)
        return

    if event_type != "response_item":
        return

    if payload_type == "function_call":
        call_id = str(payload.get("call_id") or payload.get("id") or "unknown")
        name = str(payload.get("name") or "function_call")
        ms.pending_calls[call_id] = name

        if contains_approval_request(payload):
            ms.pending_approvals[call_id] = name
            set_state(ms, STATE_YELLOW, f"approval_needed:{name}")
        else:
            set_state(ms, STATE_RED, f"tool_call:{name}")
        return

    if payload_type == "function_call_output":
        call_id = str(payload.get("call_id") or "unknown")
        name = ms.pending_calls.pop(call_id, "tool")
        ms.pending_approvals.pop(call_id, None)

        output = str(payload.get("output") or "")
        if "process exited with code 1" in output.lower() or "error" in output.lower():
            set_state(ms, STATE_RED, f"tool_output_error:{name}")
        elif ms.pending_approvals:
            set_state(ms, STATE_YELLOW, "approval_pending")
        else:
            set_state(ms, STATE_RED, f"tool_output:{name}")
        return

    if payload_type in {"message", "reasoning"}:
        if not ms.pending_approvals:
            set_state(ms, STATE_RED, payload_type)


def connect_sqlite(path: Path) -> Optional[sqlite3.Connection]:
    if not path.exists():
        return None
    uri = f"file:{path}?mode=ro"
    try:
        con = sqlite3.connect(uri, uri=True)
        con.row_factory = sqlite3.Row
        return con
    except sqlite3.Error:
        return None


def sqlite_max_id(con: sqlite3.Connection) -> int:
    try:
        row = con.execute("select coalesce(max(id), 0) as max_id from logs").fetchone()
        return int(row["max_id"] if row else 0)
    except sqlite3.Error:
        return 0


def handle_sqlite_logs(ms: MonitorState, con: sqlite3.Connection, last_id: int) -> int:
    try:
        rows = con.execute(
            "select id, level, target, feedback_log_body from logs "
            "where id > ? order by id asc limit 500",
            (last_id,),
        ).fetchall()
    except sqlite3.Error:
        return last_id

    for row in rows:
        last_id = max(last_id, int(row["id"]))
        level = str(row["level"] or "")
        body = str(row["feedback_log_body"] or "")
        target = str(row["target"] or "")

        if level == "ERROR":
            set_state(ms, STATE_RED, f"sqlite_error:{target}")
            continue

        if body.startswith("app-server event: item/completed"):
            ms.completed_hint_at = time.monotonic()
            ms.reason = "app_server_item_completed"
            continue

        if body.startswith("app-server event: thread/status/changed"):
            ms.completed_hint_at = time.monotonic()
            ms.reason = "thread_status_changed"

    return last_id


def apply_idle_rules(ms: MonitorState, quiet_timeout: float, complete_grace: float) -> None:
    now = time.monotonic()

    if ms.pending_approvals:
        ms.state = STATE_YELLOW
        ms.reason = "approval_pending"
        return

    if ms.pending_calls:
        ms.state = STATE_RED
        ms.reason = "tool_running"
        return

    if ms.completed_hint_at is not None and now - ms.completed_hint_at >= complete_grace:
        ms.state = STATE_GREEN
        ms.reason = "completed_hint"
        ms.active_turn_id = None
        return

    if ms.state != STATE_GREEN and now - ms.last_activity >= quiet_timeout:
        ms.state = STATE_GREEN
        ms.reason = "quiet_timeout"
        ms.active_turn_id = None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Monitor Codex logs for CodexLight.")
    parser.add_argument("--sessions-root", type=Path, default=default_sessions_root())
    parser.add_argument("--sqlite", type=Path, default=default_sqlite_path())
    parser.add_argument("--poll", type=float, default=0.5, help="Polling interval in seconds.")
    parser.add_argument("--max-age-days", type=int, default=2, help="Session files to scan.")
    parser.add_argument(
        "--quiet-timeout",
        type=float,
        default=20.0,
        help="Fallback seconds with no activity before returning to GREEN.",
    )
    parser.add_argument(
        "--complete-grace",
        type=float,
        default=1.5,
        help="Seconds after app-server item/completed before returning to GREEN.",
    )
    parser.add_argument("--from-start", action="store_true", help="Process existing JSONL content.")
    parser.add_argument("--serial", help="Optional serial port, for example COM5.")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--repeat", action="store_true", help="Print/send repeated identical states.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    emitter = StateEmitter(args.serial, args.baud, args.repeat)
    ms = MonitorState()
    offsets: Dict[Path, int] = {}

    con = connect_sqlite(args.sqlite)
    last_sqlite_id = sqlite_max_id(con) if con is not None else 0

    emitter.emit(ms.state, ms.reason)

    while True:
        for path in iter_jsonl_files(args.sessions_root, args.max_age_days):
            for line in read_new_lines(path, offsets, args.from_start):
                event = json_payload(line)
                if event is not None:
                    handle_session_event(ms, event)

        if con is None:
            con = connect_sqlite(args.sqlite)
            last_sqlite_id = sqlite_max_id(con) if con is not None else last_sqlite_id
        else:
            last_sqlite_id = handle_sqlite_logs(ms, con, last_sqlite_id)

        apply_idle_rules(ms, args.quiet_timeout, args.complete_grace)
        emitter.emit(ms.state, ms.reason)
        time.sleep(args.poll)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("\nStopped.", flush=True)
        raise SystemExit(130)
