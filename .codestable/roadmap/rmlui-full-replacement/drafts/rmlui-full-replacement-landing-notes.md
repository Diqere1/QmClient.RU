---
doc_type: roadmap_draft
slug: rmlui-full-replacement-landing-notes
status: working
created: 2026-05-07
updated: 2026-05-07
tags: [rmlui, roadmap, landing, integration]
related_roadmap: [rmlui-full-replacement]
---

# RmlUI 落地说明草案

这份草案不是新的 roadmap，而是给后续 `cs-feat-design` / `cs-feat-accept` 用的实施伴随文档。主 roadmap 负责定方向和契约，这里负责回答“具体先从哪里切进去、哪些旧逻辑暂时不要碰、验收时该拿什么证据回来”。

Feature-specific implementation guides should treat this document as the upstream slicing reference, not as a substitute for checklist or acceptance.

## 1. 实施切片原则

1. 先替换宿主壳层，再替换内部业务内容。
2. 先做可回退的接入点，再做更完整的视觉替代。
3. 先共用旧逻辑执行路径，再考虑把逻辑也迁到 RmlUI 模型。
4. 先保证 OpenGL / Vulkan / Android 上层协议一致，再讨论某个后端的表现细节。
5. 每个切片都必须有“失败后回旧 UI”的明确证据。

## 2. 宿主切入顺序

### 2.1 Monitoring HUD

- 首切入口：`CGameClient::RenderQmMonitoringHud`
- 原因：已经存在 RmlUI 特例接缝，最适合从“旁路实验”收回到正式 layer/runtime 调度。
- 现状提醒：这条 Monitoring HUD 接入目前仍是试点实现，日志里出现过可渲染不代表它已经稳定可复用，后续 design 不能默认它的内部绘制链已经成熟。
- 第一刀要做的事：
  - 保留函数级入口不动。
  - 把里面的 RmlUI 分支改成统一 module descriptor + frame request 调度。
  - 保留旧 HUD fallback，不在这一步改图表业务逻辑。
- 这一刀不要做的事：
  - 不直接在这个 feature 里重做图表算法。
  - 不把 diagnostics、resource pipeline、render bridge 的职责继续塞回 `gameclient.cpp`。

### 2.2 菜单试点页

- 首切入口：`CMenus` 页面分发，再到单页内容。
- 原因：菜单是高风险交互 surface，必须先有一个最小页级试点，而不是全菜单同时迁移。
- 推荐策略：
  - 先用 RmlUI 承担页面容器、标题、返回、空态/错误态。
  - 页内具体功能短期继续复用旧逻辑或旧绘制结果承载。
  - 把 page identity 继续绑定现有 page index / menu state，不新发明第二套导航状态树。

### 2.3 设置页

- 首切入口：`CMenus::RenderSettings(MainView)`。
- 原因：它已经是旧设置系统的天然总入口，最适合承接导航重组与搜索壳。
- 推荐策略：
  - 先建立分类树、搜索索引、分组模型。
  - 再把 `RenderSettingsGeneral` / `RenderSettingsGraphics` / `RenderSettingsQmClient` 等页面作为内容岛逐步迁入。
  - 配置项写入仍走现有 `g_Config` / 旧设置逻辑，不在第一阶段抽象新状态容器。

### 2.4 轮盘与 Click GUI

- 首切入口：现有 `CBindWheel`、`CPieMenu` 的状态机，而不是先写新 UI。
- 原因：真正难的是交互生命周期，不是画扇区。
- 推荐策略：
  - 先定义统一 action model：打开、取消、选择、确认、执行。
  - 再定义统一 pointer / keyboard / cancel 协议。
  - 表现层最后再替换成 RmlUI。

### 2.5 HUD 编辑器

- 首切入口：`CHudEditor`
- 原因：现有 editor 已经有拖拽、缩放、布局持久化，不值得推倒重来。
- 推荐策略：
  - 先增强旧 editor 的吸附线、尺寸调整、可视反馈。
  - 第一阶段把布局数据、吸附线、尺寸调整、调试可视反馈全部落在 `CHudEditor` 现有宿主中。
  - 如果要引入 RmlUI editor 表现层，必须单开 feature-design，明确 document 生命周期、`EDITOR_OVERLAY` 输入独占、旧 editor fallback、以及与 `rmlui-monitoring-hud-migration` 的数据同步方案。

## 3. Render Bridge 落地分层

### 3.1 上层禁止项

- 不允许再用 `SDL_GL_GetCurrentContext()` 作为 runtime 可用性的前提。
- 不允许在 RmlUI 模块内部直接 `MakeCurrent`、`BindFramebuffer`、`UseProgram`。
- 不允许把 Vulkan/Android 支持推迟成“以后另外做一套”。

### 3.2 中层桥接目标

目标不是立即造一套全新的图形子系统，而是把 RmlUI geometry 翻译到当前 `graphics_threaded` 能接受的抽象上：

- viewport
- clip/scissor
- blend mode
- texture handle lifecycle
- batched geometry submission

优先顺序：

1. 先证明 geometry 可以进入现有 threaded graphics 提交链。
2. 再补齐 texture/scissor/blend 的语义对齐。
3. 最后再处理后端特有优化。

### 3.3 后端适配边界

- OpenGL：允许作为第一个打通的底层实现，但不能让上层协议带上 GL 假设。
- Vulkan：必须共用同一套 frame request / module registry / diagnostics 协议。
- Android：必须假设窗口、drawable size、back button、asset path 都有平台差异，但这些差异只能收敛在 backend/runtime 外层，不得扩散到每个 feature。

## 4. 输入与焦点仲裁落地规则

### 4.1 统一输入路径

所有 RmlUI surface 的输入都要经过同一条桥，而不是谁需要谁自己接：

- cursor move
- button/key press
- button/key release
- text input
- focus gain/loss
- cancel/back
- release-state

### 4.2 surface 级行为约束

- `GAME_HUD`：默认只读，不抢 gameplay 输入。
- `DEBUG_OVERLAY`：默认只读，不抢 gameplay 输入。
- `MENU_PAGE` / `MENU_MODAL`：可以消费鼠标和键盘，但必须允许统一 cancel 返回旧路径。
- `RADIAL_OVERLAY`：必须支持 press-hold-release 生命周期，异常时直接 cancel + release-state。
- `EDITOR_OVERLAY`：激活时独占输入焦点。

### 4.3 不要在 feature 里各写各的行为

以下行为必须在输入桥或 layer manager 一层统一，不允许菜单/轮盘/editor 各自做一套：

- Escape / Android back
- 关闭时释放 hover / pressed 状态
- 控制台激活后的文本输入让渡
- 异常时回旧 UI

## 5. 配置与持久化边界

### 5.1 可以新增的配置

- RmlUI 全局总开关
- 模块级试点开关
- 安全模式开关
- 预设选择
- editor/hud 布局相关序列化状态

### 5.2 第一阶段不要重构的配置

- 旧设置项 key
- 旧 `g_Config` 语义
- 已有 HUD 布局字符串格式，除非 feature 明确声明迁移方案

### 5.3 diagnostics 落盘约束

- roadmap 目标态：统一导出到 `dumps/QmClient_Crash/`
- 当前 prototype 现状：Monitoring HUD diagnostics 仍写到构建目录同级 `log/`，Phase A / `rmlui-resource-diagnostics` 需要显式把它收口到统一导出目录
- 当前 prototype 现状：Monitoring HUD 本身仍是试点宿主，不要把它的现状当成后续所有 RmlUI surface 的成熟基座。
- 文件名应带模块名与运行时间
- feature 验收文档需要引用至少一个真实样例

## 6. feature-design 时建议直接引用的检查表

每次从 roadmap 拆 feature 时，优先确认这几项：

1. 这次切的是“宿主壳层”还是“内部内容”？
2. 旧路径 fallback 具体落在哪个函数/页面/组件上？
3. 失败时谁负责触发 `cancel/release-state`？
4. 这次改动是否会引入新的后端假设？
5. 验收时准备回带哪三类证据：成功渲染、失败回退、输入边界？

## 7. 平台回归清单草案

这份清单用于 feature-design 和 acceptance 之间传递平台风险。不是每个 feature 都必须把三端都跑完，但必须明确“本次已验证 / 本次未覆盖 / 本次不适用”。

### 7.1 OpenGL 桌面

- 启动路径：从构建目录启动客户端，启用 `qm_rmlui_enable` 与目标模块开关。
- 成功证据：RmlUI surface 与主画面同屏可见，日志里有 runtime/module 初始化成功。
- 失败证据：关闭或破坏目标模块资源后，旧 UI fallback 生效。
- 特别观察：不应出现 `wglMakeCurrent(): 请求的资源在使用中`、`no active OpenGL context` 被当成正常前提。

### 7.2 Vulkan 桌面

- 启动路径：使用 Vulkan backend 进入同一个 surface。
- 成功证据：上层 module registry、frame request、diagnostics 字段不需要改名或分叉。
- 失败证据：若底层 bridge 尚未实现，失败必须清楚标为 backend unsupported，并回旧 UI。
- 特别观察：不能出现为了 RmlUI 重新建立 GL context 的代码路径。

### 7.3 Android

- 启动路径：进入对应 surface，触发一次返回键 / back action。
- 成功证据：Android back 进入统一 cancel 行为，窗口尺寸变化后 viewport 能刷新。
- 失败证据：窗口重建或资源缺失时不崩溃，模块回退旧 UI。
- 特别观察：资源路径不要假设桌面当前工作目录，输入桥必须覆盖 focus loss 和 release-state。

## 8. 诊断文件格式草案

目标目录采用 roadmap 口径：现有保存目录下的 `dumps/QmClient_Crash/`。当前 Monitoring HUD prototype 的 `log/` 输出是过渡状态，Phase A / resource diagnostics 需要收口。

文件名建议：

```text
rmlui_{module}_{yyyy-mm-dd_hh-mm-ss}_{result}.txt
```

最小字段：

```text
schema=rmlui-diagnostics-v1
module=monitoring_hud
stage=render_hud
layer=GAME_HUD
result=fallback_required
core_initialized=0
core_available=0
context_available=0
document_loaded=0
core_failure=backend_init_failed
backend_failure=no_gl_context
document_failure=none
fallback_owner=CGameClient::RenderQmMonitoringHud
backend_assumption=opengl_direct_context
timestamp_local=2026-05-07_18-15-12
```

写入规则：

- 开发环境按需自动导出；正式默认不落盘。
- 同一模块同一运行阶段可以节流，避免每帧刷文件。
- 失败诊断必须包含 fallback owner，方便从日志直接回到代码宿主。
- backend assumption 字段只描述本次失败暴露出的假设，不替代后续 render bridge 设计。

## 9. Feature 切片模板

后续每个 feature-design 可以直接套这个落地表：

| 字段 | 示例 |
|---|---|
| host owner | `CGameClient::RenderQmMonitoringHud` |
| fallback owner | `CGameClient::RenderQmMonitoringHud` |
| legacy render target | `m_QmMonitoring.RenderHud(Layout.m_PanelRect)` |
| diagnostics owner | `CRmlUiRuntime` 导出结构化文件，surface 只补充模块状态 |
| input owner | `GAME_HUD` 不消费输入；交互型 surface 交给 `RmlUiInputBridge` |
| backend assumption | 不新增 direct GL context 假设 |
| evidence owner | design/checklist 指定 acceptance 需带回成功日志、失败回退日志、导出文件样例 |

切片纪律：

- 如果一个 feature 同时想改 runtime、render bridge 和具体 UI surface，优先拆回 roadmap item。
- 如果 fallback owner 说不清，说明这条 feature 还不能进入实现。
- 如果 backend assumption 只能写“OpenGL 下能跑”，该 feature 只能作为试验，不满足 roadmap 验收。

## 10. 当前最值得优先展开的 feature-design

### rmlui-runtime-shell

- 要把 module registry、frame request、diagnostics export、safe mode hook 一次搭齐。
- 以 Monitoring HUD 作为唯一接入模块，不接受空文档/空模块式闭环描述。

### rmlui-render-command-bridge

- 要先回答 geometry 如何进入 threaded graphics。
- 不能把“Monitoring HUD 先能画出来”当成 bridge 完成。

### rmlui-monitoring-hud-migration

- 先接正式 runtime/layer/bridge。
- 再收 CSS、字体、graph rect、图表线这些 surface 级问题。

### rmlui-settings-reorg

- 要从 `CMenus::RenderSettings` 宿主层切。
- 第一阶段重点是导航、分类、搜索壳，不是把每个设置项逻辑都搬家。
