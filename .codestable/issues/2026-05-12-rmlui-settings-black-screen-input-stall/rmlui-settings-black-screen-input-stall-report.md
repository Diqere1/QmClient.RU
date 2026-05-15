---
doc_type: issue-report
issue: 2026-05-12-rmlui-settings-black-screen-input-stall
status: confirmed
severity: P1
summary: RmlUI settings host enters a black-screen and input-stall state after opening the settings page
tags: [rmlui, settings, menu-pilot, input, rendering]
---

# RmlUI Settings Black Screen Input Stall Issue Report

## 1. 问题现象

启用 RmlUI settings host 后，进入设置页面会出现明显异常：

- 页面顶部仍能看到旧菜单栏图标。
- 设置页主内容区域变成黑屏。
- 当前页面没有正常的可见设置框架或可点击内容。
- 鼠标交互明显卡顿，点击后页面没有按预期响应。

当前现象已经不同于早前的 fallback banner 路径：这次用户反馈中没有看到 fallback 提示，而是直接进入了黑屏且不可交互的状态。

## 2. 复现步骤

1. 使用当前工作树的 `build-ninja/DDNet.exe` 从构建目录启动客户端。
2. 进入主菜单或游戏内菜单。
3. 打开设置页面，并进入启用了 RmlUI settings host 的路径。
4. 观察到：顶部菜单栏可见，但设置页主体发黑，鼠标交互卡顿，点击页面内容没有有效响应。

复现频率：用户多次反馈，当前可重复观察到。

## 3. 期望 vs 实际

**期望行为**：进入设置页面后，应至少能看到稳定可用的 RmlUI 设置页基础框架，包括可见内容容器、正常导航和基本可交互响应。

**实际行为**：进入设置页面后，主内容区域黑屏，页面交互失效或明显卡顿，当前 RmlUI 设置页基础框架未能稳定显示。

## 4. 环境信息

- 涉及模块 / 功能：RmlUI settings host、`menu_pilot`、settings page host seam
- 相关文件 / 函数：
  - `src/game/client/components/menus_settings.cpp`
  - `src/game/client/components/menus.cpp`
  - `src/game/client/gameclient.cpp`
  - `src/game/client/RmlUi/RmlUiMenuPilot.cpp`
- 运行环境：本地 Windows 开发构建，`build-ninja/DDNet.exe`
- 其他上下文：
  - 现有截图证据：`C:/Users/11054/AppData/Local/quickclipboard/clipboard_images/1cdbf1eb82c01f0f.png`
  - 相关日志中已确认当前工作树存在 RmlUI settings host 路径和 `menu_pilot` 运行链，但这轮黑屏路径还缺一份对应的新鲜 diagnostics artifact
  - 当前问题与“是否已经 fallback 到 legacy settings”需要继续在后续分析阶段区分

## 5. 严重程度

**P1** — 这会直接阻塞当前 settings 页 RmlUI 试点的实际可用性，用户已无法把它当成一个可用的设置页基础框架来验收。

## 备注

- 当前代码现状已经不应再按“旧 settings 与 RmlUI settings 长期并排共存”理解；但运行结果仍未达到“active RmlUI settings host 可稳定显示”的状态。
- 截止本报告落档时，问题报告只记录现象，不把线程边界、context 生命周期或 scissor/clipping 等候选根因写入结论。
