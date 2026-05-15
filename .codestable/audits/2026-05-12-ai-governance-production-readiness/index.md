---
doc_type: audit-index
title: AI 约束系统生产就绪性复审
audit_scope: AGENTS/Claude + workflow docs + check-gate + workflow checker + release notes + baseline allowlist + event schema
status: superseded
superseded_by: .codestable/audits/2026-05-12-ai-governance-postfix-review/
created_at: 2026-05-12
---

# AI 约束系统生产就绪性复审

## 范围

- `AGENTS.md`
- `Claude.md`
- `.codestable/reference/workflow-entry.md`
- `.codestable/reference/workflow-manifest.json`
- `.codestable/reference/pre-merge-verification.md`
- `.codestable/reference/pr-review-checklist.md`
- `.codestable/reference/release-notes-template.md`
- `.codestable/reference/event-summary-schema.md`
- `qmclient_scripts/check-gate.ps1`
- `qmclient_scripts/check_workflow_docs.py`
- `qmclient_scripts/refresh_baseline_debt_allowlist.py`
- `qmclient_scripts/generate_release_notes.py`
- `src/engine/client/rmlui_event_log.h`

## 总评

- 这批修改已经把“入口分散、脚本各跑各的、AI 容易漏读规则”的问题收口了不少。
- 但如果标准是“真正生产级、可以长期稳定约束 AI 而不靠人工补脑”，当前结论仍然是 `Needs Fix`。
- 主要残余风险集中在四点：
  - baseline allowlist 的 bootstrap 和治理还不稳
  - release notes 自动化还会混入不该发布的产物
  - workflow 契约检查仍偏浅层
  - 一些关键机制已经写进文档，但还没有足够强的机器化防呆

## 交叉分类矩阵

| Finding | 性质 | 严重度 | 置信度 | 建议动作 |
|---|---|---|---|---|
| 01 | bug | P1 | high | cs-issue |
| 02 | maintainability | P1 | high | cs-refactor |
| 03 | maintainability | P1 | high | cs-refactor |
| 04 | arch-drift | P2 | high | cs-refactor |

## 发现清单

1. [finding-01.md](./finding-01.md)
2. [finding-02.md](./finding-02.md)
3. [finding-03.md](./finding-03.md)
4. [finding-04.md](./finding-04.md)

## 下一步建议

1. 先修 `finding-01` 和 `finding-02`，否则 baseline debt 体系还不算可依赖的正式门。
2. 再修 `finding-03`，把 release notes 初稿从“能生成”提升到“不会误导发布”。
3. 最后补 `finding-04`，把 workflow 契约检查从“存在性”继续推进到“同步性”。
