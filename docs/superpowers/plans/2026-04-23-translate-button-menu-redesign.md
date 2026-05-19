# 翻译按钮菜单重设计实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 将翻译按钮弹出菜单从平铺按钮列表改为下拉框式菜单，拆分入站/出站语言设置，扩展语言列表到 10 种。

**架构：** 在 `chat.cpp` 中重写 `PopupLanguageMenu` 为下拉框式渲染；新增 `QmTranslateOutgoingTarget` 配置变量；出站翻译逻辑改用新配置；保持现有交互方式（左键菜单、右键开关）。

**技术栈：** C++17, DDNet UI 系统 (CUi, CUIRect, DoPopupMenu)

---

## 文件清单

| 文件 | 职责 |
|------|------|
| `src/engine/shared/config_variables_qmclient_extra.h` | 新增 `QmTranslateOutgoingTarget` 配置变量 |
| `src/game/client/components/chat.h` | 新增下拉框展开状态枚举和字段 |
| `src/game/client/components/chat.cpp` | 重写 `PopupLanguageMenu` 为下拉框式；修改 `RenderTranslateButton` 圆角；更新 `OpenLanguageMenu` 尺寸 |
| `src/game/client/components/qmclient/translate.cpp` | 出站翻译使用 `QmTranslateOutgoingTarget` |

---

### 任务 1：新增出站语言配置变量

**文件：**
- 修改：`src/engine/shared/config_variables_qmclient_extra.h:134-135`

- [ ] **步骤 1：在自动出站翻译配置后添加新变量**

在 `QmTranslateAutoOutgoingMode` 配置后插入：

```cpp
MACRO_CONFIG_STR(QmTranslateOutgoingTarget, qm_translate_outgoing_target, 16, "en", CFGFLAG_CLIENT | CFGFLAG_SAVE, "出站翻译目标语言代码")
```

- [ ] **步骤 2：Commit**

```bash
git add src/engine/shared/config_variables_qmclient_extra.h
git commit -m "feat(translate): add QmTranslateOutgoingTarget config variable"
```

---

### 任务 2：更新 chat.h 新增下拉框状态

**文件：**
- 修改：`src/game/client/components/chat.h:188-208`

- [ ] **步骤 1：在 CLanguagePopupContext 前添加下拉框展开状态枚举**

在 `STranslateButtonState` 结构体后、`CLanguagePopupContext` 前添加：

```cpp
	// 翻译菜单下拉框展开状态
	enum class ETranslateDropdown : int
	{
		NONE = 0,
		INBOUND_LANG,
		OUTBOUND_LANG,
		BACKEND,
	};
```

- [ ] **步骤 2：在 CLanguagePopupContext 中添加下拉框状态字段**

修改 `CLanguagePopupContext` 类：

```cpp
	class CLanguagePopupContext : public SPopupMenuId
	{
	public:
		CChat *m_pChat = nullptr;
		ETranslateDropdown m_DropdownOpen = ETranslateDropdown::NONE;
	};
```

- [ ] **步骤 3：Commit**

```bash
git add src/game/client/components/chat.h
git commit -m "feat(translate): add dropdown state enum to chat header"
```

---

### 任务 3：修改出站翻译逻辑使用新配置

**文件：**
- 修改：`src/game/client/components/qmclient/translate.cpp:1637-1644`

- [ ] **步骤 1：修改 StartAutoOutgoingTranslate 使用 QmTranslateOutgoingTarget**

找到 `StartAutoOutgoingTranslate` 方法中设置目标语言的代码：

```cpp
	const char *pTarget = GetEffectiveTranslateTarget(g_Config.m_QmTranslateTarget);
```

替换为：

```cpp
	const char *pTarget = GetEffectiveTranslateTarget(g_Config.m_QmTranslateOutgoingTarget);
```

- [ ] **步骤 2：Commit**

```bash
git add src/game/client/components/qmclient/translate.cpp
git commit -m "feat(translate): use QmTranslateOutgoingTarget for outbound translation"
```

---

### 任务 4：重写 PopupLanguageMenu 为下拉框式菜单

**文件：**
- 修改：`src/game/client/components/chat.cpp:2218-2389`

- [ ] **步骤 1：重写整个 PopupLanguageMenu 方法**

替换 `PopupLanguageMenu` 实现为下拉框式菜单。新实现包含：

1. 标题"翻译设置"
2. 自动翻译开关按钮
3. 入站语言下拉框（10 种语言）
4. 出站语言下拉框（10 种语言）
5. 翻译后端下拉框（4 种后端）
6. 后端未配置警告

关键代码结构：

```cpp
CUi::EPopupMenuFunctionResult CChat::PopupLanguageMenu(void *pContext, CUIRect View, bool Active)
{
	CLanguagePopupContext *pPopupContext = static_cast<CLanguagePopupContext *>(pContext);
	CChat *pChat = pPopupContext->m_pChat;
	CUi *pUi = pChat->Ui();

	const float Margin = 5.0f;
	View.Margin(Margin, &View);

	const float FontSize = 10.0f;
	const float RowHeight = 18.0f;
	const float DropdownHeaderHeight = 20.0f;

	ColorRGBA OptionSelectedColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_QmTranslateMenuOptionSelected));
	ColorRGBA OptionNormalColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_QmTranslateMenuOptionNormal));

	// 标题
	CUIRect TitleRect;
	View.HSplitTop(RowHeight, &TitleRect, &View);
	pUi->DoLabel(&TitleRect, Localize("Translation Settings"), FontSize, TEXTALIGN_ML);

	// 自动翻译开关
	{
		CUIRect ToggleRect;
		View.HSplitTop(RowHeight, &ToggleRect, &View);
		ToggleRect.VMargin(2.0f, &ToggleRect);

		const bool Enabled = g_Config.m_QmTranslateAutoOutgoing != 0;
		const ColorRGBA ToggleColor = Enabled ? OptionSelectedColor : OptionNormalColor;
		ToggleRect.Draw(ToggleColor, IGraphics::CORNER_ALL, 4.0f);

		if(pUi->DoButtonLogic(&g_Config.m_QmTranslateAutoOutgoing, 0, &ToggleRect, BUTTONFLAG_LEFT))
		{
			pChat->ToggleAutoTranslate();
			return CUi::POPUP_KEEP_OPEN;
		}

		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Auto Translate"), Enabled ? Localize("On") : Localize("Off"));
		pUi->DoLabel(&ToggleRect, aBuf, FontSize, TEXTALIGN_ML);
	}

	// 分隔线
	{
		CUIRect SepRect;
		View.HSplitTop(8.0f, &SepRect, &View);
		SepRect.HMargin(3.0f, &SepRect);
		SepRect.Draw(ColorRGBA(0.5f, 0.5f, 0.5f, 0.5f), IGraphics::CORNER_NONE, 0.0f);
	}

	// 语言列表定义
	static const struct
	{
		const char *m_pCode;
		const char *m_pName;
	} s_aLanguages[] = {
		{"zh", "中文"},
		{"en", "English"},
		{"ja", "日本語"},
		{"ko", "한국어"},
		{"zh-TW", "繁體中文"},
		{"ru", "Русский"},
		{"de", "Deutsch"},
		{"fr", "Français"},
		{"es", "Español"},
		{"pt", "Português"},
	};

	// 辅助函数：渲染下拉框
	auto RenderDropdown = [&](const char *pLabel, const char *pCurrentValue, ETranslateDropdown DropdownId, auto &&GetValueName) -> bool
	{
		// 标签
		CUIRect LabelRect;
		View.HSplitTop(RowHeight * 0.8f, &LabelRect, &View);
		pUi->DoLabel(&LabelRect, pLabel, FontSize * 0.9f, TEXTALIGN_ML);

		// 下拉框头部
		CUIRect HeaderRect;
		View.HSplitTop(DropdownHeaderHeight, &HeaderRect, &View);
		HeaderRect.VMargin(2.0f, &HeaderRect);

		const bool IsOpen = pPopupContext->m_DropdownOpen == DropdownId;
		const ColorRGBA HeaderColor = IsOpen ? OptionSelectedColor : OptionNormalColor;
		HeaderRect.Draw(HeaderColor, IGraphics::CORNER_ALL, 4.0f);

		// 显示当前值和箭头
		char aDisplayBuf[64];
		str_format(aDisplayBuf, sizeof(aDisplayBuf), "%s %s", GetValueName(pCurrentValue), IsOpen ? "▲" : "▼");
		pUi->DoLabel(&HeaderRect, aDisplayBuf, FontSize, TEXTALIGN_ML);

		// 点击头部切换展开/收起
		if(pUi->DoButtonLogic(&DropdownId, 0, &HeaderRect, BUTTONFLAG_LEFT))
		{
			pPopupContext->m_DropdownOpen = IsOpen ? ETranslateDropdown::NONE : DropdownId;
			return true;
		}

		// 如果展开，渲染列表
		if(IsOpen)
		{
			// 列表区域 - 在头部下方展开
			float ListHeight = RowHeight * 10; // 10 个语言选项
			CUIRect ListRect;
			ListRect.x = HeaderRect.x;
			ListRect.y = HeaderRect.y + HeaderRect.h + 2.0f;
			ListRect.w = HeaderRect.w;
			ListRect.h = ListHeight;

			// 背景
			ListRect.Draw(ColorRGBA(0.15f, 0.15f, 0.15f, 0.95f), IGraphics::CORNER_ALL, 4.0f);

			CUIRect ItemRect = ListRect;
			ItemRect.h = RowHeight;

			for(const auto &Item : s_aLanguages)
			{
				const bool Selected = str_comp(pCurrentValue, Item.m_pCode) == 0;
				const ColorRGBA ItemColor = Selected ? OptionSelectedColor : ColorRGBA(0.15f, 0.15f, 0.15f, 0.95f);

				CUIRect ItemBgRect = ItemRect;
				ItemBgRect.VMargin(2.0f, &ItemBgRect);
				ItemBgRect.Draw(ItemColor, IGraphics::CORNER_ALL, 3.0f);

				if(pUi->DoButtonLogic(&Item, 0, &ItemBgRect, BUTTONFLAG_LEFT))
				{
					str_copy(const_cast<char *>(pCurrentValue), Item.m_pCode, 16);
					pPopupContext->m_DropdownOpen = ETranslateDropdown::NONE;
					return true;
				}

				pUi->DoLabel(&ItemBgRect, Item.m_pName, FontSize, TEXTALIGN_ML);

				ItemRect.y += RowHeight;
			}
		}

		return false;
	};

	// 入站语言下拉框
	{
		auto GetLangName = [&](const char *pCode) -> const char *
		{
			for(const auto &Lang : s_aLanguages)
				if(str_comp(pCode, Lang.m_pCode) == 0)
					return Lang.m_pName;
			return pCode;
		};

		if(RenderDropdown(Localize("Inbound Language"), g_Config.m_QmTranslateTarget, ETranslateDropdown::INBOUND_LANG, GetLangName))
			return CUi::POPUP_KEEP_OPEN;
	}

	// 出站语言下拉框
	{
		auto GetLangName = [&](const char *pCode) -> const char *
		{
			for(const auto &Lang : s_aLanguages)
				if(str_comp(pCode, Lang.m_pCode) == 0)
					return Lang.m_pName;
			return pCode;
		};

		if(RenderDropdown(Localize("Outbound Language"), g_Config.m_QmTranslateOutgoingTarget, ETranslateDropdown::OUTBOUND_LANG, GetLangName))
			return CUi::POPUP_KEEP_OPEN;
	}

	// 分隔线
	{
		CUIRect SepRect;
		View.HSplitTop(8.0f, &SepRect, &View);
		SepRect.HMargin(3.0f, &SepRect);
		SepRect.Draw(ColorRGBA(0.5f, 0.5f, 0.5f, 0.5f), IGraphics::CORNER_NONE, 0.0f);
	}

	// 后端列表定义
	static const struct
	{
		const char *m_pCode;
		const char *m_pName;
	} s_aBackends[] = {
		{"llm", "LLM API"},
		{"tencentcloud", "Tencent Cloud"},
		{"libretranslate", "LibreTranslate"},
		{"ftapi", "FTAPI"},
	};

	// 翻译后端下拉框
	{
		auto GetBackendName = [&](const char *pCode) -> const char *
		{
			for(const auto &Backend : s_aBackends)
				if(str_comp_nocase(pCode, Backend.m_pCode) == 0)
					return Backend.m_pName;
			return pCode;
		};

		// 标签
		CUIRect LabelRect;
		View.HSplitTop(RowHeight * 0.8f, &LabelRect, &View);
		pUi->DoLabel(&LabelRect, Localize("Translate Backend"), FontSize * 0.9f, TEXTALIGN_ML);

		// 下拉框头部
		CUIRect HeaderRect;
		View.HSplitTop(DropdownHeaderHeight, &HeaderRect, &View);
		HeaderRect.VMargin(2.0f, &HeaderRect);

		const bool IsOpen = pPopupContext->m_DropdownOpen == ETranslateDropdown::BACKEND;
		const ColorRGBA HeaderColor = IsOpen ? OptionSelectedColor : OptionNormalColor;
		HeaderRect.Draw(HeaderColor, IGraphics::CORNER_ALL, 4.0f);

		char aDisplayBuf[64];
		str_format(aDisplayBuf, sizeof(aDisplayBuf), "%s %s", GetBackendName(g_Config.m_QmTranslateBackend), IsOpen ? "▲" : "▼");
		pUi->DoLabel(&HeaderRect, aDisplayBuf, FontSize, TEXTALIGN_ML);

		if(pUi->DoButtonLogic(&s_aBackends[0], 0, &HeaderRect, BUTTONFLAG_LEFT))
		{
			pPopupContext->m_DropdownOpen = IsOpen ? ETranslateDropdown::NONE : ETranslateDropdown::BACKEND;
			return CUi::POPUP_KEEP_OPEN;
		}

		if(IsOpen)
		{
			float ListHeight = RowHeight * 4; // 4 个后端选项
			CUIRect ListRect;
			ListRect.x = HeaderRect.x;
			ListRect.y = HeaderRect.y + HeaderRect.h + 2.0f;
			ListRect.w = HeaderRect.w;
			ListRect.h = ListHeight;

			ListRect.Draw(ColorRGBA(0.15f, 0.15f, 0.15f, 0.95f), IGraphics::CORNER_ALL, 4.0f);

			CUIRect ItemRect = ListRect;
			ItemRect.h = RowHeight;

			for(const auto &Item : s_aBackends)
			{
				const bool Selected = str_comp_nocase(g_Config.m_QmTranslateBackend, Item.m_pCode) == 0;
				const ColorRGBA ItemColor = Selected ? OptionSelectedColor : ColorRGBA(0.15f, 0.15f, 0.15f, 0.95f);

				CUIRect ItemBgRect = ItemRect;
				ItemBgRect.VMargin(2.0f, &ItemBgRect);
				ItemBgRect.Draw(ItemColor, IGraphics::CORNER_ALL, 3.0f);

				if(pUi->DoButtonLogic(&Item, 0, &ItemBgRect, BUTTONFLAG_LEFT))
				{
					str_copy(g_Config.m_QmTranslateBackend, Item.m_pCode, sizeof(g_Config.m_QmTranslateBackend));
					pPopupContext->m_DropdownOpen = ETranslateDropdown::NONE;
					return CUi::POPUP_KEEP_OPEN;
				}

				pUi->DoLabel(&ItemBgRect, Item.m_pName, FontSize, TEXTALIGN_ML);

				ItemRect.y += RowHeight;
			}
		}
	}

	// 后端未配置警告
	bool IsConfigured = true;
	const char *pConfigWarning = nullptr;
	if(str_comp_nocase(g_Config.m_QmTranslateBackend, "tencentcloud") == 0)
	{
		IsConfigured = g_Config.m_QmTranslateTcSecretId[0] != '\0' && g_Config.m_QmTranslateTcSecretKey[0] != '\0';
		pConfigWarning = Localize("⚠️ Tencent Cloud API not configured");
	}
	else if(str_comp_nocase(g_Config.m_QmTranslateBackend, "libretranslate") == 0)
	{
		IsConfigured = g_Config.m_QmTranslateLibreKey[0] != '\0';
		pConfigWarning = Localize("⚠️ LibreTranslate API Key not set");
	}
	else if(str_comp_nocase(g_Config.m_QmTranslateBackend, "llm") == 0)
	{
		IsConfigured = g_Config.m_QmTranslateLlmKeyZhipu[0] != '\0' ||
			       g_Config.m_QmTranslateLlmKeyDeepseek[0] != '\0' ||
			       g_Config.m_QmTranslateLlmKeyOpenai[0] != '\0' ||
			       g_Config.m_QmTranslateLlmKeyCustom[0] != '\0';
		pConfigWarning = Localize("⚠️ LLM API Key not configured");
	}

	if(!IsConfigured && pConfigWarning)
	{
		CUIRect WarningRect;
		View.HSplitTop(RowHeight, &WarningRect, &View);
		WarningRect.VMargin(2.0f, &WarningRect);
		WarningRect.Draw(ColorRGBA(0.7f, 0.3f, 0.3f, 0.6f), IGraphics::CORNER_ALL, 4.0f);
		pUi->DoLabel(&WarningRect, pConfigWarning, FontSize * 0.9f, TEXTALIGN_ML);
	}

	return CUi::POPUP_KEEP_OPEN;
}
```

- [ ] **步骤 2：Commit**

```bash
git add src/game/client/components/chat.cpp
git commit -m "feat(translate): rewrite PopupLanguageMenu with dropdown style"
```

---

### 任务 5：调整菜单尺寸和按钮圆角

**文件：**
- 修改：`src/game/client/components/chat.cpp:2206-2214`
- 修改：`src/game/client/components/chat.cpp:2161`

- [ ] **步骤 1：调整菜单尺寸为 150px 宽，自动计算高度**

修改 `OpenLanguageMenu` 中的菜单尺寸：

```cpp
		// 菜单尺寸
		const float MenuWidth = 150.0f;
		const float MenuHeight = 320.0f; // 增加高度以容纳两个下拉框
```

- [ ] **步骤 2：调整按钮圆角为 5px**

修改 `RenderTranslateButton` 中的圆角计算：

```cpp
	const float ButtonRounding = maximum(5.0f, ButtonRect.h * 0.28f);
```

- [ ] **步骤 3：Commit**

```bash
git add src/game/client/components/chat.cpp
git commit -m "feat(translate): adjust menu size and button rounding"
```

---

### 任务 6：构建验证

- [ ] **步骤 1：运行 Windows 构建**

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30
```

预期：构建成功，无编译错误。

- [ ] **步骤 2：运行测试**

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_rust_tests
```

预期：所有测试通过。

---

## 自检

**规格覆盖度：**
- ✅ 下拉框式菜单布局
- ✅ 入站/出站语言拆分
- ✅ 语言列表扩展到 10 种
- ✅ 蓝紫色主题
- ✅ 后端未配置警告
- ✅ 按钮圆角 5px
- ✅ 菜单宽度 150px
- ✅ 交互方式保持（左键菜单、右键开关）

**占位符扫描：** 无占位符。

**类型一致性：**
- `ETranslateDropdown` 枚举在 chat.h 和 chat.cpp 中一致使用
- `QmTranslateOutgoingTarget` 配置变量在 config 和 translate.cpp 中一致使用
