# PR Review Checklist

## 目标

本文件定义 QmClient 仓库的 PR / 提交前代码审查输出格式、检查顺序和最小对照清单。

它属于“审查操作手册”，不是根规则入口；根规则仍以 `AGENTS.md` 为准。

## 适用场景

- 用户要求“review”
- PR 合并前人工/AI 审查
- 功能完成后的对照验收
- 重大修复后的回归审查

## 审查输出格式

每条问题按以下字段输出：

- `Severity`：`Critical` / `Major` / `Minor`
- `File + line`
- `Problem description`
- `Why it matters`
- `Suggested fix`

问题列完后，补充：

- `Overall verdict`：`Correct` / `Needs Fix` / `Unsafe`
- `Short explanation`
- `Optional refactoring suggestions`

## 审查优先级

1. 协议 / 文件格式兼容性
2. 地图 / 物理 / 预测兼容性
3. 正确性
4. 内存与生命周期问题
5. 热路径性能
6. 是否符合 DDNet 既有风格

## 仓库级审查对照清单

## 按改动类型裁剪清单

不要每次把整张表全部硬扫。先判断本次改动属于哪类，再选最小必要清单。

### 脚本 / 文档 / 工作流

重点看：

- 入口文档是否互相引用一致
- 脚本默认入口是否仍是 `qmclient_scripts/check-gate.ps1`
- 命令示例、模式说明、输出口径是否与真实脚本一致
- 是否引入新的平行入口、重复规则或失效路径

可降低优先级：

- 协议 / 物理 / 热路径

### 一般客户端逻辑

重点看：

- 正确性与生命周期
- 现有 UI / 输入 / 状态切换是否被回归
- 是否补了对应构建、测试或人工运行验证

### RmlUI / 渲染 / 生命周期

重点看：

- `RL-01` 到 `RL-10`
- 状态、前置条件、线程归属、fallback reason 是否显式
- 只读查询在空状态 / 未初始化状态下是否无害
- 是否记录了结构化阶段日志或稳定事件
- 是否做过 release / build-ninja 真实路径验证

### 协议 / 预测 / 物理 / 地图行为

重点看：

- 是否触碰兼容性红线
- 是否明确落在 `client` / `server` / `shared` 哪一侧
- 是否做了联调或兼容性回归验证

### 重构 / 债务清理

重点看：

- 是否真的是“行为不变、结构变”
- 是否顺手扩大了改动范围
- 是否把旧债务误包装成当前任务必须一并修复
- 重构前后验证是否对称

### 兼容性

- 是否触碰网络协议、快照、输入、碰撞、回放、地图行为或文件格式
- 如果触碰高风险区域，是否先明确是 `client` / `server` / `shared` 哪一侧
- 是否复用仓库里已有类似实现，而不是重新发明一套

### 正确性与生命周期

- 是否存在空指针、悬垂引用、越界访问、未初始化读取或 use-after-free 风险
- 只读查询接口在早期生命周期、失败态和空资源态下是否仍安全
- 外部输入、索引、边界和错误路径是否得到校验

### 渲染 / 生命周期

- 是否违反 `AGENTS.md` 中的 `RL-01` 到 `RL-10`
- 是否把状态、前置条件、线程归属和 fallback reason 写清楚
- 是否明确了全局渲染状态恢复责任

### 热路径

- 是否在每帧 / 每 tick / 每玩家 / 每实体路径上引入额外分配
- 是否改变预测一致性、服务端确定性或序列化/碰撞的额外开销
- 是否扩大协议体积或带宽占用

### 风格与维护性

- 是否违反仓库命名、类型、所有权、线程边界约束
- 是否为了“现代化”引入与模块风格明显不一致的新抽象
- 是否出现无必要模板化、工具层化、宏化或默认参数滥用

## 审查输出前先声明范围

建议在审查开头先写清：

- 本次按哪一类改动清单审
- 哪些高风险项明确未触碰
- 哪些结论来自代码、哪些来自构建/测试/运行证据

## 审查时的最低输入

开始审查前，至少补齐：

1. `AGENTS.md`
2. `.codestable/attention.md`
3. `.codestable/reference/workflow-entry.md`

如果任务涉及 RmlUI / 崩溃 / 生命周期，再继续读：

- `.codestable/compound/2026-05-09-learning-windows-debug-symbol-workflow.md`
- `.codestable/compound/2026-05-10-decision-render-lifecycle-safety-constraints.md`

## 与其他文档的边界

- `AGENTS.md`
  - 放红线和硬约束
- 本文件
  - 放“怎么审查”
- `.codestable/reference/pre-merge-verification.md`
  - 放“审完以后怎么验”
