# Pre-Merge Verification

## 目标

本文件定义 QmClient 仓库在提交前、合并前和修复闭环时的验证动作清单。

它回答的是“改完以后要怎么证明”，不是“最初有哪些根规则”。

## 验证原则

- 按改动类型选验证，不要空泛声称“已修复”或“测试通过”。
- 优先提供真实构建、测试、运行、联调或日志证据。
- 涉及 release only 问题时，禁止只靠 `build-debug` 通过就宣布结束。
- 默认优先走仓库级总入口 `qmclient_scripts/check-gate.ps1`，不要在普通提交前手动拼一套零散验证命令替代它。

## 默认总入口

提交前默认验证口径：

- 日常提交前：`powershell -NoProfile -ExecutionPolicy Bypass -File qmclient_scripts\check-gate.ps1 -Mode default -BaseRef main`
- 集中收口：`powershell -NoProfile -ExecutionPolicy Bypass -File qmclient_scripts\check-gate.ps1 -Mode full -BaseRef main`

说明：

- `check-gate.ps1` 是仓库级规范化总入口。
- `strict-debug-check.ps1` 是总入口下的专项严格调试检查，不应在文档口径上和 `check-gate.ps1` 并列成两个默认入口。
- 只有在调试、崩溃、渲染、生命周期等场景需要进一步下钻时，才直接使用 `strict-debug-check.ps1`。

## 基础验证动作

按需组合：

- 构建相关目标
- 运行 `run_cxx_tests`、`run_rust_tests` 或 `run_tests`
- 做 client/server 联调
- 做兼容性回归验证
- 必要时运行 `scripts/fix_style.py`

## 推荐验证映射

### 纯脚本 / 文档 / 工作流改动

- 默认先跑 `check-gate.ps1 -Mode quick`
- 脚本语法检查
- 目标脚本真实跑一遍最小模式
- 核对输出与文档说明一致

### 一般客户端逻辑改动

- 默认先跑 `check-gate.ps1 -Mode default`
- 对应构建目标
- `run_cxx_tests`
- 需要时做人工运行验证

### Rust 相关改动

- 默认先跑 `check-gate.ps1 -Mode full` 或在 `default` 基础上显式补 Rust 测试
- 对应构建目标
- `run_rust_tests`

### 协议 / 地图 / 物理 / 预测 / 回放相关改动

- 默认先跑 `check-gate.ps1 -Mode default`
- 对应构建目标
- 相关测试
- client/server 联调
- 兼容性回归验证

### RmlUI / 渲染 / 生命周期 / 崩溃修复

- 默认先跑 `check-gate.ps1 -Mode default`
- `qmclient_scripts/strict-debug-check.ps1`
- 必要的定向测试
- 对应构建真实运行验证
- 核对 `debug-artifacts` / `startup_trace` / dump / 关键日志

## 发布态问题专项要求

凡是只在 `build-ninja` / `Release` / `RelWithDebInfo` 出现的问题，修复后至少补：

1. 对应构建的真实运行验证
2. 最新 `debug-artifacts` / dump / WER / 关键日志核对

## 旧代码与大范围清理

如果仓库里已有大量不符合新规范的旧代码：

- 需要重构时，明确说明当前任务是“重构”
- 必要时再次强调“严格遵循新规则”
- 大范围清理拆成独立重构任务逐步推进

不要在普通功能或 bug 修复里顺手做大面积“顺便清理”。

## 与其他文档的边界

- `AGENTS.md`
  - 放必须记住的红线和硬约束
- `.codestable/reference/pr-review-checklist.md`
  - 放审查格式和审查顺序
- 本文件
  - 放改完后的验证动作和闭环要求
