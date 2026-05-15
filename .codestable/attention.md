# Attention

本文件是 CodeStable 技能启动必读的项目注意事项入口。所有 CodeStable 子技能开始工作前必须读取它。

继续分流时，统一跳转到 `.codestable/reference/workflow-entry.md`；不要直接在 `.codestable/` 目录树里猜下一份该读什么。

## 项目碎片知识

<!-- cs-note managed: 用 cs-note 维护，新条目按下面分节追加 -->

### 编译与构建

- Windows 构建目录口径统一为：`build-ninja=Release 默认运行目录`，`build-debug=调试目录`，`build-analyze=/analyze 专用目录`，`build-asan=默认允许跳过的实验目录`。
- 除非任务明确要求，不要再把 `RelWithDebInfo` / `MinSizeRel` 当成正式并行 Windows 构建模式扩散到脚本、文档或口头说明里。

### 运行与本地起服务

### 测试

- QmClient 的工程实现默认走 TDD：先写失败测试，再实现最小改动让测试通过，最后再做必要整理。
- 涉及 UI 或游戏内表现的最终验收统一由人工检查完成，不把截图当作默认验收产物。
- Windows 启动期崩溃先用 `startup_trace` 判断死前阶段，再优先用 `build-debug/DDNet.exe` + `build-debug/DDNet.pdb` 收敛根因，不要先围着 `build-ninja` 的 release offset 猜。

### 命令与脚本陷阱

- 处理 `DDNet.exe` 进程前必须先核对可执行路径，只能操作当前工作树的 `build-ninja/DDNet.exe`，不能按进程名批量结束，否则会误杀别的客户端。
- RmlUI、连接链路或崩溃修复后优先跑 `.\qmclient_scripts\strict-debug-check.ps1`；它现在是正式的 `build-debug`、`build-analyze`、`build-asan` + `clang-tidy` 一键入口，其中 `build-asan` 当前仅是实验目录，不替代 `build-debug` 人工验收。
- `strict-debug-check.ps1` 的 `/analyze` 默认只扫本次改动涉及的 `.c/.cc/.cpp` 编译单元；要做全仓首方源码全量扫时，显式传 `-AnalyzeAll`。
- `strict-debug-check.ps1` 打出来的 CMake / clang-tidy / 编译警告、Git CRLF 警告，以及 `Suppressed warnings` 这类被过滤掉的 warning 文本都要继续追，不要把“脚本跑完了”当成绿色通过。
- Windows 上的 Debug CRT hook 现在由 `QM_ENABLE_WINDOWS_CRT_ASSERT_LOGGER` 统一控制；`strict-debug-check.ps1` 会显式打开它，方便把 CRT 断言写进 `debug-artifacts/`。
- 当前仓库的 `.cargo/config.toml` 会在 MSVC 下强制 Rust `+crt-static`；在这条约束移除前，`strict-debug-check.ps1` 默认要明确打印并跳过 `build-asan`，继续后续检查；只有显式传 `-RequireAsan` 时才把它当成硬失败。

### 路径与目录约定

- QmClient 的配置项统一使用 `qm_` / `Qm` 前缀，不使用 `cl_` 前缀命名项目自有配置。

### 环境变量与凭证

### 其他

- RmlUI 规划与 design 不允许用“空模块 / 空文档 / stub / 以后再决定”作为交付描述；若落地路径未明确，需要先补规划分析或直接向用户确认。
- 涉及线程、启动链、渲染链或 backend context 的改动前，必须先梳理原始调用链、线程归属、初始化时机和 fallback；关系没搞清楚时先补 explore / learning，再改代码。
