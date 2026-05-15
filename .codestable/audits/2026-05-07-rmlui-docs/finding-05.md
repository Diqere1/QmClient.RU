---
doc_type: audit-finding
audit: 2026-05-07-rmlui-docs
finding_id: F5
status: resolved
severity: P2
nature: currency
confidence: medium
---

# F5: developer-guide 未引用 compound 和 architecture 文档

## 问题

[developer-guide.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/reference/rmlui-developer-guide.md) 的推荐阅读列表中包含 requirements 和 roadmap 文档，但未提及：
- `architecture/ARCHITECTURE.md` 和 `architecture/ui-rmlui-current.md`（当前架构）
- `compound/2026-05-07-learn-rmlui-gl-context-prototype.md`
- `compound/2026-05-07-trick-rmlui-module-fallback-pattern.md`
- `compound/2026-05-07-decide-rmlui-architecture-constraints.md`

## 为什么是问题

developer-guide 是新加入开发者的第一入口。如果它不指向架构文档和 compound 层，新人会认为 RmlUI 项目只有 planning doc 和 reference，不知道已有 learn/trick/decide 可复用。

## 建议

在 developer guide 的 references 节增加 "Architecture" 和 "Knowledge (compound)" 两个子节，列出对应文档并说明阅读顺序。

## 处理结果

已解决。developer guide 已补入 architecture 与 compound 阅读入口。
