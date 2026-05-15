---
doc_type: audit-finding
audit: 2026-05-07-rmlui-docs
finding_id: F7
status: resolved
severity: P2
nature: completeness
confidence: low
---

# F7: PRD 文档孤立，未被引用

## 问题

[rmlui-full-replacement-prd.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-prd.md) 存在于 roadmap 目录下，但未在以下文件中被引用：
- requirements/rmlui-full-replacement.md
- roadmap/rmlui-full-replacement-roadmap.md
- developer-guide 的推荐阅读列表

## 为什么是问题

孤立的文档让读者困惑：这份文件是过期的还是有用的？是 roadmap 的上游输入还是独立产物？没有引用链就意味着无法判断它的权威性和时效性。

## 建议

如果 PRD 是有用的输入：在 requirements 或 roadmap 的 references 节中引用它，并标注它的角色（如 "original product brief"）。
如果 PRD 已被 requirements 替代：标记 `status: outdated` + `superseded-by: requirements/rmlui-full-replacement.md`。

## 处理结果

已解决。PRD 已被 requirements 和 roadmap 引用并标注为原始产品简报。
