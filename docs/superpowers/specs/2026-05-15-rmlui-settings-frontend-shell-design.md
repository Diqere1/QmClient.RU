# RmlUI Settings 前端壳设计文档

**日期**：2026-05-15
**版本**：2.0
**作者**：AI Assistant
**状态**：按最新产品口径重写，待审查

---

## 1. 概述

### 1.1 背景

settings 已经不再被当作“旧 UI 与 RmlUI 并行过渡区”来设计。新的方向是把 settings 直接作为 RmlUI 的原生前端之一来重建，底层保留原有配置、数据、命令和功能内核，前端则重新组织页面结构、交互和视觉。

当前最重要的变化不是“做一个更漂亮的壳”，而是：

- 让 settings 有自己的 RmlUI page / modal 前端 owner。
- 让页面结构不再依赖旧 UI 的宿主逻辑。
- 让未完成内容表现为“尚未重建完成”，而不是通过 fallback 回到旧 UI。

### 1.2 本 spec 的职责

这份 spec 负责定义 settings frontend shell 本身：

- 页面 IA 和布局结构
- page / modal 的文档边界
- 导航、工作区、上下文区、状态区的职责拆分
- frontend 如何消费 semantic surface 输出

它不负责：

- 选择 settings 路由
- 定义 settings 的完整语义模型
- 决定具体配置项的业务写回

这些由前置语义层 spec 提供：

- `2026-05-15-rmlui-settings-semantic-surface-design.md`

---

## 2. 目标与非目标

### 2.1 目标

- 建立一个真正独立的 settings RmlUI frontend shell。
- 明确 page 和 modal 的生命周期与 DOM 边界。
- 让 settings 的 UI 结构适合长页面、分类导航和高信息密度配置场景。
- 支持后续把更多 editor / map editor 风格界面也按同样前端原则重建。

### 2.2 非目标

- 不要求本 spec 一次性完成所有 settings domain 的 1:1 页面。
- 不继续沿用共享壳里“临时嵌一块内容”的思路。
- 不把旧 settings renderer 作为长期内容宿主。
- 不在这里实现 settings 的完整业务闭环。

---

## 3. 核心设计

### 3.1 独立 frontend owner

新增独立 owner：

```cpp
class CRmlUiSettingsFrontend
{
public:
	bool Init(...);
	void Shutdown();
	bool RenderPage(const SRmlUiSettingsPageModel &Model);
	bool RenderModal(const SRmlUiSettingsModalModel &Model);
	bool HandleAction(const SRmlUiSettingsAction &Action);
};
```

职责：

- 持有 settings page document
- 持有 settings modal document
- 把 semantic surface 的模型映射到 RmlUI DOM
- 把 DOM 事件翻译回 settings action

不负责：

- 页面语义的来源定义
- 当前 route 的选择
- 旧 UI fallback

### 3.2 page / modal 的边界

settings frontend 固定拆成两个文档域：

1. `settings page`
2. `settings modal`

边界要求：

- page 负责主分类导航、section 列表、控件区和状态区。
- modal 负责确认、输入、选择器和高优先级提示。
- modal 打开后必须有独立的输入和关闭语义，不允许把 page 文档当 modal 用。

### 3.3 页面 IA

建议采用固定三栏结构：

1. 左侧 domain rail
2. 中央 main workspace
3. 右侧 context rail

#### 左侧 domain rail

职责：

- 承载一级 domain
- 显示当前 route
- 显示未重建完成的 domain 状态

#### 中央 main workspace

职责：

- 渲染当前 route 的 section 与 control
- 支持滚动和局部锚点
- 承接真实配置控件，而不是空白占位

#### 右侧 context rail

职责：

- 显示 restart 状态
- 显示 route 帮助与变更摘要
- 显示与当前 route 相关的 availability reason

### 3.4 顶部工具条

settings frontend 需要自己的顶部工具条，而不是复用旧主菜单模式条。

建议包含：

- 当前 domain 标题
- 当前 route 标识
- 搜索入口
- 当前配置状态摘要

### 3.5 control 渲染约束

frontend shell 只渲染 semantic surface 提供的 control model，不自行硬编码业务语义。

最小支持控件：

- toggle row
- slider row
- enum selector row
- button row
- text input row
- color row
- info / warning banner

约束：

- control id 必须与 semantic surface 返回的 id 一致。
- 控件更新必须通过 action -> result -> model refresh 闭环。
- DOM 不允许长期持有业务状态作为真实来源。

### 3.6 对未完成域的表达

frontend shell 必须稳定表达两种情况：

1. `domain not yet rebuilt`
2. `complex page intentionally deferred for RmlUI`

呈现要求：

- 有明确标题
- 有明确 reason
- 有当前 route 所属域说明

禁止：

- 留空大面板
- 写成“以后再做”的模糊占位
- 假装已完成然后回到旧 UI

---

## 4. 文档与样式结构

### 4.1 新增 RML / RCSS

建议新增：

- `data/qmclient/rmlui/settings_frontend.rml`
- `data/qmclient/rmlui/settings_frontend.rcss`
- `data/qmclient/rmlui/settings_modal.rml`
- `data/qmclient/rmlui/settings_modal.rcss`

### 4.2 必需 DOM 锚点

`settings_frontend.rml` 至少保留：

- `settings_frontend_root`
- `settings_domain_rail`
- `settings_toolbar`
- `settings_workspace`
- `settings_context_rail`
- `settings_restart_banner`
- `settings_route_title`
- `settings_route_subtitle`
- `settings_section_host`
- `settings_search_slot`

`settings_modal.rml` 至少保留：

- `settings_modal_root`
- `settings_modal_title`
- `settings_modal_body`
- `settings_modal_actions`

### 4.3 样式策略

允许：

- 复用已有颜色 token、字号层级、阴影和按钮语言
- 提高信息密度
- 让 settings 更适合长页面浏览

不允许：

- 把 settings 强塞回旧主菜单壳层内容注入模式
- 让结构本身继续依赖 placeholder 语义

---

## 5. 代码落点

### 5.1 新增

- `src/game/client/RmlUi/RmlUiSettingsFrontend.h`
- `src/game/client/RmlUi/RmlUiSettingsFrontend.cpp`
- `src/test/rmlui_settings_frontend_test.cpp`

### 5.2 修改

- `src/game/client/RmlUi/RmlUiRuntime.h`
  - 接入独立 settings frontend owner
- `src/game/client/RmlUi/RmlUiRuntime.cpp`
  - 负责 settings page / modal 文档初始化、刷新与渲染
- `src/game/client/RmlUi/RmlUiSurface.h`
  - 明确 settings page / modal 的 surface 角色
- `CMakeLists.txt`
  - 注册新 frontend 文件、资源和测试

---

## 6. 验证策略

### 6.1 结构测试

新增测试至少覆盖：

- `settings_frontend.rml` 必需 DOM 锚点存在
- `settings_modal.rml` 必需 DOM 锚点存在
- frontend 在无 semantic model 时给出明确 unavailable 结果

### 6.2 路由测试

新增测试至少覆盖：

- domain rail 能稳定映射 route tree
- 切换核心 domain 时页面标题和 section host 正确更新
- 未完成域能显示 reason，不留空白

### 6.3 交互测试

新增测试至少覆盖：

- control 事件能转成 settings action
- action result 触发 restart banner 刷新
- modal 打开/关闭不污染 page route 状态

### 6.4 人工验证

至少人工验证：

1. `qm_ui_stack rmlui`
2. 进入 settings
3. 切换核心 domain
4. 触发一个需要 restart 的设置
5. 触发一个需要 modal 的动作

预期：

- settings 不再依附旧 UI 的内容宿主才能存在。
- page / modal 都有明确生命周期。
- 未完成内容明确显示为 RmlUI 内部未重建状态。

---

## 7. 成功标准

- settings 有真正独立的 RmlUI frontend owner。
- 它消费 semantic surface，而不是回调旧 renderer。
- 核心 domain 能在这个 frontend 中形成首批真实页面。
- 未完成域也具备完整上下文说明，而不是依赖旧 UI 接管。

---

## 8. 后续衔接

这份 spec 完成后，下一阶段应进入实现计划，顺序固定为：

1. `rmlui-settings-semantic-surface`
2. `rmlui-settings-frontend-shell`
3. `rmlui-settings-core-domains-1to1`
4. `rmlui-editor-ui-rebuild-planning`

如果跳过前两步直接实现本 spec，settings frontend 最终只会再次变成一层更复杂的壳。
