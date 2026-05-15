---
doc_type: learning
track: pitfall
date: 2026-05-08
slug: rmlui-startup-render-stall
component: RmlUI 启动期与渲染链
severity: high
tags: [rmlui, startup, render, thread, menu, pitfall]
---

# RmlUI 启动期渲染卡死与误判排查经验

## 1. 问题

RmlUI 启用后，客户端在 Windows 上出现过多种看起来像“同一个问题”的症状：只打印早期 storage 日志后窗口不开、黑屏、菜单阶段像卡死、Monitoring HUD 和旧 UI 状态混在一起。

这类问题的危险点在于：它们都发生在启动期或渲染期，表面现象很像，但根因可能横跨接口缓存时机、线程边界、菜单层误接线和未完成模块的假 dispatch。

## 2. 症状

- 从 `build-ninja/DDNet.exe` 启动后，只看到 `storage` 相关早期日志，误以为客户端完全没往下走。
- 启用 RmlUI 后出现黑屏，或者看起来像菜单层没有继续渲染。
- 日志一度停在 `CMenus::OnRender()` 附近，容易让人直接把问题归咎到“菜单逻辑坏了”。
- Monitoring HUD 已经能跑时，菜单页和菜单弹窗仍然表现得像“也已经接入了 RmlUI”，但实际没有对应模块。

## 3. 没用的做法

- 看到 `RmlUI` / `OpenGL` / `wglMakeCurrent()` 相关字样，就直接假定所有症状都来自同一个 GL context 冲突。
- 在没有先梳理调用链的情况下，直接改线程、改初始化时机、改 backend 调度顺序。
- 把菜单页和菜单弹窗的 `DispatchRmlUiMenuPageSlot()` / `DispatchRmlUiMenuModalSlot()` 当成“菜单 RmlUI 已经完成”的证据，而不是先核对 module registry 里到底有没有对应 surface。
- 为了排查运行问题按进程名批量结束 `DDNet.exe`，结果误杀别的客户端，污染运行结论。

## 4. 解法

这次收口有效的做法不是“继续猜一个大根因”，而是把启动期稳定性和 RmlUI 功能完整度拆开处理：

- 先保证启动期接口访问可容错：
  - `CComponentInterfaces::Input()` 增加 kernel 回退。
  - `CBinds::GetKeyBindName()` 增加空 `Input()` 容错。
  - `CGameClient` 若干 getter 的 kernel 回退放到 `.cpp`，避免 header 内联回退在不完整类型场景下引入新的编译问题。
- 把启动期不该过早执行的逻辑后移：
  - `InputOverlay` 不在 `OnInit()` 直接触碰输入状态和配置时间戳，改到运行时再初始化。
- 把“崩就断言”的路径改成“记录并自修正”：
  - `CMenus::RenderLoading()` 的进度越界从断言改为告警并修正总量。
- 把未完成的 RmlUI 菜单接线明确收口：
  - `CRmlUiRuntime` 提供 `HasModuleForLayer(...)`。
  - 菜单页、菜单弹窗、调试层只有在对应 layer 真的注册过模块时才允许 dispatch。

## 5. 为什么有效

有效的关键不是“把所有异常都修掉了”，而是先让运行时状态和真实实现对齐：

- 启动期接口回退解决的是“组件在缓存尚未齐备时就被调用”的时机问题。
- `InputOverlay` 延后初始化解决的是“功能上无关的 overlay 抢跑启动链”的问题。
- `RenderLoading()` 自修正解决的是异步加载阶段因为计数不齐导致的非必要中断。
- 菜单 dispatch 收口解决的是“代码看起来像已经接入 RmlUI，实际没有模块”的伪完成状态。

也就是说，这次不是单纯修了一个崩点，而是把几条容易互相掩盖的启动期风险拆开了。

## 6. 预防

- 只要改动涉及线程、启动链、渲染链、backend context 或 resource ownership，先完整梳理原始调用链、线程归属、初始化时机和 fallback，再动代码。
- 看到“症状都发生在同一时段”不等于“根因只有一个”；先分层确认是启动期接口访问、渲染调度、模块注册，还是资源/上下文问题。
- 菜单层、HUD、overlay 这类 surface 迁移到 RmlUI 时，先查 registry 和 runtime contract，不要把“有 dispatch 调用”当成“功能已完成”。
- 运行探针只能操作当前工作树的可执行文件和自己启动出来的 PID，避免误杀其他客户端，导致证据失真。
- 如果线程边界或宿主调用关系还没搞清楚，先补 `cs-explore` 或 `cs-learn`，必要时再起更完整的设计文档，不要靠试错硬冲。
