# Git / PR 规范

这份文档只定义提交信息、PR 描述和最终汇报的稳定格式。

## Commit 规范

提交信息使用：

```text
<type>(<scope>): <中文简述>
```

规则：

- `type` 用英文小写：`feat`、`fix`、`perf`、`refactor`、`docs`、`test`、`chore`、`ci`、`revert`
- `scope` 用短英文或仓库内模块名：如 `ui`、`skins`、`gate`、`docs`、`hud`
- 冒号后面用中文简述
- 不写空话，如“修改代码”“更新一下”

示例：

```text
fix(skins): 修正本地换肤动画立即触发链路
perf(settings): 收紧资源页预览缓存失效范围
docs(ai): 精简 agent 文档体系并补充 git 规范
```

## Commit Body

只有在一句标题说不清时才写 body。

建议写：

- 改动原因
- 关键做法
- 影响范围

不要把验证日志整段贴进 commit body。

## PR 标题

PR 标题默认与最终 squash commit 保持同一风格：

```text
<type>(<scope>): <中文简述>
```

## PR 描述

PR 描述保持短而完整，至少包含：

### 1. Summary

写这次改了什么。

### 2. Verification

按下面格式列验证证据：

```text
- Command: <命令>
  Result: <PASS/FAIL 与关键结果>
  Scope: <证明了什么>
```

### 3. Risks / Gaps

明确还没验证的部分，尤其是：

- 视觉验收
- 运行时行为
- 上游兼容性风险

## Release 说明

GitHub release 说明由 `qmclient_scripts/generate_release_notes.py` 统一生成。

默认来源是 tag 区间内的 commit subject；为了让 release 说明更稳定，commit body 可以额外提供：

```text
Release-ZH: 中文发布说明
Release-EN: English release note
```

规则：

- `Release-ZH` / `Release-EN` 都是可选项
- 如果缺失，脚本会回退到 commit subject 的 description
- 真正面向用户的重要功能或修复，优先补这两个字段，避免 release 页只看到生硬的 commit 标题

## 版本 / Tag / Release

- 仓库内版本统一通过 `python qmclient_scripts/bump_version.py --version X.Y.Z` 或 `--tag vX.Y.Z` 更新
- 不要在 workflow 或本地脚本里直接 `sed version.h`
- tag 构建时，CI 也应调用同一个 `bump_version.py`，保证 `src/game/version.h` 和 `docs/info.json` 同步
- `CLIENT_RELEASE_VERSION` 的源头是 `QMCLIENT_VERSION`；如果版本口径变了，先改这里，再看文档与 release 流程

## 最终汇报格式

用户确认后，最终汇报里的提交说明分三组：

- `FEAT`
- `FIX`
- `DEL`

这三组是给用户看的归纳，不替代真实 git commit message。


# 最终汇报示例（不是 commit message）

```
FEAT: 添加显示丢包率功能并优化投票设置

## DEL
- 取消丢包率显示对 `cl_showpred` 的依赖，现在预测时间和丢包率可以分开控制。
- 移除 TC 设置里“只能自动拒绝换图”的单一模式限制。
## FEAT
- 新增独立开关 `cl_show_packet_loss`，用于单独控制游戏内丢包率显示。
- TC 界面的自动换图投票改为三段模式：`Off / Agree / Reject`。
- `tc_auto_vote_when_far` 扩展为模式配置：0 关闭，1 自动拒绝，2 自动同意
- 赞助名单新增
- 版本号更新至 2.61.16。
## FIX
- 修复举报扫描请求在 `HTTP` 失败或取消时读取状态码导致断言的问题。
- 修复举报功能可在 `Axiom` 服务器内使用的问题：入口按钮会禁用，逻辑层也会拦截。
- 修复丢包率独立显示后与右上角消息、冻结 `HUD` 布局避让不一致的问题。
- 引入 `CBackgroundParticles` 组件，用于管理和渲染背景粒子。
- 实现了粒子生成、更新和渲染逻辑，并支持自定义大小、速度和颜色等属性。
- 在设置菜单中添加了粒子数量、深度和光晕效果的配置选项。
- 增强了队伍切换功能，支持在快速练习模式下使用观战命令。
- 更新了皮肤随机生成机制，支持根据所选皮肤使用自定义颜色。
- 改进了移动方块的逻辑，使其能够识别水域图像以实现动态交互。
- 重构了现有代码，以提高可读性和可维护性。
- 修正部分 UI 控件 ID，避免滑条和数值输入冲突。 ）
```
