---
doc_type: audit-finding
audit: 2026-05-07-rmlui-docs
finding_id: F9
status: resolved
severity: P2
nature: currency
confidence: high
---

# F9: test-strategy 仍声称没有 runtime-shell tests

## 问题

[rmlui-test-strategy.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/reference/rmlui-test-strategy.md) 的 Current gaps 仍写着 `No runtime-shell tests.`，但仓库里已经存在 `src/test/rmlui_runtime_test.cpp`，并且它包含 runtime-shell、fallback、diagnostics 和 compat 相关测试。

## 证据

- [test-strategy:L21-L24](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/reference/rmlui-test-strategy.md#L21-L24)：Current gaps 仍列出 `No runtime-shell tests.`
- [rmlui_runtime_test.cpp:L85-L325](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/src/test/rmlui_runtime_test.cpp#L85-L325)：已存在 runtime-shell 相关测试集合
- [rmlui_runtime_test.cpp:L85](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/src/test/rmlui_runtime_test.cpp#L85) / [L258](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/src/test/rmlui_runtime_test.cpp#L258)：分别覆盖 global/module disable、runtime unavailable fallback、diagnostics export gate and dedupe

## 为什么是问题

测试策略文档是后续 feature 的 TDD 入口。如果它继续写“没有 runtime-shell tests”，读者会误以为 runtime-shell 仍处于测试空白，进而影响后续 feature 的测试切片设计和 acceptance 预期。

## 建议

把 Current gaps 改成“runtime-shell tests 已存在，后续缺 safe-mode / resource diagnostics / render bridge tests”，或者直接按当前状态重写该节。

## 处理结果

已解决。test-strategy 已移除过期表述并改写为当前真实缺口。
