# RmlUI 主菜单 / 服务器列表 / 设置页替代实现计划

> **状态（2026-05-17）：** 已停止推进。当前工作树不再继续实现 RmlUI 正式代码，本计划仅保留历史记录，不再作为执行计划。

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 把 `main menu`、`serverbrowser`、`settings` 收口为同一条 RmlUI 前端替代主线，停止“过渡壳 + 局部占位”的表述，转入真实前端重建与旧 UI 内核解耦。

**架构：** `Main menu shell` 作为共享 RmlUI 菜单壳层，承载一级导航、顶部工具条、右侧社交栏和底部状态条。`Serverbrowser` 与 `Settings` 都作为 shell 内的一级主页面进入中央工作区；其中 `Settings` 再拆成语义层和 frontend shell 两层，旧 `CMenus` / `menus_settings*.cpp` 只保留数据、动作、配置和功能内核，不再承担页面 owner。

**技术栈：** C++、RmlUI v6.2、CMake、GTest、QmClient 现有 `CMenus` / `menus_browser.cpp` / `menus_settings*.cpp` / `RmlUiRuntime`。

> **回填说明（2026-05-17）：** 以下勾选已按当前仓库代码、测试文件和 `docs/superpowers/roadmap/2026-05-16-rmlui验收记录.md` 回填。只能从当前事实直接证明的结果会勾选；像“先写失败测试”“当时单次人工验证”“当时单独 commit”这类过程步骤，若仓库无法反证，就不倒推勾选。

> **当前状态修正（2026-05-17）：** 当前运行态截图仍不满足目标壳层质量。已出现的明确问题包括：中央主区大面积空白、右侧快捷卡片与好友栏挤压堆叠、首屏主任务信息密度过低、视觉焦点落到次要区块。因此，本计划中已勾选的结构步骤不等于“UI 壳已完成”；`closer-to-reference.html` 的 1:1 壳层复刻、布局收敛和 release 视觉验收仍未完成。

> **量化进度（2026-05-17，代码与测试已刷新）：**
> - 三份 settings 相关 spec：`3 / 3` 已完成，`100%`
> - 本计划勾选进度：`23 / 46`，`50.0%`
> - 当前验证状态：`run_cxx_tests = 804 / 804 PASS`，`game-client` 构建通过
> - 当前已落地但尚未回填为完成的实现：`main_menu_shell.rml/.rcss` 壳层收口、`serverbrowser_page.rml/.rcss` 三分区主任务布局、主菜单/服务器页/settings 前端可见文案本地化补强
> - 距离本计划 `100%` 仍缺：release 运行验收、`home/serverbrowser/settings` 截图矩阵、editor 后续入口拆分、文档事实回写

> **冲到 100% 的下一批任务（按优先级）：**
> 1. 完成 `home / serverbrowser / settings` 三页的运行态视觉验收
> 2. 补齐 `1366x768 / 1920x1080 / 2560x1440 / 3840x2160 / 5:4 / 4:3 / 21:9 / 窄高窗口` 截图
> 3. 逐项回填当前已经落地的 main menu / serverbrowser / settings frontend 实现到计划勾选项
> 4. 回写 current-state / global-ui-stack 文档事实源
> 5. 拆出 editor / map editor 的下一轮实现计划入口

---

## 前置约束

- 这份计划建立在“RmlUI 是唯一 UI 前端目标”的产品口径上。
- 不允许在文档、实现或测试里再引入 settings fallback、legacy owner、dual-stack surface 之类概念。
- 旧 UI 保留的边界只有：配置、数据、命令、功能逻辑、游戏世界渲染核心。
- `serverbrowser` 是 `main menu` 内的一级主页面，不单独升格为另一套 UI 体系。
- `settings` 与 `serverbrowser` 共用同一套 main menu shell，但 `settings` 自己拥有独立 frontend owner 和 semantic surface。
- 后续 editor / map editor UI 也按同样原则规划：保留功能内核，重建 RmlUI 前端。

## 视觉与交互硬约束（2026-05-17 补充）

- 共享壳层的视觉基线以 `.superpowers/brainstorm/rmlui-1778846115/content/closer-to-reference.html` 为准；当前实现不得再以“占位 shell 已存在”为由偏离该结构。
- 必须遵循 `$ui-ux-pro-max` 的高优先级规则，优先满足：信息层级、布局响应、表单可达性、主任务可见性、次要操作收纳和交互不遮挡。
- 必须显式落实 `$ui-ux-pro-max` 中的交互反馈规则，不能只借用布局口号却缺失按钮/控件状态反馈。
- 首屏必须优先展示当前页面的主任务内容；不允许出现“中央主区大面积空白，而次要卡片占据视线”的布局。
- 壳层结构固定为：左导航、顶部分段条、中部主内容、右好友栏、底部状态条；不得把主内容重新收缩成一个小角落卡片。
- 关键操作按钮不能被输入态、弹层、临时面板或右侧浮动工具遮挡。
- 高级筛选、次要操作和低频入口默认进入次级区域、弹层或折叠区，不得淹没主内容。
- 日期、时间和其他结构化字段默认使用专用控件，不以自由文本输入作为主路径。
- 所有长内容页必须可滚动到末尾，且滚动后主操作仍可定位；不得依赖超高静态画布去“留出空间”。
- 右侧好友栏和侧边工具列必须在各自容器内完成裁切、滚动或折叠；宁可内部滚动，也不能发生越界堆叠。
- 需要覆盖至少三类视窗兼容性：宽屏桌面（如 1920x1080）、常见桌面（如 1366x768）和窄高窗口（如 1280x720 或同等比例窗口化）。
- 需要覆盖至少两类纵横比：16:9 和更窄的窗口比例，确保右栏不会因为宽度不足而挤压主区。
- 窗口缩放时必须优先保持主任务区可读，不能靠放大整体字号或隐藏核心内容来“假装适配”。
- 在小窗口下，右侧好友栏、侧工具列和顶部外链区必须允许内部滚动或折叠，不能跨容器溢出。
- 验收截图必须包含窗口化状态，不只测全屏；至少要有一张窄高窗口截图和一张标准桌面宽高截图。
- 所有一级和二级按钮都必须具备可见的 `hover` / `focus` / `pressed` / `active` / `disabled` 状态，且状态差异不能只靠极细微的颜色变化。
- 主要 CTA、导航按钮、列表行和浮动工具按钮必须提供按压反馈；当前“看起来像按钮但点击无明显反馈”的状态视为未完成。
- 键盘焦点必须可见，不能移除 focus ring；RmlUI 中的可交互元素需要稳定的焦点高亮和可预测的 Tab 顺序。
- 交互目标应满足最小点击区，优先按 `44x44` 口径设计；如果视觉按钮较小，命中区域也必须补足。
- 不允许把交互反馈只留给逻辑层处理而样式层无表现；按钮反馈必须在 RCSS / DOM class 切换层可见。
- 计划中具体要补的状态类至少包括：`is-hovered`、`is-focused`、`is-pressed`、`is-active`、`is-selected`、`is-disabled`、`is-open`、`is-unavailable`、`is-deferred`。
- 计划中具体要补的结构类至少包括：`panel`、`panel_title`、`panel_body`、`chip`、`nav_button`、`pill_button`、`list_entry`、`tool_tile`、`friend_entry`、`status_block` 的状态变体。
- 主要按钮和列表项的反馈必须同时包含：视觉高亮、边框或阴影变化、可读性不降级的文本变化；不能只改一个很浅的底色。
- 需要在 plan 中显式约束 DOM 类切换点：hover / focus / pressed 不能只由逻辑变量存在，而必须映射到可测的 class 或状态属性。
- 当前截图里的以下现象明确视为不达标：
  - 中央主区巨大空洞，未承载首屏主任务
  - 右侧 `Quick Actions` / `Command Deck` 与好友栏彼此挤压
  - 好友栏、浮动工具和顶部外链区同时争抢边缘空间
  - 整体视觉焦点落在次级摘要卡，而不是当前主页面
  - 一级按钮、导航按钮和动作按钮缺少明显交互反馈
- 当前截图验收时还要额外检查：
  - 左侧导航每个条目都有明确 active / hover / pressed 区分
  - 顶部分段条与顶部外链区在视觉上有明确主次层级
  - 中央主区的首屏主标题、说明和主 CTA 同时可见，不被空白吞没
  - 右栏内容在 1080p 下不与主区发生重叠或挤压越界
  - 按钮在鼠标悬停和点击后能看出“发生了什么”
  - 窄高窗口下右栏和主区仍保持可达，不能靠隐藏关键区域“适配”

## 实现清单（压缩版）

- [ ] 右侧栏改成真正的好友列表，条目必须包含：皮肤头像、好友名字、备注/说明、在线状态或提示徽标。
- [ ] 右侧好友条目必须支持 `active` / `hover` / `pressed` / `focus` / `disabled` 状态，头像与文字一起响应。
- [ ] 所有用户可见文案必须走 `Localize(...)`，禁止裸英文字符串；例如 `Localize("服务器列表")`、`Localize("好友列表")`、`Localize("设置")`、`Localize("服务器")`、`Localize("地图")`、`Localize("刷新")`。
- [ ] 左导航保持中文本地化标签，且每项都必须有清晰的选中与悬停反馈。
- [ ] 顶部分段条保留一级模式切换，外链/社区按钮降级为次级操作。
- [ ] 中央主区必须填满真实内容，不允许首屏大块空白。
- [ ] 服务器列表必须显示：服务器名、地图名、人数、延迟、筛选/排序状态，且行点击有反馈。
- [ ] 服务器页右侧工具列只能放次级操作，不得压过主内容区。
- [ ] 1920x1080 与 1366x768 两档截图都要通过：无堆叠、无遮挡、无空白主区、无越界。
- [ ] `hover` / `focus` / `pressed` / `active` 的状态必须能从 DOM class / attribute 直接验证。
- [ ] 所有按钮都必须有明显按压反馈，不接受“能点但像没点”的交互。

## 文件结构

- 修改：`docs/superpowers/specs/2026-05-15-rmlui-settings-dual-stack-reset-design.md`
  - 作为 settings 单一前端重置的设计事实源。
- 修改：`docs/superpowers/specs/2026-05-15-rmlui-settings-semantic-surface-design.md`
  - 作为 settings 语义层设计事实源。
- 修改：`docs/superpowers/specs/2026-05-15-rmlui-settings-frontend-shell-design.md`
  - 作为 settings frontend shell 设计事实源。
- 创建或继续修改：`src/game/client/RmlUi/RmlUiMainMenuShell.h`
  - 定义共享菜单壳层 route 与 view model。
- 创建或继续修改：`src/game/client/RmlUi/RmlUiMainMenuShell.cpp`
  - 实现共享菜单壳层 document 绑定与工作区分发。
- 创建或继续修改：`src/game/client/RmlUi/RmlUiServerBrowserPage.h`
  - 定义 serverbrowser 页面模型与绑定入口。
- 创建或继续修改：`src/game/client/RmlUi/RmlUiServerBrowserPage.cpp`
  - 实现 serverbrowser 页面 document 绑定。
- 创建：`src/game/client/RmlUi/RmlUiSettingsSemanticSurface.h`
  - 定义 settings route / page model / action / result 语义接口。
- 创建：`src/game/client/RmlUi/RmlUiSettingsSemanticSurface.cpp`
  - 实现 settings 语义模型构建与动作写回适配。
- 创建或继续修改：`src/game/client/RmlUi/RmlUiSettingsFrontend.h`
  - 定义 settings page / modal frontend owner。
- 创建或继续修改：`src/game/client/RmlUi/RmlUiSettingsFrontend.cpp`
  - 实现 settings frontend document、DOM 绑定与 action 分发。
- 修改：`src/game/client/RmlUi/RmlUiRuntime.h`
  - 接入共享 shell、serverbrowser page、settings frontend owner。
- 修改：`src/game/client/RmlUi/RmlUiRuntime.cpp`
  - 调整 page / modal 生命周期与 surface 分发。
- 修改：`src/game/client/RmlUi/RmlUiSurface.h`
  - 明确 `MAIN_MENU`、`SERVERBROWSER`、`SETTINGS_PAGE`、`SETTINGS_MODAL` 的角色。
- 修改：`src/game/client/components/menus.h`
  - 暴露 shell route、serverbrowser 数据、settings 语义与动作入口。
- 修改：`src/game/client/components/menus.cpp`
  - 收口菜单页分发和 settings 宿主职责。
- 修改：`src/game/client/components/menus_browser.cpp`
  - 抽出 serverbrowser 只读模型装配。
- 修改：`src/game/client/components/menus_settings.cpp`
  - 抽出 settings route、section、restart、action 相关语义。
- 按需修改：`src/game/client/components/menus_settings7.cpp`
  - 抽出 tee / 7.x 相关只读语义。
- 按需修改：`src/game/client/components/menus_settings_assets.cpp`
  - 抽出资源类页面语义摘要。
- 按需修改：`src/game/client/components/tclient/menus_tclient.cpp`
  - 抽出功能型设置语义。
- 按需修改：`src/game/client/components/qmclient/menus_qmclient.cpp`
  - 抽出 QmClient 模块页语义。
- 修改：`data/qmclient/rmlui/main_menu_shell.rml`
  - 作为共享壳层事实源。
- 修改：`data/qmclient/rmlui/main_menu_shell.rcss`
  - 作为共享壳层样式事实源。
- 修改：`data/qmclient/rmlui/serverbrowser_page.rml`
  - 作为 serverbrowser 页面结构事实源。
- 修改：`data/qmclient/rmlui/serverbrowser_page.rcss`
  - 作为 serverbrowser 页面样式事实源。
- 创建：`data/qmclient/rmlui/settings_frontend.rml`
  - 作为 settings page 结构事实源。
- 创建：`data/qmclient/rmlui/settings_frontend.rcss`
  - 作为 settings page 样式事实源。
- 创建：`data/qmclient/rmlui/settings_modal.rml`
  - 作为 settings modal 结构事实源。
- 创建：`data/qmclient/rmlui/settings_modal.rcss`
  - 作为 settings modal 样式事实源。
- 修改：`CMakeLists.txt`
  - 注册新增实现与测试。
- 创建或继续修改：`src/test/rmlui_main_menu_shell_test.cpp`
  - 覆盖共享 shell route 与状态模型。
- 创建或继续修改：`src/test/rmlui_serverbrowser_page_test.cpp`
  - 覆盖 serverbrowser 页面模型与绑定。
- 创建：`src/test/rmlui_settings_semantic_surface_test.cpp`
  - 覆盖 settings 语义层。
- 创建或继续修改：`src/test/rmlui_settings_frontend_test.cpp`
  - 覆盖 settings frontend DOM 绑定与 action 闭环。

## 任务 1：统一主线口径，清理“骨架 / 过渡 / 双路径”事实源

**文件：**
- 修改：`docs/superpowers/specs/2026-05-15-rmlui-settings-dual-stack-reset-design.md`
- 修改：`docs/superpowers/specs/2026-05-15-rmlui-settings-semantic-surface-design.md`
- 修改：`docs/superpowers/specs/2026-05-15-rmlui-settings-frontend-shell-design.md`
- 修改：`docs/superpowers/plans/2026-05-15-rmlui-main-menu-serverbrowser-settings-skeleton.md`

- [x] **步骤 1：重写 settings 相关 spec**

要求：

- 去掉 `fallback`、`dual-stack`、`legacy-only`、`mixed host` 等口径。
- 明确 RmlUI 是唯一前端目标。
- 明确旧 UI 只保留内核能力。
- 明确未来 editor / map editor 也走同样路线。

- [x] **步骤 2：重写实现计划头部和文件结构**

要求：

- 不再把 `settings skeleton` 当 active owner。
- 改成面向真实替代工作的执行计划。

- [x] **步骤 3：扫描残留词汇**

运行：

```powershell
rg -n "fallback|dual-stack|legacy-only|mixed host|并行|切回旧 UI" docs\superpowers\specs docs\superpowers\plans
```

预期：本计划与三份 settings spec 不再残留这些旧口径。

## 任务 2：收紧共享 main menu shell，使它成为后续页面替代的唯一菜单宿主

**文件：**
- 修改：`src/game/client/RmlUi/RmlUiMainMenuShell.h`
- 修改：`src/game/client/RmlUi/RmlUiMainMenuShell.cpp`
- 修改：`data/qmclient/rmlui/main_menu_shell.rml`
- 修改：`data/qmclient/rmlui/main_menu_shell.rcss`
- 修改：`src/test/rmlui_main_menu_shell_test.cpp`

- [ ] **步骤 1：补失败测试，锁定 shell 的长期职责**

在 `src/test/rmlui_main_menu_shell_test.cpp` 增加：

```cpp
TEST(RmlUiMainMenuShell, ServerbrowserAndSettingsRemainPrimaryPages);
TEST(RmlUiMainMenuShell, FriendsRailAndStatusBarPersistAcrossPageSwitches);
TEST(RmlUiMainMenuShell, ShellDoesNotTreatSettingsAsEmbeddedFragment);
```

- [ ] **步骤 2：运行测试验证失败**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：新测试失败，提示 shell model 或 route 约束未满足。

- [x] **步骤 3：收口 shell model**

在 `RmlUiMainMenuShell.h` 明确：

```cpp
enum class ERmlUiMainMenuPage
{
	HOME,
	SERVERBROWSER,
	SETTINGS,
};

struct SRmlUiMainMenuShellModel
{
	ERmlUiMainMenuPage m_Page;
	bool m_ShowFriendsRail;
	bool m_ShowBottomStatusBar;
};
```

- [x] **步骤 4：实现最小壳层绑定**

在 `RmlUiMainMenuShell.cpp` 让 shell 只负责：

- 一级页切换
- 顶部工具条
- 右侧好友栏
- 底部状态条
- 中央工作区宿主

禁止在 shell 内部再写 settings 内容拼接逻辑。

补充布局要求：

- 左导航保持稳定宽度，优先作为一级入口，不随中央内容挤压变形。
- 顶部分段条优先承载一级模式切换；外链或社区按钮属于次要操作，不得反过来压缩一级模式区。
- 中央主区一旦存在页面模型，首屏就必须用真实内容填充，不能继续保留大块空白背景。
- 右好友栏保持稳定宽度和独立滚动，不得让好友卡片、快捷卡片或浮动按钮溢出到壳外。
- 底部状态条只承载状态摘要，不承担主内容补偿职责。
- 壳层内所有可点击元素至少要有两层反馈：静态可识别态 + 交互态；不能停留在“文字可点，但无 hover / pressed 变化”的低完成度状态。
- 左导航、顶部分段条、外链按钮和右侧浮动工具都要共享一致的反馈语言，避免有的像按钮、有的像纯文本标签。
- 具体需要检查的交互状态包含：当前页 active、鼠标 hover、键盘 focus、鼠标按压 pressed、不可用 disabled、展开 open。
- 左导航 item、顶部模式 button、外链 button、右侧 friend entry、float tool 都要使用统一的按压时长和反馈强度，不允许一部分“有反馈”一部分“像死按钮”。

- [x] **步骤 5：重新运行测试验证通过**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：`RmlUiMainMenuShell.*` 新增测试 PASS。

## 任务 3：完成 serverbrowser 页面从 placeholder 到真实 RmlUI 页面模型的收口

**文件：**
- 修改：`src/game/client/RmlUi/RmlUiServerBrowserPage.h`
- 修改：`src/game/client/RmlUi/RmlUiServerBrowserPage.cpp`
- 修改：`src/game/client/components/menus_browser.cpp`
- 修改：`data/qmclient/rmlui/serverbrowser_page.rml`
- 修改：`data/qmclient/rmlui/serverbrowser_page.rcss`
- 修改：`src/test/rmlui_serverbrowser_page_test.cpp`

- [ ] **步骤 1：补失败测试，锁定真实页面分区**

在 `src/test/rmlui_serverbrowser_page_test.cpp` 增加：

```cpp
TEST(RmlUiServerBrowserPage, BuildsServerRowsFromBrowserState);
TEST(RmlUiServerBrowserPage, BuildsSelectedServerSummaryAndMapCards);
TEST(RmlUiServerBrowserPage, DoesNotDependOnLegacyRendererCalls);
```

- [ ] **步骤 2：运行测试验证失败**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：新测试失败，提示 page model 缺失或仍依赖旧路径。

- [x] **步骤 3：在 `menus_browser.cpp` 抽只读模型**

提供最小入口，例如：

```cpp
bool BuildRmlUiServerBrowserModel(SRmlUiServerBrowserPageModel *pModel) const;
```

要求：

- 只读装配
- 不直接绘制
- 不持有旧页面 DOM / 渲染状态

- [x] **步骤 4：在 `RmlUiServerBrowserPage.cpp` 绑定真实页面**

至少绑定：

- server rows
- selected server summary
- map overview
- filter summary

补充页面要求：

- `serverbrowser` 首屏必须把“选服”作为主任务，中央主区优先展示服务器信息、地图信息和排名信息，不得退化为大面积空白占位。
- 高级筛选与次要动作默认进入右下或下半区，不得抢占首屏主信息区。
- 页面信息密度应向 `closer-to-reference.html` 靠拢，形成稳定的主内容三分区，而不是“左大空区 + 右小卡片”。
- 服务器列表行、刷新、排序、筛选和地图/更多信息按钮都必须给出清晰交互反馈，至少覆盖 hover、selected、pressed、disabled 四类状态。
- `server_row_*`、`server_action_refresh`、`server_action_cycle_sort`、`server_action_reset_filter` 都要在 plan 中被视为必须有状态反馈的交互锚点。
- `servers_footer_strip` 必须保留，但只能是摘要，不得被拿来补偿主区信息不足。
- 右侧 `servers_side_tools` 只允许承载次级工具，不允许在视觉上抢过 `servers_main_column`。

- [x] **步骤 5：重新运行测试验证通过**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：`RmlUiServerBrowserPage.*` 测试 PASS。

## 任务 4：建立 settings semantic surface，先锁住 route / page model / action 闭环

**文件：**
- 创建：`src/game/client/RmlUi/RmlUiSettingsSemanticSurface.h`
- 创建：`src/game/client/RmlUi/RmlUiSettingsSemanticSurface.cpp`
- 修改：`src/game/client/components/menus.h`
- 修改：`src/game/client/components/menus.cpp`
- 修改：`src/game/client/components/menus_settings.cpp`
- 按需修改：`src/game/client/components/menus_settings7.cpp`
- 按需修改：`src/game/client/components/menus_settings_assets.cpp`
- 按需修改：`src/game/client/components/tclient/menus_tclient.cpp`
- 按需修改：`src/game/client/components/qmclient/menus_qmclient.cpp`
- 创建：`src/test/rmlui_settings_semantic_surface_test.cpp`

- [ ] **步骤 1：先写失败测试**

在 `src/test/rmlui_settings_semantic_surface_test.cpp` 写入：

```cpp
TEST(RmlUiSettingsSemanticSurface, BuildsCoreDomainPageModels);
TEST(RmlUiSettingsSemanticSurface, ReturnsDeferredReasonForUnrebuiltDomains);
TEST(RmlUiSettingsSemanticSurface, ConsumesActionsWithoutCallingLegacyRenderer);
```

- [ ] **步骤 2：运行测试验证失败**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：构建失败，提示缺少 semantic surface 类型或接口。

- [x] **步骤 3：声明最小语义接口**

在 `RmlUiSettingsSemanticSurface.h` 定义：

```cpp
bool BuildRmlUiSettingsPageModel(const SRmlUiSettingsRoute &Route, SRmlUiSettingsPageModel *pModel);
bool ConsumeRmlUiSettingsAction(const SRmlUiSettingsAction &Action, SRmlUiSettingsActionResult *pResult);
bool QueryRmlUiSettingsRouteTree(std::vector<SRmlUiSettingsRoute> *pRoutes);
```

- [x] **步骤 4：实现首批 core domains**

优先接通：

- `HUD_AND_LANGUAGE`
- `GRAPHICS`
- `SOUND`

要求：

- 提供 page model
- 提供 restart state
- 提供最小 action 写回

- [x] **步骤 5：对复杂域返回 deferred 语义**

对 `TEE`、`RESOURCES`、`FEATURES`、`CONFIGURATION` 中尚未重建完成的 route 返回稳定 reason，例如：

```text
domain_not_yet_rebuilt
complex_editor_page_deferred
```

- [x] **步骤 6：重新运行测试验证通过**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：`RmlUiSettingsSemanticSurface.*` 测试 PASS。

## 任务 5：建立 settings frontend owner，完成 page / modal 的真实 RmlUI 宿主

**文件：**
- 创建或修改：`src/game/client/RmlUi/RmlUiSettingsFrontend.h`
- 创建或修改：`src/game/client/RmlUi/RmlUiSettingsFrontend.cpp`
- 修改：`src/game/client/RmlUi/RmlUiRuntime.h`
- 修改：`src/game/client/RmlUi/RmlUiRuntime.cpp`
- 修改：`src/game/client/RmlUi/RmlUiSurface.h`
- 创建：`data/qmclient/rmlui/settings_frontend.rml`
- 创建：`data/qmclient/rmlui/settings_frontend.rcss`
- 创建：`data/qmclient/rmlui/settings_modal.rml`
- 创建：`data/qmclient/rmlui/settings_modal.rcss`
- 创建或修改：`src/test/rmlui_settings_frontend_test.cpp`

- [ ] **步骤 1：先写失败测试**

在 `src/test/rmlui_settings_frontend_test.cpp` 写入：

```cpp
TEST(RmlUiSettingsFrontend, BindsRequiredDomAnchors);
TEST(RmlUiSettingsFrontend, RendersCoreDomainPageModels);
TEST(RmlUiSettingsFrontend, KeepsModalLifecycleSeparateFromPageState);
```

- [ ] **步骤 2：运行测试验证失败**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：构建失败，提示缺少 frontend owner、RML 资源或 DOM 绑定实现。

- [x] **步骤 3：定义 page / modal 前端接口**

在 `RmlUiSettingsFrontend.h` 定义：

```cpp
class CRmlUiSettingsFrontend
{
public:
	bool RenderPage(const SRmlUiSettingsPageModel &Model);
	bool RenderModal(const SRmlUiSettingsModalModel &Model);
	bool HandleAction(const SRmlUiSettingsAction &Action);
};
```

- [x] **步骤 4：编写 RML / RCSS 资源**

`settings_frontend.rml` 至少包含：

```rml
<body id="settings_frontend_root">
  <nav id="settings_domain_rail"></nav>
  <header id="settings_toolbar"></header>
  <main id="settings_workspace"></main>
  <aside id="settings_context_rail"></aside>
</body>
```

`settings_modal.rml` 至少包含：

```rml
<body id="settings_modal_root">
  <header id="settings_modal_title"></header>
  <section id="settings_modal_body"></section>
  <footer id="settings_modal_actions"></footer>
</body>
```

- [x] **步骤 5：绑定 semantic surface 输出**

要求：

- page 渲染只消费 `SRmlUiSettingsPageModel`
- modal 只消费 `SRmlUiSettingsModalModel`
- action 经由 semantic surface 闭环回写

补充布局要求：

- settings 需要沿用共享壳层的主内容优先原则，首屏优先显示当前 route 的核心设置控件，而不是说明性占位文本。
- `settings_domain_rail`、`settings_workspace`、`settings_context_rail` 必须形成清晰主次关系：工作区为主，右侧上下文区为辅。
- 高级设置、低频辅助动作和说明文本默认放进次级区、折叠区或 modal，不得把主设置区挤成窄列。
- 长表单必须可滚动到底，restart 提示和关键动作要保持可定位。
- settings 内 toggle、enum、button row、text input row、modal action button 必须具备显式状态反馈，至少覆盖 hover、focus、pressed、disabled、selected。
- 若某个动作已经触发写回或打开 modal，界面必须立即给出视觉反馈，不能出现“按钮逻辑触发了，但界面像没点到”的体验。

- [x] **步骤 6：重新运行测试验证通过**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：`RmlUiSettingsFrontend.*` 测试 PASS。

## 任务 6：接入 runtime 与 surface 分发，确保 settings 始终停留在 RmlUI 文档体系内

**文件：**
- 修改：`src/game/client/RmlUi/RmlUiRuntime.h`
- 修改：`src/game/client/RmlUi/RmlUiRuntime.cpp`
- 修改：`src/game/client/RmlUi/RmlUiSurface.h`
- 修改：`src/game/client/components/menus.cpp`

- [x] **步骤 1：补失败测试或现有断言**

如果现有测试基础允许，在相关测试中增加：

```cpp
EXPECT_TRUE(RenderSettingsUsesRmlUiFrontend());
EXPECT_FALSE(RenderSettingsDelegatesToLegacyContentOwner());
```

- [x] **步骤 2：接入 settings page / modal owner**

要求：

- `PAGE_SETTINGS` 进入 settings frontend page
- settings 相关 modal 进入 settings frontend modal
- runtime 生命周期显式成对出现

- [x] **步骤 3：禁止 settings 重新退回旧页面宿主**

要求：

- 移除旧“settings workspace fragment 即主线”的假设
- 不允许在 RmlUI path 下再次混入 `RenderSettings*` 页面内容

- [x] **步骤 4：运行测试验证通过**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：相关测试全部 PASS。

## 任务 7：Release 构建、真实运行验证与 editor UI 后续入口

**文件：**
- 修改：`docs/superpowers/specs/2026-05-15-rmlui-global-ui-stack-design.md`
- 按需修改：`docs/superpowers/specs/2026-05-06-rmlui-qmclient-current-state.md`
- 如有必要创建：后续 editor / map editor UI 规划文档

- [x] **步骤 1：跑 C++ 测试**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：新增 shell / serverbrowser / settings semantic / settings frontend 测试全部 PASS。

- [x] **步骤 2：跑主目标构建**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30
```

预期：`game-client` 构建通过。

- [ ] **步骤 3：做 Release 运行验证**

运行：

```powershell
cd build-ninja
.\DDNet.exe
```

预期：

- `rmlui` 栈下主菜单进入真实 shared shell。
- `serverbrowser` 进入真实页面而不是统一 placeholder。
- `settings` 始终停留在 RmlUI 文档体系内。
- 已接通的 core domains 可以真实展示。
- 未完成域在 RmlUI 内部明确显示 deferred reason。
- `1920x1080` 首屏下不再出现中央大面积空白。
- 右侧好友栏、快捷卡片和浮动工具不发生越界堆叠。
- 首屏视觉焦点稳定落在主任务区，而不是摘要卡或边缘栏。
- 次要入口即使保留，也不会压缩主任务空间到失衡状态。
- 一级导航、顶部分段条、主要 CTA、服务器列表行、settings 控件都具备明确交互反馈，不再出现“按钮没有反馈”的完成度缺口。
- 截图验收要同时看三张固定图：`home`、`serverbrowser`、`settings`，分别确认主区填充、按钮反馈、右栏不堆叠。
- 截图里必须能一眼看出当前 active page、当前 primary CTA、当前 hover 或 active item，而不是只看到静态壳子。
- `settings` 的截图验收还要确认至少一个 toggle / enum / modal action 的状态变化可见。

- [ ] **步骤 4：回写当前事实文档**

把 current-state / global-ui-stack 文档更新成：

```text
main menu shell、serverbrowser、settings 已进入 RmlUI 前端替代主线；
旧 UI 仅保留内核能力，不再作为 settings 并行前端或 fallback 方案。
```

- [ ] **步骤 5：拆下一轮 editor UI 计划入口**

创建后续计划时至少列出：

```text
map editor / resource editor / complex capture pages
延续“保留功能内核，重建 RmlUI 前端”的统一原则
```

补充执行清单：

- 具体实现项见 `docs/superpowers/plans/checklist/2026-05-17-rmlui-shell-实现清单.md`
