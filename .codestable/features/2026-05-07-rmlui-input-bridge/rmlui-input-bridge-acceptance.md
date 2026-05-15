# RmlUI 输入桥验收报告

> 阶段：阶段 3（验收闭环）
> 验收日期：2026-05-08
> 关联方案 doc：`.codestable/features/2026-05-07-rmlui-input-bridge/rmlui-input-bridge-design.md`

## 1. 接口契约核对

- `SRmlUiInputResult`、`SRmlUiActiveInputModule`、`SRmlUiHostInputDecision` 已落地，接口示例与实际代码一致。
- `CRmlUiInputBridge` 已实现鼠标、按键、滚轮、文本、cancel、release-state 路由。
- `CRmlUiRuntime::ActiveInputModule()` 现在不仅看 toggle 和 layer，还要求宿主 active predicate 为真，避免把“模块已启用”误判成“当前正在接管输入”。
- `CGameClient::OnUpdate()` 已按 `CRmlUiRuntime::ActiveInputModule()` + `ResolveHostInputDecision()` 执行宿主决策，并使用 `legacy fallback owner` 恢复到对应旧宿主起点，而不是无差别退回整条旧输入链。
- 流程图中的输入宿主、桥接、runtime 决策与 release-state 路径都能在代码中 grep 到落点。

## 2. 行为与决策核对

- `m_RequiresInput=false` 的模块不会进入输入桥，HUD 默认不抢 gameplay 输入。
- `cancel/back` 统一进入 cancel 协议，并由宿主执行 release-state。
- 纯 `FLAG_RELEASE` 事件即使被 RmlUI 路由过，仍继续遵守旧 `m_vpInput` 的 release 广播纪律，不会把旧组件的释放态截断。
- 旧 GUI 生命周期 owner 没有被 runtime-shell 改写，菜单 / 轮盘 / editor 仍由原宿主负责关闭与回退。
- RmlUI 先持有平台文本输入后，如果 console / legacy `CLineInput` 再激活，会主动让出 ownership，不与旧文本输入 owner 共管 IME 生命周期。
- 栖梦设置页只保留全局 `qm_rmlui_enable`，Monitoring 专属图形开关已移除。
- 交互式 surface 的 fallback 仍回到旧宿主，不存在 render bridge 或菜单迁移的越权实现。

## 3. 验收场景核对

- `RmlUiRuntime.ActiveInputModuleRequiresGlobalAndModuleToggle`：通过。
- `RmlUiRuntime.ActiveInputModuleRequiresExplicitHostActiveState`：通过。
- `RmlUiRuntime.ActiveInputModulePrioritizesMenuModalOverMenuPage`：通过。
- `RmlUiRuntime.ResolveHostInputDecisionConsumesConsumedRoutes`：通过。
- `RmlUiRuntime.ResolveHostInputDecisionFallsBackCancelToLegacyOwner`：通过。
- `RmlUiInputBridge.InteractiveModuleCanConsumePointerAndKeyboardInput`：通过。
- `RmlUiInputBridge.CancelActionRequestsCloseAndReleaseStateForActiveModule`：通过。
- `RmlUiInputBridge.ReleaseStateClearsTrackedTextInputContext`：通过。
- `RmlUiInputBridge.TextInputHandlerYieldsPlatformInputWhenLegacyOwnerActivatesAfterAcquire`：通过。
- `RmlUiInputBridge.TextInputHandlerYieldsPlatformInputWhenConsoleActivatesAfterAcquire`：通过。
- `qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 1`：通过。
- `qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 1`：通过，`762` 个测试全绿。

## 4. 术语一致性

- `输入桥`、`release-state`、`cancel action`、`当前激活输入模块`、`宿主输入决策` 的命名与设计一致。
- 未引入设计外的新概念。

## 5. 架构归并

- 已更新 `.codestable/architecture/ui-rmlui-current.md`，把 `CRmlUiInputBridge` 的 release 广播纪律、文本输入 ownership 让渡和 runtime active predicate 写入当前架构。
- 已更新 `.codestable/architecture/ARCHITECTURE.md`，把 input bridge 的宿主 active 判定和 owner 定向 fallback 约束归并到总入口。

## 6. requirement 回写

- 已将 `.codestable/requirements/rmlui-full-replacement.md` 状态改为 `current`。
- 已把 `2026-05-07-rmlui-input-bridge` 加入 `implemented_by`。
- 已补充变更日志，记录输入桥与设置页开关收口已验收。

## 7. roadmap 回写

- 已将 `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-items.yaml` 中 `rmlui-input-bridge` 改为 `done`。
- 已同步 `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-roadmap.md` 的子 feature 清单与进度描述。
- 已同步 `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-readiness-matrix.md` 的就绪状态。

## 8. attention.md 候选

- 无候选。

## 9. 遗留

- 后续主线进入 `rmlui-popup-migration` / `rmlui-menu-pilot`。
