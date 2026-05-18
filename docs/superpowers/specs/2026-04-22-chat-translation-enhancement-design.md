# 聊天翻译功能增强设计文档

**日期**: 2026-04-22  
**版本**: 1.2  
**作者**: AI Assistant  
**状态**: 设计完成，待实现

---

## 1. 概述

### 1.1 背景

QmClient 已具备基础翻译功能（腾讯、Libre、智谱AI、FTAPI），但需要增强以下能力：

1. **通用 LLM API 支持** - 支持 OpenAI 兼容格式的任意 LLM 服务
2. **自动出站翻译** - 检测中文输入自动翻译后发送
3. **聊天框翻译按钮** - 在输入框旁提供可视化控制
4. **Prompt 优化** - 内置游戏术语优化，支持自定义提示词

### 1.2 目标

- 提供完整的入站/出站双向翻译体验
- 支持主流 LLM 服务（OpenAI、DeepSeek、智谱AI等）
- 保持与现有代码架构的兼容性
- 提供直观易用的 UI 交互

---

## 2. 架构设计

### 2.1 方案选择

采用 **方案A：最小改动扩展**

- 复用现有 `ITranslateBackend` 架构
- 在 `translate.cpp` 中增量添加功能
- 保持向后兼容性

### 2.2 模块交互

```
┌─────────────────────────────────────────────────────────────────┐
│                           CChat                                 │
│  ┌─────────────────┐    ┌─────────────────┐                     │
│  │   输入框渲染     │───▶│   翻译按钮渲染   │                     │
│  │   (OnRender)    │    │   (Render)      │                     │
│  └─────────────────┘    └─────────────────┘                     │
│           │                      │                              │
│           ▼                      ▼                              │
│  ┌─────────────────────────────────────────┐                   │
│  │           OnInput 事件处理               │                   │
│  │  1. 优先检查翻译按钮（阻止输入框处理）    │                   │
│  │  2. 重置 m_MouseSelection 状态           │                   │
│  │  3. 消费事件，不传递给输入框             │                   │
│  └─────────────────────────────────────────┘                   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                        CTranslate                               │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────┐ │
│  │   入站翻译       │    │   出站翻译       │    │  自动检测    │ │
│  │   (现有)        │    │   (新增)        │    │  中文输入    │ │
│  └─────────────────┘    └─────────────────┘    └─────────────┘ │
│           │                    │                    │          │
│           └────────────────────┼────────────────────┘          │
│                                ▼                               │
│              ┌─────────────────────────────────┐               │
│              │      ITranslateBackend          │               │
│              │   - CTranslateBackendOpenAI     │               │
│              │   - CTranslateBackendTencent    │               │
│              │   - CTranslateBackendLibre      │               │
│              │   - CTranslateBackendZhipuAI    │               │
│              └─────────────────────────────────┘               │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3. 功能规格

### 3.1 通用 LLM API 后端

#### 3.1.1 支持的后端类型

| 后端标识 | 说明 | 端点格式 |
|---------|------|---------|
| `openai` | 通用 OpenAI 兼容 API | 用户自定义或预设 |
| `zhipuai` | 智谱AI（OpenAI 兼容模式） | 内置默认端点 |
| `tencentcloud` | 腾讯云 TMT | 现有 |
| `libretranslate` | LibreTranslate | 现有 |
| `ftapi` | FreeTranslateAPI | 现有 |

#### 3.1.2 后端预设与 LLM 通用接口

**设计理念**：
- `openai` 和 `zhipuai` 后端都使用 **统一的 LLM 通用接口**（OpenAI 兼容格式）
- 区别仅在于 **预设端点** 和 **默认模型**
- 用户可通过配置变量覆盖任何预设

**智谱AI 预设**：
- 端点：`https://open.bigmodel.cn/api/paas/v4/chat/completions`（内置）
- 默认模型：`glm-4.5-flash`
- 认证方式：Bearer Token（API Key）
- **服务端限制**：免费模型 `glm-4.5-flash` 并发数限制为 2（这是智谱AI服务端限制，不是客户端限制）

**OpenAI 预设**：
- 端点：`https://api.openai.com/v1/chat/completions`（内置）
- 默认模型：`gpt-3.5-turbo`
- 认证方式：Bearer Token（API Key）

**其他兼容服务**（用户自定义端点）：
- DeepSeek：`https://api.deepseek.com/v1/chat/completions`
- 本地模型：`http://localhost:11434/v1/chat/completions`（Ollama）
- 其他 OpenAI 兼容服务

**并发数配置**：
- 范围：1-100（用户根据服务端限制自行调整）
- 默认：1（安全值，适用于所有限制较严格的服务）
- 注意：智谱AI免费模型需设为 1-2，付费版或其他服务可设更高

#### 3.1.3 配置变量

```cpp
// 新增/修改配置变量
MACRO_CONFIG_STR(TcTranslateBackend, tc_translate_backend, 32, "zhipuai", CFGFLAG_CLIENT | CFGFLAG_SAVE, "翻译后端 (zhipuai/openai/tencentcloud/libretranslate/ftapi)")
MACRO_CONFIG_STR(TcTranslateLlmModel, tc_translate_llm_model, 64, "glm-4.5-flash", CFGFLAG_CLIENT | CFGFLAG_SAVE, "通用LLM翻译模型名称")
MACRO_CONFIG_STR(TcTranslateLlmEndpoint, tc_translate_llm_endpoint, 256, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "通用LLM API端点（留空使用预设）")
MACRO_CONFIG_INT(TcTranslateLlmConcurrency, tc_translate_llm_concurrency, 1, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "LLM翻译并发数（根据服务端限制调整，智谱AI免费模型最大2）")
MACRO_CONFIG_STR(TcTranslateSystemPrompt, tc_translate_system_prompt, 512, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "自定义翻译提示词（覆盖内置提示词）")
```

#### 3.1.4 端点自动检测

```cpp
enum class ELlmPreset
{
    NONE,       // 通用OpenAI兼容
    ZHIPU_AI,   // 智谱AI
};

ELlmPreset DetectLlmPreset(const char* pEndpoint)
{
    if(str_find_nocase(pEndpoint, "bigmodel.cn") || 
       str_find_nocase(pEndpoint, "zhipuai"))
        return ELlmPreset::ZHIPU_AI;
    return ELlmPreset::NONE;
}

const char* GetDefaultEndpoint(ELlmPreset Preset)
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

#### 3.1.5 内置提示词

**默认提示词**（针对 DDNet 游戏术语优化）：

```
You are a professional translator for the DDNet (DDraceNetwork) game.
Translate the following text to {target_language}.

Common game terms:
- hook = 钩子
- freeze = 冻结
- deep freeze = 深冻
- unfreeze = 解冻
- team = 队伍
- race = 竞速
- checkpoint = 检查点 / CP
- tele = 传送
- swap = 交换
- dummy = 分身
- hammer = 锤子
- shotgun = 霰弹枪
- grenade = 榴弹
- laser = 激光
- ninja = 忍者
- fly = 飞
- solo = 单人
- part = 段落
- go = 走 / 出发
- wait = 等待

Only output the translation result, no explanations.
```

**自定义提示词**（用户可通过 `tc_translate_system_prompt` 覆盖）

#### 3.1.6 请求格式

```json
{
  "model": "glm-4.5-flash",
  "messages": [
    {
      "role": "system",
      "content": "You are a professional translator..."
    },
    {
      "role": "user",
      "content": "text to translate"
    }
  ],
  "temperature": 0.3,
  "max_tokens": 1024
}
```

#### 3.1.7 响应解析

```cpp
// 成功响应
{
  "choices": [
    {
      "message": {
        "content": "translated text"
      }
    }
  ]
}

// 错误响应
{
  "error": {
    "message": "error description"
  }
}
```

---

### 3.2 自动出站翻译

#### 3.2.1 功能描述

检测用户输入，在发送前自动翻译为目标语言。

#### 3.2.2 触发模式

| 模式 | 说明 |
|-----|------|
| 0 - 仅中文 | 检测到中文输入时自动触发（默认） |
| 1 - 始终 | 所有输入都自动翻译 |

#### 3.2.3 中文检测算法

```cpp
bool ContainsChinese(const char* pText)
{
    // 检查 UTF-8 中文字符
    // CJK Unified Ideographs: 0x4E00-0x9FFF
    // CJK Unified Ideographs Extension A: 0x3400-0x4DBF
    
    for(const char* p = pText; *p;)
    {
        const unsigned char c1 = (unsigned char)*p;
        if((c1 & 0xF0) == 0xE0) // 3-byte UTF-8
        {
            const unsigned char c2 = (unsigned char)*(p+1);
            const unsigned char c3 = (unsigned char)*(p+2);
            if(c2 && c3)
            {
                const unsigned int codepoint = ((c1 & 0x0F) << 12) | 
                                               ((c2 & 0x3F) << 6) | 
                                               (c3 & 0x3F);
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

#### 3.2.4 目标语言策略

- **默认**：使用 `tc_translate_target` 配置
- **临时切换**：通过翻译按钮右键菜单快速切换
- **混合模式**：默认固定语言，可临时切换

#### 3.2.5 配置变量

```cpp
MACRO_CONFIG_INT(TcTranslateAutoOutgoing, tc_translate_auto_outgoing, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "自动翻译发送的消息")
MACRO_CONFIG_INT(TcTranslateAutoOutgoingMode, tc_translate_auto_outgoing_mode, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "自动翻译模式 (0=仅中文输入时触发, 1=始终翻译)")
MACRO_CONFIG_STR(TcTranslateTempTarget, tc_translate_temp_target, 16, "", CFGFLAG_CLIENT, "临时目标语言（覆盖默认设置）")
```

#### 3.2.6 发送流程

```
用户按 Enter
    │
    ▼
检查 TcTranslateAutoOutgoing
    │
    ├── 关闭 ─────────────────▶ 直接发送原文
    │
    └── 开启
            │
            ▼
    检查触发模式
            │
            ├── 仅中文模式 ──▶ 检测中文 ──▶ 无中文 ──▶ 直接发送原文
            │                      │
            │                      ▼
            │                   有中文
            │                      │
            ▼                      ▼
    始终翻译模式            创建翻译任务
            │                      │
            │                      ▼
            │               等待翻译完成
            │                      │
            │                      ▼
            │               发送翻译结果
            │                      │
            └──────────────────────┘
```

---

### 3.3 聊天框翻译按钮

#### 3.3.1 功能描述

在聊天输入框右侧提供翻译控制按钮，支持：
- 左键：切换自动翻译开关
- 右键：打开语言选择菜单

#### 3.3.2 视觉设计

```
┌─────────────────────────────────────────────────────────────────┐
│ Team: [输入文字在这里...                          ] [A 文] ●  │
└─────────────────────────────────────────────────────────────────┘
                                                    └──┘  └─
                                                    按钮   指示点
```

**图标**：
- 来源：Font Awesome 6 - LANGUAGE 图标
- Unicode：`\uF1AB`（UTF-8: `\xEF\x86\xAB`）
- 视觉：经典的翻译/语言切换符号，显示 "A" 和 "文" 字符
- 需要在 `src/engine/textrender.h` 中添加定义：
  ```cpp
  [[maybe_unused]] static const char *FONT_ICON_LANGUAGE = "\xEF\x86\xAB";
  ```

**尺寸**：
- 按钮大小：`max(16.0f, FontSize * 1.35f)`
- 与输入框间隙：`4.0f`
- 圆角：`max(3.0f, ButtonHeight * 0.28f)`

**颜色状态**：
| 状态 | 颜色 |
|-----|------|
| 默认 | `ColorRGBA(0.16f, 0.16f, 0.16f, 0.82f)` |
| 悬停 | `ColorRGBA(0.28f, 0.28f, 0.28f, 0.90f)` |
| 菜单打开 | `ColorRGBA(0.35f, 0.45f, 0.70f, 0.90f)` |
| 图标颜色 | `ColorRGBA(1.0f, 1.0f, 1.0f, 0.95f)` |
| 状态指示点（开启） | `ColorRGBA(0.2f, 0.8f, 0.2f, 1.0f)` |

#### 3.3.3 数据结构

```cpp
// chat.h
struct STranslateButtonState
{
    bool m_IsPressed = false;      // 按钮按下状态
    bool m_RectValid = false;      // 位置是否有效
    float m_X = 0.0f;              // 按钮位置 X
    float m_Y = 0.0f;              // 按钮位置 Y
    float m_W = 0.0f;              // 按钮宽度
    float m_H = 0.0f;              // 按钮高度
    bool m_AutoTranslateEnabled = false;  // 自动翻译开关状态
};

STranslateButtonState m_TranslateButton;
```

#### 3.3.4 输入处理（关键）

```cpp
bool CChat::OnInput(const IInput::CEvent &Event)
{
    // ===== 1. 优先检查翻译按钮（在输入框处理之前）=====
    if(m_Mode != MODE_NONE && Event.m_Key == KEY_MOUSE_1 && m_TranslateButton.m_RectValid)
    {
        const vec2 MousePos = GetChatMousePos();
        const bool InsideButton = 
            MousePos.x >= m_TranslateButton.m_X && 
            MousePos.x <= m_TranslateButton.m_X + m_TranslateButton.m_W &&
            MousePos.y >= m_TranslateButton.m_Y && 
            MousePos.y <= m_TranslateButton.m_Y + m_TranslateButton.m_H;
        
        if(Event.m_Flags & IInput::FLAG_PRESS)
        {
            m_TranslateButton.m_IsPressed = InsideButton;
            if(InsideButton)
            {
                // 关键：重置输入框的鼠标选择状态
                CLineInput::SMouseSelection *pMouseSel = m_Input.GetMouseSelection();
                pMouseSel->m_Selecting = false;
                pMouseSel->m_PressMouse = vec2(0, 0);
                pMouseSel->m_ReleaseMouse = vec2(0, 0);
                return true; // 消费事件，不传递给输入框
            }
        }
        else if(Event.m_Flags & IInput::FLAG_RELEASE)
        {
            const bool Activate = m_TranslateButton.m_IsPressed && InsideButton;
            m_TranslateButton.m_IsPressed = false;
            if(Activate)
            {
                ToggleAutoTranslate(); // 切换自动翻译
                return true;
            }
        }
    }
    
    // 右键打开语言菜单
    if(m_Mode != MODE_NONE && Event.m_Key == KEY_MOUSE_2 && m_TranslateButton.m_RectValid)
    {
        const vec2 MousePos = GetChatMousePos();
        const bool InsideButton = 
            MousePos.x >= m_TranslateButton.m_X && 
            MousePos.x <= m_TranslateButton.m_X + m_TranslateButton.m_W &&
            MousePos.y >= m_TranslateButton.m_Y && 
            MousePos.y <= m_TranslateButton.m_Y + m_TranslateButton.m_H;
        
        if(InsideButton && (Event.m_Flags & IInput::FLAG_PRESS))
        {
            OpenLanguageMenu(); // 打开语言选择菜单
            return true;
        }
    }
    
    // ===== 2. 原有输入处理 =====
    // ... m_Input.ProcessInput(Event) ...
}
```

#### 3.3.5 语言选择菜单

```cpp
void OpenLanguageMenu()
{
    // 常用语言列表
    static const char* aLanguages[] = {
        "en",    // English
        "zh",    // 中文
        "ja",    // 日本語
        "ko",    // 한국어
        "ru",    // Русский
        "de",    // Deutsch
        "fr",    // Français
        "es",    // Español
        "pt",    // Português
    };
    
    // 打开下拉菜单
    Ui()->OpenPopupMenu(&m_LanguageMenuId, m_TranslateButton.m_X, 
                        m_TranslateButton.m_Y + m_TranslateButton.m_H);
}
```

#### 3.3.6 按钮渲染代码

```cpp
void CChat::RenderTranslateButton(const CUIRect &ButtonRect)
{
    // 保存按钮位置用于点击检测
    m_TranslateButton.m_X = ButtonRect.x;
    m_TranslateButton.m_Y = ButtonRect.y;
    m_TranslateButton.m_W = ButtonRect.w;
    m_TranslateButton.m_H = ButtonRect.h;
    m_TranslateButton.m_RectValid = true;

    // 计算悬停状态
    const vec2 MousePos = GetChatMousePos();
    const bool Hovered = MousePos.x >= ButtonRect.x && 
                         MousePos.x <= ButtonRect.x + ButtonRect.w &&
                         MousePos.y >= ButtonRect.y && 
                         MousePos.y <= ButtonRect.y + ButtonRect.h;
    
    // 菜单打开状态
    const bool IsOpen = Ui()->IsPopupOpen(&m_TranslateSettingsPopupId);
    
    // 按钮背景颜色
    const ColorRGBA ButtonColor = IsOpen ? ColorRGBA(0.35f, 0.45f, 0.70f, 0.90f) :
                                (Hovered ? ColorRGBA(0.28f, 0.28f, 0.28f, 0.90f) : 
                                           ColorRGBA(0.16f, 0.16f, 0.16f, 0.82f));
    const float ButtonRounding = maximum(3.0f, ButtonRect.h * 0.28f);
    
    // 绘制按钮背景
    ButtonRect.Draw(ButtonColor, IGraphics::CORNER_ALL, ButtonRounding);
    
    // 绘制图标
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
    Ui()->DoLabel(&IconRect, FontIcons::FONT_ICON_LANGUAGE, IconSize, TEXTALIGN_MC);
    TextRender()->SetRenderFlags(0);
    TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
    TextRender()->TextColor(TextRender()->DefaultTextColor());
    
    // 状态指示点（自动翻译开启时显示绿色小圆点）
    if(m_TranslateButton.m_AutoTranslateEnabled)
    {
        CUIRect DotRect;
        DotRect.x = ButtonRect.x + ButtonRect.w - 6.0f;
        DotRect.y = ButtonRect.y + 2.0f;
        DotRect.w = 4.0f;
        DotRect.h = 4.0f;
        DotRect.Draw(ColorRGBA(0.2f, 0.8f, 0.2f, 1.0f), IGraphics::CORNER_ALL, 2.0f);
    }
    
    // 工具提示
    if(Hovered)
    {
        Ui()->SetHotItem(&m_TranslateSettingsButton);
        GameClient()->m_Tooltips.DoToolTip(&m_TranslateSettingsButton, &ButtonRect, 
                                           Localize("Chat translate settings"));
    }
}
```

---

### 3.4 并发控制

#### 3.4.1 任务队列管理

```cpp
class CTranslate
{
    // 当前进行的任务
    std::vector<CTranslateJob> m_vJobs;
    
    // 待处理任务（超出并发限制时排队）
    std::deque<CTranslateJob> m_vPendingJobs;
    
    void OnRender()
    {
        // 获取当前后端的最大并发数
        int MaxConcurrency = GetMaxConcurrency();
        
        // 启动排队中的任务
        while((int)m_vJobs.size() < MaxConcurrency && !m_vPendingJobs.empty())
        {
            m_vJobs.push_back(std::move(m_vPendingJobs.front()));
            m_vPendingJobs.pop_front();
        }
        
        // 更新进行中的任务
        UpdateJobs();
    }
    
    int GetMaxConcurrency()
    {
        // LLM 后端使用用户配置的并发数
        if(str_comp_nocase(g_Config.m_TcTranslateBackend, "zhipuai") == 0 ||
           str_comp_nocase(g_Config.m_TcTranslateBackend, "openai") == 0)
            return g_Config.m_TcTranslateLlmConcurrency;
        return 5; // 传统翻译后端默认5
    }
};
```

---

## 4. 接口定义

### 4.1 新增后端类

```cpp
// translate.h
class CTranslateBackendOpenAI : public ITranslateBackendHttp
{
private:
    bool ParseResponse(CTranslateResponse &Out) override;
    bool ParseHttpError() const override { return true; }
    
    void BuildRequestBody(char* pOut, size_t OutSize, const char* pText, const char* pTarget);
    void BuildSystemPrompt(char* pOut, size_t OutSize, const char* pTarget);
    
public:
    const char* Name() const override { return "OpenAI"; }
    const char* EncodeTarget(const char* pTarget) const override;
    
    CTranslateBackendOpenAI(IHttp &Http, const char* pText, const char* pTarget);
};
```

### 4.2 新增 CTranslate 方法

```cpp
// translate.h
class CTranslate : public CComponent
{
public:
    // ... 现有方法 ...
    
    // 出站翻译
    bool TryTranslateOutgoingChat(int Team, const char* pText);
    
    // 自动翻译开关
    void ToggleAutoTranslate();
    bool IsAutoTranslateEnabled() const { return m_AutoTranslateEnabled; }
    
    // 临时目标语言
    void SetTempTargetLanguage(const char* pLang);
    const char* GetEffectiveTarget() const;
    
private:
    // 中文检测
    bool ContainsChinese(const char* pText);
    
    // 最大并发数
    int GetMaxConcurrency() const;
    
    // 待处理队列
    std::deque<COutgoingTranslateJob> m_vPendingJobs;
    bool m_AutoTranslateEnabled = false;
    char m_aTempTargetLanguage[16] = "";
};
```

### 4.3 CChat 新增成员

```cpp
// chat.h
class CChat : public CComponent
{
    // ... 现有成员 ...
    
    // 翻译按钮状态
    struct STranslateButtonState
    {
        bool m_IsPressed = false;
        bool m_RectValid = false;
        float m_X, m_Y, m_W, m_H;
    };
    STranslateButtonState m_TranslateButton;
    
    // 方法
    void RenderTranslateButton(const CUIRect &InputRect);
    vec2 GetChatMousePos() const;
    void ToggleAutoTranslate();
    void OpenLanguageMenu();
    
    friend class CTranslate;
};
```

---

## 5. 配置说明

### 5.1 完整配置变量列表

| 变量名 | 类型 | 默认值 | 范围 | 说明 |
|-------|------|--------|------|------|
| `tc_translate_backend` | str | "zhipuai" | - | 翻译后端类型 |
| `tc_translate_target` | str | "zh" | - | 默认目标语言 |
| `tc_translate_endpoint` | str | "https://tmt.tencentcloudapi.com/" | - | 腾讯云端点 |
| `tc_translate_key` | str | "" | - | API Key |
| `tc_translate_secret_id` | str | "" | - | 腾讯云 SecretId |
| `tc_translate_secret_key` | str | "" | - | 腾讯云 SecretKey |
| `tc_translate_region` | str | "ap-guangzhou" | - | 腾讯云地域 |
| `tc_translate_auto` | int | 0 | 0-1 | 自动翻译入站消息 |
| `tc_translate_llm_model` | str | "glm-4.5-flash" | - | LLM模型名称 |
| `tc_translate_llm_endpoint` | str | "" | - | LLM端点（留空使用预设） |
| `tc_translate_llm_concurrency` | int | 1 | 1-100 | LLM并发数（根据服务端限制调整） |
| `tc_translate_system_prompt` | str | "" | - | 自定义系统提示词 |
| `tc_translate_auto_outgoing` | int | 0 | 0-1 | 自动翻译发送的消息 |
| `tc_translate_auto_outgoing_mode` | int | 0 | 0-1 | 触发模式 |

### 5.2 使用示例

```bash
# 使用智谱AI（默认）
tc_translate_backend zhipuai
tc_translate_key "your-api-key"
tc_translate_target en
tc_translate_auto_outgoing 1

# 使用 OpenAI
tc_translate_backend openai
tc_translate_llm_endpoint "https://api.openai.com/v1/chat/completions"
tc_translate_llm_model "gpt-3.5-turbo"
tc_translate_key "your-openai-key"

# 使用 DeepSeek
tc_translate_backend openai
tc_translate_llm_endpoint "https://api.deepseek.com/v1/chat/completions"
tc_translate_llm_model "deepseek-chat"
tc_translate_key "your-deepseek-key"
```

---

## 6. 测试计划

### 6.1 单元测试

| 测试项 | 描述 |
|-------|------|
| 中文检测 | 验证 ContainsChinese 函数正确识别中英文混合文本 |
| 端点检测 | 验证 DetectLlmPreset 正确识别智谱AI端点 |
| 提示词构建 | 验证 BuildSystemPrompt 正确替换目标语言变量 |
| JSON 解析 | 验证 ParseResponse 正确处理成功/错误响应 |

### 6.2 集成测试

| 测试项 | 描述 |
|-------|------|
| 智谱AI 翻译 | 端到端测试智谱AI翻译流程 |
| 并发限制 | 验证并发数限制为1时任务排队行为 |
| 按钮交互 | 验证按钮点击不触发输入框光标 |
| 自动翻译 | 验证中文输入自动触发翻译并发送 |

### 6.3 回归测试

| 测试项 | 描述 |
|-------|------|
| 现有后端 | 验证腾讯、Libre等现有后端仍正常工作 |
| 入站翻译 | 验证现有入站翻译功能不受影响 |
| 聊天功能 | 验证普通聊天功能不受影响 |

---

## 7. 风险评估

| 风险 | 影响 | 缓解措施 |
|-----|------|---------|
| API 调用失败 | 消息发送失败 | 翻译失败时回退到原文发送 |
| 并发超限 | API 限流 | 默认并发设为1，用户根据服务端限制调整 |
| 响应延迟 | 发送延迟 | 异步处理，添加加载指示 |
| UI 冲突 | 按钮与光标冲突 | 优先检查按钮，重置选择状态 |

---

## 8. 实现计划

详见配套实现计划文档：`2026-04-22-chat-translation-enhancement-plan.md`

---

## 9. 附录

### 9.1 智谱AI API 参考

**端点**: `https://open.bigmodel.cn/api/paas/v4/chat/completions`

**认证**: `Authorization: Bearer YOUR_API_KEY`

**模型**: `glm-4.5-flash`（默认）

**服务端限制**: 免费模型 `glm-4.5-flash` 并发数限制为 2（付费版或其他模型可能不同）

### 9.2 语言代码参考

| 代码 | 语言 |
|-----|------|
| zh | 简体中文 |
| zh-TW | 繁体中文 |
| en | English |
| ja | 日本語 |
| ko | 한국어 |
| ru | Русский |
| de | Deutsch |
| fr | Français |
| es | Español |
| pt | Português |

---

**文档结束**
