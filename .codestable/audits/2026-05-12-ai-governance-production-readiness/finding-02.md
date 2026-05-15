---
doc_type: audit-finding
title: refresh_baseline_debt_allowlist.py 会无差别放行当前报告中的所有 baseline FAIL
severity: P1
category: maintainability
confidence: high
suggested_action: cs-refactor
created_at: 2026-05-12
---

# Finding 02

## Summary

`refresh_baseline_debt_allowlist.py` 当前采用“从报告里把所有 baseline_debt 失败全量抄进 allowlist”的策略，没有和已有 allowlist 做 diff，也没有要求操作者确认“哪些是历史债务、哪些其实是这次新引入的失败”。这会把 refresh 脚本从“初始化工具”变成“潜在的一键漂白工具”。

## Evidence

- `qmclient_scripts/refresh_baseline_debt_allowlist.py:22-38`
  - 对 `report["Items"]` 中所有 `CategoryId == "baseline_debt"` 且失败的项，直接收集为新的 allowlist entry。
- `qmclient_scripts/refresh_baseline_debt_allowlist.py:40-45`
  - 输出时直接重写整个 allowlist，没有保留旧条目来源、没有新增/删除 diff、没有人工确认环节。

## Why It Matters

- 只要有人在一个脏工作树里跑了 refresh，新引入的格式/规则回归也会被立即“合法化”。
- 对 AI 约束系统来说，这等于把“历史债务”与“当前改坏了”重新混回一起，只是换成了另一种静默方式。
- 这类工具一旦进入日常工作流，最危险的不是报错，而是错误地成功。

## Suggested Fix

- 刷新脚本至少应输出“新增条目 / 已存在条目 / 将被删除条目”的 diff，而不是直接无差别覆盖。
- 最好要求显式确认或增加 `--merge` / `--rewrite` 区分，默认只生成候选文件或 dry-run 报告。
- 更稳妥的做法是把 allowlist 绑定到一个已知基线快照，而不是任何一次当前工作树报告。
