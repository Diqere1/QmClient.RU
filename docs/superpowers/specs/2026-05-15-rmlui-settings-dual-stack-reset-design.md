# RmlUI Settings 单一前端重置设计文档

> **状态（2026-05-17）：** 已停止推进。当前工作树不再继续实现 RmlUI 正式代码，本设计仅保留历史记录，不再作为执行依据。

**日期**：2026-05-15
**版本**：2.0
**作者**：AI Assistant
**状态**：按最新产品口径重写，待审查

---

## 1. 概述

### 1.1 背景

当前分支已经有 `qm_ui_stack` 的全局方向，也已经把 `main menu`、`serverbrowser`、`settings` 接进了第一版 RmlUI 菜单骨架。但 settings 这条主线仍然带着明显的过渡态思维：

- settings 还被写成“旧 UI 与 RmlUI 并行的双路径”问题。
- 页面结构还在围绕共享壳片段和临时占位组织。
- 旧 `menus_settings*.cpp` 仍同时承担页面语义、动作、副作用和旧渲染宿主角色。

这与当前产品方向冲突。最新口径已经明确：

- `RmlUI` 是唯一的 UI 前端目标。
- 不允许把旧 UI 作为并行界面或 fallback 方案继续设计。
- 旧 UI 只能作为重建期间的参考实现和底层能力来源。
- 游戏世界渲染核心、配置、数据、命令与功能内核继续保留。

因此，这份文档不再讨论“如何在 settings 里保留双路径”，而是定义 **settings 如何收口为单一 RmlUI 前端入口**。

### 1.2 本 spec 的职责

这份 spec 负责建立 settings 主线的基础边界：

- `settings page` / `settings modal` 作为单一 RmlUI 前端 surface 的入口定义。
- `CMenus` 与旧 settings renderer 的新职责边界。
- `settings` 前端与 settings 语义/数据内核之间的分层关系。
- 当前骨架页如何退出“过渡主线”身份，转成真实前端重建的临时参考。

它不负责：

- 定义 settings 的完整语义模型。
- 定义 settings 的最终页面 IA 与视觉系统。
- 解决地图编辑器或其他复杂 UI 的重建细节。

这些内容分别由后续 spec 和 plan 承接：

- `2026-05-15-rmlui-settings-semantic-surface-design.md`
- `2026-05-15-rmlui-settings-frontend-shell-design.md`
- 对应的实现计划文档

### 1.3 现有代码锚点

- 全局 UI 栈：`src/game/client/ui_stack.h`、`src/game/client/ui_stack.cpp`
- 当前 RmlUI runtime / surface：`src/game/client/RmlUi/RmlUiRuntime.h`、`src/game/client/RmlUi/RmlUiSurface.h`
- 当前 settings 宿主入口：`src/game/client/components/menus.cpp`、`src/game/client/components/menus_settings.cpp`
- 当前 settings 骨架资源：`data/qmclient/rmlui/settings_page.rml`

### 1.4 核心决策

- `settings` 从现在开始只定义一条目标前端路径：`RmlUI settings frontend`。
- 旧 settings renderer 不再作为 settings 主线 architecture 的一部分。
- `CMenus::RenderSettings(...)` 必须从“页面 owner”降级为“数据/动作内核适配入口”。
- 任何“尚未完成”的 settings 域都应表述为“尚未在 RmlUI 中重建完成”，而不是“切回旧 UI”。
- page 与 modal 从一开始就视为两个独立 surface，不允许把 modal 临时塞回 page 文档冒充完成。

---

## 2. 目标与非目标

### 2.1 目标

- 把 settings 主线从“过渡态双路径”重置为“单一 RmlUI 前端重建”。
- 明确 page / modal 的单一 owner 规则。
- 让 `CMenus` 与旧 settings 代码退出页面宿主角色，只保留底层数据、命令和语义能力。
- 为后续 semantic surface 和 frontend shell 提供稳定边界。
- 为未来 editor / map editor 等复杂 UI 的 RmlUI 重建确立同一思路：保留功能内核，重建 UI 前端。

### 2.2 非目标

- 不在本阶段完成 settings 所有分类的 1:1 RmlUI 页面。
- 不在本阶段删除旧 settings 代码。
- 不在本阶段改变配置键、默认值、持久化格式和业务动作语义。
- 不在本阶段完成 map editor UI 的实现计划细化。
- 不在本阶段继续维护“旧 UI 与 RmlUI 并行长期共存”的设计。

---

## 3. 设计

### 3.1 单一前端 owner

settings 前端只有一个目标 owner：

```cpp
enum class ERmlUiSettingsSurface
{
	PAGE,
	MODAL,
};
```

约束：

- `PAGE` 与 `MODAL` 都由 RmlUI frontend 负责。
- 同一帧内，不允许再让旧 settings renderer 作为 active 页面 owner 出现。
- 旧代码可以继续提供数据、状态、命令与配置写回，但不能继续控制页面生命周期。

### 3.2 `CMenus` 的新角色

`CMenus::RenderSettings(CUIRect MainView)` 不再承担 settings 页面宿主职责。它在新架构中的角色应收口为三类：

1. 暴露当前 settings route / 状态 / restart 信号等只读信息。
2. 暴露配置写回、命令触发、modal 请求等动作入口。
3. 为 semantic surface 提供最小必要的 legacy 功能适配。

禁止继续做的事情：

- 不再把 `RenderSettingsQmClient(...)`、`RenderSettingsTClient(...)` 等旧内容函数当成新页面内容源直接绘制。
- 不再把 `settings_page.rml` 片段当作长期宿主，内部再嵌旧内容。
- 不再引入“当前域尚未完成，所以切回旧页面”的行为或措辞。

### 3.3 page 与 modal 的硬边界

从这一版文档开始，settings 至少拆成两个独立 frontends：

- `SETTINGS_PAGE`
- `SETTINGS_MODAL`

边界要求：

- page 承载路由、分类导航、section 列表、控件工作区和右侧上下文区。
- modal 承载确认、输入、选择器、错误提示和特殊编辑流。
- modal 的打开、关闭和结果回传必须显式建模，不能继续借 page 内的临时 DOM 冒充 modal 流程。

### 3.4 当前骨架页的重新定位

当前这些实现不再代表 settings 主线的完成度：

- `main_menu_shell.rml` 中的 settings 工作区片段
- `settings_page.rml` 作为片段式宿主
- `BuildRmlUiSettingsWorkspaceModel(...)` 之类仅服务骨架阶段的装配逻辑

它们现在只保留两类价值：

- 视觉结构参考
- 主菜单壳层和 route 组织方式的过渡素材

这意味着后续如果某个分类仍未重建完成，正确表达应是：

- “该分类尚未完成 RmlUI 重建”

而不是：

- “该分类仍走旧 UI”

### 3.5 与未来 editor UI 的一致性

settings 不是例外，而是第一批验证对象。后续 map editor、assets/editor 风格页面也遵循同一原则：

- 保留原有功能、数据、命令和业务内核。
- 重新实现 UI 前端、路由、交互组织和页面结构。
- 不再把旧 UI 当作长期并行界面继续保留在产品层。

因此，这份 spec 也为后续 editor UI 重建提供统一原则，而不只是服务 settings 一页。

---

## 4. 文件与模块落点

### 4.1 新增

- `src/game/client/RmlUi/RmlUiSettingsFrontend.h`
- `src/game/client/RmlUi/RmlUiSettingsFrontend.cpp`
- `src/test/rmlui_settings_frontend_test.cpp`

### 4.2 修改

- `src/game/client/components/menus.h`
  - 暴露 settings route、状态、动作与 restart 查询入口
- `src/game/client/components/menus.cpp`
  - 收口 `RenderSettings(...)` 的职责边界
- `src/game/client/components/menus_settings.cpp`
  - 抽出可复用的语义/动作辅助函数
- `src/game/client/RmlUi/RmlUiRuntime.h`
  - 接入 settings page / modal frontend owner
- `src/game/client/RmlUi/RmlUiSurface.h`
  - 明确 `SETTINGS_PAGE` / `SETTINGS_MODAL` surface
- `CMakeLists.txt`
  - 注册新增实现与测试

说明：

- 文件名可以暂时保留现有命名，不要求为了文档口径立即大规模重命名。
- 但模块语义必须改对，不能继续用“dual-stack/fallback/legacy owner”当真实设计。

---

## 5. 验证策略

### 5.1 单元测试

新增或补强测试至少覆盖：

- settings page / modal 的 surface 查询稳定存在。
- `CMenus` 暴露的只读接口可在不渲染旧页面的前提下构建 settings 语义模型。
- settings 动作写回不依赖旧页面渲染调用。

### 5.2 宿主行为测试

新增或扩展测试覆盖：

- `RenderSettings(...)` 不再承担旧页面内容 owner 职责。
- settings frontend 可以独立驱动 page / modal 生命周期。
- 前端未完成的 route 会返回稳定的 unavailable / deferred 语义，而不是隐式跳到旧页面。

### 5.3 人工验证

至少验证：

1. `qm_ui_stack=rmlui`
2. 进入 settings
3. 切换已接入的 settings 分类
4. 触发一个需要 restart 的设置
5. 触发一个需要 modal 的动作

预期：

- settings 始终停留在 RmlUI 文档体系内。
- 不再出现“RmlUI 壳包着旧 settings 内容”的混合态。
- 未完成分类会在 RmlUI 内部明确展示，而不是被旧 UI 接管。

---

## 6. 成功标准

- settings 主线的架构表述彻底摆脱“双路径 / fallback / mixed host”。
- page / modal 都被定义为单一 RmlUI frontend surface。
- `CMenus` 和旧 settings 代码只保留功能内核与语义适配角色。
- 后续 semantic surface 和 frontend shell 可以在不借助旧页面宿主的前提下推进。
- 这套原则可直接外推到后续 editor / map editor UI 重建。

---

## 7. 后续衔接

本 spec 完成后，后续顺序为：

1. `rmlui-settings-semantic-surface`
2. `rmlui-settings-frontend-shell`
3. `settings core domains` 的 1:1 重建
4. `editor / map editor UI` 的同类规划

顺序原因：

- 没有 semantic surface，新的 settings frontend 会再次耦合旧 renderer。
- 没有 frontend shell，settings 只会停留在壳层占位，而不是可用页面。
- 没有统一的“保留内核、重建前端”原则，后续 editor UI 会重新掉回旧 UI 并行模式。
