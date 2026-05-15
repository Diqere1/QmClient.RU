---
doc_type: issue-fix
issue: 2026-05-10-rmlui-menu-pilot-opengl-fallback
status: in_progress
related: [rmlui-menu-pilot-opengl-fallback-report.md, rmlui-menu-pilot-opengl-fallback-analysis.md]
tags: [rmlui, menu-pilot, popup-modal, opengl, backend-thread, config-toggle]
---

# RmlUI Menu Pilot OpenGL Fallback 修复说明

## 修复内容

- 将 `src/game/client/gameclient.cpp` 中 `menu_pilot` 的 document render 从主线程 `AcquireBackendFrameContext()` 直抢 GL context，改为 `Graphics()->RunOnBackendThread(...)` 的 backend-frame callback 执行。
- 将 `popup_modal` 同步切到同一条 backend-thread render-command-bridge 路径，避免继续保留与 `monitoring_hud` 分叉的 backend ownership 模式。
- 在 `src/game/client/gameclient.h` 新增 `SRmlUiMenuPilotBackendFrame` / `SRmlUiPopupBackendFrame` 以及对应 callback 声明，只承载这两类 surface 的最小回传数据，不扩展到其他未请求模块。
- 保留现有 runtime / switchboard / diagnostics / failure-reason 接口，主线程仍只消费 `rendered` / `fallback_required` 结果和 surface failure 元数据，不额外引入新抽象层。
- 补修 `IsRmlUiToggleEnabled("qm_rmlui_menu_pilot")` 的配置映射：此前误返回 `g_Config.m_QmRmluiEnable`，导致 runtime 模块启停状态与 `CanRenderRmlUiMenuPilot()` / host gate 使用的 `g_Config.m_QmRmluiMenuPilot` 不一致；现已改为读取 `g_Config.m_QmRmluiMenuPilot`。

## 验证

- `qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 1`
  - 结果：通过，`DDNet.exe` 成功链接。
- `qmclient_scripts\cmake-windows.cmd --build build-ninja --target testrunner -j 1`
  - 结果：未通过，但阻塞原因为已有 `build-ninja/testrunner.exe` 文件锁，链接阶段报 `LNK1104: 无法打开文件“testrunner.exe”`，不是本次源码编译错误。

## 备注

- 当前 issue 仍在推进中：backend-thread bridge 已落地，但用户最新回报“还是 fallback 了”后，又补定位出 `qm_rmlui_menu_pilot` 的 runtime toggle 映射错误；因此本 fix-note 先切回 `in_progress`，待这轮重新验证后再回填最终结论。
- 这次修复范围仍只收口到 `menu_pilot` / `popup_modal` 与 backend-thread render-command-bridge、以及 `menu_pilot` 自身 toggle 对齐，不顺手扩展到其他未请求的 RmlUI surface。
- 下一步验证重点：OpenGL 下 settings 页是否仍显示 fallback banner；若仍失败，需要抓当前 `DescribeRmlUiMenuPilotFallback(...)` 输出和 host diagnostics，继续区分是 host gate、runtime toggle 还是 render path 的残余问题。
