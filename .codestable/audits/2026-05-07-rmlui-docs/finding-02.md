---
doc_type: audit-finding
audit: 2026-05-07-rmlui-docs
finding_id: F2
status: resolved
severity: P1
nature: maintainability
confidence: high
---

# F2: render-cmd-bridge 是唯一有 design 无 checklist 的 feature

## 问题

5 份 feature design 中，runtime-shell、input-bridge、safe-mode、resource-diagnostics 均有 checklist.yaml，但 render-cmd-bridge 没有。

## 证据

- Glob `features/2026-05-07-rmlui-render-command-bridge/` 只有 `rmlui-render-command-bridge-design.md`，无 checklist 文件。
- 其他 4 个 feature 目录均有配对 checklist.yaml。
- [design:L1-L3](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-render-command-bridge/rmlui-render-command-bridge-design.md#L1-L3)：`status: draft`，且注明 "This draft is intentionally not approved. It is kept as investigation output only."

## 为什么是问题

不一致的文档结构让流程使用者困惑：是 render-cmd-bridge 不需要 checklist（因为未 approved），还是 checklist 还没生成？没有明确说明。
其他 4 份 design 都遵循 "design → checklist → impl → acceptance" 的固定链，render-cmd-bridge 打乱了这个模式。

## 建议

在 design frontmatter 或 §0 中显式声明："No checklist until status changes from draft to approved." 或者直接生成一份空 checklist 并标注 `status: blocked`。

## 处理结果

已解决。`rmlui-render-command-bridge-design.md` 已明确该 draft 故意不生成 checklist，直到设计转为 approved。
