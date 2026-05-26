# Session Progress Log

## Current State

**Last Updated:** 2026-05-27
**Active Feature:** gate/build 基建收口中，目标是统一 Python-first 入口、Windows 构建入口、`cmake-build-*` 目录口径，并让 `check_gate.py` 在失败/异常时稳定产出 summary 与 JSON 报告

## Status

### What's Done

- [x] feat-001 修复列宽拖拽（已 commit 4217bd737）
- [x] feat-002 UI 动画系统增量扩展（PR#1-4 全实施）
- [x] feat-003 设计 token + 11 共享 UI 原语（PR-A 到 PR-E 全实施）
- [x] feat-004 主菜单/启动屏/Menubar 现代化（PR-004a 到 d 全实施）
- [x] feat-005 服务器浏览器视觉升级（PR-A 到 PR-E 全实施）
  - **PR-A 已存在的 ui_listbox.cpp 改动纳入**：全局 hover SURFACE_HIGHLIGHT / selected ACCENT_PRIMARY_DIM（影响所有列表，作为 feat-005 全局基础）
  - **PR-B 行卡片 + hover spring**：menus_browser.cpp `RenderServerbrowserServerList` 注入 ResolveUiAnimValueColor + ui_curve::BTN_HOVER spring（非选中行 SURFACE_GLASS 玻璃卡片软淡入）+ `DoAutoSpacing(2.0f)` 行间隙 + selected 行 DrawOutline(BORDER_FOCUS)
  - **PR-C 加载/刷新 shimmer 动效**：RefreshBar 改 Draw4(ACCENT_PRIMARY dim/hot) + sin(GlobalTime\*2.5) 推动 Hot alpha 形成扫光
  - **PR-D 列视觉强化**：COL_COMMUNITY 加 SURFACE_ELEVATED chip 圆角背景 / COL_FRIENDS 多人计数改 ACCENT_PRIMARY_DIM pill bottom-right / COL_PLAYERS 按填充率 fill→偏红 渐变着色（空服务器 TEXT_DISABLED）
  - **PR-E 底部状态栏 + Connect 强调**：StatusBox 加 SURFACE_HIGHLIGHT 面板背景 + BORDER_SUBTLE 边框 / Connect 按钮颜色 (0.5,1,0.5,0.5) 绿色 → ACCENT_PRIMARY (Steam 蓝, alpha 0.55)
  - **PR-A 基建**：`UiTokens.h`（36 token）+ `UiContext.h`（IUiContext + MakeUiScopeHash）+ `QmUiTokensTest.cpp`（编译期 static_assert + 3 runtime tests）
  - **PR-B MVP**：`QmAnimResolve.{h,cpp}`（从 menus.cpp 提取 BuildUiAnimNodeKey/ResolveUiAnimValue/Rect/Color）+ `UiContainers.h`（DrawCard 模板）+ `UiButtons.{h,cpp}`（Primary/Secondary，hover 色彩 spring）
  - **PR-C 表单**：`UiForms.{h,cpp}`（TextField 含 focus ring spring、Toggle 含 knob spring、Slider 含 label）+ IconButton 追加到 UiButtons
  - **PR-D 导航/覆盖**：`UiNavigation.{h,cpp}`（TabBar 含下划线 EMPHASIZED 滑动 + ListItem hover）+ `UiOverlays.h`（Tooltip 转发 + Modal 含 SCALE spring + ESC 关闭）
  - **PR-E 视觉验证**：`UiDogfood.{h,cpp}` 展示 11 原语（双栏 UiScale 1.0/0.78）+ `dbg_qm_ui_dogfood` config + 接管 RenderSettingsQmClient
- [x] Harness 迁移到 `deusyu/harness-engineering` 风格：
  - 根 `AGENTS.md` / `CLAUDE.md` 改为短入口地图，不再承载完整手册。
  - 新增 `.ai/harness.md`、`.ai/session-lifecycle.md`、`.ai/ddnet-development.md`、`.ai/verification.md`、`.ai/review.md` 五个分层规则文件。
  - `.ai/reference.md` 改为路由表和脚本入口事实源。
  - `.ai/feature_list.json`、`.ai/progress.md`、`.ai/session-handoff.md`、`qmclient_scripts/init.sh` 作为仓库记录系统的一部分。
  - `qmclient_scripts/gate/check_workflow_docs.py` 新增 harness 结构检查：根地图长度、分层文档引用、状态文件分节、`.ai/feature_list.json` 状态枚举和单 active feature 约束、`qmclient_scripts/init.sh` 入口片段。
  - `sync_agents_claude.py` 与 gate 文档统一到大写 `CLAUDE.md`，修掉旧的 `Claude.md` / `.codestable` 漂移。
- [x] feat-006 设置面板现代化
  - `menus_settings.cpp`：设置页外壳改为右侧导航 + 主玻璃内容区，导航项接入 QmUi token 色板与 hover 动画。
  - `menus_settings_controls.cpp`：controls 页集中分组块改为 SURFACE_GLASS/SURFACE_ELEVATED 卡片、顶部高亮和 token 化标题/展开图标颜色。
  - 不改配置项语义、配置写回路径、语言页/资产页/音频编辑器状态逻辑。
- [x] feat-007 演示回放界面现代化
  - `menus_demo.cpp`：demo 播放控制浮层改为 token 化玻璃面板，seekbar/切片/标记颜色切换到 ACCENT/WARNING/DANGER。
  - demo 浏览器主区域、列表、详情、底部按钮区加入卡片化外壳和选中态强调。
  - 不改 demo 播放、seek、切片、渲染、文件选择/删除/重命名逻辑。
- [x] feat-008 游戏内 HUD 现代化
  - `hud.cpp`：movement/key/spectator/local-time 等 HUD 小面板统一成 SURFACE_GLASS。
  - `scoreboard.cpp`：scoreboard 外层面板、队伍标题、本地/跟随玩家高亮切换到 QmUi token。
  - 不改 HUD 数据来源、玩家排序、计分、spectator 或 scoreboard 激活条件。
- [x] feat-009 游戏内通知与覆盖层现代化
  - `chat.cpp`：非 old chat 的消息背景切换到 SURFACE_GLASS。
  - `motd.cpp`：MOTD 主面板切换到 SURFACE_ELEVATED。
  - `emoticon.cpp`：表情轮盘外圈/内圈/中心底色切换到 token 化 overlay/highlight/elevated。
  - `voting.cpp`：投票面板和 vote bars 切换到 token 化 surface/success/danger。
  - `infomessages.cpp`：当前仓库没有 `killmessages.cpp`，击杀/完成消息实际在 `infomessages.cpp`，已加入右上消息卡片背景。
  - 不改聊天存储、MOTD 网络消息、表情发送、投票命令、击杀/完成消息生命周期。
- [x] feat-011 用户反馈批处理
  - Demo 节选导出渲染保存明确的 pending 切片来源，普通 demo 渲染仍使用当前列表选择。
  - 快速练习支持启用后连接 dummy，重建本地 practice world/anchor/预测状态，并在 anchor 恢复时保持可用基础武器 ammo 语义。
  - 新增 `/tofinish`，按 game layer 后 front layer 的固定顺序查找首个 `TILE_FINISH`。
  - 灵动岛新增默认开启的 `qm_hud_island_show_team`，显示本地或观战目标 DDRace team，并纳入宽度/避让计算。
  - 实体层背景配置写回规范化为 `entity_bg/...`，加载优先 `assets/entity_bg/...` 并保留 `maps/entity_bg/...` fallback；不迁移文件。
  - 设置页实体层背景选择弹窗按用户实际路径列出 `maps` 和 `mapres`；`mapres/...` 选择值会按原路径加载，不再错误拼到 `maps/mapres/...`。资源管理侧仍保留 `assets/entity_bg` 目录。
  - 实体层视频背景让 FFmpeg 仅转换最终上传帧且不伪造播放时钟，让 MediaFoundation 仅保留最新帧，降低后台解码/搬运压力。
- [x] MMP 版本更新：UI epic 从 `2.56.2` 更新到 `2.57.0`；feat-011 从 `2.57.0` 更新到 `2.58.0`。

### What's In Progress

- [ ] gate/build 基建收口：`check_gate.py` / `check_docs.py` 成为唯一活入口，删除 shell 遗留入口，统一 Windows 默认走 `qmclient_scripts\cmake-windows.cmd`。
- [ ] `full` gate 复验：确认真实失败项只剩仓库既有 debt / 环境债，不再出现 gate 入口自身挂住或无 summary/无 report 的情况。

### What's Next

1. 修完 gate/build 基建阻塞项：稳定测试入口、Windows 临时文件清理、活文档入口统一。
2. 重跑 `python qmclient_scripts\gate\check_gate.py --mode full --base-ref main --report-json-path ...`，确认 summary/report 终态。
3. 读取子代理只读审查报告，修复剩余阻塞项后再决定提交策略。
4. gate/build 基建收口后，再回到菜单 UI / feat-011 的实机视觉与运行时回归。

## Blockers / Risks

- 006~009 为视觉改造，当前只有构建证据；仍需用户实机视觉验收，尤其是 UI scale、不同分辨率、demo 回放、在线投票和聊天消息堆叠。
- 2026-05-27 这一轮菜单 UI 统一虽然已经通过当前 canonical `cmake-build-release` 口径对应的构建与测试验证，但 menubar、browser、settings、demo、skins 仍需实机截图确认最终视觉是否符合设计稿。
- feat-011 的 demo 渲染起点、快速练习 dummy/武器、灵动岛布局和视频 CPU/FPS 属于运行时行为，当前已覆盖编译和已有测试，但仍需客户端实机回归。
- 历史 quick gate 证据来自旧 shell 入口 `check-gate.sh --mode quick`；当前入口已统一到 `python qmclient_scripts/gate/check_gate.py --mode quick --base-ref main`。
- 历史 `check_workflow_docs.py` 失败记录仅表示旧门禁状态；当前 docs 入口已统一到 `python qmclient_scripts/gate/check_docs.py`。
- 已知设计取舍：Slider 用 `pValue` 同时作为 Id（dogfood 内多控件共享 id 风险已规避：DoToButton 使用 CButtonContainer，其他用各自地址）。
- `bash qmclient_scripts/init.sh` 当前会调用 Windows WSL `C:\Windows\System32\bash.exe`，本机 WSL vhdx 缺失导致失败；已通过 `python qmclient_scripts/gate/check_workflow_docs.py` 和 `py_compile` 验证 harness 脚本主体。

## Decisions Made

- **色板**：保留 QmClient 玻璃态作 surface（SURFACE_GLASS 0.08/0.09/0.12/0.70），借 Steam 主蓝 #66c0f4 ≈ ColorRGBA(0.4, 0.753, 0.957) 作 ACCENT_PRIMARY。
- **2026-05-27 菜单卡片系统**：菜单主面板颜色与透明度不再散落在 browser/settings/demo 各处，统一改为 `cl_menu_panel_color`、`cl_menu_panel_opacity`、`cl_menu_panel_elevated_opacity` 三个配置项驱动。
- **2026-05-27 menubar 规则**：顶部导航中间项必须真实切出 `6px` 间隙；左上主菜单常驻蓝色圆底；右上纯图标按钮 idle 透明，仅 hover/active 显示玻璃底。
- **2026-05-27 assets/runtime cache**：`Entity Preview` 这类不进入全局 config hash 的局部布局状态，必须进 settings page runtime key 或显式失效 FBO；本轮通过文件级状态 + assets page cache invalidate 收口。
- **依赖注入**：用 IUiContext POD struct，避免把 11 个原语全部绑定到 CMenus 类。
- **header-only Modal/Card/Tooltip**：template 模板 + lambda body 模式；零开销。
- **`ColorRGBA constexpr` 限制**：MSVC C++20 strict mode 在 constexpr 上下文只允许 union 激活成员 `x/y/z/a`，不能用 `r/g/b`。static_assert 用 `.x` 而非 `.r`，runtime EXPECT_NEAR 仍用 `.r/.g/.b`。
- **QmAnimResolve 提取时机**：feat-002 PR#3 把 Rect/Color 封装放 menus.cpp 匿名命名空间；feat-003 PR-B 需要跨文件调用，故立即提取到 `QmUi/QmAnimResolve.{h,cpp}`，menus.cpp 改 include。无破坏性变更。
- **Dogfood 入口**：不开新 tab，简单"接管"现有 RenderSettingsQmClient（条件 `dbg_qm_ui_dogfood != 0`）。最小侵入。

## Files Modified This Session

### feat-002（已提前完成）
- `src/game/client/QmUi/QmAnim.h/cpp`
- `src/game/client/QmUi/QmAnimCurves.h`（新）
- `src/game/client/components/menus.cpp`（PR#3 包装，PR-B 时迁出）
- `src/test/QmAnimTest.cpp`

### feat-005 PR-A 到 PR-E
- `src/game/client/ui_listbox.cpp`（PR-A，已存在改动；173-189 行 SURFACE_HIGHLIGHT/ACCENT_PRIMARY_DIM 替换）
- `src/game/client/components/menus_browser.cpp`（PR-B/C/D/E；+3 include / +DoAutoSpacing / +hover card spring ~15 行 / +selected outline 2 行 / +shimmer Draw4 ~10 行 / +COL_COMMUNITY chip 1 行 / +COL_FRIENDS pill 9 行 / +COL_PLAYERS fill-tint ~15 行 / +StatusBox 面板背景 2 行 / Connect 颜色改 ACCENT_PRIMARY）

### feat-003 PR-A 到 PR-E
- `src/game/client/QmUi/UiTokens.h`（新）
- `src/game/client/QmUi/UiContext.h`（新，header-only）
- `src/game/client/QmUi/QmAnimResolve.{h,cpp}`（新，从 menus.cpp 提取）
- `src/game/client/QmUi/UiContainers.h`（新，header-only 模板）
- `src/game/client/QmUi/UiButtons.{h,cpp}`（新）
- `src/game/client/QmUi/UiForms.{h,cpp}`（新）
- `src/game/client/QmUi/UiNavigation.{h,cpp}`（新）
- `src/game/client/QmUi/UiOverlays.h`（新，header-only）
- `src/game/client/QmUi/UiDogfood.{h,cpp}`（新）
- `src/game/client/components/menus.cpp`（include QmAnimResolve.h、删除匿名命名空间 ResolveUiAnim*）
- `src/game/client/components/qmclient/menus_qmclient.cpp`（include + dogfood 入口）
- `src/engine/shared/config_variables_qmclient.h`（DbgQmUiDogfood 配置）
- `src/test/QmUiTokensTest.cpp`（新，3 测试）
- `CMakeLists.txt`（注册 11 个新文件 + 1 新测试）

### harness 迁移
- `AGENTS.md`（新入库，短入口地图）
- `CLAUDE.md`（新入库，与 AGENTS 同步）
- `.ai/harness.md`
- `.ai/session-lifecycle.md`
- `.ai/ddnet-development.md`
- `.ai/verification.md`
- `.ai/review.md`
- `.ai/reference.md`
- `.ai/workflow-manifest.json`
- `.ai/feature_list.json`
- `.ai/progress.md`
- `.ai/session-handoff.md`
- `qmclient_scripts/init.sh`
- `.gitignore`
- `qmclient_scripts/gate/check_workflow_docs.py`
- `qmclient_scripts/gate/sync_agents_claude.py`
- `qmclient_scripts/gate/check-gate.sh`
- `qmclient_scripts/gate/check-gate-workflow.md`
- `qmclient_scripts/脚本总览.md`

### feat-006 到 feat-009
- `src/game/client/components/menus_settings.cpp`
- `src/game/client/components/menus_settings_controls.cpp`
- `src/game/client/components/menus_demo.cpp`
- `src/game/client/components/hud.cpp`
- `src/game/client/components/scoreboard.cpp`
- `src/game/client/components/chat.cpp`
- `src/game/client/components/motd.cpp`
- `src/game/client/components/emoticon.cpp`
- `src/game/client/components/voting.cpp`
- `src/game/client/components/infomessages.cpp`
- `src/game/version.h`
- `docs/info.json`

### feat-011 用户反馈批处理
- `src/engine/shared/config_variables_qmclient_extra.h`
- `src/game/client/components/background.cpp`
- `src/game/client/components/background.h`
- `src/game/client/components/hud.cpp`
- `src/game/client/components/menus.cpp`
- `src/game/client/components/menus.h`
- `src/game/client/components/menus_demo.cpp`
- `src/game/client/components/menus_settings.cpp`
- `src/game/client/components/qmclient/menus_qmclient.cpp`
- `src/game/client/components/tclient/fast_practice.cpp`
- `src/game/client/components/tclient/fast_practice.h`

### 2026-05-27 菜单 UI 统一收口
- `docs/superpowers/plans/2026-05-27-菜单-ui-统一实现计划.md`
- `src/engine/shared/config_variables.h`
- `src/game/client/components/menus.h`
- `src/game/client/components/menus.cpp`
- `src/game/client/components/menus_browser.cpp`
- `src/game/client/components/menus_demo.cpp`
- `src/game/client/components/menus_settings.cpp`
- `src/game/client/components/menus_settings_assets.cpp`
- `src/game/version.h`
- `docs/info.json`
- `.ai/feature_list.json`
- `.ai/progress.md`
- `.ai/session-handoff.md`

## Evidence of Completion

- [x] testrunner：729 tests PASSED（含 7 既有 UiV2Anim + 5 UiV2AnimSpring + 4 UiV2AnimEasing + 3 QmUiTokens）。
- [x] game-client release build：99-151/152 通过，链接 DDNet.exe 成功，零新警告。
- [x] harness 文档一致性：`python qmclient_scripts/gate/check_workflow_docs.py` 通过。
- [x] harness Python 语法：`python -m py_compile qmclient_scripts\gate\check_workflow_docs.py qmclient_scripts\gate\sync_agents_claude.py` 通过。
- [x] feat-006/007 首轮构建：`qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10` 通过，链接 `DDNet.exe` 成功。中途修复 `MakeUiScopeHash` include 缺失。
- [x] feat-008 构建：`qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10` 通过，链接 `DDNet.exe` 成功。
- [x] feat-009 构建：`qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10` 通过，链接 `DDNet.exe` 成功。
- [x] 最终构建：`qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10` 通过，101/101 完成，链接 `DDNet.exe` 成功；包含 `2.57.0` 版本号重编。
- [x] 当前 build tree 的 C++ 测试：`cmake-build-release\testrunner.exe` 通过，601 tests from 80 test suites PASSED。`run_cxx_tests` 目标在当前 `cmake-build-release` Ninja target 列表中不存在，已直接运行实际测试二进制。
- [x] 最终 harness 文档一致性：`python qmclient_scripts\gate\check_workflow_docs.py` 通过，未发现断链。
- [x] feat-011 客户端构建：`qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10` 通过。
- [x] feat-011 背景配置定向测试：`cmake-build-release\testrunner.exe --gtest_filter=BackgroundEntitiesValue.*` 通过，8/8 tests PASSED。
- [x] feat-011 实体背景选择页补测：`cmake-build-release\testrunner.exe --gtest_filter=BackgroundEntitiesValue.*:AssetsResourceRegistry.EntityBg*` 通过，17/17 tests PASSED。
- [x] feat-011 全量 C++ 测试：`qmclient_scripts\cmake-windows.cmd --build cmake-build-debug --target run_tests -j 4` 通过，729/729 tests PASSED。
- [x] 2026-05-27 菜单 UI 统一 build-ninja 构建：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10` 通过，154/154 完成并链接 `build-ninja\DDNet.exe` 成功。
- [x] 2026-05-27 菜单 UI 统一 build-ninja 测试：`build-ninja\testrunner.exe` 通过，877 tests from 97 test suites PASSED。
- [x] 2026-05-27 菜单 UI 统一审查后收口：右上页面图标重新参与 underline，动作图标继续排除；browser 左右卡片真实留出 10px 间距；assets 的 `Entity Preview` 与 `Show Workshop Assets` 均会主动失效页面缓存。
- [x] 2026-05-27 harness 文档校验脚本收口：`python qmclient_scripts\gate\check_workflow_docs.py` 通过；已移除对旧 `governance.yml`、`strict-debug-check.sh`、`refresh_baseline_debt_allowlist.py` 的过时依赖，并改为当前 `check_gate.py` / `check_docs.py` 体系。
- [x] 2026-05-27 菜单 UI 统一最终 build-ninja 构建：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10` 通过，增量重编并重新链接 `build-ninja\DDNet.exe` 成功。
- [x] 2026-05-27 菜单 UI 统一最终 build-ninja 测试：`build-ninja\testrunner.exe` 通过，877 tests from 97 test suites PASSED；测试日志中的 `.tmp` 路径为既有 filesystem / gameworld 测试临时目录输出，不代表失败。
- [x] 2026-05-27 构建目录口径统一：当前 canonical 命名统一为 `cmake-build-release` / `cmake-build-debug` / `cmake-build-analyze`；上面保留的 `build-ninja` 记录仅表示改名前的历史验证证据。
- [x] feat-011 diff 格式检查：`git diff --check` 通过（仅显示现有工作树的 LF/CRLF 转换提示）。
- [ ] 实体背景选择页 `maps`/`mapres` 补丁后的 Release 链接：`qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10` 99/99 编译完成后在链接阶段失败，原因是当前 `cmake-build-release\DDNet.exe` 进程正在运行并锁住输出文件；不是编译错误。
- [ ] 裸 `cmake --build cmake-build-debug --target run_tests --config Debug -j 4`：失败，MSVC 环境未加载导致缺少 `cstddef`；改用 Windows 包装脚本后通过，不属于代码回归。
- [ ] feat-011 quick gate（历史旧入口证据）：`C:\Program Files\Git\bin\bash.exe qmclient_scripts/gate/check-gate.sh --mode quick --base-ref main` 运行完成但失败；阻断为仓库既有 33 个未使用配置项，新增 `qm_hud_island_show_team` 不在失败列表，其他 quick 子检查通过。
- [ ] `bash qmclient_scripts/init.sh`：未通过，原因是本机 WSL 磁盘路径缺失，不是脚本检查项失败。
- [ ] 006~009 视觉验证：未执行。本会话留给用户。
- [ ] feat-011 实机交互和视频性能回归：未执行，按反馈计划中的场景验证。

## Notes for Next Session

- 视觉验证脚本：`cmake-build-release\DDNet.exe`。
- 006 验证：设置页右侧导航、主内容卡片、controls 分组块展开/收起。
- 007 验证：Demo 浏览器列表/详情/按钮区、Demo 播放控制浮层、seekbar marker/slice 颜色。
- 008 验证：HUD movement/key/spectator/local-time 小面板、scoreboard 双队/单队/多人列表、本地玩家高亮。
- 009 验证：chat 消息背景、MOTD、emoticon wheel、vote panel、infomessages 击杀/完成消息卡片。
- feat-011 验证：节选导出直接渲染首帧；快速练习后连 dummy 与 reset 后手枪；灵动岛队伍开关/观战；entity_bg 新旧路径；视频 FPS/CPU 与循环；`/tofinish` 三类地图。
- 下一次 agent 启动应先读 `AGENTS.md` 短地图，再按任务读取 `.ai/` 分层文档。
