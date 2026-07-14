# CodexLight Bridge

[简体中文](README.md) | English

`codex_light_monitor.py` is the computer-side monitor for CodexLight. It watches local Codex Desktop logs and maps Codex activity to three LED states:

- `GREEN`: idle, completed, or no recent agent activity
- `RED`: thinking, writing, running tools, or taking action
- `YELLOW`: waiting for approval or explicit user input

## Log Sources

Primary source:

```text
C:\Users\<you>\.codex\sessions\YYYY\MM\DD\rollout-*.jsonl
```

The script watches appended JSONL events such as `task_started`, `function_call`, `function_call_output`, `agent_message`, and `turn_aborted`.

Secondary source:

```text
C:\Users\<you>\.codex\logs_2.sqlite
```

The script uses SQLite only as a completion hint by watching app-server diagnostic events such as `app-server event: item/completed`. It does not treat SQLite log text as a stable public API.

## State Rules

| State | Rule |
| --- | --- |
| `GREEN` | startup default, `turn_aborted`, app-server completion hint, or quiet timeout |
| `RED` | `task_started`, agent message/reasoning, tool call, tool output, or SQLite `ERROR` |
| `YELLOW` | a tool-call payload contains approval/user-input markers such as `require_escalated`, `sandbox_permissions`, `request_user_input`, `approval`, or `permission` |

The local Codex Desktop JSONL logs I inspected do not currently expose a confirmed single `task_finished` or `waiting_for_user` event. Because of that, the script returns to `GREEN` using a conservative combination of app-server completion hints and quiet timeout.

## Run

Console-only test:

```powershell
python Bridge\codex_light_monitor.py
```

Process existing recent logs from the beginning, useful for debugging state rules:

```powershell
python Bridge\codex_light_monitor.py --from-start --quiet-timeout 5
```

Send states to an ESP32 over serial:

```powershell
python Bridge\codex_light_monitor.py --serial COM5 --baud 115200
```

Serial output is one ASCII line per state change:

```text
GREEN
RED
YELLOW
```

The firmware can read complete lines from `Serial` and call the matching LED control method.

## Useful Options

```text
--poll 0.5              Poll interval in seconds
--quiet-timeout 20      Fallback seconds with no activity before GREEN
--complete-grace 1.5    Delay after app-server item/completed before GREEN
--repeat                Print/send repeated identical states
```

## Notes

Serial mode requires pyserial in the Python environment used to run the script:

```powershell
pip install pyserial
```

The default console monitoring mode has no third-party dependency.
