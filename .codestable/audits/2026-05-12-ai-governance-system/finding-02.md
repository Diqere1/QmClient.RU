---
doc_type: audit-finding
title: 发布说明生成器仍停留在目录枚举层
severity: P1
category: maintainability
confidence: high
suggested_action: cs-refactor
created_at: 2026-05-12
---

# Finding 02

## Summary

`generate_release_notes.py` 目前只统计 artifact 数量并列出路径，没有读取 acceptance / fix-note 的内容，也没有抽取验证证据，因此它还不够格叫“半自动汇总”。

## Evidence

- `qmclient_scripts/generate_release_notes.py:26-33`
  - 只收集文件路径和父目录 slug。
- `qmclient_scripts/generate_release_notes.py:48-103`
  - 绝大多数字段仍是占位文案或“请人工补充”。
- `qmclient_scripts/generate_release_notes.py:90-97`
  - 即使提供 gate report，也只打印 PASS/WARN/FAIL 计数，不抽取失败类别、验证命令或发布态证据。

## Why It Matters

- 这会让脚本产出看起来像自动化，实际仍需要人工重新读一遍全部源材料。
- 如果团队或 AI 开始依赖这份初稿，发布说明会倾向于失真或遗漏真正的用户影响。

## Suggested Fix

- 解析 `*-acceptance.md` / `*-fix-note.md` 的结构化段落或 YAML frontmatter，抽出：
  - 用户可感知变化
  - 风险 / 兼容性说明
  - 验证动作
- 对 gate report 增加：
  - failure category 摘要
  - 实际运行命令
  - release/build-ninja 验证提示
