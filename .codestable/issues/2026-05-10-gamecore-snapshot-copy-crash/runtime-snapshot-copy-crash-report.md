---
doc_type: issue-report
issue: 2026-05-10-gamecore-snapshot-copy-crash
status: confirmed
severity: P1
summary: 进入服务器或打开聊天框时崩溃，栈落到 std::_Tree_val<...>::_Orphan_ptr
tags: [gamecore, snapshot, crash, prediction]
---

# GameCore 快照复制崩溃 Issue Report

## 1. 问题现象

进入游戏或打开聊天框时，客户端会直接崩溃。WinDbg 栈顶落在 `std::_Tree_val<...>::_Orphan_ptr`，异常为 `0xC0000005` 读 `0x8`。

## 2. 复现步骤

1. 启动当前工作树构建。
2. 进入服务器。
3. 或在游戏中打开聊天框。
4. 客户端崩溃。

复现频率：100%。

## 3. 期望 vs 实际

**期望行为**：正常进入游戏并打开聊天框。

**实际行为**：进入服务器或打开聊天框时触发崩溃。

## 4. 备注

- 这不是 RmlUI 灰层 issue 的同一个根因。
- 当前证据指向 `CCharacterCore` 的快照复制路径污染了 `std::set<int> m_AttachedPlayers`。
