# 会话生命周期

QmClient 的智能体会话请遵循这个生命周期。它能让长时任务可恢复，也能防止范围漂移。

## 启动

1. Read `AGENTS.md` / `CLAUDE.md`.
2. Read `.ai/feature_list.json`.
3. Read `.ai/progress.md`.
4. Read `.ai/session-handoff.md`.
5. 条件允许时运行 `bash qmclient_scripts/init.sh`。如果不方便，运行 `python qmclient_scripts/gate/check_docs.py`。
6. Check `git status --short` and treat unrelated changes as user work.

## 选定任务

- 一次只做一个功能，或者一个用户明确请求的修复。
- 优先选择 `.ai/feature_list.json` 里已经是 `in-progress` 的功能。
- 如果当前没有活跃功能能匹配用户请求，先把任务定义清楚，再更新状态文件。
- 如果请求有歧义，先问清楚再实现。

## 执行

- 修改前先读附近源码、调用点、配置变量、翻译和测试。
- 把补丁限制在最小安全改动面。
- 没有明确批准，不要改高风险的 DDNet 行为。
- 重要决定一旦稳定下来，就记到 `.ai/progress.md`。

## 验证

用 `.ai/verification.md` 来选取合适的检查。

证据应包括：

- Exact command.
- Result.
- Build/test warnings if any.
- Known unverified areas, especially visual checks.

## 收尾

在结束一个较大的会话前：

1. 用完成内容、证据、阻塞项和下一步更新 `.ai/progress.md`。
2. 如果功能状态或证据变了，更新 `.ai/feature_list.json`。
3. 用当前目标、已改文件、已知风险和精确恢复命令更新 `.ai/session-handoff.md`。
4. 让工作树保持可理解。不要回退无关的用户改动。
5. 如果功能已完成，确认版本元数据遵循 MMP 规则，除非用户明确把任务范围排除在代码改动之外。

## 完成态要求

任务不是“代码改了”就算完成。完成意味着行为已实现，并且有对应验证证据。如果验证无法运行，要在 `.ai/progress.md` 和最终回复里明确说明。
