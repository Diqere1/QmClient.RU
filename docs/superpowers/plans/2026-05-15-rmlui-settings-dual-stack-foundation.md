# RmlUI Settings 单一前端基础层实现计划

> **状态（2026-05-17）：** 已停止推进。当前工作树不再继续实现 RmlUI 正式代码，本计划仅保留历史记录，不再作为执行计划。

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 在当前 `qm_ui_stack` 全局栈基线之上，为 `settings` 落地首个可执行的单一前端基础层：先切断 mixed host，再抽出可消费的 settings 语义接口，最后建立独立的 RmlUI settings frontend 壳。

**架构：** 这份计划只覆盖 settings 主线前三步：`owner reset`、`semantic surface`、`frontend shell`。`CMenus::RenderSettings(...)` 退化为宿主 seam，通过新的 settings path selector 只选择当前 active settings path；`RmlUiSettingsSemanticSurface` 提供 route/page/action 语义；`RmlUiSettingsFrontend` 持有独立 page/modal 文档并消费该语义层。`core domains 1:1` 页面实现不在本计划内，它将以这份基础层为前提另起计划。

**技术栈：** C++、RmlUI v6.2、CMake、GTest、QmClient 现有 `CMenus` / `menus_settings*.cpp` / `menus_tclient.cpp` / `menus_qmclient.cpp` 状态与配置源。

> **回填说明（2026-05-17）：** 以下勾选已按当前仓库代码、测试文件和 `docs/superpowers/roadmap/2026-05-16-rmlui验收记录.md` 回填。只能从当前事实直接证明的结果会勾选；像“先写失败测试”“当时单次人工验证”“当时单独 commit”这类过程步骤，若仓库无法反证，就不倒推勾选。

> **当前状态修正（2026-05-17）：** 当前 settings frontend 只证明了结构和接口已经接通，不代表页面壳层已经达标。结合最新运行态截图，当前 shared shell / settings shell 仍存在主内容承载不足、上下文区侵占视线和布局密度失衡的问题，因此本计划的“frontend shell 已接通”不能解读为“settings 壳已完成”。

> **量化进度（2026-05-17，代码与测试已刷新）：**
> - 三份 settings 相关 spec：`3 / 3` 已完成，`100%`
> - 本计划勾选进度：`20 / 36`，`55.6%`
> - 当前验证状态：`run_cxx_tests = 804 / 804 PASS`，`game-client` 构建通过
> - 距离本计划 `100%` 仍缺：独立前端人工验收、视窗/纵横比截图矩阵、最终验证、最终人工验收、过程性 commit 收口

> **冲到 100% 的下一批任务（按优先级）：**
> 1. 完成 `settings_frontend` 的人工运行验收，覆盖主区优先、上下文区不挤压、控件反馈和 modal 行为
> 2. 按 `2k / 4k / 5:4 / 4:3 / 21:9 / 窄高窗口` 补齐 settings 验收截图
> 3. 运行最终验证并把结果回填到本计划对应步骤
> 4. 完成 `RmlUiSettingsPage.*` 历史兼容边界的最终人工确认
> 5. 在上述验证收口后再处理计划里的过程性 commit 步骤

---

## 前置约束

- 这份计划建立在以下 spec 已完成的前提上：
  - `docs/superpowers/specs/2026-05-15-rmlui-settings-dual-stack-reset-design.md`
  - `docs/superpowers/specs/2026-05-15-rmlui-settings-semantic-surface-design.md`
  - `docs/superpowers/specs/2026-05-15-rmlui-settings-frontend-shell-design.md`
- 当前 `docs/superpowers/plans/2026-05-15-rmlui-main-menu-serverbrowser-settings-skeleton.md` 中的 settings 骨架只当历史过渡壳，不再定义 settings 主线完成度。
- `settings` 的主线口径已经是 `settings frontend + semantic surface`；本计划里出现的 path selector 只用于描述当前 active 宿主选择，不再把 `settings` 表述成并行前端设计。
- 不允许同一帧同时渲染 active settings RmlUI 页面和 legacy settings 内容。
- `settings page` 与 `settings modal` 必须从计划一开始就按两条独立 surface 对待；即使初期仍复用同一 runtime，也不得在语义和测试层把 modal 混回 page。
- Windows 构建与测试必须使用 `qmclient_scripts\cmake-windows.cmd`。

## 视觉与交互硬约束（2026-05-17 补充）

- `settings frontend` 的视觉目标继续服从 `.superpowers/brainstorm/rmlui-1778846115/content/closer-to-reference.html` 的共享壳基线，而不是停留在孤立的占位式设置页。
- 必须遵循 `$ui-ux-pro-max` 中关于主任务优先、响应式布局、长表单可达、交互不遮挡和次要操作收纳的高优先级规则。
- 必须把 `$ui-ux-pro-max` 的交互反馈要求落成 settings frontend 的显式验收项，不能只满足结构接通。
- `settings_workspace` 是首屏主内容区，必须优先承载当前 route 的核心设置控件，不得让说明文案或上下文卡片反客为主。
- `settings_context_rail` 只承载 restart、帮助、availability reason 和次级说明；不允许它把主设置区挤压成难以使用的窄列。
- 高级设置和低频动作默认折叠到二级区、展开区或 modal；不能在首屏把主要设置区挤满零散卡片。
- 所有长表单必须确保滚动到底仍可完成操作；关键确认按钮和 restart 提示不得被输入态或临时面板遮挡。
- 日期、时间、枚举、颜色等结构化字段优先用专用控件，不以自由文本输入充当默认路径。
- 若当前 route 仍未重建完成，应显示清晰 deferred / unavailable 说明，但不能制造“大面积空白 + 少量角落提示”的失败壳层。
- settings 内所有控件必须具备可见的 hover、focus、pressed、selected、disabled 状态；不能只有“能点”而没有“点了以后界面有反馈”。
- toggle、enum selector、button row、modal actions 和 domain rail 项都需要稳定的按压/选中反馈，避免用户无法判断当前操作是否已生效。
- 键盘焦点可见性是硬要求，不能去掉 focus ring 或让当前焦点位置难以辨认。
- 需要覆盖至少四类视窗兼容性：2k、4k、常见桌面（如 1366x768）和窄高窗口（如 1280x720 或同等比例窗口化）。
- 需要覆盖至少五类纵横比：16:9、2k 常见宽屏、4k 宽屏、5:4、4:3、21:9 与更窄窗口比例，确保 settings 右侧上下文区不会挤压主内容。
- 窗口缩放时必须优先保持 settings 主任务可读，不能靠放大整体字号或隐藏核心内容来“假装适配”。
- 在小窗口下，`settings_context_rail` 和 modal 区必须允许内部滚动或折叠，不能跨容器溢出。
- 验收截图必须包含窗口化状态，不只测全屏；至少要有一张 2k 截图、一张 4k 截图、一张 5:4 截图、一张 4:3 截图、一张 21:9 截图、一张窄高窗口截图和一张标准桌面宽高截图。

## 文件结构

- 创建 `src/game/client/RmlUi/RmlUiSettingsPathSelector.h`
  - 定义 settings page / modal 的 path selector 类型、reason 和纯函数选择逻辑。
- 创建 `src/game/client/RmlUi/RmlUiSettingsPathSelector.cpp`
  - 实现 settings surface 的唯一 active path 选择。
- 创建 `src/test/rmlui_settings_path_selector_test.cpp`
  - 覆盖 selector 的 `legacy` / `rmlui` / runtime unavailable / frontend unavailable / safe mode blocked 分支。
- 创建 `src/game/client/RmlUi/RmlUiSettingsSemanticSurface.h`
  - 定义 settings route、page model、control kind、action 和 action result。
- 创建 `src/game/client/RmlUi/RmlUiSettingsSemanticSurface.cpp`
  - 实现 route tree、core domains 页面模型、restart 聚合与最小 action 语义。
- 创建 `src/test/rmlui_settings_semantic_surface_test.cpp`
  - 覆盖 route tree、core domain 页面模型、legacy-only/deferred reason 与 action result。
- 创建 `src/game/client/RmlUi/RmlUiSettingsFrontend.h`
  - 定义独立 settings frontend owner，持有 page/modal 文档更新与 action 回接接口。
- 创建 `src/game/client/RmlUi/RmlUiSettingsFrontend.cpp`
  - 实现 page/modal 文档加载、DOM 锚点刷新、action 派发与 restart banner 更新。
- 创建 `src/test/rmlui_settings_frontend_test.cpp`
  - 覆盖前端壳 DOM 锚点、route refresh、deferred reason 和 modal 生命周期。
- 创建 `data/qmclient/rmlui/settings_frontend.rml`
  - 独立 settings page 文档，不再依赖 `workspace_fragment_host`。
- 创建 `data/qmclient/rmlui/settings_frontend.rcss`
  - 独立 settings page 样式。
- 创建 `data/qmclient/rmlui/settings_modal.rml`
  - 独立 settings modal 文档。
- 创建 `data/qmclient/rmlui/settings_modal.rcss`
  - 独立 settings modal 样式。
- 修改 `src/engine/shared/config_variables_qmclient.h`
  - 新增 `qm_rmlui_settings_frontend` 配置项。
- 修改 `src/game/client/RmlUi/RmlUiSurface.h`
  - 补 `SETTINGS_MODAL` surface 与必要辅助声明。
- 修改 `src/game/client/RmlUi/RmlUiSurface.cpp`
  - 让 settings modal 能返回稳定 surface name / document path / default reason。
- 修改 `src/game/client/RmlUi/RmlUiRuntime.h`
  - 持有 `CRmlUiSettingsFrontend`，暴露 settings frontend 可用性与渲染入口。
- 修改 `src/game/client/RmlUi/RmlUiRuntime.cpp`
  - 初始化独立 settings frontend 资源；从旧 `settings_page.rml` 工作区模型切到 `settings_frontend.rml` / `settings_modal.rml` 文档流。
- 修改 `src/game/client/RmlUi/RmlUiSettingsPage.h`
  - 收缩旧骨架模型职责，只保留与历史 skeleton 兼容的最小类型，避免与 semantic/front-end 新模型重名冲突。
- 修改 `src/game/client/RmlUi/RmlUiSettingsPage.cpp`
  - 移除 active owner 角色；保留为历史 skeleton 兼容代码并在文件头显式标注。
- 修改 `src/game/client/components/menus.h`
  - 暴露 settings tab、restart 状态与最小 route/label 查询接口；声明 settings path selector 所需入口。
- 修改 `src/game/client/components/menus.cpp`
  - 补 settings route label / state access 的最小桥接。
- 修改 `src/game/client/components/menus_settings.cpp`
  - 将 `RenderSettings(...)` 重置为 path selector 宿主 seam；抽出可复用的 restart / route / domain 语义函数。
- 修改 `src/game/client/components/menus_settings7.cpp`
  - 补 tee/7.x 标签或摘要所需的最小只读语义函数。
- 修改 `src/game/client/components/menus_settings_assets.cpp`
  - 补 deferred resource domain 的 availability reason 与标题摘要接口。
- 修改 `src/game/client/components/tclient/menus_tclient.cpp`
  - 为 `FEATURES` / `CONFIGURATION` 域暴露最小只读标签或状态入口，不引入 active render 依赖。
- 修改 `src/game/client/components/qmclient/menus_qmclient.cpp`
  - 为 `FEATURES` 域暴露最小只读标签或状态入口，不引入 active render 依赖。
- 修改 `CMakeLists.txt`
  - 注册新增源文件、测试文件和 `EXPECTED_DATA` 资源。

## 任务 1：先落 settings path selector，把 `RenderSettings(...)` 变成单一 active 宿主

**文件：**
- 创建：`src/game/client/RmlUi/RmlUiSettingsPathSelector.h`
- 创建：`src/game/client/RmlUi/RmlUiSettingsPathSelector.cpp`
- 创建：`src/test/rmlui_settings_path_selector_test.cpp`
- 修改：`src/engine/shared/config_variables_qmclient.h`
- 修改：`src/game/client/RmlUi/RmlUiSurface.h`
- 修改：`src/game/client/RmlUi/RmlUiSurface.cpp`
- 修改：`src/game/client/components/menus.h`
- 修改：`src/game/client/components/menus_settings.cpp`
- 修改：`CMakeLists.txt`

- [ ] **步骤 1：编写失败测试，锁定 settings path selector 结果**

在 `src/test/rmlui_settings_path_selector_test.cpp` 先写最小失败测试：

```cpp
#include <gtest/gtest.h>

#include <game/client/RmlUi/RmlUiSettingsPathSelector.h>
#include <game/client/ui_stack.h>

TEST(RmlUiSettingsPathSelector, LegacyStackForcesLegacyPath)
{
	const SSettingsUiPathRequest Request{
		ESettingsUiSurface::PAGE,
		EQmUiStack::LEGACY,
		true,
		true,
		true,
		false,
	};
	const SSettingsUiPathResult Result = SelectSettingsUiPath(Request);
	EXPECT_EQ(Result.m_Path, ESettingsUiPath::LEGACY);
	EXPECT_EQ(Result.m_Reason, ESettingsUiPathReason::GLOBAL_STACK_LEGACY);
}

TEST(RmlUiSettingsPathSelector, DisabledSettingsFrontendStaysOnLegacy)
{
	const SSettingsUiPathRequest Request{
		ESettingsUiSurface::PAGE,
		EQmUiStack::RMLUI,
		false,
		true,
		true,
		false,
	};
	const SSettingsUiPathResult Result = SelectSettingsUiPath(Request);
	EXPECT_EQ(Result.m_Path, ESettingsUiPath::LEGACY);
	EXPECT_EQ(Result.m_Reason, ESettingsUiPathReason::SETTINGS_FRONTEND_DISABLED);
}

TEST(RmlUiSettingsPathSelector, RuntimeUnavailableFallsBackToLegacySettings)
{
	const SSettingsUiPathRequest Request{
		ESettingsUiSurface::PAGE,
		EQmUiStack::RMLUI,
		true,
		false,
		true,
		false,
	};
	const SSettingsUiPathResult Result = SelectSettingsUiPath(Request);
	EXPECT_EQ(Result.m_Path, ESettingsUiPath::LEGACY);
	EXPECT_EQ(Result.m_Reason, ESettingsUiPathReason::RUNTIME_UNAVAILABLE);
}

TEST(RmlUiSettingsPathSelector, FrontendAvailableEntersRmlUiPath)
{
	const SSettingsUiPathRequest Request{
		ESettingsUiSurface::PAGE,
		EQmUiStack::RMLUI,
		true,
		true,
		true,
		false,
	};
	const SSettingsUiPathResult Result = SelectSettingsUiPath(Request);
	EXPECT_EQ(Result.m_Path, ESettingsUiPath::RMLUI);
	EXPECT_EQ(Result.m_Reason, ESettingsUiPathReason::NONE);
}
```

- [ ] **步骤 2：运行测试验证失败**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：构建失败，提示缺少 `RmlUiSettingsPathSelector` 类型、枚举或 `SelectSettingsUiPath(...)` 实现。

- [x] **步骤 3：声明 selector 类型与最小实现**

在 `src/game/client/RmlUi/RmlUiSettingsPathSelector.h` 定义：

```cpp
#ifndef GAME_CLIENT_RMLUI_RMLUI_SETTINGS_PATH_SELECTOR_H
#define GAME_CLIENT_RMLUI_RMLUI_SETTINGS_PATH_SELECTOR_H

#include <game/client/ui_stack.h>

enum class ESettingsUiSurface
{
	PAGE,
	MODAL,
};

enum class ESettingsUiPath
{
	LEGACY,
	RMLUI,
};

enum class ESettingsUiPathReason
{
	NONE,
	GLOBAL_STACK_LEGACY,
	SETTINGS_FRONTEND_DISABLED,
	RUNTIME_UNAVAILABLE,
	FRONTEND_UNAVAILABLE,
	SAFE_MODE_BLOCKED,
};

struct SSettingsUiPathRequest
{
	ESettingsUiSurface m_Surface;
	EQmUiStack m_ActiveStack;
	bool m_SettingsFrontendEnabled;
	bool m_RuntimeAvailable;
	bool m_FrontendAvailable;
	bool m_SafeModeBlocked;
};

struct SSettingsUiPathResult
{
	ESettingsUiPath m_Path = ESettingsUiPath::LEGACY;
	ESettingsUiPathReason m_Reason = ESettingsUiPathReason::GLOBAL_STACK_LEGACY;
};

SSettingsUiPathResult SelectSettingsUiPath(const SSettingsUiPathRequest &Request);

#endif
```

在 `src/game/client/RmlUi/RmlUiSettingsPathSelector.cpp` 写最小纯函数：

```cpp
#include "RmlUiSettingsPathSelector.h"

SSettingsUiPathResult SelectSettingsUiPath(const SSettingsUiPathRequest &Request)
{
	if(Request.m_ActiveStack == EQmUiStack::LEGACY)
		return {ESettingsUiPath::LEGACY, ESettingsUiPathReason::GLOBAL_STACK_LEGACY};
	if(!Request.m_SettingsFrontendEnabled)
		return {ESettingsUiPath::LEGACY, ESettingsUiPathReason::SETTINGS_FRONTEND_DISABLED};
	if(!Request.m_RuntimeAvailable)
		return {ESettingsUiPath::LEGACY, ESettingsUiPathReason::RUNTIME_UNAVAILABLE};
	if(!Request.m_FrontendAvailable)
		return {ESettingsUiPath::LEGACY, ESettingsUiPathReason::FRONTEND_UNAVAILABLE};
	if(Request.m_SafeModeBlocked)
		return {ESettingsUiPath::LEGACY, ESettingsUiPathReason::SAFE_MODE_BLOCKED};
	return {ESettingsUiPath::RMLUI, ESettingsUiPathReason::NONE};
}
```

- [x] **步骤 4：注册配置项与测试文件**

在 `src/engine/shared/config_variables_qmclient.h` 紧接 `qm_ui_stack` 后加入：

```cpp
MACRO_CONFIG_INT(QmRmluiSettingsFrontend, qm_rmlui_settings_frontend, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Settings 页面使用独立 RmlUI frontend（修改后重启生效）")
```

在 `CMakeLists.txt` 的 client/test 列表中加入：

```cmake
    src/game/client/RmlUi/RmlUiSettingsPathSelector.cpp
    src/test/rmlui_settings_path_selector_test.cpp
```

- [x] **步骤 5：运行测试验证 selector 通过**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：`RmlUiSettingsPathSelector` 相关测试 PASS。

- [x] **步骤 6：编写宿主 seam 失败测试，锁定 `RenderSettings(...)` 不再混合两套 owner**

在 `src/test/rmlui_surface_registry_test.cpp` 追加一个 focused 测试，锁定 `SETTINGS_MODAL` 已被注册成独立 surface：

```cpp
TEST(RmlUiSettingsPage, SettingsModalSurfaceIsRegisteredSeparately)
{
	const auto Surfaces = AllRmlUiSurfaces();
	EXPECT_NE(std::find(Surfaces.begin(), Surfaces.end(), ERmlUiSurface::SETTINGS_PAGE), Surfaces.end());
	EXPECT_NE(std::find(Surfaces.begin(), Surfaces.end(), ERmlUiSurface::SETTINGS_MODAL), Surfaces.end());
}
```

同时在 `src/game/client/RmlUi/RmlUiSurface.h/.cpp` 扩展 `ERmlUiSurface` 和 `AllRmlUiSurfaces()`，确保 `SETTINGS_MODAL` 作为独立 surface 存在。

- [x] **步骤 7：把 `RenderSettings(...)` 改成 path selector 宿主 seam**

在 `src/game/client/components/menus_settings.cpp` 的 `RenderSettings(CUIRect MainView)` 开头引入：

```cpp
const SSettingsUiPathRequest PageRequest{
	ESettingsUiSurface::PAGE,
	GameClient()->ActiveUiStack(),
	g_Config.m_QmRmluiSettingsFrontend != 0,
	GameClient()->RmlUiRuntime().Ready(),
	GameClient()->RmlUiRuntime().SettingsFrontendAvailable(),
	false,
};
const SSettingsUiPathResult PagePath = SelectSettingsUiPath(PageRequest);
if(PagePath.m_Path == ESettingsUiPath::RMLUI)
{
	GameClient()->RmlUiRuntime().RenderSettingsFrontendPage();
	return;
}
```

在这个阶段：

- `RenderSettings(...)` 仍保留现有 legacy 渲染分支。
- 但一旦 selector 返回 `RMLUI`，必须立即 `return`，不能继续向下执行 legacy 内容渲染。

- [x] **步骤 8：运行 C++ 测试与客户端构建**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10
```

预期：

- C++ 测试通过。
- 客户端构建通过。

- [ ] **步骤 9：人工验证 settings 双路径入口**

运行：

```powershell
cd build-ninja
.\DDNet.exe
```

人工验证：

- `qm_ui_stack legacy`
- `qm_ui_stack rmlui; qm_rmlui_settings_frontend 0`
- `qm_ui_stack rmlui; qm_rmlui_settings_frontend 1`

预期：

- settings 总是进入单一路径。
- 不再出现 RmlUI settings active path 下继续渲染 legacy 内容的 mixed host 状态。

- [ ] **步骤 10：Commit**

```bash
git add src/engine/shared/config_variables_qmclient.h src/game/client/RmlUi/RmlUiSettingsPathSelector.h src/game/client/RmlUi/RmlUiSettingsPathSelector.cpp src/game/client/RmlUi/RmlUiSurface.h src/game/client/RmlUi/RmlUiSurface.cpp src/game/client/components/menus_settings.cpp src/test/rmlui_settings_path_selector_test.cpp src/test/rmlui_settings_page_test.cpp CMakeLists.txt
git commit -m "feat: add settings dual-path selector"
```

## 任务 2：抽出 settings semantic surface，先让 core domains 变成可消费数据

**文件：**
- 创建：`src/game/client/RmlUi/RmlUiSettingsSemanticSurface.h`
- 创建：`src/game/client/RmlUi/RmlUiSettingsSemanticSurface.cpp`
- 创建：`src/test/rmlui_settings_semantic_surface_test.cpp`
- 修改：`src/game/client/components/menus.h`
- 修改：`src/game/client/components/menus.cpp`
- 修改：`src/game/client/components/menus_settings.cpp`
- 修改：`src/game/client/components/menus_settings7.cpp`
- 修改：`src/game/client/components/menus_settings_assets.cpp`
- 修改：`src/game/client/components/tclient/menus_tclient.cpp`
- 修改：`src/game/client/components/qmclient/menus_qmclient.cpp`
- 修改：`CMakeLists.txt`

- [ ] **步骤 1：编写失败测试，先锁 route tree 与 core domain page model**

在 `src/test/rmlui_settings_semantic_surface_test.cpp` 先写：

```cpp
#include <gtest/gtest.h>

#include <game/client/RmlUi/RmlUiSettingsSemanticSurface.h>

TEST(RmlUiSettingsSemanticSurface, BuildsStableRouteTreeForCoreDomains)
{
	std::vector<SRmlUiSettingsRoute> vRoutes;
	ASSERT_TRUE(QueryRmlUiSettingsRouteTree(&vRoutes));
	EXPECT_FALSE(vRoutes.empty());
	EXPECT_EQ(vRoutes[0].m_Domain, ERmlUiSettingsDomain::HUD_AND_LANGUAGE);
	EXPECT_STREQ(vRoutes[0].m_pDestinationId, "hud-overview");
}

TEST(RmlUiSettingsSemanticSurface, BuildsGraphicsPageModel)
{
	SRmlUiSettingsPageModel Model;
	ASSERT_TRUE(BuildRmlUiSettingsPageModel({ERmlUiSettingsDomain::GRAPHICS, "graphics-display"}, &Model));
	EXPECT_EQ(Model.m_Route.m_Domain, ERmlUiSettingsDomain::GRAPHICS);
	EXPECT_FALSE(Model.m_vSections.empty());
	EXPECT_FALSE(Model.m_RequiresRestart && Model.m_pRestartReason == nullptr);
}

TEST(RmlUiSettingsSemanticSurface, MarksDeferredResourceRoutesExplicitly)
{
	SRmlUiSettingsPageModel Model;
	ASSERT_TRUE(BuildRmlUiSettingsPageModel({ERmlUiSettingsDomain::RESOURCES, "resources-overview"}, &Model));
	EXPECT_TRUE(Model.m_IsLegacyOnly);
	EXPECT_STREQ(Model.m_pAvailabilityReason, "complex_editor_page_deferred");
}
```

- [ ] **步骤 2：运行测试验证失败**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：构建失败，提示缺少 `RmlUiSettingsSemanticSurface` 类型、route/page/action 定义或实现。

- [x] **步骤 3：声明 semantic surface 的核心类型**

在 `src/game/client/RmlUi/RmlUiSettingsSemanticSurface.h` 定义最小可编译类型：

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

struct SRmlUiSettingsControlModel
{
	std::string m_Id;
	ERmlUiSettingsControlKind m_Kind;
	std::string m_Label;
	std::string m_ValueText;
};

struct SRmlUiSettingsSectionModel
{
	std::string m_Title;
	std::vector<SRmlUiSettingsControlModel> m_vControls;
};

struct SRmlUiSettingsPageModel
{
	SRmlUiSettingsRoute m_Route;
	std::string m_Title;
	std::string m_Subtitle;
	std::vector<SRmlUiSettingsSectionModel> m_vSections;
	bool m_RequiresRestart = false;
	const char *m_pRestartReason = nullptr;
	bool m_IsLegacyOnly = false;
	const char *m_pAvailabilityReason = nullptr;
};
```

- [x] **步骤 4：实现最小 route tree 与 page model**

在 `src/game/client/RmlUi/RmlUiSettingsSemanticSurface.cpp` 先写出最小稳定 route tree：

```cpp
bool QueryRmlUiSettingsRouteTree(std::vector<SRmlUiSettingsRoute> *pRoutes)
{
	if(pRoutes == nullptr)
		return false;
	*pRoutes = {
		{ERmlUiSettingsDomain::HUD_AND_LANGUAGE, "hud-overview"},
		{ERmlUiSettingsDomain::HUD_AND_LANGUAGE, "language-list"},
		{ERmlUiSettingsDomain::GRAPHICS, "graphics-display"},
		{ERmlUiSettingsDomain::SOUND, "sound-output"},
		{ERmlUiSettingsDomain::RESOURCES, "resources-overview"},
		{ERmlUiSettingsDomain::FEATURES, "feature-qmclient-overview"},
		{ERmlUiSettingsDomain::CONFIGURATION, "configuration-profiles"},
	};
	return true;
}
```

并为 `graphics-display`、`sound-output`、`hud-overview` 提供最小结构化页面模型；对 `resources-overview` 返回：

```cpp
Model.m_IsLegacyOnly = true;
Model.m_pAvailabilityReason = "complex_editor_page_deferred";
```

- [x] **步骤 5：补 restart 聚合与最小 action 结果**

在同文件追加：

```cpp
bool QueryRmlUiSettingsRestartState(bool *pNeedsRestart, const char **ppReason)
{
	if(pNeedsRestart == nullptr || ppReason == nullptr)
		return false;
	*pNeedsRestart = false;
	*ppReason = nullptr;
	return true;
}

bool ConsumeRmlUiSettingsAction(const SRmlUiSettingsAction &Action, SRmlUiSettingsActionResult *pResult)
{
	if(pResult == nullptr)
		return false;
	*pResult = {};
	if(Action.m_Type == ERmlUiSettingsActionType::OPEN_MODAL)
	{
		pResult->m_OpenModal = true;
		pResult->m_pModalId = "settings-info-modal";
	}
	return true;
}
```

这一步先让接口闭环，不追求把所有真实写回一次做完。

- [x] **步骤 6：抽 legacy settings 的最小只读入口**

在 `src/game/client/components/menus.h` 先补出可复用查询：

```cpp
const char *SettingsTabLabel(int Page) const;
bool NeedRestartGraphics() const;
bool NeedRestartSound() const;
bool NeedRestartUpdate() const;
int CurrentSettingsPage() const;
```

这些查询当前大多已存在；本步骤只把 semantic surface 真正需要的入口补齐，避免它去读 UI 缓存或渲染路径。

在 `src/game/client/components/menus_settings.cpp` 抽出最小 helper，例如：

```cpp
static const char *RmlUiSettingsRouteTitle(int SettingsPage);
static const char *RmlUiSettingsRouteDestination(int SettingsPage);
```

- [x] **步骤 7：运行测试验证 semantic surface 通过**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：`RmlUiSettingsSemanticSurface` 相关测试通过。

- [ ] **步骤 8：Commit**

```bash
git add src/game/client/RmlUi/RmlUiSettingsSemanticSurface.h src/game/client/RmlUi/RmlUiSettingsSemanticSurface.cpp src/game/client/components/menus.h src/game/client/components/menus_settings.cpp src/game/client/components/menus_settings7.cpp src/game/client/components/menus_settings_assets.cpp src/game/client/components/tclient/menus_tclient.cpp src/game/client/components/qmclient/menus_qmclient.cpp src/test/rmlui_settings_semantic_surface_test.cpp CMakeLists.txt
git commit -m "feat: add settings semantic surface"
```

## 任务 3：建立独立 settings frontend 壳，脱离 `workspace_fragment_host`

**文件：**
- 创建：`src/game/client/RmlUi/RmlUiSettingsFrontend.h`
- 创建：`src/game/client/RmlUi/RmlUiSettingsFrontend.cpp`
- 创建：`src/test/rmlui_settings_frontend_test.cpp`
- 创建：`data/qmclient/rmlui/settings_frontend.rml`
- 创建：`data/qmclient/rmlui/settings_frontend.rcss`
- 创建：`data/qmclient/rmlui/settings_modal.rml`
- 创建：`data/qmclient/rmlui/settings_modal.rcss`
- 修改：`src/game/client/RmlUi/RmlUiRuntime.h`
- 修改：`src/game/client/RmlUi/RmlUiRuntime.cpp`
- 修改：`src/game/client/RmlUi/RmlUiSurface.h`
- 修改：`src/game/client/RmlUi/RmlUiSurface.cpp`
- 修改：`src/game/client/RmlUi/RmlUiSettingsPage.h`
- 修改：`src/game/client/RmlUi/RmlUiSettingsPage.cpp`
- 修改：`CMakeLists.txt`

- [ ] **步骤 1：编写失败测试，先锁定 frontend DOM 锚点**

在 `src/test/rmlui_settings_frontend_test.cpp` 写入：

```cpp
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

namespace
{
std::string ReadFileText(const char *pPath)
{
	std::ifstream File(pPath, std::ios::binary);
	std::ostringstream Stream;
	Stream << File.rdbuf();
	return Stream.str();
}
}

TEST(RmlUiSettingsFrontend, FrontendDocumentContainsStableAnchors)
{
	const std::string Markup = ReadFileText("../data/qmclient/rmlui/settings_frontend.rml");
	EXPECT_NE(Markup.find("settings_frontend_root"), std::string::npos);
	EXPECT_NE(Markup.find("settings_domain_rail"), std::string::npos);
	EXPECT_NE(Markup.find("settings_workspace"), std::string::npos);
	EXPECT_NE(Markup.find("settings_context_rail"), std::string::npos);
	EXPECT_NE(Markup.find("settings_restart_banner"), std::string::npos);
}

TEST(RmlUiSettingsFrontend, ModalDocumentContainsStableAnchors)
{
	const std::string Markup = ReadFileText("../data/qmclient/rmlui/settings_modal.rml");
	EXPECT_NE(Markup.find("settings_modal_root"), std::string::npos);
	EXPECT_NE(Markup.find("settings_modal_title"), std::string::npos);
	EXPECT_NE(Markup.find("settings_modal_body"), std::string::npos);
	EXPECT_NE(Markup.find("settings_modal_actions"), std::string::npos);
}
```

- [ ] **步骤 2：运行测试验证失败**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：测试失败，提示找不到 `settings_frontend.rml` / `settings_modal.rml` 或 DOM 锚点缺失。

- [x] **步骤 3：创建独立 settings page / modal 文档**

在 `data/qmclient/rmlui/settings_frontend.rml` 写入最小结构：

```rml
<rml>
	<head>
		<title>QmClient settings frontend</title>
		<link type="text/rcss" href="settings_frontend.rcss"/>
	</head>
	<body>
		<div id="settings_frontend_root">
			<div id="settings_toolbar">
				<div id="settings_route_title"></div>
				<div id="settings_route_subtitle"></div>
				<div id="settings_search_slot"></div>
			</div>
			<div id="settings_frontend_body">
				<div id="settings_domain_rail"></div>
				<div id="settings_workspace">
					<div id="settings_restart_banner"></div>
					<div id="settings_section_host"></div>
				</div>
				<div id="settings_context_rail"></div>
			</div>
		</div>
	</body>
</rml>
```

在 `data/qmclient/rmlui/settings_modal.rml` 写入：

```rml
<rml>
	<head>
		<title>QmClient settings modal</title>
		<link type="text/rcss" href="settings_modal.rcss"/>
	</head>
	<body>
		<div id="settings_modal_root">
			<div id="settings_modal_title"></div>
			<div id="settings_modal_body"></div>
			<div id="settings_modal_actions"></div>
		</div>
	</body>
</rml>
```

- [x] **步骤 4：声明 frontend owner 类型**

在 `src/game/client/RmlUi/RmlUiSettingsFrontend.h` 定义：

```cpp
class CRmlUiSettingsFrontend
{
public:
	bool Init(Rml::Context *pContext);
	void Shutdown();
	bool Available() const { return m_pPageDocument != nullptr && m_pModalDocument != nullptr; }
	bool RenderPage(const SRmlUiSettingsPageModel &Model);
	bool ShowModal(const char *pTitle, const char *pBody);

private:
	Rml::ElementDocument *m_pPageDocument = nullptr;
	Rml::ElementDocument *m_pModalDocument = nullptr;
};
```

- [x] **步骤 5：实现最小文档加载与 page refresh**

在 `src/game/client/RmlUi/RmlUiSettingsFrontend.cpp` 先实现：

```cpp
bool CRmlUiSettingsFrontend::Init(Rml::Context *pContext)
{
	if(pContext == nullptr)
		return false;
	m_pPageDocument = pContext->LoadDocument("qmclient/rmlui/settings_frontend.rml");
	m_pModalDocument = pContext->LoadDocument("qmclient/rmlui/settings_modal.rml");
	if(m_pPageDocument != nullptr)
		m_pPageDocument->Show();
	if(m_pModalDocument != nullptr)
		m_pModalDocument->Hide();
	return Available();
}
```

并在 `RenderPage(...)` 中至少刷新：

```cpp
SetElementText(m_pPageDocument, "settings_route_title", Model.m_Title.c_str());
SetElementText(m_pPageDocument, "settings_route_subtitle", Model.m_Subtitle.c_str());
SetElementText(m_pPageDocument, "settings_restart_banner", Model.m_RequiresRestart ? Model.m_pRestartReason : "");
```

补充布局要求：

- `settings_toolbar`、`settings_domain_rail`、`settings_workspace`、`settings_context_rail` 必须形成清晰的主次层级，工作区始终是第一视觉焦点。
- 首屏应至少展示当前 route 的核心设置 section，而不是只显示标题、subtitle 和空白背景。
- 右侧上下文区采用固定宽度与内部滚动，不得因内容增多而把主设置区压缩失衡。
- 当前 route 的主要操作按钮、应用/重置按钮、modal 确认按钮必须立即反馈按压态与触发态，不能出现“按钮无反馈”的空点击感。

- [x] **步骤 6：把 runtime 接到独立 frontend**

在 `src/game/client/RmlUi/RmlUiRuntime.h` 增加：

```cpp
#include "RmlUiSettingsFrontend.h"

bool SettingsFrontendAvailable() const { return m_SettingsFrontend.Available(); }
bool RenderSettingsFrontendPage();
```

并添加成员：

```cpp
CRmlUiSettingsFrontend m_SettingsFrontend;
```

在 `src/game/client/RmlUi/RmlUiRuntime.cpp` 中先定义当前 route 映射 helper：

```cpp
static SRmlUiSettingsRoute CurrentSettingsRoute(const CMenus &Menus)
{
	switch(CMenus::NormalizeSettingsPageValue(Menus.CurrentSettingsPage()))
	{
	case CMenus::SETTINGS_GRAPHICS:
		return {ERmlUiSettingsDomain::GRAPHICS, "graphics-display"};
	case CMenus::SETTINGS_SOUND:
		return {ERmlUiSettingsDomain::SOUND, "sound-output"};
	default:
		return {ERmlUiSettingsDomain::HUD_AND_LANGUAGE, "hud-overview"};
	}
}
```

然后在 `Init(...)` 中初始化 `m_SettingsFrontend`；在 `RenderSettingsFrontendPage()` 中：

```cpp
SRmlUiSettingsPageModel Model;
if(!BuildRmlUiSettingsPageModel(CurrentSettingsRoute(), &Model))
	return false;
return m_SettingsFrontend.RenderPage(Model);
```

- [x] **步骤 7：运行测试验证 frontend 壳通过**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10
```

预期：

- frontend 文档结构测试通过。
- 客户端构建通过。
- frontend 结构通过不等于视觉完成；后续仍必须满足共享壳层的 1:1 布局验收。
- settings 前端的主交互控件已经具备基础状态反馈，而不是只有 DOM 锚点和数据绑定。
- 需要在 plan 中显式定义的状态类包括：`is-hovered`、`is-focused`、`is-pressed`、`is-selected`、`is-disabled`、`is-open`、`is-unavailable`、`is-deferred`。
- 需要在 plan 中显式定义的控件类包括：`setting_toggle_row`、`setting_enum_row`、`setting_button_row`、`setting_text_input_row`、`setting_color_row`、`settings_modal_button`、`settings_domain_item`。
- 交互反馈必须通过可测的 class / attribute 切换体现，而不是只在 C++ 逻辑里更新数值。

- [ ] **步骤 8：人工验证独立 settings 前端**

运行：

```powershell
cd build-ninja
.\DDNet.exe
```

人工验证：

- `qm_ui_stack rmlui`
- `qm_rmlui_settings_frontend 1`
- 进入 settings
- 切 `Language/Graphics/Sound` 对应 route

预期：

- 不再依赖 `workspace_fragment_host`
- 中央工作区不再是旧 `settings_page.rml` 骨架
- restart banner 与 deferred reason 可见
- 首屏主设置区已被真实内容填充，而不是“大面积空白 + 边缘卡片”
- 右侧上下文区不会遮挡、挤压或转移主任务焦点
- 长内容 route 在滚动后仍可完成关键操作
- domain rail、section 控件、modal 按钮和主操作按钮都有清晰可感知的交互反馈
- 窄高窗口下仍必须看得见当前 route 标题和一个可操作的主控件，不能把主区压到只剩空壳。
- 需要额外截图确认 `settings_domain_rail`、`settings_workspace`、`settings_context_rail` 三栏同时存在且层级正确。
- 需要额外截图确认一个 toggle、一个 enum selector、一个 button row、一个 modal action 的状态变化。
- 需要额外截图确认 focus ring 可见，且不会被背景或边框吞没。
- `settings` 的验收截图必须明确显示：`settings_toolbar`、`settings_domain_rail`、`settings_workspace`、`settings_context_rail` 四个区域都在，但主任务仍由 `settings_workspace` 承担。

- [ ] **步骤 9：Commit**

```bash
git add src/game/client/RmlUi/RmlUiSettingsFrontend.h src/game/client/RmlUi/RmlUiSettingsFrontend.cpp src/game/client/RmlUi/RmlUiRuntime.h src/game/client/RmlUi/RmlUiRuntime.cpp src/game/client/RmlUi/RmlUiSettingsPage.h src/game/client/RmlUi/RmlUiSettingsPage.cpp data/qmclient/rmlui/settings_frontend.rml data/qmclient/rmlui/settings_frontend.rcss data/qmclient/rmlui/settings_modal.rml data/qmclient/rmlui/settings_modal.rcss src/test/rmlui_settings_frontend_test.cpp CMakeLists.txt
git commit -m "feat: add standalone rmlui settings frontend shell"
```

## 任务 4：收口验证与计划边界，确认 skeleton 路径不再冒充 settings 主线

**文件：**
- 修改：`docs/superpowers/plans/2026-05-15-rmlui-main-menu-serverbrowser-settings-skeleton.md`
- 修改：`src/game/client/RmlUi/RmlUiSettingsPage.h`
- 修改：`src/game/client/RmlUi/RmlUiSettingsPage.cpp`

- [x] **步骤 1：更新旧 skeleton 计划的 settings 边界说明**

在 `docs/superpowers/plans/2026-05-15-rmlui-main-menu-serverbrowser-settings-skeleton.md` 追加一句明确说明：

```md
- `settings` 主线已转入 `2026-05-15-rmlui-settings-dual-stack-foundation.md`，本计划中的 settings 骨架只保留为历史过渡层与视觉参考。
```

- [x] **步骤 2：把旧 `RmlUiSettingsPage.*` 收缩为历史兼容代码**

在 `src/game/client/RmlUi/RmlUiSettingsPage.h` 顶部加入明确注释，固定采用“保留旧类型名但标注历史角色”的做法：

```cpp
// Historical skeleton model for the main-menu workspace plan. Not the active settings frontend owner.
```

并在 `src/game/client/RmlUi/RmlUiSettingsPage.cpp` 文件头加入同样说明，确保后续实现者不会再把它当作 active settings frontend owner。

- [ ] **步骤 3：运行最终验证**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10
```

预期：全部通过。

- [ ] **步骤 4：最终人工验收**

人工验证以下路径：

1. `qm_ui_stack legacy`
2. `qm_ui_stack rmlui + qm_rmlui_settings_frontend 0`
3. `qm_ui_stack rmlui + qm_rmlui_settings_frontend 1`

并检查：

- settings path 始终单一路径
- active RmlUI path 下不再混 legacy 内容
- deferred domain 有文字说明，不留空白大面板

- [ ] **步骤 5：Commit**

```bash
git add docs/superpowers/plans/2026-05-15-rmlui-main-menu-serverbrowser-settings-skeleton.md src/game/client/RmlUi/RmlUiSettingsPage.h src/game/client/RmlUi/RmlUiSettingsPage.cpp
git commit -m "docs: clarify settings skeleton handoff"
```

## 自检

- [x] **规格覆盖度检查**
  - `dual-stack reset` 是否有独立 selector、配置项、`RenderSettings(...)` 宿主 seam 与 `SETTINGS_MODAL` surface 任务。
  - `semantic surface` 是否有 route tree、page model、action、restart state 与 core domains 覆盖任务。
  - `frontend shell` 是否有独立 page/modal 文档、runtime owner、DOM 锚点与 route refresh 任务。
- [ ] **占位符扫描**
  - 搜索计划中是否存在“TODO / 待定 / 后续实现 / 类似任务 N”。
- [x] **类型一致性检查**
  - 确认 `ESettingsUiSurface` / `ESettingsUiPath` / `SRmlUiSettingsRoute` / `SRmlUiSettingsPageModel` / `CRmlUiSettingsFrontend` 等名字在所有任务中保持一致。

补充执行清单：

- 具体实现项见 `docs/superpowers/plans/checklist/2026-05-17-rmlui-shell-实现清单.md`
