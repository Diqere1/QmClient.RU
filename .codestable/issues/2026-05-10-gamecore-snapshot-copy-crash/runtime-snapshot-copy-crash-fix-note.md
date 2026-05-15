---
doc_type: issue-fix-note
issue: 2026-05-10-gamecore-snapshot-copy-crash
status: done
related: [runtime-snapshot-copy-crash-report.md, runtime-snapshot-copy-crash-analysis.md]
tags: [gamecore, snapshot, crash, prediction]
---

# GameCore 快照复制崩溃修复说明

## 修复内容

- 为 `CCharacterCore` 增加显式 `SnapshotCopy()`。
- 客户端快照持久化路径改为只走 snapshot copy。
- 不再复制 `m_AttachedPlayers` 这类运行时关系字段。

## 验证

- `build-debug` 下的定向测试覆盖已补。
- `testrunner` 目标通过。

## 备注

这次修复只处理当前确认的快照污染问题，不代表所有运行时崩溃都已根治。
