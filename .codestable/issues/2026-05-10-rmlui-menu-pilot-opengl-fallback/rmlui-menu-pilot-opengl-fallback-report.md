---
doc_type: issue-report
issue: 2026-05-10-rmlui-menu-pilot-opengl-fallback
status: confirmed
severity: P1
summary: RmlUI settings menu pilot in OpenGL always falls back because menu_pilot and popup_modal still acquire the GL frame context directly
tags: [rmlui, menu-pilot, popup-modal, opengl, fallback]
---

# RmlUI Menu Pilot OpenGL Fallback Issue Report

## 1. 问题现象

RmlUI settings 页的 `menu_pilot` 在当前工作树中始终走 legacy fallback。这个现象已经确认与全局开关、`qm_rmlui_menu_pilot` 配置项或 Vulkan 专属路径无关：

- 之前 Vulkan 下 `monitoring_hud` 已能稳定走 RmlUI 路径。
- 这次即使切到 OpenGL，settings 页仍然 fallback。
- 现有日志和代码链路都表明，失败点出在渲染上下文获取方式，而不是 view model、文档资源或 layer dispatch 本身。

## 2. 复现步骤

1. 启动当前工作树构建。
2. 打开启用了 RmlUI menu pilot 的 settings 页路径。
3. 观察 settings 页未进入 RmlUI shell，而是进入 legacy fallback，并打印对应 fallback reason。
4. 在 OpenGL 环境下查看日志，可见 `wglMakeCurrent(): 请求的资源在使用中` 相关报错。

复现频率：100%

## 3. 期望 vs 实际

**期望行为**：`menu_pilot` 和其配套 `popup_modal` 应该像 `monitoring_hud` 一样通过 `RunOnBackendThread` 进入 render-command-bridge 基线，在 OpenGL 下不再争抢当前线程 GL context。

**实际行为**：`menu_pilot` / `popup_modal` 仍通过 `AcquireBackendFrameContext()` 直接抢占 backend frame context，OpenGL 下触发 `wglMakeCurrent(): 请求的资源在使用中`，runtime 随后返回 fallback。

## 4. 环境信息

- 涉及模块 / 功能：RmlUI `menu_pilot`、`popup_modal`、settings shell
- 相关文件 / 函数：
  - `src/game/client/gameclient.cpp`
  - `CGameClient::RenderRmlUiModule(...)`
  - `CGraphics_Threaded::AcquireBackendFrameContext()`
  - `CGraphics_Threaded::RunOnBackendThread(...)`
- 运行环境：Windows，OpenGL
- 已排除项：全局开关、菜单页 host gate、Vulkan-only 假设

## 5. 严重程度

**P1** — 当前页面级 RmlUI 主线 `menu_pilot` 无法在 OpenGL 下进入正式渲染路径，阻塞 settings page 试点验收，也说明 `popup_modal` 仍存在同类 backend ownership 风险。

## 备注

- 本 issue 关注的是“为什么始终 fallback”的根因，不把其他已拆分的灰层 / snapshot 崩溃问题混入同一条分析。
- 目前已知最关键的对照样板是 `monitoring_hud`：它已经切到 `RunOnBackendThread` 的 render-command-bridge 基线。
