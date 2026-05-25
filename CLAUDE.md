# AGENTS.md / CLAUDE.md

QmClient is a customized DDNet/TaterClient fork. This file is the agent map for the repository, not the full manual. Keep it short, then load the focused documents below only when the task needs them.

## 入口原则

- The repository is the record system: decisions, plans, feature state, verification evidence, and handoff notes belong in versioned files.
- This file is a map, not a manual. Do not append long task history or one-off fixes here; put durable details in the right `.ai/` document or in `progress.md`.
- Work on one feature at a time. Use `feature_list.json` as the scope boundary and `progress.md` / `session-handoff.md` as continuity state.
- If a feature request is ambiguous, ask until the behavior, scope, and compatibility boundary are clear.
- Read real code before changing behavior. Prefer local patterns and DDNet compatibility over generic modern C++ preferences.

## 启动顺序

1. Read this file.
2. Read `feature_list.json` and identify the active or highest-priority unfinished feature.
3. Read `progress.md` and `session-handoff.md` for verified state, blockers, and the next action.
4. Run `./init.sh` when the shell environment supports bash. If it is too expensive for the current turn, at minimum run `python qmclient_scripts/gate/check_workflow_docs.py`.
5. Read the focused `.ai/` document that matches the task.
6. Inspect nearby source, call sites, config variables, translations, and tests before editing.

## 文档地图

| Path | When to read |
|------|--------------|
| `.ai/harness.md` | Harness principles adapted from `deusyu/harness-engineering`: repository record system, map-not-manual, mechanical enforcement, and entropy control. |
| `.ai/session-lifecycle.md` | Starting, selecting one feature, executing, verifying, updating state, and leaving a resumable handoff. |
| `.ai/ddnet-development.md` | DDNet/QmClient C++ rules, compatibility constraints, style, ownership, performance, and risk boundaries. |
| `.ai/verification.md` | Build, test, quick/default/full gate commands, visual checks, and what counts as evidence. |
| `.ai/review.md` | Code review stance, severity format, DDNet-specific risk areas, and output format. |
| `.ai/reference.md` | Detailed routing for scripts, PR validation, release notes, and workflow document maintenance. |
| `qmclient_scripts/gate/check-gate-workflow.md` | Gate script semantics, mode split, allowlist behavior, and report format. |
| `qmclient_scripts/脚本总览.md` | Script inventory and which script to use for each maintenance task. |

## 全局硬约束

- Protect DDNet compatibility: do not change protocol, demo/skin formats, physics, prediction, collision, map behavior, rank reachability, or existing gameplay semantics without explicit approval.
- Keep patches focused. Do not rewrite unrelated upstream DDNet code or introduce broad abstractions for small changes.
- QmClient-specific work normally belongs in `src/game/client/components/qmclient/`, `src/game/client/QmUi/`, QmClient config headers, translations, docs, metadata, and `qmclient_scripts/`.
- Out-of-scope areas need explicit user approval: upstream engine core, server gameplay, map editor, third-party libraries, CI release workflow, protocol fields, physics, prediction, snapshots, inputs, collision, timing, and replay semantics.
- When completing a full feature or improvement, update the QmClient version by the MMP rule unless the user explicitly limits the task to investigation or text-only output.
- After implementation and user confirmation, provide commit notes grouped as `FEAT`, `FIX`, and `DEL`.

## 机械化入口

Use scripts instead of relying on memory:

```bash
python qmclient_scripts/gate/check_workflow_docs.py
bash qmclient_scripts/gate/check-gate.sh --mode quick --base-ref main
bash qmclient_scripts/gate/check-gate.sh --mode default --base-ref main
```

When changing `AGENTS.md`, `CLAUDE.md`, `.ai/`, workflow scripts, or governance CI, run:

```bash
python qmclient_scripts/gate/sync_agents_claude.py --prefer agents
python qmclient_scripts/gate/check_workflow_docs.py
```

Before closing a feature, record evidence in `progress.md`, update `feature_list.json`, refresh `session-handoff.md`, and run the relevant build/test gate from `.ai/verification.md`.
