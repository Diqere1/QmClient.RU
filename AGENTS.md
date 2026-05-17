# AGENTS.md

## 项目定位

QmClient（Q1menG Client）是基于 DDNet / TaterClient 的第三方定制客户端。

- 主要语言：C++
- 辅助语言：Rust、Python、少量平台相关语言
- 构建系统：CMake
- 依赖管理：Git Submodules（`ddnet-libs/`）
- 目标平台：Windows、Linux、macOS、Android

这是一个 Fork 项目。默认目标是完成本地功能与维护性改动，不主动引入会影响上游兼容性的行为变化。

## 文档入口/文档说明

- `.ai/reference.md` 是统一参考手册；需要查”提交验证 / PR 审查 / 文档维护 / 发布说明”时，从这里继续，不要自己在目录树里猜入口。
- 仓库级检查时，默认入口是 `qmclient_scripts/gate/check-gate.sh`；需要判断文档或脚本入口是否漂移时，用 `qmclient_scripts/gate/check_workflow_docs.py`。
- 除了 AGENTS.md 和 Claude.md 这类 Agent 文档，其他的如 plan、spec 之类的文档名称使用中文
- `AGENTS.md` 与 `Claude.md` 必须保持 1:1 镜像；本地只改任意一处后，先跑 `qmclient_scripts/gate/sync_agents_claude.py`，再跑 `qmclient_scripts/gate/check_workflow_docs.py`

本文件只保留：

- 项目定位与工作边界
- 不能忘的构建 / 运行 / 测试硬约束
- DDNet 兼容性红线
- 风格与正确性底线
- 约束系统的强制摘要入口

更细的脚本语义、发布/PR 摘要等定位，统一下沉到 `reference.md` 和专项文档。

## 代码审查、Github 提交、文档维护

- 提交前验证、PR 审查清单、文档维护原则和发布说明模板，统一看 `.ai/reference.md`
- 代码审查需要用相关的技能（Skills），如：/chinese-code-review、/code-review-excellence

## 工作边界

- 优先遵循 DDNet 现有实现模式，不为了”现代化”重写既有代码
- 通用现代 C++ 最佳实践（见 context7）与 DDNet 既有风格或兼容性约束冲突时，优先服从 DDNet 约束
- 新功能、玩法变化或较大行为改动，默认先讨论，不直接扩展实现    
- 默认不要修改根目录 `CMakeLists.txt`、协议字段、序列化布局或文件格式定义，除非任务明确要求
- QmClient 的配置项统一使用 `qm_` / `Qm` 前缀，不使用 `cl_` 前缀
- 不要写空模块、空文档、stub 或”以后再决定”这类占位式交付描述
- 工程实现默认走 TDD：先写失败测试，再做最小实现通过测试，最后再整理代码
- BDD（behavior-driven-development、行为驱动开发），是 TDD 的演进，关注外部可见的系统行为，它是业务驱动的、更高层级的验收测试
- 编码必须使用 UTF-8，保留原 BOM 状态（如有）
- 修改前检测原文件换行符（CRLF/LF）和缩进风格（Tab/空格数），修改后保持一致

## 长文件修改规范（强制执行）

1. **禁止**使用 Python `open().write()` 或 PowerShell `Set-Content` 直接重写整个文件
2. 优先做局部插入和局部编辑，不要整篇重生成文档，除非用户明确要求重写整份文件
3. 如果文件超过 300 行，先分段读取，只修改必要片段

## 分支与工作区

- 先确认当前分支，再开始修改。
- 用户明确指定分支时，优先在该分支工作。
- 切换分支前先检查工作区是否有未提交改动。
- 不要擅自 `reset`、覆盖、清理或回退用户本地改动。

## 构建命令

Windows 上必须通过 `qmclient_scripts/cmake-windows.cmd` 调用 CMake，不能默认假设当前 PowerShell 已具备 MSVC 环境。

Windows 构建目录只认这四类语义：

- `build-ninja/`：默认运行目录，约定为 `Release`
- `build-debug/`：调试目录，优先用于启动期/交互期崩溃与黑屏排查
- `build-analyze/`：只供 `qmclient_scripts/gate/strict-debug-check.sh` 跑 MSVC `/analyze`
- `build-asan/`：实验目录；当前 Rust `+crt-static` 约束下默认允许跳过，不算正式依赖构建模式

更细的构建模式选择（Release / Debug / RelWithDebInfo / build-release-pdb）和诊断口径，统一看 `qmclient_scripts/gate/check-gate-workflow.md`。

补充硬约束与常见问题说明：
- 同一个构建目录一次只允许一条构建命令，不要并行跑两个 `cmake --build` / `ninja`。
- 更细的 `qmclient_scripts/gate/strict-debug-check.sh` 扫描范围、降级条件、JSON 字段和 warning 处理规则，统一看 `qmclient_scripts/gate/check-gate-workflow.md`。

Windows：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30
```

Linux / macOS：

```bash
cmake -S . -B cmake-build-release
cmake --build cmake-build-release --target game-client -j 10 | tail -n 30
```

## 仓库级检查与测试

### 门禁入口(包含测试项目)

默认的仓库级规范化检查入口是 `qmclient_scripts/gate/check-gate.sh`：

- `quick`：只做源码卫生层，适合开发期快速自查
- `default`：源码卫生 + 严格调试检查 + C++ 测试，适合日常提交前严格门
- `full`：在 `default` 基础上增加 Rust 测试，适合集中收口

不要在对话里临时拼一套”等价命令”替代它。更细的模式说明、失败分类和输出口径，统一看 `qmclient_scripts/gate/check-gate-workflow.md`。

pre-commit hook 自动跑 `quick` 模式。首次克隆后需要配置一次：

```bash
git config core.hooksPath qmclient_scripts/hooks
```

新增 C++ 测试时：测试文件放在 `src/test/`，文件名使用 `<module>_test.cpp`，并更新根目录 `CMakeLists.txt` 中的测试列表。

### 单独运行测试

默认情况下使用上面的门禁，但是想单独跑某个测试（必要）时：

Windows：
```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_rust_tests
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_tests
```

Linux / macOS：
```bash
cmake --build cmake-build-release --target run_cxx_tests
cmake --build cmake-build-release --target run_rust_tests
cmake --build cmake-build-release --target run_tests
```

## 运行客户端

- 客户端必须从构建目录运行，不要从仓库根目录直接启动 `DDNet.exe`。
- Vulkan 着色器和运行时资源依赖当前工作目录。
- Windows 默认运行目录是 `build-ninja/`，它代表发布态 `Release`；调试排查默认运行目录是 `build-debug/`。
- 运行命令（相对路径）：`cd .\build-ninja; .\DDNet.exe`

## 关键目录

- `src/base/`：基础库；允许与主项目不同的命名风格
- `src/engine/`：引擎层
- `src/game/`：游戏逻辑与客户端界面
- `src/game/client/QmUi/`：QmClient 特有 UI
- `src/test/`：C++ 测试
- `src/rust-bridge/`：Rust 模块
- `qmclient_scripts/`：QmClient 的项目辅助脚本
- `scripts/`：上游脚本与代码生成脚本
- `data/`：客户端资源
- `docs/`：文档

引用文件路径时，统一使用相对于仓库根目录的相对路径，例如 `src/game/client/components/menus.cpp`。

## DDNet 兼容性红线

- 不要破坏网络协议兼容性。
- 不要破坏 demo、skin 等文件格式兼容性。
- 不要破坏现有地图行为。
- 不要让已有 rank 变得不可达。
- 不要在未明确批准的情况下改变物理行为。
- 不要在未明确批准的情况下让已完成地图变得更容易。

以下区域默认高风险，修改前先确认影响范围：

- physics
- dummy
- prediction
- snapshot
- input
- collision
- timer
- replay / demo
- map behaviour
- protocol fields

如果任务涉及上述区域，先明确它落在 `client`、`server`、`shared` 的哪一侧，再检查是否已有类似实现可复用。

## DDNet / C++ 风格约束

### 命名

- 除 `src/base/` 外，变量、方法、类名使用 UpperCamelCase。
- 成员变量使用 `m_` 前缀。
- 全局变量使用 `g_` 前缀。
- 静态变量使用 `s_` 前缀。
- 指针使用 `p` 前缀。
- 定长数组或 `std::array` 使用 `a` 前缀。
- `std::vector` 使用 `v` 前缀。
- 类使用 `C` 前缀。
- 接口使用 `I` 前缀。
- 枚举类型使用 `E` 前缀。
- 命名应有明确语义；除很短且上下文清晰的局部循环变量外，避免 `i`、`k`、`tmp`。

### 枚举与常量

- 新代码中的枚举优先使用 `enum class`。
- 枚举值使用全大写风格。
- 常量优先使用 `constexpr`。
- 位标志优先使用 `inline constexpr` 常量，而不是传统叠位 `enum`。

### 使用边界

- 可以使用现代 C++，但不要引入与当前模块风格明显不一致的新抽象。
- 不要为了“现代化”把现有接口整体模板化、泛型化或工具层化。
- 可以使用 `std::optional`、`std::variant`、移动语义和 `std::string_view`，前提是不会让接口风格和生命周期管理变得更难追踪。
- 避免原始 `new/delete`，除非周边代码已经这样管理。
- 避免 `goto`。
- 避免不必要的宏。
- 避免在 `if` 条件中做赋值，除非这样明显更易读。
- 避免把整数直接当布尔语义使用。
- 避免默认参数滥用。
- 避免隐式所有权转移。
- 避免无意义的堆分配、额外拷贝和热路径临时对象。

### 正确性与生命周期

- 校验外部输入、数组边界、索引和空指针。
- 不要静默忽略错误。
- 若模块既有风格不是异常驱动，不要强行改成异常风格。
- 区分调试断言与运行时容错。
- 明确对象所有权、引用有效期和序列化边界。
- 使用 `std::string_view`、裸指针或引用时，先确认底层存储生命周期足够长。
- 重点避免悬垂引用、悬垂指针、越界访问、未初始化读取和 use-after-free。

### 并发与线程边界

- 不要为了“未来可能并发”预埋锁和原子变量。
- 涉及音频、图形、HTTP、数据库、日志或平台层时，先确认线程边界。
- 只有在确有共享状态风险时才引入 `std::mutex` 或 `std::atomic`。

## 热路径关注点

DDNet 是实时联网游戏。以下问题在 review 时必须额外关注：

- 每帧 / 每 tick / 每玩家 / 每实体路径上的额外分配
- 预测一致性
- 服务端确定性
- 快照、序列化、碰撞检测的额外开销
- 协议体积和带宽变化

## 注释规范（强制执行）

使用 /code-comment-guidelines 技能

## 常见问题

- 构建失败：
    - 先检查 `ddnet-libs/` 是否已初始化
    - Windows 下构建是否通过 `qmclient_scripts/cmake-windows.cmd` 启动（MSVC环境）
- Vulkan 着色器报错或资源页面闪退，先检查是否从正确的构建目录运行