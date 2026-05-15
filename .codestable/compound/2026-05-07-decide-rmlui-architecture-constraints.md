---
doc_type: decide
type: constraint
status: active
slug: rmlui-architecture-constraints
created: 2026-05-07
tags: [rmlui, architecture, constraint, decision, convention]
related_roadmap: rmlui-full-replacement
---

# RmlUI 长期架构约束与规约

## 概述

以下约束已在 RmlUI 替代项目中被明确拍板。后续所有 design、实现和 refactor 都受这些约束管辖。新增或修改约束必须走 `cs-decide` 流程。

## 1. 平台约束

### D1: 旧 UI 是永久正式路径

旧 UI（`CGameClient::RenderQmMonitoringHud` 等）不是临时脚手架，不会被删除或降级。RmlUI 是可选增强层。

拍板来源：[rmlui-runtime-shell-design.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-design.md) 第 2 节，"旧 UI 永远存在" 决策。

影响：
- 旧 UI 代码可以独立接收功能更新。
- "迁移" 的含义是"让某个 surface 也能用 RmlUI 渲染"，不是"从旧 UI 删除该 surface"。
- 每个 RmlUI module 的 `m_HasLegacyFallback` 默认为 `true`。

### D2: RmlUI 默认关闭

`qm_rmlui_enable` 默认值为 `0`。用户必须通过配置显式启用。

拍板来源：[implementation-guide.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-implementation-guide.md) 第 1 节，Working Assumptions 第一条。

影响：
- 在任何 surface 开始 RmlUI 初始化之前，先检查全局开关。
- 全局关闭时，整个 RmlUI pipeline（backend/core/runtime/module）都不初始化。

## 2. 命名约束

### D3: 统一使用 qm_ / Qm 前缀

所有 QmClient 拥有的 RmlUI 配置键使用 `qm_` 前缀（C++ 侧用 `Qm`）。禁止引入 `cl_` 前缀给 RmlUI 配置。

拍板来源：[AGENTS.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/AGENTS.md) RmlUI / CodeStable 约束节。

当前 canonical 名称：
- `qm_rmlui_enable`
- `qm_rmlui_safe_mode`
- `qm_rmlui_monitoring_hud`

兼容性说明：现有实验名称 `qm_monitoring_use_rmlui` 在 runtime-shell 实现时需要一个明确的迁移策略（镜像/别名/替换）。

### D4: 诊断文件路径约定

结构化诊断文件写入 `dumps/QmClient_Crash/`，格式与现有 crash dump 目录一致。

诊断文件名必须包含模块名和时间戳。目标文件路径约定与现有的 saved diagnostics 区域一致，不新建独立目录。

拍板来源：[api-reference.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/reference/rmlui-runtime-api-reference.md) diagnostics target 节 + [acceptance-template.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-acceptance-template.md) 3.4 节。

## 3. 渲染约束

### D5: 禁止新增 SDL_GL 上下文获取

RmlUI 的 runtime、module、core、backend 代码中不允许新增 `SDL_GL_MakeCurrent` 或 `SDL_GL_GetCurrentContext` 调用。

拍板来源：[implementation-guide.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-implementation-guide.md) implementation boundaries 节。

影响：
- 现有 backend 对 `SDL_GL_GetCurrentContext` 的依赖被标记为 prototype/实验状态，不作为目标架构。
- render-command-bridge 负责在不获取 context 的前提下提交渲染命令。

### D6: GL3 后端是原型状态，不是目标架构

当前的 GL3 后端实现（`RenderInterface_GL3`、`RmlGL3` 命名空间）是 prototype 状态。它被允许存在但必须被标记为实验性的。正式的后端无关渲染桥在 `rmlui-render-command-bridge` 中定义。

拍板来源：[api-reference.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/reference/rmlui-runtime-api-reference.md) 7 条 implementation guardrails 最后一条。

## 4. 开发流程约束

### D7: RmlUI feature 实现走 TDD

所有 RmlUI feature 在实现前先写失败测试。测试必须证明：
- 关闭路径可达。
- 失败路径有合理的 fallback 结果。
- 成功路径有可验证的输出（frame result/diagnostics/log）。

拍板来源：[AGENTS.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/AGENTS.md) RmlUI / CodeStable 约束节 + [design.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-design.md) 第 1 节。

### D8: Monitoring HUD 是 prototype host，不是稳定模板

当前的 Monitoring HUD RmlUI 实现是第一个实验性宿主模块。它用来验证开关、fallback、runtime 判定、诊断导出是否正常工作。后续新增的稳定 module 不能以当前的 Monitoring HUD 实现作为代码模板。

拍板来源：[design.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-design.md) noun definitions 节，Monitoring HUD 标注。

## 5. 架构文档约束

### D9: 不在 acceptance 前回写目标模块到 ARCHITECTURE.md

ARCHITECTURE.md 和 ui-rmlui-current.md 只记录已存在且可运行的当前实现。roadmap 中规划的目标模块（RenderBridge、LayerManager、InputBridge 等）不能在这些文档中被描述为"当前架构"，必须在对应 feature 的 acceptance 完成后才能回写。

拍板来源：[readiness-matrix.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-readiness-matrix.md) design expansion policy 节，"不要 backfill 未来模块到 architecture 作为 current state"。
