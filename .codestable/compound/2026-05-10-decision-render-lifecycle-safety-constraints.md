---
doc_type: decision
category: constraint
status: active
slug: render-lifecycle-safety-constraints
created: 2026-05-10
tags: [rendering, lifecycle, state-management, diagnostics, testing, constraint]
related_docs:
  - AGENTS.md
  - .codestable/attention.md
---

# 渲染链与生命周期高风险代码长期约束

## 概述

本决策把图像渲染逻辑、backend context、线程边界、生命周期状态机、fallback 和 release-only 崩溃排查中的长期工程约束固化下来。

这些约束的目标不是“写得更优雅”，而是减少两类高成本问题：

1. 人或 AI 在高状态复杂度代码中凭感觉补逻辑，导致引入新的上下文/生命周期 bug。
2. 问题发生后无法快速回答“错在状态、线程、资源、调用顺序还是 fallback 设计”。

`AGENTS.md` 中的 `RL-01` 到 `RL-10` 是本决策的执行摘要。遇到措辞冲突时，以本决策文档为详细版、以 `AGENTS.md` 为启动时的硬约束摘要版。

## 背景

QmClient 当前同时存在以下高风险表面：

- RmlUI / legacy UI 双路径并存。
- OpenGL / Vulkan / SDL backend context 约束严格，错误常表现为 release-only 崩溃或 fallback。
- 启动期、切页期、render callback 内存在明显生命周期窗口，`build-debug` 与 `build-ninja` 行为可能不同。
- 许多入口是“看起来像只读查询”的 API，但会在菜单绘制、组件初始化或 diagnostics 导出时被提前触发。

最近一轮实际问题已经证明：只读查询接口在 `OnInit()` 前被调用时，如果直接解引用未初始化依赖，会造成 release 启动期崩溃，而且非常难靠肉眼从大函数里直接猜中。

## 决策结论

## 执行协议

每次涉及渲染链、生命周期、状态机、fallback、backend context、启动期只读查询的实现、修复或 review，必须先按下面顺序过一遍：

1. 先判断本次改动主要落在 `RL-01/02`、`RL-03/04`、还是 `RL-05~10` 哪一组。
2. 至少明确写出一个“当前状态/线程/owner/资源是否 ready”的事实，而不是直接猜修复点。
3. 若代码命中本决策中的“禁止事项”，必须二选一：
   - 直接修掉；
   - 或在评审/文档里明确标记为“历史例外”，说明为什么暂时保留。
4. 修完后必须补与根因对应的最小测试或最小证据链；二者至少有一个，最好两者都有。

## 当前允许的历史例外

以下例外是当前仓库在过渡期内允许存在、但不得扩散的存量边界。它们不是推荐模式，也不能作为新代码模板。

### EX-01 backend 内部上下文探测允许保留，业务层禁止扩散

`src/engine/client/rmlui_backend.cpp` 中基于 `SDL_GL_GetCurrentContext()` 的探测，当前允许保留为 backend 内部的可用性检查。

原因：

- 它位于 backend 封装层内部，用于回答“当前是否已经有活动 OpenGL context”。
- 它本身不负责抢占 context，也不应被业务层复制扩散。

约束：

- 新代码禁止在 game/client/components、RmlUI module、menus、surface callback 等业务层再次直接调用 `SDL_GL_GetCurrentContext()`。
- 如果后续 backend 层能被更稳定的 owner/context bridge 替代，应优先消除此例外。

### EX-02 Acquire/Release backend frame context 仅允许出现在集中封装入口

`AcquireBackendFrameContext()` / `ReleaseBackendFrameContext()` 当前仅允许出现在少数集中封装入口，例如 `CScopedRmlUiBackendFrameContext` 这类明确 owner 的 scoped helper 中。

原因：

- 当前仓库还处在 RmlUI 过渡期，需要少量封装点承接 legacy backend/context 约束。

约束：

- 新代码禁止在业务 surface、模块回调或零散工具函数里直接展开 Acquire/Release 成对调用。
- 若发现新的调用点，默认先尝试并入现有 scoped helper 或 backend-thread bridge。

### RL-01 状态显式化

涉及渲染链、UI runtime、backend context、模块启停、资源装载/释放的模块，必须先明确状态集合和状态转移。

最低要求：

- 必须能明确区分 `未初始化`、`已初始化但未可用`、`可运行`、`销毁中`、`已销毁` 这类状态。
- 必须说明哪些入口允许在每个状态被调用。
- 非法状态转移必须通过断言、稳定失败返回或显式 fallback 暴露出来。

禁止事项：

- 禁止把关键状态只隐含在多个布尔字段、空指针、调用顺序和“默认大家都知道”的约定里。
- 禁止把“第一次调用顺序刚好正确”当成状态设计。

### RL-02 只读查询无害化

命名为 `Is*`、`Has*`、`Get*`、`Num*` 的只读查询接口，默认必须在生命周期早期、空资源态和失败态下安全调用。

期望语义：

- `Is*` / `Has*` 在依赖未就绪时返回 `false`。
- `Num*` 在依赖未就绪时返回 `0`。
- `Get*` 在依赖未就绪或索引非法时返回 `nullptr` / 空值。

禁止事项：

- 禁止让只读查询直接解引用尚未初始化的 backend、http、runtime、context、surface 或 renderer 对象。
- 禁止把“调用者应该知道现在不能问”当成接口契约，除非函数名本身就是驱动语义而非查询语义。

### RL-03 驱动接口前置条件

命名为 `Init*`、`Refresh*`、`Start*`、`Render*`、`Destroy*`、`Shutdown*` 的驱动接口，必须写明前置条件、线程归属和可重入性。

最低要求：

- 必须清楚说明它依赖哪些对象已经 ready。
- 必须清楚说明它只能在哪个线程或哪个 render owner 上执行。
- 必须清楚说明重复调用、失败重试、部分初始化后再进入的语义。

允许的失败表达：

- 调试断言
- 明确错误返回
- 稳定 fallback / skip / demote

禁止事项：

- 禁止把驱动接口做成“好像什么时候调都行，但内部靠运气绕过去”。

### RL-04 线程与上下文单一入口

OpenGL / Vulkan / SDL backend context、render command、surface acquire 这类访问必须通过统一桥接层或明确 owner 入口。

推荐模式：

- backend thread bridge
- render-command bridge
- scoped frame context
- 明确 owner 的 helper API

禁止事项：

- 禁止业务模块直接抢 context。
- 禁止为了“先跑起来”在多个 UI surface、模块 callback、工具函数里散落 `MakeCurrent` / `AcquireContext` 逻辑。
- 禁止跨线程触碰 renderer 全局状态，除非该路径本身就是被定义好的 backend thread 入口。

### RL-05 帧边界成对出现

Acquire / Begin / Render / Submit / Present / Release 这类帧级操作必须能在代码结构和日志中成对对应。

最低要求：

- 必须能看出一帧从哪里开始、在哪里提交、在哪里释放。
- 必须能看出哪个阶段负责错误恢复或中断退出。
- 必须能定位“进入了哪一步但没出来”。

禁止事项：

- 禁止把资源初始化、帧提交、状态回滚、错误恢复混在一个匿名巨型分支里。
- 禁止出现“Acquire 在 A，Release 靠 B 的顺手逻辑兜底”这种跨区域隐式配对。

### RL-06 状态恢复可验证

任何会修改 GL/Vulkan/renderer 全局状态的代码，都必须明确谁负责恢复，并且恢复责任可验证。

优先做法：

- scoped guard
- 统一 restore helper
- diagnostics 快照导出
- 进入前后状态采样

禁止事项：

- 禁止假设“后面的代码自然会覆盖前面的状态”。
- 禁止依赖下一个 surface 或下一个模块帮当前模块擦屁股。

### RL-07 生命周期日志边界化

排查启动期、切页期、fallback 或 release-only 崩溃时，必须在组件边界记录结构化阶段日志。

最低要求：

- 进入组件
- 关键分支前后
- 成功退出
- 失败原因或最后已知阶段

日志字段应尽量稳定，例如：

- 当前状态 / page / popup / layer / current type
- 线程归属 / owner
- fallback reason
- resource availability

禁止事项：

- 禁止只打“走到这里了”“应该不是这里”的自由文本日志。
- 禁止完全依赖 offset 猜测而不补组件边界证据。

### RL-08 Fallback 可归因

所有 fallback、skip、defer、demote 行为都必须带稳定的 reason 字段，或有等价的诊断输出。

推荐 reason 分类包括：

- toggle disabled
- state not ready
- backend unavailable
- context unavailable
- resource missing
- thread mismatch
- runtime unavailable
- module render failure

禁止事项：

- 禁止仅返回布尔值表示“失败了”。
- 禁止让 fallback 行为既不崩也不报原因，导致上层只能猜。

### RL-09 测试优先锁脆弱语义

修复渲染链、生命周期、状态机 bug 时，测试必须优先覆盖脆弱语义，而不是只测 happy path。

优先覆盖列表：

- 未初始化状态是否安全
- 空资源态是否安全
- 重复调用是否幂等或有明确定义
- 失败后是否污染后续状态
- fallback / skip reason 是否正确
- release-only 问题能否用更小的生命周期单测锁住根因

禁止事项：

- 禁止只补“正常路径能跑”的测试就宣布问题被封住。

### RL-10 发布态证据闭环

凡是只在 `build-ninja` / `Release` / `RelWithDebInfo` 出现的问题，修复后必须至少做一次对应构建的真实运行验证。

最低要求：

- 重新运行对应构建产物。
- 核对最新 `debug-artifacts`、dump、WER、关键结构化日志。
- 确认崩点是否消失、是否后移、是否变成新问题。

禁止事项：

- 禁止只靠 `build-debug` 通过就宣布 release 问题结束。
- 禁止把“没有立刻崩”当作唯一证据而不核对日志链路是否真的穿过原始崩点。

## 为什么这样定

这套约束优先解决的不是代码美观，而是工程上的三个核心成本：

1. 降低高复杂状态代码中的误判率。
2. 缩短 release-only 问题的定位时间。
3. 给人和 AI 一个统一的硬边界，避免每次都重新发明调试和防守方式。

如果不固化这些约束，最常见的退化模式就是：

- 又回到“猜一行改一行”的 patch 式调试。
- 只在 debug 语境下自证正确。
- 让 context / thread / state ownership 继续分散在各模块里。

## 影响

后续所有涉及下列表面的 design、实现、issue-fix、review 都受本决策约束：

- RmlUI runtime / module / popup / menu pilot / monitoring HUD
- graphics backend / SDL backend / render-command bridge
- serverbrowser、http、startup trace、diagnostics 这类启动期会被菜单或渲染提前触发的查询接口
- 任何新增的 fallback / safe mode / demotion 逻辑

直接影响如下：

- `AGENTS.md` 只保留摘要，详细解释统一回链本决策。
- 后续若要新增例外，必须显式说明是对哪一条 `RL-*` 破例，以及为什么值得破例。
- 代码审查时应把这些条目当作硬性检查项，而不是建议项。

## 相关文档

- `AGENTS.md`
- `.codestable/attention.md`
- `.codestable/compound/2026-05-09-learning-windows-debug-symbol-workflow.md`
- `.codestable/compound/2026-05-07-trick-rmlui-module-fallback-pattern.md`
