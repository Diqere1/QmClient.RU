# RmlUI 全局 UI 栈分路设计文档

> **状态（2026-05-17）：** 已停止推进。当前工作树不再继续实现 RmlUI 正式代码，本设计仅保留历史记录，不再作为执行依据。

**日期**：2026-05-15
**版本**：1.0
**作者**：AI Assistant
**状态**：设计完成，待审查

---

## 1. 概述

### 1.1 背景

本轮不沿用历史 RmlUI roadmap、旧 design 或其他分支上的 RmlUI 试点实现，而是在当前分支重新定义一条清晰的 UI 分路。目标不是继续把 RmlUI 塞进旧 `CUI` / QmUI 的生命周期里，而是从一开始就把两套 UI 栈拆开：

- 旧 UI 是一套完整 UI 栈。
- RmlUI 是另一套完整 UI 栈。
- 启动后全局只激活其中一套。
- RmlUI 模式下不再把旧 UI 当 fallback。

这个设计的核心不是删除旧 UI，而是把两套 UI 的生命周期、输入、渲染和故障语义分开。旧 UI 仍然保留为可选全局栈，但不再作为 RmlUI 栈失败时的兜底路径。

### 1.2 已确认决策

- 使用全局栈选择器，而不是 surface 级切换矩阵。
- 配置项修改后提示重启生效，不做运行时热切换。
- RmlUI 模式下未完成的 surface 显示 RmlUI 内部「未实现 / 不可用」placeholder。
- placeholder 必须记录稳定 reason，不能静默吞掉问题。
- RmlUI 集成必须参考 RmlUI v6.2 官方 C++ 集成实践。

### 1.3 资料依据

- 本轮对话中已经确认的产品与架构决策：
  - 使用全局 UI 栈选择器。
  - 配置修改后提示重启生效。
  - RmlUI 模式不做 legacy fallback。
  - 未实现 surface 显示 RmlUI 内部 placeholder。
- 当前实现按重新落地处理；历史 RmlUI 试点代码、历史 roadmap 和历史 design 不作为本规格的功能输入。
- RmlUI v6.2 官方文档要点：
  - 初始化顺序：先安装 render / system interface，再 `Rml::Initialise()`，再创建 context、加载字体、加载 document。
  - 主循环顺序：input 先于 `Context::Update()`；`Update()` 之后到 `Render()` 之前不再修改 DOM 或提交事件。
  - interface 生命周期：传给 RmlUI 的 render / system / file interface 必须活到 `Rml::Shutdown()` 之后。
  - context 语义：context 是独立 document 集合和输入 / 焦点域，不应把多个需要独立生命周期的 surface 硬塞进同一个 context 轮流更新。
  - data model：设置页这类表单 UI 应优先使用 data model / data binding，通过 dirty variable 驱动更新。

---

## 2. 目标与非目标

### 2.1 目标

- 新增一个全局 UI 栈选择机制，让客户端启动时选择 `legacy` 或 `rmlui`。
- 保证启动后只有一个 UI 栈处于 active 状态。
- RmlUI 模式下，禁止同帧或静默 fallback 到旧 UI。
- 为未实现 surface 提供 RmlUI 内部 placeholder，保证开发期能看到明确状态。
- 将 RmlUI v6.2 的初始化、主循环、context、input、data binding 和 render interface 约束写成后续实现的硬约束。
- 为后续设置页、主菜单、popup、轮盘、HUD editor 等 UI surface 的完整 RmlUI 重建提供统一入口。
- 第一阶段只要求 RmlUI 栈能启动、能显示统一 placeholder、能稳定记录 reason；不把任何历史 surface 视为已完成。
- 在 placeholder 基线之后，第二阶段优先把 `main menu`、`serverbrowser`、`settings page` 做成首批真实 RmlUI frontend 骨架。

### 2.2 非目标

- 不删除旧 UI。
- 不做 RmlUI 失败自动回旧 UI。
- 不做运行时热切换 UI 栈。
- 不要求第一阶段完成所有页面的 1:1 RmlUI 实现。
- 不改变物理、网络协议、demo、地图行为、预测逻辑或资源格式。
- 不把 RmlUI 作为地图世界渲染系统使用。
- 不把 placeholder 伪装成已完成功能。

---

## 3. 核心概念

### 3.1 UI 栈

`LegacyUiStack` 表示当前 CUI / QmUI 旧界面路径。它继续保留现有渲染、输入、设置页、菜单和 popup 行为。

`RmlUiStack` 表示新的 RmlUI 界面路径。它拥有自己的 runtime、context、document、input bridge、surface registry 和 placeholder。第一阶段只有 stack shell 和 placeholder 是必须完成的真实 UI，业务 surface 后续逐个原生实现。

第二阶段开始，`main menu`、`serverbrowser` 和 `settings page` 不再停留在统一 placeholder，而是进入共享壳层下的真实 frontend 骨架。

### 3.2 全局栈选择器

全局栈选择器在客户端启动时读取配置，并给出唯一结果：

```cpp
enum class EQmUiStack
{
	LEGACY,
	RMLUI,
};
```

第一阶段建议配置项为：

```text
qm_ui_stack legacy|rmlui
```

约束：

- 默认值为 `legacy`。
- 配置变更后只标记「重启后生效」。
- 启动完成后，本次进程内不允许改变 active UI stack。
- RmlUI runtime 初始化失败时，仍然保持 `RMLUI` active stack 语义，并显示 RmlUI 错误 / 不可用界面；不得自动切到 legacy。

### 3.3 Surface

surface 是用户可感知的一块 UI 表面，例如：

- main menu
- settings page
- fullscreen popup
- popup menu
- monitoring HUD
- debug overlay
- click GUI
- bind wheel
- emote wheel
- HUD editor

全局栈选择后，每个 surface 都只能由当前 active stack 的实现负责。RmlUI 模式下，尚未实现的 surface 必须进入 RmlUI placeholder，而不是调用旧 renderer。

第一阶段所有业务 surface 都按 `NOT_IMPLEMENTED` 处理。也就是说，RmlUI 模式的最小可用闭环不是「已经有某个真实页面」，而是「所有入口都能稳定进入 RmlUI 栈，并给出明确 placeholder 和 reason」。

### 3.4 Placeholder

placeholder 是 RmlUI 栈内部的缺失 / 不可用界面。它不是 fallback，也不是临时用旧 UI 填洞。

placeholder 至少需要包含：

- surface 名称。
- 稳定 reason。
- 当前 active stack。
- 可选的开发诊断文本。

稳定 reason 初始集合：

```text
surface_not_implemented
document_missing
runtime_unavailable
frontend_unavailable
context_unavailable
data_model_unavailable
input_bridge_unavailable
render_bridge_unavailable
```

---

## 4. 架构设计

### 4.1 总体结构

```text
QmClient UI
├── QmUiStackSelector
│   └── 启动时读取 qm_ui_stack，决定本进程 active UI stack
├── LegacyUiStack
│   └── 当前 CUI / QmUI 实现，完整保留
└── RmlUiStack
    ├── RmlUiRuntimeOwner
    ├── RmlUiSurfaceRegistry
    ├── RmlUiPlaceholderSurface
    ├── RmlUiInputOwner
    ├── RmlUiRenderOwner
    └── RmlUiDataModelLayer
```

### 4.2 QmUiStackSelector

职责：

- 在启动阶段解析 `qm_ui_stack`。
- 暴露本进程只读的 active stack。
- 为设置页提供「修改后需重启」状态查询。
- 禁止 UI 渲染路径自行重新解释配置。

建议接口：

```cpp
enum class EQmUiStack
{
	LEGACY,
	RMLUI,
};

struct SQmUiStackState
{
	EQmUiStack m_ActiveStack;
	EQmUiStack m_ConfiguredStack;
	bool m_RestartRequired;
};

EQmUiStack ActiveUiStack() const;
SQmUiStackState UiStackState() const;
```

约束：

- `ActiveUiStack()` 在进程生命周期内稳定。
- `m_ConfiguredStack` 可反映用户新写入的配置值。
- 当 `m_ConfiguredStack != m_ActiveStack` 时，设置页显示重启提示。

### 4.3 LegacyUiStack

职责：

- 保持当前旧 UI 行为。
- 在 `ActiveUiStack() == LEGACY` 时负责所有 UI surface。
- 不承担 RmlUI 失败 fallback 职责。

约束：

- 不为了 RmlUI 分路主动重构旧 UI 内部。
- 不把 RmlUI placeholder 或 RmlUI runtime 状态塞进旧 renderer。
- 旧 UI 的存在意义是「另一套可选全局栈」，不是「RmlUI 出错兜底」。

### 4.4 RmlUiStack

职责：

- 在 `ActiveUiStack() == RMLUI` 时负责所有 UI surface。
- 为已实现 surface 调用对应 RmlUI frontend；第一阶段默认没有已实现业务 frontend。
- 为未实现或不可用 surface 显示 RmlUI placeholder。
- 持有 RmlUI runtime、context、input、render 和 data model 生命周期。

约束：

- 不调用 legacy renderer 作为页面内容。
- 不同 frame 内不得同时渲染 RmlUI surface 和同 surface 的 legacy 内容。
- RmlUI 初始化失败也不得把 active stack 改成 legacy。
- 失败路径必须留下稳定 reason。

### 4.5 RmlUiSurfaceRegistry

职责：

- 记录每个 surface 的实现状态。
- 为 RmlUI stack 返回 frontend 或 placeholder。
- 输出稳定诊断。

建议接口：

```cpp
enum class ERmlUiSurface
{
	MAIN_MENU,
	SETTINGS_PAGE,
	FULLSCREEN_POPUP,
	POPUP_MENU,
	MONITORING_HUD,
	DEBUG_OVERLAY,
	CLICK_GUI,
	BIND_WHEEL,
	EMOTE_WHEEL,
	HUD_EDITOR,
};

enum class ERmlUiSurfaceStatus
{
	READY,
	NOT_IMPLEMENTED,
	UNAVAILABLE,
};

struct SRmlUiSurfaceQuery
{
	ERmlUiSurface m_Surface;
	int m_ViewportWidth;
	int m_ViewportHeight;
};

struct SRmlUiSurfaceResult
{
	ERmlUiSurfaceStatus m_Status;
	const char *m_pReason;
};
```

约束：

- `NOT_IMPLEMENTED` 必须走 placeholder。
- `UNAVAILABLE` 必须走 placeholder 或错误 surface。
- `READY` 才能进入真实 frontend。
- reason 字符串必须稳定，便于日志、测试和排障。

### 4.6 RmlUiPlaceholderSurface

职责：

- 提供统一的 RmlUI 内部不可用界面。
- 在 RmlUI stack 下承接所有未实现 surface。
- 给开发者和用户一个明确状态，而不是黑屏或旧 UI 假象。

第一阶段可采用统一 document：

```text
data/qmclient/rmlui/ui_placeholder.rml
data/qmclient/rmlui/ui_placeholder.rcss
```

显示内容必须克制：

- 当前 surface 名称。
- 不可用 reason。
- 提示这是 RmlUI 模式下的未完成界面。

不显示实现计划、快捷键说明或大段开发说明。

### 4.7 RmlUiDataModelLayer

职责：

- 为设置页、菜单列表、资源页等状态型 UI 提供 data model。
- 用 dirty variable 驱动文档刷新。
- 避免在每帧手写 DOM 拼接或反复查找元素。

约束：

- 第一阶段可以只建立接口边界和测试夹具，不要求立刻承载真实设置项。
- data model 在 document 加载前创建。
- C++ 数据对象生命周期必须覆盖 data model 使用期。
- 客户端侧数据变化后必须标记 dirty variable。
- `Context::Update()` 后到 `Context::Render()` 前不得继续改 data model 或 DOM。

### 4.8 RmlUI v6.2 集成纪律

所有后续 RmlUI frontend 必须遵守以下顺序：

1. 创建窗口 / 图形环境和 QmClient 必要服务。
2. 安装 `Rml::RenderInterface`、`Rml::SystemInterface` 和项目 file interface。
3. 调用 `Rml::Initialise()`。
4. 创建 surface 所需 context。
5. 加载字体。
6. 创建 data model。
7. 加载 document。
8. `Show()` document。
9. 每帧在 `Context::Update()` 前提交 input。
10. 调用 `Context::Update()`。
11. 渲染游戏或底层画面。
12. 调用 `Context::Render()` 渲染 UI。
13. 关闭时先 `Rml::Shutdown()`，再销毁 interface。

硬约束：

- render / system / file interface 必须活到 `Rml::Shutdown()` 之后。
- 独立输入、焦点或 modal 生命周期的 surface 默认使用独立 context。
- 不允许多个独立 surface 共用一个 context 后轮流 `Update()` / `Render()`。
- `Update()` 与 `Render()` 之间不提交 input、不修改 DOM、不改变 data model。

---

## 5. 用户体验行为

### 5.1 设置项行为

设置页显示一个全局 UI 栈选项：

```text
UI 栈：旧 UI / RmlUI
```

当用户修改该项后：

- 配置值立即保存。
- 当前进程 active stack 不变。
- 设置页显示「重启客户端后生效」。
- 不尝试立即销毁当前 UI 栈。
- 不尝试立即初始化另一套 UI 栈。

### 5.2 启动行为

启动时读取配置：

- `qm_ui_stack=legacy`：只初始化旧 UI 必需路径。
- `qm_ui_stack=rmlui`：初始化 RmlUI stack，并让 UI host seam 全部进入 RmlUI surface registry。

如果 RmlUI runtime 初始化失败：

- active stack 仍然是 `RMLUI`。
- 展示 RmlUI 错误 / placeholder surface。
- 记录 reason，例如 `runtime_unavailable` 或 `render_bridge_unavailable`。
- 不自动切到 legacy。

### 5.3 未实现 surface 行为

RmlUI 模式下进入未实现 surface：

- 显示 RmlUI placeholder。
- 记录 `surface_not_implemented`。
- 不调用旧 UI 绘制内容。
- 不把 placeholder 状态写成该 surface 已完成。

第一阶段所有业务 surface 都应能走通这一行为。后续某个 surface 只有在拥有自己的 RmlUI document、context 边界、输入语义、reason 分类和验收证据之后，才能从 placeholder 升级为 `READY`。

---

## 6. 初始 Surface 矩阵

| Surface | Legacy 模式 | RmlUI 模式第一阶段 | 说明 |
|---|---|---|---|
| Main menu | 旧 UI | placeholder | 后续重新实现独立 `RmlUiMenuFrontend` |
| Settings page | 旧 UI | placeholder | 后续重新实现独立 settings frontend，不沿用旧 settings renderer |
| Fullscreen popup | 旧 UI | placeholder | 后续重新实现 RmlUI popup frontend |
| Popup menu | 旧 UI | placeholder | 后续重新实现 |
| Monitoring HUD | 旧 UI | placeholder | 后续重新实现，不继承历史 mixed render 试点 |
| Debug overlay | 旧 UI | placeholder | 后续按 overlay context 重新实现 |
| Click GUI | 旧 UI | placeholder | 强交互 surface 后置 |
| Bind wheel | 旧 UI | placeholder | 强交互 surface 后置 |
| Emote wheel | 旧 UI | placeholder | 强交互 surface 后置 |
| HUD editor | 旧 UI | placeholder | 编辑器型 surface 后置 |

第一阶段的验收重点是：上表每个 surface 在 RmlUI 模式下都能进入 RmlUI placeholder，并输出对应 `surface_not_implemented` reason。任何 surface 都不得为了「看起来可用」而调用 legacy renderer。

### 6.1 第二阶段首批真实 frontend 范围

第二阶段不追求三块页面的全量业务闭环，而是先把以下三块做成**真实 RmlUI 页面骨架**：

| 页面 | 在全局栈里的角色 | 第二阶段目标 | 明确不做 |
|---|---|---|---|
| Main menu | 整个 RmlUI 菜单系统的共享壳层 | 建立共享 shell、导航、顶部条、右好友栏、底部状态条 | 不一次性完成所有菜单功能 |
| Serverbrowser | Main menu 里的一级主页面 | 建立真实的服务器浏览工作区、详情区、过滤区和社交上下文区 | 不在本阶段补齐全部 serverbrowser 业务语义 |
| Settings page | Main menu 里的一级主页面 | 建立共享壳层下的设置工作区、分类导航和状态卡位 | 不在本阶段完成全部设置项控件 parity |

约束：

- `serverbrowser` 是 `main menu` 里的一级主页面，不单独升格为独立全局 surface。
- `settings page` 与 `serverbrowser` 必须共用同一套 main menu shell，而不是切成另一种布局体系。
- 第二阶段只要求真实页面骨架、真实 document、真实导航与真实数据模型边界；允许业务内容仍有局部 placeholder。

### 6.2 第二阶段视觉 IA 决策

本轮已确认的视觉 IA 方向如下：

- 使用**左侧纵向主导航**，承载 Home、Servers、Explore、Editor、Demos、Settings 等一级入口。
- 使用**顶部模式条/操作条**承载当前一级页下的二级模式或快捷动作。
- 使用**中央大面板工作区**承载当前主页面的核心内容。
- 使用**右侧好友/社交窄栏**，明确展示在线好友、社交状态和相关快捷入口，而不是泛化成抽象信息栏。
- 使用**底部状态条**承载当前服务器摘要、地图摘要、排名摘要、ping 等低频持续信息。
- 整体视觉采用**深色半透明玻璃面板 + 背景氛围图 + 高亮标题色**，更接近游戏内 UI，而不是通用 SaaS 后台。

### 6.3 第二阶段页面关系

第二阶段固定采用下面这组关系：

- `Main menu shell` 是共享宿主。
- `Serverbrowser` 是 `Main menu shell` 的一级主页面之一。
- `Settings page` 是 `Main menu shell` 的一级主页面之一。
- `Right friends rail` 在 `Main menu shell` 下长期存在，不因为当前位于 `Serverbrowser` 或 `Settings page` 而切走。
- `Bottom status bar` 在 `Main menu shell` 下长期存在，用于承接页面无关但与当前会话强相关的摘要信息。

---

## 7. 错误与诊断

### 7.1 日志字段

每次 RmlUI surface 渲染结果至少记录：

- active stack。
- surface。
- status。
- reason。
- context name。
- document path。

示例：

```text
rmlui_ui_stack active=rmlui surface=settings_page status=not_implemented reason=surface_not_implemented
```

### 7.2 Reason 分类

初始 reason 集合：

- `surface_not_implemented`
- `document_missing`
- `document_invalid`
- `runtime_unavailable`
- `frontend_unavailable`
- `context_unavailable`
- `data_model_unavailable`
- `input_bridge_unavailable`
- `render_bridge_unavailable`
- `font_load_failed`
- `resource_load_failed`

约束：

- 新增 reason 必须是稳定字符串。
- 不使用自由文本作为测试依赖。
- 可读错误信息可以另加，但不能替代 reason。

---

## 8. 验收标准

### 8.1 配置与启动

- 默认配置为 legacy。
- 修改配置后当前进程 active stack 不变。
- 修改配置后设置页提示重启生效。
- 重启后按配置进入对应 UI 栈。

### 8.2 分路隔离

- legacy 模式下不初始化 RmlUI frontend 作为 active UI。
- rmlui 模式下不调用 legacy renderer 作为 active surface 内容。
- 同一 surface、同一帧只出现一个 active owner。

### 8.3 Placeholder

- RmlUI 模式下未实现 surface 显示 RmlUI placeholder。
- placeholder 包含 surface 名称和稳定 reason。
- 日志能区分 `surface_not_implemented`、`runtime_unavailable` 和 `document_missing`。
- 第一阶段所有业务 surface 都以 placeholder 验收，不允许把历史试点实现算作已完成 surface。

### 8.4 RmlUI 集成纪律

- input 在 `Context::Update()` 前提交。
- `Context::Update()` 后到 `Context::Render()` 前不修改 DOM 或 data model。
- fonts 先于 document 加载。
- render / system / file interface 生命周期覆盖 `Rml::Shutdown()`。
- 独立 surface 的 context 拆分有明确理由，不能无说明地共享 context。

### 8.5 回归边界

- 不改变游戏物理、网络协议、地图行为或 demo 格式。
- 不删除旧 UI。
- 不把 RmlUI 失败路径切回 legacy。

---

## 9. 设计取舍

### 9.1 为什么不做 surface 级切换

surface 级切换适合灰度，但容易让旧 UI 和 RmlUI 在同一帧、同一生命周期里互相兜底。当前目标是重构 UI 框架边界，优先级应是把两套 UI 的 owner 关系切干净，而不是让每个 surface 自己协商路径。

### 9.2 为什么不做 fallback

fallback 可以降低短期可用性风险，但它会掩盖 RmlUI surface 的真实完成度。全局 RmlUI 模式下显示 placeholder，能让缺失 surface、资源错误和 runtime 错误都变成可见事实。

### 9.3 为什么要求重启生效

运行时热切换会立刻引入 context 销毁、input release、文本输入 ownership、popup 状态机、资源释放和 render interface 生命周期问题。第一阶段目标是建立清晰边界，因此选择重启生效更稳。

### 9.4 为什么保留旧 UI

保留旧 UI 是为了长期提供另一套可选实现和对照路径，不是为了给 RmlUI 兜底。这样既不破坏现有用户，也不会让 RmlUI 重构继续依赖旧 UI 生命周期。

---

## 10. 后续实现顺序建议

1. 新增全局 UI 栈配置和启动期 selector。
2. 为设置页增加「重启后生效」提示。
3. 建立 RmlUI surface registry 和 placeholder document。
4. 把主要 UI host seam 改为按 active stack 分路。
5. 在 RmlUI 模式下先让未实现 surface 全部进入 placeholder。
6. 为 placeholder、reason 分类、启动期 selector 和无 legacy fallback 写最小测试。
7. 逐步重新实现 settings、main menu、popup、HUD、wheel、editor 的 RmlUI frontend。
8. placeholder 基线稳定后，优先进入 `main menu shell -> serverbrowser workspace -> settings workspace` 这一条首批真实 frontend 主线。

每一步都必须保持一个原则：RmlUI 模式下不调用旧 UI 作为 active surface 内容。
