---
doc_type: roadmap_prd
slug: rmlui-full-replacement
status: draft
created: 2026-05-06
last_reviewed: 2026-05-07
tags: [rmlui, ui, rendering, migration, prd]
related_requirements: [rmlui-full-replacement]
related_roadmap: [rmlui-full-replacement]
---

# RmlUI 全面替代 PRD

This document is the original product brief for the RmlUI full-replacement effort. The requirements and roadmap docs refine it into executable planning artifacts.

## 1. 问题定义

当前 RmlUI 已经能在部分场景里初始化并加载文档，但还不是一条可以替代现有 UI 的稳定管线。已观测到的真实问题包括：

- 进入游戏后开启 RmlUI HUD，会覆盖或挤占游戏正常渲染，而不是像现有 CUI / QmUI 一样并行叠加。
- Monitoring HUD 只显示部分卡片，图表区域会出现 `w=0 h=0` 的无效矩形，数据线无法正常绘制。
- RmlUI 资源链路仍不稳定，存在字体缺失、样式 shorthand 不兼容、诊断输出不够结构化等问题。
- 当前调试成功只证明“某些帧能画出来”，没有证明“失败时可回退、模块间可开关、对主画面无副作用”。

这说明现在的问题不是单个 HUD 样式 bug，而是 RmlUI 接入方式还停留在实验性旁路阶段。现有 Monitoring HUD 代码只能算试点实现，不能默认当成一条稳定可复用的正式路径。

RmlUI 在这个项目里的产品定位也需要收紧：它不是新的地图渲染层，而是玩家可选的现代化 UI 套件，主要负责主菜单、设置页面、HUD、轮盘、Click GUI 和其他非地图渲染界面。

## 2. 产品目标

我们要把 RmlUI 建成一条可灰度启用的新 UI 管线，逐步承接 Monitoring HUD、调试 HUD、弹窗、主菜单、设置页面和其他非地图渲染界面能力，同时保留旧 CUI / QmUI 路径作为稳定 fallback。

目标不是“先让 RmlUI 全部显示出来”，而是：

- 让 RmlUI 在客户端 UI 渲染顺序里成为受控的一层。
- 让每个迁移模块都能独立开关、独立验收、独立回退。
- 让开发环境排查路径固定，不再依赖手工复制日志和临时猜测。
- 让渲染桥和运行时设计保持后端无关，能够覆盖 OpenGL、Vulkan 和 Android 渲染环境。
- 让主菜单、设置页面、HUD 和功能面板能逐步现代化，但不侵入实际地图渲染。

## 3. 非目标

- 不在这一轮删除旧 UI。
- 不把 RmlUI 设计成未来必然完全取代旧 UI 的唯一路径。
- 不追求一次性把所有界面迁到 RmlUI。
- 不在本轮重做视觉设计语言。
- 不把 OpenGL 直绘保留为长期主路径。
- 不借这次迁移去改协议、物理、预测、地图行为或其他高风险核心逻辑。
- 不在第一阶段承诺所有后端一次性同等完成，但架构必须从第一天就为多后端留出一致接口。
- 不让 RmlUI 接管地图本体、世界元素或地图内真实渲染内容。

## 4. 用户与使用场景

### 玩家

- 在游戏中打开 HUD 或调试界面时，主画面必须继续正常渲染。
- RmlUI 模块出错时，不应看到半屏黑块、整层覆盖或空白图表。
- 可以把 RmlUI 当成一套额外可选的现代化 UI，而不是只能接受一次性替换。

### 开发者

- 可以按模块启用 RmlUI，观察真实效果，而不是只能一把梭替换。
- 可以直接查看现有 `dumps/QmClient_Crash/` 目录下的结构化诊断文件定位问题。

### 维护者

- 可以知道某个 RmlUI 模块目前处于实验、试点还是稳定状态。
- 可以在不影响旧路径的前提下继续推进后续迁移。
- 可以长期同时维护旧 UI 和 RmlUI，而不是被计划强迫收敛到单一路径。

## 4.1 目标界面范围

本轮 RmlUI 的主要覆盖面：

- 主菜单
- 设置页面
- HUD 与调试 HUD
- 弹窗、轮盘、Click GUI
- 其他地图渲染之外的辅助界面

明确排除：

- 实际地图渲染
- 世界对象、地图层、场景内容绘制
- 任何会改变游戏主渲染职责边界的做法

## 4.2 体验方向

RmlUI 的视觉方向参考 `ddnet-rs` 的设计讨论 issue 156，重点参考其中的面板层级、圆角、间距、字体层级、半透明背景和按钮体系，而不是照搬某个具体页面。

参考链接：

- [ddnet-rs issue #156](https://github.com/ddnet/ddnet-rs/issues/156)

## 5. 成功指标

### 体验指标

- 游戏中启用 RmlUI Monitoring HUD 时，主画面保持可见且可持续渲染。
- Monitoring HUD 的文字、数值卡片、图表线条和布局完整显示。
- 菜单、弹窗等后续迁移模块启用时，不出现错误消费 gameplay 输入的问题。
- 在不改动 UI 生命周期协议的前提下，render bridge 可以分后端落地，而不是每个后端各写一套独立 RmlUI 接入方案。
- 设置页面可以按功能区块重新整合，减少旧页面割裂感。
- 设置页面支持搜索或快速定位，优先提升“找得到”的体验，而不是只换视觉容器。
- Click GUI、Bind 轮盘和表情轮盘可以作为同一类现代交互入口逐步落地。
- 轮盘类入口应优先收敛成统一的交互系统，而不是分别做多套实现。

### 工程指标

- 所有 RmlUI 模块都通过统一注册表声明 `layer`、`toggle`、`document` 和 `fallback`。
- RmlUI 运行失败时返回明确失败原因，并触发旧 UI 回退，而不是静默失败或直接占住渲染。
- 仅开发模式自动导出诊断文件，路径固定为现有保存目录下的 `dumps/QmClient_Crash/`，且不能只绑定 `CONF_DEBUG`。
- 后续 feature-design 不需要重新发明运行时和开关协议，直接沿用 roadmap 契约。
- 渲染桥契约对上层 UI 模块保持一致，不因为 OpenGL、Vulkan 或 Android 切换而改变模块注册和调用方式。

### 发布指标

- 默认配置下旧 UI 仍是主路径。
- 至少支持“全局总开关 + 模块级开关”两层控制。
- 任意单个试点模块关闭后，不影响其他模块和旧 UI。
- 即使 RmlUI 成熟，旧 UI 仍保留为正式可用路径。

## 6. 关键约束

- 必须遵守 DDNet/QmClient 现有渲染和输入边界，不允许 RmlUI 自己抢 GL context 成为长期设计。
- 必须把 OpenGL、Vulkan 和 Android 视为目标渲染环境，禁止把某个桌面 GL 快捷路径写成唯一正式实现。
- 必须从构建目录运行客户端，诊断导出相对运行目录生效。
- 自动导出日志只在开发环境启用，避免污染正式发行行为。
- 旧 UI 路径必须始终可回退，直到某个模块完成验收并明确转正。
- 即使某个模块完成验收并转正，也不能把“删除旧 UI”作为默认后续动作。

## 7. 开关与灰度策略

### 总开关

- `qm_rmlui_enable`
- 作用：统一控制 RmlUI runtime 是否允许启动。
- 默认值：`0`

### 全局护栏

- `qm_rmlui_safe_mode`
- 作用：控制 RmlUI 异常时是否执行统一安全回退。
- 默认值：`1`

`qm_rmlui_safe_mode` 是全局策略开关，不属于模块迁移开关，也不受“模块开关只有在总开关开启时才生效”这条规则约束。

### 模块开关

- `qm_rmlui_monitoring_hud`
- `qm_rmlui_debug_hud`
- `qm_rmlui_popup`
- `qm_rmlui_menu_pilot`
- `qm_rmlui_click_gui`
- `qm_rmlui_settings_reorg`
- `qm_rmlui_settings_search`
- `qm_rmlui_radial_action_system`
- `qm_rmlui_hud_editor`
- `qm_rmlui_dev_inspector`
- `qm_rmlui_presets`

模块开关只有在总开关开启时才生效。任一模块开关关闭时，直接走旧实现。

### 失败回退策略

- runtime 初始化失败：整条 RmlUI 路径不可用，全部回退旧 UI。
- 模块文档或资源加载失败：仅该模块回退旧实现。
- 渲染桥提交失败：该帧回退旧实现，并记录模块级诊断。
- 输入桥异常：
  交互型 surface 必须执行 close/cancel/release-state，并立即回退旧 UI。
  非交互 overlay 才允许退化为禁用输入消费并保留旧路径。

### 玩家可见切换行为

- 总开关关闭：玩家继续看到完整旧 UI，不应感知到“半启用”的 RmlUI 残留。
- 模块开关开启但模块失败：只回退该模块，不扩大成整套 UI 一起失败，除非 runtime 级别不可用。
- 安全模式触发：玩家应该被明确带回旧路径，而不是留在一个还能看见但已经失去交互能力的坏页面。
- 设置页、菜单、轮盘、编辑器类 surface 的回退动作都应保持和旧路径相同的退出预期，例如返回、关闭、取消，而不是突然无响应。

## 8. 阶段规划与门禁

### Phase A：运行时壳与诊断基线

目标：

- 建立 runtime、模块注册、总开关与模块开关。
- 固定开发环境诊断导出格式。
- 实现过程采用 TDD，先建立失败测试，再补最小实现。

进入下一阶段前必须满足：

- 可以注册 Monitoring HUD 模块，并通过失败测试、成功实现、回退验证三步闭环。
- 模块启用、禁用、失败回退三种路径都可观测。
- `dumps/QmClient_Crash/` 导出包含模块名、阶段名、失败原因和运行时间。

### Phase B：渲染桥接入

目标：

- 让 RmlUI 绘制进入正常客户端渲染顺序，而不是长期依赖 direct GL path。
- 固定一套后端无关的上层桥接契约，为 OpenGL、Vulkan 和 Android 后端分别落地做准备。

进入下一阶段前必须满足：

- 渲染桥不再调用 `SDL_GL_MakeCurrent`。
- 游戏主画面与 RmlUI 叠加时可同时可见。
- scissor、texture、blend 状态不会污染主画面。
- OpenGL 之外的后端接入不需要改写 runtime、module registry 和 layer manager 协议。

### Phase C：Monitoring HUD 迁移

目标：

- 用 Monitoring HUD 验证真正的端到端迁移闭环，但不把当前试点实现当成已稳定的参考实现。

进入下一阶段前必须满足：

- 卡片、文本、图表完整显示。
- HUD 不消费 gameplay 输入。
- 关闭模块开关后立即回到旧 HUD。

### Phase D：安全护栏与交互型 UI 试点

目标：

- 先补上安全模式，再迁移弹窗和单个菜单页面，验证输入桥和层级切换。

进入下一阶段前必须满足：

- 某个模块异常时，用户可被安全带回旧 UI。
- 菜单和弹窗可以消费 UI 输入。
- 控制台激活时 RmlUI 不抢文本输入。
- 任一试点关闭时仍能返回旧菜单或旧弹窗。

### Phase E：体验整合与编辑工具

目标：

- 在替代闭环稳定后，逐步处理设置页重组、Click GUI、轮盘和可视化编辑器这类增强能力。

进入下一阶段前必须满足：

- 设置页重组不破坏现有配置语义。
- Click GUI 和轮盘类入口遵守统一输入消费与 fallback 规则。
- HUD 编辑器和组件编辑器明确是低优先级增强，不阻塞核心替代闭环。

### Phase F：开发效率与长期配置增强

目标：

- 为 RmlUI 长期演进补上开发者检查器和预设能力，降低后续迁移成本。

进入下一阶段前必须满足：

- 开发者可以快速定位样式、布局、节点和资源问题。
- 预设能力不破坏原有设置和旧 UI 兼容性。

## 9. 当前已知风险

- 当前 direct GL path 虽然已经能降低部分 context 冲突，但仍不足以作为全面替代方案。
- 如果 render bridge 继续绑定 OpenGL 语义，上层模块一旦扩展到 Vulkan 或 Android 会被迫重复迁移。
- RmlUI CSS 与现有样式写法存在兼容差异，需要纳入资源诊断而不是逐处撞日志。
- 字体资源链路不完整会导致布局测量失真，进一步放大图表区域尺寸问题。
- 如果没有统一 layer manager，后续每个迁移模块都可能重复发明开关和 fallback 逻辑。
- 如果输入桥只转发离散 key event，而没有 cursor move、release、focus/cancel 生命周期，菜单、轮盘和 HUD 编辑器迁移都会卡在交互边界不稳定上。
- 如果没有提前锁死“永久保留旧 UI”这个原则，后续 feature 很容易把 RmlUI 误做成强制替换路线。
- 聊天命令类入口当前还没有明确代码宿主，本 PRD 先不把它列为已拍板落地项，后续需要独立 explore 后再决定是否并入 Click GUI 套件。

## 10. 验收口径

这个 PRD 的验收不是“某个界面看起来差不多了”，而是满足下面三条：

- 架构上：RmlUI 已经从旁路实验路径，收敛成可控制、可回退、可诊断的 UI 管线。
- 迁移上：至少一个真实模块完成端到端迁移并稳定回退，且试点宿主的现有问题已被重新收敛或显式记录。
- 流程上：后续任何 RmlUI feature 都可以基于现有开关、契约、日志和阶段门禁继续推进。

## 10.1 实施配套文档

当前 roadmap 的实施伴随说明位于：

- [.codestable/roadmap/rmlui-full-replacement/drafts/rmlui-full-replacement-landing-notes.md](C:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/roadmap/rmlui-full-replacement/drafts/rmlui-full-replacement-landing-notes.md)

它不替代 roadmap/PRD，而是给后续 `cs-feat-design` 提供更细的宿主接缝、输入/后端边界和验收建议。

## 11. 里程碑出口物

- `rmlui-runtime-shell`：证明这不是一次性替换，而是受控灰度。
- `rmlui-render-command-bridge`：证明 RmlUI 可以不抢上下文进入正式渲染顺序。
- `rmlui-monitoring-hud-migration`：证明第一条真实模块迁移闭环成立。
- `rmlui-input-bridge` + `rmlui-menu-pilot`：证明交互型 UI 也能迁移，而不是只会画静态 HUD。
- `rmlui-settings-reorg` + `rmlui-click-gui-suite`：证明 RmlUI 不只是复刻旧页，还能承载现代化交互组织。
- `rmlui-settings-search` + `rmlui-radial-action-system`：证明现代化体验来自信息架构和统一交互，不只是换皮。
- `rmlui-safe-mode` + `rmlui-dev-inspector`：证明 RmlUI 能长期演进，而不是一套脆弱的实验 UI。
