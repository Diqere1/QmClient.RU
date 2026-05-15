---
doc_type: audit-index
title: AI 约束系统修后复审
audit_scope: 刚修复后的规则/文档/脚本体系生产就绪性复查
status: active
created_at: 2026-05-12
---

# AI 约束系统修后复审

## 范围

- `AGENTS.md`
- `Claude.md`
- `.codestable/reference/workflow-entry.md`
- `.codestable/reference/workflow-manifest.json`
- `.codestable/reference/release-notes-template.md`
- `qmclient_scripts/check-gate.ps1`
- `qmclient_scripts/check_workflow_docs.py`
- `qmclient_scripts/refresh_baseline_debt_allowlist.py`
- `qmclient_scripts/generate_release_notes.py`
- `.github/workflows/build.yml`

## 总评

- 刚才那轮修复是有效的，至少把“allowlist bootstrap 死锁”“一键全量漂白”“release notes 无差别收录”这几类问题收住了。
- 所以问题 1 的答案是：这次修改**大体完整**，上一轮审计中的核心阻断项已经不是当前主问题。
- 但问题 2 的答案仍然是否定的：它还**不是**真正生产级的 AI 约束系统。
- 当前剩余差距主要在三点：
  - release artifact 的状态词汇还没和仓库现状统一
  - baseline debt allowlist 仍缺少治理元数据
  - 总入口没有进入 CI，导致“生产级约束”实际上还停留在本地自觉执行

## 交叉分类矩阵

| Finding | 性质 | 严重度 | 置信度 | 建议动作 |
|---|---|---|---|---|
| 01 | arch-drift | P1 | high | cs-refactor |
| 02 | maintainability | P2 | high | cs-refactor |
| 03 | arch-drift | P1 | high | cs-refactor |

## 发现清单

1. [finding-01.md](./finding-01.md)
2. [finding-02.md](./finding-02.md)
3. [finding-03.md](./finding-03.md)

## 下一步建议

1. 先修 `finding-03`，把 `check-gate.ps1` 接进 CI 或至少补一条与 CI 等价的正式流水线。
2. 再修 `finding-01`，统一 CodeStable 产物状态枚举，避免 release automation 和现有文档互相打架。
3. 最后修 `finding-02`，补 allowlist 的来源、基线和治理元数据。
