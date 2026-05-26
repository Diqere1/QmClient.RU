# AGENTS.md / CLAUDE.md

QmClient 是一个定制化的 DDNet/TaterClient 分支。这个文件是仓库的智能体导航图，不是完整手册。请保持简短，并且只在任务需要时再读取下方对应的聚焦文档。

## 入口原则

- 仓库就是记录系统：决策、计划、功能状态、验证证据和交接说明都应该写进版本化文件。
- 这个文件是导航图，不是手册。不要在这里追加冗长的任务历史或一次性修复说明；把长期有效的信息放进对应的 `.ai/` 文档或 `.ai/progress.md`。
- 一次只处理一个功能。用 `.ai/feature_list.json` 作为范围边界，用 `.ai/progress.md` / `.ai/session-handoff.md` 作为连续状态。
- 如果功能请求有歧义，先问清行为、范围和兼容性边界，再开始实现。
- 改行为之前先读真实代码。优先遵循本地模式和 DDNet 兼容性，而不是套用泛化的现代 C++ 偏好。

## 启动顺序

1. Read this file.
2. Read `.ai/feature_list.json` and identify the active or highest-priority unfinished feature.
3. Read `.ai/progress.md` and `.ai/session-handoff.md` for verified state, blockers, and the next action.
4. 如果当前 shell 支持 bash，就运行 `bash qmclient_scripts/init.sh`。如果这一轮成本太高，至少运行 `python qmclient_scripts/gate/check_docs.py`。
5. 读取与任务匹配的聚焦 `.ai/` 文档。
6. 修改前检查附近源码、调用点、配置变量、翻译和测试。

## 文档地图

| 路径 | 何时阅读 |
|------|--------------|
| `.ai/harness.md` | 读取 harness 核心原则：仓库记录系统、地图而非手册、机械化约束、熵管理。 |
| `.ai/session-lifecycle.md` | 读取会话生命周期：启动、选定单一功能、执行、验证、更新状态、留下可恢复交接。 |
| `.ai/ddnet-development.md` | 读取 DDNet/QmClient 的 C++ 规则、兼容性约束、风格、所有权、性能和风险边界。 |
| `.ai/verification.md` | 读取构建、测试、quick/default/full gate 命令、视觉检查和证据标准。 |
| `.ai/review.md` | 读取代码审查立场、严重级别格式、DDNet 特有风险点和输出格式。 |
| `.ai/reference.md` | 读取脚本入口、PR 验证、发布说明和工作流文档维护的详细路由。 |
| `qmclient_scripts/gate/check-gate-workflow.md` | 读取 gate 脚本语义、模式拆分、allowlist 行为和报告格式。 |
| `qmclient_scripts/脚本总览.md` | 读取脚本清单，以及每类维护任务该用哪一个脚本。 |

## 全局硬约束

- Protect DDNet compatibility: do not change protocol, demo/skin formats, physics, prediction, collision, map behavior, rank reachability, or existing gameplay semantics without explicit approval.
- Keep patches focused. Do not rewrite unrelated upstream DDNet code or introduce broad abstractions for small changes.
- QmClient-specific work normally belongs in `src/game/client/components/qmclient/`, `src/game/client/QmUi/`, QmClient config headers, translations, docs, metadata, and `qmclient_scripts/`.
- Out-of-scope areas need explicit user approval: upstream engine core, server gameplay, map editor, third-party libraries, CI release workflow, protocol fields, physics, prediction, snapshots, inputs, collision, timing, and replay semantics.
- Windows 上默认用 `qmclient_scripts\cmake-windows.cmd` 作为构建入口；只有已确认当前 shell 已注入可用的 VS/MSVC 环境时，才直接使用裸 `cmake`。
- When completing a full feature or improvement, update the QmClient version by the MMP rule unless the user explicitly limits the task to investigation or text-only output.
- After implementation and user confirmation, provide commit notes grouped as `FEAT`, `FIX`, and `DEL`.

## 机械化入口

优先用脚本，不要依赖记忆：

```bash
python qmclient_scripts/gate/check_docs.py
python qmclient_scripts/gate/check_gate.py --mode quick --base-ref main
python qmclient_scripts/gate/check_gate.py --mode default --base-ref main
```

修改 `AGENTS.md`、`CLAUDE.md`、`.ai/`、workflow 脚本或 governance CI 后，运行：

```bash
python qmclient_scripts/gate/check_docs.py --sync-only --prefer agents
python qmclient_scripts/gate/check_docs.py
```

关闭一个功能前，把证据写入 `.ai/progress.md`，更新 `.ai/feature_list.json`，刷新 `.ai/session-handoff.md`，并运行 `.ai/verification.md` 里对应的构建/测试 gate。
