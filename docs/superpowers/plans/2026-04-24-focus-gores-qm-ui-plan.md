# Focus Mode、Gores Mode 与栖梦页面数值控件修复实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 修复 `Focus Mode` 与 `Gores Mode` 的行为语义和恢复链路，并统一栖梦页面高优先级数值控件的交互方式。

**架构：** 以 `qmclient.cpp` 中的状态应用函数为中心，重新划清 `Focus` / `Gores` 的职责边界；先通过测试锁定行为，再修改生产代码；最后仅在栖梦页面收敛高优先级“滑块 + 输入框”实现。

**技术栈：** C++17, gtest, DDNet/QmClient UI system, Windows Ninja build

---

## 执行策略

按用户确认，采用“方案 1”，并分两批执行：

1. 第一批：`Focus Mode` + `Gores Mode`
2. 第二批：栖梦页面高优先级“滑块 + 输入框”

这样可以优先解决行为 bug，再处理 UI 一致性问题。

## 文件清单

| 文件 | 职责 |
|------|------|
| `src/test/qmclient_state_override_test.cpp` | 增加 `Focus` / `Gores` 行为回归测试 |
| `src/game/client/components/qmclient/config_override.h` | 视需要调整 override 辅助逻辑或补充纯函数 |
| `src/game/client/components/qmclient/qmclient.cpp` | `ApplyFocusModeEffects()` 与 `ApplyGoresFastInputLink()` 的主要修复点 |
| `src/game/client/components/qmclient/qmclient.h` | 保存态、快照结构或 helper 声明的收敛点 |
| `src/game/client/components/hud.cpp` | `Hide HUD` / `Hide unnecessary UI` 的渲染职责校正 |
| `src/game/client/components/chat.cpp` | `Hide chat` / `Hide echo` 的过滤与强制可见规则 |
| `src/game/client/components/nameplates.cpp` | 名字、头顶元素、聊天气泡的专注模式门控 |
| `src/game/client/components/scoreboard.cpp` | 计分板门控 |
| `src/game/client/components/infomessages.cpp` | 信息提示门控 |
| `src/game/client/components/qmclient/statusbar.cpp` | 状态栏门控 |
| `src/game/client/components/qmclient/menus_qmclient.cpp` | Gores UI 文案更新与 Qm 页面数值控件统一 |

---

### 任务 1：冻结 Gores 自动快输语义的失败测试

**文件：**
- 修改：`src/test/qmclient_state_override_test.cpp`
- 参考：`src/game/client/components/qmclient/config_override.h`

- [ ] **步骤 1：编写“关闭 Gores 后自动关闭 fast input”的失败测试**

在 `src/test/qmclient_state_override_test.cpp` 中新增最小测试，明确目标行为不是“恢复旧值”，而是“关闭后为 0”。

- [ ] **步骤 2：运行定向测试验证红灯**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 | Select-Object -Last 120
```

预期：

- 新增的 `Gores` 语义测试失败
- 失败原因是当前 override 逻辑仍在恢复旧值

- [ ] **步骤 3：提交测试草案**

```bash
git add src/test/qmclient_state_override_test.cpp
git commit -m "test(qmclient): lock gores fast input auto-toggle semantics"
```

---

### 任务 2：修复 Gores 自动快输联动

**文件：**
- 修改：`src/game/client/components/qmclient/qmclient.cpp`
- 可能修改：`src/game/client/components/qmclient/qmclient.h`
- 修改：`src/game/client/components/qmclient/menus_qmclient.cpp`

- [ ] **步骤 1：将 `ApplyGoresFastInputLink()` 改为状态推导逻辑**

要求：

- `GoresActive && QmGoresFastInput` 时 `tc_fast_input = 1`
- 否则 `tc_fast_input = 0`
- `GoresActive && QmGoresFastInput && QmGoresFastInputOthers` 时 `tc_fast_input_others = 1`
- 否则 `tc_fast_input_others = 0`

- [ ] **步骤 2：同步修改栖梦页面 Gores 相关文案**

目标文案语义：

- `Auto-toggle fast input`
- `Auto-toggle fast input others`

或等价中文本地化文案，但必须体现“自动联动”而不是“直接配置本体”。

- [ ] **步骤 3：运行定向测试验证绿灯**

运行：

```powershell
.\build-ninja\testrunner.exe --gtest_filter=QmClientConfigOverride*
```

预期：

- `Gores` 新增语义测试通过
- 现有无关测试不被破坏

- [ ] **步骤 4：提交修复**

```bash
git add src/game/client/components/qmclient/qmclient.cpp src/game/client/components/qmclient/menus_qmclient.cpp src/test/qmclient_state_override_test.cpp
git commit -m "fix(qmclient): make gores fast input follow gores mode state"
```

---

### 任务 3：冻结 Focus Mode 职责矩阵的失败测试

**文件：**
- 修改：`src/test/qmclient_state_override_test.cpp`
- 参考：`src/game/client/components/qmclient/qmclient.cpp`

- [ ] **步骤 1：为 `Hide unnecessary UI` 漏项补失败测试**

至少锁定以下预期：

- `Hide unnecessary UI` 影响 `TcStatusBar`
- `Hide unnecessary UI` 影响 `QmPlayerStatsMapProgress`
- `Hide unnecessary UI` 影响 `QmSmtcShowHud`
- `Hide unnecessary UI` 还应覆盖 `QmDummyMiniView`

- [ ] **步骤 2：为 `Hide HUD` 与 `Hide names` 的边界补回归测试**

要求：

- `Hide HUD` 继续只覆盖 HUD 主体项
- `Hide names` 只覆盖名字项

- [ ] **步骤 3：运行定向测试验证红灯**

运行：

```powershell
.\build-ninja\testrunner.exe --gtest_filter=QmClientConfigOverride*
```

预期：

- UI 漏项测试在当前实现下失败
- 失败能直接指向 `Focus` 集合不完整或职责不一致

- [ ] **步骤 4：提交测试草案**

```bash
git add src/test/qmclient_state_override_test.cpp
git commit -m "test(qmclient): lock focus mode ui and hud responsibility matrix"
```

---

### 任务 4：修复 Focus Mode 的配置层职责

**文件：**
- 修改：`src/game/client/components/qmclient/qmclient.cpp`
- 修改：`src/game/client/components/qmclient/qmclient.h`

- [ ] **步骤 1：补齐 `Hide unnecessary UI` 的配置覆盖集合**

至少纳入：

- `TcStatusBar`
- `QmPlayerStatsMapProgress`
- `QmSmtcShowHud`
- `QmDummyMiniView`

- [ ] **步骤 2：校正 `SFocusHudConfigSnapshot` 的保存字段**

保证新增集合项都有对应保存位，关闭专注模式后可恢复到非专注模式的正常配置态。

- [ ] **步骤 3：保持 `Hide HUD` / `Hide names` / `Hide UI` 三组集合互不混淆**

不要把名字项塞进 HUD 集合，也不要把 HUD 主体项塞进 UI 集合。

- [ ] **步骤 4：运行定向测试验证绿灯**

运行：

```powershell
.\build-ninja\testrunner.exe --gtest_filter=QmClientConfigOverride*
```

预期：

- `Focus` 职责矩阵相关测试通过

- [ ] **步骤 5：提交修复**

```bash
git add src/game/client/components/qmclient/qmclient.cpp src/game/client/components/qmclient/qmclient.h src/test/qmclient_state_override_test.cpp
git commit -m "fix(qmclient): align focus mode override groups with product semantics"
```

---

### 任务 5：修复 Focus Mode 的渲染层短路边界

**文件：**
- 修改：`src/game/client/components/hud.cpp`
- 修改：`src/game/client/components/chat.cpp`
- 修改：`src/game/client/components/nameplates.cpp`
- 修改：`src/game/client/components/scoreboard.cpp`
- 修改：`src/game/client/components/infomessages.cpp`
- 修改：`src/game/client/components/qmclient/statusbar.cpp`

- [ ] **步骤 1：逐个核对渲染层门控点与职责矩阵是否一致**

重点核对：

- `Hide HUD`
- `Hide unnecessary UI`
- `Hide chat`
- `Hide echo`
- `Hide scoreboard`
- `Hide overhead indicators`

- [ ] **步骤 2：保留 `Hide echo` 的强制可见白名单**

保证专注模式开关提示仍可见，但普通 echo 能被隐藏。

- [ ] **步骤 3：避免“配置已恢复但渲染仍被短路”的矛盾状态**

要求每个门控点都能用规格中的职责解释清楚。

- [ ] **步骤 4：运行定向测试和全量 C++ 测试**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 | Select-Object -Last 120
```

预期：

- 相关测试通过
- 无新增回归

- [ ] **步骤 5：提交修复**

```bash
git add src/game/client/components/hud.cpp src/game/client/components/chat.cpp src/game/client/components/nameplates.cpp src/game/client/components/scoreboard.cpp src/game/client/components/infomessages.cpp src/game/client/components/qmclient/statusbar.cpp
git commit -m "fix(qmclient): align focus mode render gates with override responsibilities"
```

---

### 任务 6：盘点并选择栖梦页面高优先级数值项

**文件：**
- 修改：`src/game/client/components/qmclient/menus_qmclient.cpp`

- [ ] **步骤 1：列出 `Qm` 页面仍使用旧 `DoScrollbarOption(...)` 的高优先级数值项**

优先候选包括：

- `QmChatBubbleDuration`
- `QmChatBubbleAlpha`
- `QmChatBubbleFontSize`
- `QmFriendOnlineRefreshSeconds`
- `QmAutoReplyCooldown`
- `QmPieMenuScale`
- `QmPieMenuOpacity`
- `QmPieMenuMaxDistance`
- `QmCollisionHitboxAlpha`
- `QmAutoTeamLockDelay`
- `QmSpeedrunTimer*`

- [ ] **步骤 2：确认本轮只替换高优先级项，不扩展到旧 TClient 页面**

- [ ] **步骤 3：提交盘点结果到工作树**

```bash
git add src/game/client/components/qmclient/menus_qmclient.cpp
git commit -m "chore(qmclient): prepare qm page numeric control unification scope"
```

---

### 任务 7：统一栖梦页面“滑块 + 输入框”交互

**文件：**
- 修改：`src/game/client/components/qmclient/menus_qmclient.cpp`
- 参考：`src/game/client/ui.cpp`
- 参考：`src/game/client/ui.h`

- [ ] **步骤 1：收敛一个统一 helper**

该 helper 需要覆盖：

- 滑块
- 数值输入框
- 回车提交
- 外点提交
- 自动钳制
- 数值居中

- [ ] **步骤 2：确保纯文本输入框不套用数值策略**

要求：

- 自定义文本不居中
- 点击时不自动全选
- 默认光标在末尾

- [ ] **步骤 3：替换任务 6 选出的高优先级 Qm 数值项**

- [ ] **步骤 4：运行构建验证**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 80
```

预期：

- `DDNet.exe` 链接成功

- [ ] **步骤 5：提交修复**

```bash
git add src/game/client/components/qmclient/menus_qmclient.cpp
git commit -m "refactor(qmclient): unify qm page slider and numeric input interactions"
```

---

### 任务 8：最终验证

**文件：**
- 无新增文件

- [ ] **步骤 1：运行全量 C++ 测试**

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 | Select-Object -Last 120
```

- [ ] **步骤 2：运行定向 override 测试**

```powershell
.\build-ninja\testrunner.exe --gtest_filter=QmClientConfigOverride*
```

- [ ] **步骤 3：完整构建客户端**

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 80
```

- [ ] **步骤 4：运行态 sanity check**

```powershell
Set-Location build-ninja
Start-Process .\DDNet.exe
```

- [ ] **步骤 5：人工核对清单**

核对：

- `Gores Mode` 开/关时 fast input 联动
- `Focus Mode` 开/关时 HUD、UI、名字、echo、计分板恢复
- 栖梦页面数值输入框的回车/外点提交手感

---

## 交接说明

推荐按“第一批行为修复、第二批 UI 一致性修复”执行，不建议把所有任务一次性混在同一个补丁里。

如果进入实现，建议优先使用子代理驱动模式：

1. 主线程负责 `spec` / `plan` 对照与最终集成
2. 子代理一批处理 `Focus/Gores` 测试与行为修复
3. 子代理另一批处理 Qm 页面数值控件统一
