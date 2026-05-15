---
doc_type: feature-acceptance
feature: 2026-05-07-rmlui-render-command-bridge
status: pass
created: 2026-05-08
tags: [rmlui, render-bridge, acceptance, graphics-thread]
related_design: rmlui-render-command-bridge-design.md
related_checklist: rmlui-render-command-bridge-checklist.yaml
---

# rmlui-render-command-bridge 验收报告

> 阶段：阶段 3（验收闭环）
> 验收日期：2026-05-08
> 关联方案 doc：`features/2026-05-07-rmlui-render-command-bridge/rmlui-render-command-bridge-design.md`

## 1. 接口契约核对

- [x] `IGraphics::RunOnBackendThread(FBackendThreadCallback, void *)` 已落地到 `src/engine/graphics.h`，并由 `CGraphics_Threaded` 实现同步 backend-thread callback。
- [x] `CGameClient::RenderRmlUiMonitoringModule(...)` 已改为构造 `SRmlUiMonitoringBackendFrame`，通过 `Graphics()->RunOnBackendThread(...)` 执行 Monitoring HUD 的 RmlUI document render。
- [x] `CRmlUiMonitoringHud` 已拆分为 `RenderDocument(...)` 与 `DrawGraphs(...)` 两段职责，分别承接 graphics-thread document render 和主线程 overlay 绘制。
- [x] design 第 2.2 节主流程图对应的调用链已落到 `RenderQmMonitoringHud` → `RenderQmMonitoringHudRmlUi` → `CRmlUiRuntime::RenderRmlUiLayer(...)` → `RenderRmlUiMonitoringModule(...)` → `Graphics()->RunOnBackendThread(...)` → `RunRmlUiMonitoringBackendFrame(...)` → `CRmlUiMonitoringHud::RenderDocument(...)` / `DrawGraphs(...)`。

## 2. 行为与决策核对

- [x] Monitoring HUD surface 路径不再调用 `AcquireBackendFrameContext()` / `ReleaseBackendFrameContext()`；当前宿主只剩 backend-thread callback 路径。
- [x] backend-thread callback 仍是同步最小桥接入口，没有扩展成通用业务执行器；RmlUI 相关用法当前只承接 Monitoring HUD frame 与 shutdown。
- [x] fallback owner 保持不变：`CRmlUiRuntime` 只返回结果，旧 HUD 仍由 `CGameClient::RenderQmMonitoringHud` 执行。
- [x] graph overlay 仍走现有 `IGraphics` 绘制；本 feature 没有把 geometry/scissor/texture full bridge 伪装成已完成。
- [x] 反向核对“明确不做”项：未迁移菜单/弹窗/设置页/轮盘/编辑器，未删除 `CRmlUiBackend` GL3 prototype，未删除旧 HUD fallback。

## 3. 验收场景核对

- [x] 宿主路径核对：`git grep` 结果显示 `src/game/client/gameclient.cpp` 的 Monitoring HUD 宿主只命中 `RunOnBackendThread(...)`，不再命中 `AcquireBackendFrameContext` / `ReleaseBackendFrameContext`。
- [x] 构建验证：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 1` 重新链接通过。
- [x] RmlUI 回归验证：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target testrunner -j 1` 后，`.\\build-ninja\\testrunner.exe --gtest_filter='*RmlUi*:*rmlui*'` 运行 21 个 RmlUI 相关测试全部通过。
- [x] 运行日志验证：2026-05-08 13:33:03 的实际启动日志出现 `rmlui: backend init success`、`rmlui: core init success`、`rmlui_hud: document load success`，且同一批日志未再出现 Monitoring HUD surface 直接触发的 `wglMakeCurrent(): 请求的资源在使用中`。
- [x] fallback 语义核对：`RenderQmMonitoringHud(...)` 在 `FALLBACK_REQUIRED` 时仍先绘制旧版 Monitoring HUD，再叠加 fallback notice，没有跳过 legacy HUD。

## 4. 术语一致性

- `backend-thread callback`：代码命名为 `FBackendThreadCallback` / `RunOnBackendThread(...)`，与 design 保持一致。
- `render bridge minimal slice`：当前实现只覆盖 graphics-thread document render + legacy overlay split，没有冒充 full bridge。
- `document render step`：代码命名为 `RenderDocument(...)`，与 design 保持一致。
- `graph overlay step`：代码命名为 `DrawGraphs(...)`，与 design 保持一致。

## 5. 架构归并

- [x] `architecture/ARCHITECTURE.md` 已补入“Monitoring HUD 当前通过 backend-thread callback 执行 RmlUI document render，overlay 仍走 legacy graphics path”的现状。
- [x] `architecture/ui-rmlui-current.md` 已补入 Monitoring HUD current flow、`CRmlUiMonitoringHud` 当前职责拆分，以及“最小 bridge 已落地但 full backend-neutral bridge 仍未完成”的边界。
- [x] 未回写 future-only 内容：scissor/texture/geometry full bridge、layer switchboard、input bridge、菜单/弹窗迁移仍保持非 current-state。

## 6. requirement 回写

- [x] requirement `rmlui-full-replacement` 仍保持 draft 愿景层，本次不升级 requirement 状态。
- [x] 本 feature 完成的是 render bridge 的最小切片与运行时争用收敛，不等于 `rmlui-full-replacement` 整体能力已 current。

## 7. roadmap 回写

- [x] `roadmap=rmlui-full-replacement`，`roadmap_item=rmlui-render-command-bridge` 已确认。
- [x] `rmlui-full-replacement-items.yaml` 已把 `rmlui-render-command-bridge` 从 `in-progress` 改为 `done`，并保留“仅完成 minimal slice”的 notes。
- [x] `rmlui-full-replacement-roadmap.md`、`rmlui-full-replacement-readiness-matrix.md` 和 `features/RMLUI_FEATURE_INDEX.md` 已同步 accepted minimal bridge baseline 的真实状态。

## 8. attention.md 候选盘点

- 本 feature 暴露出一条现有 attention 候选：同一个 `build-ninja` 目录不能并行开两条构建，否则会遇到 `git_revision.cpp` 文件锁冲突；该约束已经存在于仓库 `AGENTS.md`，本次不重复写入 `attention.md`。

## 9. 遗留

- `Failed to load font face from fonts/Icons.ttf` 仍出现在运行日志中；这是资源链问题，不属于本 feature 的 render-command bridge 闭环。
- 当前 bridge 仍是 Monitoring HUD 专用的最小切片；scissor、texture、blend、geometry lifecycle 和 layer switchboard 仍待后续 feature。
- 本次运行期证据来自 2026-05-08 的实际启动日志与本地重建/测试；没有在本 turn 内补录新的同屏截图或录屏。

## 最终结论

`rmlui-render-command-bridge` 已完成当前 design 声明的最小闭环：Monitoring HUD document render 已迁到 graphics thread callback，宿主不再通过 `AcquireBackendFrameContext()` 争抢 context，旧 HUD fallback 仍保留，构建与 RmlUI 相关测试均通过，且实际启动日志已显示 backend/core/document 路径成功跑通。当前可以把这个 roadmap item 视为 accepted minimal bridge baseline，但不能据此宣称 full backend-neutral render bridge 已完成。
