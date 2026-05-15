---
doc_type: audit-finding
audit: 2026-05-07-rmlui-docs
finding_id: F1
status: resolved
severity: P1
nature: maintainability
confidence: high
---

# F1: items.yaml 未同步 4 份已有 design doc

## 问题

4 份 design doc 已存在于 features/ 下，但 roadmap items.yaml 中对这些 item 的 status 和 feature 字段均未更新。

## 证据

| Item | design frontmatter | items.yaml status | items.yaml feature |
|---|---|---|---|
| rmlui-render-command-bridge | [design:L1-L8](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-render-command-bridge/rmlui-render-command-bridge-design.md#L1-L8)：status=draft | [items.yaml:L32-L38](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-items.yaml#L32-L38)：`status: planned` | `feature: null` |
| rmlui-input-bridge | [design:L1-L9](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-input-bridge/rmlui-input-bridge-design.md#L1-L9)：status=draft | `status: planned` | `feature: null` |
| rmlui-safe-mode | [design:L1-L10](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-safe-mode/rmlui-safe-mode-design.md#L1-L10)：status=approved；[acceptance:L1-L8](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-safe-mode/rmlui-safe-mode-acceptance.md#L1-L8)：status=pass | `status: done` | `feature: 2026-05-07-rmlui-safe-mode` |
| rmlui-resource-diagnostics | [design:L1-L9](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-resource-diagnostics/rmlui-resource-diagnostics-design.md#L1-L9)：status=approved；[acceptance:L1-L8](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-resource-diagnostics/rmlui-resource-diagnostics-acceptance.md#L1-L8)：status=pass | `status: done` | `feature: 2026-05-07-rmlui-resource-diagnostics` |

对比 runtime-shell 的正确同步：
- [items.yaml:L10-L11](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-items.yaml#L10-L11)：`status: in-progress` + `feature: 2026-05-07-rmlui-runtime-shell`。

## 为什么是问题

items.yaml 是 roadmap 的事实调度器。readiness-matrix.md、explore-doc-completeness.md 的评估、以及 developer-guide 的"看 roadmap → 选 feature → 做 design → 做 checklist"流程都依赖 items.yaml 的 status 字段。如果 items.yaml 不反映真实状态，这些下游文档和流程就会产生错误信息。

## 建议

同步 4 个 item 的 `status` 为 `in-progress`（或 `design-draft`），`feature` 为对应的 feature 目录名。

## 处理结果

已解决。`rmlui-full-replacement-items.yaml` 已为相关 feature 填入对应 `feature` 目录，消除调度器与设计文档的断裂。
