---
doc_type: issue-report
issue: 2026-05-12-startup-flash-crash-ui-page-sanitize
status: confirmed
severity: P0
summary: 当前工作树构建出的 DDNet 客户端在启动阶段直接闪退，死前停在 menus 的 ui_page_sanitize 路径
tags: [startup, crash, menus, ui-page-sanitize, serverbrowser, rmlui]
---

# Startup Flash Crash Ui Page Sanitize Issue Report

## 1. 问题现象

当前工作树 `C:\Users\11054\.codex\worktrees\140c\QmClient` 构建出的 `DDNet.exe` 启动后直接闪退，用户无法进入客户端主界面。

这次异常在 `build-ninja` 与 `build-debug` 两个构建目录中都可复现。最新运行日志显示，进程在 `gameclient_init -> menus init -> ui_page_sanitize` 阶段崩溃；在 `build-ninja` 的最新日志里，死前最后一条稳定日志是：

- `qm_init/menus: ui_page_sanitize before favorite_communities`

最新 fatal report 为访问违规：

- `2026-05-12 16:39:48.935`
- `Exception code: 0xC0000005`
- `Access violation target address: 0x00000000000025D4`

## 2. 复现步骤

1. 在当前工作树编译客户端：
   - `qmclient_scripts\cmake-windows.cmd --build build-debug --target game-client -j 10`
   - 或 `qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10`
2. 从构建目录启动客户端：
   - `build-debug\DDNet.exe`
   - 或 `build-ninja\DDNet.exe`
3. 观察到：客户端在启动阶段直接闪退，没有进入可交互主界面。

复现频率：稳定

## 3. 期望 vs 实际

**期望行为**：客户端应正常完成启动初始化，进入主界面或至少进入可见的加载/菜单页面。

**实际行为**：客户端在启动期直接崩溃退出，停在 `menus` 初始化阶段，无法进入主界面。

## 4. 环境信息

- 涉及模块 / 功能：启动链、`CMenus::OnInit`、`ui_page_sanitize`、favorite community page 入口
- 相关文件 / 函数：
  - `src/game/client/components/menus.cpp`
  - `src/game/client/gameclient.cpp`
  - `CMenus::OnInit`
  - `ui_page_sanitize`
- 运行环境：本地 Windows dev
- 其他上下文：
  - `build-ninja` 最新 runtime log：`build-ninja/debug-artifacts/DDNet_win64_runtime_2026-05-12_16-37-50_2944_ce35f5c54bc93f5c5140d03625bd0243.log`
  - `build-ninja` 最新 fatal report：`build-ninja/debug-artifacts/DDNet_win64_crash_log_2026-05-12_16-37-50_2944_ce35f5c54bc93f5c5140d03625bd0243_fatal_report.txt`
  - `build-debug` 最新 runtime log：`build-debug/debug-artifacts/DDNet_win64_runtime_2026-05-12_16-39-47_34848_ce35f5c54bc93f5c5140d03625bd0243.log`
  - `build-debug` 最新 fatal report：`build-debug/debug-artifacts/DDNet_win64_crash_log_2026-05-12_16-39-47_34848_ce35f5c54bc93f5c5140d03625bd0243_fatal_report.txt`
  - `build-ninja` 日志显示 `g_Config.m_UiPage=9`，并进入 favorite community page 的 `ui_page_sanitize` 分支
  - 这是独立于 `2026-05-12-rmlui-settings-black-screen-input-stall` 的新问题；当前崩溃发生在进入设置页交互之前

## 5. 严重程度

**P0** — 当前本地构建产物无法完成启动，直接阻塞客户端基本可用性与后续 settings/RmlUI 验收。

## 备注

- 这次 report 只记录现象，不记录根因。
- 当前已经有足够的新鲜运行证据，建议下一步进入标准路径的阶段 2 根因分析。
