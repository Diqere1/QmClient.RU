---
doc_type: feature-acceptance
feature: 2026-05-07-rmlui-safe-mode
status: pass
created: 2026-05-07
tags: [rmlui, safe-mode, acceptance, diagnostics]
related_design: rmlui-safe-mode-design.md
related_checklist: rmlui-safe-mode-checklist.yaml
---

# rmlui-safe-mode 验收报告

> 阶段：阶段 3（验收闭环）
> 验收日期：2026-05-07
> 关联方案 doc：`features/2026-05-07-rmlui-safe-mode/rmlui-safe-mode-design.md`

## 1. 接口契约核对

- [x] `SRmlUiDiagnostics` 已扩展 safe-mode 字段，覆盖 `m_SafeModeFailureCount`、`m_SafeModeThreshold`、`m_SafeModeSessionDemoted`、`m_pSafeModeDecision`、`m_pSafeModeTriggerReason` 和 `m_pSafeModeResetReason`。
- [x] `CRmlUiRuntime` 已扩展 `SRmlUiSafeModeState`，用于持有 per-module failure streak、session demotion、trigger reason 和 reset reason。
- [x] `qm_rmlui_safe_mode` 已落地为 QmClient 配置项，并由 runtime toggle callback 实际消费。
- [x] safe-mode 结果契约已落地：trip frame 返回 `FALLBACK_REQUIRED reason=safe_mode_trip`，post-trip frame 返回 `SKIPPED_UNAVAILABLE reason=safe_mode_session_disabled`。

## 2. 行为与决策核对

- [x] safe-mode 只统计 enabled module 的连续 `FALLBACK_REQUIRED`，`SKIPPED_DISABLED` 不计入 streak。
- [x] safe-mode 是 runtime policy，不替代 legacy fallback；旧 HUD fallback 仍由宿主执行。
- [x] trip threshold 固定为 `3`，并由 `CRmlUiRuntime` 当前实现内的 `SAFE_MODE_THRESHOLD` 持有。
- [x] `render_success`、module toggle cycle、`qm_rmlui_safe_mode` toggle cycle 三类 reset 都已在 runtime 内实现。
- [x] safe-mode 状态仍留在 `CRmlUiRuntime`，没有下沉到 backend、surface 或 `CGameClient` 状态表。

## 3. 验收场景核对

- [x] `src/test/rmlui_runtime_test.cpp` 覆盖 `SafeModeTripsOnThirdConsecutiveFallbackAndDemotesLaterFrames`。
- [x] `src/test/rmlui_runtime_test.cpp` 覆盖 `SafeModeSuccessRenderResetsFailureStreak`。
- [x] `src/test/rmlui_runtime_test.cpp` 覆盖 `SafeModeCountsEnsureCoreFallbackFailures`。
- [x] `src/test/rmlui_runtime_test.cpp` 覆盖 `SafeModeModuleToggleCycleClearsDemotionState`。
- [x] `src/test/rmlui_runtime_test.cpp` 覆盖 `SafeModeGlobalToggleCycleClearsDemotionState`。
- [x] 证据命令：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 1`
- [x] 测试结果：725 个 C++ 测试通过。

## 4. 术语一致性

- `safe mode`：代码与设计统一落到 `qm_rmlui_safe_mode`、`SRmlUiSafeModeState`、`m_SafeMode*` diagnostics 字段。
- `trip frame`：代码与测试统一使用 `safe_mode_trip`。
- `session demoted`：代码与测试统一使用 `safe_mode_session_disabled` 作为 post-trip host-visible reason。
- `reset reason`：代码与测试统一使用 `render_success`、`module_toggle_cycle`、`safe_mode_toggle_cycle`。

## 5. 架构归并

- [x] `architecture/ARCHITECTURE.md` 已补入 “safe-mode policy 已进入 runtime” 的 current-state 口径。
- [x] `architecture/ui-rmlui-current.md` 已补入 `CRmlUiRuntime` 当前拥有 safe-mode streak / demotion / reset policy 的事实。
- [x] `reference/rmlui-gameclient-host-reference.md` 已回写 “host 仍拥有 legacy fallback，但 safe-mode state 已在 runtime”。
- [x] 未回写 future-only 内容：input bridge implementation details、render bridge behavior 仍保持非 current-state。

## 6. requirement 回写

- [x] requirement `rmlui-full-replacement` 仍保持 roadmap/draft 能力愿景层，本次不单独升级 requirement 状态。
- [x] 本次实现属于既有 roadmap item 的安全护栏闭环，不新增独立 requirement 文档。

## 7. roadmap 回写

- [x] `roadmap=rmlui-full-replacement`，`roadmap_item=rmlui-safe-mode` 已确认。
- [x] `rmlui-full-replacement-readiness-matrix.md` 已同步为 `done`，不再错误声称 safe-mode 仍停在 design 前。
- [x] `items.yaml` 和 `RMLUI_FEATURE_INDEX.md` 已同步 safe-mode 的 accepted 状态。

## 8. attention.md 候选盘点

- 本 feature 未暴露新的 attention.md 候选。现有 `qm_` 前缀和默认 TDD 约束已覆盖本次经验。

## 9. 遗留

- resource/export ownership 仍带 Monitoring HUD host residue；这不是 safe-mode feature 本身的问题，但会影响后续统一 diagnostics contract。
- 目前 acceptance 证据仍以单测和代码核对为主，缺少真实游戏内强制 trip 的录屏或截图证据。

## 最终结论

`rmlui-safe-mode` 已完成最小闭环：runtime 拥有 per-module failure streak、trip/demotion、reset 路径和 safe-mode diagnostics 字段；trip frame 与 post-trip frame 的 host-visible reason 语义已固定；`ensure_core` 路径也被纳入 streak 统计；相关 C++ 测试已覆盖并通过。当前可以把 safe-mode 视为 downstream baseline 的一部分，并继续做 feature/roadmap/backfill 状态同步。
