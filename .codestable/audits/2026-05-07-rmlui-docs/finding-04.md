---
doc_type: audit-finding
audit: 2026-05-07-rmlui-docs
finding_id: F4
status: partially-resolved
severity: P1
nature: maintainability
confidence: medium
---

# F4: 5 份 design 的 Architecture Backfill 节全为占位

## 问题

每份 feature design 的 §8 Architecture Backfill 列出了 "After implementation acceptance, backfill:" 条目，但没有任何一条已被回写到 ARCHITECTURE.md 或 ui-rmlui-current.md 中。

## 证据

| design | §8 声明回写的条目数 | 实际回写状态 |
|---|---|---|
| [runtime-shell](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-design.md) §8 | runtime model, layer policy, diagnostics contract, config migration 等 | 零条 |
| [render-cmd-bridge](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-render-command-bridge/rmlui-render-command-bridge-design.md) §7 | command bridge contract | 零条 |
| [input-bridge](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-input-bridge/rmlui-input-bridge-design.md) §8 | input route model, cancel/release-state policy, console focus protection | 零条 |
| [safe-mode](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-safe-mode/rmlui-safe-mode-design.md) §8 | safe-mode policy fields, failure streak model, recovery/reset semantics | 零条 |
| [resource-diag](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-resource-diagnostics/rmlui-resource-diagnostics-design.md) §7 | resource diagnostics taxonomy, export target convention, dev-only rule | 零条 |

ARCHITECTURE.md 和 ui-rmlui-current.md 均只记录当前已存在的代码基础设施（backend/core/MonitoringHUD prototype），不包括任何 §8 声明的回写目标。

## 为什么是问题

Architecture Backfill 是 design 文档与架构文档之间的协议。如果协议一直不执行，architecture 文档将永远停留在 prototype 状态，即使 features 已通过 acceptance。这意味着 architecture 将不再是 "system-of-record"。

## 建议

此问题与 F3 同一根源：没有 acceptance evidence 就没有 backfill 的输入。需要在 runtime-shell 通过 acceptance 后，按 design §8 逐条回写。

## 处理结果

部分解决。runtime-shell 的 architecture backfill 已完成；其余 draft feature 仍待 acceptance 后回写，因此该 finding 仅对未完成 feature 继续成立。
