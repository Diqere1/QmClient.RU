---
doc_type: requirement
slug: rmlui-full-replacement
pitch: 让 RmlUI 成为可灰度启用的新 UI 管线，而不是抢占游戏画面的旁路渲染。
status: current
last_reviewed: 2026-05-09
implemented_by:
  - 2026-05-07-rmlui-runtime-shell
  - 2026-05-07-rmlui-resource-diagnostics
  - 2026-05-07-rmlui-render-command-bridge
  - 2026-05-07-rmlui-safe-mode
  - 2026-05-08-rmlui-scissor-texture-bridge
  - 2026-05-08-rmlui-layer-switchboard
  - 2026-05-08-rmlui-monitoring-hud-migration
  - 2026-05-07-rmlui-input-bridge
  - 2026-05-08-rmlui-popup-migration
tags: [rmlui, ui, rendering, migration]
related_prd: [rmlui-full-replacement]
---

# RmlUI 全面替代

## 用户故事

- 作为正在游戏中的玩家，我希望开启 RmlUI 调试 HUD 时游戏画面仍正常显示，而不是被灰屏或覆盖。
- 作为玩家，我希望 RmlUI 是一套我可以选择启用的现代化 UI，而不是强制替换现有 UI。
- 作为开发者，我希望 RmlUI 可以通过开关逐步替代旧 UI，而不是一次性破坏 CUI/QmUI 的稳定路径。
- 作为跨平台维护者，我希望新的 RmlUI 管线可以兼容 OpenGL、Vulkan 和 Android 渲染后端，而不是再次绑定到单一桌面后端。
- 作为调试 HUD 使用者，我希望 RmlUI 版本显示完整面板、文字和数据线，而不是只显示部分卡片或丢失图表。
- 作为维护者，我希望 RmlUI 出错时能自动回退旧实现，并留下开发环境可读的诊断信息。
- 作为界面重度用户，我希望主菜单、设置页、HUD、轮盘和可视化功能面板逐步迁到更现代的 UI，但地图本身的游戏渲染不被这一套 UI 接管。

## 为什么需要

当前 RmlUI 接入仍然像一个独立的 OpenGL 旁路渲染器：它直接获取图形上下文绘制，容易和 DDNet 的 command buffer 渲染顺序冲突。结果是进入游戏后开启调试 HUD 会挤占正常画面，图表区域尺寸异常，数据线消失，样式和字体警告也会干扰排查。更重要的是，这种路径天然难以迁移到 Vulkan 和 Android 后端，也偏离了 RmlUI 的真正职责。RmlUI 应该承载玩家可选的现代化菜单、设置、HUD 和功能面板，而不是介入地图本体渲染。继续局部修 HUD 只能缓解表象，不能让 RmlUI 可靠承担未来 UI 替代。

## 怎么解决

RmlUI 需要变成一条可灰度的新 UI 管线：旧 UI 完整保留，RmlUI 作为玩家可选的第二套现代化 UI，通过配置开关逐步启用。它主要覆盖主菜单、设置页面、HUD、轮盘和图形化功能面板等地图渲染之外的界面。RmlUI 的文档、布局、输入和资源由统一运行时管理，最终渲染进入和 CUI/QmUI 一致的客户端 UI 绘制顺序。渲染桥设计必须尽量后端无关，至少把 OpenGL、Vulkan 和 Android 作为明确目标平台考虑进去，而不是继续把 GL context 直绘当作长期核心。每个迁移模块都必须有旧实现 fallback，确保 RmlUI 出错时不影响游戏主画面。

## 边界

- 本需求不要求一次性迁移全部菜单和 HUD。
- 本需求不改变游戏物理、网络协议、地图行为或预测逻辑。
- 本需求不接管实际地图渲染、世界元素绘制或其他游戏内场景渲染。
- 本需求不移除旧 CUI/QmUI，不只是迁移期间保留，而是长期永久保留旧路径作为可选实现。
- 本需求不把所有 UI 样式一次性重做，只定义 RmlUI 替代所需的稳定管线。
- Vulkan 和 Android 支持可以分阶段落地，但设计时不能把 OpenGL 直绘作为唯一长期方案。
- HUD 编辑器、组件编辑器等创作型工具属于后续低优先级能力，不阻塞首批替代闭环。

## 成功是什么

- 开启 RmlUI HUD 或调试界面时，游戏主画面保持可见且可正常渲染，不再出现被整层覆盖或灰屏的情况。
- RmlUI 的职责边界稳定为“地图渲染之外的现代化可选 UI”，不会继续膨胀到接管实际地图渲染。
- 每个迁移到 RmlUI 的 UI 模块都可以单独开关，关闭后立即回到旧实现，不影响其他模块。
- 旧 UI 在 RmlUI 完善之后仍然可保留和可切回，而不是最终被删除。
- RmlUI 模块失败时，客户端优先回退旧 UI，而不是把失败暴露成玩家可见的坏画面。
- RmlUI 的运行时和渲染桥方案不再写死 OpenGL，后续接入 Vulkan 和 Android 时不需要重做整套 UI 生命周期与模块开关体系。
- 开发环境可以直接从现有诊断目录 `dumps/QmClient_Crash/` 拿到按模块和时间命名的诊断文件，快速判断是运行时、资源、样式还是渲染桥问题。
- 第一批迁移完成后，Monitoring HUD 至少要完整显示文字、数值卡片和折线图，作为后续迁移的稳定样板。

## 变更日志

- 2026-05-09：回写 `rmlui-popup-migration` 已验收，确认 `fullscreen_popup` 下的低风险提示型弹窗已成为首条已验收的交互式 modal surface；整体 requirement 继续保持 `current`，后续 menu-pilot 可直接复用 popup/input/safe-mode 基线。
- 2026-05-08：回写 `rmlui-monitoring-hud-migration` 已验收，确认 Monitoring HUD 已成为第一条 concrete RmlUI 迁移样板；整体 requirement 进入 `current`，因为基础设施与交互输入协议已经闭环。
- 2026-05-08：回写 runtime-shell、resource-diagnostics、render-command-bridge、safe-mode 与 scissor-texture-bridge 这五条已验收切片到 `implemented_by`；整体 requirement 进入 `current`，因为完整的 RmlUI 替代路线已具备可灰度基线。
- 2026-05-08：回写 `rmlui-layer-switchboard` 已验收的 host dispatch order / fallback ownership 基线到 `implemented_by`；整体 requirement 进入 `current`，因为 concrete menu/debug/popup migration 现在可以继续按已验收契约推进。
- 2026-05-08：回写 `rmlui-input-bridge` 已验收的输入投递、消费、cancel / release-state 和设置页开关收口基线到 `implemented_by`；整体 requirement 进入 `current`，因为后续菜单 / popup / 轮盘等交互式迁移已有稳定协议基线。
