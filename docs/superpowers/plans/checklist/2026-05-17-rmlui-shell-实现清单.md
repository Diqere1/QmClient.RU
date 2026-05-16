# RmlUI 壳层实现清单版

> **状态（2026-05-17）：** 已停止推进。当前工作树不再继续实现 RmlUI 正式代码，本清单仅保留历史记录，不再作为执行清单。

**日期：** 2026-05-17

**关联计划：**
- `docs/superpowers/plans/2026-05-15-rmlui-main-menu-serverbrowser-settings-skeleton.md`
- `docs/superpowers/plans/2026-05-15-rmlui-settings-dual-stack-foundation.md`

**视觉基线：**
- `.superpowers/brainstorm/rmlui-1778846115/content/closer-to-reference.html`

**强约束：**
- 必须优先参考 `$ui-ux-pro-max`
- 首屏先保证主任务可见
- 关键操作不能被输入态或临时面板遮挡
- 结构化字段默认用专用控件
- 高级操作默认进入次级区域
- 长内容必须可滚动可达
- 所有用户可见字符串必须走 `Localize(...)`

**这份文档的定位：**
- 不再写“方向性建议”
- 直接压成可实现、可分配、可截图验收的清单
- 默认对应 `RML / RCSS / binder / 文案 / 响应式 / 验收截图`

**量化进度（2026-05-17，当前代码基线）：**
- 三份 settings 相关 spec：`3 / 3`，`100%`
- settings 基础层计划：`19 / 36`，`52.8%`
- main menu / serverbrowser / settings 主线计划：`21 / 46`，`45.7%`
- 本清单勾选进度：`0 / 214`，`0%`
- 当前代码验证：`run_cxx_tests = 804 / 804 PASS`，`game-client` 构建通过

**说明：**
- 本清单目前仍按“强约束验收表”使用，尚未逐项回填勾选；`0%` 不代表代码为零实现，而是代表这份清单还没做逐条验收签收。
- 当前已经有真实代码落点：`main_menu_shell.rml/.rcss`、`serverbrowser_page.rml/.rcss`、`settings_frontend.rml/.rcss`、`settings_modal.rml/.rcss`、相关 `RmlUiRuntime / RmlUiServerBrowserPage / RmlUiSettingsFrontend` 绑定与测试。

**冲到 100% 的任务列表：**
1. 把 `main_menu_shell` 五段结构、右侧好友栏和状态反馈逐条按本清单回填勾选
2. 把 `serverbrowser` 三分区主任务布局、列表反馈、次级工具列和中文文案逐条回填勾选
3. 把 `settings` 四区结构、主区优先、上下文区边界、控件反馈逐条回填勾选
4. 补齐 `2k / 4k / 5:4 / 4:3 / 21:9 / 窄高窗口` 截图矩阵后回填“兼容性”和“验收截图”章节
5. 只在真实运行态确认后再勾选“完成标准”六项

---

## 0. 当前问题定义

- [ ] 当前共享壳层仍不达标，不能按“已经完成”处理
- [ ] 中央主区存在大面积空白，首屏主任务没有占住视觉中心
- [ ] 右侧内容出现堆叠和互相抢位，未收敛成单一好友栏
- [ ] 一级按钮、列表项、右侧工具按钮的反馈仍不足
- [ ] 当前实现只证明“结构接通”，没有证明“UI 壳达到参考质量”

## 1. 实施顺序

- [ ] 阶段 1：先收口共享壳层骨架，不继续往空壳里堆卡片
- [ ] 阶段 2：把右侧栏固定为好友列表，不再混放摘要卡
- [ ] 阶段 3：把服务器页首屏主内容补齐，中央主区不再空
- [ ] 阶段 4：把设置页四区结构做成稳定布局
- [ ] 阶段 5：补全 `hover / focus / pressed / active / disabled` 反馈
- [ ] 阶段 6：完成 `Localize(...)` 文案替换
- [ ] 阶段 7：完成 `1366x768 / 1080p / 2k / 4k / 5:4 / 4:3 / 21:9 / 窄高窗口` 验收截图

## 2. 共享壳层文件拆分

### 2.1 `data/qmclient/rmlui/main_menu_shell.rml`

- [ ] 固定五段结构：
  - `shell_left_nav`
  - `shell_top_bar`
  - `shell_main_content`
  - `shell_right_friends_rail`
  - `shell_bottom_status_bar`
- [ ] 左导航必须是一级导航，不接受“导航 + 杂项卡片”混排
- [ ] 顶部分段条只承载一级模式切换和少量次级入口
- [ ] 中央主区只承载当前页面主任务
- [ ] 右侧栏只承载好友列表
- [ ] 底部状态条只承载摘要信息

### 2.2 `data/qmclient/rmlui/main_menu_shell.rcss`

- [ ] 定义共享面板基类：
  - `.panel`
  - `.panel_title`
  - `.panel_body`
  - `.status_block`
- [ ] 定义共享按钮基类：
  - `.nav_button`
  - `.pill_button`
  - `.tool_tile`
- [ ] 定义共享列表基类：
  - `.list_entry`
  - `.friend_entry`
- [ ] 所有交互态在 RCSS 层可见，不允许只在逻辑层变化

### 2.3 `src/game/client/RmlUi/RmlUiMainMenuShell.cpp`

- [ ] 只负责共享壳层路由、工作区分发、右栏与状态条装配
- [ ] 不在 shell owner 内硬编码设置页内部结构
- [ ] 不在 shell owner 内把右栏混成快捷工具摘要区
- [ ] 明确给 DOM 写入状态类或状态属性，便于验收

## 3. 左导航实现清单

- [ ] 左导航条目改为中文本地化：
  - `Localize("主页")`
  - `Localize("服务器列表")`
  - `Localize("发现")`
  - `Localize("编辑器")`
  - `Localize("录像")`
  - `Localize("设置")`
- [ ] 每个导航条目具备图标区、文案区、当前态标识
- [ ] 当前页条目必须明显高于 hover 态
- [ ] hover 态不能只加一点透明度
- [ ] pressed 态必须有位移、阴影或明暗变化
- [ ] focus 态必须保留清晰 focus ring
- [ ] disabled 态必须降低强调度，但文字仍可读

**建议 DOM / class：**
- [ ] `nav_button`
- [ ] `nav_button is-active`
- [ ] `nav_button is-hovered`
- [ ] `nav_button is-focused`
- [ ] `nav_button is-pressed`
- [ ] `nav_button is-disabled`

## 4. 顶部分段条实现清单

- [ ] 顶部分段条只保留一级模式切换
- [ ] 外链、社区、断开连接等放到右侧次级区
- [ ] 一级模式区和次级操作区视觉主次必须分离
- [ ] 一级模式切换按钮必须统一高度、统一节奏、统一反馈
- [ ] 窄窗口下次级区允许折叠，不允许挤压一级模式区

**建议文案：**
- [ ] `Localize("游戏")`
- [ ] `Localize("地图")`
- [ ] `Localize("投票")`
- [ ] `Localize("玩家")`
- [ ] `Localize("幽灵")`

## 5. 中央主区总要求

- [ ] 中央主区首屏必须出现真实主任务，不允许空白背景占大头
- [ ] 页面标题、页面说明、主列表或主控件必须同时可见
- [ ] 高级操作不得压住主内容
- [ ] 次级摘要不得比主列表更抢眼
- [ ] 页面切换后主区高度变化应稳定，不允许闪出巨大空洞

## 6. 服务器页实现清单

### 6.1 `data/qmclient/rmlui/serverbrowser_page.rml`

- [ ] 中央主区固定为三块：
  - `servers_primary_info`
  - `servers_map_info`
  - `servers_rank_info`
- [ ] 底部摘要带固定为：
  - `servers_footer_strip`
- [ ] 次级工具列固定为：
  - `servers_side_tools`
- [ ] 不再把右栏好友列表并入服务器页内部

### 6.2 首屏必须可见的主任务元素

- [ ] 页面标题：`Localize("服务器列表")`
- [ ] 列表区标题：`Localize("服务器")`
- [ ] 地图区标题：`Localize("地图")`
- [ ] 排行区标题：`Localize("排行")`
- [ ] 主动作按钮至少包含：
  - `Localize("刷新")`
  - `Localize("筛选")`
  - `Localize("排序")`
- [ ] 服务器列表行至少显示：
  - 服务器名
  - 地图名
  - 玩家数
  - 延迟
  - 当前筛选或排序提示

### 6.3 服务器行状态

- [ ] `server_row_*` 必须具备：
  - 默认态
  - hover 态
  - selected 态
  - pressed 态
  - disabled 态
- [ ] 选中态必须能明显看出当前关注的是哪一行
- [ ] hover 态和 selected 态不能长得几乎一样
- [ ] pressed 态必须给出即时反馈

**建议 DOM / class：**
- [ ] `list_entry server_row`
- [ ] `list_entry server_row is-hovered`
- [ ] `list_entry server_row is-selected`
- [ ] `list_entry server_row is-pressed`
- [ ] `list_entry server_row is-disabled`

### 6.4 服务器页次级操作收纳

- [ ] `servers_side_tools` 只放次级工具
- [ ] 不在这里放主任务摘要卡来补空白
- [ ] 不在这里复制右栏好友信息
- [ ] 小窗口下允许内部滚动

## 7. 右侧好友栏实现清单

### 7.1 右栏角色重定义

- [ ] 右栏明确命名为好友列表，不再叫 `Quick Actions` 或 `Command Deck`
- [ ] 右栏标题文案改为：
  - `Localize("好友列表")`
- [ ] 右栏说明文案只保留简短状态，不堆摘要卡

### 7.2 好友条目结构

- [ ] 每个好友条目必须包含：
  - 皮肤头像
  - 好友名字
  - 备注/说明
  - 在线状态
- [ ] 头像使用皮肤渲染结果，不再用纯色方块占位
- [ ] 备注允许显示短文本，不允许把条目高度撑爆
- [ ] 在线状态需要有图形提示和文字提示

**建议文案：**
- [ ] `Localize("在线")`
- [ ] `Localize("离线")`
- [ ] `Localize("备注")`

### 7.3 好友条目状态

- [ ] 好友条目必须具备：
  - hover
  - focus
  - pressed
  - active
  - disabled
- [ ] 头像区和文本区一起响应状态变化
- [ ] 当前选中好友要有明确 active 态
- [ ] hover 不能只作用于背景，不作用于头像和标题

**建议 DOM / class：**
- [ ] `friend_entry`
- [ ] `friend_entry is-hovered`
- [ ] `friend_entry is-focused`
- [ ] `friend_entry is-pressed`
- [ ] `friend_entry is-active`
- [ ] `friend_entry is-disabled`

### 7.4 右栏尺寸与滚动

- [ ] 右栏宽度固定到一档稳定范围，不跟中央主区互相挤压
- [ ] 好友列表内容超过高度时只在右栏内部滚动
- [ ] 不允许越界挤到中央主区
- [ ] 不允许把右栏做成多个不同职责的 stacked cards

## 8. 设置页实现清单

### 8.1 `data/qmclient/rmlui/settings_frontend.rml`

- [ ] 固定四区结构：
  - `settings_toolbar`
  - `settings_domain_rail`
  - `settings_workspace`
  - `settings_context_rail`
- [ ] `settings_workspace` 永远是主任务区
- [ ] `settings_context_rail` 只放 restart / reason / help / hint
- [ ] 首屏必须至少有一个真实 section card
- [ ] 禁止只剩标题和空白背景

### 8.2 设置页首屏主任务要求

- [ ] 页面标题：`Localize("设置")`
- [ ] 域导航至少包含：
  - `Localize("图像")`
  - `Localize("声音")`
  - `Localize("语言")`
  - `Localize("应用")`
- [ ] 主区必须至少出现一种真实控件：
  - toggle
  - enum
  - button row
  - text input
  - color input
- [ ] 高级设置默认折叠或下沉到二级区

### 8.3 设置项状态清单

- [ ] `setting_toggle_row` 具备 hover / focus / pressed / disabled
- [ ] `setting_enum_row` 具备 hover / focus / pressed / expanded / disabled
- [ ] `setting_button_row` 具备 hover / focus / pressed / disabled
- [ ] `setting_text_input_row` 具备 focus / error / disabled
- [ ] `setting_color_row` 具备 hover / focus / pressed / disabled
- [ ] `settings_modal_button` 具备 hover / focus / pressed / disabled

### 8.4 长表单与可达性

- [ ] 长表单必须滚动可达到底部
- [ ] 主操作按钮不能被 modal、输入态或提示遮挡
- [ ] restart banner 不得压住主控件
- [ ] 小窗口下 `settings_context_rail` 可滚动或折叠

## 9. 本地化实现清单

### 9.1 必须走 `Localize(...)` 的区域

- [ ] 左导航全部文本
- [ ] 顶部分段条全部文本
- [ ] 服务器页标题、按钮、区块标题
- [ ] 右栏好友标题、状态、备注标签
- [ ] 设置页标题、域标题、按钮、提示语
- [ ] modal 按钮和说明文案

### 9.2 禁止项

- [ ] 禁止新增裸英文 UI 字符串
- [ ] 禁止把参考 HTML 里的英文原样搬进 RML 成品
- [ ] 禁止中英混搭作为正式默认文案

### 9.3 首批建议文案清单

- [ ] `Localize("主页")`
- [ ] `Localize("服务器列表")`
- [ ] `Localize("发现")`
- [ ] `Localize("编辑器")`
- [ ] `Localize("录像")`
- [ ] `Localize("设置")`
- [ ] `Localize("游戏")`
- [ ] `Localize("地图")`
- [ ] `Localize("投票")`
- [ ] `Localize("玩家")`
- [ ] `Localize("服务器")`
- [ ] `Localize("排行")`
- [ ] `Localize("刷新")`
- [ ] `Localize("筛选")`
- [ ] `Localize("排序")`
- [ ] `Localize("好友列表")`
- [ ] `Localize("在线")`
- [ ] `Localize("离线")`
- [ ] `Localize("备注")`
- [ ] `Localize("图像")`
- [ ] `Localize("声音")`
- [ ] `Localize("语言")`
- [ ] `Localize("应用")`
- [ ] `Localize("重置")`

## 10. 交互反馈实现清单

- [ ] 所有主要按钮至少包含三种同时可见的反馈：
  - 底色变化
  - 边框或阴影变化
  - 文字或图标可读性变化
- [ ] focus ring 不能删除
- [ ] pressed 态不能只有逻辑变化
- [ ] active 态必须比 hover 态更稳定、更明确
- [ ] disabled 态不能只是不可点击，还要明显降级
- [ ] “能点但看起来没反应”一律视为未完成

### 10.1 建议统一状态类

- [ ] `is-hovered`
- [ ] `is-focused`
- [ ] `is-pressed`
- [ ] `is-active`
- [ ] `is-selected`
- [ ] `is-disabled`
- [ ] `is-open`
- [ ] `is-unavailable`
- [ ] `is-deferred`

## 11. 分辨率与纵横比兼容清单

### 11.1 必测分辨率

- [ ] `1366x768`
- [ ] `1920x1080`
- [ ] `2560x1440`
- [ ] `3840x2160`

### 11.2 必测纵横比

- [ ] `16:9`
- [ ] `5:4`
- [ ] `4:3`
- [ ] `21:9`

### 11.3 必测窗口态

- [ ] 标准桌面窗口
- [ ] 窄高窗口，例如 `1280x720`

### 11.4 不同视窗的具体要求

- [ ] `1366x768`：
  - 主区标题和主列表首屏同时可见
  - 右栏允许内部滚动
  - 顶部分段条次级区允许收紧
- [ ] `1920x1080`：
  - 五段结构完整可见
  - 右栏不与主区重叠
  - 主区首屏无大空洞
- [ ] `2560x1440`：
  - 不因为空间变大而把内容摊得过散
  - 主任务仍保持中心聚焦
- [ ] `3840x2160`：
  - 不允许单纯按像素硬拉大造成内容漂散
  - 需要维持合理最大内容宽度
- [ ] `5:4` / `4:3`：
  - 右栏和上下文栏优先内部滚动或折叠
  - 不允许把主区压成难用窄列
- [ ] `21:9`：
  - 不允许把中央主区变成过长稀薄条
  - 需要控制主内容最大宽度和信息聚合感
- [ ] 窄高窗口：
  - 先保证主任务区可读
  - 次级区可以折叠
  - 不允许靠隐藏主区“适配”

## 12. 验收截图清单

- [ ] `home`：
  - 左导航有 active 态
  - 顶部分段条有主次
  - 主区不是空白
  - 右栏不堆叠
- [ ] `serverbrowser`：
  - 三分区完整
  - 列表选中态可见
  - 刷新/筛选/排序至少一项反馈可见
- [ ] `settings`：
  - 四区结构完整
  - 至少一个 toggle / enum / modal action 状态可见
  - 主区不是空壳

### 12.1 必交截图矩阵

- [ ] 一张 `1366x768`
- [ ] 一张 `1920x1080`
- [ ] 一张 `2560x1440`
- [ ] 一张 `3840x2160`
- [ ] 一张 `5:4`
- [ ] 一张 `4:3`
- [ ] 一张 `21:9`
- [ ] 一张窄高窗口

## 13. 文件落点清单

- [ ] `data/qmclient/rmlui/main_menu_shell.rml`
- [ ] `data/qmclient/rmlui/main_menu_shell.rcss`
- [ ] `data/qmclient/rmlui/serverbrowser_page.rml`
- [ ] `data/qmclient/rmlui/serverbrowser_page.rcss`
- [ ] `data/qmclient/rmlui/settings_frontend.rml`
- [ ] `data/qmclient/rmlui/settings_frontend.rcss`
- [ ] `data/qmclient/rmlui/settings_modal.rml`
- [ ] `data/qmclient/rmlui/settings_modal.rcss`
- [ ] `src/game/client/RmlUi/RmlUiMainMenuShell.cpp`
- [ ] `src/game/client/RmlUi/RmlUiServerBrowserPage.cpp`
- [ ] `src/game/client/RmlUi/RmlUiSettingsFrontend.cpp`
- [ ] 相关 binder / model / test 文件

## 14. 完成标准

- [ ] 共享壳层已经接近 `closer-to-reference.html` 的结构收敛
- [ ] 右侧栏已经明确是好友列表，而不是混合摘要区
- [ ] 中央主区在 `home / serverbrowser / settings` 下都不再空
- [ ] 所有主交互都有显式反馈
- [ ] 所有用户可见文本都已走 `Localize(...)`
- [ ] `2k / 4k / 5:4 / 4:3 / 21:9 / 窄高窗口` 均有截图验收
