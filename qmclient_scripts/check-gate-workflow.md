# check-gate.ps1 工作流说明

## 目标

`qmclient_scripts/check-gate.ps1` 是 QmClient 的仓库级总入口。

它的职责不是重写一套新规则，而是把仓库里已经可信的脚本和构建入口按层组织起来，形成统一的本地检查工作流。

它还承担两件额外收口职责：

- 用 `.codestable/reference/workflow-manifest.json` 校验根规则和 workflow 文档入口是否齐全
- 用 `qmclient_scripts/baseline_debt_allowlist.json` 把“已知历史债务”和“当前新增阻断”区分开

## Windows 构建目录口径

当前仓库在 Windows 上只收口这几类 `build-*` 目录：

- `build-ninja`
  - 默认运行目录
  - 应固定代表 `Release`
  - 用于日常运行验证、发布态问题复现和 `run_cxx_tests/run_rust_tests/run_tests`
- `build-debug`
  - 调试目录
  - 固定代表 `Debug`
  - 用于黑屏、崩溃、RmlUI 生命周期、`startup_trace`、PDB 与 WinDbg 收敛
- `build-analyze`
  - 静态分析目录
  - 固定代表 `Debug + /analyze`
  - 只供 `strict-debug-check.ps1` 使用
- `build-asan`
  - 保留实验目录
  - 当前 Rust `+crt-static` 约束下默认允许跳过
  - 不是当前正式可依赖构建模式

不要把 `RelWithDebInfo`、`MinSizeRel` 再扩成新的并行 Windows 构建目录口径；需要试验时单独说明，不纳入默认工作流。

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
  - `strict-debug-check.ps1`
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

`strict-debug-check.ps1` 复用同一套默认范围语义，并额外提供：

- `-PrintFileScope`：打印纳入文件、排除文件和稳定 `reason`
- `-ReportJsonPath`：输出 `BaseRef`、`Degraded`、`InputScope`、`DefaultScope`、`EffectiveScope`

## 检查项分级

### 默认硬门

- `check_config_variables.py`
- `check_workflow_docs.py`
  - 规则来源：`.codestable/reference/workflow-manifest.json`
- `check_header_guards.py`
- `check_standard_headers.py`
- `fix_style.py -n`
  - 最低要求 `clang-format >= 20`
  - Windows 上会按命令行长度预算分批调用，避免一次性全仓参数触发 `WinError 206`
  - 当 `check-gate.ps1` 已算出 scoped files 时，会只把当前收敛后的首方源码文件列表传给 `fix_style.py`
- `strict-debug-check.ps1`
- `run_cxx_tests`
- `run_rust_tests`（仅 `full`）

这里的默认假设是：

- 运行/测试目录默认是 `build-ninja`
- 严格调试检查目录默认是 `build-debug/build-analyze`
- `build-asan` 当前只作为附带实验目录，不能和前两者等权理解

### 默认关闭，仅按需开启

- `check_unused_header_files.py`

原因：当前仓库噪音偏大，不适合作为默认门。

### 附加 `WARN` 层

- `-EnableClangFormatCheck`
- `-EnableFullClangTidyWarn`

这两项的定位是“额外发现问题”，不是默认阻断。

## 结果分类

`check-gate.ps1` 的汇总和 JSON 报告现在应把失败至少分成三类：

- `环境/工具`
  - 解释器、PATH、缺脚本、缺依赖目录、构建目录占用、基线解析失败
- `仓库基线债务`
  - 配置变量、workflow 文档入口、header guard、style、标识符、这类长期源码卫生问题
- `当前改动/构建阻断`
  - `strict-debug-check.ps1`、`run_cxx_tests`、`run_rust_tests`、`run_tests`、JSON 报告写盘这类当前执行路径上的真实阻断

这样做的目的不是“自动判断责任归属”，而是让使用者第一眼区分：

- 是本机没配好
- 还是仓库本身历史债务很多
- 还是当前这次改动/构建链路真的炸了

同时，总入口应尽量保留底层脚本 / 构建命令的原始失败摘要，而不是只给一条抽象的“退出码非零”。

这样分类结果、控制台汇总和 JSON 报告才能对得上，也方便后续继续把失败归因细化到环境、历史债务或当前阻断。

控制台输出约束：

- 默认应输出中文阶段标题、命令行和最终汇总
- 单个底层脚本如果输出过长，总入口应自动折叠中间段，只保留前后窗口和失败摘要
- 机器可消费的完整事实以 JSON 报告为准，不要求把超长原始输出完整刷到终端

## baseline debt allowlist 机制

当 `仓库基线债务` 类检查失败时，总入口会按 `Title + DetailHash` 去匹配：

- `qmclient_scripts/baseline_debt_allowlist.json`

命中后行为：

- 控制台与 JSON 报告里的 `OriginalLevel` 仍保留 `FAIL`
- 实际汇总层级 `Level` 会降级成 `WARN`
- 附带 `AllowlistReason` 与 `DetailHash`
- allowlist 条目本身应附带 `base_ref`、`added_at`、`source_report`、`source_generated_at`，避免后续无法追溯它是基于哪份基线报告写入的

这样可以区分：

- 历史已知债务
- 本次新引入的同类失败

默认不应手改 hash。推荐流程是：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File qmclient_scripts\check-gate.ps1 -Mode quick -BaseRef main -ReportJsonPath tmp\check-gate-report.json
python qmclient_scripts\refresh_baseline_debt_allowlist.py --report tmp\check-gate-report.json --output qmclient_scripts\baseline_debt_allowlist.json
```

使用约束：

- allowlist 文件缺失时，总入口应按空 allowlist 继续，而不是在 preflight 直接失败。
- 只允许把“已经确认是历史债务”的项加入 allowlist
- `refresh_baseline_debt_allowlist.py` 默认只做增量合并；需要按当前报告全量重写时，显式传 `--rewrite`
- 任何当前新增失败都不应直接刷新进去当作通过；至少先看脚本打印出的新增 diff
- allowlist 只负责降级 `仓库基线债务`，不会屏蔽环境问题或真实构建/测试阻断

## 推荐用法

### 日常开发

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File qmclient_scripts\check-gate.ps1 -Mode quick
```

### 提交前

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File qmclient_scripts\check-gate.ps1 -Mode default -BaseRef main
```

### 集中收口

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File qmclient_scripts\check-gate.ps1 -Mode full -BaseRef main -EnableClangFormatCheck -EnableFullClangTidyWarn
```

### 机器可消费报告

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File qmclient_scripts\check-gate.ps1 -Mode default -ReportJsonPath tmp\check-gate-report.json
```

JSON 报告当前至少应包含：

- `BaseRef`
- `GeneratedAt`
- `Summary`
- `FailureSummaryByCategory`
- `ScopedFiles`
- `Items[*].OriginalLevel`
- `Items[*].Level`
- `Items[*].CategoryId`
- `Items[*].AllowlistReason`
- `Items[*].DetailHash`

### 范围 / debug 诊断

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File qmclient_scripts\check-gate.ps1 -Mode default -BaseRef main -ExplainScope -ScopeReportPath tmp\check-gate-scope.json
```

这条模式适合排查：

- 为什么某些文件被纳入或排除
- staged / untracked 文件有没有被真正扫到
- 当前基线 `BaseRef` 是否过旧或不符合预期

scope 报告应至少提供：

- `BaseRef` 是否可用
- `BaseRef` 失败原因
- 纳入文件列表
- 排除文件列表及稳定 `reason`

CI 侧同样应保留 scope 证据：

- `quick-gate-windows` 上传 `tmp/check-gate-ci-scope.json`
- `default-gate-windows` 上传 `tmp/check-gate-default-ci-scope.json`
- 这样失败后不仅能看 log，还能直接复核“为什么这批文件被扫到 / 没被扫到”

## 当前仍需继续改进的点

- `strict-debug-check.ps1` 仍是 Windows 专用重入口；当前已经有中文阶段汇总、范围诊断和 JSON 报告，后续重点应放在真实构建失败归因与可调试性增强，而不是再起一套平行入口
- `clang-format` 与 `fix_style.py` 的职责边界还需要长期观察，不建议现在直接二选一
- CI 应优先复用本脚本的模式定义和报告格式，而不是另起一套规则；当前入口为 `.github/workflows/governance.yml`
