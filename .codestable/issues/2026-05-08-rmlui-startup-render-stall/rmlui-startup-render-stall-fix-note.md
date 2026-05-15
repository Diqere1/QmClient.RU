---
doc_type: issue-fix
issue: 2026-05-08-rmlui-startup-render-stall
path: fast-track
fix_date: 2026-05-08
tags: [rmlui, startup, windows, render, menu]
status: confirmed
---

# RmlUI 启动渲染卡死修复记录

## 1. 问题现象

当前工作树的 `build-ninja/DDNet.exe` 在 Windows 上启动时，日志可能只停留在早期初始化阶段，或者在开启 RmlUI 相关配置后表现为窗口不正常显示、黑屏、看起来像“没有继续往下走”。

排查期间曾稳定观察到渲染阶段卡在 `CMenus::OnRender()` 附近，因此需要先确认启动链路和菜单层 RmlUI 接入状态。

## 2. 根因

这次问题不是单点崩溃，而是两类启动期风险叠加：

1. 启动早期部分组件会先于 `CGameClient` 的接口缓存完成就访问 `Input()`、`Storage()` 等接口，导致空指针路径不稳定。
2. 当前真正完成注册的 RmlUI 模块只有 `monitoring_hud`，但菜单页、菜单弹窗和调试层仍会无条件发起 RmlUI dispatch，代码表现会给人造成“菜单层已经接入 RmlUI”的假象，也会干扰运行时诊断。

另外，这轮排查中为了定位卡点加入了大量临时日志；这些日志本身不是修复内容，必须在收尾时全部清理。

## 3. 修复内容

本次保留的修复只包含以下几类最小改动：

- 给 `CComponentInterfaces::Input()` 增加 kernel 回退，避免组件在启动早期因为 `m_pInput` 尚未缓存而直接崩掉。
- 给 `CBinds::GetKeyBindName()` 增加空 `Input()` 容错，避免按键名查询在启动阶段形成二次崩溃。
- 将 `CGameClient` 若干接口 getter 的 kernel 回退改为 `.cpp` 中的实体实现，既保留启动期容错，又避免在只见前向声明的编译单元里触发模板实例化错误。
- 将 `CInputOverlay` 的运行时初始化延后到 `OnRender()`，避免启动阶段过早触发输入/配置访问。
- 在 `CMenus::RenderLoading()` 中把进度越界从断言改为告警并自修正，避免异步初始化阶段因为进度计数不齐直接中断。
- 在 `CRmlUiRuntime` 增加按 layer 判断模块是否已注册的能力，并让菜单页、菜单弹窗、调试层在没有对应模块时直接跳过 dispatch，明确当前只有 Monitoring HUD 真正接入 RmlUI。
- 清理本轮用于定位的临时日志，恢复常规运行时输出。

## 4. 变更文件

- `.codestable/attention.md`
- `src/engine/client/client.cpp`
- `src/game/client/RmlUi/RmlUiRuntime.cpp`
- `src/game/client/RmlUi/RmlUiRuntime.h`
- `src/game/client/component.cpp`
- `src/game/client/components/binds.cpp`
- `src/game/client/components/menus.cpp`
- `src/game/client/components/qmclient/input_overlay.cpp`
- `src/game/client/components/qmclient/input_overlay.h`
- `src/game/client/components/qmclient/qmclient.cpp`
- `src/game/client/components/sounds.cpp`
- `src/game/client/gameclient.cpp`
- `src/game/client/gameclient.h`

## 5. 验证

- `qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client --clean-first -j 1` 通过。
- `qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 1` 复编通过。
- 使用当前工作树的 `build-ninja/DDNet.exe` 做短时启动探针时，默认参数和 `+gfx_refresh_rate 1 +gfx_backgroundrender 1 +gfx_async_render_old 0` 两条路径都能进入并持续通过 `CMenus::OnRender()`。
- 本次探针只操作自己 `Start-Process` 返回的 PID，没有再按进程名批量结束 `DDNet.exe`。

## 6. 当前边界

这次修复只把“启动期稳定性”和“菜单层伪接入”收口了，不代表菜单页 / 菜单弹窗已经完成 RmlUI 化。当前真正可工作的 RmlUI UI 面仍以 Monitoring HUD 为主，菜单层后续要继续按 reference 层与 CodeStable 设计主线补完整实现。
