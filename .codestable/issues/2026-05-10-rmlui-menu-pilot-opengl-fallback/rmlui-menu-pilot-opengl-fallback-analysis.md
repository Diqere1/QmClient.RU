---
doc_type: issue-analysis
issue: 2026-05-10-rmlui-menu-pilot-opengl-fallback
status: confirmed
root_cause_type: backend-thread-mismatch
related: [rmlui-menu-pilot-opengl-fallback-report.md]
tags: [rmlui, menu-pilot, popup-modal, opengl, backend-thread]
---

# RmlUI Menu Pilot OpenGL Fallback 根因分析

## 1. 问题定位

| 关键位置 | 说明 |
|---|---|
| `src/game/client/gameclient.cpp:2901-2940` | `menu_pilot` 分支在 `CGameClient::RenderRmlUiModule(...)` 中仍使用 `CScopedRmlUiBackendFrameContext`，即主线程显式调用 `AcquireBackendFrameContext()` / `ReleaseBackendFrameContext()`。 |
| `src/game/client/gameclient.cpp:2972-3003` | `popup_modal` 也走相同模式，说明交互式菜单 surface 仍停留在旧 direct-GL path。 |
| `src/game/client/gameclient.cpp:2942-2969` | `monitoring_hud` 已改走 `Graphics()->RunOnBackendThread(&CGameClient::RunRmlUiMonitoringBackendFrameCallback, &Frame)`，并把 document render 放到 backend thread callback 里执行。 |
| `src/engine/client/graphics_threaded.cpp:3690-3719` | `AcquireBackendFrameContext()` 会 `WaitForIdle()` 后让 backend 抢 frame context；`RunOnBackendThread()` 则通过 command buffer 在 backend thread 执行 callback。两者是不同 ownership 模式。 |
| `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-roadmap.md:513-518` | render-command-bridge 已被定义为“不再通过 `AcquireBackendFrameContext` / `SDL_GL_MakeCurrent` 抢占 context”的 accepted baseline。 |

## 2. 失败路径还原

**已闭合的样板路径**：`monitoring_hud` 通过 `DispatchRmlUiLayer(...)` 进入 runtime 后，最终在 `RenderRmlUiModule("monitoring_hud")` 分支里把 document render 封装进 `SRmlUiMonitoringBackendFrame`，并由 `Graphics()->RunOnBackendThread(...)` 在 backend thread 执行。这条路径符合 render-command-bridge 基线，因此不会在主线程额外抢 GL context。

**当前失败路径**：`menu_pilot` / `popup_modal` 虽然已经接入同一个 runtime / switchboard / input-bridge 壳层，但在真正渲染 document 时仍然留在旧实现：主线程创建 `CScopedRmlUiBackendFrameContext`，显式调用 `AcquireBackendFrameContext()`，然后直接在当前调用栈里跑 `m_RmlUiCore.SetViewport(...)`、`RenderDocument(...)` 和 GL state diagnostics。这意味着它们没有复用 monitoring HUD 已经打通的 backend-thread render bridge，而是在 OpenGL 下继续与现有图形线程争抢同一个上下文。

**OpenGL 下的具体后果**：当 backend thread 已占有或正在使用 GL context 时，`AcquireBackendFrameContext()` 这一侧会触发 `wglMakeCurrent(): 请求的资源在使用中`，随后 `menu_pilot` / `popup_modal` 分支返回 `backend_frame_context_unavailable`，host 再把它解释成 legacy fallback。由于 settings 页 menu pilot 试点正是走这条分支，所以用户看到的是“settings 页始终 fallback”。

## 3. 根因

**根因类型**：`backend-thread-mismatch`

**根因描述**：`menu_pilot` 和 `popup_modal` 的 runtime 接入只完成了一半。它们在模块注册、surface dispatch、input bridge 和 fallback owner 上已经进入新 runtime，但在真正 document render 时仍沿用旧的主线程 `AcquireBackendFrameContext()` 路径；而仓库里唯一已经验收的 concrete baseline `monitoring_hud` 则改成了 `RunOnBackendThread`。因此同一套 RmlUI runtime 内部同时混用了两种 backend ownership 模式，OpenGL 下 `menu_pilot` / `popup_modal` 会继续抢 GL context，并触发 `wglMakeCurrent(): 请求的资源在使用中`。

**为什么不是其他原因**：

- 不是全局开关问题：host 已经进入 `menu_pilot` dispatch，失败发生在更深的 frame-context 获取阶段。
- 不是 Vulkan 专属差异：当前失败可以在 OpenGL 下稳定复现。
- 不是 settings 页 host gate：`CanRenderRmlUiMenuPilot()` 已允许进入试点，fallback 原因来自 render path，而不是 page eligibility。
- 不是单一文档资源问题：报错链直接指向 frame-context acquire；且 `popup_modal` 与 `menu_pilot` 共用同类失败模式。

## 4. 影响面

- **直接影响**：`menu_pilot` 在 OpenGL 下无法进入 RmlUI 渲染。
- **同类风险模块**：`popup_modal` 仍保留同一条 direct context acquire 路径，即使当前不一定每次都暴露，也仍然不符合 bridge 基线。
- **架构影响**：runtime 层已经接受了 render-command-bridge 作为基线，但菜单交互 surface 还未跟进，导致 roadmap 里的“menu/popup 复用 monitoring 样板”只在文档层成立，代码层仍未完全落地。

## 5. 修复方案

### 方案 A：只修 `menu_pilot`

- **做什么**：把 `menu_pilot` 独立改成 backend-thread frame callback。
- **优点**：改动最小，能最快解除当前 settings 页阻塞。
- **缺点 / 风险**：`popup_modal` 仍保留同一类 ownership 风险，架构继续分叉。

### 方案 B：把 `menu_pilot` 和 `popup_modal` 一起切到 monitoring 同款 backend-thread bridge

- **做什么**：新增 backend-frame payload/callback，把两者的 `RenderDocument(...)` 都迁到 `RunOnBackendThread(...)` 执行，主线程只消费结果和 surface contract。
- **优点**：直接消灭这次确认的根因，并把所有当前已落地的交互式菜单 surface 收敛到同一条 render-command-bridge 基线。
- **缺点 / 风险**：改动比方案 A 稍大，需要补足测试，确保输出 slot / pending action / diagnostics 仍正确回传。

### 推荐方案

**推荐方案 B**。这次 issue 的根因不是某一行逻辑写错，而是 menu/popup 与 monitoring HUD 在 backend ownership 上已经分叉。只修 `menu_pilot` 会留下同类半迁移状态；把 `popup_modal` 一起收敛，才能让交互式 surface 真正复用已验收的 bridge 基线。
