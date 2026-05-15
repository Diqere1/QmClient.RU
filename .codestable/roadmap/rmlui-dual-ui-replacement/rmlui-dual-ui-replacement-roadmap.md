---
doc_type: roadmap
slug: rmlui-dual-ui-replacement
status: active
created: 2026-05-13
last_reviewed: 2026-05-13
tags: [rmlui, ui, dual-stack, settings, migration]
related_requirements: [rmlui-full-replacement]
related_architecture: [ui-rmlui-current]
---

# RmlUI 双栈全量替代路线

## 1. 背景

这份 roadmap 不是放弃“全量 RmlUI 替代”，而是**重写替代路线**。

当前仓库已经证明两件事：

1. RmlUI 作为 retained-mode 文档 UI，适合菜单、设置页、弹窗、轮盘和编辑器这类长生命周期界面。
2. 把新页面继续挂在旧 `CMenus::RenderSettings(...)` 内容生命周期里混合推进，会把宿主、输入、context、旧状态机和新页面模型绑死，调试成本极高。

因此新的路线不是“放弃全量替代”，而是：

- **保留旧 UI 作为正式第二套 UI**
- **为每个 surface 建立完整的 RmlUI 实现**
- **按 surface 切换使用哪套 UI**
- **同一 surface、同一帧只允许一个 active owner**

这里的“全量替代”定义为：

- 设置页、主菜单、popup、轮盘、编辑器、HUD 等都具备 RmlUI 实现；
- 旧 UI 不被删除，而是保留为并行正式路径、fallback 和对照实现；
- 替代的是“默认或可选主线路径”，不是“物理删除旧代码”。

## 2. 范围与明确不做

### 本 roadmap 覆盖

- RmlUI 与 legacy UI 的双栈并行策略。
- surface 级 path selector、生命周期解耦和宿主切换矩阵。
- settings 主线的回退边界、语义接口抽取和独立前端重建。
- 后续主菜单、popup、Click GUI、轮盘、editor 等 surface 的双栈替代顺序。
- 最终的全量 parity 收口与长期共存策略。

### 明确不做

- 不删除 legacy UI。
- 不再允许同一 surface 在 active RmlUI path 中 mixed render legacy 内容。
- 不要求第一阶段就让所有 settings 页面 1:1 原生完工。
- 不修改现有配置键、默认值、持久化格式、网络协议或地图行为。
- 不把“fallback 到旧 UI”伪装成“RmlUI 页面已完成”。
- 不把仓库外 demo 当主线交付。

## 3. 模块拆分（概设）

```text
RmlUI 双栈全量替代
├── DualUiPathSelector：决定每个 surface 当前走 legacy 还是 RmlUI
├── LegacySemanticSurface：把旧 UI 的真实语义抽成可消费接口
├── RmlUiSettingsFrontend：独立 settings page/modal/document/navigation 前端
├── RmlUiMenuFrontend：主菜单与菜单级 popup 的独立前端
├── RmlUiInteractivePanels：Click GUI、轮盘、编辑器等交互面板
└── DualUiParityAndHardening：双栈切换、诊断、收口与长期共存策略
```

### DualUiPathSelector

- **职责**：把“某个 surface 当前到底走哪套 UI”收口成统一选择器。
- **承载的子 feature**：`rmlui-settings-dual-stack-reset`, `rmlui-main-menu-dual-stack`
- **触碰的现有代码 / 模块**：`CMenus`, `CGameClient`, layer/runtime toggle

### LegacySemanticSurface

- **职责**：从旧 `menus_settings*.cpp`、`menus_tclient.cpp`、`menus_qmclient.cpp` 等路径抽出“配置语义、枚举、动作、副作用”接口，不让旧 renderer 再做 active owner。
- **承载的子 feature**：`rmlui-settings-semantic-surface`, `rmlui-settings-complex-pages-bridge`
- **触碰的现有代码 / 模块**：legacy settings renderer family, popup/action callbacks, config writes

### RmlUiSettingsFrontend

- **职责**：提供独立的 settings page context、modal context、页面树、导航、section/control model 和 1:1 页面实现。
- **承载的子 feature**：`rmlui-settings-frontend-shell`, `rmlui-settings-core-domains-1to1`, `rmlui-settings-search-and-polish`
- **触碰的现有代码 / 模块**：`src/game/client/RmlUi/`, `data/qmclient/rmlui/`, settings host seam

### RmlUiMenuFrontend

- **职责**：把主菜单和菜单级 popup 从 `menu_pilot` 过渡壳推进成真正的独立前端。
- **承载的子 feature**：`rmlui-main-menu-dual-stack`, `rmlui-popup-input-completion`
- **触碰的现有代码 / 模块**：menu shell, fullscreen popup, popup_menu, text input popup paths

### RmlUiInteractivePanels

- **职责**：承接 Click GUI、Bind/表情轮盘、HUD editor、developer inspector 等强交互 surface。
- **承载的子 feature**：`rmlui-click-gui-wheels-dual-stack`, `rmlui-editor-inspector-dual-stack`
- **触碰的现有代码 / 模块**：wheel, pie menu, HUD editor, developer surfaces

### DualUiParityAndHardening

- **职责**：定义 surface toggle 矩阵、诊断、验收策略和长期双栈共存边界。
- **承载的子 feature**：`rmlui-full-parity-hardening`
- **触碰的现有代码 / 模块**：config toggles, diagnostics, acceptance gates, docs sync

## 4. 模块间接口契约 / 共享协议（架构层详设）

### 4.1 Surface Path Selector

**方向**：host seam → DualUiPathSelector  
**形式**：函数调用

**契约**：

```cpp
enum class EDualUiSurface
{
	SETTINGS_PAGE,
	SETTINGS_MODAL,
	MAIN_MENU,
	FULLSCREEN_POPUP,
	POPUP_MENU,
	CLICK_GUI,
	BIND_WHEEL,
	EMOTE_WHEEL,
	HUD_EDITOR,
};

enum class EDualUiPath
{
	LEGACY,
	RMLUI,
};

struct SDualUiPathRequest
{
	EDualUiSurface m_Surface;
	bool m_GlobalRmlUiEnabled;
	bool m_SurfaceToggleEnabled;
	bool m_RuntimeAvailable;
	bool m_FrontendAvailable;
	bool m_SafeModeBlocked;
};

struct SDualUiPathResult
{
	EDualUiPath m_Path;
	const char *m_pReason;
};

SDualUiPathResult SelectDualUiPath(const SDualUiPathRequest &Request);
```

**约束**：

- 同一 `surface`、同一帧只能返回一个 active path。
- `RMLUI` path 失败时，允许宿主退出到 `LEGACY` path，但**不允许同帧 mixed render**。
- `m_pReason` 必须稳定区分 toggle、safe-mode、runtime unavailable、frontend unavailable。

### 4.2 Settings Semantic Surface

**方向**：RmlUiSettingsFrontend → LegacySemanticSurface  
**形式**：函数调用

**契约**：

```cpp
enum class ERmlUiSettingsDomain
{
	TEE,
	HUD_AND_LANGUAGE,
	GRAPHICS,
	SOUND,
	RESOURCES,
	CONFIGURATION,
	FEATURES,
	SEARCH,
};

struct SRmlUiSettingsRoute
{
	ERmlUiSettingsDomain m_Domain;
	const char *m_pDestinationId;
};

struct SRmlUiSettingsPageModel;
struct SRmlUiSettingsAction;
struct SRmlUiSettingsActionResult;

bool BuildRmlUiSettingsPageModel(const SRmlUiSettingsRoute &Route, SRmlUiSettingsPageModel *pModel);
bool ConsumeRmlUiSettingsAction(const SRmlUiSettingsAction &Action, SRmlUiSettingsActionResult *pResult);
bool QueryRmlUiSettingsRestartState(bool *pNeedsRestart, const char **ppReason);
```

**约束**：

- `BuildRmlUiSettingsPageModel(...)` 只能返回页面模型，不得调用旧 renderer。
- `ConsumeRmlUiSettingsAction(...)` 只执行业务语义、副作用和状态写回，不得直接触发 legacy 绘制路径。
- `Route` 必须稳定映射到八个一级 domain，不再把 `Configs`、`Contributors` 当顶层 legacy tab。

### 4.3 Settings Rollback Boundary

**方向**：roadmap → 后续 feature-design  
**形式**：代码边界约束

**契约**：

下面这些路径**退出 active settings 主线**，只保留为 legacy path 或参考实现：

- `src/game/client/components/menus_settings.cpp` 中 active `RenderRmlUiMenuPilot(...)` host 分支
- `src/game/client/components/menus.cpp` 中 `HasActiveRmlUiMenuPilot()` / `BuildRmlUiMenuPilotViewModel(...)` 的 settings mainline 角色
- `src/game/client/RmlUi/RmlUiMenuPilot.*`
- `src/game/client/RmlUi/RmlUiSettingsPageAdapter.*`
- `data/qmclient/rmlui/menu_pilot.rml`
- `data/qmclient/rmlui/menu_pilot.rcss`

下面这些路径**保留并复用**：

- `src/game/client/RmlUi/RmlUiCore.*`
- `src/game/client/RmlUi/RmlUiInputBridge.*`
- `src/game/client/RmlUi/RmlUiPopupModal.*`
- `src/game/client/RmlUi/RmlUiRuntime.*`
- `MENU_PAGE` / `MENU_MODAL` context split
- RmlUI 资源目录与热重载链路

**约束**：

- 旧 `menu_pilot` 可以作为历史实验或参考，不再作为 settings 主线继续扩展。
- 新 settings frontend 不得依赖 `RenderSettingsContent(...)`、`RenderSettingsQmClient(...)`、`RenderSettingsTClient(...)` 作为 active content owner。

### 4.4 1:1 页面优先级协议

**方向**：roadmap → RmlUiSettingsFrontend  
**形式**：固定实现顺序

**契约**：

第一批 1:1 页面顺序固定为：

1. `游戏界面（HUD）/语言`
2. `图像`
3. `声音`

第二批为：

4. `Tee`
5. `配置`
6. `功能` 中纯表单型页面

后置复杂页：

- `资源`
- 资产/音频包编辑器
- key-bind capture 与复杂输入页
- `TClient/QmClient` 中高度状态化、编辑器化页面

**约束**：

- 第一批必须优先选择“纯表单、低输入特殊性、低编辑器依赖”的 domain。
- 复杂页允许先通过 semantic surface 保持 legacy path，不得为了赶进度重新把 mixed render 引回 active RmlUI page。

### 4.5 双栈配置矩阵

**方向**：config → DualUiPathSelector  
**形式**：配置项

**契约**：

```cpp
qm_rmlui_enable
qm_rmlui_settings_frontend
qm_rmlui_main_menu
qm_rmlui_popup_menu
qm_rmlui_click_gui
qm_rmlui_bind_wheel
qm_rmlui_emote_wheel
qm_rmlui_hud_editor
```

**约束**：

- 全局开关关掉时，所有 surface 强制走 `LEGACY`。
- surface 级开关只决定“该 surface 走哪套 UI”，不决定另一套 UI 是否被删除。
- settings 与 main menu 的 path selector 必须支持长期双栈并行存在。

## 5. 子 feature 清单

1. **rmlui-settings-dual-stack-reset** — 回退 settings 混合主线，建立 settings surface 级 path selector 与双栈宿主切换。
   - 所属模块：DualUiPathSelector
   - 依赖：无
   - 状态：planned
   - 对应 feature：未启动
   - 备注：这是最小闭环；做完后 settings 页能稳定选择“完整 legacy settings”或“独立 RmlUI settings frontend 壳”，不再 mixed render。

2. **rmlui-settings-semantic-surface** — 把旧 settings 语义、动作、副作用、restart 状态抽成前端可消费接口。
   - 所属模块：LegacySemanticSurface
   - 依赖：`rmlui-settings-dual-stack-reset`
   - 状态：planned
   - 对应 feature：未启动
   - 备注：这一层只抽语义，不做页面美化。

3. **rmlui-settings-frontend-shell** — 新建独立 `RmlUiSettingsFrontend`，持有 page/modal contexts、导航、section/control model。
   - 所属模块：RmlUiSettingsFrontend
   - 依赖：`rmlui-settings-semantic-surface`
   - 状态：planned
   - 对应 feature：未启动
   - 备注：这是新的 settings 主线入口，不再沿用 `menu_pilot`。

4. **rmlui-settings-core-domains-1to1** — 先把 `游戏界面（HUD）/语言`、`图像`、`声音` 三组页面做成真实 1:1 RmlUI 页面。
   - 所属模块：RmlUiSettingsFrontend
   - 依赖：`rmlui-settings-frontend-shell`
   - 状态：planned
   - 对应 feature：未启动
   - 备注：优先证明纯表单型 domain 可闭环。

5. **rmlui-settings-complex-pages-bridge** — 为 `资源`、编辑器页、复杂输入页建立 semantic bridge 与后置策略。
   - 所属模块：LegacySemanticSurface
   - 依赖：`rmlui-settings-core-domains-1to1`
   - 状态：planned
   - 对应 feature：未启动
   - 备注：解决“哪些复杂页延后、延后时怎么保持双栈清晰边界”。

6. **rmlui-settings-search-and-polish** — 在 core domains 稳定后接搜索、定位和统一视觉整理。
   - 所属模块：RmlUiSettingsFrontend
   - 依赖：`rmlui-settings-core-domains-1to1`
   - 状态：planned
   - 对应 feature：未启动
   - 备注：搜索和视觉刷新都后置，不挤占解耦主线。

7. **rmlui-main-menu-dual-stack** — 把主菜单从 pilot/legacy shell 迁到真正的 dual-stack main menu frontend。
   - 所属模块：RmlUiMenuFrontend
   - 依赖：`rmlui-settings-frontend-shell`
   - 状态：planned
   - 对应 feature：未启动
   - 备注：先让 settings 证明双栈模型，再复制到 main menu。

8. **rmlui-popup-input-completion** — 收口 popup_menu、文本输入型 fullscreen popup 和菜单级 modal 的双栈边界。
   - 所属模块：RmlUiMenuFrontend
   - 依赖：`rmlui-main-menu-dual-stack`
   - 状态：planned
   - 对应 feature：未启动
   - 备注：把现在已完成一部分的 popup migration 扩到难例。

9. **rmlui-click-gui-wheels-dual-stack** — 迁移 Click GUI、Bind/表情轮盘等强交互面板。
   - 所属模块：RmlUiInteractivePanels
   - 依赖：`rmlui-popup-input-completion`
   - 状态：planned
   - 对应 feature：未启动
   - 备注：这批功能依赖前面先把 page/modal/path selector 跑顺。

10. **rmlui-editor-inspector-dual-stack** — 迁移 HUD editor、developer inspector 等高优先级工具 surface。
   - 所属模块：RmlUiInteractivePanels
   - 依赖：`rmlui-click-gui-wheels-dual-stack`
   - 状态：planned
   - 对应 feature：未启动
   - 备注：编辑器型 surface 放到后段，避免过早拖慢主线。

11. **rmlui-full-parity-hardening** — 收口 toggle 矩阵、诊断、验收规则和长期双栈共存文档。
   - 所属模块：DualUiParityAndHardening
   - 依赖：`rmlui-settings-complex-pages-bridge`, `rmlui-settings-search-and-polish`, `rmlui-editor-inspector-dual-stack`
   - 状态：planned
   - 对应 feature：未启动
   - 备注：这是全量替代的最后收口，不是起步项。

**最小闭环**：第 1 条 `rmlui-settings-dual-stack-reset` 做完后，可以端到端证明“settings surface 已经不再 mixed render，且 legacy 与新 frontend 是两条明确可选路径”。

## 6. 排期思路

这条路线故意**先从 settings 开刀**，因为 settings 是当前 mixed-host 问题最重、surface 最大、语义最散的地方。只要 settings 先被解耦成功，后面的 main menu、popup、wheel、editor 都可以复用同一套“双栈 path selector + semantic surface + frontend shell”模式。

顺序上的核心逻辑是：

- 先切断 mixed-host：不再让旧宿主和新前端共享同一生命链路。
- 再抽语义接口：先把旧 UI 的真实能力变成可消费语义面。
- 再做前端壳：让 RmlUI 自己成为 active owner。
- 再做 1:1 核心页面：先拿低复杂度 domain 闭环。
- 最后再收复杂页、搜索、主菜单和强交互面板。

## 7. 观察项

- 现有 `.codestable/roadmap/rmlui-full-replacement/` 仍然保留大量“总迁移”信息，但它已不适合继续充当当前执行主线。
- 当前 `.codestable/features/2026-05-10-rmlui-settings-reorg/` 的设计与 checklist 是基于旧主线，应视为待废弃或待拆分素材，而不是继续直接实现。
- `ui-rmlui-current.md` 当前仍把部分 pilot/current-state 事实写得比较重，后续 acceptance 需要同步回写双栈主线的 current state。

## 8. 变更日志

- 2026-05-13：新建替代路线，明确“全量 RmlUI 替代”应按双栈 UI 共存推进，而不是继续沿 `menu_pilot/settings-reorg` 的混合宿主路径扩张。
