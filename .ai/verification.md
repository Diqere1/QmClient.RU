# Verification

Use the narrowest verification that covers the risk of the change, then record evidence in `progress.md`.

## Harness and docs

```bash
python qmclient_scripts/gate/check_workflow_docs.py
```

Run this after changing `AGENTS.md`, `CLAUDE.md`, `.ai/`, `feature_list.json`, `progress.md`, `session-handoff.md`, `init.sh`, governance workflow files, or gate scripts.

## Build

Windows recommended:

```bat
qmclient_scripts\cmake-windows.cmd -S . -B cmake-build-release
qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10
```

Linux/macOS:

```sh
cmake -S . -B cmake-build-release
cmake --build cmake-build-release --target game-client -j 10
```

## Tests

Windows:

```bat
qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target run_cxx_tests
qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target run_rust_tests
```

Linux/macOS:

```sh
cmake --build cmake-build-release --target run_cxx_tests
cmake --build cmake-build-release --target run_rust_tests
```

## Gate modes

```bash
bash qmclient_scripts/gate/check-gate.sh --mode quick --base-ref main
bash qmclient_scripts/gate/check-gate.sh --mode default --base-ref main
bash qmclient_scripts/gate/check-gate.sh --mode full --base-ref main
```

| Mode | Use |
|------|-----|
| `quick` | Fast governance: config variables, header guards, style, workflow docs. |
| `default` | Daily pre-commit: quick plus strict debug/static analysis and C++ tests. |
| `full` | Release-style: default plus heavier checks and Rust tests. |

Strict debug only:

```bash
bash qmclient_scripts/gate/strict-debug-check.sh --base-ref main
```

## Visual changes

For menus, HUD, UI widgets, browser rows, settings, overlays, and animations:

- Build the client.
- Launch `DDNet.exe`.
- Verify the target screen at normal UI scale and at least one non-default scale if the layout is scale-sensitive.
- Check hover, selected, disabled, modal, keyboard, and controller paths if relevant.
- Capture screenshots when preparing a PR or visual handoff.

## Evidence format

Record:

```text
Command: <exact command>
Result: <pass/fail and key output>
Scope: <what this proves>
Gaps: <what was not verified>
```

Do not mark a feature `done` without evidence. If a check cannot run because of environment or time, record that as a gap.
