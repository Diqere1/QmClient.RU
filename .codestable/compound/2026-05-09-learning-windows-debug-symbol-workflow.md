---
doc_type: learning
track: knowledge
date: 2026-05-09
slug: windows-debug-symbol-workflow
component: Windows 客户端崩溃排查链路
severity: high
tags: [windows, debug, pdb, crashdump, assert, startup-trace, workflow]
related_issue: 2026-05-09-rmlui-settings-gray-overlay-esc-crash
---

# Windows 客户端崩溃排查链路：PDB、Debug 构建与提示词用法

## 1. 这次学到的核心结论

在 QmClient 这类 Windows GUI 崩溃排查里，**`build-debug` 不是“更慢的另一个包”，而是更早暴露根因的诊断入口**；**PDB 不是可执行逻辑，而是把崩溃偏移翻译回函数 / 文件 / 行号 / 类型信息的符号地图**。

这次用户在 `build-debug/DDNet.exe` 里点击“资源设置 -> 实体层背景图”时，没有先得到一份模糊的 access violation，而是直接得到：

- `Debug Assertion Failed!`
- `File: ...\\include\\vector`
- `Expression: vector subscript out of range`

这说明同一路径在 Debug CRT / STL 检查下已经被提升成**可读的越界断言**。相比只在 `build-ninja` / Release 里拿到 `0xc0000005 + offset`，这条证据更接近源码级根因。

## 2. 这些东西分别是什么

### 2.1 PDB 是什么

PDB（Program Database）是 Windows / MSVC 调试符号文件，里面主要存：

- 函数名与地址映射
- 源文件 / 行号映射
- 类型布局、局部变量、内联信息
- 供 dump / debugger / 符号化工具读取的调试元数据

它**不是程序运行所必需的逻辑文件**。没有 PDB，程序照样能跑；但一旦崩溃，`0x215602` 这类偏移就很难直接还原成“哪个函数、哪一行、哪个容器越界”。

### 2.2 `build-debug/DDNet.exe` 是什么

这里说的“ddnet-debug”在本仓库语境下，本质上就是 **Debug 配置构建出来的 DDNet 可执行文件和同目录符号文件**，例如：

- `build-debug/DDNet.exe`
- `build-debug/DDNet.pdb`

它和常用的 `build-ninja/DDNet.exe` 的差异主要是：

- 优化更少，便于调试
- 会保留完整符号
- MSVC 的 Debug STL / CRT 会启用更多边界和迭代器检查
- 同类 bug 更容易以 assert / 明确错误暴露，而不是晚一点才炸成野指针或访问冲突

### 2.3 CrashDump / WER 是什么

- `CrashDump`：进程崩溃时的内存快照，常见在 `%LOCALAPPDATA%\\CrashDumps`
- `WER`：Windows Error Reporting，记录应用错误事件、异常码、fault offset、归档目录

它们回答的是：

- 崩了没有
- 什么时候崩的
- 异常码是什么
- 偏移是多少

但它们本身**不自动告诉你源码语义**，还需要 PDB 或相邻 debug 构建帮助翻译。

### 2.4 `startup_trace` / `debug-artifacts` 是什么

这是项目里补上的开发期诊断链路，用来记录：

- 启动阶段走到哪了
- 图形 / backend / RmlUI 什么时候开始异常
- 哪些阶段还没跑完

它们回答的是“程序死前最后走到了哪一步”，和 dump / assert 属于互补证据。

## 3. 它们是怎么配合工作的

推荐把这条链路理解成四层：

1. **Debug 构建**
   - 负责尽早把未定义行为抬升成 assert 或明确检查失败。
2. **startup trace / debug-artifacts**
   - 负责告诉我们崩前最后一个稳定阶段。
3. **WER / CrashDump**
   - 负责给出异常码、fault offset、崩溃时间点。
4. **PDB / 符号化**
   - 负责把偏移翻回函数、类型、源码行。

简单说：

- `build-debug` 更像“带护栏的现场复现”
- `dump + WER` 更像“事故现场记录”
- `PDB` 更像“现场坐标系和地图”
- `startup_trace` 更像“行车记录仪”

## 4. 这次断言图到底说明了什么

这张图不是“Visual Studio 自己坏了”，也不是“RmlUI 一定坏了”。它更精确地说明：

- 当前点击“实体层背景图”的路径里，某处对 `std::vector` 做了越界下标访问
- Debug STL 在 `<vector>` 内部先把这个错误截住了
- 如果只在 Release 下跑，常见表现会退化成更晚期的 crash、灰屏、随机行为或只有 `0xc0000005`

也就是说，这种弹窗本质上是**好消息**：

- 它虽然说明程序有 bug
- 但同时也说明 Debug 构建把 bug 收口到了“容器越界”这个层级
- 比只拿到一份 release dump 更接近可修状态

## 5. 以后默认怎么排查

### 5.1 优先顺序

如果同一个问题能在 `build-debug` 复现，默认优先：

1. 从 `build-debug` 启动复现
2. 先看 assert / 断言框 / debug 输出
3. 再补 `startup_trace`
4. 只有还不够时，再看 dump / WER / offset

如果只能在 `build-ninja` / Release 复现，默认顺序改成：

1. 取最新 `CrashDump`
2. 取对应时间段 `WER`
3. 取同时间的 `startup_trace`
4. 用 PDB 或相邻 Debug 构建把 offset 往源码映射

### 5.2 为什么不能一上来只盯 release crash

因为 release 崩溃更像“结果”，debug assert 更像“原因”。

同一个 bug：

- 在 Debug 下可能直接告诉你 `vector subscript out of range`
- 在 Release 下只会给你 `DDNet.exe + 0x215602`

两者的信息密度不是一个级别。

## 6. 以后该怎么给 AI 提示词

### 6.1 好提示词的目标

不是笼统说“又崩了，你看看”，而是明确：

- 这是 Debug 还是 Release
- 有没有 assert
- 最新 dump / trace 在哪
- 最希望 AI 先做哪条链路

### 6.2 推荐提示词模板

#### 模板 A：Debug 断言优先

```text
这是 build-debug 的断言框，不要先猜 RmlUI 或渲染结论。
先把断言翻译成代码级错误类型，再给我最短定位链路：
现象 -> 可能的容器/索引 -> 对应源码入口 -> 下一步应加的打点。
```

#### 模板 B：Release 崩溃优先

```text
这是 build-ninja/Release 的崩溃。
请按 Windows 崩溃链路排查，不要先猜：
1. 最新 CrashDump
2. 同时间 WER
3. 同时间 startup_trace
4. offset 对源码的收敛结论
最后再给怀疑点。
```

#### 模板 C：要求系统化闭环

```text
不要只给怀疑点。
请把这次问题拆成：
1. 当前最强证据
2. 还缺什么证据
3. 下一步用 debug build 还是 release dump
4. 需要补什么可持续调试工具
并把结论同步成项目 learning。
```

#### 模板 D：要求少猜、多验证

```text
先不要改代码。
先证明它是：
- 容器越界
- 空指针
- 生命周期问题
- 渲染上下文问题
中的哪一类，再往下走。
```

## 7. 这次以后该形成的默认做法

- UI / 资源页 / 设置页这类交互崩溃，**先问能不能在 `build-debug` 复现**。
- 如果 `build-debug` 已经弹出 STL / CRT assert，优先围绕 assert 做源码定位，不要继续从 release 偏移硬猜。
- `startup_trace` 用来回答“死前走到哪”，PDB / debug build 用来回答“为什么死”。
- dump / WER 是底层证据，不该替代 debug 构建。
- 当一次崩溃同时涉及 UI、渲染、资源页，先区分“assert 级容器错误”和“图形上下文错误”，不要把所有症状混成一个大黑盒。

## 8. 给后续工作的直接建议

这次“实体层背景图”断言之后，下一轮排查不该从 RmlUI 总体假设继续发散，而应直接把问题收敛成：

- 哪个资源页路径在点击后读了空 / 过期 / 越界的 `vector`
- 这个 `vector` 的索引来自哪一个当前选中项 / 资源列表 / 层级列表
- Debug 构建是否能进一步给出更稳定的复现入口

如果后续要补工具链，优先级应是：

1. 保证 `build-debug` 运行路径稳定可复现
2. 完善符号化 / PDB 使用说明
3. 为资源页 / 设置页这类交互路径补更细的阶段打点

