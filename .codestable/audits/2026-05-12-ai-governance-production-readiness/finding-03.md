---
doc_type: audit-finding
title: generate_release_notes.py 会把所有 acceptance/fix-note 都纳入初稿，缺少状态过滤
severity: P1
category: maintainability
confidence: high
suggested_action: cs-refactor
created_at: 2026-05-12
---

# Finding 03

## Summary

`generate_release_notes.py` 已经能提取 `status` 和摘要，但它仍会无条件扫描 `.codestable/features/` 和 `.codestable/issues/` 下的全部 acceptance / fix-note 文档，并把它们写入发布说明初稿。这样一来，`draft`、`superseded`、实验性或仅用于内部流转的产物都可能被误带进发布材料。

## Evidence

- `qmclient_scripts/generate_release_notes.py:89-107`
  - `collect_artifacts` 对匹配到的每个 `*{suffix}` 文件都直接产出 `ArtifactItem`，没有按 `status` 做任何过滤。
- `qmclient_scripts/generate_release_notes.py:189-192`
  - 主流程直接把全量 feature / issue 结果渲染进发布初稿。
- `.codestable/reference/release-notes-template.md:40-43`
  - 文档宣称脚本会解析 `status` 和摘要，但没有限制哪些状态才允许进入正式初稿。

## Why It Matters

- 发布说明是面向外部或维护者的高信任产物，混入未完成或已废弃条目会直接误导发布判断。
- 这类问题不一定在脚本运行时暴露，往往是在“初稿看起来很完整”的情况下悄悄发生。
- 对生产级 AI 工作流来说，自动化初稿最怕的就是“真假条目混排且没有显式标识”。

## Suggested Fix

- 明确允许进入初稿的状态白名单，例如 `approved`、`done`、`accepted`、`fixed`。
- 对非白名单状态默认跳过，或者至少单独放到“待人工确认”分组，而不是混进主列表。
- 如果某个仓库阶段确实需要带上非最终状态，也应在脚本参数和输出里显式说明，而不是默认全收。
