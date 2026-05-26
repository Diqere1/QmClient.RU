# QmClient Harness 参考

这个文件是 `AGENTS.md` / `CLAUDE.md` 那张简短根导航图背后的详细路由表。

## 任务路由

读取与任务最匹配、范围最窄的文档：

| 任务 | 优先文档 |
|------|----------------|
| Agent rules, state files, harness shape | `.ai/harness.md` |
| Starting or ending a long session | `.ai/session-lifecycle.md` |
| C++ implementation in DDNet/QmClient | `.ai/ddnet-development.md` |
| Build, test, gate, visual verification | `.ai/verification.md` |
| Code review | `.ai/review.md` |
| Gate script semantics | `qmclient_scripts/gate/check-gate-workflow.md` |
| Script inventory | `qmclient_scripts/脚本总览.md` |

状态文件：

- `.ai/feature_list.json`：功能范围、依赖和状态的事实源。
- `.ai/progress.md`：已验证的会话证据和当前状态。
- `.ai/session-handoff.md`：下一次会话的精简恢复路径。
- `qmclient_scripts/init.sh`：轻量 harness 健康检查和可选的更重 gate。

## 提交前验证

在宣称代码改动完成之前，最少要满足：

- 相关构建通过，且没有新增警告。
- 相关测试通过。
- 如果任务改了 harness 文件，`python qmclient_scripts/gate/check_docs.py` 通过。
- 如果任务改了 C/C++ 源码，运行 `.ai/verification.md` 里对应的 gate。
- 证据记录进 `.ai/progress.md`，如状态有变则更新 `.ai/feature_list.json`。

Gate 对照：

| 需求 | 命令 |
|------|---------|
| Harness/document consistency | `python qmclient_scripts/gate/check_docs.py` |
| 仅同步 AGENTS / CLAUDE | `python qmclient_scripts/gate/check_docs.py --sync-only --prefer agents` |
| Fast governance check | `python qmclient_scripts/gate/check_gate.py --mode quick --base-ref main` |
| Daily pre-commit gate | `python qmclient_scripts/gate/check_gate.py --mode default --base-ref main` |
| Full release-style gate | `python qmclient_scripts/gate/check_gate.py --mode full --base-ref main` |

脚本入口：

- `qmclient_scripts/gate/check_gate.py`
- `qmclient_scripts/gate/check_docs.py`
- `qmclient_scripts/generate_release_notes.py`

## PR 审查清单

完整审查格式见 `.ai/review.md`。最少检查这些点：

- 与需求行为是否一致的正确性。
- 未定义行为、内存生命周期、迭代器失效，以及空值/边界处理。
- DDNet 兼容性：协议、demo/skin 格式、地图行为、物理、预测、snapshot、输入、回放和 rank。
- 热路径成本：每帧、每 tick、每玩家、每实体的工作量、分配、重复排序、文本布局、序列化或网络带宽。
- API 稳定性，以及补丁是否遵循现有本地模式。
- 测试和视觉证据。

## 文档维护原则

这套 harness 遵循 `deusyu/harness-engineering` 的思路：

- `AGENTS.md` / `CLAUDE.md` 是导航图，不是手册。
- 长期规则放在聚焦的 `.ai/` 文档里。
- 易变状态放在 `.ai/feature_list.json`、`.ai/progress.md` 和 `.ai/session-handoff.md`。
- 必须用机械化检查防漂移，不要依赖人的记忆。

修改 harness 文件时：

```bash
python qmclient_scripts/gate/check_docs.py --sync-only --prefer agents
python qmclient_scripts/gate/check_docs.py
```

不要只手动更新 `AGENTS.md` 或 `CLAUDE.md` 其中一个。要么同步修改两者，要么使用脚本同步。

## 发布说明模板

只有在已经有 gate report 之后，才使用发布说明生成器：

```bash
python qmclient_scripts/generate_release_notes.py --gate-report tmp/check-gate-report.json
```

发布说明的输入项期望如下：

| 字段 | 来源 |
|-------|--------|
| Highlights | 功能验收说明或 `.ai/progress.md` 里的证据 |
| Added | `FEAT` commit notes 或已验收功能记录 |
| Fixed | `FIX` commit notes 或修复证据 |
| Removed | `DEL` commit notes |
| Compatibility | Manual notes for protocol, file format, map, physics, prediction, demo, or config impacts |
| Maintainer notes | Manual reviewer guidance and known risks |
