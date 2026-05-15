# Event Summary Schema

## 目标

本文件定义 QmClient 当前推荐的事件字段和摘要模板，用来把 UI / backend / lifecycle 相关行为沉淀成稳定、可归因的事件。

它不是完整遥测系统设计，而是当前仓库可直接复用的最小事件约定。

## 适用场景

- RmlUI 页面切换
- backend/context acquire
- safe mode、fallback、skip、defer
- settings host 激活/退出
- 启动阶段关键状态切换

## 事件字段

每条事件建议至少包含：

- `event`
  - 事件名，使用稳定短词，例如 `rmlui_settings_enter`
- `phase`
  - 所处阶段，例如 `startup`、`enter`、`render`、`shutdown`
- `target`
  - 目标对象，例如 `settings_page`、`backend_context`
- `result`
  - `success` / `skipped` / `fallback` / `failed`
- `reason`
  - 稳定原因码；成功也允许填 `ok`
- `thread`
  - 可选；涉及 backend / render thread 时建议填写
- `state_before`
  - 可选；进入前状态
- `state_after`
  - 可选；退出后状态
- `details`
  - 可选；补充少量键值，不要塞大段自由文本

## 字段约束

- `event`、`phase`、`target`、`result`、`reason` 尽量固定词汇，不要会话内随意改名
- `reason` 与 `.codestable/reference/log-classification.md` 保持一致
- `details` 只放解释性上下文，不替代主字段
- 如果事件跨线程或跨生命周期边界，优先补 `thread`、`state_before`、`state_after`

## 推荐事件命名

### UI / 页面

- `rmlui_settings_enter`
- `rmlui_settings_leave`
- `menu_page_switch`
- `settings_host_activate`
- `settings_host_deactivate`

### Backend / Context

- `backend_context_acquire`
- `backend_context_release`
- `rmlui_runtime_init`
- `rmlui_runtime_shutdown`

### Fallback / 安全模式

- `rmlui_render_fallback`
- `safe_mode_enter`
- `safe_mode_leave`
- `serverbrowser_empty_state_resolve`

## 推荐 reason

- `ok`
- `toggle_disabled`
- `state_not_ready`
- `runtime_unavailable`
- `backend_unavailable`
- `context_missing`
- `thread_mismatch`
- `resource_missing`
- `validation_failed`
- `user_cancelled`

## 单条事件模板

```json
{
  "event": "rmlui_settings_enter",
  "phase": "enter",
  "target": "settings_page",
  "result": "success",
  "reason": "ok",
  "state_before": "legacy_settings",
  "state_after": "rmlui_settings",
  "details": {
    "source": "menu_toggle"
  }
}
```

## 事件摘要模板

适合写进调试记录、issue 文档或发布素材：

```md
## 事件摘要

- 事件：`rmlui_settings_enter`
- 阶段：`enter`
- 目标：`settings_page`
- 结果：`success`
- 原因：`ok`
- 关键状态变化：`legacy_settings -> rmlui_settings`
- 证据来源：`startup_trace` / 结构化日志 / dump 前最后事件
```

## 最小事件集建议

如果当前模块还没有任何事件沉淀，优先补这几类：

1. 页面进入 / 退出
2. runtime init / shutdown
3. backend/context acquire 结果
4. fallback / skip / defer 行为
5. safe mode 触发与退出

## 当前已接线的最小事件

当前仓库已经开始落地的事件点：

- 统一发射辅助：
  - 文件：`src/engine/client/rmlui_event_log.h`
  - 约束：优先复用 `LogRmlUiStructuredEvent(...)`，不要在各调用点继续散落手写 `log_info("rmlui_event", ...)`

- `settings_host_activate`
  - 文件：`src/game/client/components/menus_settings.cpp`
- `settings_host_deactivate`
  - 文件：`src/game/client/components/menus_settings.cpp`
- `rmlui_settings_enter`
  - 文件：`src/game/client/components/menus_settings.cpp`
- `rmlui_settings_leave`
  - 文件：`src/game/client/components/menus_settings.cpp`
- `backend_context_acquire`
  - 文件：`src/engine/client/rmlui_backend.cpp`
- `rmlui_render_fallback`
  - 文件：`src/game/client/gameclient.cpp`

这批事件的定位不是“完整遥测系统”，而是先把 settings host、backend acquire 和 fallback 三条最脆弱链路变成可归因的稳定日志。

## 与其他文档的边界

- `.codestable/reference/workflow-entry.md`
  - 放导航
- `.codestable/reference/log-classification.md`
  - 放日志一级分类和 reason 口径
- 本文件
  - 放事件字段和摘要模板
