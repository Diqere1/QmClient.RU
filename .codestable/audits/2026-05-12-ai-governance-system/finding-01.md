---
doc_type: audit-finding
title: 工作流文档一致性检查仍是浅层 presence check
severity: P1
category: maintainability
confidence: high
suggested_action: cs-refactor
created_at: 2026-05-12
---

# Finding 01

## Summary

`check_workflow_docs.py` 目前只能证明“少数固定文件存在，且少量字符串还在”，还不能证明整套规则真的同步一致，因此它不足以承担生产级的工作流契约检查。

## Evidence

- `qmclient_scripts/check_workflow_docs.py:64-76`
  - 只检查一个硬编码文件白名单。
- `qmclient_scripts/check_workflow_docs.py:92-145`
  - 对 `AGENTS.md`、`Claude.md`、`attention.md` 只做少量 snippet presence 校验。
- `qmclient_scripts/check_workflow_docs.py:128-145`
  - 只从 `workflow-entry.md` 提取引用，不会反向检查 `AGENTS.md` / `Claude.md` / `attention.md` 是否遗漏新入口。

## Why It Matters

- 这会把“存在文件”和“内容真的同步”混为一谈。
- 一旦后续有人新增专项文档但忘了挂入口，这个检查很可能仍然通过。
- 对 AI 约束系统来说，这意味着“脚本显示绿色”不等于“AI 真能读到正确入口”。

## Suggested Fix

- 把文档入口检查升级成显式 schema：
  - 根入口必须包含哪些节
  - 哪些专项文档必须在 `workflow-entry.md` 出现
  - `Claude.md` 与 `AGENTS.md` 的同步范围应做结构化比对，而不是只看 3 个字符串
- 最好引入一个 `workflow manifest`，由脚本读取 manifest 再校验文件与入口，而不是把规则硬编码在 Python 里。
