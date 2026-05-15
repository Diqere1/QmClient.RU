---
doc_type: architecture
status: current
created: 2026-05-06
last_reviewed: 2026-05-09
tags: [architecture, qmclient, ui, rmlui]
related_roadmap: [rmlui-full-replacement]
---

# QmClient 架构总入口

## 1. 项目简介

QmClient 是基于 DDNet / TaterClient 的第三方定制客户端。

这份架构总入口只记录“系统现在长什么样”，不写未来规划。RmlUI 相关内容也只收当前已经落地的宿主、模块、资源和约束。

## 2. 核心概念 / 术语表

- `Legacy UI`：现有 CUI / QmUI 路径，当前仍是默认回退路径。
- `RmlUI Host`：当前旧代码里的 UI 宿主入口；Monitoring HUD、debug overlay、menus / popup 仍各自保留 legacy render owner。
- `RmlUI Core`：管理 RmlUI backend、context、viewport、document 和可用性状态的封装。
- `RmlUI Layer Switchboard`：宿主级 layer 调度 owner，负责固定顺序、同帧去重、host/fallback owner 元数据和 runtime dispatch contract。
- `RmlUI Runtime`：管理 RmlUI module registry、layer-first frame request/result、diagnostics 和 fallback 判定的统一入口。
- `RmlUI Input Bridge`：管理交互式 RmlUI surface 的输入路由、cancel/release-state 和文本输入所有权保护。
- `RmlUI Backend`：当前的底层 RmlUI 图形接入层，负责初始化 RmlGL3 和 render interface。
- `RmlUI Render Bridge`：当前承接 RmlUI texture/scissor contract 的桥接层，把 bridge handle 与当前 desktop GL delegate handle 分开管理。
- `RmlUI Diagnostics`：用于记录 core / backend / HUD 可用性与失败原因的调试输出。

## 3. 子系统 / 模块索引

### 3.1 现有 UI 路径

- `Legacy HUD / Menu / Settings`：现有客户端 UI 体系，仍承担默认显示和 fallback。
- `CRmlUiLayerSwitchboard`：当前 RmlUI 的宿主级调度入口，负责把 `GAME_HUD`、`DEBUG_OVERLAY`、`MENU_PAGE`、`MENU_MODAL` 四个已接入 layer 的 host slot 收口到统一 dispatch contract。
- `CGameClient::RenderQmMonitoringHud`：当前第一条已完成验收的 concrete RmlUI surface 宿主入口；宿主先经过 switchboard，再决定是否执行 legacy HUD。Monitoring HUD 的 RmlUI document render 现已通过 backend-thread callback 执行，图表 overlay 仍走旧 `IGraphics` 路径。
- `CClient::RenderDebug`：当前已先进入 `DEBUG_OVERLAY` switchboard slot，再执行 legacy debug overlay。
- `CMenus::OnRender`：当前 menu page 与 popup 宿主已先进入 `MENU_PAGE` / `MENU_MODAL` switchboard slot；其中 `fullscreen_popup` 下的低风险提示型弹窗已可进入 `CRmlUiPopupModal`，其余 menu / popup 路径仍继续执行 legacy path。

### 3.2 RmlUI 现状子系统

详细子系统文档：[ui-rmlui-current.md](ui-rmlui-current.md)

- `CRmlUiBackend` - `src/engine/client/rmlui_backend.h/.cpp`
  - 当前的 RmlUI 底层接入层。
  - 现状：要求当前帧已经有可用 OpenGL context，初始化时直接构造 `RmlGL3`，再把 desktop `RenderInterface_GL3` 包进 `CRmlUiRenderBridge`。
  - 现状：`SetViewport`、`BeginFrame`、`EndFrame` 现在都先经过 bridge wrapper，再转发到当前 GL3 delegate。

- `CRmlUiRenderBridge` - `src/engine/client/rmlui_render_bridge.h/.cpp`
  - 当前的 RmlUI texture/scissor bridge 层。
  - 现状：负责维护 bridge texture registry、generated texture normalization contract 和 scissor translated clip state。
  - 现状：compiled geometry 仍走当前 desktop GL delegate；full backend-neutral draw bridge 和 input bridge 仍未完成。

- `CRmlUiCore` - `src/game/client/RmlUi/RmlUiCore.h/.cpp`
  - 当前的 RmlUI 运行时封装。
  - 现状：负责 backend 初始化、context 持有、viewport 更新、update 调用与失败状态跟踪。
  - 现状：对外暴露可用性和失败字符串，供宿主决定是否回退旧 UI。

- `CRmlUiRuntime` - `src/game/client/RmlUi/RmlUiRuntime.h/.cpp`
  - 当前的 RmlUI runtime shell。
  - 现状：负责 module registry、layer-first frame request/result、diagnostics gate/dedupe 和 fallback 判定。
  - 现状：不直接绘制旧 fallback；只返回 `RENDERED` / `SKIPPED_DISABLED` / `SKIPPED_UNAVAILABLE` / `FALLBACK_REQUIRED` 给宿主消费。

- `CRmlUiInputBridge` - `src/game/client/RmlUi/RmlUiInputBridge.h/.cpp`
  - 当前的 RmlUI 输入协议层。
  - 现状：负责把鼠标、按键、滚轮、文本输入、cancel action 和 release-state 映射成宿主可执行路由结果。
  - 现状：按 context 安装文本输入 handler，并保留 console / legacy text owner 的保护边界；当旧 owner 后续激活时，会主动归还平台文本输入所有权。

- `CRmlUiLayerSwitchboard` - `src/game/client/RmlUi/RmlUiLayerSwitchboard.h/.cpp`
  - 当前的宿主级 layer 调度 owner。
  - 现状：负责固定 `GAME_HUD` → `DEBUG_OVERLAY` → `MENU_PAGE` → `MENU_MODAL` 的 dispatch order，并把 host/fallback owner、surface tag 和 frame token 收口成统一 dispatch contract。
  - 现状：对没有绑定 RmlUI module 的 slot 稳定回 legacy path；不会越权绘制旧 UI。

- `CRmlUiMonitoringHud` - `src/game/client/RmlUi/RmlUiMonitoringHud.h/.cpp`
  - 当前唯一实际接入且已验收的 RmlUI 文档宿主。
  - 现状：负责加载 `data/qmclient/rmlui/monitoring_hud.rml` 和 `data/qmclient/rmlui/monitoring_hud.rcss`，并把 document render 与图表 overlay 绘制分开。
  - 现状：会记录 document 是否加载成功、surface contract、当前失败原因以及可用状态；这条实现已经是首个已验收迁移样板，但仍保持 mixed render + host fallback owner 的边界。

- `CRmlUiPopupModal` - `src/game/client/RmlUi/RmlUiPopupModal.h/.cpp`
  - 当前第二条实际接入且已验收的 RmlUI 文档宿主。
  - 现状：负责加载 `data/qmclient/rmlui/popup_modal.rml` 和 `data/qmclient/rmlui/popup_modal.rcss`，维护 popup view model、按钮/热键语义、pending action 和 document 结构校验。
  - 现状：只覆盖 `fullscreen_popup` 下的低风险提示型弹窗子集；输入与 fallback 仍由 runtime/input bridge/menus 宿主共同维持，popup module 本身不直接执行业务 callback。

- `RmlUiRenderHelpers` - `src/game/client/RmlUi/RmlUiRenderHelpers.h/.cpp`
  - 当前 RmlUI 渲染辅助工具。
  - 现状：为 monitoring HUD 的绘制和布局计算提供局部辅助，不是独立运行时。

- `ExportRmlUiMonitoringDiagnostics` - `src/game/client/gameclient.cpp`
  - 当前的 RmlUI 诊断导出入口。
  - 现状：在 Monitoring HUD runtime 或 surface 失败时输出 runtime/core/backend/HUD 状态和失败原因。

### 3.3 当前资产

- `data/qmclient/rmlui/monitoring_hud.rml`
- `data/qmclient/rmlui/monitoring_hud.rcss`
- `data/qmclient/rmlui/popup_modal.rml`
- `data/qmclient/rmlui/popup_modal.rcss`

这些资产构成当前已经挂到 RmlUI 宿主上的文档和样式资源链。

## 4. 关键架构决定

- 旧 UI 仍然是正式保留路径，RmlUI 不能把它替换成唯一实现。
- 当前 RmlUI 不是独立 UI 主循环；宿主级 layer 顺序现在由 `CRmlUiLayerSwitchboard` 固定，具体 legacy fallback 仍留在原宿主。
- 当前 RmlUI runtime shell、safe-mode policy、layer switchboard、render-command bridge 最小切片、texture/scissor bridge contract 和 input bridge baseline 都已进入 current-state，但 full backend-neutral render bridge 仍未落地为完整模块。
- 当前 RmlUI backend 仍然依赖有效 OpenGL context，这个是现状，不是目标。
- 当前 diagnostics 由宿主侧统一导出，目的是让 core、backend 和 HUD 的失败状态可见。

## 5. 已知约束 / 硬边界

- 当前已验收的 concrete RmlUI surface 只有两条：Monitoring HUD，以及 `fullscreen_popup` 下的低风险 popup modal 子集；debug overlay、menu page、`popup_menu` 和文本输入型 popup 仍未迁移成具体 RmlUI surface。
- 当前 backend 只实现了 GL3 路径，其他后端适配不在这个架构入口里伪造存在。
- 当前 RmlUI 宿主失败时必须允许回退到原宿主的旧路径；switchboard 只统一判定，不直接绘制 legacy UI。
- 当前 runtime 只统一模块注册、layer-first request/result 与 diagnostics；host dispatch order 与同帧 duplicate guard 由 switchboard 持有。
- 当前 input bridge 已经成为交互式 RmlUI surface 的宿主输入协议层，但它仍不接管菜单 / popup / 轮盘 / editor 的旧 GUI 生命周期 owner。
- 当前 input bridge 的激活判定不是“模块 toggle 开着就算 active”，而是必须同时满足宿主声明当前 surface 真的 active，legacy fallback 也按声明的 owner 落回对应旧宿主起点。
- 当前 Monitoring HUD 只把 document/core render 迁到 backend-thread callback；graph overlay 仍是旧 `IGraphics` 叠加路径。
- 当前 texture/scissor 语义已由 `CRmlUiRenderBridge` 持有，但 geometry/layer/filter/shader/full draw submission 仍依赖当前 desktop GL delegate。
- 当前 RmlUI 资源链只覆盖 monitoring HUD 与 popup modal 的文档、样式和局部渲染辅助，不代表 menu/debug/editor 已拥有完整资源体系。
