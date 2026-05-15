---
doc_type: audit-finding
title: AI 约束总入口仍未进入 CI，生产级约束仍停留在本地自觉执行
severity: P1
category: arch-drift
confidence: high
suggested_action: cs-refactor
created_at: 2026-05-12
---

# Finding 03

## Summary

当前仓库已经有本地正式入口 `check-gate.ps1`，但 CI 工作流还没有复用这套入口。也就是说，“规范化总入口”目前只在本地对话和手工流程里生效，还没有成为真实的生产流水线约束。

## Evidence

- `.github/workflows/build.yml:177-228`
  - CI 直接跑 `cmake configure/build/run_tests`，没有调用 `qmclient_scripts/check-gate.ps1`，也没有等价的 workflow 文档检查、baseline debt 分类或 release-note 前置检查。
- `.github/workflows/build.yml`
  - 搜索结果中不存在 `check-gate`、`strict-debug-check`、`fix_style.py` 等统一入口调用。

## Why It Matters

- 这意味着“AI 被约束”在真正提交、tag、发布时并没有被系统性复核。
- 只靠本地自觉执行，很难称为生产级治理；尤其是多人协作、不同环境、不同代理并存时，约束会重新碎片化。
- 一个真正生产级的规则/文档/脚本系统，至少应在 CI 中以某种等价形式得到执行或验证。

## Suggested Fix

- 在 CI 中引入与 `check-gate.ps1` 等价的正式门，优先复用已有模式定义，而不是再抄一套平行规则。
- 如果 Windows 专属脚本暂时不适合直接进跨平台 CI，至少先把 workflow 文档一致性检查、产物状态契约和部分 gate 规则迁成可跨平台执行的层。
