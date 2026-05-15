---
doc_type: trick
type: pattern
status: active
slug: rmlui-module-fallback-pattern
created: 2026-05-07
tags: [rmlui, pattern, fallback, runtime, module-registry]
related_feature: 2026-05-07-rmlui-runtime-shell
related_roadmap: rmlui-full-replacement
---

# RmlUI 模块注册与 Fallback 可复用模式

## 一句话

在游戏客户端中引入可选 UI 替换（新 UI 管线 + 旧 UI 永远是正式路径）时，用 **模块描述符 → Frame Request → Frame Result → Host Fallback** 四段协议把新 UI 与旧 UI 解耦，让任何 surface 都可以独立开关、独立失败、独立回退。

## 适用场景

- 需要逐步将旧 UI 系统迁移到新 UI 管线，而不是一次全量替换。
- 新 UI 管线有平台限制（如仅 OpenGL、未覆盖 Vulkan/Android），必须保留旧 UI 作为所有平台的正式路径。
- 新 UI 的某些 module 可能失败（缺少资源、context 不可用、渲染失败），失败时不能影响其他 module 或其他 UI 层。

## 四段协议

### 1. 模块描述符（SRmlUiModuleDescriptor）

每个 RmlUI surface 用一份稳定的描述符声明自己的身份和约束。

必需字段（来自 [rmlui-runtime-api-reference.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/reference/rmlui-runtime-api-reference.md) runtime-shell contract 节）：

| 字段 | 含义 | 示例（Monitoring HUD） |
|---|---|---|
| `m_pModuleName` | 稳定诊断/模块名 | `"monitoring_hud"` |
| `m_pConfigToggle` | qm_ 前缀的配置开关 | `"qm_rmlui_monitoring_hud"` |
| `m_Layer` | UI 层枚举（GAME_HUD / DEBUG_OVERLAY / MENU_PAGE 等） | `GAME_HUD` |
| `m_pDocumentPath` | RML 文档路径 | `"monitoring_hud.rml"` |
| `m_RequiresInput` | 是否需要输入路由 | `false` |
| `m_HasLegacyFallback` | 旧 UI 路径是否存在 | `true` |
| `m_pFallbackOwner` | 负责执行 fallback 的宿主函数 | `"CGameClient::RenderQmMonitoringHud"` |

约束：
- 描述符是只读声明。运行时不应该在运行中修改它。
- 重复注册同一个 `m_pModuleName` 不能产生两个有效模块。
- 描述符不包含实现代码。实现由 runtime 通过 frame request 调用。

### 2. Frame Request（SRmlUiFrameRequest）

每帧由宿主向 runtime 发出的请求。

必需字段：
- `m_Layer` — 请求渲染的 UI 层
- `m_ViewportWidth / m_ViewportHeight` — 视口尺寸
- `m_FrameTimeSec` — 帧时间
- `m_DebugDiagnostics` — 是否开启诊断

规则：
- Request 只携带渲染参数，不包含 GL context 或后端相关状态。
- Runtime 必须能从 request 中提取足够的 viewport 信息来初始化/更新 RmlUI context。

### 3. Frame Result（SRmlUiFrameResult）

Runtime 处理完一帧后返回给宿主的结果。

必需值（`ERmlUiFrameResult`）：
- `RENDERED` — 该层渲染成功
- `SKIPPED_DISABLED` — 全局开关或模块开关关闭
- `SKIPPED_UNAVAILABLE` — runtime 可用但没有能处理该层的模块
- `FALLBACK_REQUIRED` — 启用了但失败了，宿主必须走旧 UI

规则：
- Runtime 不绘制旧 UI。旧 UI 由 `m_pFallbackOwner` 执行。
- Failure reason 必须区分 runtime/backend 失败和 surface/document 失败。
- Host 根据 result 决定是否调用旧 UI fallback，不做额外判断。

### 4. Host Fallback

宿主（如 `CGameClient::RenderQmMonitoringHud`）的职责：
1. 构建 frame request 并调用 runtime。
2. 读取 frame result。
3. 如果 result 是 `FALLBACK_REQUIRED` 或 `SKIPPED_DISABLED`，调用旧 UI 路径。
4. 如果 result 是 `RENDERED`，不执行旧 UI（避免双重绘制）。

## 关键边界

**旧 UI 不是临时脚手架。** 它是该模块在所有平台上的正式路径。RmlUI 是可选增强。这个边界意味着：
- 旧 UI 代码不删除、不降级、不标记为 deprecated。
- 旧 UI 可以独立接收功能更新。
- 迁移到 RmlUI 是"新增一种渲染方式"，不是"替换旧渲染方式"。

## 与 DDNet 现有模式的关系

DDNet 已有大量类似的双路径设计（如 Vulkan/OpenGL 渲染后端切换、多 GPU vendor 路径）。这个模式在精神上和 DDNet 的 `GRAPHICS_DISPLAY_BACKEND` 是一致的：多实现并行，运行时选择，失败不传播。

区别是 RmlUI 的模式把 "选择" 和 "失败回退" 的粒度从 "整个渲染后端" 细化到了 "单个 UI surface"。
