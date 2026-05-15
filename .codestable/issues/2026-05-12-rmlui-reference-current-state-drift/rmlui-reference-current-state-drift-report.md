---
doc_type: issue-report
issue: 2026-05-12-rmlui-reference-current-state-drift
status: draft
severity: P2
summary: Local RmlUI reference docs no longer fully match the current settings-host and runtime code state
tags: [rmlui, docs, reference, runtime, settings]
---

# RmlUI Reference Current State Drift Issue Report

## 1. 问题现象

仓库内部分 RmlUI reference 文档与当前代码现状出现偏移，导致“官方文档沉淀 reference”无法稳定充当当前实现的可信入口。

本轮核对中已经能明确看到：

- 部分 reference 仍保留较强的早期 runtime-shell / Monitoring HUD 口径。
- 当前代码里已经存在 `menu_pilot`、`popup_modal`、settings host contract 等更晚的实现状态。
- 文档中“当前代码现状”和“未来 contract / implementation-ready contract”的边界不够干净，容易误读成“实现已经闭环”或“当前仍只到 HUD 阶段”。

## 2. 复现步骤

1. 阅读 `.codestable/reference/rmlui-runtime-api-reference.md`。
2. 对照当前代码中的 settings/menu runtime 相关实现。
3. 再对照 `.codestable/reference/rmlui-settings-host-contract.md` 与 `.codestable/compound/2026-05-12-explore-rmlui-settings-host-current-state.md`。
4. 观察到：reference 对当前 settings/menu runtime 状态的描述与真实代码现状并不完全一致，阅读者需要额外横跳多个文档才能避免误判。

复现频率：稳定存在，属于文档现状问题。

## 3. 期望 vs 实际

**期望行为**：本地 reference 应清晰区分“官方 API 事实”“当前代码现状”“未来 contract”，并且能准确反映当前 settings host、`menu_pilot`、`popup_modal` 和 runtime 的真实落地状态。

**实际行为**：当前 reference 仍有部分历史口径残留，导致阅读者难以直接从 reference 判断 settings/menu 相关 RmlUI 代码到底已经做到哪一步、哪些仍是 contract 而非已验收实现。

## 4. 环境信息

- 涉及模块 / 功能：RmlUI 本地 reference、settings host current-state 文档、runtime reference
- 相关文件 / 函数：
  - `.codestable/reference/rmlui-runtime-api-reference.md`
  - `.codestable/reference/rmlui-settings-host-contract.md`
  - `.codestable/compound/2026-05-12-explore-rmlui-settings-host-current-state.md`
  - `src/game/client/gameclient.cpp`
  - `src/game/client/components/menus_settings.cpp`
- 运行环境：文档与代码对照审查
- 其他上下文：
  - 官方文档本身本轮已通过 Context7 再核对
  - 当前偏移针对的是“仓库内沉淀 reference”，不是 RmlUI 官方文档本身

## 5. 严重程度

**P2** — 这不会直接导致程序崩溃，但会降低后续 RmlUI 分析、实现和验收的可信度，容易把团队继续带回错误的 current-state 假设。

## 备注

- “设置页尚未完全解耦旧宿主”本身不单独作为本 issue 的 defect 结论；这里只记录 reference 和 current-state 口径不一致的问题。
- 后续如果确认需要把“解耦边界”从现状说明提升为正式 roadmap/update，再走对应规划流程，不在本 report 里偷做范围扩张。
