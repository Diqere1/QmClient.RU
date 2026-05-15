---
doc_type: audit-index
status: partially-resolved
slug: rmlui-docs
created: 2026-05-07
total_findings: 9
scope: RmlUI 文档体系 (requirements/roadmap/architecture/features/reference/compound, 24 份文件)
tags: [rmlui, documentation, audit]
---

# RmlUI 文档体系审计

## 范围

全部 RmlUI 相关 CodeStable 文档，跨越 6 个实体类型：
- requirements (1份)
- roadmap (5份)
- architecture (2份)
- features/design+checklist+impl+templates (8份)
- reference (3份)
- compound (5份)
共计 24 份文件（含 2 份同文件纠错变体）。

## 总评

文档体系已从一周前的 0 基建迅速扩张到覆盖 6 个实体类型、49 层覆盖度。首轮审计指出的多数“连接断裂”已经被后续回写修复；当前重点已经从“有没有链路”转成“哪些 draft feature 还没进入 acceptance 闭环”：

- **runtime-shell 已形成完整闭环**：design、checklist、implementation-guide、acceptance、architecture backfill、roadmap 回写均已落地。
- **draft feature 仍是当前主要风险点**：resource-diagnostics、safe-mode、input-bridge、render-command-bridge 仍需完成 review/approval。
- **参考层已显著收敛**：developer-guide、test-strategy、PRD/roadmap/requirements 引用链和 landing-notes/impl-guide 边界已补齐。

## 发现清单

| # | 维度 | 严重度 | 性质 | 置信度 | 简述 |
|---|---|---|---|---|---|
| F1 | 一致性 | P1 | maintainability | high | 已解决：items.yaml 已同步已有 design doc |
| F2 | 一致性 | P1 | maintainability | high | 已解决：render-cmd-bridge 已声明 draft 阶段不生成 checklist |
| F3 | 完整性 | P1 | maintainability | high | 已解决：runtime-shell acceptance 已落地 |
| F4 | 完整性 | P1 | maintainability | medium | 部分解决：runtime-shell 已回写 architecture，其余 draft feature 仍待未来 acceptance |
| F5 | 时效性 | P2 | currency | medium | 已解决：developer-guide 已引用 architecture 与 compound |
| F6 | 冗余 | P2 | redundancy | medium | 已解决：test-strategy 与 implementation-guide 边界已明确 |
| F7 | 完整性 | P2 | completeness | low | 已解决：PRD 已建立引用链 |
| F8 | 冗余 | P2 | redundancy | low | 已解决：landing-notes 与 impl-guide 边界已明确 |
| F9 | 时效性 | P2 | currency | high | 已解决：test-strategy 已改成当前真实缺口 |

## 交叉分类表

|  | P0 | P1 | P2 |
|---|---|---|---|
| consistency | — | F1, F2 | — |
| completeness | — | F3, F4 | F7 |
| redundancy | — | — | F6, F8 |
| currency | — | — | F5, F9 |

## 建议下一步

- `rmlui-resource-diagnostics` 已不再是 design review / approval 对象；后续应以 acceptance 后的状态同步为主。
- 后续每个 feature 都要按 acceptance 复制 checklist、architecture、roadmap 回写闭环。
- 进入实现前先做参考层 review，尤其是 render/input/backend 相关 feature。
