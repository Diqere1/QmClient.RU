---
doc_type: issue-fix
issue: 2026-05-12-rmlui-settings-black-screen-input-stall
path: standard
fix_date: 2026-05-12
tags: [rmlui, settings, menu-pilot, input, backend-thread, lifecycle]
status: confirmed
---

# RmlUI Settings Black Screen Input Stall 修复记录

## 1. 本次修复目标

按 analysis 中已确认的方案 A，把 `MENU_PAGE` 上 `menu_pilot` 这条交互链路从“主线程直接打 `Rml::Context` + backend 线程 `Update()/Render()`”收口成单 owner 模式：

- 主线程仍负责采集输入。
- `menu_pilot` 激活时，输入不再立刻调用 `Rml::Context` raw API，而是先进入 `CRmlUiInputBridge` 的 deferred queue。
- backend page frame 在真正执行 `Update()/Render()` 前，统一 flush 这批输入。

## 2. 代码改动

- `src/game/client/RmlUi/RmlUiInputBridge.h`
- `src/game/client/RmlUi/RmlUiInputBridge.cpp`
- `src/game/client/gameclient.h`
- `src/game/client/gameclient.cpp`
- `src/test/rmlui_input_bridge_test.cpp`

具体收口内容：

- 给 `CRmlUiInputBridge` 增加 deferred context input 模式，支持把鼠标、滚轮、键盘、文本输入和 release-state 排队后统一 flush。
- `menu_pilot` 成为 active input module 时，`CGameClient::OnUpdate()` 只切换 bridge 到 deferred 模式，不再让该路径即时触碰 `MENU_PAGE` context。
- `RunRmlUiMenuPilotBackendFrame()` 在 `RenderDocument()` 前显式 flush deferred input，再执行文档更新和渲染。
- `ReleaseRmlUiInputState()` 在 deferred 模式下改为通过 backend-thread flush 完成 release，避免把 release-state 又打回主线程直接碰 context。
- 补了 bridge 级测试，锁住“排队后 flush 才触发回调”和“deferred release-state 会在 flush 中真正释放”这两个脆弱语义。
- 根据后续审查补上 lifecycle 收尾：主线程在 `menu_pilot -> 非 menu_pilot` 切换前先显式走一次 backend-owner release；`menu_pilot` 的 `unavailable / deactivate` 路径也会补做 release 后再 hide/clear，避免残留卡键、卡鼠标和输入法 ownership。
- 又补了一轮线程边界修正：把 backend-safe 的 active-context / tracked-state 清理与 main-thread 的 platform text input release 拆开。`ClearDeferredInput()` 现在只做 backend-safe 清理；`ReleasePlatformTextInputOwnership()` 由主线程在同步返回点统一调用，避免在 backend callback 里直接触发 SDL `StopTextInput()`。

## 3. 验证结果

- `qmclient_scripts\cmake-windows.cmd --build build-debug --target testrunner -j 10` 通过。
- `build-debug\testrunner.exe --gtest_filter=RmlUiInputBridge.*` 通过，16/16 通过。
- `qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10` 通过。

额外检查：

- `qmclient_scripts\strict-debug-check.ps1 -BaseRef main` 未通过，但失败点不是本次修复代码编译错误，而是已有的 CMake dev warning：
  - `CMakeLists.txt:449`
  - `CMakeLists.txt:466`
- 对 `build-ninja\DDNet.exe` 做 5 秒启动烟测时，进程直接以 `-1073741819` 退出。
- 这次新产物的运行日志显示进程死在 `gameclient_init -> menus init -> ui_page_sanitize` 更早阶段，尚未进入本 issue 需要人工验收的 settings page 交互路径。

## 4. 当前结论

这次 fix 已经把 analysis 里确认的主根因修到代码和单元测试层：

- `menu_pilot` 的 `MENU_PAGE` context 不再由主线程即时提交 raw input。
- 交互 surface 的 input / update / render 已经收回到同一条 backend page-frame owner 路径。

这次 fix 和 roadmap 主线并不冲突。它修的是“当前 settings host 仍在过渡态时，RmlUI page owner 与 legacy host seam 不能混跑”的基础约束：

- **分离的是 active owner**：active RmlUI settings frame 不再允许 legacy settings renderer 同帧并排接管输入和渲染。
- **保留的是系统级 fallback**：旧 settings path 仍然存在，但语义是 fallback / safe path，不是 active sibling renderer。
- **尚未完成的是原生迁移**：这不等于所有 settings 语义和控件都已迁成 RmlUI 原生实现，`rmlui-settings-reorg` 主线仍需继续推进。

但本次 **还不能宣告用户症状已完全验收关闭**，原因是：

- 当前 `build-ninja/DDNet.exe` 仍存在更早的启动崩溃。
- 这个崩溃阻塞了“实际打开设置页，重新观察黑屏和鼠标卡顿是否消失”的最终人工验收。

## 5. 后续阻塞项

需要单独处理当前启动崩溃，再回到本 issue 做最终 settings page 人工验收。当前已确认的阻塞证据：

- `build-ninja/debug-artifacts/DDNet_win64_runtime_2026-05-12_15-31-07_30288_ce35f5c54bc93f5c5140d03625bd0243.log`
- `build-ninja/debug-artifacts/DDNet_win64_crash_log_2026-05-12_15-31-07_30288_ce35f5c54bc93f5c5140d03625bd0243_fatal_report.txt`
