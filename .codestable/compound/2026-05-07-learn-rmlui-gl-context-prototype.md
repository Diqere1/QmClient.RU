---
doc_type: learn
type: pitfall
status: active
slug: rmlui-gl-context-prototype
created: 2026-05-07
tags: [rmlui, opengl, context, backend, pitfall]
related_feature: 2026-05-07-rmlui-runtime-shell
related_roadmap: rmlui-full-replacement
---

# RmlUI GL Context 原型集成踩坑

## 发生了什么

Monitoring HUD 的 RmlUI 原型直接在 `CGameClient::RenderQmMonitoringHud()` 中硬接 RmlUI GL3 后端，导致 GL context 获取失败、context 竞争和渲染重叠。

## 根因

三条叠加：

1. **硬编码的 GL context 获取**：[rmlui_backend.cpp:L160](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/src/engine/client/rmlui_backend.cpp#L160) — `Init()` 内部直接调用 `SDL_GL_GetCurrentContext()`，而不是从外部注入 context 或 context provider。
2. **单后端类型耦合**：[rmlui_backend.cpp:L55](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/src/engine/client/rmlui_backend.cpp#L55) — `BeginFrame()` 内 `dynamic_cast<RenderInterface_GL3 *>` 将整个 backend 接口与 GL3 实现强绑定。
3. **调用时机不确定**：原型在 gameclient 的渲染流程中直接尝试初始化和渲染，与 DDNet 主渲染管线没有经过调度层，可能在 GL context 尚未就绪或已被其他渲染步骤占用时执行。

## 为什么这是坑

- RmlUI 的官方 API 假设调用者已经持有 GL context，但 QmClient 的渲染是 multi-pass 的，context 所有权在多个子系统之间流转。
- 直接调用 `SDL_GL_GetCurrentContext()` 等于假设"调用者一定持有 context"，这个假设在 prototype 路径中不成立。
- 一旦 context 获取失败，RmlUI 整个链路崩溃，连 fallback 逻辑也被跳过（因为错误处理也在同一个 hard-wired 分支中）。

## 学了什么

1. **不要在 surface module 中获取 GL context**。context 应该由渲染管线的一侧提供，RmlUI 代码只消费已就绪的 context 或通过 command bridge 间接提交渲染命令。
2. **backend 初始化必须与 rendering 分离**。Backend 的 `Init()` 应该在渲染管线有能力提供 context 时被调用，而不是在 surface module 自己的初始化步骤中。
3. **fallback 路径不能依赖 RmlUI 自身**。如果 RmlUI backend/core 失败，fallback 决策必须发生在调用 RmlUI 的代码之前（或至少有独立于 RmlUI 的 error path），否则 RmlUI 失败会吞掉 fallback。

## 如何在后续实现中避免

- `rmlui-runtime-shell` 的 frame request/result 模型中，runtime 只负责判定和报告，不直接绘制，也不获取 GL context。
- `rmlui-render-command-bridge` 将 RmlUI 的渲染调用转换为后端无关的命令，由 QmClient 渲染管线统一提交，不再让 RmlUI 代码直接触碰 GL context。
- `qm_rmlui_enable` 全局开关在发生 context 失败时提供了一条不进入 RmlUI 的路径，确保旧 HUD fallback 始终可达。
