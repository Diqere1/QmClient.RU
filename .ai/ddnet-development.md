# DDNet / QmClient 开发规则

在 QmClient 中做 C++ 实现、重构或调试时，使用这份文档。

## 兼容性优先

DDNet 兼容性优先级高于泛化的现代 C++ 偏好。

没有明确批准时，不要改这些：

- Network protocol fields or layout.
- Demo, skin, map, config, or save file formats.
- Physics, collision, prediction, snapshots, inputs, timing, replay, or map behavior.
- Anything that makes existing ranks unreachable or existing maps easier.

如果任务触碰到这些区域，先指出风险，再开始实现，并把补丁保持在最小范围。

## 范围边界

QmClient 的常规范围包括：

- `src/game/client/components/qmclient/`
- `src/game/client/QmUi/`
- `src/engine/shared/config_variables_qmclient*.h`
- `src/game/version.h`
- `data/languages/simplified_chinese.txt`
- `docs/info.json`
- `qmclient_scripts/`
- `.ai/`、`AGENTS.md`、`CLAUDE.md` 和其他 agent/harness 文件

没有明确请求时，以下内容都算超范围：

- Upstream engine core outside QmClient config.
- Server gameplay, editor internals, protocol, physics, collision, prediction, snapshots, replay behavior.
- Third-party libraries in `ddnet-libs/` or `src/engine/external/`.
- Release CI workflow behavior.

## 风格

优先遵循现有 DDNet 风格，而不是泛化的 C++ 风格：

- Prefer UpperCamelCase for local variables, methods, and classes outside special areas like `src/base`.
- Use existing prefixes: `m_` members, `g_` globals, `s_` statics, `p` pointers, `a` fixed arrays, `v` vectors, `C` classes, `I` interfaces.
- Prefer semantic names. Short loop variables are acceptable only when the scope is tiny and obvious.
- Prefer early returns and small focused functions, but do not split code so much that DDNet-style readability suffers.

## 现代 C++

如果和当前模块风格匹配，可以使用：

- `constexpr`
- `enum class` with `E...` names and uppercase values
- `std::optional`
- `std::variant`
- move semantics
- `std::array`
- carefully scoped `std::string_view`

避免：

- raw `new` / `delete` unless the surrounding code owns objects that way
- unnecessary macros
- `goto`
- assignment inside `if` conditions
- treating integers as booleans
- hidden ownership transfer
- unnecessary heap allocation
- broad template or RAII rewrites that do not match the module

## 运行时与热路径

DDNet 是实时联网游戏。先判断代码是不是跑在每帧、每 tick、每玩家、每实体、每个 snapshot、每个渲染项或文本布局路径上。

要特别警惕：

- heap allocations in render/tick paths
- repeated string construction
- repeated sorting or scanning
- repeated `TextWidth` or layout computation
- config writes on unchanged state
- serialization/deserialization inside frequent loops
- extra network bandwidth or protocol growth

不要过早优化，但也不要把明显的热路径浪费带进去。

## 错误处理与数据边界

- 不要静默忽略文件、网络、解析、配置、控制台、资源或外部数据失败。
- 校验索引、大小、指针和外部输入。
- 对开发者不变量使用 debug assertion，对用户/外部失败使用运行时处理。
- 遵循当前模块既有的错误传播风格，不要大面积改成异常驱动流程。

## 内存与生命周期

重点检查：

- dangling pointers or references
- returning references to local data
- invalidated iterators
- out-of-bounds access
- use-after-free or double free
- uninitialized reads
- `string_view` or pointer lifetime mismatches

当代码使用缓存、静态或全局状态时，要考虑初始化顺序和线程安全。

## 线程

不要为了“以防万一”就引入线程、锁或原子变量。如果代码碰到音频、图形、HTTP、存储、数据库、日志、平台代码或后台任务，先识别线程边界和共享可变状态。
