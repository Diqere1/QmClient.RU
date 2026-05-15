---
doc_type: audit-finding
title: baseline debt allowlist 仍缺少来源与基线元数据，长期治理能力不足
severity: P2
category: maintainability
confidence: high
suggested_action: cs-refactor
created_at: 2026-05-12
---

# Finding 02

## Summary

`baseline_debt_allowlist.json` 和刷新脚本已经比上一版安全，但 allowlist 条目仍只有 `title/detail_hash/reason` 三元组，没有记录它来自哪个 `BaseRef`、哪次报告、何时加入、为何仍保留。这让它适合当前短期使用，但还不够支撑长期债务治理。

## Evidence

- `qmclient_scripts/baseline_debt_allowlist.json:1-20`
  - 当前条目只有 `title`、`detail_hash`、`reason`。
- `qmclient_scripts/refresh_baseline_debt_allowlist.py:33-37`
  - 刷新脚本生成的新条目同样只写入这三个字段。

## Why It Matters

- 一段时间后，团队很难判断某条 allowlist 是“主线历史债务”、某次临时分支债务，还是已经过时但没人清理。
- 这会削弱 allowlist 作为治理资产的可追溯性，最后又容易退化成“神秘白名单”。
- 生产级约束系统不仅要能跑，还要能解释“为什么放过它”。

## Suggested Fix

- 给条目增加最小治理元数据，例如 `base_ref`、`added_at`、`source_report`、`note`。
- refresh 脚本至少应保留旧条目元数据，并在新增时补齐来源，而不是只写三元组。
