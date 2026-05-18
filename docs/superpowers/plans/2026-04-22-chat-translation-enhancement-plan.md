# 聊天翻译功能增强实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 增强聊天翻译功能，支持通用 LLM API、自动出站翻译、聊天框翻译按钮 UI

**架构：** 在现有 `CTranslateBackendZhipuAI` 基础上扩展通用 OpenAI 兼容后端，在 `CChat` 中添加翻译按钮 UI 和自动出站翻译逻辑

**技术栈：** C++、CMake、HTTP API、Font Awesome 图标

---

## 文件结构

### 新增文件
- 无

### 修改文件
| 文件 | 职责 |
|-----|------|
| `src/engine/textrender.h` | 添加 `FONT_ICON_LANGUAGE` 图标定义 |
| `src/engine/shared/config_variables_qmclient.h` | 添加新配置变量 |
| `src/engine/shared/config_variables_qmclient_extra.h` | 添加 LLM 相关配置变量 |
| `src/game/client/components/qmclient/translate.h` | 添加自动出站翻译接口 |
| `src/game/client/components/qmclient/translate.cpp` | 实现 OpenAI 后端、自动出站翻译、Prompt 系统 |
| `src/game/client/components/chat.h` | 添加翻译按钮状态结构 |
| `src/game/client/components/chat.cpp` | 实现翻译按钮 UI 和交互 |

---

## 任务 1：添加配置变量

**文件：**
- 修改：`src/engine/shared/config_variables_qmclient.h`
- 修改：`src/engine/shared/config_variables_qmclient_extra.h`

- [ ] **步骤 1：添加 LLM 配置变量到 config_variables_qmclient_extra.h**

在 `src/engine/shared/config_variables_qmclient_extra.h` 的 Translate 部分添加：

```cpp
// Translate - LLM General
MACRO_CONFIG_STR(TcTranslateLlmEndpoint, tc_translate_llm_endpoint, 256, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "通用LLM API端点（留空使用预设）")
MACRO_CONFIG_INT(TcTranslateLlmConcurrency, tc_translate_llm_concurrency, 1, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "LLM翻译并发数（根据服务端限制调整）")
MACRO_CONFIG_STR(TcTranslateSystemPrompt, tc_translate_system_prompt, 512, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "自定义翻译提示词（覆盖内置提示词）")

// Translate - Auto Outgoing
MACRO_CONFIG_INT(TcTranslateAutoOutgoing, tc_translate_auto_outgoing, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "自动翻译发送的消息")
MACRO_CONFIG_INT(TcTranslateAutoOutgoingMode, tc_translate_auto_outgoing_mode, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "自动翻译模式 (0=仅中文输入时触发, 1=始终翻译)")
```

- [ ] **步骤 2：更新默认后端配置**

在 `src/engine/shared/config_variables_qmclient.h` 中修改：

```cpp
MACRO_CONFIG_STR(TcTranslateBackend, tc_translate_backend, 32, "zhipuai", CFGFLAG_CLIENT | CFGFLAG_SAVE, "翻译后端（zhipuai/openai/tencentcloud/libretranslate/ftapi）")
```

- [ ] **步骤 3：构建验证**

运行：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30`

预期：构建成功，无编译错误

- [ ] **步骤 4：Commit**

```bash
git add src/engine/shared/config_variables_qmclient.h src/engine/shared/config_variables_qmclient_extra.h
git commit -m "feat(translate): add LLM and auto-outgoing config variables"
```

---

## 任务 2：添加翻译图标定义

**文件：**
- 修改：`src/engine/textrender.h`

- [ ] **步骤 1：添加 FONT_ICON_LANGUAGE 定义**

在 `src/engine/textrender.h` 的 `FontIcons` 命名空间中添加（按字母顺序插入）：

```cpp
[[maybe_unused]] static const char *FONT_ICON_KEY = "\xEF\x82\x84";
[[maybe_unused]] static const char *FONT_ICON_LANGUAGE = "\xEF\x86\xAB";  // 新增
[[maybe_unused]] static const char *FONT_ICON_LAYER_GROUP = "\xEF\x97\xBD";
```

- [ ] **步骤 2：构建验证**

运行：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30`

预期：构建成功

- [ ] **步骤 3：Commit**

```bash
git add src/engine/textrender.h
git commit -m "feat(ui): add FONT_ICON_LANGUAGE icon for translate button"
```

---

## 任务 3：实现通用 OpenAI 兼容后端

**文件：**
- 修改：`src/game/client/components/qmclient/translate.cpp`

- [ ] **步骤 1：添加内置提示词常量**

在 `translate.cpp` 的匿名命名空间中添加：

```cpp
constexpr const char *DEFAULT_TRANSLATE_PROMPT = 
    "You are a professional translator for the DDNet (DDraceNetwork) game. "
    "Translate the following text to %s. "
    "Game terms: hook=钩子, freeze=冻结, team=队伍, race=竞速, checkpoint=检查点, "
    "unfreeze=解冻, deep freeze=深冻, tele=传送, swap=交换, dummy=分身, "
    "hammer=锤子, shotgun=霰弹枪, grenade=榴弹, laser=激光, ninja=忍者. "
    "Only output the translation result, no explanations.";
```

- [ ] **步骤 2：添加端点预设检测函数**

```cpp
enum class ELlmPreset
{
    NONE,
    ZHIPU_AI,
};

ELlmPreset DetectLlmPreset(const char *pEndpoint)
{
    if(str_find_nocase(pEndpoint, "bigmodel.cn") || str_find_nocase(pEndpoint, "zhipuai"))
        return ELlmPreset::ZHIPU_AI;
    return ELlmPreset::NONE;
}

const char *GetDefaultLlmEndpoint(ELlmPreset Preset)
{
    switch(Preset)
    {
        case ELlmPreset::ZHIPU_AI:
            return "https://open.bigmodel.cn/api/paas/v4/chat/completions";
        default:
            return "https://api.openai.com/v1/chat/completions";
    }
}
```

- [ ] **步骤 3：实现 CTranslateBackendOpenAI 类**

在 `CTranslateBackendZhipuAI` 类之后添加：

```cpp
class CTranslateBackendOpenAI : public ITranslateBackendHttp
{
private:
    bool ParseResponseJson(const json_value *pObj, CTranslateResponse &Out)
    {
        if(!pObj)
        {
            str_copy(Out.m_Text, "Response is not JSON");
            return false;
        }

        if(pObj->type != json_object)
        {
            str_copy(Out.m_Text, "Response is not object");
            return false;
        }

        const json_value *pError = json_object_get(pObj, "error");
        if(pError != &json_value_none)
        {
            const json_value *pMessage = json_object_get(pError, "message");
            const char *pMessageStr = pMessage != &json_value_none && pMessage->type == json_string ? pMessage->u.string.ptr : "OpenAI request failed";
            str_copy(Out.m_Text, pMessageStr);
            return false;
        }

        const json_value *pChoices = json_object_get(pObj, "choices");
        if(pChoices == &json_value_none || pChoices->type != json_array || pChoices->u.array.length == 0)
        {
            str_copy(Out.m_Text, "No valid choices in response");
            return false;
        }

        const json_value *pChoice = pChoices->u.array.values[0];
        const json_value *pMessage = json_object_get(pChoice, "message");
        const json_value *pContent = json_object_get(pMessage, "content");
        
        if(pContent == &json_value_none || pContent->type != json_string)
        {
            str_copy(Out.m_Text, "No content in message");
            return false;
        }

        str_copy(Out.m_Text, pContent->u.string.ptr);
        Out.m_Language[0] = '\0';
        return true;
    }

protected:
    bool ParseResponse(CTranslateResponse &Out) override
    {
        json_value *pObj = m_pHttpRequest->ResultJson();
        bool Res = ParseResponseJson(pObj, Out);
        json_value_free(pObj);
        return Res;
    }

    bool ParseHttpError() const override { return true; }

public:
    const char *Name() const override { return "OpenAI"; }

    CTranslateBackendOpenAI(IHttp &Http, const char *pText, const char *pTarget)
    {
        if(g_Config.m_TcTranslateKey[0] == '\0')
        {
            SetInitError("Missing API Key: set tc_translate_key");
            return;
        }

        // Determine endpoint
        const char *pEndpoint;
        char aEndpointBuf[256];
        if(g_Config.m_TcTranslateLlmEndpoint[0] != '\0')
        {
            str_copy(aEndpointBuf, g_Config.m_TcTranslateLlmEndpoint, sizeof(aEndpointBuf));
            pEndpoint = aEndpointBuf;
        }
        else
        {
            pEndpoint = GetDefaultLlmEndpoint(DetectLlmPreset(g_Config.m_TcTranslateLlmEndpoint));
        }

        // Build system prompt
        char aSystemMessage[1024];
        if(g_Config.m_TcTranslateSystemPrompt[0] != '\0')
        {
            str_copy(aSystemMessage, g_Config.m_TcTranslateSystemPrompt, sizeof(aSystemMessage));
        }
        else
        {
            str_format(aSystemMessage, sizeof(aSystemMessage), DEFAULT_TRANSLATE_PROMPT, EncodeTarget(pTarget));
        }

        // Escape strings for JSON
        char aEscapedText[4096];
        char aEscapedSystem[1024];
        EscapeJsonString(pText, aEscapedText, sizeof(aEscapedText));
        EscapeJsonString(aSystemMessage, aEscapedSystem, sizeof(aEscapedSystem));

        // Determine model
        const char *pModel = g_Config.m_QmTranslateZhipuaiModel[0] != '\0' ? 
                             g_Config.m_QmTranslateZhipuaiModel : "glm-4.5-flash";

        // Build request body
        char aPayload[8192];
        str_format(aPayload, sizeof(aPayload),
            "{"
            "\"model\":\"%s\","
            "\"messages\":["
            "{\"role\":\"system\",\"content\":%s},"
            "{\"role\":\"user\",\"content\":%s}"
            "],"
            "\"temperature\":0.3,"
            "\"max_tokens\":1024"
            "}",
            pModel, aEscapedSystem, aEscapedText);

        // Build Authorization header
        char aAuthorization[512];
        str_format(aAuthorization, sizeof(aAuthorization), "Bearer %s", g_Config.m_TcTranslateKey);

        m_pHttpRequest = std::make_shared<CHttpRequest>(pEndpoint);
        m_pHttpRequest->LogProgress(HTTPLOG::FAILURE);
        m_pHttpRequest->FailOnErrorStatus(false);
        m_pHttpRequest->Timeout(CTimeout{10000, 0, 500, 10});
        m_pHttpRequest->HeaderString("Content-Type", "application/json");
        m_pHttpRequest->HeaderString("Authorization", aAuthorization);
        m_pHttpRequest->Post(reinterpret_cast<const unsigned char *>(aPayload), str_length(aPayload));
        Http.Run(m_pHttpRequest);
    }
};
```

- [ ] **步骤 4：更新 CreateTranslateBackend 函数**

修改 `CreateTranslateBackend` 函数：

```cpp
static std::unique_ptr<ITranslateBackend> CreateTranslateBackend(IHttp &Http, const char *pText, const char *pTarget)
{
    if(str_comp_nocase(g_Config.m_TcTranslateBackend, "libretranslate") == 0)
        return std::make_unique<CTranslateBackendLibretranslate>(Http, pText, pTarget);
    if(str_comp_nocase(g_Config.m_TcTranslateBackend, "ftapi") == 0)
        return std::make_unique<CTranslateBackendFtapi>(Http, pText, pTarget);
    if(str_comp_nocase(g_Config.m_TcTranslateBackend, "tencentcloud") == 0)
        return std::make_unique<CTranslateBackendTencentCloud>(Http, pText, pTarget);
    if(str_comp_nocase(g_Config.m_TcTranslateBackend, "zhipuai") == 0)
        return std::make_unique<CTranslateBackendZhipuAI>(Http, pText, pTarget);
    if(str_comp_nocase(g_Config.m_TcTranslateBackend, "openai") == 0)
        return std::make_unique<CTranslateBackendOpenAI>(Http, pText, pTarget);
    return nullptr;
}
```

- [ ] **步骤 5：构建验证**

运行：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30`

预期：构建成功

- [ ] **步骤 6：Commit**

```bash
git add src/game/client/components/qmclient/translate.cpp
git commit -m "feat(translate): add OpenAI-compatible backend with DDNet game terms prompt"
```

---

## 任务 4：实现中文检测和自动出站翻译

**文件：**
- 修改：`src/game/client/components/qmclient/translate.h`
- 修改：`src/game/client/components/qmclient/translate.cpp`

- [ ] **步骤 1：在 translate.h 中添加接口**

```cpp
class CTranslate : public CComponent
{
    // ... 现有成员 ...
    
public:
    // ... 现有方法 ...
    
    // 自动出站翻译
    bool ShouldAutoTranslateOutgoing(const char *pText) const;
    void StartAutoOutgoingTranslate(int Team, const char *pText);
    
private:
    // 中文检测
    static bool ContainsChinese(const char *pText);
    
    // 获取最大并发数
    int GetMaxConcurrency() const;
};
```

- [ ] **步骤 2：在 translate.cpp 中实现中文检测**

```cpp
bool CTranslate::ContainsChinese(const char *pText)
{
    if(!pText)
        return false;
    
    for(const char *p = pText; *p;)
    {
        const unsigned char c1 = static_cast<unsigned char>(*p);
        if((c1 & 0xF0) == 0xE0) // 3-byte UTF-8
        {
            const unsigned char c2 = static_cast<unsigned char>(*(p + 1));
            const unsigned char c3 = static_cast<unsigned char>(*(p + 2));
            if(c2 && c3)
            {
                const unsigned int codepoint = ((c1 & 0x0F) << 12) | 
                                               ((c2 & 0x3F) << 6) | 
                                               (c3 & 0x3F);
                // CJK Unified Ideographs: 0x4E00-0x9FFF
                // CJK Unified Ideographs Extension A: 0x3400-0x4DBF
                if((codepoint >= 0x4E00 && codepoint <= 0x9FFF) ||
                   (codepoint >= 0x3400 && codepoint <= 0x4DBF))
                    return true;
            }
        }
        p = str_utf8_forward(p, 1);
    }
    return false;
}
```

- [ ] **步骤 3：实现自动出站翻译判断逻辑**

```cpp
bool CTranslate::ShouldAutoTranslateOutgoing(const char *pText) const
{
    if(!g_Config.m_TcTranslateAutoOutgoing)
        return false;
    
    if(!pText || pText[0] == '\0' || pText[0] == '/')
        return false;
    
    // 模式 0: 仅中文输入时触发
    if(g_Config.m_TcTranslateAutoOutgoingMode == 0)
        return ContainsChinese(pText);
    
    // 模式 1: 始终翻译
    return true;
}

void CTranslate::StartAutoOutgoingTranslate(int Team, const char *pText)
{
    if(m_vJobs.size() + m_vOutgoingJobs.size() >= static_cast<size_t>(GetMaxConcurrency()))
    {
        GameClient()->m_Chat.Echo("Translation queue full, sending original text");
        GameClient()->m_Chat.SendChatQueued(Team, pText, false);
        return;
    }
    
    COutgoingTranslateJob Job;
    Job.m_Team = Team;
    str_copy(Job.m_aTarget, g_Config.m_TcTranslateTarget, sizeof(Job.m_aTarget));
    Job.m_pBackend = CreateTranslateBackend(*Http(), pText, Job.m_aTarget);
    
    if(!Job.m_pBackend)
    {
        GameClient()->m_Chat.Echo("Invalid translate backend, sending original text");
        GameClient()->m_Chat.SendChatQueued(Team, pText, false);
        return;
    }
    
    char aBuf[128];
    str_format(aBuf, sizeof(aBuf), "Translating to %s...", Job.m_aTarget);
    GameClient()->m_Chat.Echo(aBuf);
    m_vOutgoingJobs.emplace_back(std::move(Job));
}

int CTranslate::GetMaxConcurrency() const
{
    if(str_comp_nocase(g_Config.m_TcTranslateBackend, "zhipuai") == 0 ||
       str_comp_nocase(g_Config.m_TcTranslateBackend, "openai") == 0)
        return g_Config.m_TcTranslateLlmConcurrency;
    return 5;
}
```

- [ ] **步骤 4：更新 OnRender 中的并发控制**

修改 `CTranslate::OnRender()` 中的任务处理，使用 `GetMaxConcurrency()`：

```cpp
void CTranslate::OnRender()
{
    const int MaxJobs = GetMaxConcurrency();
    // ... 使用 MaxJobs 限制并发 ...
}
```

- [ ] **步骤 5：构建验证**

运行：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30`

预期：构建成功

- [ ] **步骤 6：Commit**

```bash
git add src/game/client/components/qmclient/translate.h src/game/client/components/qmclient/translate.cpp
git commit -m "feat(translate): add auto-outgoing translation with Chinese detection"
```

---

## 任务 5：添加聊天框翻译按钮数据结构

**文件：**
- 修改：`src/game/client/components/chat.h`

- [ ] **步骤 1：添加翻译按钮状态结构**

在 `CChat` 类中添加：

```cpp
class CChat : public CComponent
{
    // ... 现有成员 ...
    
private:
    // 翻译按钮状态
    struct STranslateButtonState
    {
        bool m_IsPressed = false;
        bool m_RectValid = false;
        float m_X = 0.0f;
        float m_Y = 0.0f;
        float m_W = 0.0f;
        float m_H = 0.0f;
        bool m_AutoTranslateEnabled = false;
    };
    STranslateButtonState m_TranslateButton;
    
    // 语言菜单
    int m_LanguageMenuId = 0;
    bool m_LanguageMenuOpen = false;
    
public:
    // 翻译按钮相关方法
    vec2 GetChatMousePos() const;
    void RenderTranslateButton(const CUIRect &InputRect);
    void ToggleAutoTranslate();
    void OpenLanguageMenu();
    bool IsLanguageMenuOpen() const { return m_LanguageMenuOpen; }
    
    // 供 CTranslate 调用的接口
    friend class CTranslate;
};
```

- [ ] **步骤 2：构建验证**

运行：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30`

预期：构建成功（可能有未实现方法的链接错误，下一步解决）

- [ ] **步骤 3：Commit**

```bash
git add src/game/client/components/chat.h
git commit -m "feat(chat): add translate button state structure"
```

---

## 任务 6：实现翻译按钮渲染

**文件：**
- 修改：`src/game/client/components/chat.cpp`

- [ ] **步骤 1：实现 GetChatMousePos 方法**

```cpp
vec2 CChat::GetChatMousePos() const
{
    const float Height = 300.0f;
    const float Width = Height * Graphics()->ScreenAspect();
    const vec2 WindowSize(maximum(1.0f, static_cast<float>(Graphics()->WindowWidth())), 
                          maximum(1.0f, static_cast<float>(Graphics()->WindowHeight())));
    return Ui()->MousePos() * vec2(Ui()->Screen()->w, Ui()->Screen()->h) / WindowSize;
}
```

- [ ] **步骤 2：实现 RenderTranslateButton 方法**

```cpp
void CChat::RenderTranslateButton(const CUIRect &InputRect)
{
    using namespace FontIcons;
    
    const float ButtonSize = maximum(16.0f, m_FontSize * 1.35f);
    const float ButtonGap = 4.0f;
    
    CUIRect ButtonRect;
    ButtonRect.x = InputRect.x + InputRect.w + ButtonGap;
    ButtonRect.y = InputRect.y;
    ButtonRect.w = ButtonSize;
    ButtonRect.h = maximum(InputRect.h, ButtonSize);
    
    m_TranslateButton.m_X = ButtonRect.x;
    m_TranslateButton.m_Y = ButtonRect.y;
    m_TranslateButton.m_W = ButtonRect.w;
    m_TranslateButton.m_H = ButtonRect.h;
    m_TranslateButton.m_RectValid = true;
    
    const vec2 MousePos = GetChatMousePos();
    const bool Hovered = MousePos.x >= ButtonRect.x && 
                         MousePos.x <= ButtonRect.x + ButtonRect.w &&
                         MousePos.y >= ButtonRect.y && 
                         MousePos.y <= ButtonRect.y + ButtonRect.h;
    
    const bool IsOpen = m_LanguageMenuOpen;
    const ColorRGBA ButtonColor = IsOpen ? ColorRGBA(0.35f, 0.45f, 0.70f, 0.90f) :
                                    (Hovered ? ColorRGBA(0.28f, 0.28f, 0.28f, 0.90f) : 
                                               ColorRGBA(0.16f, 0.16f, 0.16f, 0.82f));
    const float ButtonRounding = maximum(3.0f, ButtonRect.h * 0.28f);
    
    ButtonRect.Draw(ButtonColor, IGraphics::CORNER_ALL, ButtonRounding);
    
    CUIRect IconRect;
    ButtonRect.Margin(1.0f, &IconRect);
    const float IconSize = IconRect.h * CUi::ms_FontmodHeight;
    
    TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
    TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | 
                                 ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | 
                                 ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | 
                                 ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGNMENT | 
                                 ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
    TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.95f);
    Ui()->DoLabel(&IconRect, FONT_ICON_LANGUAGE, IconSize, TEXTALIGN_MC);
    TextRender()->SetRenderFlags(0);
    TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
    TextRender()->TextColor(TextRender()->DefaultTextColor());
    
    if(m_TranslateButton.m_AutoTranslateEnabled)
    {
        CUIRect DotRect;
        DotRect.x = ButtonRect.x + ButtonRect.w - 6.0f;
        DotRect.y = ButtonRect.y + 2.0f;
        DotRect.w = 4.0f;
        DotRect.h = 4.0f;
        DotRect.Draw(ColorRGBA(0.2f, 0.8f, 0.2f, 1.0f), IGraphics::CORNER_ALL, 2.0f);
    }
    
    if(Hovered)
    {
        GameClient()->m_Tooltips.DoToolTip(&m_TranslateButton, &ButtonRect, 
            Localize("Auto translate toggle (Right-click for language)"));
    }
}
```

- [ ] **步骤 3：实现 ToggleAutoTranslate 方法**

```cpp
void CChat::ToggleAutoTranslate()
{
    m_TranslateButton.m_AutoTranslateEnabled = !m_TranslateButton.m_AutoTranslateEnabled;
    g_Config.m_TcTranslateAutoOutgoing = m_TranslateButton.m_AutoTranslateEnabled ? 1 : 0;
}
```

- [ ] **步骤 4：在 OnRender 中调用渲染**

在 `CChat::OnRender()` 的输入框渲染部分添加：

```cpp
// 在输入框渲染后
if(m_Mode != MODE_NONE)
{
    // ... 现有输入框渲染代码 ...
    
    // 渲染翻译按钮
    RenderTranslateButton(InputRect);
}
else
{
    m_TranslateButton.m_RectValid = false;
}
```

- [ ] **步骤 5：构建验证**

运行：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30`

预期：构建成功

- [ ] **步骤 6：Commit**

```bash
git add src/game/client/components/chat.cpp
git commit -m "feat(chat): implement translate button rendering"
```

---

## 任务 7：实现翻译按钮输入处理

**文件：**
- 修改：`src/game/client/components/chat.cpp`

- [ ] **步骤 1：在 OnInput 中添加按钮点击处理**

在 `CChat::OnInput()` 函数开头添加：

```cpp
bool CChat::OnInput(const IInput::CEvent &Event)
{
    // ===== 翻译按钮处理（优先级高于输入框）=====
    if(m_Mode != MODE_NONE && m_TranslateButton.m_RectValid)
    {
        const vec2 MousePos = GetChatMousePos();
        const bool InsideButton = 
            MousePos.x >= m_TranslateButton.m_X && 
            MousePos.x <= m_TranslateButton.m_X + m_TranslateButton.m_W &&
            MousePos.y >= m_TranslateButton.m_Y && 
            MousePos.y <= m_TranslateButton.m_Y + m_TranslateButton.m_H;
        
        // 左键处理
        if(Event.m_Key == KEY_MOUSE_1)
        {
            if(Event.m_Flags & IInput::FLAG_PRESS)
            {
                m_TranslateButton.m_IsPressed = InsideButton;
                if(InsideButton)
                {
                    // 重置输入框的鼠标选择状态
                    CLineInput::SMouseSelection *pMouseSel = m_Input.GetMouseSelection();
                    if(pMouseSel)
                    {
                        pMouseSel->m_Selecting = false;
                        pMouseSel->m_PressMouse = vec2(0, 0);
                        pMouseSel->m_ReleaseMouse = vec2(0, 0);
                    }
                    return true;
                }
            }
            else if(Event.m_Flags & IInput::FLAG_RELEASE)
            {
                const bool Activate = m_TranslateButton.m_IsPressed && InsideButton;
                m_TranslateButton.m_IsPressed = false;
                if(Activate)
                {
                    ToggleAutoTranslate();
                    return true;
                }
            }
        }
        
        // 右键处理
        if(Event.m_Key == KEY_MOUSE_2 && InsideButton && (Event.m_Flags & IInput::FLAG_PRESS))
        {
            OpenLanguageMenu();
            return true;
        }
    }
    
    // ===== 原有输入处理 =====
    // ... 现有代码 ...
}
```

- [ ] **步骤 2：实现 OpenLanguageMenu 方法**

```cpp
void CChat::OpenLanguageMenu()
{
    m_LanguageMenuOpen = !m_LanguageMenuOpen;
    if(m_LanguageMenuOpen)
    {
        Ui()->EnablePopupMenu(&m_LanguageMenuId);
    }
}
```

- [ ] **步骤 3：构建验证**

运行：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30`

预期：构建成功

- [ ] **步骤 4：Commit**

```bash
git add src/game/client/components/chat.cpp
git commit -m "feat(chat): implement translate button input handling with mouse selection reset"
```

---

## 任务 8：集成自动出站翻译到聊天发送流程

**文件：**
- 修改：`src/game/client/components/chat.cpp`

- [ ] **步骤 1：修改 SendChat 函数**

找到发送聊天的位置，添加自动翻译检查：

```cpp
// 在发送消息前检查是否需要自动翻译
if(GameClient()->m_Translate.ShouldAutoTranslateOutgoing(pLine))
{
    GameClient()->m_Translate.StartAutoOutgoingTranslate(Team, pLine);
    return; // 等待翻译完成后再发送
}

// 原有发送逻辑
GameClient()->m_Translate.TryTranslateOutgoingChat(Team, pLine) ||
SendChatQueued(Team, pLine);
```

- [ ] **步骤 2：构建验证**

运行：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30`

预期：构建成功

- [ ] **步骤 3：Commit**

```bash
git add src/game/client/components/chat.cpp
git commit -m "feat(chat): integrate auto-outgoing translation into chat send flow"
```

---

## 任务 9：运行测试验证

**文件：**
- 无修改

- [ ] **步骤 1：运行 C++ 测试**

运行：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests`

预期：所有测试通过

- [ ] **步骤 2：手动功能测试**

1. 启动客户端：`cd build-ninja && .\DDNet.exe`
2. 测试翻译按钮：
   - 打开聊天框
   - 悬停翻译按钮，确认工具提示显示
   - 点击按钮，确认状态指示点切换
3. 测试自动出站翻译：
   - 开启自动翻译
   - 输入中文消息并发送
   - 确认翻译后发送

- [ ] **步骤 3：最终 Commit**

```bash
git add -A
git commit -m "feat(translate): complete chat translation enhancement

- Add OpenAI-compatible backend with DDNet game terms prompt
- Add auto-outgoing translation with Chinese detection
- Add translate button UI with mouse selection fix
- Add configurable concurrency (1-100)
- Update default backend to zhipuai"
```

---

## 验收标准

- [ ] 智谱AI 后端正常工作
- [ ] 通用 OpenAI 后端正常工作
- [ ] 翻译按钮正确渲染和响应点击
- [ ] 按钮点击不触发输入框光标选择
- [ ] 自动出站翻译正确检测中文
- [ ] 翻译失败时回退到原文发送
- [ ] 并发数配置生效
- [ ] 所有 C++ 测试通过
