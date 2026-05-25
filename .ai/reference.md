# QmClient Harness Reference

This file is the detailed routing table behind the short root map in `AGENTS.md` / `CLAUDE.md`.

## 任务路由

Read the narrowest document that matches the task:

| Task | First document |
|------|----------------|
| Agent rules, state files, harness shape | `.ai/harness.md` |
| Starting or ending a long session | `.ai/session-lifecycle.md` |
| C++ implementation in DDNet/QmClient | `.ai/ddnet-development.md` |
| Build, test, gate, visual verification | `.ai/verification.md` |
| Code review | `.ai/review.md` |
| Gate script semantics | `qmclient_scripts/gate/check-gate-workflow.md` |
| Script inventory | `qmclient_scripts/脚本总览.md` |

State files:

- `feature_list.json`: source of truth for feature scope, dependencies, and status.
- `progress.md`: verified session evidence and current state.
- `session-handoff.md`: concise restart path for the next session.
- `init.sh`: lightweight harness health check and optional heavier gates.

## 提交前验证

Minimum before claiming a code change is complete:

- The relevant build passes with no new warnings.
- Relevant tests pass.
- If the task changes harness files, `python qmclient_scripts/gate/check_workflow_docs.py` passes.
- If the task changes C/C++ source, run the appropriate gate from `.ai/verification.md`.
- Evidence is recorded in `progress.md`, and `feature_list.json` is updated if status changes.

Gate mapping:

| Need | Command |
|------|---------|
| Harness/document consistency | `python qmclient_scripts/gate/check_workflow_docs.py` |
| Fast governance check | `bash qmclient_scripts/gate/check-gate.sh --mode quick --base-ref main` |
| Daily pre-commit gate | `bash qmclient_scripts/gate/check-gate.sh --mode default --base-ref main` |
| Full release-style gate | `bash qmclient_scripts/gate/check-gate.sh --mode full --base-ref main` |
| Strict debug/static analysis only | `bash qmclient_scripts/gate/strict-debug-check.sh --base-ref main` |

Script facts:

- `qmclient_scripts/gate/check-gate.sh`
- `qmclient_scripts/gate/check_workflow_docs.py`
- `qmclient_scripts/gate/strict-debug-check.sh`
- `qmclient_scripts/generate_release_notes.py`

## PR 审查清单

Use `.ai/review.md` for the full review format. At minimum, check:

- Correctness against the requested behavior.
- Undefined behavior, memory lifetime, iterator invalidation, and null/ bounds handling.
- DDNet compatibility: protocol, demo/skin formats, map behavior, physics, prediction, snapshots, input, replay, and ranks.
- Hot-path cost: per-frame, per-tick, per-player, per-entity work, allocations, repeated sorting, text layout, serialization, or network bandwidth.
- API stability and whether the patch follows the existing local pattern.
- Test and visual evidence.

## 文档维护原则

The harness follows the `deusyu/harness-engineering` approach:

- `AGENTS.md` / `CLAUDE.md` are maps, not manuals.
- Long-lived rules live in focused `.ai/` documents.
- Volatile state lives in `feature_list.json`, `progress.md`, and `session-handoff.md`.
- Mechanical checks must guard drift; do not rely on human memory.

When editing harness files:

```bash
python qmclient_scripts/gate/sync_agents_claude.py --prefer agents
python qmclient_scripts/gate/check_workflow_docs.py
```

Do not manually update only one of `AGENTS.md` and `CLAUDE.md`. Keep them synchronized through the script or by editing both identically.

## 发布说明模板

Use the release note generator only after there is a gate report:

```bash
python qmclient_scripts/generate_release_notes.py --gate-report tmp/check-gate-report.json
```

Expected release note inputs:

| Field | Source |
|-------|--------|
| Highlights | Feature acceptance notes or `progress.md` evidence |
| Added | `FEAT` commit notes or accepted feature records |
| Fixed | `FIX` commit notes or bugfix evidence |
| Removed | `DEL` commit notes |
| Compatibility | Manual notes for protocol, file format, map, physics, prediction, demo, or config impacts |
| Maintainer notes | Manual reviewer guidance and known risks |
