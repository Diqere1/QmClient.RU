# Session Handoff

## Current Objective

- Goal: 完成 QmClient UI epic 的验收准备，并收口 `feat-011` 用户反馈批处理。
- Current status: `feat-006` 至 `feat-009` 已等待视觉验收；`feat-011` 已实现并通过构建/自动测试，版本号已按 MMP 从 `2.57.0` 升到 `2.58.0`。

## Completed This Session

- [x] 读取 `AGENTS.md`、`.ai/harness.md`、`.ai/ddnet-development.md`、`.ai/verification.md`、`feature_list.json`、`progress.md`、`session-handoff.md`。
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
- [x] `feature_list.json` 更新 `feat-006` 到 `feat-009` 为 `done`，写入构建证据。
- [x] `progress.md` 更新完成范围、风险、验证证据和下次视觉验证清单。
- [x] `src/game/version.h` 与 `docs/info.json` 更新到 `2.57.0`。
- [x] `feat-011` 用户反馈批处理：
  - Demo 切片渲染 pending 源、快速练习 dummy 后连接和基础武器状态修复。
  - 灵动岛队伍显示及配置开关、快速练习 `/tofinish`。
  - `entity_bg/...` 配置规范化/fallback；设置页实体层背景选择弹窗改为枚举用户实际使用的 `maps` 和 `mapres`，并支持 `mapres/...` 直接加载；实体层视频 FFmpeg 只转换最终上传帧且保持播放时钟同步，MediaFoundation 只保留最新帧。
- [x] `src/game/version.h` 与 `docs/info.json` 继续按 MMP 更新到 `2.58.0`。
- [x] `feature_list.json`、`progress.md` 与本 handoff 补充 feat-011 状态、验证证据和实机回归缺口。

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
| entity bg picker maps/mapres release build rerun | `qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10` | FAIL only at link，99/99 编译完成，`cmake-build-release\DDNet.exe` 正在运行导致 LNK1104 |
| direct debug build without VS env | `cmake --build cmake-build-debug --target run_tests --config Debug -j 4` | FAIL，缺少 `cstddef`；使用仓库 Windows wrapper 后通过 |
| feat-011 quick gate | `C:\Program Files\Git\bin\bash.exe qmclient_scripts/gate/check-gate.sh --mode quick --base-ref main` | FAIL，被既有 33 个未使用配置项阻断；新增 `qm_hud_island_show_team` 不在失败列表，其他 quick 子检查通过 |

Note: `qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target run_cxx_tests -j 10` 在当前 build tree 返回 `unknown target 'run_cxx_tests'`，因此直接运行实际存在的 `testrunner.exe`。

## Files Changed

- `feature_list.json`
- `progress.md`
- `session-handoff.md`
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

## Blockers / Risks

- 006~009 当前只有构建证据，仍需用户实机视觉验收。
- 009 涉及多个上游 DDNet UI 文件，PR 描述需要显式声明扩域；本次只改绘制外观，不改网络消息、投票命令、demo 回放、聊天存储或 HUD 数据逻辑。
- `bash init.sh` 仍受本机 WSL vhdx 缺失影响，不能作为当前机器的可用入口；PowerShell 下的 Python harness check 和 Windows CMake wrapper 可用。
- `feat-011` 仍需要实机验证：切片视频首帧、快速练习连接 dummy/reset 后武器、灵动岛各显示状态、entity_bg 新旧资源 fallback、视频播放 CPU/FPS 和 `/tofinish` 目标选择。
- Quick gate 现可经 Git Bash 执行，但会被当前仓库既有 33 个未使用配置项阻断；该失败与 `feat-011` 新增配置无关。

## Next Session Startup

1. Read `AGENTS.md` and focused `.ai/` docs.
2. Run `python qmclient_scripts\gate\check_workflow_docs.py`.
3. Build if needed: `qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10`.
4. Launch `cmake-build-release\DDNet.exe` and visually verify:
   - settings page and controls blocks,
   - demo browser/player controls,
   - HUD and scoreboard surfaces,
   - chat/MOTD/emoticon/vote/infomessages overlays.
   - feat-011 demo cut render、practice dummy/weapon、dynamic island team、entity background/video 与 `/tofinish`。

## Recommended Next Step

Ask the user for visual confirmation of 006~009 and feat-011 runtime scenarios, then prepare commit notes grouped as `FEAT` / `FIX` / `DEL`.
