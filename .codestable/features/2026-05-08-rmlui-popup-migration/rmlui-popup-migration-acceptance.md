# RmlUI 弹窗迁移验收报告

> 阶段：阶段 3（验收闭环）
> 验收日期：2026-05-09
> 关联方案 doc：`.codestable/features/2026-05-08-rmlui-popup-migration/rmlui-popup-migration-design.md`

## 1. 接口契约核对

- [x] `src/game/client/RmlUi/RmlUiPopupModal.h` 已落地 `ERmlUiPopupKind`、`SRmlUiPopupViewModel`、`ERmlUiPopupAction` 与 `CRmlUiPopupModal`，和 design 中“popup module 自持语义动作、宿主只消费 action token”的接口方向一致。
- [x] `CRmlUiPopupModal::ERenderFailure`、`FailureReason(...)`、`FailureStageName(...)` 已把 popup surface 失败语义收口到 `popup_unavailable`、`document_missing`、`document_invalid` 与 `surface_document` / `surface_render`，没有把 popup 失败继续压回宿主私有字符串。
- [x] `src/game/client/gameclient.cpp` 已把 `popup_modal` 注册为 `MENU_MODAL` 模块，配置开关是 `qm_rmlui_popup`，fallback owner 指向 `CMenus::OnInput`，文档路径指向 `qmclient/rmlui/popup_modal.rml`。
- [x] `src/game/client/components/menus.cpp` 已把 fullscreen popup 宿主接到 `RenderRmlUiPopupModal(...)`，并通过 `CMenus::ConsumeRmlUiPopupAction(...)` 复用现有 callback / `NextPopup` 语义，而不是让 RmlUI 直接持有业务回调。
- [x] `RmlUiPopupModal.MigratableFullscreenPopupsSkipLegacyPreDispatchSlot` 已覆盖“可迁移 fullscreen popup 不再先走 legacy pre-dispatch slot”的宿主判定修复，避免 `fullscreen_popup` 先被旧 slot 占用后导致本帧永远回不到 RmlUI surface。

## 2. 行为与决策核对

- [x] 迁移范围已收紧到 `POPUP_MESSAGE`、`POPUP_CONFIRM`、`POPUP_WARNING`、`POPUP_DISCONNECTED`、`POPUP_QUIT`、`POPUP_RESTART`；`POPUP_PASSWORD`、`POPUP_RENDER_DEMO`、`POPUP_SAVE_SKIN`、`popup_menu` 仍完整保留 legacy 路径。
- [x] popup module 自己解释按钮与热键语义。`ResolveConfirmAction(...)` / `ResolveEscapeAction(...)` 负责把 Enter / Escape 映射成 `ACKNOWLEDGE`、`CONFIRM`、`CANCEL`、`ABORT_RECONNECT`；runtime-shell 不解析 DOM 事件参数。
- [x] popup active predicate 已收口为“全局开关开启 + 模块开关开启 + menus active + 当前 popup 命中可迁移集合”。`CMenus::IsRmlUiPopupModalActive()` 与 runtime `ActiveInputModule()` 的联合判断已符合 design。
- [x] 迁移后确认/取消动作仍复用现有 `CMenus` 行为。宿主消费 `ERmlUiPopupAction` 后继续调用原有 `BUTTON_CONFIRM` / `BUTTON_CANCEL` 对应的 callback / `NextPopup`，没有改写业务状态机。
- [x] 栖梦设置页图形开关边界保持不变：仍只保留全局 `qm_rmlui_enable`，没有重新引入 Monitoring 专属图形开关，也没有把 popup 模块开关暴露成第二个设置页按钮。

## 3. 验收场景核对

- [x] **S1：低风险单按钮弹窗**
  - 证据来源：`RmlUiPopupModal.ConfirmAndEscapeActionsMatchApprovedPopupSemantics`、`RmlUiPopupAssets.DocumentIncludesExpectedPopupAnchors`
  - 结果：通过。`MESSAGE` / `WARNING` 的 Enter / Escape 都会落成 `ACKNOWLEDGE`，文档锚点完整存在，静态文案只保留 `RmlUI` marker。

- [x] **S2：低风险双按钮确认弹窗**
  - 证据来源：`RmlUiPopupModal.PendingActionOnlyReturnsSemanticTokenAndResetsAfterConsume`、代码核对 `CMenus::ConsumeRmlUiPopupAction(...)`
  - 结果：通过。`CONFIRM` / `QUIT` / `RESTART` 已由 popup module 只上报语义 token，确认与取消动作继续复用旧 callback / `NextPopup` 语义。

- [x] **S3：断线弹窗动态语义**
  - 证据来源：`RmlUiPopupModal.ConfirmAndEscapeActionsMatchApprovedPopupSemantics`
  - 结果：通过。`DISCONNECTED` 在普通状态下走 `ACKNOWLEDGE`，带重连倒计时时改走 `ABORT_RECONNECT`，符合当前 legacy 语义。

- [x] **S4：popup active predicate 与非迁移守护**
  - 证据来源：`RmlUiPopupModal.MigratablePopupScopeIsLimitedToApprovedFullscreenSubset`、`RmlUiRuntime.ActiveInputModuleRequiresExplicitHostActiveState`
  - 结果：通过。非迁移 popup 不进入 popup modal，模块 toggle 开启本身也不会被误判为“当前正在接管输入”。

- [x] **S5：fallback / safe-mode**
  - 证据来源：`RmlUiPopupModal.FailureMetadataMapsSurfaceFailuresToDocumentDiagnostics`、代码核对 `CRmlUiRuntime::ResolveHostInputDecision(...)`
  - 结果：通过。popup surface 失败会携带结构化失败原因，并按输入桥既有协议先走 release-state，再回到 legacy popup owner。

- [x] **S6：宿主 dispatch 修复**
  - 证据来源：`RmlUiPopupModal.MigratableFullscreenPopupsSkipLegacyPreDispatchSlot`
  - 结果：通过。可迁移 fullscreen popup 不再先占用 legacy slot，RmlUI popup surface 可以真正进入 `fullscreen_popup` runtime path。

- [x] **S7：构建与定向测试**
  - 证据来源：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 1`、`.\\build-ninja\\testrunner.exe --gtest_filter="RmlUiPopupModal.*:RmlUiPopupAssets.*:RmlUiLayerSwitchboard.*:RmlUiRuntime.*:RmlUiConfigCompat.*"`
  - 结果：通过。`game-client` 已重新链接 `DDNet.exe`，定向测试共 `42` 条全部通过。

- [x] **S8：最终 UI 表现口径**
  - 证据来源：项目约束 + 本 feature 人工验收清单
  - 结果：自动证据负责证明范围、输入、fallback 与宿主语义；最终游戏内 UI 终审仍由人工完成，不以截图作为默认验收产物。

## 4. 术语一致性

- [x] `popup modal module` 在代码中对应 `CRmlUiPopupModal`，没有再引入第二套 `fullscreen popup view` 命名。
- [x] `popup action token` 在代码中统一落为 `ERmlUiPopupAction`，没有把 `BUTTON_CONFIRM`、`BUTTON_CANCEL` 直接暴露给 runtime-shell。
- [x] `可迁移弹窗集合` 统一由 `CRmlUiPopupModal::IsMigratablePopup(...)` 与 `CMenus::NeedsLegacyRmlUiFullscreenPopupSlot(...)` 共同表达，没有出现设计外的第三套范围判定。

## 5. 架构归并

- [x] `.codestable/architecture/ARCHITECTURE.md` 已回写 popup modal 为当前第二条已验收的 concrete RmlUI surface，并补齐 `CRmlUiPopupModal` 与 popup 资源链现状。
- [x] `.codestable/architecture/ui-rmlui-current.md` 已回写 popup modal 的当前范围、宿主流程、失败语义与边界，明确它只覆盖 `fullscreen_popup` 下的低风险提示型弹窗子集。

## 6. requirement 回写

- [x] `.codestable/requirements/rmlui-full-replacement.md` 已把 `2026-05-08-rmlui-popup-migration` 追加到 `implemented_by`。
- [x] requirement 变更日志已补充“首个交互式 modal surface 已验收”的 current-state 说明。

## 7. roadmap 回写

- [x] `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-items.yaml` 已把 `rmlui-popup-migration` 从 `in-progress` 改为 `done`。
- [x] `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-roadmap.md` 已把第 10 项同步改为 `done`，并把主线优先级从“popup / menu-pilot”更新为“popup 已完成，下一步进入 menu-pilot”。
- [x] `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-readiness-matrix.md` 已把 `rmlui-popup-migration` 从 `ready-for-impl` 回写为 `done`，并把 `rmlui-menu-pilot` 提升到可继续设计的状态。
- [x] `.codestable/features/RMLUI_FEATURE_INDEX.md` 已把 popup migration 收口进已验收基线，并修正 input bridge 的索引漂移。

## 8. attention.md 候选盘点

- [x] 本 feature 没有新增必须追加到 `attention.md` 的新环境约束；现有“最终 UI 表现由人工验收，不用截图做默认证据”的规则已足够覆盖本轮验收。

## 9. 遗留

- 当前已验收的是 `fullscreen_popup` 下的低风险提示型弹窗子集，不包含 `popup_menu`。
- 当前已验收的是无文本输入的 popup modal；`POPUP_PASSWORD`、`POPUP_RENAME_DEMO`、`POPUP_RENDER_DEMO`、`POPUP_SAVE_SKIN` 仍走 legacy。
- 下一步主线不再是重复收口 popup，而是进入 `rmlui-menu-pilot` 的 design / impl 闭环，继续复用已验收的 popup migration、input bridge 与 safe-mode 基线。
