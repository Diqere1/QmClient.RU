---
doc_type: issue-analysis
issue: 2026-05-09-rmlui-settings-gray-overlay-esc-crash
status: draft
root_cause_type: state-pollution
related: [rmlui-settings-gray-overlay-esc-crash-report.md]
tags: [rmlui, settings, qmclient, scrollregion, clip, crash]
---

# RmlUI 设置页灰层与 Esc 闪退根因分析

## 1. 问题定位

| 关键位置 | 说明 |
|---|---|
| `src/game/client/gameclient.cpp:2422-2426` | 当前代码已经把 `CanRenderRmlUiMenuPilot()` 硬编码为 `false`，并明确注释 `MENU_PAGE RmlUi still conflicts with backend-thread GL ownership`。这意味着旧版 menu pilot 根因链在当前代码里已经不会命中。 |
| `src/game/client/components/menus.cpp:1718-1724` | `HasActiveRmlUiMenuPilot()` 依赖 `CanRenderRmlUiMenuPilot()`；当前它会在 settings 页面直接短路为 `false`。 |
| `src/game/client/components/qmclient/menus_qmclient.cpp:811` | QmClient 设置页在进入大模块布局前调用 `s_ScrollRegion.Begin(&MainView, &ScrollOffset, &ScrollParams)`，这一步会执行 `Ui()->ClipEnable(...)`。 |
| `src/game/client/ui_scrollregion.cpp:69-78` | `CScrollRegion::Begin()` 会压入 clip，`CScrollRegion::End()` 才会 `Ui()->ClipDisable()`。这一对必须成对调用，否则 clip 会污染后续整帧 UI。 |
| `src/game/client/components/qmclient/menus_qmclient.cpp:2068-2085` | `ShouldDeferQmClientSettingsTail()` 命中时，函数会在 `s_ScrollRegion.Begin(...)` 之后直接 `return`，但没有执行 `s_ScrollRegion.End()`。 |
| `src/game/client/components/menus_settings.cpp:167-200` | `SETTINGS_QMCLIENT` 进入时会通过 `BeginDeferredSettingsPage(...)` 打开 tail defer；默认 `gs_SettingsDeferredFrames = 1`，因此首次进入页面时 `ShouldDeferQmClientSettingsTail()` 是正常命中的。 |
| `src/game/client/gameclient.cpp:1883-1884` | 每帧先全屏 `Clear(...)`。截图里的大片灰色与 clear 色一致，说明不是额外绘制了一层灰遮罩，而是后续菜单渲染只发生在受污染的局部 clip / viewport 区域。 |
| `src/engine/client/backend/opengl/backend_opengl.cpp:33-40` | DDNet 自身只在显式 viewport 更新命令时调用 `glViewport(...)`，不是每帧强制重设；一旦有外部路径污染真实 GL 状态，后续 legacy 渲染可能长期沿用错误状态。 |
| `src/engine/external/rmlui/Backends/RmlUi_Renderer_GL3.cpp:913-962` | RmlUI GL3 后端会直接改写真实 `glViewport` / `glScissor` / framebuffer，再尝试恢复。它仍然是 Esc 崩溃链路的高风险候选，但当前没有来自当前 build 的新 dump 去证明 Esc 崩溃就发生在这里。 |

## 2. 失败路径还原

**正常路径**：进入 `PAGE_SETTINGS -> SETTINGS_QMCLIENT` 后，如果这是首次进入该页，`BeginDeferredSettingsPage(SETTINGS_QMCLIENT)` 会把 `gs_SettingsDeferredFrames` 设为 `1`。随后 `RenderSettingsQmClient(...)` 可以在首帧用 `ShouldDeferQmClientSettingsTail()` 走轻量占位内容，但无论是否 early return，都必须执行与 `s_ScrollRegion.Begin(...)` 对应的 `s_ScrollRegion.End()`，这样 `Ui()->ClipEnable(...)` / `Ui()->ClipDisable()` 才能配对，下一帧整个菜单仍然会按全屏布局正常绘制。

**失败路径**：进入 `SETTINGS_QMCLIENT` 首帧时，`RenderSettingsQmClient(...)` 先调用 `s_ScrollRegion.Begin(...)` 压入 clip，然后因为 `ShouldDeferQmClientSettingsTail()` 为真，直接走 `2068-2085` 的 early return。这个分支只做了提示文案、transition 处理和 `FinishDeferredQmVisualFrame(...)`，却没有调用 `s_ScrollRegion.End()`。结果是本帧留下的 clip 状态泄漏到后续菜单渲染，后续 legacy 设置页只能在一个中间矩形内绘制，而全屏 `ClearColor` 露成截图里的顶部、右侧和四周大片灰区。

**分叉点**：

- `src/game/client/components/qmclient/menus_qmclient.cpp:811` — scroll region 在首帧 defer 之前就启用了 clip。
- `src/game/client/components/qmclient/menus_qmclient.cpp:2068-2085` — defer-tail 早退没有执行 `s_ScrollRegion.End()`。
- `src/game/client/gameclient.cpp:2422-2426` — 当前代码已禁用 menu pilot，证明旧分析中的 `MENU_PAGE` 整屏 RmlUi 壳层不再是当前灰层的执行路径。

## 3. 根因

**根因类型**：`state-pollution`

**根因描述**：当前灰层不是“Roadmap 还没走到 RmlUI 可用”导致的占位行为，也不是当前代码里仍在运行的 menu pilot 壳层。当前代码已经把 menu pilot 入口关掉了。真正已经读代码确认的根因，是 `SETTINGS_QMCLIENT` 首帧 defer 分支在 scroll region 开启之后直接 early return，导致 UI clip 状态泄漏到后续整帧 / 后续帧。这会把 legacy 设置页长期裁进一个中间矩形，外侧只剩 `OnRender()` 一开始清出来的灰色背景，因此表现成“内容区正常，但上方和右边大片发灰”。

**Esc 崩溃状态**：灰层这条根因已确认；`Esc` 崩溃暂时还不能归到同一条根因。当前能确认的只是：旧的 menu pilot `Esc` 分析同样已经失效，因为 menu pilot 当前不会激活；而现有 crash report 混用了两个不同可执行文件路径，且新加的 `rmlui_*.txt` 诊断文件还没有从当前 build 产出，因此 `Esc` 崩溃仍需一轮基于当前 `build-ninja/DDNet.exe` 的新证据来收敛。

**是否有多个根因**：是。

- 已确认主根因：QmClient 设置页 defer-tail 早退导致 scroll-region clip 泄漏。
- 待确认次根因：`Esc` 关闭链路可能仍与 RmlUI 的真实 GL 状态恢复 / frame-context acquire-release 冲突有关，但当前证据不足以把它钉到具体 file:line。
- 额外说明：后续在“进入服务器 / 打开聊天框”的 runtime 崩溃已经被单独确认为 `CCharacterCore` 快照复制污染问题，不应再和本 issue 混成同一个根因。

## 4. 影响面

- **影响范围**：当前直接影响 `SETTINGS_QMCLIENT` 首帧进入路径；任何未来在 `s_ScrollRegion.Begin(...)` 之后 early return、却忘记 `End()` 的设置页分支都会复制同类问题。
- **潜在受害模块**：QmClient 设置页所有复用 deferred-tail / scroll-region 模式的 tab；此外，只要 RmlUI 继续以“直接改真实 GL 状态后再恢复”的方式嵌入 DDNet，`Esc` 崩溃链路仍可能波及 `popup_modal`、`monitoring_hud` 等其他 RmlUI surface。
- **数据完整性风险**：灰层本身没有持久化数据损坏证据；`Esc` 崩溃仍需新 dump 才能判断是否伴随状态破坏或空指针路径。
- **严重程度复核**：维持 `P1`。灰层 100% 复现且进入核心设置页即中招；即便把 `Esc` 崩溃先视为独立问题，单灰层也已足够构成严重 UI 回归。

## 5. 修复方案

### 方案 A：先修灰层，补齐 scroll-region 收尾
- **做什么**：在 `RenderSettingsQmClient()` 的 defer-tail 早退分支里，补上 `s_ScrollRegion.End()`，确保 `Begin()` / `End()` 成对；同时把同类路径做一次局部扫查，避免 QmClient 设置页别的 early return 再漏 clip 收尾。
- **优点**：直接命中当前已经确认的 file:line 根因；改动小，验收信号清晰，最有机会立刻消掉灰层。
- **缺点 / 风险**：只解决灰层，不保证 `Esc` 崩溃同时消失。
- **影响面**：主要在 `src/game/client/components/qmclient/menus_qmclient.cpp`，可能顺带补一两个本文件的收尾保护。

### 方案 B：修灰层同时补 GL 状态漂移诊断
- **做什么**：在方案 A 基础上，再给 RmlUI frame-context 渲染路径补“进入前 / 退出后真实 viewport、scissor、framebuffer”采样，并在状态未恢复时导出到 `rmlui_*.txt`。
- **优点**：能把当前还未钉死的 `Esc` 崩溃从“怀疑 RmlUI 状态恢复冲突”推进到可证伪的日志证据；符合这次任务里“不要靠猜，要把调试工具补齐”的方向。
- **缺点 / 风险**：改动面比方案 A 大；需要小心调试采样本身不要再污染主渲染状态。
- **影响面**：`menus_qmclient.cpp` + `gameclient.cpp` + RmlUI / backend 调试辅助代码。

### 方案 C：先做运行路径/崩溃证据收口，再修
- **做什么**：不立即改逻辑，先把当前 `build-ninja/DDNet.exe` 的 build-id / exe path / crash breadcrumb 强制打进日志与 dump，确认用户验收的是当前工作树构建，再针对 `Esc` crash 做一次新的可符号化采样。
- **优点**：最稳妥，能避免继续被“不同 exe 路径”误导。
- **缺点 / 风险**：不能立刻消除灰层；用户侧体验改善最慢。
- **影响面**：主要是调试基础设施和日志。

### 推荐方案

**推荐方案 A 或 B，优先 B**。如果目标只是尽快让灰层消失，A 足够；但结合这轮排查里已经暴露出的 RmlUI / GL 状态不透明问题，以及 `Esc` 崩溃还没被新证据钉死，B 更合适。它先把已经确认的灰层根因修掉，同时把下一轮 `Esc` 崩溃所需的关键诊断补上，能形成真正闭环，而不是修完灰层后继续靠猜。 
