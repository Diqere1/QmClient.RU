---
doc_type: issue-fix-note
issue: 2026-05-12-startup-flash-crash-ui-page-sanitize
status: fixed
related:
  - startup-flash-crash-ui-page-sanitize-report.md
  - startup-flash-crash-ui-page-sanitize-analysis.md
tags: [startup, crash, menus, skins7, chat, release, uninitialized-state]
---

# Startup Flash Crash Ui Page Sanitize 修复记录

## 1. 本次修复内容

围绕同一条启动闪退链路，实际落了三处定点修复：

1. `src/game/client/components/menus.cpp` / `menus.h`
   - favorite-community 启动 sanitize 只在 cached / kernel `IServerBrowser *` 一致且非空时才继续使用。
   - 若 interface unavailable 或 mismatch，则稳定重置到 `PAGE_INTERNET`，不再解引用坏指针。

2. `src/game/client/components/skins7.cpp` / `skins7.h`
   - 修复 `LoadSkin` 中 JSON 文件名 basename 提取的长度计算，避免把“去掉 `.json` 后的长度”误当作目标缓冲区大小传给 `str_copy`。
   - 新增安全 helper，统一处理长文件名截断。

3. `src/game/client/components/chat.h`
   - 显式初始化 `CChat` 与 `CChat::CLine` 的启动期标量状态，收口 release-only 未初始化 UB。
   - 这是最后一层崩点：前两处收口后，`build-ninja` 仍会在 `CChat::OnInit` 边界闪退；加入临时日志后崩溃消失，说明问题对代码布局敏感，最终通过显式初始化稳定解决。

## 2. 验证结果

- `qmclient_scripts\cmake-windows.cmd --build build-debug --target run_cxx_tests -j 10`
  - 结果：通过，`835` 个 C++ 测试全部通过。
- `qmclient_scripts\cmake-windows.cmd --build build-debug --target game-client -j 10`
  - 结果：通过。
- `qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10`
  - 结果：通过。
- 启动冒烟：
  - `build-ninja\DDNet.exe` 连续 3 次存活 5 秒以上，由人工强制结束。
  - `build-debug\DDNet.exe` 连续 2 次存活 5 秒以上，由人工强制结束。

## 3. 修复后行为

- release / debug 构建都能稳定跨过原先的启动闪退阶段，进入可运行主界面循环。
- favorite-community 启动 sanitize 不再因为污染的 cached serverbrowser 指针提前崩溃。
- `skins7` 启动扫描不再包含已确认的名字缓冲区写坏点。
- `CChat` 启动路径不再依赖未定义的初始内存内容。

## 4. 影响面回归

- 相关测试：
  - `src/test/menus_test.cpp` 新覆盖 cached/kernel serverbrowser mismatch。
  - `src/test/skins7_test.cpp` 新覆盖长文件名截断与短文件名保持。
- 启动链回归：
  - `CMenus::OnInit`
  - `CSkins7::OnInit` / `LoadSkin`
  - `CChat::OnInit`

## 5. 顺手发现

> `CChat` 之前缺少大量显式初始化，本次已经收口启动期直接相关成员；如果后续还出现类似“release-only、加日志后消失”的现象，优先继续排查同类未初始化状态，而不是先怀疑 RmlUI 渲染层。
