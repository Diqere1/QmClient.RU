# RmlUI 全局 UI 栈实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 在当前分支从零建立 `legacy` / `rmlui` 全局 UI 栈选择器，并让 RmlUI 模式下所有接入的 UI surface 显示 RmlUI 内部“未实现/不可用”界面与稳定 reason，不做旧 UI fallback。

**架构：** `CGameClient` 启动时冻结 active UI stack，旧 UI 与 RmlUI 作为两套互斥全局栈存在。RmlUI 栈拥有独立 runtime owner、surface registry、frame contract 和 RML/RCSS 不可用界面；业务 surface 第一阶段全部返回 `surface_not_implemented`，真实 frontend 由独立计划逐个替换。

**技术栈：** C++、CMake、GTest、RmlUI v6.2、DDNet/QmClient 配置系统、Superpowers TDD 工作流。

---

## 前置约束

- 不使用 `cs-*` 技能推进实现；仓库内 `.codestable/attention.md` 与 `AGENTS.md` 只作为约束来源。
- 不读取或复用历史 RmlUI roadmap/design/旧分支实现作为功能输入。
- 配置项使用 `qm_` / `Qm` 前缀：`qm_ui_stack`。
- `qm_ui_stack` 修改后只提示重启生效，不做运行时热切换。
- RmlUI 模式下不得调用旧 UI renderer 作为 active surface 内容。
- RmlUI v6.2 集成顺序必须固定：安装 render/system/file interfaces，`Rml::Initialise()`，创建 context，加载字体，创建 data model，加载 document，`Show()`，每帧 input -> `Context::Update()` -> 游戏/底层渲染 -> `Context::Render()`，关闭时先 `Rml::Shutdown()` 再销毁 interfaces。
- `Context::Update()` 到 `Context::Render()` 之间不得提交 input、修改 DOM 或修改 data model。
- 影响核心 UI 生命周期的实现完成后，按仓库规则派发子代理进行 `/cs-audit` 代码审查，并等待报告完成后再收口。

## 文件结构

- 修改 `src/engine/shared/config_variables_qmclient.h`
  - 新增 `QmUiStack` 字符串配置，默认 `legacy`。
- 创建 `src/game/client/ui_stack.h`
  - 定义 `EQmUiStack`、`SQmUiStackState`、解析与重启状态查询的纯函数。
- 创建 `src/game/client/ui_stack.cpp`
  - 实现 `ParseQmUiStack`、`QmUiStackName`、`BuildQmUiStackState`。
- 修改 `src/game/client/gameclient.h`
  - 持有启动时冻结的 `m_QmActiveUiStack`，暴露 `ActiveUiStack()` 与 `UiStackState()`。
- 修改 `src/game/client/gameclient.cpp`
  - 在 `OnInit()` 早期读取 `g_Config.m_QmUiStack` 并冻结 active stack。
- 创建 `src/game/client/RmlUi/RmlUiSurface.h`
  - 定义 RmlUI surface、status、reason 与 query/result。
- 创建 `src/game/client/RmlUi/RmlUiSurface.cpp`
  - 实现 surface 名称、reason 字符串、第一阶段 registry 查询。
- 创建 `src/game/client/RmlUi/RmlUiRuntime.h`
  - 定义 RmlUI runtime owner 的状态、frame phase、初始化/关闭/渲染入口。
- 创建 `src/game/client/RmlUi/RmlUiRuntime.cpp`
  - 实现 RmlUI v6.2 初始化顺序、frame contract、不可用界面加载与 reason 记录。
- 创建 `src/game/client/RmlUi/RmlUiFileInterface.h`
  - 封装从 QmClient storage 读取 `data/qmclient/rmlui/*.rml` / `.rcss` 的 RmlUI file interface。
- 创建 `src/game/client/RmlUi/RmlUiFileInterface.cpp`
  - 实现 file open/read/seek/tell/close，所有失败返回稳定 reason 到 runtime diagnostics。
- 创建 `src/game/client/RmlUi/RmlUiRenderInterface.h`
  - 定义最小 render interface，收集/提交 RmlUI 几何并维护 scissor 状态。
- 创建 `src/game/client/RmlUi/RmlUiRenderInterface.cpp`
  - 实现 `Rml::RenderInterface` 必需方法；第一阶段支持无贴图纯色几何，不加载业务图片。
- 创建 `src/game/client/RmlUi/RmlUiSystemInterface.h`
  - 定义时间与日志 system interface。
- 创建 `src/game/client/RmlUi/RmlUiSystemInterface.cpp`
  - 将 RmlUI 日志转到 `dbg_msg("rmlui_ui_stack", ...)`。
- 创建 `data/qmclient/rmlui/ui_unavailable.rml`
  - RmlUI 内部不可用界面文档。
- 创建 `data/qmclient/rmlui/ui_unavailable.rcss`
  - 不可用界面样式。
- 修改 `src/game/client/components/menus.h`
  - 声明 RmlUI surface 渲染转发入口。
- 修改 `src/game/client/components/menus.cpp`
  - 在 menu/settings/fullscreen popup/popup menu host seam 按 active stack 分路。
- 修改 `src/game/client/components/debughud.cpp`
  - 在 RmlUI active 时把 debug overlay surface 转给 RmlUI registry。
- 修改 `src/game/client/components/hud_editor.cpp`
  - 在 RmlUI active 时把 HUD editor surface 转给 RmlUI registry。
- 修改 `src/game/client/components/pie_menu.cpp`
  - 在 RmlUI active 时把 bind/emote wheel 类 surface 转给 RmlUI registry。
- 修改 `src/game/client/QmUi/QmRt.cpp`
  - RmlUI active 时跳过旧 QmUI runtime v2，每帧只保留 RmlUI owner。
- 修改 `CMakeLists.txt`
  - 加入 RmlUI 依赖、RmlUI 源文件、测试文件与测试额外源文件。
- 创建 `src/test/qm_ui_stack_test.cpp`
  - 覆盖配置解析、启动冻结、重启提示。
- 创建 `src/test/rmlui_surface_registry_test.cpp`
  - 覆盖 surface -> status/reason/name 映射。
- 创建 `src/test/rmlui_frame_contract_test.cpp`
  - 覆盖 input/update/render phase 顺序与非法 mutation reason。

## 任务 1：全局 UI 栈纯函数与配置默认值

**文件：**
- 创建：`src/game/client/ui_stack.h`
- 创建：`src/game/client/ui_stack.cpp`
- 创建：`src/test/qm_ui_stack_test.cpp`
- 修改：`src/engine/shared/config_variables_qmclient.h`
- 修改：`CMakeLists.txt`

- [ ] **步骤 1：编写失败测试**

在 `src/test/qm_ui_stack_test.cpp` 写入：

```cpp
#include "test.h"

#include <engine/shared/config.h>

#include <game/client/ui_stack.h>

#include <gtest/gtest.h>

TEST(QmUiStack, DefaultConfigIsLegacy)
{
	EXPECT_STREQ(CConfig::ms_pQmUiStack, "legacy");
}

TEST(QmUiStack, ParsesKnownValuesCaseInsensitive)
{
	EXPECT_EQ(ParseQmUiStack("legacy"), EQmUiStack::LEGACY);
	EXPECT_EQ(ParseQmUiStack("LEGACY"), EQmUiStack::LEGACY);
	EXPECT_EQ(ParseQmUiStack("rmlui"), EQmUiStack::RMLUI);
	EXPECT_EQ(ParseQmUiStack("RMLUI"), EQmUiStack::RMLUI);
}

TEST(QmUiStack, InvalidValueFallsBackToLegacyForStartupSelection)
{
	EXPECT_EQ(ParseQmUiStack(""), EQmUiStack::LEGACY);
	EXPECT_EQ(ParseQmUiStack("mixed"), EQmUiStack::LEGACY);
}

TEST(QmUiStack, RestartRequiredWhenConfiguredStackDiffersFromActiveStack)
{
	const SQmUiStackState State = BuildQmUiStackState("legacy", "rmlui");

	EXPECT_EQ(State.m_ActiveStack, EQmUiStack::LEGACY);
	EXPECT_EQ(State.m_ConfiguredStack, EQmUiStack::RMLUI);
	EXPECT_TRUE(State.m_RestartRequired);
}

TEST(QmUiStack, RestartNotRequiredWhenConfiguredStackMatchesActiveStack)
{
	const SQmUiStackState State = BuildQmUiStackState("rmlui", "rmlui");

	EXPECT_EQ(State.m_ActiveStack, EQmUiStack::RMLUI);
	EXPECT_EQ(State.m_ConfiguredStack, EQmUiStack::RMLUI);
	EXPECT_FALSE(State.m_RestartRequired);
}
```

- [ ] **步骤 2：运行测试验证失败**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：构建失败，错误包含找不到 `game/client/ui_stack.h` 或 `ParseQmUiStack` 未定义。

- [ ] **步骤 3：新增配置项**

在 `src/engine/shared/config_variables_qmclient.h` 的现有 UI 调试配置之后加入：

```cpp
MACRO_CONFIG_STR(QmUiStack, qm_ui_stack, 16, "legacy", CFGFLAG_CLIENT | CFGFLAG_SAVE, "全局 UI 栈（legacy 或 rmlui，修改后重启生效）")
```

- [ ] **步骤 4：实现最小 selector 纯函数**

创建 `src/game/client/ui_stack.h`：

```cpp
#ifndef GAME_CLIENT_UI_STACK_H
#define GAME_CLIENT_UI_STACK_H

enum class EQmUiStack
{
	LEGACY,
	RMLUI,
};

struct SQmUiStackState
{
	EQmUiStack m_ActiveStack = EQmUiStack::LEGACY;
	EQmUiStack m_ConfiguredStack = EQmUiStack::LEGACY;
	bool m_RestartRequired = false;
};

EQmUiStack ParseQmUiStack(const char *pValue);
const char *QmUiStackName(EQmUiStack Stack);
SQmUiStackState BuildQmUiStackState(const char *pStartupValue, const char *pCurrentValue);

#endif
```

创建 `src/game/client/ui_stack.cpp`：

```cpp
#include "ui_stack.h"

#include <base/system.h>

EQmUiStack ParseQmUiStack(const char *pValue)
{
	if(pValue != nullptr && str_comp_nocase(pValue, "rmlui") == 0)
		return EQmUiStack::RMLUI;
	return EQmUiStack::LEGACY;
}

const char *QmUiStackName(EQmUiStack Stack)
{
	switch(Stack)
	{
	case EQmUiStack::LEGACY: return "legacy";
	case EQmUiStack::RMLUI: return "rmlui";
	}
	return "legacy";
}

SQmUiStackState BuildQmUiStackState(const char *pStartupValue, const char *pCurrentValue)
{
	SQmUiStackState State;
	State.m_ActiveStack = ParseQmUiStack(pStartupValue);
	State.m_ConfiguredStack = ParseQmUiStack(pCurrentValue);
	State.m_RestartRequired = State.m_ActiveStack != State.m_ConfiguredStack;
	return State;
}
```

- [ ] **步骤 5：把新文件接入构建和测试**

在 `CMakeLists.txt` 的 `set_src(GAME_CLIENT GLOB_RECURSE src/game/client` 列表中加入：

```cmake
    ui_stack.cpp
    ui_stack.h
```

在 `set_src(TESTS GLOB src/test` 列表中加入：

```cmake
    qm_ui_stack_test.cpp
```

在 `set(TESTS_EXTRA` 中加入：

```cmake
    src/game/client/ui_stack.cpp
```

- [ ] **步骤 6：运行测试验证通过**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：`QmUiStack.*` 测试 PASS。

- [ ] **步骤 7：Commit**

```powershell
git add src\engine\shared\config_variables_qmclient.h src\game\client\ui_stack.h src\game\client\ui_stack.cpp src\test\qm_ui_stack_test.cpp CMakeLists.txt
git commit -m "feat: add global UI stack selector"
```

## 任务 2：启动期冻结 active stack 并暴露重启状态

**文件：**
- 修改：`src/game/client/gameclient.h`
- 修改：`src/game/client/gameclient.cpp`
- 修改：`src/test/qm_ui_stack_test.cpp`

- [ ] **步骤 1：补充失败测试**

在 `src/test/qm_ui_stack_test.cpp` 追加：

```cpp
TEST(QmUiStack, StateUsesStartupValueAsImmutableActiveStack)
{
	const SQmUiStackState State = BuildQmUiStackState("rmlui", "legacy");

	EXPECT_EQ(State.m_ActiveStack, EQmUiStack::RMLUI);
	EXPECT_EQ(State.m_ConfiguredStack, EQmUiStack::LEGACY);
	EXPECT_TRUE(State.m_RestartRequired);
	EXPECT_STREQ(QmUiStackName(State.m_ActiveStack), "rmlui");
}
```

- [ ] **步骤 2：运行测试验证失败或未覆盖代码路径**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：如果任务 1 已实现 `BuildQmUiStackState`，测试 PASS；继续本任务把相同行为接入 `CGameClient`。

- [ ] **步骤 3：在 `CGameClient` 中增加状态**

在 `src/game/client/gameclient.h` 加入 include：

```cpp
#include "ui_stack.h"
```

在 `CGameClient` public 区域加入：

```cpp
	EQmUiStack ActiveUiStack() const { return m_QmActiveUiStack; }
	SQmUiStackState UiStackState() const;
```

在 private 成员区域加入：

```cpp
	EQmUiStack m_QmActiveUiStack = EQmUiStack::LEGACY;
	char m_aQmStartupUiStack[16] = "legacy";
```

- [ ] **步骤 4：在 `OnInit()` 冻结启动配置**

在 `src/game/client/gameclient.cpp` 的 `CGameClient::OnInit()` 中，配置迁移之后、UI 初始化之前加入：

```cpp
	str_copy(m_aQmStartupUiStack, g_Config.m_QmUiStack, sizeof(m_aQmStartupUiStack));
	m_QmActiveUiStack = ParseQmUiStack(m_aQmStartupUiStack);
	dbg_msg("qm_ui_stack", "active=%s configured=%s restart_required=0", QmUiStackName(m_QmActiveUiStack), g_Config.m_QmUiStack);
```

在 `gameclient.cpp` 中实现：

```cpp
SQmUiStackState CGameClient::UiStackState() const
{
	return BuildQmUiStackState(m_aQmStartupUiStack, g_Config.m_QmUiStack);
}
```

- [ ] **步骤 5：构建验证**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30
```

预期：`game-client` 构建成功。

- [ ] **步骤 6：Commit**

```powershell
git add src\game\client\gameclient.h src\game\client\gameclient.cpp src\test\qm_ui_stack_test.cpp
git commit -m "feat: freeze active UI stack on startup"
```

## 任务 3：RmlUI surface registry 与稳定 reason

**文件：**
- 创建：`src/game/client/RmlUi/RmlUiSurface.h`
- 创建：`src/game/client/RmlUi/RmlUiSurface.cpp`
- 创建：`src/test/rmlui_surface_registry_test.cpp`
- 修改：`CMakeLists.txt`

- [ ] **步骤 1：编写失败测试**

创建 `src/test/rmlui_surface_registry_test.cpp`：

```cpp
#include "test.h"

#include <game/client/RmlUi/RmlUiSurface.h>

#include <gtest/gtest.h>

TEST(RmlUiSurfaceRegistry, AllInitialBusinessSurfacesAreNotImplemented)
{
	for(ERmlUiSurface Surface : AllRmlUiSurfaces())
	{
		const SRmlUiSurfaceResult Result = QueryInitialRmlUiSurface(Surface);
		EXPECT_EQ(Result.m_Status, ERmlUiSurfaceStatus::NOT_IMPLEMENTED);
		EXPECT_EQ(Result.m_Reason, ERmlUiReason::SURFACE_NOT_IMPLEMENTED);
		EXPECT_STREQ(RmlUiReasonName(Result.m_Reason), "surface_not_implemented");
	}
}

TEST(RmlUiSurfaceRegistry, SurfaceNamesAreStable)
{
	EXPECT_STREQ(RmlUiSurfaceName(ERmlUiSurface::MAIN_MENU), "main_menu");
	EXPECT_STREQ(RmlUiSurfaceName(ERmlUiSurface::SETTINGS_PAGE), "settings_page");
	EXPECT_STREQ(RmlUiSurfaceName(ERmlUiSurface::FULLSCREEN_POPUP), "fullscreen_popup");
	EXPECT_STREQ(RmlUiSurfaceName(ERmlUiSurface::POPUP_MENU), "popup_menu");
	EXPECT_STREQ(RmlUiSurfaceName(ERmlUiSurface::MONITORING_HUD), "monitoring_hud");
	EXPECT_STREQ(RmlUiSurfaceName(ERmlUiSurface::DEBUG_OVERLAY), "debug_overlay");
	EXPECT_STREQ(RmlUiSurfaceName(ERmlUiSurface::CLICK_GUI), "click_gui");
	EXPECT_STREQ(RmlUiSurfaceName(ERmlUiSurface::BIND_WHEEL), "bind_wheel");
	EXPECT_STREQ(RmlUiSurfaceName(ERmlUiSurface::EMOTE_WHEEL), "emote_wheel");
	EXPECT_STREQ(RmlUiSurfaceName(ERmlUiSurface::HUD_EDITOR), "hud_editor");
}
```

- [ ] **步骤 2：运行测试验证失败**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：构建失败，错误包含找不到 `RmlUiSurface.h`。

- [ ] **步骤 3：实现 surface 类型与 registry**

创建 `src/game/client/RmlUi/RmlUiSurface.h`：

```cpp
#ifndef GAME_CLIENT_RMLUI_RMLUI_SURFACE_H
#define GAME_CLIENT_RMLUI_RMLUI_SURFACE_H

#include <array>

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

enum class ERmlUiReason
{
	NONE,
	SURFACE_NOT_IMPLEMENTED,
	DOCUMENT_MISSING,
	DOCUMENT_INVALID,
	RUNTIME_UNAVAILABLE,
	FRONTEND_UNAVAILABLE,
	CONTEXT_UNAVAILABLE,
	DATA_MODEL_UNAVAILABLE,
	INPUT_BRIDGE_UNAVAILABLE,
	RENDER_BRIDGE_UNAVAILABLE,
	FONT_LOAD_FAILED,
	RESOURCE_LOAD_FAILED,
	FRAME_PHASE_VIOLATION,
};

struct SRmlUiSurfaceResult
{
	ERmlUiSurfaceStatus m_Status = ERmlUiSurfaceStatus::UNAVAILABLE;
	ERmlUiReason m_Reason = ERmlUiReason::RUNTIME_UNAVAILABLE;
};

std::array<ERmlUiSurface, 10> AllRmlUiSurfaces();
const char *RmlUiSurfaceName(ERmlUiSurface Surface);
const char *RmlUiReasonName(ERmlUiReason Reason);
SRmlUiSurfaceResult QueryInitialRmlUiSurface(ERmlUiSurface Surface);

#endif
```

创建 `src/game/client/RmlUi/RmlUiSurface.cpp`：

```cpp
#include "RmlUiSurface.h"

std::array<ERmlUiSurface, 10> AllRmlUiSurfaces()
{
	return {
		ERmlUiSurface::MAIN_MENU,
		ERmlUiSurface::SETTINGS_PAGE,
		ERmlUiSurface::FULLSCREEN_POPUP,
		ERmlUiSurface::POPUP_MENU,
		ERmlUiSurface::MONITORING_HUD,
		ERmlUiSurface::DEBUG_OVERLAY,
		ERmlUiSurface::CLICK_GUI,
		ERmlUiSurface::BIND_WHEEL,
		ERmlUiSurface::EMOTE_WHEEL,
		ERmlUiSurface::HUD_EDITOR,
	};
}

const char *RmlUiSurfaceName(ERmlUiSurface Surface)
{
	switch(Surface)
	{
	case ERmlUiSurface::MAIN_MENU: return "main_menu";
	case ERmlUiSurface::SETTINGS_PAGE: return "settings_page";
	case ERmlUiSurface::FULLSCREEN_POPUP: return "fullscreen_popup";
	case ERmlUiSurface::POPUP_MENU: return "popup_menu";
	case ERmlUiSurface::MONITORING_HUD: return "monitoring_hud";
	case ERmlUiSurface::DEBUG_OVERLAY: return "debug_overlay";
	case ERmlUiSurface::CLICK_GUI: return "click_gui";
	case ERmlUiSurface::BIND_WHEEL: return "bind_wheel";
	case ERmlUiSurface::EMOTE_WHEEL: return "emote_wheel";
	case ERmlUiSurface::HUD_EDITOR: return "hud_editor";
	}
	return "unknown";
}

const char *RmlUiReasonName(ERmlUiReason Reason)
{
	switch(Reason)
	{
	case ERmlUiReason::NONE: return "none";
	case ERmlUiReason::SURFACE_NOT_IMPLEMENTED: return "surface_not_implemented";
	case ERmlUiReason::DOCUMENT_MISSING: return "document_missing";
	case ERmlUiReason::DOCUMENT_INVALID: return "document_invalid";
	case ERmlUiReason::RUNTIME_UNAVAILABLE: return "runtime_unavailable";
	case ERmlUiReason::FRONTEND_UNAVAILABLE: return "frontend_unavailable";
	case ERmlUiReason::CONTEXT_UNAVAILABLE: return "context_unavailable";
	case ERmlUiReason::DATA_MODEL_UNAVAILABLE: return "data_model_unavailable";
	case ERmlUiReason::INPUT_BRIDGE_UNAVAILABLE: return "input_bridge_unavailable";
	case ERmlUiReason::RENDER_BRIDGE_UNAVAILABLE: return "render_bridge_unavailable";
	case ERmlUiReason::FONT_LOAD_FAILED: return "font_load_failed";
	case ERmlUiReason::RESOURCE_LOAD_FAILED: return "resource_load_failed";
	case ERmlUiReason::FRAME_PHASE_VIOLATION: return "frame_phase_violation";
	}
	return "runtime_unavailable";
}

SRmlUiSurfaceResult QueryInitialRmlUiSurface(ERmlUiSurface Surface)
{
	(void)Surface;
	return {ERmlUiSurfaceStatus::NOT_IMPLEMENTED, ERmlUiReason::SURFACE_NOT_IMPLEMENTED};
}
```

- [ ] **步骤 4：接入构建**

在 `CMakeLists.txt` 的 `GAME_CLIENT` 列表中加入：

```cmake
    RmlUi/RmlUiSurface.cpp
    RmlUi/RmlUiSurface.h
```

在 `TESTS` 列表中加入：

```cmake
    rmlui_surface_registry_test.cpp
```

在 `TESTS_EXTRA` 中加入：

```cmake
    src/game/client/RmlUi/RmlUiSurface.cpp
```

- [ ] **步骤 5：运行测试验证通过**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：`RmlUiSurfaceRegistry.*` 测试 PASS。

- [ ] **步骤 6：Commit**

```powershell
git add src\game\client\RmlUi\RmlUiSurface.h src\game\client\RmlUi\RmlUiSurface.cpp src\test\rmlui_surface_registry_test.cpp CMakeLists.txt
git commit -m "feat: add RmlUI surface registry reasons"
```

## 任务 4：RmlUI frame contract 测试夹具

**文件：**
- 创建：`src/game/client/RmlUi/RmlUiFrameContract.h`
- 创建：`src/game/client/RmlUi/RmlUiFrameContract.cpp`
- 创建：`src/test/rmlui_frame_contract_test.cpp`
- 修改：`CMakeLists.txt`

- [ ] **步骤 1：编写失败测试**

创建 `src/test/rmlui_frame_contract_test.cpp`：

```cpp
#include "test.h"

#include <game/client/RmlUi/RmlUiFrameContract.h>

#include <gtest/gtest.h>

TEST(RmlUiFrameContract, AllowsInputBeforeUpdate)
{
	CRmlUiFrameContract Contract;
	EXPECT_TRUE(Contract.BeginFrame());
	EXPECT_TRUE(Contract.SubmitInput());
	EXPECT_TRUE(Contract.Update());
	EXPECT_TRUE(Contract.Render());
	EXPECT_EQ(Contract.LastReason(), ERmlUiReason::NONE);
}

TEST(RmlUiFrameContract, RejectsInputBetweenUpdateAndRender)
{
	CRmlUiFrameContract Contract;
	EXPECT_TRUE(Contract.BeginFrame());
	EXPECT_TRUE(Contract.Update());
	EXPECT_FALSE(Contract.SubmitInput());
	EXPECT_EQ(Contract.LastReason(), ERmlUiReason::FRAME_PHASE_VIOLATION);
}

TEST(RmlUiFrameContract, RejectsDataMutationBetweenUpdateAndRender)
{
	CRmlUiFrameContract Contract;
	EXPECT_TRUE(Contract.BeginFrame());
	EXPECT_TRUE(Contract.Update());
	EXPECT_FALSE(Contract.MutateDataModel());
	EXPECT_EQ(Contract.LastReason(), ERmlUiReason::FRAME_PHASE_VIOLATION);
}
```

- [ ] **步骤 2：运行测试验证失败**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：构建失败，错误包含找不到 `RmlUiFrameContract.h`。

- [ ] **步骤 3：实现 frame contract**

创建 `src/game/client/RmlUi/RmlUiFrameContract.h`：

```cpp
#ifndef GAME_CLIENT_RMLUI_RMLUI_FRAME_CONTRACT_H
#define GAME_CLIENT_RMLUI_RMLUI_FRAME_CONTRACT_H

#include "RmlUiSurface.h"

enum class ERmlUiFramePhase
{
	IDLE,
	INPUT,
	UPDATED,
	RENDERED,
};

class CRmlUiFrameContract
{
public:
	bool BeginFrame();
	bool SubmitInput();
	bool MutateDataModel();
	bool Update();
	bool Render();
	ERmlUiReason LastReason() const { return m_LastReason; }

private:
	bool FailPhaseViolation();

	ERmlUiFramePhase m_Phase = ERmlUiFramePhase::IDLE;
	ERmlUiReason m_LastReason = ERmlUiReason::NONE;
};

#endif
```

创建 `src/game/client/RmlUi/RmlUiFrameContract.cpp`：

```cpp
#include "RmlUiFrameContract.h"

bool CRmlUiFrameContract::BeginFrame()
{
	m_Phase = ERmlUiFramePhase::INPUT;
	m_LastReason = ERmlUiReason::NONE;
	return true;
}

bool CRmlUiFrameContract::SubmitInput()
{
	if(m_Phase != ERmlUiFramePhase::INPUT)
		return FailPhaseViolation();
	return true;
}

bool CRmlUiFrameContract::MutateDataModel()
{
	if(m_Phase != ERmlUiFramePhase::INPUT)
		return FailPhaseViolation();
	return true;
}

bool CRmlUiFrameContract::Update()
{
	if(m_Phase != ERmlUiFramePhase::INPUT)
		return FailPhaseViolation();
	m_Phase = ERmlUiFramePhase::UPDATED;
	return true;
}

bool CRmlUiFrameContract::Render()
{
	if(m_Phase != ERmlUiFramePhase::UPDATED)
		return FailPhaseViolation();
	m_Phase = ERmlUiFramePhase::RENDERED;
	return true;
}

bool CRmlUiFrameContract::FailPhaseViolation()
{
	m_LastReason = ERmlUiReason::FRAME_PHASE_VIOLATION;
	return false;
}
```

- [ ] **步骤 4：接入构建**

在 `GAME_CLIENT` 列表中加入：

```cmake
    RmlUi/RmlUiFrameContract.cpp
    RmlUi/RmlUiFrameContract.h
```

在 `TESTS` 列表中加入：

```cmake
    rmlui_frame_contract_test.cpp
```

在 `TESTS_EXTRA` 中加入：

```cmake
    src/game/client/RmlUi/RmlUiFrameContract.cpp
```

- [ ] **步骤 5：运行测试验证通过**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：`RmlUiFrameContract.*` 测试 PASS。

- [ ] **步骤 6：Commit**

```powershell
git add src\game\client\RmlUi\RmlUiFrameContract.h src\game\client\RmlUi\RmlUiFrameContract.cpp src\test\rmlui_frame_contract_test.cpp CMakeLists.txt
git commit -m "test: lock RmlUI frame phase contract"
```

## 任务 5：接入 RmlUI v6.2 依赖

**文件：**
- 修改：`CMakeLists.txt`
- 新增或确认：`ddnet-libs/rmlui`

- [ ] **步骤 1：确认依赖来源**

在仓库根目录运行：

```powershell
Test-Path ddnet-libs\rmlui
```

预期：当前分支返回 `False`。

- [ ] **步骤 2：引入 RmlUI v6.2 源码**

使用项目接受的 dependency 管理方式将 RmlUI v6.2 固定到 `ddnet-libs/rmlui`。如果使用 git submodule，命令为：

```powershell
git submodule add https://github.com/mikke89/RmlUi.git ddnet-libs/rmlui
git -C ddnet-libs\rmlui fetch --tags
git -C ddnet-libs\rmlui checkout 6.2
```

验证：

```powershell
git -C ddnet-libs\rmlui describe --tags --exact-match
```

预期：输出 `6.2` 或项目认可的 `6.2.x` tag。

- [ ] **步骤 3：CMake 接入源码构建**

在 `CMakeLists.txt` 的依赖配置区域加入：

```cmake
if(CLIENT)
  set(RMLUI_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
  set(RMLUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(RMLUI_BUILD_LUA_BINDINGS OFF CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  add_subdirectory(ddnet-libs/rmlui EXCLUDE_FROM_ALL)
endif()
```

在 `game-client` 链接库区域加入：

```cmake
    rmlui_core
```

在 `target_include_directories(${TARGET_CLIENT}` 的 private include 区域加入：

```cmake
    ddnet-libs/rmlui/Include
```

- [ ] **步骤 4：构建验证**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30
```

预期：`game-client` 链接成功；如果 RmlUI target 名称不是 `rmlui_core`，以 RmlUI v6.2 CMake 导出的实际 target 为准，只改 CMake target 名称，不改运行时设计。

- [ ] **步骤 5：Commit**

```powershell
git add .gitmodules ddnet-libs\rmlui CMakeLists.txt
git commit -m "build: add RmlUI v6.2 dependency"
```

## 任务 6：RmlUI runtime owner 与 interface 生命周期

**文件：**
- 创建：`src/game/client/RmlUi/RmlUiRuntime.h`
- 创建：`src/game/client/RmlUi/RmlUiRuntime.cpp`
- 创建：`src/game/client/RmlUi/RmlUiSystemInterface.h`
- 创建：`src/game/client/RmlUi/RmlUiSystemInterface.cpp`
- 创建：`src/game/client/RmlUi/RmlUiFileInterface.h`
- 创建：`src/game/client/RmlUi/RmlUiFileInterface.cpp`
- 创建：`src/game/client/RmlUi/RmlUiRenderInterface.h`
- 创建：`src/game/client/RmlUi/RmlUiRenderInterface.cpp`
- 修改：`CMakeLists.txt`

- [ ] **步骤 1：声明 runtime 状态接口**

创建 `src/game/client/RmlUi/RmlUiRuntime.h`：

```cpp
#ifndef GAME_CLIENT_RMLUI_RMLUI_RUNTIME_H
#define GAME_CLIENT_RMLUI_RMLUI_RUNTIME_H

#include "RmlUiFrameContract.h"
#include "RmlUiSurface.h"

class CGameClient;

enum class ERmlUiRuntimeState
{
	UNINITIALIZED,
	READY,
	UNAVAILABLE,
	SHUTDOWN,
};

struct SRmlUiRuntimeDiagnostics
{
	ERmlUiRuntimeState m_State = ERmlUiRuntimeState::UNINITIALIZED;
	ERmlUiReason m_Reason = ERmlUiReason::NONE;
	char m_aContextName[32] = "qm_ui";
	char m_aDocumentPath[128] = "qmclient/rmlui/ui_unavailable.rml";
};

class CRmlUiRuntime
{
public:
	bool Init(CGameClient *pGameClient);
	void Shutdown();
	void BeginFrame();
	bool RenderUnavailableSurface(ERmlUiSurface Surface, ERmlUiReason Reason);
	const SRmlUiRuntimeDiagnostics &Diagnostics() const { return m_Diagnostics; }
	bool Ready() const { return m_Diagnostics.m_State == ERmlUiRuntimeState::READY; }

private:
	void SetUnavailable(ERmlUiReason Reason);
	void LogSurfaceResult(ERmlUiSurface Surface, ERmlUiSurfaceStatus Status, ERmlUiReason Reason) const;

	CGameClient *m_pGameClient = nullptr;
	CRmlUiFrameContract m_FrameContract;
	SRmlUiRuntimeDiagnostics m_Diagnostics;
};

#endif
```

- [ ] **步骤 2：实现 system/file/render interface 文件骨架**

创建 `RmlUiSystemInterface.h` / `.cpp`，类名为 `CQmRmlUiSystemInterface`，继承 `Rml::SystemInterface`，至少实现 `GetElapsedTime()` 和 `LogMessage(...)`，日志格式：

```cpp
dbg_msg("rmlui_ui_stack", "level=%d message=%s", (int)Type, Message.c_str());
```

创建 `RmlUiFileInterface.h` / `.cpp`，类名为 `CQmRmlUiFileInterface`，继承 `Rml::FileInterface`，路径只允许通过 `IStorage` 读取 `qmclient/rmlui/` 下资源，读取失败不崩溃。

创建 `RmlUiRenderInterface.h` / `.cpp`，类名为 `CQmRmlUiRenderInterface`，继承 `Rml::RenderInterface`，实现 RmlUI v6.2 必需方法：

```cpp
Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> Vertices, Rml::Span<const int> Indices) override;
void RenderGeometry(Rml::CompiledGeometryHandle Geometry, Rml::Vector2f Translation, Rml::TextureHandle Texture) override;
void ReleaseGeometry(Rml::CompiledGeometryHandle Geometry) override;
void EnableScissorRegion(bool Enable) override;
void SetScissorRegion(Rml::Rectanglei Region) override;
Rml::TextureHandle LoadTexture(Rml::Vector2i &TextureDimensions, const Rml::String &Source) override;
Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> Source, Rml::Vector2i SourceDimensions) override;
void ReleaseTexture(Rml::TextureHandle Texture) override;
```

第一阶段 `LoadTexture` / `GenerateTexture` 返回 `0` 并记录 `resource_load_failed`，不可用界面不得依赖图片。

- [ ] **步骤 3：实现 runtime 初始化顺序**

在 `RmlUiRuntime.cpp` 中按以下顺序实现：

```cpp
bool CRmlUiRuntime::Init(CGameClient *pGameClient)
{
	m_pGameClient = pGameClient;
	m_Diagnostics = {};
	m_Diagnostics.m_State = ERmlUiRuntimeState::UNAVAILABLE;

	if(m_pGameClient == nullptr)
	{
		SetUnavailable(ERmlUiReason::RUNTIME_UNAVAILABLE);
		return false;
	}

	// 1. interfaces 必须先于 Rml::Initialise() 安装，并且作为 runtime 成员活到 Shutdown 之后。
	// 2. Rml::Initialise()
	// 3. Rml::CreateContext("qm_ui", window size)
	// 4. Rml::LoadFontFace(...)
	// 5. context->LoadDocument("qmclient/rmlui/ui_unavailable.rml")
	// 6. document->Show()
	m_Diagnostics.m_State = ERmlUiRuntimeState::READY;
	m_Diagnostics.m_Reason = ERmlUiReason::NONE;
	return true;
}
```

实现时用真实 `Rml::SetRenderInterface`、`Rml::SetSystemInterface`、`Rml::SetFileInterface`、`Rml::Initialise()`、`Rml::CreateContext()`、`Rml::LoadFontFace()`、`Context::LoadDocument()`。任一步失败调用 `SetUnavailable(...)`，不得切换到 legacy。

- [ ] **步骤 4：实现关闭顺序**

在 `CRmlUiRuntime::Shutdown()` 中按顺序执行：

```cpp
if(m_Diagnostics.m_State == ERmlUiRuntimeState::READY || m_Diagnostics.m_State == ERmlUiRuntimeState::UNAVAILABLE)
{
	Rml::Shutdown();
}
m_Diagnostics.m_State = ERmlUiRuntimeState::SHUTDOWN;
```

`CQmRmlUiRenderInterface`、`CQmRmlUiSystemInterface`、`CQmRmlUiFileInterface` 必须是 `CRmlUiRuntime` 成员，不能是局部变量。

- [ ] **步骤 5：接入构建**

在 `GAME_CLIENT` 列表中加入所有 `src/game/client/RmlUi/RmlUi*.cpp/.h` 新文件。

- [ ] **步骤 6：构建验证**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30
```

预期：`game-client` 构建成功；如果 RmlUI v6.2 抽象接口签名与代码不一致，以官方 v6.2 头文件为准修正签名。

- [ ] **步骤 7：Commit**

```powershell
git add src\game\client\RmlUi CMakeLists.txt
git commit -m "feat: add RmlUI runtime owner"
```

## 任务 7：RmlUI 内部不可用界面资源

**文件：**
- 创建：`data/qmclient/rmlui/ui_unavailable.rml`
- 创建：`data/qmclient/rmlui/ui_unavailable.rcss`

- [ ] **步骤 1：创建 RML 文档**

创建 `data/qmclient/rmlui/ui_unavailable.rml`：

```xml
<rml>
	<head>
		<title>RmlUI unavailable</title>
		<link type="text/rcss" href="qmclient/rmlui/ui_unavailable.rcss"/>
	</head>
	<body>
		<div id="shell">
			<div class="label">RmlUI</div>
			<div id="surface">surface_not_set</div>
			<div id="reason">surface_not_implemented</div>
			<div class="hint">This interface is not available in the active RmlUI stack.</div>
		</div>
	</body>
</rml>
```

- [ ] **步骤 2：创建 RCSS 样式**

创建 `data/qmclient/rmlui/ui_unavailable.rcss`：

```css
body {
	width: 100%;
	height: 100%;
	font-family: sans;
	color: #f2f2f2;
	background-color: rgba(8, 10, 14, 210);
}

#shell {
	width: 520px;
	margin-left: auto;
	margin-right: auto;
	margin-top: 160px;
	padding: 22px;
	border-width: 1px;
	border-color: #3b4a5f;
	background-color: rgba(18, 24, 32, 235);
}

.label {
	font-size: 16px;
	color: #93c5fd;
	margin-bottom: 12px;
}

#surface {
	font-size: 24px;
	margin-bottom: 8px;
}

#reason {
	font-size: 16px;
	color: #fbbf24;
	margin-bottom: 14px;
}

.hint {
	font-size: 14px;
	color: #cbd5e1;
}
```

- [ ] **步骤 3：资源检查**

运行：

```powershell
Test-Path data\qmclient\rmlui\ui_unavailable.rml
Test-Path data\qmclient\rmlui\ui_unavailable.rcss
```

预期：两行均输出 `True`。

- [ ] **步骤 4：构建验证资源进入 data glob**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30
```

预期：`game-client` 构建成功；`CMakeLists.txt` 已包含 `rml` / `rcss` data glob，不需要新增 data glob 扩展名。

- [ ] **步骤 5：Commit**

```powershell
git add data\qmclient\rmlui\ui_unavailable.rml data\qmclient\rmlui\ui_unavailable.rcss
git commit -m "feat: add RmlUI unavailable surface document"
```

## 任务 8：RmlUI stack 接入 `CGameClient`

**文件：**
- 修改：`src/game/client/gameclient.h`
- 修改：`src/game/client/gameclient.cpp`
- 修改：`src/game/client/QmUi/QmRt.cpp`

- [ ] **步骤 1：在 `CGameClient` 持有 runtime**

在 `gameclient.h` 加入：

```cpp
#include "RmlUi/RmlUiRuntime.h"
```

在 private 成员区域加入：

```cpp
	CRmlUiRuntime m_RmlUiRuntime;
```

在 public 区域加入：

```cpp
	CRmlUiRuntime &RmlUiRuntime() { return m_RmlUiRuntime; }
	const CRmlUiRuntime &RmlUiRuntime() const { return m_RmlUiRuntime; }
```

- [ ] **步骤 2：初始化与关闭 runtime**

在 `CGameClient::OnInit()` 中，冻结 `m_QmActiveUiStack` 后加入：

```cpp
	if(m_QmActiveUiStack == EQmUiStack::RMLUI)
	{
		m_RmlUiRuntime.Init(this);
	}
```

在 `CGameClient::OnShutdown()` 或现有清理入口中加入：

```cpp
	m_RmlUiRuntime.Shutdown();
```

- [ ] **步骤 3：RmlUI active 时禁用旧 QmUI runtime v2**

在 `src/game/client/QmUi/QmRt.cpp` 的 `CUiRuntimeV2::OnRender()` 开头加入：

```cpp
	if(m_pGameClient->ActiveUiStack() == EQmUiStack::RMLUI)
		return;
```

- [ ] **步骤 4：构建验证**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30
```

预期：`game-client` 构建成功。

- [ ] **步骤 5：Commit**

```powershell
git add src\game\client\gameclient.h src\game\client\gameclient.cpp src\game\client\QmUi\QmRt.cpp
git commit -m "feat: attach RmlUI runtime to active stack"
```

## 任务 9：菜单、设置、弹窗 surface 分路

**文件：**
- 修改：`src/game/client/components/menus.h`
- 修改：`src/game/client/components/menus.cpp`

- [ ] **步骤 1：添加 RmlUI surface 转发方法**

在 `menus.h` 的 private 方法区域加入：

```cpp
	bool RenderRmlUiSurface(ERmlUiSurface Surface);
```

在 `menus.cpp` 加入 include：

```cpp
#include <game/client/RmlUi/RmlUiSurface.h>
#include <game/client/ui_stack.h>
```

实现：

```cpp
bool CMenus::RenderRmlUiSurface(ERmlUiSurface Surface)
{
	const SRmlUiSurfaceResult Result = QueryInitialRmlUiSurface(Surface);
	GameClient()->RmlUiRuntime().RenderUnavailableSurface(Surface, Result.m_Reason);
	return true;
}
```

- [ ] **步骤 2：在 menu host seam 分路**

在 `CMenus::Render()` 的 `STATE_OFFLINE` 分支，调用 `m_MenusStart.RenderStartMenuV2(Screen)` 前加入：

```cpp
	if(GameClient()->ActiveUiStack() == EQmUiStack::RMLUI)
	{
		RenderRmlUiSurface(ERmlUiSurface::MAIN_MENU);
		break;
	}
```

在 `RenderSettings(MainView)` 调用点改为：

```cpp
	if(GameClient()->ActiveUiStack() == EQmUiStack::RMLUI)
		RenderRmlUiSurface(ERmlUiSurface::SETTINGS_PAGE);
	else
		RenderSettings(MainView);
```

在 `RenderPopupFullscreen(Screen)` 调用点改为：

```cpp
	if(GameClient()->ActiveUiStack() == EQmUiStack::RMLUI)
		RenderRmlUiSurface(ERmlUiSurface::FULLSCREEN_POPUP);
	else
		RenderPopupFullscreen(Screen);
```

在 `Ui()->RenderPopupMenus()` 调用点改为：

```cpp
	if(GameClient()->ActiveUiStack() == EQmUiStack::RMLUI)
		RenderRmlUiSurface(ERmlUiSurface::POPUP_MENU);
	else
		Ui()->RenderPopupMenus();
```

- [ ] **步骤 3：构建验证**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30
```

预期：`game-client` 构建成功。

- [ ] **步骤 4：Commit**

```powershell
git add src\game\client\components\menus.h src\game\client\components\menus.cpp
git commit -m "feat: route menu surfaces through RmlUI stack"
```

## 任务 10：游戏内 overlay、wheel、HUD editor 分路

**文件：**
- 修改：`src/game/client/components/debughud.cpp`
- 修改：`src/game/client/components/hud_editor.cpp`
- 修改：`src/game/client/components/pie_menu.cpp`
- 修改：`src/game/client/components/tclient/bindwheel.cpp`
- 修改：`src/game/client/components/emoticon.cpp`

- [ ] **步骤 1：debug overlay 分路**

在 `debughud.cpp` 的 `CDebugHud::OnRender()` 开头加入：

```cpp
	if(GameClient()->ActiveUiStack() == EQmUiStack::RMLUI)
	{
		const SRmlUiSurfaceResult Result = QueryInitialRmlUiSurface(ERmlUiSurface::DEBUG_OVERLAY);
		GameClient()->RmlUiRuntime().RenderUnavailableSurface(ERmlUiSurface::DEBUG_OVERLAY, Result.m_Reason);
		return;
	}
```

- [ ] **步骤 2：HUD editor 分路**

在 `hud_editor.cpp` 的 `CHudEditor::OnRender()` 开头加入：

```cpp
	if(GameClient()->ActiveUiStack() == EQmUiStack::RMLUI)
	{
		const SRmlUiSurfaceResult Result = QueryInitialRmlUiSurface(ERmlUiSurface::HUD_EDITOR);
		GameClient()->RmlUiRuntime().RenderUnavailableSurface(ERmlUiSurface::HUD_EDITOR, Result.m_Reason);
		return;
	}
```

- [ ] **步骤 3：click GUI 分路**

在 `pie_menu.cpp` 的 `CPieMenu::OnRender()` 开头、`m_Active` 检查之后加入：

```cpp
	if(GameClient()->ActiveUiStack() == EQmUiStack::RMLUI)
	{
		const SRmlUiSurfaceResult Result = QueryInitialRmlUiSurface(ERmlUiSurface::CLICK_GUI);
		GameClient()->RmlUiRuntime().RenderUnavailableSurface(ERmlUiSurface::CLICK_GUI, Result.m_Reason);
		return;
	}
```

- [ ] **步骤 4：bind wheel 分路**

在 `src/game/client/components/tclient/bindwheel.cpp` 的 `CBindWheel::OnRender()` 开头加入：

```cpp
	if(GameClient()->ActiveUiStack() == EQmUiStack::RMLUI)
	{
		const SRmlUiSurfaceResult Result = QueryInitialRmlUiSurface(ERmlUiSurface::BIND_WHEEL);
		GameClient()->RmlUiRuntime().RenderUnavailableSurface(ERmlUiSurface::BIND_WHEEL, Result.m_Reason);
		return;
	}
```

- [ ] **步骤 5：emote wheel 分路**

在 `src/game/client/components/emoticon.cpp` 的 `CEmoticon::OnRender()` 中，client state 检查之后加入：

```cpp
	if(GameClient()->ActiveUiStack() == EQmUiStack::RMLUI)
	{
		const SRmlUiSurfaceResult Result = QueryInitialRmlUiSurface(ERmlUiSurface::EMOTE_WHEEL);
		GameClient()->RmlUiRuntime().RenderUnavailableSurface(ERmlUiSurface::EMOTE_WHEEL, Result.m_Reason);
		return;
	}
```

- [ ] **步骤 6：构建验证**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30
```

预期：`game-client` 构建成功。

- [ ] **步骤 7：Commit**

```powershell
git add src\game\client\components\debughud.cpp src\game\client\components\hud_editor.cpp src\game\client\components\pie_menu.cpp src\game\client\components\tclient\bindwheel.cpp src\game\client\components\emoticon.cpp
git commit -m "feat: route in-game UI surfaces through RmlUI stack"
```

## 任务 11：设置页显示“重启生效”

**文件：**
- 修改：`src/game/client/components/menus_settings.cpp`

- [ ] **步骤 1：在 settings restart bar 纳入 UI stack**

在 `RenderSettings` 中计算 `NeedRestart` 的位置，将：

```cpp
const bool NeedRestart = m_NeedRestartGraphics || m_NeedRestartSound || m_NeedRestartUpdate;
```

改为：

```cpp
const bool NeedRestartUiStack = GameClient()->UiStackState().m_RestartRequired;
const bool NeedRestart = m_NeedRestartGraphics || m_NeedRestartSound || m_NeedRestartUpdate || NeedRestartUiStack;
```

- [ ] **步骤 2：在 QmClient 设置区域增加下拉项**

在 QmClient 设置页面的通用配置卡片中加入：

```cpp
static const char *s_apUiStackNames[] = {Localize("旧 UI"), "RmlUI"};
static const char *s_apUiStackValues[] = {"legacy", "rmlui"};
int CurrentUiStack = str_comp_nocase(g_Config.m_QmUiStack, "rmlui") == 0 ? 1 : 0;
static CUi::SDropDownState s_UiStackDropDownState;
const int NewUiStack = Ui()->DoDropDown(&ControlCol, CurrentUiStack, s_apUiStackNames, std::size(s_apUiStackNames), s_UiStackDropDownState);
if(NewUiStack != CurrentUiStack)
	str_copy(g_Config.m_QmUiStack, s_apUiStackValues[NewUiStack], sizeof(g_Config.m_QmUiStack));
```

在同一块区域，如果 `GameClient()->UiStackState().m_RestartRequired` 为 true，绘制一行：

```cpp
Ui()->DoLabel(&ControlCol, Localize("重启客户端后生效"), LG_BodySize * 0.85f, TEXTALIGN_ML);
```

- [ ] **步骤 3：构建验证**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30
```

预期：`game-client` 构建成功。

- [ ] **步骤 4：Commit**

```powershell
git add src\game\client\components\menus_settings.cpp
git commit -m "feat: show restart notice for UI stack changes"
```

## 任务 12：最终验证与审查

**文件：**
- 修改：无固定代码文件

- [ ] **步骤 1：运行 C++ 测试**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

预期：新增 `QmUiStack.*`、`RmlUiSurfaceRegistry.*`、`RmlUiFrameContract.*` 全部 PASS，既有测试无新增失败。

- [ ] **步骤 2：构建客户端**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30
```

预期：构建成功。

- [ ] **步骤 3：快速 gate**

运行：

```powershell
qmclient_scripts\check-gate.ps1 -Mode quick -BaseRef main
```

预期：不出现由本次改动引入的 FAIL。

- [ ] **步骤 4：人工运行验证**

从构建目录启动：

```powershell
cd build-ninja
.\DDNet.exe
```

验证：

- `qm_ui_stack legacy`：旧 UI 正常显示，RmlUI 不作为 active UI 绘制。
- `qm_ui_stack rmlui` 后重启：菜单、设置、弹窗、overlay、wheel、HUD editor 入口显示 RmlUI 内部不可用界面。
- RmlUI 模式日志包含稳定字段，例如：

```text
rmlui_ui_stack active=rmlui surface=settings_page status=not_implemented reason=surface_not_implemented
```

- [ ] **步骤 5：派发子代理代码审查**

实现影响核心 UI 生命周期，完成本计划后派发一个子代理进行 `/cs-audit` 代码审查。子代理提示词：

```text
请按 /cs-audit 审查当前分支的 RmlUI 全局 UI 栈改动，只审查代码与构建/测试接入，不修改文件。重点看：
1. RmlUI 模式是否仍可能调用 legacy UI 作为 active surface fallback；
2. qm_ui_stack 是否启动后冻结，修改后仅提示重启生效；
3. RmlUI v6.2 初始化/Shutdown/interface 生命周期是否有悬垂风险；
4. Context::Update 与 Render 之间是否存在 input/DOM/data model mutation；
5. reason 是否为稳定字符串且覆盖 runtime/document/surface 失败；
6. 新增测试是否锁住 selector、surface registry、frame contract。
请给出按严重程度排序的中文 findings 和残余风险。
```

预期：等待子代理完成并读取报告。若有必须修复项，回到对应任务补测试和修复；若无必须修复项，记录审查结论。

## 自检

- 规格覆盖：配置默认 legacy、启动冻结、重启提示、RmlUI/legacy 分离、无 legacy fallback、未实现 surface reason、RmlUI v6.2 初始化/帧顺序/Shutdown 约束均有对应任务。
- 类型一致性：`EQmUiStack`、`SQmUiStackState`、`ERmlUiSurface`、`ERmlUiReason`、`CRmlUiFrameContract`、`CRmlUiRuntime` 在首次使用前均已定义。
- 风险边界：不修改协议、物理、demo、地图格式；RmlUI 依赖接入作为独立 commit；每个核心语义先有测试。
