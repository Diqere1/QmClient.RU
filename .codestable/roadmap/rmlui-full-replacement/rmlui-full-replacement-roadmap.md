---
doc_type: roadmap
slug: rmlui-full-replacement
status: paused
created: 2026-05-06
last_reviewed: 2026-05-13
tags: [rmlui, ui, rendering, migration]
related_requirements: [rmlui-full-replacement]
related_prd: [rmlui-full-replacement]
related_architecture: []
---

# RmlUI 全面替代计划

> 2026-05-13 起，这份 roadmap 不再作为当前执行主线。  
> 当前执行主线改为 [rmlui-dual-ui-replacement](../rmlui-dual-ui-replacement/rmlui-dual-ui-replacement-roadmap.md)：
> 继续追求全量 RmlUI 替代，但采用“双栈 UI 共存 + surface 级切换 + 生命周期完全解耦”的路线，而不是继续沿 `menu_pilot / settings-reorg` 的混合宿主推进。

## 1. 背景

现有代码和验收记录表明，Monitoring HUD 已经成为当前第一条完成验收的 RmlUI 混合迁移样板：宿主先走 switchboard / runtime，文档壳层由 RmlUI 持有，图表 overlay 仍由旧 `IGraphics` 在 RmlUI 计算出的矩形内绘制，失败时由原宿主立即回退旧 HUD。这个样板证明了“RmlUI 不必抢走主画面也能稳定并行”的最小闭环已经成立，但它并不等于整个 RmlUI 替代已经完成。

运行结果也暴露出更深层的问题：当前实现仍然只闭合了 OpenGL 上的最小 bridge slice，图表绘制也仍是 mixed render 形态；更大范围的 input bridge、menu/popup migration 和真正后端无关的 full render bridge 仍未完成。因此 roadmap 的后半段仍然成立，不能因为 Monitoring HUD 已验收就把后续工作提前宣布结束。

本 roadmap 的目标是把 RmlUI 从“旁路调试实验”推进成“可灰度启用的新 UI 管线”。旧 UI 必须保留，新 RmlUI 路径用开关逐步启用，每一步都能独立验证和回退。

这条路线的职责边界也需要提前锁死：RmlUI 是玩家可选的另一套现代化 UI，主要覆盖主菜单、设置页面、HUD、轮盘、弹窗和其他地图渲染之外的界面，不负责接管实际地图渲染。

## 2. 范围与明确不做

### 本 roadmap 覆盖

- RmlUI 运行时生命周期、资源、字体、文档加载和错误状态管理。
- RmlUI 渲染接入策略，避免直接挤占游戏主画面，并为 OpenGL、Vulkan、Android 保持一致上层协议。
- RmlUI layer 顺序、fallback 规则和模块级开关。
- Monitoring HUD 作为最小闭环迁移目标。
- 后续 HUD、菜单、设置、Click GUI、轮盘和编辑器等 UI 的分阶段迁移入口。

### 明确不做

- 不一次性删除 CUI / QmUI 旧实现。
- 不把旧 UI 视作过渡代码。即使 RmlUI 成熟，旧 UI 仍作为正式保留路径存在。
- 不改变游戏物理、预测、网络协议、demo 或地图行为。
- 不让 RmlUI 接管实际地图渲染或世界内容绘制。
- 不在第一阶段强行完成全菜单迁移。
- 不把 OpenGL 直绘作为长期唯一方案，也不接受只覆盖桌面 OpenGL 的“半替代”。
- 不把 RmlUI 样式系统扩展成独立设计系统，视觉重做归后续 feature。

## 2.1 当前证据与问题归因

目前这份 roadmap 不是从抽象设想出发，而是基于已经出现的运行日志和现象倒推出来的：

- `backend init failed: no active OpenGL context` 说明 runtime 还依赖“当前帧正好有 GL context”这种不稳定前提。
- `wglMakeCurrent(): 请求的资源在使用中` 说明 direct GL path 会和现有图形线程争用上下文。
- `invalid main graph rect ... w=0.00 h=0.00` 说明文档虽已加载，但布局与图表绘制链路还没形成稳定契约。
- 字体缺失、RCSS shorthand 警告说明资源兼容问题会直接外溢成可见 UI 异常。
- 当前问题虽然先在 Windows OpenGL 暴露，但如果上层契约继续绑定 GL 直绘语义，Vulkan 与 Android 只会更难接入。
- 现有 Monitoring HUD 接入本身仍是试点实现，日志里出现过“可渲染”不代表它已经满足后续迁移的稳定性要求，更不代表内部布局、图表和资源链已经达到可复用标准。
- 2026-05-10 的菜单 Monitoring HUD 异常进一步说明：把多个独立 surface 再塞回同一个 `Rml::Context` 再各自轮流 `Update()/Render()` 会放大串场和焦点污染；这条约束现在统一收口到 settings-host / runtime-host contract。
- 2026-05-12 结合 RmlUi 官方文档复核后，`Rml::Context` 被明确说明为“独立的 document 集合”，每个 context 自己持有尺寸、鼠标范围、hover/focus 等输入状态，并可独立接收 input、`Update()` 与 `Render()`；这意味着 settings page 和 settings modal 不是“最好分开”，而是应该作为 roadmap 级硬约束拆分。
- 同一轮官方文档复核也说明：RmlUi 已经提供 `<form>` 容器、`input` / `button` / `select` 等标准表单控件，以及 `data-model` / `data-value` / `data-event-*` 这类 data binding 能力，足够支撑“先从 settings 里挑一组 domain 做原生控件闭环”，没必要继续把“原生控件可不可行”混进 settings host 稳定化阶段里一起赌。

结论是：现阶段的主要矛盾不是单个 HUD 组件，而是 RmlUI 还没有被纳入正式 UI runtime、render bridge 和 layer/fallback 体系。

## 3. 模块拆分（概设）

```text
RmlUI 全面替代
├── RmlUiRuntime：统一管理 RmlUI 生命周期、context、文档、字体和失败状态
├── RmlUiRenderBridge：把 RmlUI geometry 转进后端无关的客户端 UI 渲染管线
├── RmlUiLayerManager：定义 RmlUI 与旧 UI 的层级顺序、开关和 fallback
├── RmlUiInputBridge：把鼠标、键盘、文本输入按 UI 状态投递给 RmlUI
├── RmlUiResourcePipeline：管理 rml/rcss/font/image 资源路径、热重载和诊断
├── RmlUiNavigationSuite：承接设置页重组、Click GUI 与轮盘类交互
├── RmlUiAuthoringTools：承接 HUD 编辑器与组件编辑器等低优先级创作工具
├── RmlUiSafetyTooling：承接安全模式、开发者检查器与预设等长期演进能力
└── RmlUiMigrationTargets：按模块迁移 Monitoring HUD、debug HUD、菜单和弹窗
```

### RmlUiRuntime

- **职责**：负责 RmlUI 初始化、关闭、viewport 更新、document 加载、Update 调用、错误状态和诊断导出。
- **长期注意事项**：context 是独立 UI 域，而不是简单的 document 容器别名。菜单/弹窗类 surface 与 HUD/overlay 类 surface 必须默认分属不同 context，不能为了少改代码继续共享一个 context 后再靠 `Hide()/Show()/PullToFront()` 做宿主级互斥。
- **承载的子 feature**：rmlui-runtime-shell, rmlui-resource-diagnostics
- **触碰的现有代码 / 模块**：`src/game/client/RmlUi/`, `src/engine/client/rmlui_backend.*`, `src/game/client/gameclient.*`

### RmlUiRenderBridge

- **职责**：替代当前 `RenderInterface_GL3` 直绘路径，提供可接入客户端渲染顺序、并能分别落地到 OpenGL / Vulkan / Android 后端的 RmlUI 渲染桥。
- **承载的子 feature**：rmlui-render-command-bridge, rmlui-scissor-texture-bridge
- **触碰的现有代码 / 模块**：`src/engine/client/graphics_threaded.*`, `src/engine/graphics.h`, `src/game/client/RmlUi/`

### RmlUiLayerManager

- **职责**：定义 RmlUI 文档在哪些层渲染，以及旧 UI 与新 UI 同屏时的先后顺序和 fallback。
- **承载的子 feature**：rmlui-layer-switchboard
- **触碰的现有代码 / 模块**：`src/engine/client/client.cpp`, `src/game/client/gameclient.*`

### RmlUiInputBridge

- **职责**：把输入事件投递给 RmlUI，并根据文档是否捕获输入决定是否继续传给游戏 / 旧 UI。
- **承载的子 feature**：rmlui-input-bridge
- **触碰的现有代码 / 模块**：client input dispatch, menu / console active state

### RmlUiResourcePipeline

- **职责**：规范 RmlUI 资源路径、字体加载、样式兼容诊断和开发环境导出。
  - **承载的子 feature**：rmlui-resource-diagnostics, rmlui-style-compatibility
- **触碰的现有代码 / 模块**：`data/qmclient/rmlui/`, `data/fonts/`, storage file interface

### RmlUiMigrationTargets

- **职责**：把实际 UI 模块分批迁移到 RmlUI，每个目标都有独立开关、fallback 和验收标准。
- **承载的子 feature**：rmlui-monitoring-hud-migration, rmlui-debug-hud-migration, rmlui-popup-migration, rmlui-menu-pilot
- **触碰的现有代码 / 模块**：Monitoring HUD, debug HUD, menus, popups

### RmlUiNavigationSuite

- **职责**：把设置页面、Click GUI 和 Bind/表情轮盘这类交互式功能面板收敛到统一导航体验；对 settings 主线继续细分为 host 稳定化、IA/state adapter、首个原生 domain、搜索与视觉刷新几个串行阶段。
- **承载的子 feature**：rmlui-settings-reorg, rmlui-settings-ia-state-adapter, rmlui-settings-first-native-domain, rmlui-settings-search, rmlui-settings-visual-refresh, rmlui-click-gui-suite
- **触碰的现有代码 / 模块**：`CMenus::RenderSettings` 总入口、`menus_settings*.cpp` 各分页、`components/tclient/bindwheel.cpp`、`components/pie_menu.cpp`、chat command entry

### RmlUiAuthoringTools

- **职责**：提供 HUD 布局编辑器与组件编辑器等低优先级创作能力，强调拖拽、吸附和可视化调整。
- **承载的子 feature**：rmlui-hud-layout-editor, rmlui-component-editor
- **触碰的现有代码 / 模块**：现有 `CHudEditor`、HUD layout config、settings UI、optional editor surfaces

### RmlUiSafetyTooling

- **职责**：为 RmlUI 提供安全回退、开发者排障和预设管理能力，降低试点和长期维护风险。
- **承载的子 feature**：rmlui-safe-mode, rmlui-dev-inspector, rmlui-preset-system
- **触碰的现有代码 / 模块**：runtime diagnostics, settings storage, developer debug surfaces

## 4. 模块间接口契约 / 共享协议（架构层详设）

### 4.1 RmlUI 运行时帧接口

**方向**：GameClient / Client render loop → RmlUiRuntime
**形式**：函数调用

**契约**：

```cpp
enum class ERmlUiLayer
{
	GAME_HUD,
	DEBUG_OVERLAY,
	MENU_PAGE,
	MENU_MODAL,
	RADIAL_OVERLAY,
	EDITOR_OVERLAY,
};

enum class ERmlUiFrameResult
{
	RENDERED,
	SKIPPED_DISABLED,
	SKIPPED_UNAVAILABLE,
	FALLBACK_REQUIRED,
};

struct SRmlUiFrameRequest
{
	ERmlUiLayer m_Layer;
	int m_ViewportWidth;
	int m_ViewportHeight;
	float m_FrameTimeSec;
	bool m_DebugDiagnostics;
};

struct SRmlUiFrameResult
{
	ERmlUiFrameResult m_Result;
	const char *m_pFailureReason;
};

SRmlUiFrameResult RenderRmlUiLayer(const SRmlUiFrameRequest &Request);
```

**约束**：

- 调用方按固定 layer 顺序调用，不允许 RmlUI 模块自行抢占 GL context。
- 返回 `FALLBACK_REQUIRED` 时调用方必须执行旧 UI 路径。
- 正式发行默认不得输出每帧诊断日志；开发模式需要能在 `Release` / `RelWithDebInfo` 本地调试时按需打开诊断导出。
- 同一个 `Rml::Context` 在一帧内只能按“该 context 的全部可见文档”语义完成一次 update/render 流；如果 surface 需要独立输入、焦点或渲染生命周期，必须拆成独立 context，而不是让多个模块轮流对同一个 context 各自 `Update()/Render()`。

建议的固定顺序：

1. `GAME_HUD`
2. `DEBUG_OVERLAY`
3. `MENU_PAGE`
4. `MENU_MODAL`
5. `RADIAL_OVERLAY`
6. `EDITOR_OVERLAY`

surface 到 layer 的映射约定：

| surface | layer | 说明 |
|---|---|---|
| Monitoring HUD、常驻 HUD | `GAME_HUD` | 叠加在游戏画面之上，但不接管世界渲染 |
| 调试 HUD、开发诊断覆盖层 | `DEBUG_OVERLAY` | 只做信息叠加，不拦截 gameplay 输入 |
| 主菜单、设置页、单页导航壳 | `MENU_PAGE` | 页面级表面 |
| 确认框、提示框、资源错误弹窗 | `MENU_MODAL` | 菜单之上的模态层 |
| Bind 轮盘、表情轮盘、Click GUI 浮层 | `RADIAL_OVERLAY` | 需要独立输入消费边界 |
| HUD 编辑器、开发者检查器 | `EDITOR_OVERLAY` | 高优先级编辑态覆盖层 |

焦点仲裁规则：

- `EDITOR_OVERLAY` 激活时独占输入焦点，并屏蔽 `MENU_PAGE`、`MENU_MODAL`、`RADIAL_OVERLAY` 的输入路由，只保留统一 cancel 路径。
- `RADIAL_OVERLAY` 激活时不得再同时进入新的 editor 态；若 editor 先激活，轮盘类入口必须拒绝打开或直接回退旧实现。
- 所有跨层关闭动作统一走 cancel/release-state 协议，不允许各 surface 自己残留按下、hover 或选择状态。

### 4.2 RmlUI 模块注册接口

**方向**：UI 模块 → RmlUiLayerManager
**形式**：函数调用 / 注册表

**契约**：

```cpp
struct SRmlUiModuleDescriptor
{
	const char *m_pModuleName;
	const char *m_pConfigToggle;
	ERmlUiLayer m_Layer;
	const char *m_pDocumentPath;
	bool m_RequiresInput;
	bool m_HasLegacyFallback;
};

bool RegisterRmlUiModule(const SRmlUiModuleDescriptor &Descriptor);
bool IsRmlUiModuleEnabled(const char *pModuleName);
```

**约束**：

- 每个 RmlUI 模块必须声明旧实现 fallback。
- 模块开关默认关闭，除非该模块已验收为稳定。
- 模块名用于日志、诊断文件和配置映射，必须稳定。

### 4.2.1 全局策略开关

| 开关 | 层级 | 默认值 | 作用 |
|---|---|---:|---|
| `qm_rmlui_enable` | 全局 | `0` | 控制 RmlUI runtime 是否允许启动 |
| `qm_rmlui_safe_mode` | 全局策略 | `1` | 控制 RmlUI 异常时是否自动执行安全回退 |

约束：

- `qm_rmlui_safe_mode` 是全局护栏，不属于模块迁移开关，也不受模块开关语义约束。
- 即使 `qm_rmlui_enable=0`，safe mode 的状态也必须可用于定义“启动失败 / 关闭过程中 / 模块异常时如何安全回旧 UI”的统一策略。

### 4.2.2 模块开关矩阵

| 开关 | 层级 | 默认值 | 作用 |
|---|---|---:|---|
| `qm_rmlui_monitoring_hud` | 模块 | `0` | 控制 Monitoring HUD 是否走 RmlUI |
| `qm_rmlui_debug_hud` | 模块 | `0` | 控制 debug HUD 是否走 RmlUI |
| `qm_rmlui_popup` | 模块 | `0` | 控制弹窗类 UI 是否走 RmlUI |
| `qm_rmlui_menu_pilot` | 模块 | `0` | 控制菜单导览页是否走 RmlUI |
| `qm_rmlui_click_gui` | 模块 | `0` | 控制 Click GUI 与轮盘类入口是否走 RmlUI |
| `qm_rmlui_settings_reorg` | 模块 | `0` | 控制重组后的设置页是否走 RmlUI |
| `qm_rmlui_settings_search` | 模块 | `0` | 控制设置搜索与快速定位能力是否启用 |
| `qm_rmlui_radial_action_system` | 模块 | `0` | 控制统一轮盘交互系统是否启用 |
| `qm_rmlui_hud_editor` | 模块 | `0` | 控制 HUD 编辑器增强入口是否走 RmlUI |
| `qm_rmlui_dev_inspector` | 模块 | `0` | 控制开发者检查器是否启用 |
| `qm_rmlui_presets` | 模块 | `0` | 控制现代 UI 预设系统是否启用 |

约束：

- 全局开关关闭时，所有模块强制回到旧实现。
- 模块开关开启但 runtime 不可用时，必须输出一次模块级失败原因，然后回退旧实现。
- 未进入 roadmap 验收阶段的模块，不允许默认开启。

### 4.2.3 保留式切换原则

- 新旧 UI 必须按“模块切换”并存，而不是按“全局替换”切换。
- 设置页、菜单、轮盘、HUD 等每个 surface 都要允许独立回到旧路径。
- 首批落地优先采用“RmlUI 壳层 + 旧逻辑承载”的方式，不要求一开始重写全部业务绘制逻辑。

### 4.3 RmlUI 渲染桥接口

**方向**：RmlUiRuntime → RmlUiRenderBridge → backend-agnostic graphics command submission
**形式**：Rml::RenderInterface 实现

**契约**：

```cpp
class CRmlUiRenderBridge : public Rml::RenderInterface
{
public:
	void BeginFrame(int ViewportWidth, int ViewportHeight);
	void EndFrame();

	void RenderGeometry(Rml::Vertex *pVertices, int NumVertices, int *pIndices, int NumIndices, Rml::TextureHandle Texture, const Rml::Vector2f &Translation) override;
	void EnableScissorRegion(bool Enable) override;
	void SetScissorRegion(int X, int Y, int Width, int Height) override;
	bool LoadTexture(Rml::TextureHandle &TextureHandle, Rml::Vector2i &TextureDimensions, const Rml::String &Source) override;
	bool GenerateTexture(Rml::TextureHandle &TextureHandle, const Rml::byte *pSource, const Rml::Vector2i &SourceDimensions) override;
	void ReleaseTexture(Rml::TextureHandle TextureHandle) override;
};
```

**约束**：

- 不调用 `SDL_GL_MakeCurrent`。
- 不直接 `glBindFramebuffer` / `glUseProgram`。
- 上层桥接契约不得暴露只属于 OpenGL 的生命周期假设。
- 所有绘制进入 DDNet / QmUI 认可的渲染提交路径。
- scissor、texture、blend 状态必须通过桥接层转换，不能污染主渲染状态。
- OpenGL、Vulkan 和 Android 后端允许分别实现底层提交，但必须共享同一套模块注册、layer 调度和 frame request 协议。

### 4.4 输入桥接口

**方向**：Client input dispatch → RmlUiInputBridge → Rml::Context
**形式**：函数调用

**契约**：

```cpp
enum class ERmlUiInputRoute
{
	PASS_THROUGH,
	CONSUMED,
	FALLBACK_TO_LEGACY,
};

struct SRmlUiPointerState
{
	float m_X;
	float m_Y;
	bool m_InsideViewport;
};

struct SRmlUiInputResult
{
	ERmlUiInputRoute m_MouseRoute;
	ERmlUiInputRoute m_KeyboardRoute;
	ERmlUiInputRoute m_TextRoute;
	bool m_RequestClose;
};

void DispatchRmlUiCursorMove(const SRmlUiPointerState &Pointer);
SRmlUiInputResult DispatchRmlUiInputEvent(const IInput::CEvent &Event);
void DispatchRmlUiTextInput(const char *pTextUtf8);
void DispatchRmlUiFocusChanged(bool Focused);
void DispatchRmlUiCancelAction();
void DispatchRmlUiReleaseState();
```

**约束**：

- 游戏中 HUD 默认不消费移动 / 射击等 gameplay 输入。
- 菜单、弹窗、轮盘、编辑器必须同时支持光标移动、按下、释放、文本输入、焦点切换与取消动作，不允许只转发离散 key event。
- 控制台激活时 RmlUI 不抢控制台文本输入。
- 当模块失活、菜单关闭、输入焦点切换或发生异常回退时，必须调用 `DispatchRmlUiReleaseState()`，避免鼠标按下或 hover 状态残留。
- `DispatchRmlUiCancelAction()` 需要统一承接 Escape / Android back / 关闭手势，供菜单、弹窗、轮盘和编辑器共用。
- 非交互 overlay 在输入桥异常时可以退化为“不消费输入 + 保留旧路径”。
- 交互型 surface 在输入桥异常时必须执行 `close/cancel/release-state`，并立即回退旧 UI；仅“禁用输入消费”不足以视为安全回退。

### 4.5 共享诊断数据

**方向**：RmlUiRuntime / modules → diagnostics
**形式**：结构体 + 开发环境文件导出

**契约**：

```cpp
struct SRmlUiDiagnostics
{
	const char *m_pStage;
	const char *m_pModuleName;
	bool m_CoreInitialized;
	bool m_ContextAvailable;
	bool m_DocumentLoaded;
	const char *m_pCoreFailure;
	const char *m_pBackendFailure;
	const char *m_pDocumentFailure;
};
```

**约束**：

- 仅开发模式自动导出诊断文件，开发模式不能绑定到 `CONF_DEBUG` 单一编译宏；Windows 常用的 `Release` / `RelWithDebInfo` 本地调试也必须可启用。
- 默认导出到现有保存目录下的 `dumps/QmClient_Crash/`。
- 诊断文件名必须带模块名和运行时间。

### 4.6 宿主接缝矩阵

这一节回答“RmlUI 具体挂到哪一层旧代码上”，避免后续 feature-design 再从零猜宿主。

| surface / feature | 当前宿主入口 | 旧路径 fallback | 计划中的 RmlUI 宿主接缝 | 首要约束 |
|---|---|---|---|---|
| Monitoring HUD | `CGameClient::RenderQmMonitoringHud` | 现有 monitoring HUD 绘制分支 | 当前已先进入 `CRmlUiLayerSwitchboard` 的 `GAME_HUD` slot，再由 switchboard 决定 runtime render 还是 legacy fallback | 必须在游戏画面上稳定叠加，不能覆盖主渲染 |
| Debug HUD | `CClient::OnRender` 期间的 HUD/overlay 渲染链 | 现有 debug overlay 路径 | 当前已先进入 `DEBUG_OVERLAY` switchboard slot，再稳定回 legacy debug overlay；具体 RmlUI surface 仍待后续迁移 | 默认不消费 gameplay 输入 |
| 弹窗 / 提示 | 菜单与客户端提示渲染链 | 现有 popup/notice UI | 当前 `connecting_popup`、`loading_popup`、`fullscreen_popup`、`popup_menu` 已先进入 `MENU_MODAL` switchboard slot；交互式 RmlUI surface 仍待 input bridge 之后迁移 | 失败时必须立刻回到旧弹窗，不允许卡住输入 |
| 菜单试点页 | `CMenus` 页面分发 | 旧菜单页 | 当前 page host 已先进入 `MENU_PAGE` switchboard slot；具体页面内容仍待 menu pilot / migration feature 接入 | 不迁移全菜单，不打散现有 page index 语义 |
| 设置页主线 | `CMenus::RenderSettings` + settings page/modal contexts | 旧设置页 | 第一阶段先把 `RenderSettings(...)` 收紧成 page host seam，只允许 legacy path 和 active RmlUI path 二选一；第二阶段再把 legacy 语义整理进 state adapter；第三阶段才挑一组 domain 做原生控件闭环 | page/modal 必须使用独立 context，active RmlUI path 禁止并排 legacy render，配置项语义、存储键和默认值不得变化 |
| Click GUI | 目前无统一现代壳，能力散落在旧 UI/命令路径 | 旧入口与命令 | 作为 `RADIAL_OVERLAY` 或菜单旁路浮层挂到新导航套件中 | 第一阶段先统一入口和状态切换，不重写动作执行 |
| Bind 轮盘 | `CBindWheel::OnInput/OnCursorMove/OnRender` | 旧 `CBindWheel` | 先抽象统一轮盘协议，再决定是否由 RmlUI 壳承载表现层 | release/cancel 生命周期必须对齐现有行为 |
| 表情/动作轮盘 | `CPieMenu::OpenMenu/OnInput/OnCursorMove/OnRender` | 旧 `CPieMenu` | 与 Bind 轮盘共用 `RADIAL_OVERLAY` 协议和状态机 | 需要兼顾聊天框写入与目标选择行为 |
| HUD 编辑器 | `CHudEditor` | 旧 editor | 以 `CHudEditor` 作为唯一宿主入口，先在旧 editor 内补拖拽吸附、尺寸调整、布局持久化与调试可视反馈；只有在 `rmlui-monitoring-hud-migration` 与 `rmlui-input-bridge` 验收后，才允许为 editor 新增独立 RmlUI 表现层 design | 编辑态独占输入，不能与轮盘/菜单抢焦点 |
| 开发者检查器 | 当前无现成宿主 | 无 | 优先作为 `EDITOR_OVERLAY` / debug surface，挂在 runtime diagnostics 与 document tree 之上 | 只服务开发排障，不反向侵入玩家主流程 |

宿主接缝规则：

- 每个 surface 的第一宿主必须来自现有渲染/输入入口，不新增一条平行“神秘 UI 主循环”。
- 首批 feature 优先替换“导航壳 / 容器 / 表现层”，动作执行、配置写入、命令触发尽量继续复用旧逻辑。
- 任何需要跨多个旧组件的 surface，都要先在宿主层定义统一 owner，再谈视觉统一。

### 4.7 后端与平台适配策略

这一节锁死“上层统一、下层分后端”的边界，避免 roadmap 以后又退回到 OpenGL 特判。

| 层次 | OpenGL | Vulkan | Android | 统一约束 |
|---|---|---|---|---|
| runtime / module registry / layer manager | 共享 | 共享 | 共享 | 不感知图形 API 差异 |
| frame request / input / diagnostics 协议 | 共享 | 共享 | 共享 | 不暴露 `GL context`、`VkCommandBuffer` 等后端细节 |
| viewport / drawable size 获取 | 走现有 `CGraphicsBackend_SDL_GL::GetViewportSize` / GL drawable | 走现有 SDL Vulkan drawable size | 走 Android 窗口重建后的 drawable size 语义 | 统一对上只暴露 viewport 宽高变化 |
| geometry 提交 | 可映射到 threaded graphics/GL backend | 可映射到 threaded graphics/Vulkan command processor | 依赖 Android 当前实际后端，仍然通过相同 bridge 协议下发 | 所有 geometry 都先过同一层 render bridge |
| texture/image 资源加载 | 共享 storage/resource pipeline | 共享 storage/resource pipeline | 需要兼顾 Android asset 读取与外部存储目录 | 资源路径与失败语义必须保持一致 |
| cancel/back 行为 | Escape / close action | Escape / close action | Android back 映射到统一 cancel action | 统一落到 `DispatchRmlUiCancelAction()` |

平台约束：

- `backend_sdl.cpp` 已经区分 OpenGL 与 Vulkan drawable/context 语义；RmlUI 上层不得再自己分叉第二套窗口/上下文管理。
- Android 窗口可能重建，RmlUI runtime 不得缓存依赖旧 window/native surface 生命周期的后端对象。
- Vulkan 路径不允许通过“先拿 GL context 再画 UI”绕过；如果某个实现只能在 OpenGL 下成立，它只能作为 debug fallback，不能进入正式 roadmap 验收。
- 纹理、裁剪、blend 的行为口径必须先在 bridge 层抽象，再分别映射到底层 OpenGL/Vulkan 提交能力。

### 4.8 阶段验收证据包

每个子 feature 在进入 `cs-feat-accept` 前，至少要带回一组可复查证据，而不是只说“看起来能用了”。

| phase | 最低证据 | 目的 |
|---|---|---|
| Phase A | 模块开关路径人工检查记录、一次成功初始化日志、一次失败回退日志、导出的诊断文件样例 | 证明 runtime 与诊断协议闭环成立 |
| Phase B | 主场景与 RmlUI 并行显示的人工检查记录、至少一次非 OpenGL 假设的桥接日志、裁剪/纹理案例日志或测试证据 | 证明 render bridge 没再走 direct GL 特判 |
| Phase C | Monitoring HUD 游戏内人工检查记录、图表线完整性证据、关闭模块后回到旧 HUD 的对照日志 | 证明首个试点 surface 可迁移可回退，且不能据此默认其内部实现已稳定可复用 |
| Phase D | 弹窗/菜单试点人工检查记录、控制台/取消键边界日志、异常回退证明 | 证明交互式 surface 不会锁死用户 |
| Phase E | 设置重组前后对照、搜索命中样例、轮盘统一行为对照、HUD 编辑器增强的人工检查记录 | 证明体验增强建立在旧语义不变之上 |
| Phase F | 检查器定位案例、预设切换案例、异常场景下的排障链路样例 | 证明后续演进成本被真正降低 |

证据规则：

- 证据优先放在 feature / acceptance 文档中引用，不把主 roadmap 变成人工验收记录仓库。
- 只要某个 feature 涉及异常回退，就必须至少保留一条“失败后回到旧 UI”的证据。
- 涉及 Android back、Vulkan backend 或窗口重建语义的 feature，验收时必须明确标注是“已验证”还是“仍待平台回归”，不允许默认视作已覆盖。

### 4.9 Feature 级落地字段

后续每份 `cs-feat-design` 都需要把本 roadmap 的共用契约下沉成可验收字段，避免实现阶段只记得“要做某个界面”，忘记旧路径、平台和诊断边界。

每份 design 至少回答：

| 字段 | 必填内容 | 目的 |
|---|---|---|
| host owner | 当前旧代码的宿主入口和这次由谁触发 RmlUI 调度 | 防止新增平行 UI 主循环 |
| fallback owner | 失败后回到哪条旧路径，以及谁负责调用 | 防止 RmlUI 失败后把玩家卡住 |
| diagnostics owner | 成功、失败、回退三类诊断由哪个模块输出 | 防止日志继续散在 surface 里 |
| input owner | 是否消费输入；若消费，谁负责 cancel/release-state | 防止菜单、轮盘、编辑器各写各的输入规则 |
| backend assumption | 本 feature 是否新增 OpenGL/Vulkan/Android 假设 | 防止单后端快捷实现混进长期接口 |
| evidence owner | acceptance 需要带回哪些人工检查记录、日志或导出文件 | 防止验收只停在口头描述 |

硬约束：

- `rmlui-runtime-shell` 必须先把字段模板落成最小可用的 runtime/module/diagnostics 数据结构。
- `rmlui-render-command-bridge` 必须单独证明 backend assumption 没向上泄漏。
- 任何交互式 surface 在 `rmlui-popup-migration` 与 `rmlui-menu-pilot` 未验收前，只能做设计或非默认试点，不进入默认可用路径。
- 所有 RmlUI feature 实现默认采用 TDD：先写失败测试或失败验收脚本，再补最小实现，再整理代码与文档。

## 5. 阶段门禁

### Phase A：运行时壳与诊断基线

- 出口条件：Monitoring HUD 模块可注册、可开关、可回退、可导出诊断；对应实现先有失败测试，再通过测试。
- 不通过表现：runtime 失败后仍尝试继续画、日志只在控制台瞬时出现、模块没有独立开关。

### Phase B：渲染桥接入

- 出口条件：RmlUI 可叠加显示且不覆盖主画面，不再依赖长期 direct GL path。
- 不通过表现：游戏画面被覆盖、图形线程仍然争用 context、桥接状态污染主渲染，或者桥接协议仍然写死 OpenGL 语义。

### Phase C：Monitoring HUD 迁移

- 出口条件：卡片、字体、图表、布局完整，HUD 不消费 gameplay 输入，可立即 fallback。
- 不通过表现：图表线缺失、区域尺寸为零、字体缺失导致布局错乱、关闭开关后不能回到旧 HUD。

### Phase D：安全护栏与交互型 UI 试点

- 出口条件：安全模式先可用，再迁移弹窗和单页菜单这类首批可交互 surface，控制台与 gameplay 输入边界稳定。
- 不通过表现：菜单或弹窗失败后无法回旧 UI、输入状态残留导致鼠标或按键卡住、RmlUI 抢文本输入。

### Phase E：体验整合与创作工具

- 出口条件：设置页重组、Click GUI 与轮盘遵守统一导航和输入规则，HUD 编辑器作为低优先级增强可独立演进。
- 不通过表现：设置语义被打散、轮盘和 Click GUI 各写各的入口、编辑器反向绑死核心 HUD 替代。

### Phase F：开发效率与长期配置增强

- 出口条件：开发者检查器可定位页面问题，预设系统可管理现代 UI 配置。
- 不通过表现：排障仍主要靠猜日志、预设改动破坏旧配置兼容。

## 6. 子 feature 清单

1. **rmlui-runtime-shell** — 建立 RmlUI runtime、模块注册、开关和 fallback 骨架。
   - 所属模块：RmlUiRuntime / RmlUiLayerManager
   - 依赖：无
   - 状态：done
   - 对应 feature：`2026-05-07-rmlui-runtime-shell`
   - 备注：第一条工程闭环，以 Monitoring HUD 为唯一接入模块，证明可开关、可 fallback、可诊断，并为后续 layer 调度立规矩；这里的 Monitoring HUD 仍按试点宿主看待，不默认其内部绘制链已足够稳定。
   - 交付标准：Monitoring HUD 模块能通过全局开关、模块开关和失败回退三条路径；对应测试先失败再通过。

2. **rmlui-resource-diagnostics** — 统一字体、rml/rcss 资源路径、样式兼容错误和 debug 导出。
   - 所属模块：RmlUiResourcePipeline
   - 依赖：rmlui-runtime-shell
   - 状态：done
   - 对应 feature：`2026-05-07-rmlui-resource-diagnostics`
   - 交付标准：字体缺失、文档缺失、RCSS 错误能落成结构化诊断，而不只是一条裸控制台日志。

3. **rmlui-render-command-bridge** — 实现不抢 GL context 的 RmlUI 渲染桥，输出到客户端渲染管线。
   - 所属模块：RmlUiRenderBridge
   - 依赖：rmlui-runtime-shell
   - 状态：done
   - 对应 feature：`2026-05-07-rmlui-render-command-bridge`
   - 交付标准：Monitoring HUD 当前最小桥接切片已证明主画面与 RmlUI document path 可并行；bridge 不再通过宿主 `AcquireBackendFrameContext` / `SDL_GL_MakeCurrent` 抢占 context，但上层协议对 Vulkan / Android 的 full backend-neutral bridge 仍待后续 feature。

4. **rmlui-scissor-texture-bridge** — 收紧 scissor 与 texture load/generate/release 的 bridge contract，并把句柄 ownership 提到 QmClient bridge 层。
   - 所属模块：RmlUiRenderBridge / RmlUiResourcePipeline
   - 依赖：rmlui-render-command-bridge
   - 状态：done
   - 对应 feature：`2026-05-08-rmlui-scissor-texture-bridge`
   - 交付标准：texture/scissor 先经过 QmClient bridge contract，desktop GL 当前路径仍可用；geometry/layer/filter/shader full bridge 继续留在后续 feature。

5. **rmlui-layer-switchboard** — 把 RmlUI layer 接入 game HUD、debug overlay、menu、popup 的固定渲染顺序。
   - 所属模块：RmlUiLayerManager
   - 依赖：rmlui-render-command-bridge
   - 状态：done
   - 对应 feature：`2026-05-08-rmlui-layer-switchboard`
   - 交付标准：调用顺序固定，模块不再自己决定何时抢渲染，fallback 路径统一；当前 accepted 范围仅到 host dispatch order / fallback ownership，不宣称 concrete menu/debug/popup RmlUI surface 已迁移。

6. **rmlui-monitoring-hud-migration** — 用新管线迁移 Monitoring HUD，保留旧 HUD fallback。
   - 所属模块：RmlUiMigrationTargets
   - 依赖：rmlui-scissor-texture-bridge, rmlui-layer-switchboard
   - 状态：done
   - 对应 feature：`2026-05-08-rmlui-monitoring-hud-migration`
   - 备注：当前第一条已验收的 concrete RmlUI migration 样板；仍保持 mixed render 和 host fallback owner，不把 input bridge、debug/menu/popup migration 混进这次验收。
   - 交付标准：文字、卡片、图表线完整显示，游戏中叠加稳定，关闭模块开关立即回到旧 HUD。

7. **rmlui-input-bridge** — 为菜单、弹窗等可交互 RmlUI 文档建立输入投递和消费规则。
   - 所属模块：RmlUiInputBridge
   - 依赖：rmlui-layer-switchboard, rmlui-safe-mode
   - 状态：done
   - 对应 feature：`2026-05-07-rmlui-input-bridge`（approved）
   - 交付标准：UI 输入消费边界清晰，HUD 默认不吃 gameplay 输入，控制台文本输入不被抢占；交互型 surface 异常时可统一 close/cancel/release-state 并回退旧路径；当前阶段图形化设置只保留栖梦页中的全局 RmlUI 开关。
   - 备注：已验收输入投递、消费、cancel / release-state、owner priority 和设置页收口，后续 popup / menu-pilot 直接复用该协议基线。

8. **rmlui-debug-hud-migration** — 迁移 debug overlay 类 HUD，验证和游戏画面同屏叠加稳定。
   - 所属模块：RmlUiMigrationTargets
   - 依赖：rmlui-monitoring-hud-migration
   - 状态：planned
   - 对应 feature：未启动
   - 交付标准：至少一个 debug HUD 模块能在游戏中稳定叠加并保留旧路径。
   - 备注：当前已具备 design 前置条件，可作为复用 Monitoring HUD 样板的并行非交互扩展示例，但不抢占 `rmlui-menu-pilot -> rmlui-settings-reorg` 主线。

9. **rmlui-safe-mode** — 为 RmlUI 页面异常提供自动禁用与回退到旧 UI 的安全模式。
   - 所属模块：RmlUiSafetyTooling
   - 依赖：rmlui-runtime-shell, rmlui-resource-diagnostics
   - 状态：done
   - 对应 feature：`2026-05-07-rmlui-safe-mode`
   - 交付标准：某模块连续失败后可自动回退并提示用户，不会把玩家锁死在坏 UI。

10. **rmlui-popup-migration** — 迁移低风险弹窗 / 提示 UI，验证输入消费和 fallback。
   - 所属模块：RmlUiMigrationTargets
   - 依赖：rmlui-input-bridge, rmlui-safe-mode
   - 状态：done
   - 对应 feature：`2026-05-08-rmlui-popup-migration`
   - 交付标准：弹窗获得 UI 输入消费能力，失败时回到旧弹窗。
   - 备注：当前低风险 `fullscreen_popup` modal 子集已经验收闭环，并成为后续 `rmlui-menu-pilot` 的 modal/input/fallback 基线；`popup_menu` 与文本输入型 popup 仍明确保持 legacy，但它们不再阻塞当前主线切到 menu-pilot。

11. **rmlui-menu-pilot** — 选择一个菜单页面作为 RmlUI 试点，不迁移全菜单。
   - 所属模块：RmlUiMigrationTargets
   - 依赖：rmlui-input-bridge, rmlui-popup-migration, rmlui-safe-mode
   - 状态：done
   - 对应 feature：`2026-05-09-rmlui-menu-pilot`
   - 交付标准：单页菜单导览页可用且可一键回到旧菜单，不扩大到全菜单。
   - 备注：当前 design、checklist、实现、targeted tests 与 acceptance 文档都已落盘；它现在是 settings 主线的页面壳基线，而不是完整 settings 替代本身。

12. **rmlui-settings-reorg** — settings host 稳定化；把 active RmlUI settings path 收紧成单宿主路径。
    - 所属模块：RmlUiNavigationSuite
    - 依赖：rmlui-menu-pilot
    - 状态：in-progress
    - 对应 feature：`2026-05-10-rmlui-settings-reorg`
    - 交付标准：`CMenus::RenderSettings(...)` 只在 legacy path 与 RmlUI path 之间二选一；settings page / modal 使用独立 context；active RmlUI path 不再 mixed render。
    - 落地提示：优先接入 `CMenus::RenderSettings` 总入口，按官方文档口径把 context 当独立 document + size + input state owner，而不是继续共享 menu context。
    - 备注：当前挂在这个 item 上的 in-progress feature 已经超出这个更窄范围，继续实现前要先把 feature scope 收回 host 稳定化本身。

13. **rmlui-settings-ia-state-adapter** — 把旧 settings 语义、子页态和副作用整理成 RmlUI 可消费的 domain / destination / section model。
    - 所属模块：RmlUiNavigationSuite
    - 依赖：rmlui-settings-reorg
    - 状态：planned
    - 对应 feature：未启动
    - 交付标准：八个一级 domain 和对应 destination 能稳定驱动 RmlUI-owned 内容模型，且不再依赖 `RenderSettingsContent(...)` 充当 active content owner。
    - 落地提示：先把 `TClient` / `QmClient` / `Configs` / `Contributors` 之类历史入口整理进统一 IA，再谈原生控件 parity。
    - 备注：这是 settings host 稳定化之后的第二步，目标是把 legacy renderer 降级为 reference，而不是继续作为运行时内容层。

14. **rmlui-settings-first-native-domain** — 选择一组高价值 settings domain，建立首条 RmlUI 原生表单控件闭环。
    - 所属模块：RmlUiNavigationSuite
    - 依赖：rmlui-settings-ia-state-adapter
    - 状态：planned
    - 对应 feature：未启动
    - 交付标准：至少一组 settings domain 可由 RmlUI 原生 form controls 稳定承载，且配置项语义、默认值与存储键不变。
    - 落地提示：优先使用官方文档已覆盖的 `<form>`、`input`、`button`、`select` 与 `data-model` / `data-value` / `data-event-*` 能力，不另造平行控件体系。
    - 备注：这一步的目标是证明“settings 原生控件化”在当前宿主和 data binding 约束下可行，而不是一口气追求全量 parity。

15. **rmlui-settings-search** — 为设置页提供搜索和快速定位能力，优先提升可发现性。
    - 所属模块：RmlUiNavigationSuite
    - 依赖：rmlui-settings-first-native-domain
    - 状态：planned
    - 对应 feature：未启动
    - 交付标准：设置项可按关键词或分类快速定位，且不改变原有配置语义。
    - 落地提示：优先建立 settings 索引层、结果跳转与高亮回接，而不是只在 legacy reference 外面包一层假搜索。
    - 备注：搜索必须建立在稳定 IA 和首个原生 domain 闭环之上，否则只会把旧结构再包一层壳。

16. **rmlui-settings-visual-refresh** — 在 settings 原生控件与搜索基础上，统一 settings 页的现代视觉系统、版式节奏与交互反馈。
    - 所属模块：RmlUiNavigationSuite
    - 依赖：rmlui-settings-first-native-domain, rmlui-settings-search
    - 状态：planned
    - 对应 feature：未启动
    - 交付标准：settings 页在全屏、自适应壳和原生控件基础上形成一致视觉层次，不破坏旧语义和 fallback。
    - 落地提示：视觉刷新必须建立在真实的全屏宿主、原生控件和搜索入口之上，不在 host 稳定化阶段混做。
    - 备注：这一步承接视觉统一，避免在 IA 和控件宿主未稳时反复返工。

17. **rmlui-click-gui-suite** — 提供 Click GUI、Bind 轮盘和表情轮盘等现代交互入口。
    - 所属模块：RmlUiNavigationSuite
    - 依赖：rmlui-input-bridge, rmlui-menu-pilot, rmlui-safe-mode
    - 状态：planned
    - 对应 feature：未启动
    - 交付标准：图形化入口可用，输入消费稳定，旧入口仍保留。
    - 落地提示：先做新入口壳和统一状态切换，动作执行继续复用现有命令/功能开关路径。
    - 备注：基础设施层面已经具备开启 feature-design 的前置条件，但它仍属于 settings 主线之后的扩展项。

18. **rmlui-radial-action-system** — 抽象统一的轮盘交互系统，承载 Bind、表情和快捷动作入口。
    - 所属模块：RmlUiNavigationSuite
    - 依赖：rmlui-click-gui-suite
    - 状态：planned
    - 对应 feature：未启动
    - 交付标准：轮盘类入口共享统一交互模型和数据接入方式，旧轮盘仍可保留。
    - 落地提示：以现有 `CBindWheel` 与 `CPieMenu` 两套既有生命周期为调研样本，先抽取共享的打开/取消/选择/确认/执行语义，再统一输入与状态机协议；`hold-release` 与 `open-close` 的差异要优先收敛。

19. **rmlui-dev-inspector** — 提供页面节点、布局、样式和资源状态的开发者检查器。
    - 所属模块：RmlUiSafetyTooling
    - 依赖：rmlui-resource-diagnostics, rmlui-menu-pilot
    - 状态：planned
    - 对应 feature：未启动
    - 交付标准：开发者可直接定位节点、布局和资源问题，减少纯日志盲查。
    - 落地提示：先围绕 RmlUI document tree、element id、layout rect 和 resource load state 做最小检查器。
    - 备注：资源诊断、menu/page/modal current state 与 context 边界都已经足够明确，现在可以开始 feature-design，但实现优先级继续后置。

20. **rmlui-preset-system** — 为 HUD、Click GUI 和轮盘提供可切换预设能力。
    - 所属模块：RmlUiSafetyTooling
    - 依赖：rmlui-settings-first-native-domain, rmlui-radial-action-system
    - 状态：planned
    - 对应 feature：未启动
    - 交付标准：现代 UI 配置可保存和切换预设，同时不破坏旧配置兼容。

21. **rmlui-hud-layout-editor** — 提供 HUD 元素位置与大小的拖拽布局编辑器，支持红绿虚线吸附。
    - 所属模块：RmlUiAuthoringTools
    - 依赖：rmlui-monitoring-hud-migration, rmlui-settings-reorg
    - 状态：planned
    - 对应 feature：未启动
    - 交付标准：可拖拽调整 HUD 元素布局，支持吸附线提示，不影响旧 HUD 使用。
    - 落地提示：第一阶段只允许在 `CHudEditor` 内增强拖拽吸附、尺寸调整、布局持久化与调试可视反馈；若要引入 RmlUI editor 表现层，必须先补独立 feature-design，明确 runtime、input、layer 和 fallback 方案。

22. **rmlui-component-editor** — 提供组件编辑器，用于扩展可视化组件配置和设置页自定义。
    - 所属模块：RmlUiAuthoringTools
    - 依赖：rmlui-hud-layout-editor
    - 状态：planned
    - 对应 feature：未启动
    - 交付标准：作为低优先级扩展能力独立交付，不反向阻塞主替代闭环。

**第一条工程闭环**：第 1 条 `rmlui-runtime-shell` 做完后，可以在开发环境用 `qm_rmlui_enable` 与 `qm_rmlui_monitoring_hud` 启用 Monitoring HUD 模块，确认初始化、layer 调度、fallback、诊断文件以及对应失败测试都能闭环，并且旧 UI 不受影响。

## 7. 推进节奏

截至 2026-05-09，runtime-shell、resource-diagnostics、render-command-bridge、scissor-texture-bridge、layer-switchboard、safe-mode、Monitoring HUD migration、input bridge 和 popup migration 都已经进入已验收状态。当前真正闭合的是“非交互样板 + 宿主接缝 + 最小 bridge + 稳定 fallback + 交互输入协议 + 首条 modal surface”；仍未闭合的是页面级 `MENU_PAGE` concrete surface。

### 7.1 主线优先

当前主线收紧为四步，按顺序推进：

1. `rmlui-menu-pilot`
   - 目标：先把首个页面级 `MENU_PAGE` surface 跑通。
   - 当前真实状态：design 已 approved、checklist 已完成、当前实现与 targeted tests 已落到当前工作树，仍待 acceptance 文档和用户侧验收收口。
   - 作用：把已验收的 input bridge、popup modal 和 safe-mode 基线复用到真正的页面级交互 surface。

2. `rmlui-settings-reorg`
   - 目标：只解决 settings host ownership，把 `RenderSettings(...)` 收紧成单宿主 seam，并明确 page/modal 多-context 边界。
   - 当前真实状态：roadmap 已按更窄 scope 收口，但当前挂载的 in-progress feature 仍然超出这一步的合理范围，需要先 rescope 再继续实现。
   - 作用：先把“谁拥有 active render path、谁拥有 input/focus state”这层问题单独闭环，不再和 IA / parity / visual 刷新混做。

3. `rmlui-settings-ia-state-adapter`
   - 目标：把旧 settings 的 domain、destination、子页态和副作用整理成 RmlUI 可消费的 IA 与 state adapter。
   - 当前真实状态：代码和探索材料都已经足够，但还没把这一步从原 `settings-reorg` 大包里剥离成单独 feature。
   - 作用：把“旧 settings 语义怎么迁”从 host 稳定化中拆出来，先完成信息架构与状态抽象，再进入控件级 parity。

4. `rmlui-settings-first-native-domain`
   - 目标：选择一组高价值 settings domain，基于 RmlUi 官方已有 `<form>` / `input` / `select` / data binding 能力做首条原生控件闭环。
   - 当前真实状态：官方文档已证明基础能力可用，但依赖前一步先把 IA 和 adapter 稳定下来。
   - 作用：把“settings 原生化是否可行”压缩成一个可验证的最小闭环，而不是直接承诺整页 parity。

5. `rmlui-settings-search`
   - 目标：在稳定 IA 与首个原生 domain 宿主之上，为 settings 提供搜索、定位和跳转。
   - 当前真实状态：已明确为主线后续步骤，但必须晚于 `rmlui-settings-first-native-domain`。
   - 作用：避免只对 legacy reference 做表层搜索壳，而是基于真实的 settings 原生结构建立索引与定位。

6. `rmlui-settings-visual-refresh`
   - 目标：在全屏壳、原生控件与搜索入口稳定后，再统一现代视觉系统。
   - 当前真实状态：范围已固定为 settings 主线尾段，当前不提前混入 host 稳定化。
   - 作用：把视觉系统刷新建立在真实的信息架构和原生交互宿主之上，避免返工。

这条主线的核心约束是：先证明 `MENU_PAGE` 和 settings host ownership 稳定，再把 IA/state adapter 收口，随后只挑一组 domain 验证原生控件化，最后再接搜索和视觉刷新；不要让 debug HUD、Click GUI 或 editor 方向分走主线注意力。

### 7.2 已可启动设计的后续项

- `rmlui-debug-hud-migration`：已具备 design 前置条件，可以并行补 feature-design，复用 Monitoring HUD 样板验证另一条非交互 overlay surface。
- `rmlui-settings-reorg`：当前应该先回到更窄的 host stabilization 范围；如果继续沿用旧 feature scope，会再次把 host、IA 和原生控件 parity 混在一个实现包里。
- `rmlui-click-gui-suite`：现在也可以启动 feature-design，先定义入口壳、统一动作模型和 fallback，暂不进入实现。
- `rmlui-dev-inspector`：对象边界和运行时 current state 已够清楚，可以启动 feature-design，继续后置实现优先级。

popup migration 不再属于“还要继续收口才能往前走”的未闭合项。当前已验收的是低风险 `fullscreen_popup` modal 子集，这已经足够作为 menu-pilot 的 modal/input/fallback 基线；剩余 `popup_menu` 与文本输入型 popup 明确留在 legacy，作为后续独立范围，不回流抢占主线。

### 7.3 后置增值项

以下条目继续后置，等 settings 主线稳定后再接：

- `rmlui-click-gui-suite`
- `rmlui-radial-action-system`
- `rmlui-dev-inspector`
- `rmlui-preset-system`
- `rmlui-hud-layout-editor`
- `rmlui-component-editor`

这些条目的共同原则是：它们要么依赖 settings 主线先把页面宿主、IA/state adapter、首个原生控件闭环和搜索基线收稳，要么属于提高后续开发效率和可定制性的增值能力；在当前阶段都不应挤占 menu-pilot -> settings-reorg -> settings-ia-state-adapter -> settings-first-native-domain 的主线节奏。

## 8. 观察项

- 当前 `RenderInterface_GL3` 直绘路径可以继续保留为实验 / debug fallback，但不应作为全面替代的长期路径。
- QmUI 现有 `CUiRuntimeV2` 仍偏骨架；当前 accepted minimal bridge 没有复用它，后续 full render-interface bridge 是否需要借它承载仍待新的 feature-design 明确。
- Vulkan 后端支持不能靠 OpenGL context 直绘解决，Android 也不能依赖桌面 GL 语义，render bridge 方案应避免把后端绑定死。
- 当前 Monitoring HUD 的 RmlUI CSS 存在 shorthand 不兼容和图表区域尺寸问题，可在迁移 feature 中收敛，不建议继续单点补丁扩散。
- 菜单/弹窗与 HUD 的 context 边界已经成为架构约束，而不是实现细节；后续任何 menu、popup、debug HUD、editor feature 都不得重新退回“共享一个 context + 每个模块单独 render”模型。
- `ddnet-rs` issue 156 可以作为视觉参考，但需要做 QmClient 语境下的本地化取舍，不宜机械照搬每个控件细节。
- 如果没有安全模式，菜单、弹窗、轮盘和编辑器类 feature 的试错成本会明显偏高，且存在把用户锁死在坏 UI 的风险。
- 开发者检查器缺席会显著放大设置页、Click GUI 和编辑器类 feature 的排障成本，但它可以晚于安全模式落地。
- 现有 `BindWheel` 与 `PieMenu` 已经形成两套独立状态机，若不尽早统一，后续 RmlUI 轮盘相关 feature 会继续复制交互逻辑。
- 现有 `CHudEditor` 已具备拖拽和缩放基础，roadmap 后续不应假设 HUD 编辑器必须从零实现。
- 聊天命令类入口当前还没有像设置页、轮盘、HUD 编辑器那样清晰的宿主接缝；后续若要纳入 RmlUI，需要先补一次 explore，再决定挂在 Click GUI、聊天面板还是设置页快捷入口。
- 由于本次 roadmap update 明确把 `rmlui-settings-reorg` 缩回 host stabilization，当前 `.codestable/features/2026-05-10-rmlui-settings-reorg/` 下已存在的 design / checklist / in-progress 实现会出现 spec drift；继续编码前应先把该 feature 重写到更窄范围，或拆成新的 feature 链。

## 9. 变更日志

- 2026-05-06：创建 RmlUI 全面替代 roadmap 初稿。
- 2026-05-06：补充当前问题证据、开关矩阵、阶段门禁和各子 feature 的交付标准。
- 2026-05-06：补充 RmlUI 职责边界、永久保留旧 UI 的原则，以及设置页、Click GUI、HUD 编辑器等后续能力规划。
- 2026-05-06：补充设置搜索、统一轮盘系统、安全模式、开发者检查器和预设系统等精修路线。
- 2026-05-06：基于代码调研补充设置页、轮盘系统和 HUD 编辑器的现有接缝与落地提示。
- 2026-05-07：补充宿主接缝矩阵、后端/平台适配策略和阶段验收证据包，方便后续 feature-design 直接落地。
- 2026-05-07：补充 feature 级落地字段，明确每个子 feature 必须携带宿主、fallback、诊断、输入、后端假设和证据责任。
- 2026-05-07：补充 `rmlui-full-replacement-readiness-matrix.md`，用于追踪每个 roadmap item 的 design/implementation readiness 和解锁条件。
- 2026-05-08：把 `rmlui-render-command-bridge` 回写为 accepted minimal bridge baseline，并同步 resource-diagnostics / safe-mode / menu-pilot 的真实 roadmap 状态。
- 2026-05-08：把 `rmlui-scissor-texture-bridge` 回写为 accepted texture/scissor bridge baseline，并明确 save-layer texture handle 重写已进入 current contract。
- 2026-05-08：把 `rmlui-layer-switchboard` 回写为 accepted host dispatch baseline，并同步 Monitoring HUD / debug / menu / popup 的当前宿主接缝状态。
- 2026-05-08：把 `rmlui-monitoring-hud-migration` 回写为第一条已验收的 concrete RmlUI migration 样板，并把默认 UI 验收口径统一收口到人工检查 + 手工运行日志/构建测试证据。
- 2026-05-08：补充当前进度收口，明确基础设施、非交互样板与交互输入协议均已完成，下一步优先推进 popup / menu-pilot，`rmlui-debug-hud-migration` 调整为 ready-for-design 的次优先扩展示例。
- 2026-05-08：将 `rmlui-input-bridge` 设计批准并回写为 `done`，同时明确当前阶段栖梦设置页只保留全局 RmlUI 图形开关，不再保留 Monitoring 专属图形开关。
- 2026-05-09：将 `rmlui-popup-migration` 回写为 `done`，确认低风险 fullscreen popup modal 已成为当前第一条已验收的交互式 RmlUI surface，并把主线优先级切到 `rmlui-menu-pilot`。
- 2026-05-09：将 `rmlui-menu-pilot` 设计批准并生成 checklist，同时把 roadmap item 推进到 `in-progress`，开始首个 `MENU_PAGE` concrete surface 的实现。
- 2026-05-09：为 `rmlui-menu-pilot` 新建设计草稿，范围收口为 `PAGE_SETTINGS` 的页面壳试点；待 review 通过后再生成 checklist 并推进实现。
- 2026-05-09：重排推进节奏为“主线优先 / 可并行扩展示例 / 后置增值项”三层，明确 `menu-pilot -> settings-reorg` 是当前主线，`debug-hud` 只作为并行样板，popup baseline 已闭合不再反复占用主线叙述。
- 2026-05-10：回写当前进度，`rmlui-menu-pilot` 已补齐 acceptance 并正式收口为已验收页面壳基线；在此基础上，`rmlui-settings-reorg` 转入可直接实现状态，`rmlui-click-gui-suite` 和 `rmlui-dev-inspector` 仍具备启动 feature-design 的前置条件。
- 2026-05-10：补充 context 边界长期约束，明确 menu/popup 与 HUD/overlay 不得继续共享同一个 `Rml::Context` 再由模块各自 `Update()/Render()`；后续 roadmap 与 feature 设计默认按多-context 最佳实践推进。
- 2026-05-10：收紧 settings 主线顺序，明确 `rmlui-settings-reorg` 完成后立即接 `rmlui-settings-native-controls -> rmlui-settings-search -> rmlui-settings-visual-refresh`，并把“RmlUI 的全原生化从设置页开始”固化进 roadmap，而不是继续停留在口头约束。
- 2026-05-11：根据 settings 主线最新决策，把 `rmlui-settings-reorg` 从“RmlUI 壳 + legacy island”方向重置为“RmlUI 单独持有整页 host + dedicated page/modal contexts”；旧 settings 保留为独立 legacy path 与实现参考，但不再作为 active RmlUI settings 的运行时内容层。
- 2026-05-12：结合 Context7 中的 RmlUi 官方文档，确认 `Context` 是独立的 document / size / input-state owner，并确认 `<form>` + 标准表单控件 + data binding 足以承载 settings 原生化；据此把 settings 主线进一步拆成 host stabilization、IA/state adapter、首个原生 domain、搜索和视觉刷新五段，避免再把多种风险塞进一个 feature。
