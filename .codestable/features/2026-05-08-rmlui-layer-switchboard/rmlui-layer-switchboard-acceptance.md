# rmlui-layer-switchboard 验收报告

> 阶段：阶段 3（验收闭环）
> 验收日期：2026-05-08
> 关联方案 doc：`.codestable/features/2026-05-08-rmlui-layer-switchboard/rmlui-layer-switchboard-design.md`

## 1. 接口契约核对

对照方案第 2.1 节名词层逐一核查：

**接口示例逐项核对**：
- [x] `SRmlUiLayerDispatchRule`（`src/game/client/RmlUi/RmlUiLayerSwitchboard.h`）：实际包含 `m_Layer`、`m_pHostOwner`、`m_pFallbackOwner`、`m_pStage` 与 `m_AttemptRuntimeRender`；和“固定 layer rule + host/fallback owner + runtime attempt gate”的设计一致。
- [x] `SRmlUiLayerDispatchRequest`（`src/game/client/RmlUi/RmlUiLayerSwitchboard.h`）：实际包含 `m_FrameToken` 与 `m_pSurfaceTag`。验收时已把 design 回填为 accepted contract，用于同帧状态重置与同层 modal surface 区分；未扩大 feature 范围。
- [x] `SRmlUiLayerDispatchResult`（`src/game/client/RmlUi/RmlUiLayerSwitchboard.h`）：实际包含 `m_RenderedRmlUi`、`m_ShouldRenderLegacy`、`m_RuntimeResult`、`m_pFailureReason`、`m_pStage`、`m_pHostOwner`、`m_pFallbackOwner`；与“switchboard 只统一判定，不替宿主画 fallback”的设计一致。

**名词层“现状 → 变化”逐项核对**：
- [x] `layer switchboard`：代码已落地为 `CRmlUiLayerSwitchboard`，不再停留在 runtime shell 或 host 内联逻辑。
- [x] `host dispatch point`：`CGameClient::RenderQmMonitoringHud`、`CClient::RenderDebug`、`CMenus::OnRender` 均已有实际 switchboard 调用点。
- [x] `legacy fallback chain`：实际仍由原宿主执行，switchboard 只返回 `m_ShouldRenderLegacy=true` 或 runtime 非 `RENDERED` 的判定结果。
- [x] `dispatch order contract`：`CRmlUiLayerSwitchboard::Dispatch` 通过 rule order、frame token 和 surface tag 维持同帧顺序与去重。

**流程图核对**（第 2.2 节开头 mermaid 图）：
- [x] 图中 `Monitoring HUD host` / `CClient::RenderDebug` / `CMenus::OnRender` → `CRmlUiLayerSwitchboard` → `CRmlUiRuntime::RenderRmlUiLayer(...)` 的主调用关系在代码均有实际落点。

## 2. 行为与决策核对

对照方案第 1 节 + 第 2.2 节：

**需求摘要逐项验证**：
- [x] `GAME_HUD`、`DEBUG_OVERLAY`、`MENU_PAGE`、`MENU_MODAL` 四个 layer 均有明确宿主 dispatch point。证据：`gameclient.cpp` 的 `RenderQmMonitoringHudRmlUi`，`client.cpp` 的 `DispatchRmlUiDebugOverlaySlot()`，`menus.cpp` 的 `DispatchRmlUiMenuPageSlot()` / `DispatchRmlUiMenuModalSlot(...)`。
- [x] Monitoring HUD 先进入 switchboard，再进入 runtime；旧 HUD fallback 仍由宿主执行。证据：`RenderQmMonitoringHudRmlUi(...)` 先构造 `SRmlUiLayerDispatchRequest`，再调用 `DispatchRmlUiLayer(...)`；非 `RENDERED` 时由宿主继续 legacy path。
- [x] debug/menu/popup 当前已先过 switchboard，再回 legacy path。证据：`client.cpp`、`menus.cpp` 的 host slot 调用存在，而 `CRmlUiLayerSwitchboard::Dispatch(...)` 对未绑定 runtime 的 rule 返回 `layer_slot_unbound` + `m_ShouldRenderLegacy=true`。
- [x] frame 内顺序固定。证据：`CRmlUiLayerSwitchboard::Dispatch(...)` 对越序返回 `dispatch_order_violation`，测试 `OutOfOrderDispatchFallsBackBeforeRuntime` 已覆盖。

**明确不做逐项核对**：
- [x] 没有实现 input bridge。grep 结果只涉及 host dispatch / runtime request / tests，没有新增输入分发模块。
- [x] 没有迁移 menu page、popup 或 debug HUD 的具体 RmlUI surface。当前只有 `monitoring_hud` module 注册为实际 RmlUI module。
- [x] 没有宣称 `RADIAL_OVERLAY` / `EDITOR_OVERLAY` 当前宿主已实现。switchboard 只注册并使用了四个既定 slot。
- [x] 没有改变 legacy menu / popup / debug HUD 的视觉与交互行为。当前只是加了 slot 调用，legacy render 仍在原宿主中执行。
- [x] 没有把 runtime 改成新的全局 UI 主循环。runtime 仍是单层 `RenderRmlUiLayer(...)`，host order 由 switchboard 持有。

**关键决策落地**：
- [x] D1“新增 switchboard owner，不把 runtime 升级成 host scheduler”：已落地为 `CRmlUiLayerSwitchboard`，runtime 仍保持单次 layer render 接口。
- [x] D2“只收口四个宿主层”：实际 rule 集合只覆盖 `GAME_HUD`、`DEBUG_OVERLAY`、`MENU_PAGE`、`MENU_MODAL`。
- [x] D3“Monitoring HUD 仍由原宿主持有 fallback owner”：`gameclient.cpp` 中 `RenderQmMonitoringHud` / `RenderQmMonitoringHudRmlUi` 仍是最终 legacy fallback 执行者。
- [x] D4“debug/menu/popup 先占位收口宿主顺序”：`menus.cpp` 已对 `connecting_popup`、`loading_popup`、`fullscreen_popup`、`popup_menu` 加入 modal slot 调用。
- [x] D5“diagnostics 仍留在 runtime/host 导出链”：本 feature 未新增第二套 diagnostics 系统。

**编排层“现状 → 变化”逐项核对**：
- [x] Monitoring host 从 direct runtime call 改为 switchboard dispatch。证据：`RenderQmMonitoringHudRmlUi(...)` 只调 `DispatchRmlUiLayer(...)`，没有再直接调 runtime。
- [x] debug/menu host 增加了固定 slot。证据：`DispatchRmlUiDebugOverlaySlot()`、`DispatchRmlUiMenuPageSlot()`、`DispatchRmlUiMenuModalSlot(...)`。

**流程级约束核对**：
- [x] “switchboard 是唯一允许把 host render request 送进 runtime 的宿主级入口”：当前 `gameclient.cpp` 的 runtime 调用只出现在 switchboard runtime callback 路径。
- [x] “fallback 执行者仍是原宿主”：`Dispatch(...)` 返回结果只带判定，不调用任何 legacy render。
- [x] “没有 module / module disabled / runtime unavailable 时稳定回 legacy path”：`layer_slot_unbound`、`runtime_dispatch_missing`、runtime 非 `RENDERED` 都保留 `m_ShouldRenderLegacy=true`。

**挂载点反向核对（可卸载性）**：
- [x] `src/game/client/RmlUi/RmlUiLayerSwitchboard.*`：新增 owner 本体，与 design 一致。
- [x] `src/game/client/gameclient.*`：Monitoring HUD host 已改为通过 switchboard 调度，与 design 一致。
- [x] `src/engine/client/client.cpp`：debug overlay host 已进入 switchboard slot，与 design 一致。
- [x] `src/game/client/components/menus.cpp`：menu page / popup 宿主已进入 switchboard slot，与 design 一致。
- [x] `src/game/client/RmlUi/RmlUiRuntime.*`：只补了 `m_pSurfaceTag` 到 frame request；没有把 runtime 重新升级成 host scheduler。
- [x] 反向 grep 未发现新的宿主直连 runtime 路径；同类 runtime 调用都收口到了 switchboard 回调中。
- [x] 拔除沙盘推演：如果移除 switchboard 文件与四个 host slot 调用，debug/menu/popup 会直接回到纯 legacy host；Monitoring HUD 只需恢复原 direct runtime call 路径，不存在散落在其它宿主里的第二套调度残留。

## 3. 验收场景核对

对照方案第 3 节关键场景清单，逐条可观察证据验证：

- [x] **S1**：Monitoring HUD 宿主尝试走 RmlUI → 先进入 switchboard，再由 switchboard 调 runtime；render 成功时跳过 legacy HUD，失败时回旧 HUD。
  - 证据来源：代码核对 + 单测 `RuntimeRenderedSkipsLegacyAndForwardsRequest` / `RuntimeFallbackKeepsLegacyPath`
  - 结果：通过

- [x] **S2**：当前没有对应 RmlUI 模块的 debug overlay / menu page / popup 宿主 → 先经过 switchboard slot，再稳定回 legacy path。
  - 证据来源：代码核对 + 单测 `UnboundSlotReturnsLegacyWithoutRuntimeDispatch`
  - 结果：通过

- [x] **S3**：frame 内同时存在多个已接入宿主 → 调度顺序固定，不由单个宿主临时决定插入时机。
  - 证据来源：单测 `OutOfOrderDispatchFallsBackBeforeRuntime`
  - 结果：通过

- [x] **S4**：runtime 返回 `SKIPPED_DISABLED` / `SKIPPED_UNAVAILABLE` / `FALLBACK_REQUIRED` → legacy fallback chain 仍由原宿主执行。
  - 证据来源：单测 `RuntimeFallbackKeepsLegacyPath` + switchboard result contract
  - 结果：通过

- [x] **S5**：构建与回归 → switchboard 代码、targeted tests、`game-client` 重建通过。
  - 证据来源：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target testrunner -j 1`、`.\\build-ninja\\testrunner.exe --gtest_filter=RmlUiLayerSwitchboard.*`、`qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 1`
  - 结果：通过；`RmlUiLayerSwitchboard` 7/7 tests passed，`DDNet.exe` 重新链接成功

## 4. 术语一致性

对照方案第 0 节 + 第 2.1 节命名 grep 代码：

- `layer switchboard`：代码命中为 `CRmlUiLayerSwitchboard`，与设计一致。
- `host dispatch point`：代码命中为 `DispatchRmlUiLayer(...)`、`DispatchRmlUiDebugOverlaySlot()`、`DispatchRmlUiMenuPageSlot()`、`DispatchRmlUiMenuModalSlot(...)`，与设计一致。
- `surface tag`：代码统一使用 `m_pSurfaceTag`，无第二套 `popup kind` / `modal id` 命名分叉。
- 防冲突：未引入 `RmlUiLayerManager` 作为 current implementation 名称；该词仍只保留在 roadmap/module 层。

## 5. 架构归并

- [x] 架构 doc ` .codestable/architecture/ARCHITECTURE.md `：已写入 `RmlUI Layer Switchboard` 作为 current subsystem，并回填当前 host seams、runtime/switchboard 分工、以及“只有 Monitoring HUD 有实际 module，debug/menu/popup 只是 host slot”。
- [x] 架构 doc ` .codestable/architecture/ui-rmlui-current.md `：已把 current scope 从“只有 Monitoring HUD host”更新为“Monitoring HUD content + accepted switchboard host seams”，并新增 `CRmlUiLayerSwitchboard` current role / constraints、更新 current flow 与 failure semantics。

判定：归并后，不读 design 也能从 architecture 看出当前系统里已经有 switchboard，但 concrete menu/debug/popup surface 还没迁移。

## 6. requirement 回写

- [x] requirement ` .codestable/requirements/rmlui-full-replacement.md ` 已更新：把 `2026-05-08-rmlui-layer-switchboard` 追加到 `implemented_by`，并补充变更日志。
- [x] requirement 状态保持 `draft`：本次只验收 host dispatch order / fallback ownership baseline；完整的 RmlUI 替代能力、interactive input、concrete menu/debug/popup migration 仍未 current。

## 7. roadmap 回写

- [x] ` .codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-items.yaml `：`rmlui-layer-switchboard` 已从 `in-progress` 改为 `done`，并补充 accepted 范围说明。
- [x] ` .codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-roadmap.md `：子 feature 清单第 5 项已同步为 `done`，并补 current host seam 状态。
- [x] ` .codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-readiness-matrix.md `：`rmlui-layer-switchboard` 已同步为 `done`；`rmlui-monitoring-hud-migration` 与 `rmlui-input-bridge` 已从等待基线改为 `ready-for-design`。
- [x] ` .codestable/features/RMLUI_FEATURE_INDEX.md `：已把 `rmlui-layer-switchboard` 移到 accepted baseline，并收录 acceptance report。

## 8. attention.md 候选盘点

- [x] 本 feature 未暴露需要补入 `attention.md` 的新环境 / 工具 / 工作流约束。

## 9. 遗留

- 后续优化点：优先进入 `rmlui-monitoring-hud-migration` 或推进已有 `rmlui-input-bridge` draft。
- 已知限制：当前只有 Monitoring HUD 有实际 RmlUI module；debug/menu/popup 只是 accepted host slot，不代表 concrete surface migration 已完成。
- 已知限制：full backend-neutral render bridge 仍未完成；当前只是在 accepted minimal bridge + texture/scissor bridge 基线上新增 host dispatch order。
- 已知限制：本次验收的运行证据以 targeted test、构建和既有启动日志为主，不把 concrete UI 页面级肉眼验收误写成已完成。
