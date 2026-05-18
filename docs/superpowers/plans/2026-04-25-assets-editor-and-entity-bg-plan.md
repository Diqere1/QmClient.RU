# 资源页、实体层背景图与素材编辑器扩展实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 统一 `entity_bg` 的目录视图与商店资源显示模型，新增资源页商店资源显示开关，并扩展编辑器以支持 `strong_weak`、`skin` 与 `audio pack` 的可编辑、可导出工作流。

**架构：** 先把 `entity_bg` 层级项构建、排序和商店资源显示开关抽成可测试的纯逻辑，再让资源页 UI 接入这套模型；随后把材质编辑器的 slot 切分定义显式化，用纯布局 helper 冻结 `strong_weak` 与 `skin` 的切片语义；最后单独做 `audio pack` 编辑器，使其导出到 `audio/<pack>/...` 并复用现有声音包加载链路。

**技术栈：** C++17, gtest, DDNet/QmClient UI system, CMake, Windows Ninja build

---

## 执行策略

本计划按“先冻结纯逻辑，再改大 UI 文件，再接导出链路”的顺序执行：

1. 先把 `entity_bg` 目录项、排序、Workshop 开关与缓存恢复规则写成可测试 helper，避免在 `menus_settings_assets.cpp` 里边改边猜。
2. 再收敛资源页中的 `entity_bg` 与 Workshop 显示逻辑，确保根页、子目录页和搜索态只剩一套行为。
3. 然后把现有材质编辑器的小格子布局抽成显式定义，先落 `strong_weak`，再落 `skin`。
4. 最后单独实现 `audio pack` 编辑器，避免把贴图编辑和声音映射 UI 混在一个屏幕里。

这样拆分后：

- `entity_bg` 的运行时风险和资源页风险可以最早被测试挡住。
- `skin` 与 `strong_weak` 可以复用同一套编辑器基础，不必重复实现交互。
- `audio pack` 保持独立交付，不会拖慢资源页和贴图编辑主线。

## 文件清单

| 文件 | 职责 |
|------|------|
| `docs/superpowers/specs/2026-04-25-assets-editor-and-entity-bg-design.md` | 本计划对应的设计规格 |
| `docs/superpowers/plans/2026-04-25-assets-editor-and-entity-bg-plan.md` | 本实现计划 |
| `src/game/client/components/menus.h` | `CMenus` 状态、编辑器类型、资源页与编辑器入口声明 |
| `src/game/client/components/menus_settings_assets.cpp` | 资源页主逻辑、`entity_bg` 层级视图、Workshop 开关、卡片渲染与选择/删除 |
| `src/game/client/components/menus_settings.cpp` | `audio pack` 设置页入口、编辑器启动入口、声音包列表刷新 |
| `src/game/client/components/menus_assets_editor.cpp` | 贴图编辑器主逻辑、`strong_weak` / `skin` 模式、着色与导出 |
| `src/game/client/components/assets_resource_registry.h` | 资源页分类 helper、受保护默认资源 helper |
| `src/game/client/components/assets_resource_registry.cpp` | 资源页分类 helper 实现与名称排序辅助 |
| `src/game/client/components/background.h` | `entity_bg` 配置与运行时路径 helper |
| `src/game/client/components/background.cpp` | `entity_bg` 运行时加载逻辑与路径规范化 |
| `src/game/client/components/skins7.cpp` | 皮肤 JSON 写回与现有皮肤保存格式对齐点 |
| `src/game/client/components/sounds.cpp` | 运行时声音包加载路径规则 |
| `data/languages/simplified_chinese.txt` | 新增与修改文案的简体中文翻译 |
| `src/test/assets_resource_registry_test.cpp` | `entity_bg` 层级、排序、Workshop 可见性、缓存恢复等纯逻辑测试 |
| `src/test/assets_editor_layout_test.cpp` | `strong_weak` / `skin` slot 布局与导出映射测试 |
| `src/test/audio_pack_editor_test.cpp` | 声音槽位映射与导出路径规则测试 |
| `CMakeLists.txt` | 新增测试文件时更新测试列表 |

---

### 任务 1：冻结 `entity_bg` 层级与商店显示开关的纯逻辑

**文件：**
- 修改：`src/game/client/components/menus.h`
- 修改：`src/game/client/components/assets_resource_registry.h`
- 修改：`src/game/client/components/assets_resource_registry.cpp`
- 修改：`src/test/assets_resource_registry_test.cpp`

- [x] **步骤 1：为 `entity_bg` 目录项增加来源与显示语义结构**

在 `menus.h` 中把 `SEntityBgHierarchyEntry` 扩成最小可测试结构，至少带上“显示名、内部路径、是否目录、来源类型”。

```cpp
enum class EEntityBgSourceKind
{
	MAPS,
	ASSETS_ENTITY_BG,
	WORKSHOP_VIRTUAL,
};

struct SEntityBgHierarchyEntry
{
	char m_aName[IO_MAX_PATH_LENGTH] = {0};
	char m_aDisplayName[IO_MAX_PATH_LENGTH] = {0};
	bool m_IsDirectory = false;
	EEntityBgSourceKind m_SourceKind = EEntityBgSourceKind::MAPS;
};
```

- [x] **步骤 2：把 `entity_bg` 根页排序规则写成失败测试**

在 `src/test/assets_resource_registry_test.cpp` 中增加以下测试，先让它们失败：

```cpp
TEST(AssetsResourceRegistry, EntityBgRootSortsDirectoriesBeforeDefaultAndFiles)
{
	std::vector<std::string> vNames = {
		"default",
		"zeta.map",
		"folder_a/alpha.map",
		"folder_b/beta.map",
	};

	const auto vEntries = BuildEntityBgHierarchyEntries(vNames, "", true);

	ASSERT_GE(vEntries.size(), 4u);
	EXPECT_TRUE(vEntries[0].m_IsDirectory);
	EXPECT_TRUE(vEntries[1].m_IsDirectory);
	EXPECT_STREQ(vEntries[2].m_aName, "default");
	EXPECT_FALSE(vEntries[3].m_IsDirectory);
}

TEST(AssetsResourceRegistry, EntityBgWorkshopFolderOnlyAppearsWhenWorkshopVisible)
{
	std::vector<std::string> vNames = {"default", "entity_bg/workshop_one.map"};

	EXPECT_FALSE(HasEntityBgWorkshopFolder(BuildEntityBgHierarchyEntries(vNames, "", false)));
	EXPECT_TRUE(HasEntityBgWorkshopFolder(BuildEntityBgHierarchyEntries(vNames, "", true)));
}
```

- [x] **步骤 3：为 Workshop cache 恢复逻辑写失败测试**

继续在 `assets_resource_registry_test.cpp` 中加入一个只测字符串恢复的用例，不碰 IO：

```cpp
TEST(AssetsResourceRegistry, EntityBgWorkshopLocalNameCanBeRebuiltFromInstallPath)
{
	EXPECT_EQ(RebuildEntityBgWorkshopLocalName("assets/entity_bg/my_pack.map"), "entity_bg/my_pack");
	EXPECT_EQ(RebuildEntityBgWorkshopLocalName("assets/entity_bg/folder/my_pack.map"), "entity_bg/folder/my_pack");
}
```

- [x] **步骤 4：实现纯 helper，并让测试绿灯**

在 `assets_resource_registry.h/.cpp` 中补齐以下接口：

```cpp
bool HasEntityBgWorkshopFolder(const std::vector<SEntityBgHierarchyEntry> &vEntries);
std::string RebuildEntityBgWorkshopLocalName(std::string_view InstallPath);
bool EntityBgHierarchyEntryLess(const SEntityBgHierarchyEntry &Left, const SEntityBgHierarchyEntry &Right);
```

要求：

- `default` 固定排在目录之后、普通文件之前
- `..` 仍然保留最高优先级
- 是否显示 Workshop 根目录由布尔开关决定

- [x] **步骤 5：运行定向测试**

  结果：`assets_resource_registry_test.cpp` 中 `entity_bg` 层级、Workshop 可见性与安装路径恢复相关用例已随 `run_cxx_tests` 通过。

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 | Select-Object -Last 120
```

预期：

- `AssetsResourceRegistry.EntityBgRootSortsDirectoriesBeforeDefaultAndFiles` 通过
- `AssetsResourceRegistry.EntityBgWorkshopLocalNameCanBeRebuiltFromInstallPath` 通过

- [ ] **步骤 6：提交本任务**

  现状：未提交。本任务代码已在当前脏工作区中，且与后续资源页/背景相关改动交织，尚未整理成独立 commit。

  阻塞：本轮没有做分任务提交，用户也明确要求不要误回退无关改动。

  下一步建议：若要补历史提交，先按文件集合确认无跨任务混入，再单独 stage 任务 1 相关文件。

```bash
git add src/game/client/components/menus.h src/game/client/components/assets_resource_registry.h src/game/client/components/assets_resource_registry.cpp src/test/assets_resource_registry_test.cpp
git commit -m "test(assets): lock entity_bg hierarchy and workshop visibility semantics"
```

---

### 任务 2：把资源页切到新的 `entity_bg` 层级模型，并接入商店资源显示开关

**文件：**
- 修改：`src/game/client/components/menus.h`
- 修改：`src/game/client/components/menus_settings_assets.cpp`
- 修改：`src/game/client/components/background.h`
- 修改：`src/game/client/components/background.cpp`
- 修改：`data/languages/simplified_chinese.txt`

- [x] **步骤 1：为资源页增加会话级 Workshop 可见性状态**

在 `menus.h` 中为 `CMenus` 增加资源页会话状态，默认开启：

```cpp
bool m_ShowWorkshopAssets = true;
```

并在 `menus_settings_assets.cpp` 底部工具区加入两个按钮文案：

```cpp
Localize("Show Workshop Assets");
Localize("Sync Workshop Assets");
```

- [x] **步骤 2：把旧文案替换成新的同步/显示职责**

在 `data/languages/simplified_chinese.txt` 中补齐或替换：

```txt
Sync Workshop Assets
== 同步商店资源

Show Workshop Assets
== 显示商店资源
```

同时保留英文 key 不变，避免改动过大。

- [x] **步骤 3：重写 `RefreshEntityBgHierarchyView()` 的来源聚合**

在 `menus_settings_assets.cpp` 中让 `RefreshEntityBgHierarchyView()`：

- 始终通过同一套 helper 构造 `vEntries`
- 只在 `m_ShowWorkshopAssets == true` 时加入 `assets/entity_bg (Workshop)` 根目录
- 在当前目录失效时自动回退根目录
- 在刷新前保留并清空 preview epoch / decode queue

核心调用目标形态：

```cpp
vEntries = BuildEntityBgHierarchyEntries(
	m_vEntityBgSourceNames,
	m_aEntityBgCurrentFolder,
	m_ShowWorkshopAssets);
```

- [x] **步骤 4：修复 `entity_bg` 的路径与缓存闭环**

在 `menus_settings_assets.cpp` 与 `background.cpp` 中补上两件事：

1. Workshop cache 加载后若 `m_LocalName` 为空，则从 `install_path` 重建。
2. `entity_bg` 的运行时路径选择继续和资源页的显示来源对齐。

最小实现方向：

```cpp
if(Asset.m_LocalName.empty())
	Asset.m_LocalName = RebuildEntityBgWorkshopLocalName(Asset.m_InstallPath);
```

- [x] **步骤 5：扩大 `entity_bg` 中转缓冲区，避免路径截断**

把删除确认、待删除记录、当前目录与当前选中项涉及的 `50` 字节数组统一改为 `IO_MAX_PATH_LENGTH`。

```cpp
static char s_aPendingDeleteName[IO_MAX_PATH_LENGTH];
```

- [x] **步骤 6：接入普通资源页的 Workshop 可见性开关**

让普通资源页 tab 在 `m_ShowWorkshopAssets == false` 时：

- 不渲染未下载远端项
- 不显示相关远端占位状态
- 本地项维持原逻辑

建议在计算 `vVisibleDownloadableAssetIndices` 前统一短路：

```cpp
if(!m_ShowWorkshopAssets)
	vVisibleDownloadableAssetIndices.clear();
```

- [x] **步骤 7：运行构建与回归测试**

  结果：`run_cxx_tests` 已通过；`game-client` 重新链接时再次遇到 `DDNet.exe` 被占用导致 `LNK1104`，属于本地运行中的可执行文件锁定，不是编译错误。

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 60
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 | Select-Object -Last 120
```

预期：

- `DDNet.exe` 成功链接
- `run_cxx_tests` 全绿

- [ ] **步骤 8：提交本任务**

  现状：未提交。任务 2 的代码已在当前分支工作区内，但仍与任务 4 的汉化和 UI 收口共享文件。

  阻塞：`data/languages/simplified_chinese.txt`、`menus_settings_assets.cpp` 等文件在后续轮次又继续被修改，不适合直接按最初切分提交。

  下一步建议：等本轮资源页相关 UI 验收完成后，再按“entity_bg 资源页统一”主题做一次合并提交更稳妥。

```bash
git add src/game/client/components/menus.h src/game/client/components/menus_settings_assets.cpp src/game/client/components/background.h src/game/client/components/background.cpp data/languages/simplified_chinese.txt
git commit -m "feat(assets): unify entity_bg hierarchy and workshop visibility toggle"
```

---

### 任务 3：把贴图编辑器的小格子布局显式化，并先完成 `strong_weak`

**文件：**
- 修改：`src/game/client/components/menus.h`
- 修改：`src/game/client/components/menus_assets_editor.cpp`
- 创建：`src/test/assets_editor_layout_test.cpp`
- 修改：`CMakeLists.txt`

- [x] **步骤 1：提炼可测试的 slot 布局 helper**

在 `menus_assets_editor.cpp` 中把 slot 初始化逻辑收敛为独立 helper，至少支持：

```cpp
std::vector<CMenus::SAssetsEditorPartSlot> BuildStrongWeakEditorSlots(const char *pMainAssetName);
std::vector<CMenus::SAssetsEditorPartSlot> BuildSkinEditorSlots(const char *pMainAssetName);
```

`strong_weak` 的目标布局已按最新验收改为横向三段、每段保持 `1x1` 方格语义：

```cpp
Slot0: DstX=0, DstY=0, DstW=1, DstH=1
Slot1: DstX=1, DstY=0, DstW=1, DstH=1
Slot2: DstX=2, DstY=0, DstW=1, DstH=1
```

- [x] **步骤 2：先写 `strong_weak` 布局失败测试**

在 `src/test/assets_editor_layout_test.cpp` 中加入：

```cpp
TEST(AssetsEditorLayout, StrongWeakUsesThreeHorizontalSquareSlots)
{
	const auto vSlots = BuildStrongWeakEditorSlots("default");

	ASSERT_EQ(vSlots.size(), 3u);
	EXPECT_EQ(vSlots[0].m_DstW, 1);
	EXPECT_EQ(vSlots[0].m_DstH, 1);
	EXPECT_EQ(vSlots[1].m_DstX, 1);
	EXPECT_EQ(vSlots[2].m_DstX, 2);
}
```

- [x] **步骤 3：把编辑器初始化接到 `strong_weak` helper**

在 `AssetsEditorResetPartSlots()` 中，把 `ASSETS_EDITOR_TYPE_STRONG_WEAK` 分支改成：

```cpp
if(m_AssetsEditorState.m_Type == ASSETS_EDITOR_TYPE_STRONG_WEAK)
{
	m_AssetsEditorState.m_vPartSlots = BuildStrongWeakEditorSlots(pMainAssetName);
	return;
}
```

- [x] **步骤 4：确保交互仍沿用现有短按/长按/右键规则**

  结果：颜色选择、长按拖拽替换、右键重置与颜色选择器遮挡后的输入阻断已接入当前编辑器流程。

不改核心交互判定阈值，只验证新的三段布局能走通：

- 左键短按进入颜色选择器
- 长按或拖动进入替换
- 右键重置当前格子

若 `ColorPickerOpen` 时仍有背后控件会吃输入，在同一任务中补上结构性阻断。

- [x] **步骤 5：注册测试并运行**

  结果：`assets_editor_layout_test.cpp` 已纳入测试列表，`run_cxx_tests` 通过。

在 `CMakeLists.txt` 测试列表中加入：

```cmake
    assets_editor_layout_test.cpp
```

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 | Select-Object -Last 120
```

预期：

- `AssetsEditorLayout.StrongWeakUsesThreeHorizontalSquareSlots` 通过

- [ ] **步骤 6：提交本任务**

  现状：未提交。`strong_weak` 布局、颜色反馈和交互阻断已在当前工作区生效，但没有单独切 commit。

  阻塞：`menus_assets_editor.cpp` 被任务 3 和任务 4 共同修改，当前更接近“一次性编辑器阶段性提交”而不是两个干净的小提交。

  下一步建议：后续如果需要整理历史，可把任务 3 与任务 4 合并为一次编辑器阶段提交，避免强行拆分同文件上的连续修改。

```bash
git add src/game/client/components/menus.h src/game/client/components/menus_assets_editor.cpp src/test/assets_editor_layout_test.cpp CMakeLists.txt
git commit -m "feat(editor): define strong weak slot layout explicitly"
```

---

### 任务 4：为贴图编辑器新增 `skin` 模式，并对齐 `skins7` 导出

**文件：**
- 修改：`src/game/client/components/menus.h`
- 修改：`src/game/client/components/menus_assets_editor.cpp`
- 修改：`src/game/client/components/menus_settings.cpp`
- 修改：`src/game/client/components/skins7.cpp`
- 修改：`src/test/assets_editor_layout_test.cpp`

- [x] **步骤 1：为编辑器增加 `skin` 类型与入口**

在 `menus.h` 中扩展编辑器类型枚举：

```cpp
ASSETS_EDITOR_TYPE_SKIN,
ASSETS_EDITOR_TYPE_COUNT
```

在 `menus_settings.cpp` 的皮肤设置区域增加一个显式入口按钮，行为示例：

```cpp
if(DoButton_Menu(&s_SkinEditorButton, Localize("Edit skin texture"), 0, &Button))
	OpenAssetsEditor(ASSETS_EDITOR_TYPE_SKIN);
```

- [x] **步骤 2：先写 `skin` 参考图切分失败测试**

在 `assets_editor_layout_test.cpp` 中增加：

```cpp
TEST(AssetsEditorLayout, SkinLayoutIncludesBodyFeetAndLowerStripSlots)
{
	const auto vSlots = BuildSkinEditorSlots("default");

	EXPECT_TRUE(HasFamilyKey(vSlots, "skin:body"));
	EXPECT_TRUE(HasFamilyKey(vSlots, "skin:feet"));
	EXPECT_TRUE(HasFamilyKey(vSlots, "skin:right_strip_0"));
	EXPECT_TRUE(HasFamilyKey(vSlots, "skin:bottom_strip_0"));
}
```

并额外断言身体与脚的大块尺寸符合 `96x96` 参考语义。

- [x] **步骤 3：实现 `BuildSkinEditorSlots()`**

在 `menus_assets_editor.cpp` 中按参考图显式定义主要区域：

```cpp
AddSlot("skin:body", 0, 0, 96, 96);
AddSlot("skin:feet", 96, 0, 96, 96);
AddSlot("skin:right_strip_0", 192, 0, 32, 32);
AddSlot("skin:right_strip_1", 224, 0, 32, 32);
AddSlot("skin:right_strip_2", 192, 32, 64, 32);
AddSlot("skin:right_strip_3", 192, 64, 64, 32);
AddSlot("skin:bottom_strip_0", 0, 96, 32, 32);
```

第一版不要做“自动从贴图尺寸推导所有块”，保持显式定义。

- [x] **步骤 4：实现 `skin` 导出路径与 JSON 写回**

在 `AssetsEditorExport()` 中为 `ASSETS_EDITOR_TYPE_SKIN` 增加专用分支：

```cpp
str_format(aJsonPath, sizeof(aJsonPath), "skins/%s.json", m_AssetsEditorState.m_aExportName);
str_format(aPngPath, sizeof(aPngPath), "skins/%s.png", m_AssetsEditorState.m_aExportName);
```

然后复用 `skins7.cpp` 的现有保存语义，至少保证：

- `filename`
- `custom_colors`
- RGBA 分量

能落到运行时已识别的 skin JSON 结构。

- [x] **步骤 5：让导出后的皮肤自动刷新到列表**

导出成功后调用现有皮肤刷新入口，避免用户必须重启客户端。

```cpp
GameClient()->m_Skins7.Refresh(...);
```

若当前模块已有更合适的刷新 helper，则优先复用已有链路。

- [x] **步骤 6：运行构建与测试**

  结果：`run_cxx_tests` 已通过；皮肤页入口 `Edit skin texture` 也已补回到 `menus_settings.cpp`。

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 60
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 | Select-Object -Last 120
```

预期：

- `AssetsEditorLayout.SkinLayoutIncludesBodyFeetAndLowerStripSlots` 通过
- 客户端可成功构建

- [ ] **步骤 7：提交本任务**

  现状：未提交。`skin` 模式、导出、刷新列表和皮肤页入口目前都在工作区中，但没有单独 commit。

  阻塞：当前编辑器相关文件仍可能继续为后续 UI 验收收口；另外工作区本身较脏，不适合现在做高风险拆分提交。

  下一步建议：若后续不再改编辑器主线，可把任务 3 和任务 4 的修改连同相关测试一起整理为一个 `editor` 主题提交。

```bash
git add src/game/client/components/menus.h src/game/client/components/menus_assets_editor.cpp src/game/client/components/menus_settings.cpp src/game/client/components/skins7.cpp src/test/assets_editor_layout_test.cpp
git commit -m "feat(editor): add skin texture editor mode"
```

---

### 任务 5：新增 `audio pack` 编辑器，并导出到 `audio/<pack>/...`

**文件：**
- 修改：`src/game/client/components/menus.h`
- 修改：`src/game/client/components/menus_settings.cpp`
- 修改：`src/game/client/components/sounds.cpp`
- 创建：`src/test/audio_pack_editor_test.cpp`
- 修改：`CMakeLists.txt`

- [x] **步骤 1：先把声音槽位映射规则提炼成纯 helper**

  结果：已在 `src/game/client/components/menus.h` 中新增 `SAudioPackSlot`、`BuildAudioPackSlots()` 与 `BuildAudioPackExportPath()`。当前实现直接复用 `g_pData->m_aSounds` 展平出实际运行时会加载的音频条目，并统一把默认资源路径收敛为 pack 可导出的相对路径。

在 `menus.h` 中定义最小 helper：

```cpp
struct SAudioPackSlot
{
	const char *m_pDisplayName;
	const char *m_pRelativePath;
};

std::vector<SAudioPackSlot> BuildAudioPackSlots();
std::string BuildAudioPackExportPath(const char *pPackName, const char *pRelativePath);
```

- [x] **步骤 2：为导出路径规则写失败测试**

  结果：已新增 `src/test/audio_pack_editor_test.cpp`，覆盖 `BuildAudioPackExportPath()` 的 pack 目录拼接规则，以及 `BuildAudioPackSlots()` 会把生成的默认声音条目展平成不带 `audio/` 前缀的相对路径。

在 `src/test/audio_pack_editor_test.cpp` 中加入：

```cpp
TEST(AudioPackEditor, ExportPathUsesAudioPackDirectory)
{
	EXPECT_EQ(BuildAudioPackExportPath("my_pack", "hit.wav"), "audio/my_pack/hit.wav");
	EXPECT_EQ(BuildAudioPackExportPath("my_pack", "audio/hook_attach.opus"), "audio/my_pack/audio/hook_attach.opus");
}
```

- [x] **步骤 3：在声音设置页增加编辑器入口**

  结果：已在 `RenderSettingsSound()` 的音频包列表头部新增 `Edit audio pack` 按钮，点击后会展开内嵌编辑面板，并自动带入当前 `g_Config.m_SndPack` 作为默认包名。

在 `RenderSettingsSound()` 的声音包列表区域新增按钮：

```cpp
if(DoButton_Menu(&s_AudioPackEditorButton, Localize("Edit audio pack"), 0, &Button))
	OpenAudioPackEditor(g_Config.m_SndPack);
```

该入口不加入资源页 tab。

- [x] **步骤 4：实现第一版 `audio pack` 编辑 UI**

  结果：已在声音设置页落一版内嵌式 `audio pack` 编辑器，包含：

  - 左侧槽位列表
  - 按名称 / 相对路径搜索
  - 包名输入框
  - 当前映射文件显示
  - 源 `.wv` 绝对路径输入
  - 当前槽位试听
  - 导出到 `audio/<pack>/<relative>` 的写盘闭环

  说明：本轮采用“输入绝对路径”代替平台文件对话框，先保证导出链路稳定，不额外引入新的文件浏览依赖；zip 导出仍未实现，符合本任务原本的 MVP 约束。

第一版 UI 只做：

- 左侧槽位列表
- 当前映射文件显示
- 重新选择文件
- 试听当前文件
- 导出目录包

不要在本任务中实现 zip 导出。

最小导出行为：

```cpp
const std::string Path = BuildAudioPackExportPath(pPackName, pSlot->m_pRelativePath);
CopyAbsoluteFileToStorage(Storage(), pSelectedFile, Path.c_str());
```

- [x] **步骤 5：导出成功后刷新声音包列表并支持立即切换**

  结果：导出成功后会立即：

  - 把 `g_Config.m_SndPack` 切到当前编辑包
  - 刷新 `RefreshAudioPacks(Storage(), s_vAudioPacks)`
  - 调用 `GameClient()->m_Sounds.Reload()`
  - 同步更新声音页状态文案

  这样导出的 `audio/<pack>/...` 已能在同一页面中立刻被识别和使用。

导出后复用现有逻辑：

```cpp
RefreshAudioPacks(Storage(), s_vAudioPacks);
GameClient()->m_Sounds.Reload();
```

确保导出的 `audio/<pack>/...` 能立刻出现在设置页列表里。

- [x] **步骤 6：注册测试并运行**

  结果：已将 `audio_pack_editor_test.cpp` 注册到 `CMakeLists.txt`，并于 2026-04-25 运行 `qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 | Select-Object -Last 120`，`585 tests` 全部通过。

在 `CMakeLists.txt` 中新增：

```cmake
    audio_pack_editor_test.cpp
```

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 | Select-Object -Last 120
```

预期：

- `AudioPackEditor.ExportPathUsesAudioPackDirectory` 通过

- [ ] **步骤 7：提交本任务**

  现状：未提交。任务 5 的 helper、测试、声音设置页入口、导出闭环与刷新链路都已在当前工作区完成，但没有单独切 commit。

  阻塞：当前工作区本身较脏，且 `menus.h`、`menus_settings.cpp` 等文件与其它任务共享，不适合在未再次梳理文件边界前强拆独立提交。

  下一步建议：若后续不再改音频包编辑器主线，可按“audio pack editor MVP”主题把 helper、测试和设置页 UI 一起整理为一次提交。

```bash
git add src/game/client/components/menus.h src/game/client/components/menus_settings.cpp src/game/client/components/sounds.cpp src/test/audio_pack_editor_test.cpp CMakeLists.txt
git commit -m "feat(audio): add editable audio pack workflow"
```

---

### 任务 6：整体验证与收尾

**文件：**
- 修改：`docs/superpowers/plans/2026-04-25-assets-editor-and-entity-bg-plan.md`
- 修改：必要时补回前述任务漏掉的小修复文件

- [x] **步骤 1：做最终构建验证**

  结果：2026-04-25 已重新运行 `qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10`，命令退出码为 `0`；`run_cxx_tests` 也再次通过，共 `585 tests`。本轮还额外用 `build-ninja\\testrunner.exe --gtest_filter='AssetsEditorLayout.*:AudioPackEditor.*:AssetsResourceRegistry.EntityBg*'` 单独复跑了 `entity_bg`、`strong_weak` / `skin`、`audio pack` 相关 `11` 个用例，全部通过。

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 60
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 | Select-Object -Last 120
```

预期：

- `DDNet.exe` 成功链接
- `run_cxx_tests` 全绿

- [ ] **步骤 2：做人工回归清单**

  现状：自动化层面的构建、全量 C++ 测试、定向功能测试与最小运行态检查都已重新核对，但尚未补做人手 UI 验收，尤其缺 `entity_bg` 浏览路径、材质编辑器实际观感，以及音频包编辑器从设置页内实际点选导出的整条交互链路确认。

  结果补充：2026-04-25 已从 `build-ninja` 目录实际拉起 `DDNet.exe`，进程在启动后 5 秒仍存活后主动结束，说明当前构建产物至少能正常进入运行态；但这仍不等价于已经完成页面级人工验收。

  阻塞：当前仍缺少从已启动客户端内逐项点击确认的交互式检查，尤其是资源页层级、材质选区命中和音频包导出后即时切换这几项。

  下一步建议：优先验资源页 `entity_bg` 层级入口、`default` 排序、皮肤编辑器入口、`strong_weak` 选区命中与颜色反馈，以及音频包编辑器搜索、试听、导出到 `audio/<pack>/...` 后的即时刷新与切换。

从 `build-ninja` 目录启动客户端：

```powershell
cd build-ninja
.\DDNet.exe
```

人工验收至少覆盖：

- `entity_bg` 根页只在开启“显示商店资源”时出现 `assets/entity_bg (Workshop)` 目录
- `default` 位于所有文件夹之后第一项
- `strong_weak` 三段切分命中正确
- `skin` 模式下身体、脚、右侧/下方小块可见且可交互
- `audio pack` 导出后设置页能识别并切换

建议按下面顺序点一遍，避免来回跳页面：

1. 设置 -> Assets -> `Entity Background Image`
   预期：关闭“显示商店资源”后，根页不再出现 `entity_bg (Workshop)`；重新开启后恢复显示。
2. 保持在 `Entity Background Image` 根页
   预期：若同时存在目录与文件，`default` 紧跟在所有目录之后，仍排在普通文件之前。
3. 进入 `Assets editor` -> `Strong Weak Hook`
   预期：画布表现为横向 3 格；左键点格子进入颜色选择，右键只重置当前格子，拖拽替换不会串到其它尺寸不匹配的格子。
4. 设置 -> Tee -> 皮肤区域 -> `Edit skin texture`
   预期：能看到 body、feet、右侧 strip、底部 strip；导出后新皮肤立即出现在列表中，不需要重启客户端。
5. 设置 -> Sound -> `Edit audio pack`
   预期：搜索能按 set 名或相对路径过滤槽位；`Preview slot` 能试听；导出一个 `.wv` 到 `audio/<pack>/...` 后，声音包列表立即出现该 pack，并能立刻切换生效。

- [x] **步骤 3：回填计划状态**

把本计划中所有已完成步骤勾选，并在必要处补一行实际验证结果，例如：

```markdown
- [x] 步骤 6：运行构建与测试
  结果：`run_cxx_tests` 全绿；若 `DDNet.exe` 正在运行，`game-client` 可能因 `LNK1104` 需在关闭进程后重试链接。
```

- [ ] **步骤 4：提交最终收尾**

  现状：未提交。计划文件已回填阶段状态，但当前工作区仍含未提交代码，且任务 6 的人工回归尚未做完。

  阻塞：虽然任务 5 主线与自动化验证已经完成，但在任务 6 的人工回归未做前，不适合把“最终收尾”标记为完成。

  下一步建议：等至少完成人工验收后，再按当前工作区实际拆分主题提交；若继续沿这条分支推进，也应先明确本次提交是否要一并包含任务 3-5 的编辑器改动。

```bash
git add docs/superpowers/plans/2026-04-25-assets-editor-and-entity-bg-plan.md
git commit -m "docs(plan): record execution progress for entity bg and editor expansion"
```

---

## 自检

### 规格覆盖度

本计划已覆盖规格中的 4 条主线：

- `entity_bg` 单一层级视图
- 资源页商店资源显示开关
- `strong_weak` / `skin` / `audio pack` 编辑工作流
- 资源分享延期实现结论

其中资源分享被明确保留为非目标，没有遗漏到实现任务中。

### 占位符扫描

本计划没有保留未决占位词，也没有使用引用前文代替具体步骤的写法。

所有任务都明确写出了涉及文件、目标函数形态、测试命令与预期。

### 类型一致性

本计划统一使用以下命名：

- `EEntityBgSourceKind`
- `SEntityBgHierarchyEntry`
- `BuildEntityBgHierarchyEntries`
- `BuildStrongWeakEditorSlots`
- `BuildSkinEditorSlots`
- `BuildAudioPackExportPath`

执行时不要再引入同义但不同名的 helper，以免任务之间断链。
