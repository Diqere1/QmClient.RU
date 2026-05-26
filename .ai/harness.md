# Harness 模型

这个仓库把 `deusyu/harness-engineering` 的实践经验应用到了 QmClient 上。目标不是照搬那个仓库的内容模型，而是让 QmClient 在多轮会话里更容易被智能体稳定、安全地操作。

## 核心原则

### 仓库即记录系统

如果信息不在仓库里，智能体就无法可靠地使用它。长期有效的决策、功能状态、验证证据和重启说明都应该放进仓库文件：

- `.ai/feature_list.json` for scope and status.
- `.ai/progress.md` for verified evidence and current state.
- `.ai/session-handoff.md` for the next session's restart path.
- `.ai/*.md` for durable rules.
- `qmclient_scripts/gate/*.py|*.sh` for enforceable checks.

### 地图而非手册

`AGENTS.md` 和 `CLAUDE.md` 是入口导航图。它们应该保持简短，并指向更聚焦的文档。不要把冗长历史、功能计划或一次性修复堆到根文件里。

内容放置遵循这条规则：

| 内容 | 放置位置 |
|---------|-------|
| Stable agent routing | `AGENTS.md` / `CLAUDE.md` |
| DDNet coding rules | `.ai/ddnet-development.md` |
| Review format | `.ai/review.md` |
| Build and test commands | `.ai/verification.md` |
| Session state and evidence | `.ai/progress.md` |
| Feature scope/status | `.ai/feature_list.json` |
| Next-session restart path | `.ai/session-handoff.md` |

### 机械化执行

容易漂移的规则应该由脚本检查。`qmclient_scripts/gate/check_docs.py` 就是本地 harness 一致性检查。出现下面这些情况时，它应该带着可执行的报错失败：

- Required harness files are missing.
- Root map sections disappear.
- Root maps stop linking to focused documents.
- `AGENTS.md` and `CLAUDE.md` drift.
- `.ai/feature_list.json` is malformed or has multiple active features.
- `.ai/progress.md`, `.ai/session-handoff.md`, or `qmclient_scripts/init.sh` lose required structure.

### 智能体可读性

优先选择朴素、显式、仓库内可见的结构：

- Clear file names over hidden conventions.
- Versioned Markdown over external chat memory.
- Existing DDNet/QmClient patterns over clever new abstractions.
- Stable scripts over prose-only instructions.

### 熵管理

智能体会复现它看到的模式。要避免让坏模式变成“看起来可照抄的示例”：

- Delete obsolete rules instead of leaving contradictory instructions.
- Move durable lessons into the right `.ai/` file.
- Add or update mechanical checks when a rule matters repeatedly.
- Keep root maps concise enough that agents actually read them.

## QmClient 适配

和普通应用仓库相比，QmClient 的额外风险更高：它是一个基于 DDNet 的实时联网游戏客户端。因此 harness 规则首先要保护兼容性和运行时行为。

这意味着：

- Protocol, demo, skin, map, physics, prediction, snapshot, input, collision, timing, and replay changes need explicit user approval.
- Visual/client-only work still needs build and regression evidence.
- Hot-path changes must be reviewed for allocations, repeated layout, sorting, serialization, and per-frame/per-tick cost.
- Version bumps follow the user's MMP rule after a complete feature or improvement.
