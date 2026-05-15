---
doc_type: audit-finding
audit: 2026-05-07-rmlui-docs
finding_id: F8
status: resolved
severity: P2
nature: redundancy
confidence: low
---

# F8: landing-notes slicing 节与 impl-guide TDD order 边界模糊

## 问题

[landing-notes.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/roadmap/rmlui-full-replacement/drafts/rmlui-full-replacement-landing-notes.md) §6 implementation slicing 与 [implementation-guide.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-implementation-guide.md) 的 TDD order 有功能边界模糊。

## 证据

- landing-notes §6 描述了 "how to slice work" (meta-strategy)
- impl-guide 的 TDD order 是 "the actual slices for this feature" (tactical)
- 读者可能不清楚 landing-notes 是对所有 feature 的策略（不针对特定 feature），而 impl-guide 是 runtime-shell-specific 的执行

## 为什么是问题

如果后续 feature 的开发者以 landing-notes 为模板写他们的 impl-guide，会发现 granularity 不匹配。landing-notes 的 slicing 是按跨 feature 的通用步骤写的，impl-guide 是按单一 feature 的 test slices 写的。

## 建议

在 impl-guide 的 §0 中引用 landing-notes 作为上游策略来源，声明 "this guide implements the slicing approach described in landing-notes for this specific feature"。

## 处理结果

已解决。landing-notes 与 runtime-shell implementation-guide 的边界已明确。
