---
doc_type: audit-finding
title: workflow 文档检查仍主要验证字符串存在性，尚未形成真正的同步契约
severity: P2
category: arch-drift
confidence: high
suggested_action: cs-refactor
created_at: 2026-05-12
---

# Finding 04

## Summary

这次已经把 `workflow-manifest.json` 引进来了，但 `check_workflow_docs.py` 仍主要做“少量片段是否存在”的检查，离真正的同步契约还有距离。它可以发现明显断链，却还不能证明 `AGENTS.md`、`Claude.md`、`attention.md`、`workflow-entry.md` 在结构和职责边界上真的保持一致。

## Evidence

- `qmclient_scripts/check_workflow_docs.py:97-123`
  - `AGENTS`、`Claude`、`attention` 的检查仍是 `ordered_contains` 或 snippet presence，不能发现更大范围的内容漂移。
- `qmclient_scripts/check_workflow_docs.py:137-148`
  - 路由完整性只校验 `workflow-entry.md` 是否包含 manifest 列出的引用，不会反向确认 manifest 是否已覆盖新入口类型。
- `qmclient_scripts/check_workflow_docs.py:151-156`
  - 仍保留一个 release notes 的硬编码特判，说明检查器尚未完全收口到统一 schema。

## Why It Matters

- 这代表当前系统更像“入口体检”，还不是“契约校验”。
- 一旦以后继续扩展新的专项文档或入口职责，检查器很可能继续绿，但实际规则分层已经开始漂移。
- 这不至于立刻造成错误结果，但会逐步侵蚀“AI 应该先读什么、按什么边界行动”的稳定性。

## Suggested Fix

- 把 manifest 继续结构化，至少覆盖：
  - 各入口文件必须出现的职责分层
  - 哪些专项文档必须被哪个入口引用
  - 哪些检查仍是临时特判，计划何时回收
- 尽量减少 Python 里的硬编码 one-off 规则，让校验器本身也遵循同一套声明式来源。
