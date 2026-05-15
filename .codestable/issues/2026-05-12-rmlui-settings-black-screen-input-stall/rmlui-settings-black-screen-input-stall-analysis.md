---
doc_type: issue-analysis
issue: 2026-05-12-rmlui-settings-black-screen-input-stall
status: confirmed
root_cause_type: concurrency
related: [rmlui-settings-black-screen-input-stall-report.md]
tags: [rmlui, settings, menu-pilot, input, backend-thread, lifecycle]
---

# RmlUI Settings Black Screen Input Stall 根因分析

## 1. 问题定位

| 关键位置 | 说明 |
|---|---|
| `src/game/client/gameclient.cpp:818-830` | `CRmlUiInputBridge` 在初始化时被绑定到 `ProcessRmlUiContextMouseMove/MouseButton/Key/TextInput` 这组直接调用 `Rml::Context` raw input API 的回调。 |
| `src/game/client/gameclient.cpp:1382-1508` | 每帧 `OnUpdate()` 会先根据 `ActiveInputModule()` 选择 active context，然后在主线程直接把鼠标、按键、文本事件投递到该 `Rml::Context`。 |
| `src/game/client/gameclient.cpp:2890-2900` | `menu_pilot` 模块注册为 `MENU_PAGE`，并且 `m_RequiresInput=true`，其 input active callback 只看 `HasActiveRmlUiMenuPilot()`。 |
| `src/game/client/gameclient.cpp:3045-3052` | `menu_pilot` 的 surface render 不是在主线程完成，而是通过 `Graphics()->RunOnBackendThread(...)` 进入 backend-thread callback。 |
| `src/game/client/gameclient.cpp:3285-3307` | backend-thread callback 中会对 `MENU_PAGE` context 执行 `SetViewport(...)`，再调用 `m_RmlUiMenuPilot.RenderDocument(...)`。 |
| `src/game/client/RmlUi/RmlUiMenuPilot.cpp:255-268` | `RenderDocument(...)` 在 backend 线程里执行 `UpdateDocument(...)`、`m_pCore->Update(MENU_PAGE)`、`m_pCore->Render(MENU_PAGE)`，并在此后读取页面几何。 |
| `src/game/client/components/menus_settings.cpp:3402-3411` | 只要 `RenderRmlUiMenuPilot(...)` 返回成功，宿主就直接 `return`，不会再进入 legacy settings content。 |
| `src/game/client/gameclient.cpp:455-458` | `IsRmlUiMenuPilotActive(...)` 只看 `m_Menus.HasActiveRmlUiMenuPilot()`，不看这一帧 RmlUI page surface 是否真实渲染成功或是否可交互。 |
| `src/game/client/RmlUi/RmlUiRuntime.cpp:176-207` | `ActiveInputModule()` 只根据模块启用状态、`m_RequiresInput` 和 `m_pfnInputActive` 选择 active input module，不关心上一帧 render 是否成功。 |

## 2. 失败路径还原

**正常路径**：进入 settings page 后，宿主决定激活 `menu_pilot`，输入和页面更新/渲染都应由同一 owner 路径顺序处理：先提交输入，再 `Context::Update()`，再 `Context::Render()`，最后宿主消费已解析好的可见 surface 结果。

**失败路径**：

1. 用户进入 settings page。
2. `CMenus::HasActiveRmlUiMenuPilot()` 返回真，`menu_pilot` 被选为 active input module。
3. 主线程 `OnUpdate()` 在 `gameclient.cpp:1382-1508` 中，直接把鼠标、按钮、键盘和文本事件投递给 `MENU_PAGE` context。
4. 同一 surface 的真正 document 更新与绘制，却在 `gameclient.cpp:3045-3052`、`3285-3307` 通过 `RunOnBackendThread(...)` 切到 backend 线程，再在 `RmlUiMenuPilot.cpp:255-268` 中执行 `UpdateDocument()`、`Context::Update()`、`Context::Render()`。
5. 宿主只看 `RenderRmlUiMenuPilot(...)` 的返回值决定是否跳过 legacy settings；而输入激活完全不看 render 成功与否。
6. 结果是：同一个 `MENU_PAGE` context 的输入所有权和 update/render 所有权被拆到了两条路径上。页面一旦进入“看起来激活但实际未稳定呈现”的状态，宿主仍会持续把输入路由给 RmlUI，用户看到的就是“黑屏 + 鼠标卡顿/点击没反应”。

**分叉点 1**：`src/game/client/gameclient.cpp:1382-1508` 与 `src/game/client/gameclient.cpp:3285-3307`  
同一 `MENU_PAGE` context 的 input submission 与 `Update()/Render()` 没有处在同一 owner/同一时序里。

**分叉点 2**：`src/game/client/gameclient.cpp:455-458`、`2890-2900`、`src/game/client/RmlUi/RmlUiRuntime.cpp:176-207`  
`menu_pilot` 的 input 激活条件只看 host gate，不看 surface 是否真实渲染成功，于是 broken page 仍会继续吞输入。

## 3. 根因

**根因类型**：`concurrency`

**根因描述**：当前 settings page 的 `menu_pilot` 进入了一个“分裂所有权”的状态。主线程把 raw input 直接提交给 `MENU_PAGE` 的 `Rml::Context`，backend 线程又在另一条路径里对同一个 context 做 `UpdateDocument()`、`Context::Update()` 和 `Context::Render()`。这违反了当前仓库给 settings host 收口出来的 context/lifecycle 边界，也不符合官方 RmlUi main-loop 契约。只要 page surface 没有稳定完成渲染，宿主却仍然认为它是 active input owner，就会把“页面没画好”放大成“页面没画好且输入被吞走”，最终表现成黑屏、鼠标卡顿、点击无响应。

**是否有多个根因**：是。

- **主根因**：`MENU_PAGE` context 的 input ownership 与 render ownership 被拆到主线程和 backend 线程两侧。
- **次级放大因素**：active input module 的判断只看 host gate，不看当前 surface 的 render 成功/可见成功，因此 broken page 仍会持续捕获输入。

**为什么不是别的原因**：

- 不是“旧 UI 还在并排渲染”导致的，因为当前 active path 在代码和 contract 上都已经明确禁止同帧并排 legacy settings content。
- 不是单纯的 RCSS/RML 布局问题。布局或 scissor 当然可能是页面发黑的直接视觉表现，但它解释不了“输入继续被吞、鼠标卡顿”的组合症状；而分裂 ownership 可以同时解释“可见性异常”和“交互异常”。
- 不是单一 fallback 逻辑错误。当前用户症状已经进入“看起来走了 active path，但结果不可用”的状态，而不是简单停留在 legacy fallback banner。

## 4. 影响面

- **影响范围**：直接影响当前 `menu_pilot` 驱动的 settings page RmlUI 试点，导致页面既不可靠可见，也不可靠可交互。
- **潜在受害模块**：`popup_modal` 与任何未来复用这套 `CRmlUiInputBridge + backend-thread RenderDocument` 组合的交互式 surface，都存在同类风险。
- **数据完整性风险**：当前看起来主要是 UI 状态与输入所有权不一致，暂无直接持久化数据损坏证据；但会产生页面状态卡住、输入被误消费、宿主/表面状态不一致这类运行期状态污染。
- **严重程度复核**：维持 `P1`。它已经阻塞当前 settings 页试点验收，而且一旦继续在这条模式上扩展 popup 或其他交互 surface，影响面会扩大。

## 5. 修复方案

### 方案 A：把 `MENU_PAGE` interactive surface 改成单 owner 路径

- **做什么**：不要再在主线程直接把 raw input 打进 `Rml::Context`。改为让主线程只收集/排队输入事件，真正的 `ProcessMouseMove/Button/Key/TextInput`、`Context::Update()`、`Context::Render()` 都在同一条 backend-thread page-frame callback 里顺序执行。
- **优点**：直接修主根因，最符合官方 main-loop 契约，也符合仓库当前 settings host / render-lifecycle 约束。
- **缺点 / 风险**：改动范围最大，需要触及 `CRmlUiInputBridge`、runtime、`menu_pilot` 和相关测试；如果处理不好会影响 popup 等后续 surface。
- **影响面**：`src/game/client/gameclient.cpp`、`src/game/client/RmlUi/RmlUiInputBridge.*`、`src/game/client/RmlUi/RmlUiRuntime.*`，以及 `menu_pilot` / `popup_modal` 相关测试。

### 方案 B：先加“render 成功才接管输入”的宿主护栏

- **做什么**：保留当前 render callback 结构，但让 `menu_pilot` 只有在上一帧或当前帧已确认 surface 可见/可用时才成为 active input module；一旦 render 失败、页面几何无效或 surface contract 不完整，就立刻把输入还给 legacy settings，并强制 fallback。
- **优点**：改动比方案 A 小，能先解决“黑屏时还吞输入”的最糟糕体验，快速恢复 settings 页可操作性。
- **缺点 / 风险**：这只是止血，不解决主根因；input ownership 和 render ownership 仍然是分裂的，后续仍可能出现黑屏或状态污染。
- **影响面**：主要是 `gameclient.cpp`、`menus.cpp`、runtime active input 选择逻辑与 diagnostics/fallback contract。

### 方案 C：临时撤回 `menu_pilot` 的交互试点，保留 legacy settings 作为正式路径

- **做什么**：在 `MENU_PAGE` 路径上暂时不让 `menu_pilot` 接管交互，可选做法包括硬禁用 `qm_rmlui_menu_pilot`、或把它降级成纯诊断/只读壳层，直到 input bridge 重新设计完成。
- **优点**：风险最低，能最快恢复用户可用的 settings 页，不再继续让不稳定 page surface 影响输入。
- **缺点 / 风险**：不是修复，只是撤回试点；settings host 主线会暂停，当前试点价值大幅下降。
- **影响面**：最小，主要影响 host gate 和试点开关语义。

### 推荐方案

**推荐方案 A**，理由：这次问题的主根因不是某个页面组件写错，而是 interactive RmlUI surface 的 ownership 模式本身不成立。方案 B 适合作为短期止血，但会保留根因；方案 C 只适合在你要先保稳定、暂缓试点时采用。如果目标还是把 settings host 往可验收的 RmlUI 主线推进，最终必须把 input submission、`Update()`、`Render()` 收回到同一个 owner 路径里。

> 2026-05-12 用户已确认采用方案 A，后续 fix 阶段按单 owner 路径收口。
