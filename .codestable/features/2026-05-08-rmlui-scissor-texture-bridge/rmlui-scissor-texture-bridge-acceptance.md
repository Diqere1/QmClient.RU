---
doc_type: feature-acceptance
feature: 2026-05-08-rmlui-scissor-texture-bridge
status: pass
created: 2026-05-08
tags: [rmlui, texture, scissor, acceptance, bridge]
related_design: rmlui-scissor-texture-bridge-design.md
related_checklist: rmlui-scissor-texture-bridge-checklist.yaml
---

# rmlui-scissor-texture-bridge 验收报告

> 阶段：阶段 3（验收闭环）
> 验收日期：2026-05-08
> 关联方案 doc：`features/2026-05-08-rmlui-scissor-texture-bridge/rmlui-scissor-texture-bridge-design.md`

## 1. 接口契约核对

- [x] `CRmlUiRenderBridge` 已落地到 `src/engine/client/rmlui_render_bridge.h/.cpp`，并由 `CRmlUiBackend` 持有，不再把 raw `RenderInterface_GL3` 作为唯一 public render surface 暴露。
- [x] `IRmlUiRenderBridgeDelegate` 已作为 bridge 内部接缝落地，desktop GL 当前实现由 `src/engine/client/rmlui_backend.cpp` 内部的 `CRmlUiGl3RenderBridgeDelegate` 承接。
- [x] texture registry 契约已落地到 `SRmlUiBridgeTextureRecord`，字段包含 bridge handle、delegate handle、来源类型、尺寸和释放状态。
- [x] scissor state 契约已落地到 `SRmlUiBridgeScissorState`，字段包含 enable 状态、最近一次 Rml region 和翻译后的 clip rectangle。
- [x] design 第 2.2 节主流程图对应的调用链已落到 runtime host render request → backend-thread document render → `CRmlUiRenderBridge` → texture lifecycle/scissor translation/current desktop GL delegate。

## 2. 行为与决策核对

- [x] `LoadTexture(...)`、`GenerateTexture(...)`、`ReleaseTexture(...)` 现在都先经过 bridge registry，再落到 current delegate。
- [x] generated texture 的 premultiplied alpha 兼容规则已提升为 bridge 显式契约，入口位于 `NormalizeGeneratedTextureData(...)`，并由 delegate 的 `NeedsGeneratedTextureUnpremultiply()` 决定是否启用。
- [x] `SaveLayerAsTexture()` 现在会把 delegate raw handle 重写成 bridge handle，再返回给上层；不会再把 raw delegate handle 泄漏回 bridge namespace。
- [x] `RenderGeometry(...)` 与 `RenderShader(...)` 都会先把 bridge texture handle 翻译回当前 delegate handle，再做当前桌面路径提交。
- [x] `EnableScissorRegion(...)` / `SetScissorRegion(...)` 的 single-owner 语义已落在 bridge 层；translated clip state 只在 `UpdateScissorState(...)` 做一次。
- [x] 反向核对“明确不做”项：本 feature 没有宣称 compiled geometry/full draw submission bridge 已完成，没有宣称 layer/filter/shader/save-layer/clip-mask full bridge 已完成，没有迁移 Monitoring HUD/菜单/弹窗/轮盘/editor surface，也没有宣称 Vulkan/Android 已被当前 slice 覆盖。

## 3. 验收场景核对

- [x] targeted tests：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target testrunner -j 1` 后，`build-ninja\testrunner.exe --gtest_filter=RmlUiRenderBridge.*` 运行 5 个 bridge 专项测试全部通过。
- [x] file texture 场景：`LoadTextureRegistersBridgeHandleAndTranslatesOnRender` 证明 file texture 会建立 bridge handle，并在 geometry render 时翻译回 delegate handle。
- [x] generated texture 场景：`GenerateTextureAppliesConfiguredUnpremultiplyContract` 证明 generated texture 会按 bridge 契约执行 alpha normalization。
- [x] release / shutdown 场景：`ManualReleaseAndShutdownCleanupReleaseDelegateHandlesOnce` 证明 manual release 与 shutdown cleanup 都会清理 delegate texture handle，且只释放一次。
- [x] save-layer texture 场景：`SaveLayerTextureRegistersBridgeHandleForRenderAndRelease` 证明 save-layer texture 会进入 bridge namespace，并能被 render/release 正确消费。
- [x] scissor 场景：`ScissorStateTracksTranslatedClipSemantics` 证明 bridge 自己持有 translated clip state，且不会产生双重 Y 翻转。
- [x] 构建验证：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 1` 重新链接通过。
- [x] 运行证据：`build-ninja/ddnet-rmlui-step4.log` 与 `build-ninja/ddnet-rmlui-scissor-texture-accept.stdout.log` 都显示 desktop GL 当前路径可正常启动，包含 `I gfx: Created OpenGL 1.1 context`；本线程 2026-05-08 13:33:03 的实际运行日志还出现过 `rmlui: backend init success`、`rmlui: core init success context=...` 和 `rmlui_hud: document load success`，说明这次 texture/scissor slice 没把已跑通的 Monitoring HUD RmlUI 路径打回去。

## 4. 术语一致性

- `bridge texture registry`：代码命名为 `SRmlUiBridgeTextureRecord` / `m_TextureRegistry`，与 design 保持一致。
- `bridge scissor state`：代码命名为 `SRmlUiBridgeScissorState` / `m_ScissorState`，与 design 保持一致。
- `bridge delegate`：代码命名为 `IRmlUiRenderBridgeDelegate`，desktop GL 当前实现命名为 `CRmlUiGl3RenderBridgeDelegate`，与 design 保持一致。
- `generated texture path`：代码命名为 `GenerateTexture(...)` / `NormalizeGeneratedTextureData(...)`，与 design 保持一致。
- `geometry hold-line`：代码仍把 compiled geometry / layer / filter / shader 大面上的执行留给 current delegate，没有把术语越界写成 full bridge done。

## 5. 架构归并

- [x] `architecture/ui-rmlui-current.md` 已补入 `CRmlUiRenderBridge` current-state，明确 texture registry 与 scissor translation 已有 single owner，且 bridge 仍只覆盖 Monitoring HUD prototype path。
- [x] `architecture/ARCHITECTURE.md` 已补入 `RmlUI Render Bridge` 术语，以及 backend 现已通过 bridge wrapper 持有 texture/scissor contract 的现状。
- [x] 未回写 future-only 内容：layer switchboard、input bridge、菜单/弹窗/轮盘迁移、Vulkan/Android backend implementation 仍保持非 current-state。

## 6. requirement 回写

- [x] requirement `rmlui-full-replacement` 仍保持 `draft` 愿景层，不升级为 `current`。
- [x] `requirements/rmlui-full-replacement.md` 已补 `implemented_by` 和变更日志，把当前已验收的 RmlUI 基线切片回写为“部分已落地”，同时保留“整条替代能力尚未 current”的边界。

## 7. roadmap 回写

- [x] `roadmap=rmlui-full-replacement`，`roadmap_item=rmlui-scissor-texture-bridge` 已确认。
- [x] `rmlui-full-replacement-items.yaml` 已把 `rmlui-scissor-texture-bridge` 从 `in-progress` 改为 `done`，并把描述/acceptance/notes 收紧到真实的 texture/scissor slice。
- [x] `rmlui-full-replacement-roadmap.md`、`rmlui-full-replacement-readiness-matrix.md` 和 `features/RMLUI_FEATURE_INDEX.md` 已同步 accepted texture/scissor bridge baseline 的真实状态。

## 8. attention.md 候选盘点

- 本 feature 未暴露新的 attention.md 候选。现有 `qm_` 前缀、TDD 约束以及 Windows 构建命令约束已覆盖本次实现。

## 9. 遗留

- `CRmlUiRenderBridge` 目前只收紧了 texture/scissor ownership；compiled geometry/full draw submission、layer/filter/shader 的完整 backend-neutral bridge 仍待后续 feature。
- 当前运行证据只证明 desktop GL 启动路径未回归，以及本线程已有一次 RmlUI backend/core/document 成功日志；没有在本 turn 内补新的游戏内 Monitoring HUD 截图或录屏。
- `fonts/Icons.ttf` 缺失这一类资源问题不属于本 feature 的 texture/scissor contract 闭环，继续留给资源链或迁移阶段处理。

## 最终结论

`rmlui-scissor-texture-bridge` 已完成当前 design 声明的 texture/scissor bridge 收口：backend 现在通过 `CRmlUiRenderBridge` 持有 texture registry 与 scissor translation single-owner，save-layer texture handle 不再泄漏 raw delegate handle，geometry/shader 提交都会先做 bridge handle 翻译，targeted tests 与重建均通过，desktop GL 当前启动路径也没有回归。当前可以把这个 roadmap item 视为 accepted texture/scissor bridge baseline，但不能据此宣称 full backend-neutral render bridge、layer switchboard 或交互式 surface 迁移已经完成。
