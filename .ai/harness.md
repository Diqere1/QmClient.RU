# Harness Model

This repository applies the practical lessons from `deusyu/harness-engineering` to QmClient. The goal is not to copy that repository's content model; it is to make QmClient easier for agents to operate safely over many sessions.

## 核心原则

### 仓库即记录系统

If it is not in the repository, the agent cannot reliably use it. Durable decisions, feature state, verification evidence, and restart instructions belong in repo files:

- `feature_list.json` for scope and status.
- `progress.md` for verified evidence and current state.
- `session-handoff.md` for the next session's restart path.
- `.ai/*.md` for durable rules.
- `qmclient_scripts/gate/*.py|*.sh` for enforceable checks.

### 地图而非手册

`AGENTS.md` and `CLAUDE.md` are entry maps. They should stay short and point to focused documents. Do not append large histories, feature plans, or one-off fixes to the root files.

Use this placement rule:

| Content | Place |
|---------|-------|
| Stable agent routing | `AGENTS.md` / `CLAUDE.md` |
| DDNet coding rules | `.ai/ddnet-development.md` |
| Review format | `.ai/review.md` |
| Build and test commands | `.ai/verification.md` |
| Session state and evidence | `progress.md` |
| Feature scope/status | `feature_list.json` |
| Next-session restart path | `session-handoff.md` |

### 机械化执行

Rules that can drift should be checked by scripts. `qmclient_scripts/gate/check_workflow_docs.py` is the local harness consistency check. It should fail with actionable messages when:

- Required harness files are missing.
- Root map sections disappear.
- Root maps stop linking to focused documents.
- `AGENTS.md` and `CLAUDE.md` drift.
- `feature_list.json` is malformed or has multiple active features.
- `progress.md`, `session-handoff.md`, or `init.sh` lose required structure.

### 智能体可读性

Prefer boring, explicit, repo-local structures:

- Clear file names over hidden conventions.
- Versioned Markdown over external chat memory.
- Existing DDNet/QmClient patterns over clever new abstractions.
- Stable scripts over prose-only instructions.

### 熵管理

Agents reproduce the patterns they see. Keep bad patterns from becoming attractive examples:

- Delete obsolete rules instead of leaving contradictory instructions.
- Move durable lessons into the right `.ai/` file.
- Add or update mechanical checks when a rule matters repeatedly.
- Keep root maps concise enough that agents actually read them.

## QmClient adaptation

QmClient has extra risk compared with a normal app repository: it is a real-time networked game client derived from DDNet. Harness rules must therefore protect compatibility and runtime behavior first.

Implications:

- Protocol, demo, skin, map, physics, prediction, snapshot, input, collision, timing, and replay changes need explicit user approval.
- Visual/client-only work still needs build and regression evidence.
- Hot-path changes must be reviewed for allocations, repeated layout, sorting, serialization, and per-frame/per-tick cost.
- Version bumps follow the user's MMP rule after a complete feature or improvement.
