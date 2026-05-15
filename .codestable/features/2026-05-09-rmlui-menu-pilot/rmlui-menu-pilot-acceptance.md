---
doc_type: feature-acceptance
feature: 2026-05-09-rmlui-menu-pilot
status: pass
created: 2026-05-10
tags: [rmlui, menu, settings, acceptance]
related_design: rmlui-menu-pilot-design.md
related_checklist: rmlui-menu-pilot-checklist.yaml
---

# RmlUI 菜单试点验收报告

> 阶段：阶段 3（验收闭环）
> 验收日期：2026-05-10
> 关联方案 doc：`.codestable/features/2026-05-09-rmlui-menu-pilot/rmlui-menu-pilot-design.md`

## 1. 接口契约核对

- [x] `src/game/client/RmlUi/RmlUiMenuPilot.h` 已落地 `SRmlUiMenuPilotTab`、`SRmlUiMenuPilotViewModel`、`SRmlUiMenuPilotAction` 与 `CRmlUiMenuPilot`，与 design 中“页面壳只回传宿主语义 token，不直接重写业务状态”的接口方向一致。
- [x] `CRmlUiMenuPilot::SSurfaceContract`、`ERenderFailure`、`FailureReason(...)` 与 `FailureStageName(...)` 已把页面壳失败语义收口到 `menu_pilot_unavailable`、`document_missing`、`document_invalid`、`content_slot_invalid`、`restart_slot_invalid`，并区分 `surface_document` / `surface_rect`。
- [x] `src/game/client/gameclient.cpp` 已把 `menu_pilot` 注册为 `MENU_PAGE` 模块，配置开关为 `qm_rmlui_menu_pilot`，文档路径为 `qmclient/rmlui/menu_pilot.rml`，宿主继续通过 `CMenus::RenderSettings(...)` 消费结果。
- [x] `src/game/client/components/menus.cpp` 已落地 `HasActiveRmlUiMenuPilot()`、`BuildRmlUiMenuPilotViewModel(...)` 与 `ConsumeRmlUiMenuPilotAction(...)`，说明 settings 页壳 view model、state gate 与宿主动作回接已经正式进入 `CMenus` 宿主边界。
- [x] `src/game/client/components/menus_settings.cpp` 已把 `RenderSettings(...)` 接到 `GameClient()->RenderRmlUiMenuPilot(...)`，并通过 content slot / restart slot 承载 legacy settings content 与 restart bar，而不是再渲染旧设置页整壳。

## 2. 行为与决策核对

- [x] `PAGE_SETTINGS` 仍是本 feature 唯一 concrete `MENU_PAGE` surface。`IsRmlUiMenuPilotSettingsPage(...)` 的 allowlist 只覆盖 `LANGUAGE/GENERAL/PLAYER/TEE/APPEARANCE/CONTROLS/GRAPHICS/SOUND/DDNET/ASSETS/TCLIENT/QMCLIENT`，明确排除了 `PROFILES/CONFIGS/CONTRIBUTORS`。
- [x] settings 页壳继续走“RmlUI shell + legacy content island”模式。`RenderSettings(...)` 成功进入 menu pilot 后，只把 `RenderSettingsContent(...)` 画进 content slot，没有把设置项内容整体迁成 RmlUI 原生控件。
- [x] 页面导航仍通过 `g_Config.m_UiSettingsPage` 驱动。`ConsumeRmlUiMenuPilotAction(...)` 只在 `SELECT_SETTINGS_PAGE` 时写入已有 settings page 语义，没有新造第二套页面状态机。
- [x] fallback_to_legacy / auto_fallback_to_legacy 会把 `m_RmlUiMenuPilotDismissed` 置位，并由宿主回到 legacy settings page；没有通过全局关掉 `qm_rmlui_enable` 的方式强制退出试点。
- [x] 页面壳与 modal 同属菜单侧 context domain，但行为上保持“page 在下、modal 在上”。`HasActiveRmlUiMenuPilot()` 会在 `m_Popup != POPUP_NONE` 时关闭 page surface 激活；同时 `RenderSettings(...)` 的 fallback 说明也已把 popup active 作为明确原因暴露出来。
- [x] deactivation 已改为 backend 线程 `HideNow()`，不会再依赖“下一次 render 时顺带隐藏”的延后清理。`RunRmlUiMenuPilotDeactivate()` 现已直接调用 `m_RmlUiMenuPilot.HideNow()`。

## 3. 验收场景核对

- [x] **S1：settings page state gate**
  - 证据来源：`RmlUiMenuPilot.StateGateOnlyAllowsApprovedSettingsHosts`
  - 结果：通过。只允许 offline `PAGE_SETTINGS` 和 ingame `GAME_PAGE_SETTINGS` 这两类已批准宿主进入 page shell；popup active、dismissed、show start、错误 client state 都会拒绝进入。

- [x] **S2：页签范围守护**
  - 证据来源：`RmlUiMenuPilot.SettingsPageAllowlistMatchesApprovedPilotTabs`
  - 结果：通过。允许的 settings 页集合与 design 一致，`PROFILES/CONFIGS/CONTRIBUTORS` 明确未作为独立 pilot tab 暴露。

- [x] **S3：页面壳动作与 fallback 语义**
  - 证据来源：代码核对 `CMenus::ConsumeRmlUiMenuPilotAction(...)`、`CRmlUiMenuPilot::ProcessEvent(...)`
  - 结果：通过。页面壳只产生 `SELECT_SETTINGS_PAGE`、`FALLBACK_TO_LEGACY`、`AUTO_FALLBACK_TO_LEGACY` 语义动作，宿主继续掌管页面切换与 dismiss 状态。

- [x] **S4：content slot / restart slot 承载**
  - 证据来源：代码核对 `CRmlUiMenuPilot::RenderDocument(...)`、`CMenus::RenderSettings(...)`
  - 结果：通过。menu pilot 成功后，legacy settings content 被定向画入 `menu-pilot-content-slot`，restart bar 在 `NeedRestart` 为真且 restart slot 有效时继续由 legacy renderer 输出。

- [x] **S5：popup 抢占与输入越权守护**
  - 证据来源：代码核对 `HasActiveRmlUiMenuPilot()`、`RenderSettings(...)` 与 design checklist
  - 结果：通过。popup active 时 `MENU_MODAL` 继续优先，page shell 不再争抢输入 owner；menu pilot 失败或 popup 激活时整页回退 legacy settings。

- [x] **S6：surface diagnostics**
  - 证据来源：`RmlUiMenuPilot.FailureMetadataMapsSurfaceFailuresToExpectedDiagnosticsBuckets`
  - 结果：通过。页面壳失败现在有稳定的 failure reason / stage name，可直接进 runtime diagnostics 与 fallback 文案。

- [x] **S7：render view / host payload**
  - 证据来源：`RmlUiMenuPilot.ResolveRenderViewPrefersProvidedSurfaceRect`、`RmlUiMenuPilot.ResolveRenderViewFallsBackToViewportForInvalidRect`
  - 结果：通过。surface payload 给出矩形时优先用实际宿主 view；无效矩形时安全回退 viewport，不把 page shell 渲染到未定义区域。

- [x] **S8：menu-pilot checklist 收口**
  - 证据来源：`.codestable/features/2026-05-09-rmlui-menu-pilot/rmlui-menu-pilot-checklist.yaml`
  - 结果：通过。`steps 5/5 done`、`checks 8/8 done`，说明 design 中承诺的页面壳、content island、导航/fallback、证据闭环都已经完成。

## 4. 术语一致性

- [x] `menu pilot module` 在代码中对应 `CRmlUiMenuPilot`，没有再引入第二套 settings page shell 命名。
- [x] `menu pilot action token` 在代码中统一落为 `ERmlUiMenuPilotAction` / `SRmlUiMenuPilotAction`。
- [x] `legacy content island` 在行为上已经落到 `RenderSettingsContent(...)` 进入 content slot，不再是仅停留在设计里的抽象说法。

## 5. 架构归并

- [x] `.codestable/architecture/ARCHITECTURE.md` 需要把 `PAGE_SETTINGS` 记为当前第一条已验收的 `MENU_PAGE` concrete RmlUI surface。
- [x] `.codestable/architecture/ui-rmlui-current.md` 需要回写“RmlUI 页面壳 + legacy settings content island”的 current-state，以及 `MENU_PAGE` / `MENU_MODAL` 的菜单侧 context domain 约束。
- [x] 不回写：settings 全量原生 RmlUI 化、settings search、Click GUI、popup_menu 与文本输入型 popup 迁移。

## 6. requirement 回写

- [x] requirement `rmlui-full-replacement` 仍保持愿景层，不把 settings fully-native migration 写成 current。
- [x] 本次实现满足“首个页面级 `MENU_PAGE` surface 已闭环”的阶段目标，因此应把 `2026-05-09-rmlui-menu-pilot` 追加到 `implemented_by` 并补充 current-state 变更日志。

## 7. roadmap 回写

- [x] `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-items.yaml` 中 `rmlui-menu-pilot` 可从 `in-progress` 改为 `done`。
- [x] `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-roadmap.md` 中 `rmlui-menu-pilot` 需要同步改为 `done`，并把 `rmlui-settings-reorg` 从“已可开 design”推进为“design 已完成，等待实现”。
- [x] `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-readiness-matrix.md` 需要把 `rmlui-menu-pilot` 从 `implemented-pending-acceptance` 改为 `done`，并解除 `rmlui-settings-reorg` 的前置等待，允许它进入实现。
- [x] `.codestable/features/RMLUI_FEATURE_INDEX.md` 需要把 `rmlui-menu-pilot` 从“已批准设计，实施中”收口进已验收基线。

## 8. attention.md 候选盘点

- [x] 本 feature 没有新增必须追加到 `attention.md` 的环境约束；现有“RmlUI/menu/popup 与 HUD context 默认分离”“最终 UI 表现由人工验收完成”的规则已经足够覆盖本轮。

## 9. 遗留

- 当前已验收的是 settings page shell 重用 legacy content island，不是 settings 全量原生 RmlUI 重写。
- `rmlui-settings-reorg` 现在应该接棒成为 settings 主线，去处理新的信息架构、destination adapter，以及 `QmClient/TClient` 二级分页与旧内容链的统一承载。
- `popup_menu`、文本输入型 popup、settings search、Click GUI 和 HUD editor 仍在后续 feature 范围内，不属于本次 menu-pilot 验收闭环。
