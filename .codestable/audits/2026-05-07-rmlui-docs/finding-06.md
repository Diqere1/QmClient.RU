---
doc_type: audit-finding
audit: 2026-05-07-rmlui-docs
finding_id: F6
status: resolved
severity: P2
nature: redundancy
confidence: medium
---

# F6: implementation-guide 与 test-strategy 在 TDD 边界上重叠

## 问题

[test-strategy.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/reference/rmlui-test-strategy.md) 定义了测试层级和 TDD 要求，而 [implementation-guide.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-implementation-guide.md) 定义了 TDD 执行顺序和 test slice maps。两篇文档的 TDD 部分功能边界模糊。

## 证据

- test-strategy 定义 "where and how to write tests"
- impl-guide 定义 "5 test slices: global-off, module-off, runtime-unavailable, surface-failure, duplicate-registration"
- 读者不确定：第一次写测试时看哪份文档？test-strategy 是 policy-level 的吗？impl-guide 是 task-level 的吗？

## 为什么是问题

当两份文档描述同一流程时，不一致风险上升。后续新增 feature 的 impl-guide 是按 runtime-shell 的 impl-guide 模式写还是按 test-strategy 的模式写？

## 建议

在 test-strategy 中明确说明 "per-feature implementation guides are the tactical complement of this strategy"，或者在 impl-guide §0 中声明 "this is the feature-specific execution of the test strategy in ..."。

## 处理结果

已解决。test-strategy 与 runtime-shell implementation-guide 的边界已经明确。
