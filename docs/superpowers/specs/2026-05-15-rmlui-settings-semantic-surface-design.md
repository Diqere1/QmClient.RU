# RmlUI Settings 语义接口层设计文档

**日期**：2026-05-15
**版本**：2.0
**作者**：AI Assistant
**状态**：按最新产品口径重写，待审查

---

## 1. 概述

### 1.1 背景

settings 的问题不只是“谁画页面”，而是旧代码把渲染、语义、动作、副作用、滚动和缓存混在了一起。现在既然 RmlUI 是唯一前端目标，就必须先把旧 settings 代码里的可复用能力拆成稳定语义层，让新的 frontend 只依赖语义接口，不再依赖旧 renderer。

这份 spec 的重点不是保留双路径，而是把旧 settings 的能力收口成一层可消费的 **semantic surface**：

- 旧代码继续保留配置、动作、状态、命令与 restart 信息。
- 新 RmlUI 前端通过语义接口拿到 route、page model、action result 和 modal 请求。
- 复杂页面可以先 deferred，但不能再退回旧 UI 作为长期方案。

### 1.2 本 spec 的职责

这份 spec 负责定义：

- settings domain / route / section / control 的语义模型
- 只读页面模型构建接口
- 写操作动作接口和副作用结果
- restart / availability / deferred 的稳定原因模型

它不负责：

- 页面布局
- RCSS 视觉结构
- UI 路由选择

这些由前后相邻 spec 承接：

- 前置：`2026-05-15-rmlui-settings-dual-stack-reset-design.md`
- 后置：`2026-05-15-rmlui-settings-frontend-shell-design.md`

---

## 2. 目标与非目标

### 2.1 目标

- 提供稳定的 settings domain / route / section / control 语义模型。
- 为 RmlUI frontend 提供纯数据页面模型。
- 为写操作提供显式 action/result 接口。
- 提供 restart 状态与 route availability 的统一查询。
- 明确复杂页面的 deferred reason，但不把 deferred 解释成旧 UI fallback。

### 2.2 非目标

- 不在本阶段实现最终 RmlUI 页面。
- 不在本阶段要求所有 settings 页面都完成 1:1 原生化。
- 不改变现有配置键、默认值、存储格式和动作语义。
- 不把 semantic layer 反向做成旧 renderer 的绘制包装层。

---

## 3. 语义模型

### 3.1 一级 domain

settings 统一收口为若干一级 domain：

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
```

这些 domain 是为新的 RmlUI settings IA 服务的，不是旧 tab 的机械映射。

### 3.2 route 与 destination

domain 只回答“属于哪一类”，真实页面跳转依赖 destination id：

```cpp
struct SRmlUiSettingsRoute
{
	ERmlUiSettingsDomain m_Domain;
	const char *m_pDestinationId;
};
```

要求：

- `m_pDestinationId` 必须稳定、可测试。
- 命名使用可读且一致的 kebab-case 风格。

### 3.3 页面模型

RmlUI frontend 消费的只读页面模型建议至少包含：

```cpp
enum class ERmlUiSettingsControlKind
{
	TOGGLE,
	SLIDER,
	ENUM,
	BUTTON,
	TEXT_INPUT,
	COLOR,
	INFO,
	LINK,
};

struct SRmlUiSettingsControlModel;
struct SRmlUiSettingsSectionModel;

struct SRmlUiSettingsPageModel
{
	SRmlUiSettingsRoute m_Route;
	const char *m_pTitle;
	const char *m_pSubtitle;
	std::vector<SRmlUiSettingsSectionModel> m_vSections;
	bool m_RequiresRestart;
	const char *m_pRestartReason;
	bool m_IsDeferred;
	const char *m_pAvailabilityReason;
};
```

约束：

- 页面模型是纯数据结构，不得持有 UI 缓存对象、滚动状态、Rml DOM 指针或旧 renderer 状态句柄。
- `m_IsDeferred` 只用于表达“当前域尚未完成 RmlUI 重建”，不得触发旧 UI 绘制。
- `m_pAvailabilityReason` 必须稳定，例如：
  - `domain_not_yet_rebuilt`
  - `complex_editor_page_deferred`
  - `requires_capture_flow`

### 3.4 动作模型

写操作必须通过显式 action 进入：

```cpp
enum class ERmlUiSettingsActionType
{
	SET_BOOL,
	SET_INT,
	SET_FLOAT,
	SET_STRING,
	TRIGGER_COMMAND,
	OPEN_MODAL,
	RESET_TO_DEFAULT,
};

struct SRmlUiSettingsAction
{
	const char *m_pControlId;
	ERmlUiSettingsActionType m_Type;
};

struct SRmlUiSettingsActionResult
{
	bool m_Changed;
	bool m_RequiresRestart;
	const char *m_pRestartReason;
	bool m_OpenModal;
	const char *m_pModalId;
	const char *m_pStatusMessage;
};
```

约束：

- 动作消费函数只执行业务语义和副作用写回。
- 它不得直接调用 `RenderSettings*`，也不得直接触碰 Rml DOM。
- 所有需要 modal / picker / confirm 的动作都必须显式返回。

---

## 4. 接口设计

建议新增统一接口：

```cpp
bool BuildRmlUiSettingsPageModel(const SRmlUiSettingsRoute &Route, SRmlUiSettingsPageModel *pModel);
bool ConsumeRmlUiSettingsAction(const SRmlUiSettingsAction &Action, SRmlUiSettingsActionResult *pResult);
bool QueryRmlUiSettingsRestartState(bool *pNeedsRestart, const char **ppReason);
bool QueryRmlUiSettingsRouteTree(std::vector<SRmlUiSettingsRoute> *pRoutes);
```

### 4.1 BuildRmlUiSettingsPageModel

职责：

- 根据 route 构建页面标题、section、control 和状态信息。
- 对首批可重建 domain 返回完整结构化模型。
- 对复杂延后域返回 `m_IsDeferred=true` 的可解释页面模型。

禁止：

- 调用任何 `RenderSettings*`
- 依赖旧页面布局缓存
- 直接发起绘制或 UI 事件

### 4.2 ConsumeRmlUiSettingsAction

职责：

- 接受来自 RmlUI frontend 的控件动作。
- 更新配置或触发业务副作用。
- 返回 restart / modal / status 语义结果。

要求：

- 一次 action 只表达一次语义变化。
- 失败必须返回稳定 message，不能静默吞掉。

### 4.3 QueryRmlUiSettingsRestartState

职责：

- 统一聚合 UI 栈切换、图像、声音、更新器等 restart 信号。
- 为 frontend 提供单一 restart banner 数据源。

### 4.4 QueryRmlUiSettingsRouteTree

职责：

- 返回新的 settings IA 树，而不是旧 tab 的裸索引。
- 保证导航和搜索使用同一份 route 源。

---

## 5. 首批覆盖范围

### 5.1 第一批必须可建模的 domain

这份 semantic surface 必须首先保证下面三组能稳定构建页面模型：

1. `HUD_AND_LANGUAGE`
2. `GRAPHICS`
3. `SOUND`

原因：

- 这三组主要是表单型设置，结构稳定。
- 它们最适合验证“语义接口 -> RmlUI frontend -> 配置写回”的第一条闭环。
- 它们不依赖高度特殊的捕获式输入或编辑器式交互。

### 5.2 第二批可先出 route metadata 的 domain

以下 domain 在本层可以先提供 route tree 与状态描述，但不要求立即完整建模：

- `TEE`
- `CONFIGURATION`
- `FEATURES`
- `RESOURCES`

### 5.3 明确后置的复杂页

以下内容在 semantic surface 中只要求可归因，不要求首批原生动作闭环：

- 资源包 / 音频包编辑器
- 复杂键位捕获页
- 高状态化 `TClient` / `QmClient` 编辑型页面
- 需要外部文件浏览器耦合较深的流程

它们必须返回：

- 稳定 route
- 清晰 availability reason
- 是否需要 modal / picker / deferred 的语义说明

---

## 6. 文件与模块落点

### 6.1 新增

- `src/game/client/RmlUi/RmlUiSettingsSemanticSurface.h`
- `src/game/client/RmlUi/RmlUiSettingsSemanticSurface.cpp`
- `src/test/rmlui_settings_semantic_surface_test.cpp`

### 6.2 修改

- `src/game/client/components/menus.h`
  - 暴露必要的只读 settings 语义查询
- `src/game/client/components/menus.cpp`
  - 暴露当前 route 与 restart 语义入口
- `src/game/client/components/menus_settings.cpp`
  - 抽出可复用的 restart / domain 语义帮助函数
- `src/game/client/components/menus_settings7.cpp`
  - 如需暴露 tee/7.x 语义查询，在这里拆最小只读接口
- `src/game/client/components/menus_settings_assets.cpp`
  - 如需暴露资源类页面摘要，在这里拆最小接口
- `src/game/client/components/tclient/menus_tclient.cpp`
  - 为功能型设置暴露纯语义入口
- `src/game/client/components/qmclient/menus_qmclient.cpp`
  - 为 QmClient 模块页暴露纯语义入口

原则：

- 尽量抽小块语义函数，不要重写整个旧 renderer 文件。
- 不把 RmlUI 结构体直接渗进所有 legacy renderer 文件。

---

## 7. 验证策略

### 7.1 纯语义测试

新增测试至少覆盖：

- 一级 domain 与 destination id 的稳定映射
- core domains 的 page model 构建
- restart 状态聚合逻辑
- deferred route 的 availability reason

### 7.2 动作测试

新增测试至少覆盖：

- bool / int / float / enum 类型配置写回
- 动作结果中的 restart 标记
- modal 打开请求的语义结果
- 不支持动作的稳定失败结果

### 7.3 反回归约束

需要明确验证：

- `BuildRmlUiSettingsPageModel(...)` 不依赖 `RenderSettings(...)`
- semantic surface 的测试可以在不启动 RmlUI runtime 的情况下通过
- 旧 settings 路径仍然可以独立保留为功能内核

---

## 8. 成功标准

- RmlUI frontend 可以从单一接口获得 settings 路由树与页面模型。
- 写操作不需要调用旧 renderer 即可完成配置语义写回。
- core domains 已具备后续 1:1 页面实现所需的数据与动作闭环。
- complex editor pages 即使暂缓，也能被明确标注为 deferred，而不是继续躲在 mixed host 里。

---

## 9. 后续衔接

这份 spec 完成后，下一步进入 `rmlui-settings-frontend-shell`。
frontend shell 不再设计自己的业务模型，而是必须完整消费本 spec 定义的 route / page model / action 接口。
