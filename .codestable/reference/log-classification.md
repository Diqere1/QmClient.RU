# Log Classification

## 目标

本文件定义 QmClient 仓库当前推荐的日志分类口径，用来回答三类问题：

1. 这次失败是环境问题、仓库基线债务，还是当前改动造成的。
2. 这条日志应该归到 gate、调试、运行期事件中的哪一层。
3. fallback、skip、defer 这类“没走主路径”的行为，应该如何稳定表达原因。

它是工作流参考文档，不替代 `AGENTS.md` 中的红线约束。

## 适用场景

- 读 `qmclient_scripts/check-gate.ps1` 汇总报告
- 设计或审查结构化调试日志
- 给 RmlUI / backend / lifecycle 相关代码补 `reason`
- 写 issue report、fix-note、发布说明时判断日志证据属于哪类

## 一级分类

### 1. 环境 / 工具

定义：

- 问题主要来自本机环境、外部工具、依赖缺失或执行前提不满足
- 即使不改仓库代码，补齐环境后也可能恢复

典型例子：

- `python` / `py` / `clang-format` / `clang-tidy` 缺失
- VS、CMake、Ninja、Rust toolchain 不可用
- PowerShell 执行策略、路径、权限或编码问题
- 子模块未初始化、外部下载失败、网络不可达

处理建议：

- 在 gate 汇总中单独归类，不要混进当前改动失败
- 给出缺的工具、版本或路径
- 能自动检测时，优先输出“缺什么”和“如何确认”

### 2. 仓库基线债务

定义：

- 当前分支或基线本来就存在的问题，与本次改动没有直接因果关系
- 当前提交者不一定要在本任务里清掉，但不能误报成“这次改坏了”

典型例子：

- 历史遗留格式问题
- 既有 header guard / 配置变量命名债务
- 本次未触碰模块里的旧 warning
- 与当前改动文件无关、但 gate 能扫到的存量问题

处理建议：

- 在汇总中保留具体失败项
- 明确说明它属于基线，不要用“已通过”掩盖
- 若需要继续推进，另开 refactor / debt 清理任务

### 3. 当前改动 / 构建阻断

定义：

- 本次改动直接引入或暴露的问题
- 会阻断当前功能合入、提测或继续验证

典型例子：

- 新代码编译失败
- 新增测试失败
- 当前改动引入生命周期断言、崩溃或回归
- 作用域内格式、静态检查、接口契约被破坏

处理建议：

- 默认按硬失败处理
- 报告里要能定位到文件、步骤和最后证据
- 修复后至少重跑受影响检查

## 二级分类

### Gate 日志

来源：

- `qmclient_scripts/check-gate.ps1`
- `qmclient_scripts/strict-debug-check.ps1`
- `scripts/fix_style.py`
- 其他静态检查或测试入口

特点：

- 适合做提交前总览
- 强调“哪一层失败、是否阻断、是否已降级”

建议字段：

- `stage`
- `tool`
- `scope`
- `severity`
- `bucket`
- `summary`
- `tail`

### 调试日志

来源：

- `startup_trace`
- `debug-artifacts/`
- dump / WER / WinDbg 符号化结果
- 组件边界上的结构化阶段日志

特点：

- 强调时序、状态转移、线程归属、前置条件和失败原因
- 用于收敛根因，不用于替代总入口 gate 汇总

建议字段：

- `component`
- `phase`
- `state`
- `thread`
- `result`
- `reason`
- `details`

### 运行期事件日志

来源：

- UI 切换
- backend/context acquire
- safe mode / fallback
- host 激活和退出

特点：

- 用于把“发生过什么”稳定沉淀成事件，而不是自由文本
- 更适合后续做事件摘要和发布说明素材

建议字段：

- `event`
- `phase`
- `target`
- `result`
- `reason`
- `extra`

## Fallback / Reason 字段约束

所有 fallback、skip、defer、demote 行为，至少要满足：

- 有稳定的 `reason`
- `reason` 优先是短枚举或固定词汇，不要只写大段自然语言
- 需要时再附带 `details`

推荐 reason 词汇：

- `toggle_disabled`
- `state_not_ready`
- `runtime_unavailable`
- `backend_unavailable`
- `context_missing`
- `thread_mismatch`
- `resource_missing`
- `io_failed`
- `validation_failed`
- `baseline_debt`
- `tool_missing`

不推荐：

- `failed`
- `unknown`
- `not ok`
- “大概这里没初始化”

## 报告书写建议

### 面向 gate 汇总

- 先给一级分类
- 再列失败项
- 最后附 `--- 原始尾部输出 ---`

### 面向 issue / fix-note

- 先说明证据来自 gate、dump、startup_trace 还是运行期事件
- 再说明它证明了什么，不要只贴原始输出

### 面向发布说明

- 不直接贴低层日志
- 应先提升为“影响范围 + 已修复行为 + 验证方式”

## 与其他文档的边界

- `.codestable/reference/workflow-entry.md`
  - 放导航
- 本文件
  - 放日志分类和 reason 口径
- `.codestable/reference/event-summary-schema.md`
  - 放事件字段和摘要模板
- `.codestable/reference/pre-merge-verification.md`
  - 放验证动作和闭环要求
