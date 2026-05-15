---
doc_type: audit-finding
title: 事件摘要体系还没有升级为受约束的统一契约
severity: P1
category: arch-drift
confidence: high
suggested_action: cs-refactor
created_at: 2026-05-12
---

# Finding 03

## Summary

当前“事件摘要”已经开始写日志，但仍是多个 callsite 手写 `log_info("rmlui_event", "...")`，没有统一 emitter、没有 reason 枚举校验、也没有针对事件 schema 的测试，因此很容易再次漂移。

## Evidence

- `src/game/client/components/menus_settings.cpp:74-130`
  - `LogRmlUiEvent` / `TrackRmlUiSettingsHostEvents` 直接拼接字符串。
- `src/engine/client/rmlui_backend.cpp:230-235`
  - backend acquire 事件也是单独手写字符串格式。
- `src/game/client/gameclient.cpp:3005-3014`
  - fallback 事件同样手写，不经过统一 helper。

## Why It Matters

- 这意味着文档里的 event schema 并没有被代码真正 enforce。
- 后续任何人新增事件时，都可能拼出新的字段顺序、命名或 reason 词汇，脚本和文档无法自动发现。
- 对“生产级 AI 约束系统”来说，这种散点式 logging 只能算第一步，不算闭环。

## Suggested Fix

- 抽一个统一事件 helper，例如：
  - `LogRmlUiStructuredEvent(event, phase, target, result, reason, extra)`
- 把 `reason` 限定到共享字典
- 补至少一组针对事件文本格式或字段映射的测试
- 如果后续要导出 JSON / diagnostics，也应复用同一套 emitter
