---
doc_type: feature-acceptance
feature: 2026-05-07-rmlui-runtime-shell
status: pass
created: 2026-05-07
tags: [rmlui, runtime, acceptance, diagnostics]
related_design: rmlui-runtime-shell-design.md
related_checklist: rmlui-runtime-shell-checklist.yaml
---

# rmlui-runtime-shell 验收报告

> 阶段：阶段 3（验收闭环）
> 验收日期：2026-05-07
> 关联方案 doc：`features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-design.md`

## 1. 接口契约核对

- [x] `SRmlUiModuleDescriptor` 已落地到 `src/game/client/RmlUi/RmlUiRuntime.h`，字段包含 module name、toggle、layer、document、input requirement、fallback owner。
- [x] `SRmlUiFrameRequest` / `SRmlUiFrameResponse` 已落地到 `src/game/client/RmlUi/RmlUiRuntime.h`，并由 `CRmlUiRuntime::RenderRmlUiLayer(...)` 统一消费。
- [x] `SRmlUiDiagnostics` 已落地到 `src/game/client/RmlUi/RmlUiRuntime.h`，并由 `PopulateRmlUiRuntimeDiagnostics(...)` 与 `ExportRmlUiMonitoringDiagnostics(...)` 填充和导出。
- [x] 流程图中的宿主入口 → frame request → runtime 判定 → fallback 链已落到 `CGameClient::RenderQmMonitoringHud`、`RenderQmMonitoringHudRmlUi`、`CRmlUiRuntime::RenderRmlUiLayer` 和 `RenderRmlUiMonitoringModule`。

## 2. 行为与决策核对

- [x] runtime 只负责判定和报告，不直接绘制旧 fallback；旧 HUD 仍由 `CGameClient::RenderQmMonitoringHud` 继续执行。
- [x] `monitoring_hud` 已作为 `GAME_HUD` prototype host 注册到 runtime，注册入口为 `EnsureRmlUiMonitoringRuntimeRegistered()`。
- [x] 失败语义已区分 `SKIPPED_DISABLED`、`SKIPPED_UNAVAILABLE` 和 `FALLBACK_REQUIRED`，对应实现位于 `CRmlUiRuntime::RenderRmlUiLayer(...)`。
- [x] `qm_` 前缀配置已落地：`qm_rmlui_enable`、`qm_rmlui_debug_diagnostics`、`qm_rmlui_monitoring_hud`。
- [x] `qm_monitoring_use_rmlui` 旧实验开关仍保留，但通过 `RmlUiConfigCompat` 和 console chain 同步到新 runtime/module 开关。
- [x] 反向核对“明确不做”项：本 feature 没有新增 render bridge、input bridge、safe-mode automation、菜单/弹窗/设置页/轮盘/HUD editor 迁移，也没有新增 `SDL_GL_MakeCurrent` 或 `SDL_GL_GetCurrentContext` 使用点。

## 3. 验收场景核对

- [x] 全局关闭路径：`rmlui_runtime_test.cpp` 覆盖 `GlobalDisabledSkipsWithoutInit`，验证 `qm_rmlui_enable=0` 时返回 `SKIPPED_DISABLED`，且不初始化 core。
- [x] 模块关闭路径：`rmlui_runtime_test.cpp` 覆盖 `ModuleDisabledSkipsWhenGlobalEnabled`，验证模块关闭时返回 `SKIPPED_DISABLED`。
- [x] runtime 不可用路径：`rmlui_runtime_test.cpp` 覆盖 `RuntimeUnavailableRequiresFallback`，验证返回 `FALLBACK_REQUIRED` 并记录 diagnostics。
- [x] 模块内部失败路径：`rmlui_runtime_test.cpp` 覆盖 `SurfaceFailureFromModuleCallbackRequiresFallback`，验证 surface/document failure 仍回旧 HUD fallback。
- [x] 重复注册路径：`rmlui_runtime_test.cpp` 覆盖 `DuplicateRegistrationIsRejected`，验证不会产生两个有效模块。
- [x] diagnostics gate/dedupe：`rmlui_runtime_test.cpp` 覆盖 `DiagnosticsExportHonorsDebugGateAndDeduplicates`。
- [x] 证据命令：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10`
- [x] 测试结果：716 个 C++ 测试通过。

## 4. 术语一致性

- `RmlUI runtime`：代码命名为 `CRmlUiRuntime`，与 design 保持一致。
- `module descriptor`：代码命名为 `SRmlUiModuleDescriptor`，与 design 保持一致。
- `frame request`：代码命名为 `SRmlUiFrameRequest`，与 design 保持一致。
- `diagnostics`：代码命名为 `SRmlUiDiagnostics`，与 design 保持一致。
- `fallback owner`：代码字段命名为 `m_pFallbackOwner`，与 design 保持一致。

## 5. 架构归并

- [x] 需要回写：runtime shell 责任边界、module descriptor 稳定字段、frame result 语义、diagnostics 所有权与导出入口。
- [x] 不回写：render command bridge、input bridge、safe-mode automation、Vulkan/Android backend implementation。
- [x] 回写目标：`architecture/ARCHITECTURE.md` 与 `architecture/ui-rmlui-current.md`。

## 6. requirement 回写

- [x] requirement `rmlui-full-replacement` 仍保持 draft 愿景层，不升级状态。
- [x] 本次实现已满足“全局开关 + 模块开关 + fallback + diagnostics baseline”这条最小闭环，因此只做 requirement 关联链保持，不额外改用户故事边界。

## 7. roadmap 回写

- [x] `roadmap=rmlui-full-replacement`，`roadmap_item=rmlui-runtime-shell` 已确认。
- [x] `items.yaml` 对应条目需要从 `in-progress` 改为 `done`。
- [x] roadmap 主文档对应子 feature 清单需要从“planned / 未启动”改为“done / 2026-05-07-rmlui-runtime-shell”。

## 8. attention.md 候选盘点

- 本 feature 未暴露新的 attention.md 候选。现有 `qm_` 前缀与默认 TDD 约束已覆盖本次经验。

## 9. 遗留

- render-command-bridge 仍是 draft 设计，未实现。
- diagnostics 导出仍带 Monitoring HUD surface 特例字段（`hud_initialized` / `hud_document` / `hud_failure`），后续 `rmlui-resource-diagnostics` 需要决定如何推广为更通用的 resource/surface diagnostics 合同。
- acceptance 证据当前以单测和代码核对为主，尚缺真实游戏内截图/录屏证据。

## 最终结论

`rmlui-runtime-shell` 已完成第一条工程闭环：runtime/module registry、全局与模块开关、fallback result、diagnostics gate/dedupe、兼容开关同步和单测基线均已落地，且 716 个 C++ 测试通过。当前可以把 roadmap item `rmlui-runtime-shell` 标记为 `done`，同时把 runtime shell 相关现状回写到 architecture。超出范围的 render bridge、safe-mode automation、interactive surfaces 仍保持未实现状态。
