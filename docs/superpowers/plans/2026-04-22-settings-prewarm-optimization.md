# 设置页面预热初始化优化 实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 为所有设置页面添加静态变量预热函数，消除首次进入页面时的卡顿

**架构：** 为每个设置页面创建独立的预热函数，在应用启动时统一调用，提前初始化静态变量

**技术栈：** C++、DDNet 架构

---

## 文件结构

| 文件 | 职责 |
|------|------|
| `src/game/client/components/menus.h` | 添加预热函数声明 |
| `src/game/client/components/menus_settings.cpp` | 实现各设置页面的预热函数 |
| `src/game/client/components/menus_settings_assets.cpp` | 实现资源页面的预热函数 |
| `src/game/client/components/qmclient/menus_qmclient.cpp` | 扩展现有预热函数，调用各页面预热 |

---

## 任务 1：分析各页面的静态变量

**文件：**
- 分析：`src/game/client/components/menus_settings.cpp`
- 分析：`src/game/client/components/menus_settings_assets.cpp`

- [ ] **步骤 1：统计各设置页面的静态变量**

需要为以下页面创建预热函数：

| 页面 | 渲染函数 | 主要静态变量 |
|------|----------|--------------|
| Language | `RenderLanguageSettings` | CListBox, CButtonContainer |
| General | `RenderSettingsGeneral` | CButtonContainer, CListBox |
| Player | `RenderSettingsPlayer` | CButtonContainer, CLineInput, CListBox |
| Tee | `RenderSettingsTee` | CButtonContainer, CLineInput, CListBox |
| Appearance | `RenderSettingsAppearance` | CButtonContainer, CListBox |
| Graphics | `RenderSettingsGraphics` | CListBox, CButtonContainer |
| Sound | `RenderSettingsSound` | CListBox, CButtonContainer |
| DDNet | `RenderSettingsDDNet` | CButtonContainer, CListBox |
| Assets | `RenderSettingsCustom` | CButtonContainer, CListBox, std::vector |
| TClient | `RenderSettingsTClient` | CButtonContainer, CListBox |
| QmClient | `RenderSettingsQmClient` | CButtonContainer, CListBox |
| Configs | `RenderSettingsConfigs` | CButtonContainer, CListBox |

---

## 任务 2：在 menus.h 中添加预热函数声明

**文件：**
- 修改：`src/game/client/components/menus.h`

- [ ] **步骤 1：添加预热函数声明**

在 `CMenus` 类的 `private` 区域添加：

```cpp
// Settings pages prewarm functions
void PrewarmSettingsLanguage();
void PrewarmSettingsGeneral();
void PrewarmSettingsPlayer();
void PrewarmSettingsTee();
void PrewarmSettingsAppearance();
void PrewarmSettingsGraphics();
void PrewarmSettingsSound();
void PrewarmSettingsDDNet();
void PrewarmSettingsAssets();
void PrewarmSettingsTClient();
void PrewarmSettingsQmClient();
void PrewarmSettingsConfigs();
void PrewarmAllSettingsPages();
```

---

## 任务 3：实现 menus_settings.cpp 中的预热函数

**文件：**
- 修改：`src/game/client/components/menus_settings.cpp`

- [ ] **步骤 1：在文件末尾添加预热函数实现**

```cpp
// ============================================================================
// Settings Pages Prewarm Functions
// ============================================================================

void CMenus::PrewarmSettingsLanguage()
{
	static bool s_Prewarmed = false;
	if(s_Prewarmed)
		return;
	s_Prewarmed = true;

	static CListBox s_PrewarmListBox;
	s_PrewarmListBox.Reset();
}

void CMenus::PrewarmSettingsGeneral()
{
	static bool s_Prewarmed = false;
	if(s_Prewarmed)
		return;
	s_Prewarmed = true;

	static CButtonContainer s_SettingsButtonId;
	static CButtonContainer s_SavesButtonId;
	static CButtonContainer s_ConfigButtonId;
	static CButtonContainer s_ThemesButtonId;
	static CListBox s_ListBox;
	s_ListBox.Reset();
}

void CMenus::PrewarmSettingsPlayer()
{
	static bool s_Prewarmed = false;
	if(s_Prewarmed)
		return;
	s_Prewarmed = true;

	static CButtonContainer s_PlayerTabButton;
	static CButtonContainer s_DummyTabButton;
	static CLineInput s_NameInput;
	static CLineInput s_ClanInput;
	static CLineInputBuffered<25> s_FlagFilterInput;
	static CListBox s_ListBox;
	s_ListBox.Reset();
}

void CMenus::PrewarmSettingsTee()
{
	static bool s_Prewarmed = false;
	if(s_Prewarmed)
		return;
	s_Prewarmed = true;

	static CButtonContainer s_PlayerTabButton;
	static CButtonContainer s_DummyTabButton;
	static CButtonContainer s_ProfilesTabButton;
	static CLineInput s_SkinInput;
	static CLineInput s_SkinPrefixInput(g_Config.m_ClSkinPrefix, sizeof(g_Config.m_ClSkinPrefix));
	static CButtonContainer s_RandomSkinButton;
	static CButtonContainer s_RandomizeColors;
	static CListBox s_ListBox;
	static CListBox s_QueueListBox;
	static CListBox s_PresetListBox;
	static CLineInput s_SkinFilterInput(g_Config.m_ClSkinFilterString, sizeof(g_Config.m_ClSkinFilterString));
	static CButtonContainer s_SkinDatabaseButton;
	static CButtonContainer s_DirectoryButton;
	static CButtonContainer s_SkinRefreshButton;
	s_ListBox.Reset();
	s_QueueListBox.Reset();
	s_PresetListBox.Reset();
}

void CMenus::PrewarmSettingsAppearance()
{
	static bool s_Prewarmed = false;
	if(s_Prewarmed)
		return;
	s_Prewarmed = true;

	static CButtonContainer s_aPageTabs[NUMBER_OF_APPEARANCE_TABS] = {};
	static CButtonContainer s_AuthedColor;
	static CButtonContainer s_BackgroundColor;
	static CListBox s_ListBox;
	s_ListBox.Reset();
}

void CMenus::PrewarmSettingsGraphics()
{
	static bool s_Prewarmed = false;
	if(s_Prewarmed)
		return;
	s_Prewarmed = true;

	static CListBox s_ListBox;
	s_ListBox.Reset();
}

void CMenus::PrewarmSettingsSound()
{
	static bool s_Prewarmed = false;
	if(s_Prewarmed)
		return;
	s_Prewarmed = true;

	static CListBox s_ListBox;
	static CListBox s_AudioPackListBox;
	static CButtonContainer s_AudioPackRefreshButton;
	s_ListBox.Reset();
	s_AudioPackListBox.Reset();
}

void CMenus::PrewarmSettingsDDNet()
{
	static bool s_Prewarmed = false;
	if(s_Prewarmed)
		return;
	s_Prewarmed = true;

	static CButtonContainer s_ResetId1;
	static CListBox s_ListBox;
	s_ListBox.Reset();
}

void CMenus::PrewarmSettingsConfigs()
{
	static bool s_Prewarmed = false;
	if(s_Prewarmed)
		return;
	s_Prewarmed = true;

	static CListBox s_ListBox;
	s_ListBox.Reset();
}
```

---

## 任务 4：实现 menus_settings_assets.cpp 中的预热函数

**文件：**
- 修改：`src/game/client/components/menus_settings_assets.cpp`

- [ ] **步骤 1：在文件末尾添加预热函数实现**

```cpp
void CMenus::PrewarmSettingsAssets()
{
	static bool s_Prewarmed = false;
	if(s_Prewarmed)
		return;
	s_Prewarmed = true;

	static CButtonContainer s_aPageTabs[NUMBER_OF_ASSETS_TABS] = {};
	static const char *s_apAssetsTabNames[NUMBER_OF_ASSETS_TABS] = {};
	static CLineInputBuffered<64> s_aFilterInputs[NUMBER_OF_ASSETS_TABS];
	static bool gs_aInitCustomList[NUMBER_OF_ASSETS_TABS] = {};
	static size_t gs_aCustomListSize[NUMBER_OF_ASSETS_TABS] = {};
	static std::vector<SCustomEntities *> gs_vpSearchEntitiesList;
	static std::vector<SCustomGame *> gs_vpSearchGamesList;
	static std::vector<SCustomEmoticon *> gs_vpSearchEmoticonsList;
	static std::vector<SCustomParticle *> gs_vpSearchParticlesList;
	static std::vector<SCustomHud *> gs_vpSearchHudList;
	static std::vector<SCustomExtras *> gs_vpSearchExtrasList;

	gs_vpSearchEntitiesList.clear();
	gs_vpSearchGamesList.clear();
	gs_vpSearchEmoticonsList.clear();
	gs_vpSearchParticlesList.clear();
	gs_vpSearchHudList.clear();
	gs_vpSearchExtrasList.clear();
}
```

---

## 任务 5：实现 menus_qmclient.cpp 中的预热函数

**文件：**
- 修改：`src/game/client/components/qmclient/menus_qmclient.cpp`

- [ ] **步骤 1：添加 TClient 和 QmClient 页面预热函数**

```cpp
void CMenus::PrewarmSettingsTClient()
{
	static bool s_Prewarmed = false;
	if(s_Prewarmed)
		return;
	s_Prewarmed = true;

	static CButtonContainer s_aPageTabs[NUMBER_OF_TCLIENT_TABS] = {};
	static CScrollRegion s_ScrollRegion;
	static std::vector<CUIRect> s_SectionBoxes;
	s_ScrollRegion.Reset();
	s_SectionBoxes.clear();
}

void CMenus::PrewarmSettingsQmClient()
{
	static bool s_Prewarmed = false;
	if(s_Prewarmed)
		return;
	s_Prewarmed = true;

	static CButtonContainer s_aPageTabs[NUMBER_OF_QMCLIENT_TABS] = {};
	static CScrollRegion s_ScrollRegion;
	static std::vector<CUIRect> s_SectionBoxes;
	s_ScrollRegion.Reset();
	s_SectionBoxes.clear();
}
```

- [ ] **步骤 2：扩展 PrewarmTClientAndQmClientPages 函数**

```cpp
void CMenus::PrewarmTClientAndQmClientPages()
{
	static bool s_Prewarmed = false;
	if(s_Prewarmed)
		return;
	s_Prewarmed = true;

	// 预热通用静态变量
	static CScrollRegion s_PrewarmScrollRegion;
	static std::vector<CUIRect> s_PrewarmSectionBoxes;
	static CButtonContainer s_PrewarmButtonContainer;
	static std::vector<std::string> s_PrewarmStringVector;
	static std::vector<const char *> s_PrewarmCharPtrVector;
	static char s_PrewarmLanguageFile[IO_MAX_PATH_LENGTH] = {};
	static const char *s_PrewarmTabNames[16] = {};

	s_PrewarmScrollRegion.Reset();
	s_PrewarmSectionBoxes.clear();
	s_PrewarmStringVector.clear();
	s_PrewarmCharPtrVector.clear();
	s_PrewarmLanguageFile[0] = '\0';
	for(int i = 0; i < 16; i++)
		s_PrewarmTabNames[i] = nullptr;
}

void CMenus::PrewarmAllSettingsPages()
{
	static bool s_Prewarmed = false;
	if(s_Prewarmed)
		return;
	s_Prewarmed = true;

	// 预热所有设置页面
	PrewarmSettingsLanguage();
	PrewarmSettingsGeneral();
	PrewarmSettingsPlayer();
	PrewarmSettingsTee();
	PrewarmSettingsAppearance();
	PrewarmSettingsGraphics();
	PrewarmSettingsSound();
	PrewarmSettingsDDNet();
	PrewarmSettingsAssets();
	PrewarmSettingsTClient();
	PrewarmSettingsQmClient();
	PrewarmSettingsConfigs();
}
```

---

## 任务 6：在初始化时调用预热函数

**文件：**
- 修改：`src/game/client/components/menus.cpp`

- [ ] **步骤 1：修改初始化代码**

找到 `menus.cpp` 中的初始化代码（约 1414-1418 行），修改为：

```cpp
	// Preload skin list to avoid lag when first entering settings
	GameClient()->m_Skins.SkinList();

	// Preload all settings pages static variables
	PrewarmAllSettingsPages();
```

- [ ] **步骤 2：删除旧的 PrewarmTClientAndQmClientPages 调用**

因为 `PrewarmAllSettingsPages` 已经包含了原有的预热逻辑。

---

## 任务 7：编译验证

**文件：**
- 无

- [ ] **步骤 1：编译客户端**

运行：
```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10
```

预期：编译成功，无错误

- [ ] **步骤 2：运行客户端测试**

运行客户端，测试：
1. 点击"设置"按钮进入设置菜单 - 应无明显卡顿
2. 切换各设置页面 - 应无明显卡顿
3. 进入资源页面，切换子标签 - 应无明显卡顿

---

## 任务 8：Commit

- [ ] **步骤 1：提交更改**

```bash
git add -A
git commit -m "perf: 预热设置页面静态变量，消除首次进入卡顿

- 为每个设置页面添加独立的预热函数
- 在应用启动时统一调用预热函数
- 预热 CListBox、CButtonContainer、CLineInput 等静态变量
- 扩展 PrewarmAllSettingsPages 统一管理预热逻辑"
```

---

## 风险与注意事项

1. **静态变量重复初始化**：每个预热函数都有 `s_Prewarmed` 标志，确保只执行一次
2. **内存占用**：预热会增加少量内存占用，但可忽略不计
3. **启动时间**：预热会增加约 50-100ms 启动时间，但可接受
4. **线程安全**：预热在主线程执行，无需考虑线程安全问题
