---
doc_type: refactor-design
refactor: 2026-05-11-rmlui-settings-doc-structure
status: approved
scope: .codestable/features/2026-05-10-rmlui-settings-reorg/*.md, .codestable/roadmap/rmlui-full-replacement/*.md, .codestable/features/RMLUI_FEATURE_INDEX.md, .codestable/reference/rmlui-runtime-api-reference.md, .codestable/reference/rmlui-settings-host-contract.md
summary: 把 settings 宿主契约抽成单一 reference，再把 feature / roadmap / readiness / runtime / index 收缩为各自职责，避免同一条规则在多处重复维护。
---

# rmlui-settings-doc-structure refactor design

## 1. 本次范围

- 选中的 5 条都做：
  - settings-reorg design 从“feature 方案 + 长期 contract + current-state 观察”里拆出 canonical contract。
  - 把 `dedicated contexts / no parallel render / no legacy island` 收敛到单一 settings-host contract。
  - roadmap / readiness 只保留阶段、依赖、状态。
  - runtime reference 只保留通用 RmlUi API 边界。
  - feature index 只保留导航信息。
- 明确不做：
  - 不改 settings feature 的 IA 方向。
  - 不改 roadmap 的阶段顺序。
  - 不把 refactor 变成新的 feature 设计。

## 2. 前置依赖

- 现有 `rmlui-settings-reorg-design.md`、`rmlui-full-replacement-roadmap.md`、`rmlui-full-replacement-readiness-matrix.md`、`rmlui-runtime-api-reference.md`、`RMLUI_FEATURE_INDEX.md`。
- 官方 RmlUi context/main-loop 文档，用来校准“独立 context / size / input state / update-render order”口径。

## 3. 执行顺序

1. 新增 `rmlui-settings-host-contract.md`。
   - 引用方法：L1 行为等价迁移
   - 具体操作：把 settings 主线的长期 host 规则、上下文边界和 fallback 语义集中到一份 reference。
   - 退出信号：其余文档都能短引用这份 contract，不再复制整段规则。
   - 验证责任：AI 自证

2. 收缩 settings feature / checklist。
   - 引用方法：L2 代码级重构
   - 具体操作：保留 feature-specific IA、目标、验收和执行边界，删掉重复的长期 host 叙述。
   - 退出信号：design 读起来更像 feature 设计，而不是契约汇编。
   - 验证责任：AI 自证

3. 收缩 roadmap / readiness / runtime / index。
   - 引用方法：L3 结构拆分
   - 具体操作：roadmap 和 readiness 退回阶段/依赖/状态；runtime reference 退回 API；index 退回导航。
   - 退出信号：各文档职责单一，settings contract 只在一个地方完整出现。
   - 验证责任：AI 自证

4. 校验引用和格式。
   - 引用方法：L1/L3 组合收口
   - 具体操作：检查链接、YAML 和重复叙述是否清掉。
   - 退出信号：`validate-yaml.py` 通过，且文档引用不悬空。
   - 验证责任：AI 自证

## 4. 风险与看点

- 风险最高的是“只改了引用，没把重复正文真正删掉”。
- 需要警惕 roadmap / readiness 又被 feature 细节污染回去。
- settings-host contract 要稳，但不能把 feature 设计内容也搬进去。
