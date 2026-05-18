# 聊天翻译与聊天框翻译按钮实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 为 QmClient 实现通用 LLM 翻译后端、玩家聊天入站自动翻译、`/trans [text]` 单次出站翻译、中文自动出站翻译，以及完整聊天框中的全局翻译按钮与轻量设置面板。

**架构：** 继续以 [`translate.cpp`](/E:/Coding/DDNet/QmClient/src/game/client/components/qmclient/translate.cpp) 为翻译核心，新增可单测的策略辅助函数并在 [`translate_test.cpp`](/E:/Coding/DDNet/QmClient/src/test/translate_test.cpp) 中覆盖。聊天框按钮与轻量面板直接接到 [`chat.cpp`](/E:/Coding/DDNet/QmClient/src/game/client/components/chat.cpp)，后端/API/model/prompt 配置继续留在 [`menus_qmclient.cpp`](/E:/Coding/DDNet/QmClient/src/game/client/components/qmclient/menus_qmclient.cpp)。

**技术栈：** C++、DDNet/QmClient 现有聊天组件、HTTP 请求、Google Test、CMake/Ninja

---

## 文件结构

- 修改：`src/game/client/components/qmclient/translate.h`
  - 暴露可测试的翻译策略辅助函数声明，扩展翻译组件公开接口。
- 修改：`src/game/client/components/qmclient/translate.cpp`
  - 实现通用 LLM 后端、智谱 AI 预设、入站/出站翻译策略、`/trans` 命令解析与任务调度。
- 修改：`src/game/client/components/chat.h`
  - 添加聊天框翻译按钮与轻量面板的运行时状态。
- 修改：`src/game/client/components/chat.cpp`
  - 接入聊天框全局翻译按钮、鼠标自动显示、面板绘制与出站发送前拦截。
- 修改：`src/game/client/components/qmclient/menus_qmclient.cpp`
  - 将后端配置页从“智谱专用后端”调整为“通用 LLM + 预设”模式，并补充 prompt 配置。
- 修改：`src/engine/shared/config_variables_qmclient.h`
  - 增加聊天翻译行为配置与通用 LLM 配置项。
- 修改：`src/engine/shared/config_variables_qmclient_extra.h`
  - 迁移或扩展当前智谱模型配置为通用 LLM 相关配置。
- 修改：`src/test/translate_test.cpp`
  - 为翻译策略、命令解析、LLM 请求/响应解析与按钮可见性辅助函数增加单元测试。
- 修改：`docs/superpowers/specs/2026-04-21-chat-translation-design.md`
  - 若实现中发现与已确认规格不一致的命名差异，先同步书面规格再改代码。

## 任务 1：抽出可测试的翻译策略并先补失败测试

**文件：**
- 修改：`src/game/client/components/qmclient/translate.h`
- 修改：`src/game/client/components/qmclient/translate.cpp`
- 测试：`src/test/translate_test.cpp`

- [ ] **步骤 1：先在 `translate_test.cpp` 写失败测试，覆盖入站/出站策略与 `/trans` 解析**

```cpp
TEST(Translate, IncomingAutoTranslate_PlayerChatOnly)
{
	EXPECT_TRUE(TranslateShouldAutoTranslateIncoming(false, false, false));
	EXPECT_FALSE(TranslateShouldAutoTranslateIncoming(true, false, false));
	EXPECT_FALSE(TranslateShouldAutoTranslateIncoming(false, true, false));
	EXPECT_FALSE(TranslateShouldAutoTranslateIncoming(false, false, true));
}

TEST(Translate, OutgoingAutoTranslate_DetectChineseAndSkipCommands)
{
	EXPECT_TRUE(TranslateShouldAutoTranslateOutgoing("你好 DDRace"));
	EXPECT_FALSE(TranslateShouldAutoTranslateOutgoing("/rank"));
	EXPECT_FALSE(TranslateShouldAutoTranslateOutgoing("hello world"));
}

TEST(Translate, ParseOutgoingTranslateCommand_Basic)
{
	char aText[256] = "";
	EXPECT_TRUE(ParseOutgoingTranslateCommand("/trans 你好", aText, sizeof(aText)));
	EXPECT_STREQ(aText, "你好");
}

TEST(Translate, ParseOutgoingTranslateCommand_RejectMissingPayload)
{
	char aText[256] = "";
	EXPECT_FALSE(ParseOutgoingTranslateCommand("/trans", aText, sizeof(aText)));
	EXPECT_FALSE(ParseOutgoingTranslateCommand("/trans   ", aText, sizeof(aText)));
}
```

- [ ] **步骤 2：运行 `Translate` 相关测试，确认新测试先红灯失败**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target testrunner -j 10
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 -- --gtest_filter=Translate.*
```

预期：

- `TranslateShouldAutoTranslateIncoming`
- `TranslateShouldAutoTranslateOutgoing`
- `ParseOutgoingTranslateCommand`

这些新测试因“函数未定义”或“行为未实现”失败。

- [ ] **步骤 3：在 `translate.h/.cpp` 中实现最小可用策略辅助函数**

```cpp
bool TranslateShouldAutoTranslateIncoming(bool IsServerMsg, bool IsClientMsg, bool IsLocalPlayerMsg)
{
	return !IsServerMsg && !IsClientMsg && !IsLocalPlayerMsg;
}

bool TranslateShouldAutoTranslateOutgoing(const char *pText)
{
	if(!pText || pText[0] == '\0' || pText[0] == '/')
		return false;
	return str_utf8_find_nocase(pText, "你") != nullptr ||
		str_utf8_find_nocase(pText, "我") != nullptr;
}

bool ParseOutgoingTranslateCommand(const char *pLine, char *pOutText, size_t OutTextSize)
{
	const char *pPrefix = "/trans";
	if(!pLine || !str_startswith_nocase(pLine, pPrefix))
		return false;
	const char *pPayload = str_utf8_skip_whitespaces(pLine + str_length(pPrefix));
	if(*pPayload == '\0')
		return false;
	str_copy(pOutText, pPayload, OutTextSize);
	return true;
}
```

- [ ] **步骤 4：重新运行 `Translate.*` 测试，确认策略层全部转绿**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 -- --gtest_filter=Translate.*
```

预期：上述新增 `Translate` 测试通过。

- [ ] **步骤 5：提交这一步**

```powershell
git add src/game/client/components/qmclient/translate.h src/game/client/components/qmclient/translate.cpp src/test/translate_test.cpp
git commit -m "test: 增加聊天翻译策略测试"
```

## 任务 2：用 TDD 落地通用 LLM 后端与智谱 AI 预设

**文件：**
- 修改：`src/game/client/components/qmclient/translate.h`
- 修改：`src/game/client/components/qmclient/translate.cpp`
- 修改：`src/engine/shared/config_variables_qmclient.h`
- 修改：`src/engine/shared/config_variables_qmclient_extra.h`
- 测试：`src/test/translate_test.cpp`

- [ ] **步骤 1：先为通用 LLM 预设和请求体生成写失败测试**

```cpp
TEST(Translate, ResolveLlmPreset_ZhipuAi)
{
	CTranslateLlmPreset Preset = ResolveTranslateLlmPreset("zhipuai");
	EXPECT_STREQ(Preset.m_aEndpoint, "https://open.bigmodel.cn/api/paas/v4/chat/completions");
	EXPECT_STREQ(Preset.m_aModel, "glm-4.7-flash");
}

TEST(Translate, ResolveLlmPreset_Custom)
{
	CTranslateLlmPreset Preset = ResolveTranslateLlmPreset("custom");
	EXPECT_STREQ(Preset.m_aEndpoint, "");
	EXPECT_STREQ(Preset.m_aModel, "");
}

TEST(Translate, BuildGenericLlmPayload_UsesPromptAndUserText)
{
	char aPayload[4096] = "";
	BuildGenericLlmPayload(
		"gpt-4o-mini",
		"Translate to English. Only output the translation.",
		"你好世界",
		aPayload,
		sizeof(aPayload));
	EXPECT_TRUE(str_find(aPayload, "\"model\":\"gpt-4o-mini\"") != nullptr);
	EXPECT_TRUE(str_find(aPayload, "Translate to English. Only output the translation.") != nullptr);
	EXPECT_TRUE(str_find(aPayload, "你好世界") != nullptr);
}
```

- [ ] **步骤 2：运行 `Translate.*` 测试，确认通用 LLM 相关测试先失败**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 -- --gtest_filter=Translate.*
```

预期：`ResolveTranslateLlmPreset`、`BuildGenericLlmPayload` 相关测试失败。

- [ ] **步骤 3：增加通用 LLM 配置项并实现最小后端**

```cpp
MACRO_CONFIG_STR(QmTranslateLlmPreset, qm_translate_llm_preset, 32, "zhipuai", CFGFLAG_CLIENT | CFGFLAG_SAVE, "通用 LLM 预设（zhipuai/custom）")
MACRO_CONFIG_STR(QmTranslateLlmModel, qm_translate_llm_model, 64, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "通用 LLM 模型名称")
MACRO_CONFIG_STR(QmTranslateLlmPrompt, qm_translate_llm_prompt, 512, "You are a professional game translator. Translate to the target language and only output the translation.", CFGFLAG_CLIENT | CFGFLAG_SAVE, "通用 LLM 翻译提示词")
```

```cpp
if(str_comp_nocase(g_Config.m_TcTranslateBackend, "llm") == 0)
	return std::make_unique<CTranslateBackendGenericLlm>(Http, pText, pTarget);
```

```cpp
CTranslateLlmPreset ResolveTranslateLlmPreset(const char *pPreset)
{
	CTranslateLlmPreset Preset;
	if(str_comp_nocase(pPreset, "zhipuai") == 0)
	{
		str_copy(Preset.m_aEndpoint, "https://open.bigmodel.cn/api/paas/v4/chat/completions", sizeof(Preset.m_aEndpoint));
		str_copy(Preset.m_aModel, "glm-4.7-flash", sizeof(Preset.m_aModel));
	}
	return Preset;
}
```

- [ ] **步骤 4：重新运行 `Translate.*` 测试，确认预设与请求体测试通过**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 -- --gtest_filter=Translate.*
```

预期：通用 LLM 与现有翻译测试全部通过。

- [ ] **步骤 5：提交这一步**

```powershell
git add src/game/client/components/qmclient/translate.h src/game/client/components/qmclient/translate.cpp src/engine/shared/config_variables_qmclient.h src/engine/shared/config_variables_qmclient_extra.h src/test/translate_test.cpp
git commit -m "feat: 增加通用LLM翻译后端"
```

## 任务 3：接入聊天翻译行为配置与原设置页

**文件：**
- 修改：`src/engine/shared/config_variables_qmclient.h`
- 修改：`src/engine/shared/config_variables_qmclient_extra.h`
- 修改：`src/game/client/components/qmclient/menus_qmclient.cpp`
- 测试：`src/test/translate_test.cpp`

- [ ] **步骤 1：先为目标语言与按钮可见性辅助函数写失败测试**

```cpp
TEST(Translate, TranslateButtonVisible_OnlyWhenFullChatVisible)
{
	EXPECT_TRUE(ShouldShowChatTranslateButton(true, true, false, false));
	EXPECT_FALSE(ShouldShowChatTranslateButton(false, true, false, false));
	EXPECT_FALSE(ShouldShowChatTranslateButton(true, false, false, false));
	EXPECT_FALSE(ShouldShowChatTranslateButton(true, true, true, false));
	EXPECT_FALSE(ShouldShowChatTranslateButton(true, true, false, true));
}
```

- [ ] **步骤 2：运行 `Translate.*` 测试，确认按钮可见性测试失败**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 -- --gtest_filter=Translate.*
```

预期：`ShouldShowChatTranslateButton` 未定义导致失败。

- [ ] **步骤 3：增加聊天行为配置并更新原设置页后端区**

```cpp
MACRO_CONFIG_INT(QmTranslateChatEnable, qm_translate_chat_enable, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "聊天翻译总开关")
MACRO_CONFIG_INT(QmTranslateIncomingChat, qm_translate_incoming_chat, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "自动翻译玩家聊天")
MACRO_CONFIG_STR(QmTranslateIncomingTarget, qm_translate_incoming_target, 16, "zh", CFGFLAG_CLIENT | CFGFLAG_SAVE, "入站聊天翻译目标语言")
MACRO_CONFIG_INT(QmTranslateOutgoingChat, qm_translate_outgoing_chat, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "自动翻译自己输入的中文聊天")
MACRO_CONFIG_STR(QmTranslateOutgoingTarget, qm_translate_outgoing_target, 16, "en", CFGFLAG_CLIENT | CFGFLAG_SAVE, "出站聊天翻译目标语言")
```

```cpp
s_TranslateBackendDropDownNames = {Localize("Tencent Cloud"), "LibreTranslate", "FTAPI", "LLM"};
```

```cpp
if(str_comp_nocase(g_Config.m_TcTranslateBackend, "llm") == 0)
{
	Ui()->DoLabel(&LabelCol, Localize("Preset"), LG_BodySize, TEXTALIGN_ML);
	Ui()->DoLabel(&LabelCol, Localize("Model"), LG_BodySize, TEXTALIGN_ML);
	Ui()->DoLabel(&LabelCol, Localize("Prompt"), LG_BodySize, TEXTALIGN_ML);
}
```

- [ ] **步骤 4：重新运行 `Translate.*` 测试，确认配置辅助函数测试通过**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 -- --gtest_filter=Translate.*
```

预期：按钮可见性与目标语言辅助测试通过。

- [ ] **步骤 5：提交这一步**

```powershell
git add src/engine/shared/config_variables_qmclient.h src/engine/shared/config_variables_qmclient_extra.h src/game/client/components/qmclient/menus_qmclient.cpp src/test/translate_test.cpp
git commit -m "feat: 增加聊天翻译行为配置"
```

## 任务 4：按 TDD 接回出站翻译与 `/trans` 命令

**文件：**
- 修改：`src/game/client/components/qmclient/translate.h`
- 修改：`src/game/client/components/qmclient/translate.cpp`
- 修改：`src/game/client/components/chat.cpp`
- 测试：`src/test/translate_test.cpp`

- [ ] **步骤 1：先补失败测试，覆盖 `/trans` 优先级与自动中文翻译判定**

```cpp
TEST(Translate, ParseOutgoingTranslateCommand_PreservesPayloadWhitespace)
{
	char aText[256] = "";
	EXPECT_TRUE(ParseOutgoingTranslateCommand("/trans   hello tee", aText, sizeof(aText)));
	EXPECT_STREQ(aText, "hello tee");
}

TEST(Translate, OutgoingAutoTranslate_CommandNeverAutoTranslated)
{
	EXPECT_FALSE(TranslateShouldAutoTranslateOutgoing("/trans 你好"));
	EXPECT_FALSE(TranslateShouldAutoTranslateOutgoing("/timeout 10"));
}
```

- [ ] **步骤 2：运行 `Translate.*` 测试，确认新增出站测试先失败**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 -- --gtest_filter=Translate.*
```

预期：`/trans` 负例或保留空白逻辑失败。

- [ ] **步骤 3：在 `chat.cpp` 和 `translate.cpp` 写最小实现并重新启用出站翻译入口**

```cpp
void CChat::SendChatQueued(int Team, const char *pLine, bool AllowOutgoingTranslation)
{
	if(AllowOutgoingTranslation && GameClient()->m_Translate.TryTranslateOutgoingChat(Team, pLine))
		return;
	// 保持原有队列发送逻辑
}
```

```cpp
bool CTranslate::TryTranslateOutgoingChat(int Team, const char *pText)
{
	char aCommandText[256] = "";
	if(ParseOutgoingTranslateCommand(pText, aCommandText, sizeof(aCommandText)))
		return QueueOutgoingTranslate(Team, aCommandText, g_Config.m_QmTranslateOutgoingTarget);
	if(!g_Config.m_QmTranslateChatEnable || !g_Config.m_QmTranslateOutgoingChat)
		return false;
	if(!TranslateShouldAutoTranslateOutgoing(pText))
		return false;
	return QueueOutgoingTranslate(Team, pText, g_Config.m_QmTranslateOutgoingTarget);
}
```

- [ ] **步骤 4：运行 `Translate.*` 并做一次单点聊天发送验证**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 -- --gtest_filter=Translate.*
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10
```

手动验证：

- 在完整聊天框输入 `/trans 你好`
- 客户端不发送原命令字符串
- 翻译成功后发送翻译结果

- [ ] **步骤 5：提交这一步**

```powershell
git add src/game/client/components/qmclient/translate.h src/game/client/components/qmclient/translate.cpp src/game/client/components/chat.cpp src/test/translate_test.cpp
git commit -m "feat: 接入出站聊天翻译命令"
```

## 任务 5：给完整聊天框接入全局翻译按钮与轻量面板

**文件：**
- 修改：`src/game/client/components/chat.h`
- 修改：`src/game/client/components/chat.cpp`
- 修改：`src/test/translate_test.cpp`

- [ ] **步骤 1：先写失败测试，覆盖按钮显示条件**

```cpp
TEST(Translate, TranslateButtonVisible_ChatActiveAndMouseUnlocked)
{
	EXPECT_TRUE(ShouldShowChatTranslateButton(true, true, false, false));
}

TEST(Translate, TranslateButtonHidden_WhenMenusOrHudEditorActive)
{
	EXPECT_FALSE(ShouldShowChatTranslateButton(true, true, true, false));
	EXPECT_FALSE(ShouldShowChatTranslateButton(true, true, false, true));
}
```

- [ ] **步骤 2：运行 `Translate.*` 测试，确认按钮显示测试先失败或保持红灯**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 -- --gtest_filter=Translate.*
```

预期：如果辅助函数签名调整，相关测试先失败并提示不匹配。

- [ ] **步骤 3：在 `chat.h/.cpp` 实现按钮状态、自动鼠标显示与轻量面板**

```cpp
bool m_ShowTranslatePanel = false;
```

```cpp
const bool ShowTranslateButton = ShouldShowChatTranslateButton(
	m_Mode != MODE_NONE,
	m_MouseUnlocked,
	GameClient()->m_Menus.IsActive(),
	HudEditorPreview);

if(m_Mode != MODE_NONE && !m_MouseUnlocked)
	UnlockMouse();

if(ShowTranslateButton)
{
	if(Ui()->DoButtonLogic(&s_ChatTranslateButton, 0, &TranslateButtonRect, BUTTONFLAG_LEFT))
		m_ShowTranslatePanel = !m_ShowTranslatePanel;

	if(m_ShowTranslatePanel)
	{
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_QmTranslateChatEnable, Localize("Enable chat translate"), &g_Config.m_QmTranslateChatEnable, &PanelRow, LG_LineHeight);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_QmTranslateIncomingChat, Localize("Translate player chat"), &g_Config.m_QmTranslateIncomingChat, &PanelRow, LG_LineHeight);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_QmTranslateOutgoingChat, Localize("Translate my Chinese messages"), &g_Config.m_QmTranslateOutgoingChat, &PanelRow, LG_LineHeight);
	}
}
```

- [ ] **步骤 4：运行测试并做完整聊天框 UI 验证**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 -- --gtest_filter=Translate.*
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10
```

手动验证：

- 打开聊天输入框时自动显示聊天鼠标。
- 普通自动隐藏聊天区域不显示翻译按钮。
- 完整聊天框中显示翻译按钮。
- 点击按钮可展开/收起轻量设置面板。
- 关闭聊天框后鼠标恢复原锁定行为。

- [ ] **步骤 5：提交这一步**

```powershell
git add src/game/client/components/chat.h src/game/client/components/chat.cpp src/test/translate_test.cpp
git commit -m "feat: 增加聊天框翻译按钮"
```

## 任务 6：整体验证与收尾

**文件：**
- 修改：`docs/superpowers/specs/2026-04-21-chat-translation-design.md`
- 修改：相关实现文件（仅当验证暴露命名或行为偏差时）

- [ ] **步骤 1：运行完整的翻译相关测试集**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target testrunner -j 10
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 -- --gtest_filter=Translate.*
```

预期：`Translate.*` 全部通过，且没有新引入的失败。

- [ ] **步骤 2：运行更广的 C++ 测试回归**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10
```

预期：C++ 单元测试整体通过；若失败，定位是否为本次翻译改动引起。

- [ ] **步骤 3：构建客户端并进行手动验收**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10
```

手动验收清单：

- 玩家聊天入站自动翻译生效。
- 服务器消息不自动翻译。
- 自己发送的消息不被再次入站翻译。
- 自动中文出站翻译按开关生效。
- `/trans [text]` 单次翻译命令可用。
- 完整聊天框按钮与轻量设置面板正常工作。

- [ ] **步骤 4：若规格与实现命名不一致，先同步规格再提交**

```powershell
git add docs/superpowers/specs/2026-04-21-chat-translation-design.md
git commit -m "docs: 同步聊天翻译规格与实现"
```

- [ ] **步骤 5：提交最终收尾**

```powershell
git add src/game/client/components/qmclient/translate.h src/game/client/components/qmclient/translate.cpp src/game/client/components/chat.h src/game/client/components/chat.cpp src/game/client/components/qmclient/menus_qmclient.cpp src/engine/shared/config_variables_qmclient.h src/engine/shared/config_variables_qmclient_extra.h src/test/translate_test.cpp
git commit -m "feat: 完成聊天翻译功能"
```
