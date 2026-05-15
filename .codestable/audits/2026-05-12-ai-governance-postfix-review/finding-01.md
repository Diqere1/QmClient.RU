---
doc_type: audit-finding
title: release notes 状态白名单与现有 CodeStable 产物状态词汇不一致
severity: P1
category: arch-drift
confidence: high
suggested_action: cs-refactor
created_at: 2026-05-12
---

# Finding 01

## Summary

`generate_release_notes.py` 现在默认只收录 `accepted/approved/done/fixed/completed/active`，安全性提升了，但仓库里大量既有 acceptance / fix-note 仍使用 `pass`、`confirmed`、`unknown` 等状态。这导致脚本默认行为和仓库现状不一致，很多本应进入初稿的真实产物会被排到“待人工确认”。

## Evidence

- `qmclient_scripts/generate_release_notes.py:17-24`
  - 默认状态白名单不包含 `pass`、`confirmed`。
- `.codestable/features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-acceptance.md:4`
  - 真实 feature acceptance 使用 `status: pass`。
- `.codestable/issues/2026-05-08-rmlui-startup-render-stall/rmlui-startup-render-stall-fix-note.md:7`
  - 真实 fix-note 使用 `status: confirmed`。
- `tmp/release-notes-test-filtered.md`
  - 已实际出现“多数 feature/fix 被跳过到待人工确认分组”的结果。

## Why It Matters

- 这不是安全问题，而是治理口径问题。
- 如果状态枚举不统一，自动化就会在“更安全”和“更可用”之间摇摆，AI 很难形成稳定预期。
- 对生产级系统而言，状态字段必须是可依赖契约，而不是每个目录自己约定一套。

## Suggested Fix

- 统一 CodeStable 产物状态词汇，至少给 acceptance / fix-note 一套正式枚举。
- 在统一完成前，脚本可显式兼容 `pass -> accepted`、`confirmed -> fixed/accepted` 这类现状映射，避免默认把有效产物全排除。
