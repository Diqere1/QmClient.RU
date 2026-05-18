# 翻译按钮菜单重设计规格

## 概述

将聊天输入框旁的翻译按钮弹出菜单从当前平铺按钮列表改为下拉框式菜单，并拆分为"入站语言"和"出站语言"两个独立设置，严格遵循 brainstorm 设计文档的视觉风格。

## 动机

当前实现的问题：
1. 菜单使用平铺按钮列表，占用空间过大
2. 只有一个"目标语言"设置，无法区分接收消息和发送消息的翻译目标语言
3. 菜单选中色为浅绿色，与 brainstorm 设计的蓝紫色主题不一致

## 设计细节

### 菜单布局

菜单从按钮**上方弹出**，宽度 **150px**，背景色使用 `g_Config.m_QmTranslateMenuBgColor`。

菜单内部结构（从上到下）：
1. **标题**："翻译设置"
2. **自动翻译开关**：一行可点击的按钮，显示"自动翻译: 开/关"
3. **分隔线**
4. **入站语言**（接收消息翻译成的语言）：标签 + 下拉框
5. **出站语言**（发送消息时翻译成的语言）：标签 + 下拉框
6. **分隔线**
7. **翻译后端**：标签 + 下拉框
8. **后端未配置警告**（条件显示）

### 颜色与视觉风格

**按钮颜色**（保持当前可配置）：
- 未启用：`g_Config.m_QmTranslateBtnColorDisabled`（默认深灰色 `rgba(41, 41, 41, 0.82)`）
- 已启用：`g_Config.m_QmTranslateBtnColorEnabled`（默认蓝紫色 `rgba(89, 115, 179, 0.90)`）
- 悬停：浅灰色 `rgba(0.28, 0.28, 0.28, 0.90)`
- 菜单打开中：蓝紫色 `rgba(0.35, 0.45, 0.70, 0.90)`

**菜单颜色**（保持当前可配置）：
- 背景：`g_Config.m_QmTranslateMenuBgColor`（默认 `rgba(30, 30, 30, 0.95)`）
- 选项选中：`g_Config.m_QmTranslateMenuOptionSelected`（默认蓝紫色 `rgba(89, 115, 179, 0.90)`）
- 选项未选中：`g_Config.m_QmTranslateMenuOptionNormal`（默认灰色 `rgba(50, 50, 50, 0.90)`）

**圆角**：按钮和菜单选项统一使用 **5px 圆角**

**分隔线**：灰色半透明线 `rgba(0.5, 0.5, 0.5, 0.5)`

### 下拉框交互

**状态管理**：
- 每个下拉框需要独立的展开/收起状态
- 同一时间最多只有一个下拉框展开
- 点击下拉框头部：展开列表（如果已展开则收起）
- 点击列表中的选项：选中该选项，收起列表
- 点击菜单其他区域或按 ESC：收起所有下拉框

**视觉**：
- 头部：显示当前选中值 + 右侧 ▼ 箭头
- 展开后箭头变为 ▲
- 列表在头部下方展开，背景色与菜单背景一致
- 列表项高度约 18px

### 语言列表

**入站/出站语言下拉框选项**（10 个）：

| 代码 | 显示名称 |
|------|---------|
| zh | 中文 |
| en | English |
| ja | 日本語 |
| ko | 한국어 |
| zh-TW | 繁體中文 |
| ru | Русский |
| de | Deutsch |
| fr | Français |
| es | Español |
| pt | Português |

### 后端列表

**翻译后端下拉框选项**（4 个）：
- LLM API
- Tencent Cloud
- LibreTranslate
- FTAPI

### 后端未配置警告

当所选后端缺少必要配置时，在后端下拉框下方显示红色警告条：
- **Tencent Cloud**：未设置 SecretId/SecretKey
- **LibreTranslate**：未设置 API Key
- **LLM**：未设置任何提供商的 API Key
- **FTAPI**：无需配置（免费服务）

警告文本使用红色背景 `rgba(0.7, 0.3, 0.3, 0.6)`，字号为正常的 0.9 倍。

## 数据流与配置

### 新增配置变量

```cpp
MACRO_CONFIG_STR(QmTranslateOutgoingTarget, qm_translate_outgoing_target, 16, "en", CFGFLAG_CLIENT | CFGFLAG_SAVE, "出站翻译目标语言代码")
```

### 现有配置变更

- `QmTranslateTarget` 含义明确为"入站语言"（接收消息翻译成的语言）
- `QmTranslateOutgoingTarget` 为"出站语言"（发送消息时翻译成的语言）

### 翻译逻辑变更

- **入站翻译**（`CTranslate::StartTranslate`）：使用 `g_Config.m_QmTranslateTarget`
- **出站翻译**（`CTranslate::StartAutoOutgoingTranslate`）：使用 `g_Config.m_QmTranslateOutgoingTarget`

### 向后兼容

- 现有用户的 `QmTranslateTarget` 配置保持不变，自动成为"入站语言"
- 出站语言默认设为 `"en"`（English）

## 文件变更清单

1. `src/engine/shared/config_variables_qmclient_extra.h` — 新增 `QmTranslateOutgoingTarget`
2. `src/game/client/components/chat.cpp` — 重写 `PopupLanguageMenu`，改为下拉框式菜单
3. `src/game/client/components/chat.h` — 新增下拉框状态字段
4. `src/game/client/components/qmclient/translate.cpp` — 出站翻译使用 `QmTranslateOutgoingTarget`
5. `src/game/client/components/qmclient/translate.h` — 如有需要更新接口

## 交互方式

保持现有交互：
- **左键点击按钮**：打开/关闭菜单
- **右键点击按钮**：切换自动翻译开关

## 测试要点

1. 菜单正常弹出和关闭
2. 下拉框展开和收起正常
3. 入站/出站语言选择后正确保存到配置
4. 出站翻译实际使用 `QmTranslateOutgoingTarget`
5. 后端未配置警告正确显示
6. 颜色配置正确应用
