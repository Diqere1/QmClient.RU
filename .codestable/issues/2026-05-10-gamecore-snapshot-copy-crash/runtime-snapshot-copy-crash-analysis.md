---
doc_type: issue-analysis
issue: 2026-05-10-gamecore-snapshot-copy-crash
status: confirmed
root_cause_type: state-pollution
related: [runtime-snapshot-copy-crash-report.md]
tags: [gamecore, snapshot, crash, prediction]
---

# GameCore 快照复制崩溃根因分析

## 1. 根因

`CCharacterCore` 现在包含 `std::set<int> m_AttachedPlayers`，但客户端侧仍有“把 core 当快照对象整对象复制并长期持有”的路径。这个对象一旦被后续 `Reset()` 或 `clear()` 触发，就会把 Debug STL 的容器状态暴露出来，最终在 `std::_Tree_val<...>::_Orphan_ptr` 处崩溃。

## 2. 关键修复点

- `src/game/gamecore.h`
- `src/game/gamecore.cpp`
- `src/game/client/prediction/entities/character.h`
- `src/game/client/components/tclient/fast_practice.cpp`

修复策略是新增显式 `SnapshotCopy()`，只复制快照需要的状态字段，不再复制 `m_AttachedPlayers` 这类运行时关系字段。

## 3. 验证

- `src/test/gamecore_test.cpp` 增加了 `SnapshotCopy()` 回归测试。
- `testrunner` 目标通过。

