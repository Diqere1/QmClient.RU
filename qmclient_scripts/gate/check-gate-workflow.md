# check-gate.sh 工作流说明

## 目标

`qmclient_scripts/gate/check-gate.sh` 是 QmClient 的仓库级总入口。

它的职责不是重写一套新规则，而是把仓库里已经可信的脚本和构建入口按层组织起来，形成统一的本地检查工作流。

它还承担两件额外收口职责：

- 用 `.ai/workflow-manifest.json` 校验根规则和 workflow 文档入口是否齐全
- 用 `qmclient_scripts/gate/baseline_debt_allowlist.json` 把“已知历史债务”和“当前新增阻断”区分开

## 入口分层

### 仓库总入口

- `qmclient_scripts/gate/check-gate.sh` 是仓库级总入口
- 它负责把源码卫生检查、严格调试检查、测试、allowlist 与 JSON 报告收口成统一工作流
- 需要一次执行 `quick/default/full` 模式时，优先使用它

### 严格子入口

- `qmclient_scripts/gate/strict-debug-check.sh` 是严格调试检查的规范入口和事实源
- 它负责真实构建、MSVC `/analyze`、clang-tidy、ASan、范围收敛与严格结果汇总

总原则：

- 需要跑“仓库级门禁模式”时，用 `qmclient_scripts/gate/check-gate.sh`
- 需要单独跑“严格调试检查主体”时，用 `qmclient_scripts/gate/strict-debug-check.sh`

## 分层模式

### `quick`

- 目标：开发期快速自查
- 预期：通常应在数分钟内完成
- 默认内容：
  - 配置变量使用检查
  - 工作流文档一致性检查
  - 头文件 guard 检查
  - 标准头文件检查
  - `fix_style.py -n`
- 阻断策略：
  - 只阻断明显的脚本/规范问题
  - 不跑真实构建
  - 不跑测试

### `default`

- 目标：日常提交前严格门
- 预期：需要真实构建、严格静态分析和 C++ 测试
- 默认内容：
  - `quick` 全部
  - `qmclient_scripts/gate/strict-debug-check.sh`
  - `run_cxx_tests`
- 阻断策略：
  - 构建失败阻断
  - 严格静态分析失败阻断
  - C++ 测试失败阻断

### `full`

- 目标：集中收口 / 准发布门
- 预期：在 `default` 基础上增加更重的附加检查
- 默认内容：
  - `default` 全部
  - 标识符命名检查
  - `run_rust_tests`
- 阻断策略：
  - `default` 层失败照常阻断
  - 高噪音附加检查优先按 `WARN` 试跑，不直接作为默认硬门

## 文件范围规则

默认“改动范围”来自两部分：

- `${BaseRef}...HEAD` 的分支差异
- 当前工作树未暂存差异
- 当前工作树已暂存但未提交差异
- 当前工作树未跟踪的新建源码文件

随后统一收紧为“首方源码范围”：

- 只保留 `src/**` 下的 `.c/.cc/.cpp/.h/.hpp`
- 默认排除：
  - `src/engine/external/**`
  - `src/game/generated/**`
  - `src/rust-bridge/base/**`

这样做的目的是避免第三方、生成文件和工具桥接文件把 gate 结果污染成噪音。

`qmclient_scripts/gate/strict-debug-check.sh` 是严格子入口的规范源。它当前提供：

- `--print-file-scope`：打印纳入文件、排除文件和稳定 `reason`
- `--report-json-path`：输出 `BaseRef`、`Degraded`、`InputScope`、`DefaultScope`、`EffectiveScope`

## 检查项分级

### 默认硬门

- `check_config_variables.py`
- `qmclient_scripts/gate/check_workflow_docs.py`
  - 规则来源：`.codestable/reference/workflow-manifest.json`
- `check_header_guards.py`
- `check_standard_headers.py`
- `fix_style.py -n`
  - 最低要求 `clang-format >= 20`
  - Windows 上会按命令行长度预算分批调用，避免一次性全仓参数触发 `WinError 206`
  - 当 `check-gate.sh` 已算出 scoped files 时，会只把当前收敛后的首方源码文件列表传给 `fix_style.py`
- `qmclient_scripts/gate/strict-debug-check.sh`
- `run_cxx_tests`
- `run_rust_tests`（仅 `full`）

这里的默认假设是：

- 运行/测试目录默认是 `build-ninja`
- 严格调试检查目录默认是 `build-debug/build-analyze`
- `build-asan` 当前只作为附带实验目录，不能和前两者等权理解

额外说明：

- `qmclient_scripts/gate/check-gate.sh` 负责仓库级模式编排，但其中”严格调试检查”这一层应以 `qmclient_scripts/gate/strict-debug-check.sh` 的语义为准
- `qmclient_scripts/gate/strict-debug-check.sh` 是统一的 bash 主实现，后续应以它为事实源维护严格检查语义
- 构建目录语义以 `AGENTS.md` 的定义为准

### 默认关闭，仅按需开启

- `check_unused_header_files.py`

原因：当前仓库噪音偏大，不适合作为默认门。

### 附加 `WARN` 层

- `--enable-clang-format-check`
- `--enable-full-clang-tidy-warn`

这两项的定位是“额外发现问题”，不是默认阻断。

## 结果分类

`check-gate.sh` 的汇总和 JSON 报告现在应把失败至少分成三类：

- `环境/工具`
  - 解释器、PATH、缺脚本、缺依赖目录、构建目录占用、基线解析失败
- `仓库基线债务`
  - 配置变量、workflow 文档入口、header guard、style、标识符、这类长期源码卫生问题
- `当前改动/构建阻断`
  - `qmclient_scripts/gate/strict-debug-check.sh`、`run_cxx_tests`、`run_rust_tests`、`run_tests`、JSON 报告写盘这类当前执行路径上的真实阻断

这样做的目的不是“自动判断责任归属”，而是让使用者第一眼区分：

- 是本机没配好
- 还是仓库本身历史债务很多
- 还是当前这次改动/构建链路真的炸了

同时，总入口应尽量保留底层脚本 / 构建命令的原始失败摘要，而不是只给一条抽象的“退出码非零”。

## baseline debt allowlist 机制

当 `仓库基线债务` 类检查失败时，总入口会按 `Title + DetailHash` 去匹配：

- `qmclient_scripts/gate/baseline_debt_allowlist.json`

命中后行为：

- 控制台与 JSON 报告里的 `OriginalLevel` 仍保留 `FAIL`
- 实际汇总层级 `Level` 会降级成 `WARN`
- 附带 `AllowlistReason` 与 `DetailHash`
- allowlist 条目本身应附带 `base_ref`、`added_at`、`source_report`、`source_generated_at`

默认不应手改 hash。推荐流程是：

```bash
bash qmclient_scripts/gate/check-gate.sh --mode quick --base-ref main --report-json-path tmp/check-gate-report.json
python qmclient_scripts/gate/refresh_baseline_debt_allowlist.py --report tmp/check-gate-report.json --output qmclient_scripts/gate/baseline_debt_allowlist.json
```

## 推荐用法

### 日常开发快速自查

```bash
bash qmclient_scripts/gate/check-gate.sh --mode quick
```

### 提交前仓库级严格门

```bash
bash qmclient_scripts/gate/check-gate.sh --mode default --base-ref main
```

### 集中收口仓库级全门

```bash
bash qmclient_scripts/gate/check-gate.sh --mode full --base-ref main
```

### 单独执行严格调试检查主体

```bash
bash qmclient_scripts/gate/strict-debug-check.sh --base-ref main
```

### 严格检查机器可消费报告

```bash
bash qmclient_scripts/gate/strict-debug-check.sh --base-ref main --report-json-path tmp/strict-debug-check-report.json
```

### 严格检查范围 / debug 诊断

```bash
bash qmclient_scripts/gate/strict-debug-check.sh --base-ref main --print-file-scope --report-json-path tmp/strict-debug-check-scope.json
```

JSON 报告当前至少应包含：

- `BaseRef`
- `GeneratedAt`
- `Summary`
- `ScopedFiles`
- `Items[*].OriginalLevel`
- `Items[*].Level`
- `Items[*].CategoryId`
- `Items[*].AllowlistReason`
- `Items[*].DetailHash`

## 当前仍需继续改进的点

- `qmclient_scripts/gate/strict-debug-check.sh` 现在是严格子入口的统一规范源
- `clang-format` 与 `fix_style.py` 的职责边界还需要长期观察，不建议现在直接二选一
- CI 应优先复用本脚本的模式定义和报告格式，而不是另起一套规则；当前入口为 `.github/workflows/governance.yml`
