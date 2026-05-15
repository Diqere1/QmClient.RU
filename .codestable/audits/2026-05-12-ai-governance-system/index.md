---
doc_type: audit-index
title: AI 约束系统审计
audit_scope: 最近新增的规则/文档/脚本/RmlUI 事件摘要链路
status: superseded
superseded_by: .codestable/audits/2026-05-12-ai-governance-production-readiness/
created_at: 2026-05-12
---

# AI 约束系统审计

## 范围

- `.codestable/reference/` 新增与更新的工作流文档
- `qmclient_scripts/check-gate.ps1`
- `qmclient_scripts/check_workflow_docs.py`
- `qmclient_scripts/generate_release_notes.py`
- `src/game/client/components/menus_settings.cpp`
- `src/engine/client/rmlui_backend.cpp`
- `src/game/client/gameclient.cpp`

## 总评

- 当前这批修改对“把规则/文档/脚本收口成一个统一入口”是有效的。
- 但如果标准是“真正生产级、能够稳定约束 AI 而不靠人工补脑”，结论仍是 `Needs Fix`。
- 主要差距不在“有没有文档”，而在：
  - 文档一致性检查仍是浅层 presence check
  - release note 自动化仍只是目录枚举
  - 事件 schema 还只是散落日志字符串，没有统一 emitter / 校验 / 测试
  - gate 仍然建立在红色基线之上，AI 很难把“当前改坏了”和“仓库本来就红”彻底切开

## 交叉分类矩阵

| Finding | 性质 | 严重度 | 置信度 | 建议动作 |
|---|---|---|---|---|
| 01 | maintainability | P1 | high | cs-refactor |
| 02 | arch-drift | P1 | high | cs-refactor |
| 03 | maintainability | P1 | high | cs-refactor |
| 04 | arch-drift | P1 | high | cs-refactor |

## 发现清单

1. [finding-01.md](./finding-01.md)
2. [finding-02.md](./finding-02.md)
3. [finding-03.md](./finding-03.md)
4. [finding-04.md](./finding-04.md)

## 下一步建议

1. 先修 `finding-04`，给 baseline debt 引入 allowlist / snapshot / debt manifest，不然 `check-gate` 仍不是可依赖的 AI gate。
2. 再修 `finding-01` 和 `finding-03`，把工作流文档检查和事件 schema 从“约定”升级为“被机器验证的契约”。
3. `finding-02` 最后做，把 release note 生成从“列目录”升级为“抽 acceptance/fix-note 摘要 + gate 结果 + 验证证据”。
