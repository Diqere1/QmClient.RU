---
doc_type: audit-finding
title: check-gate 仍建立在红色基线之上，尚不是可依赖的生产级 AI gate
severity: P1
category: arch-drift
confidence: high
suggested_action: cs-refactor
created_at: 2026-05-12
---

# Finding 04

## Summary

`check-gate.ps1` 已经把失败分成 `环境/工具`、`仓库基线债务`、`当前改动/构建阻断`，但它对 baseline debt 仍然直接记为 `FAIL` 并最终 `exit 1`。这意味着仓库 baseline 本来就红时，AI 即使没有引入新问题，也拿不到真正的绿色门禁结果。

## Evidence

- `qmclient_scripts/check-gate.ps1:89-103`
  - baseline debt 只做分类，没有配套 allowlist / snapshot / debt manifest。
- `qmclient_scripts/check-gate.ps1:1161-1179`
  - `quick` 模式默认会执行这些源码卫生层检查。
- `qmclient_scripts/check-gate.ps1:1220-1222`
  - 任何 `FAIL` 都会直接退出 1。
- 本次实际运行 `check-gate.ps1 -Mode quick -BaseRef main`
  - 工作流文档一致性检查通过，但仍因配置变量、header guard、format 的历史债务整体返回失败。

## Why It Matters

- 对“约束 AI”的系统来说，最关键的是让自动体知道“这次改动坏了什么”。
- 如果默认 gate 永远红，AI 只能靠人工解释“这些红是历史债务，先别管”，这不算生产级门禁。

## Suggested Fix

- 引入 baseline debt manifest / allowlist / snapshot：
  - 已知历史债务列入清单
  - 当前改动只有在新增债务时才升级为 `FAIL`
  - 旧债务保留可见，但不阻断与本次无关的提交
- 或者让 JSON 报告显式区分：
  - `blocking_failures`
  - `baseline_debt_failures`
  - `environment_failures`
