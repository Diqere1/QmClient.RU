# Session Handoff

## Current Objective

- Goal: 收口当前 gate/build 基建，统一 Python-first/docs/gate 入口、Windows 构建入口与 `cmake-build-*` 目录口径，并让 `check_gate.py` 在失败/异常时稳定产出 summary 与 JSON 报告。
- Current status: shell 遗留入口已删除，`check_docs.py` / `check_gate.py` 已成为当前活入口；Windows 默认构建入口与目录口径已统一，但 `tests.py` 稳定测试入口、临时文件清理和 `full` gate 终态复验仍在收尾。

## Completed This Session

- [x] 读取 `AGENTS.md`、`.ai/harness.md`、`.ai/ddnet-development.md`、`.ai/verification.md`、`.ai/feature_list.json`、`.ai/progress.md`、`.ai/session-handoff.md`。
- [x] 新增计划文档 `docs/superpowers/plans/2026-05-27-菜单-ui-统一实现计划.md`，把 menubar、browser/settings/demo、skins/runtime cache、Windows 验证与只读审查拆成可执行步骤。
- [x] `menus.cpp` / `menus.h`：
  - 新增 `MenuPanelColor()` / `MenuPanelElevatedColor()` 统一菜单卡片取色 helper。
  - `RenderMenubar()` 把顶部中间项真实切出 `6px` 间隙。
  - 左上主菜单改为常驻蓝色圆底；右上纯图标按钮 idle 透明，hover/active 才出底。
  - 把 assets 的 `Entity Preview` 布局状态纳入 settings page runtime key，避免继续命中旧 FBO。
- [x] `config_variables.h` / `menus_settings.cpp`：
  - 新增 `cl_menu_panel_color`、`cl_menu_panel_opacity`、`cl_menu_panel_elevated_opacity` 三个菜单主面板配置项。
  - 在“设置 -> 图像”接入菜单卡片颜色、主透明度、强调透明度调节，并在变更时失效 settings runtime cache。
  - 设置页左右主卡片改走统一菜单卡片 helper，保留 `10px` 内容边距。
- [x] `menus_browser.cpp`：
  - 服务器列表、底栏、右栏改走统一菜单卡片配色。
  - toolbox tab active/inactive 底色改走统一卡片体系。
- [x] `menus_demo.cpp`：
  - 删除旧整页大底板。
  - list/details/buttons 三段按 `10px` 外边距/间距/内边距重新排布，并改走统一卡片 helper。
- [x] `menus_settings_assets.cpp`：
  - `Entity Preview` 状态从函数内静态提升到文件级，供 runtime key / cache invalidate 共用。
  - 资源预览继续统一走 `ComputePreviewDrawRect()` contain 规则，覆盖本地、workshop、tile preview 与 loading fallback。
  - `Entity Preview` 开关切换时显式 `InvalidateSettingsPageRuntimeCache(SETTINGS_ASSETS, -1)`。
- [x] `feat-006` 设置面板现代化：
  - `menus_settings.cpp`：设置页外壳改为右侧导航 + 主玻璃内容区。
  - `menus_settings_controls.cpp`：controls 分组块改为 QmUi token 化卡片。
- [x] `feat-007` 演示回放界面现代化：
  - `menus_demo.cpp`：demo 播放控制浮层、seekbar/marker/slice、浏览器列表/详情/按钮外壳 token 化。
- [x] `feat-008` 游戏内 HUD 现代化：
  - `hud.cpp`：movement/key/spectator/local-time 等小面板统一为 SURFACE_GLASS。
  - `scoreboard.cpp`：scoreboard 外层、队伍标题、本地/跟随玩家高亮 token 化。
- [x] `feat-009` 游戏内通知与覆盖层现代化：
  - `chat.cpp`、`motd.cpp`、`emoticon.cpp`、`voting.cpp`、`infomessages.cpp` 完成覆盖层/消息卡片化。
  - 当前仓库没有 `killmessages.cpp`，击杀/完成消息实际入口为 `infomessages.cpp`。
- [x] `.ai/feature_list.json` 更新 `feat-006` 到 `feat-009` 为 `done`，写入构建证据。
- [x] `.ai/progress.md` 更新完成范围、风险、验证证据和下次视觉验证清单。
- [x] `src/game/version.h` 与 `docs/info.json` 更新到 `2.57.0`。
- [x] `feat-011` 用户反馈批处理：
  - Demo 切片渲染 pending 源、快速练习 dummy 后连接和基础武器状态修复。
  - 灵动岛队伍显示及配置开关、快速练习 `/tofinish`。
  - `entity_bg/...` 配置规范化/fallback；设置页实体层背景选择弹窗改为枚举用户实际使用的 `maps` 和 `mapres`，并支持 `mapres/...` 直接加载；实体层视频 FFmpeg 只转换最终上传帧且保持播放时钟同步，MediaFoundation 只保留最新帧。
- [x] `src/game/version.h` 与 `docs/info.json` 继续按 MMP 更新到 `2.58.0`。
- [x] `.ai/feature_list.json`、`.ai/progress.md` 与本 handoff 补充 feat-011 状态、验证证据和实机回归缺口。

## Verification Evidence

| Check | Command | Result |
|---|---|---|
| feat-006/007 build | `qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10` | PASS，链接 `DDNet.exe` 成功；中途修复 `MakeUiScopeHash` include 缺失后通过 |
| feat-008 build | `qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10` | PASS，链接 `DDNet.exe` 成功 |
| feat-009 build | `qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10` | PASS，链接 `DDNet.exe` 成功 |
| final build | `qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10` | PASS，101/101 完成，链接 `DDNet.exe` 成功；包含 `2.57.0` 版本号重编 |
| harness consistency | `python qmclient_scripts\gate\check_workflow_docs.py` | PASS，未发现断链 |
| C++ tests | `cmake-build-release\testrunner.exe` | PASS，601 tests from 80 test suites passed |
| feat-011 diff formatting | `git diff --check` | PASS，仅有 LF/CRLF 工作树提示 |
| feat-011 client build | `qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10` | PASS |
| feat-011 background config tests | `cmake-build-release\testrunner.exe --gtest_filter=BackgroundEntitiesValue.*` | PASS，8/8 tests |
| feat-011 entity bg picker regression | `cmake-build-release\testrunner.exe --gtest_filter=BackgroundEntitiesValue.*:AssetsResourceRegistry.EntityBg*` | PASS，17/17 tests |
| feat-011 full C++ tests | `qmclient_scripts\cmake-windows.cmd --build cmake-build-debug --target run_tests -j 4` | PASS，729/729 tests |
| menu ui unification build | `qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10` | PASS，154/154 完成，链接 `build-ninja\DDNet.exe` 成功 |
| menu ui unification tests | `build-ninja\testrunner.exe` | PASS，877 tests from 97 test suites passed |
| entity bg picker maps/mapres release build rerun | `qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10` | FAIL only at link，99/99 编译完成，`cmake-build-release\DDNet.exe` 正在运行导致 LNK1104 |
| direct debug build without VS env | `cmake --build cmake-build-debug --target run_tests --config Debug -j 4` | FAIL，缺少 `cstddef`；使用仓库 Windows wrapper 后通过 |
| feat-011 quick gate（历史旧入口证据） | `C:\Program Files\Git\bin\bash.exe qmclient_scripts/gate/check-gate.sh --mode quick --base-ref main` | FAIL，被既有 33 个未使用配置项阻断；新增 `qm_hud_island_show_team` 不在失败列表，其他 quick 子检查通过 |

Note: 上表里的 `build-ninja` 记录是这次目录改名前的历史验证证据；当前 canonical 目录口径已经统一为 `cmake-build-release` / `cmake-build-debug` / `cmake-build-analyze`。
Note: `qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target run_cxx_tests -j 10` 在当前 build tree 返回 `unknown target 'run_cxx_tests'`，因此直接运行实际存在的 `testrunner.exe`。

## Files Changed

- `.ai/feature_list.json`
- `.ai/progress.md`
- `.ai/session-handoff.md`
- `docs/info.json`
- `src/game/version.h`
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
- `src/engine/shared/config_variables_qmclient_extra.h`
- `src/game/client/components/background.cpp`
- `src/game/client/components/background.h`
- `src/game/client/components/menus.cpp`
- `src/game/client/components/menus.h`
- `src/game/client/components/qmclient/menus_qmclient.cpp`
- `src/game/client/components/tclient/fast_practice.cpp`
- `src/game/client/components/tclient/fast_practice.h`
- `docs/superpowers/plans/2026-05-27-菜单-ui-统一实现计划.md`
- `.ai/reference.md`
- `.ai/workflow-manifest.json`
- `python qmclient_scripts/gate/check_docs.py`
- `src/engine/shared/config_variables.h`
- `src/game/client/components/menus.h`
- `src/game/client/components/menus_browser.cpp`
- `src/game/client/components/menus_settings_assets.cpp`

## Blockers / Risks

- 006~009 当前只有构建证据，仍需用户实机视觉验收。
- 2026-05-27 菜单 UI 统一这轮已经通过构建、自动测试和 harness 文档校验，但 menubar underline / 间距、browser/demo/settings 卡片统一、skins 预览实际显示仍需用户在客户端里确认。
- 009 涉及多个上游 DDNet UI 文件，PR 描述需要显式声明扩域；本次只改绘制外观，不改网络消息、投票命令、demo 回放、聊天存储或 HUD 数据逻辑。
- `bash qmclient_scripts/init.sh` 仍受本机 WSL vhdx 缺失影响，不能作为当前机器的可用入口；PowerShell 下的 Python harness check 和 Windows CMake wrapper 可用。
- `feat-011` 仍需要实机验证：切片视频首帧、快速练习连接 dummy/reset 后武器、灵动岛各显示状态、entity_bg 新旧资源 fallback、视频播放 CPU/FPS 和 `/tofinish` 目标选择。
- Quick gate 现可经 Git Bash 执行，但会被当前仓库既有 33 个未使用配置项阻断；该失败与 `feat-011` 新增配置无关。

## Next Session Startup

1. Read `AGENTS.md` and focused `.ai/` docs.
2. Build if needed: `qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10`.
3. Run tests if needed: `cmake-build-release\testrunner.exe`.
4. Launch `cmake-build-release\DDNet.exe` and visually verify:
   - menubar 中间项真实 `6px` 间距、左上蓝色圆底、右上页面图标参与 underline、动作图标 idle 透明；
   - browser 左列表/底栏/右栏卡片对齐；
   - settings 左右卡片与顶部导航对齐；
   - demo 页面旧大底板是否已去除且 list/details/buttons 间距统一；
   - assets/skins/entity preview 是否完整显示、无错位截断；
   - feat-011 demo cut render、practice dummy/weapon、dynamic island team、entity background/video 与 `/tofinish`。

## Recommended Next Step

Ask the user for runtime/visual confirmation on the menu UI unified layout and feat-011 scenarios, then prepare commit notes if no new UI regressions are reported.
