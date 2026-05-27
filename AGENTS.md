# AGENTS.md / CLAUDE.md

QmClient 是一个定制化的 DDNet/TaterClient 分支。这个文件是仓库的智能体导航图，不是完整手册。请保持简短，并且只在任务需要时再读取下方对应的聚焦文档。

## 入口原则

- 仓库就是记录系统：决策、计划、功能状态、验证证据和交接说明都应该写进版本化文件。
- 这个文件是导航图，不是手册。不要在这里追加冗长的任务历史或一次性修复说明；把长期有效的信息放进精简的 `.ai/` 文档，把任务内容放进 `docs/superpowers/specs/` 或 `docs/superpowers/plans/`。
- 一次只处理一个功能。直接以当前用户请求和对应的 superpowers plan/spec 作为范围边界。
- 如果功能请求有歧义，先问清行为、范围和兼容性边界，再开始实现。
- 改行为之前先读真实代码。优先遵循本地模式和 DDNet 兼容性，而不是套用泛化的现代 C++ 偏好。

## 启动顺序

1. 先读这个文件。
2. 再读 `docs/superpowers/plans/` 和 `docs/superpowers/specs/` 里与当前任务匹配的计划或规格。
3. 再读与当前任务匹配的最小 `.ai/` 规则。
4. 如果这轮涉及文档/入口/gate，先看 `check_docs.py` 是否也要同步修改。
5. 读取与任务匹配的聚焦 `.ai/` 文档。
6. 修改前检查附近源码、调用点、配置变量、翻译和测试。

## 文档地图

| 路径 | 何时阅读 |
|------|--------------|
| `.ai/meta.md` | 读取 `.ai/` 自身的边界：什么该放这里，什么不该放这里。 |
| `.ai/ddnet-development.md` | 读取 DDNet/QmClient 的 C++ 规则、兼容性约束、风格、所有权、性能和风险边界。 |
| `.ai/verification.md` | 读取构建、测试、quick/default/full gate 命令、视觉检查和证据标准。 |
| `.ai/review.md` | 读取代码审查立场、严重级别格式、DDNet 特有风险点和输出格式。 |
| `.ai/git-workflow.md` | 读取 commit、PR 标题/描述和最终汇报格式规范。 |
| `qmclient_scripts/scripts_overview.md` | 读取脚本分层、推荐入口和 gate 工作流语义。 |

## 全局硬约束

- 保护 DDNet 兼容性：没有明确批准，不要改协议、demo/skin 格式、物理、预测、碰撞、地图行为、rank 可达性或既有玩法语义。
- 补丁必须聚焦。不要重写无关的上游 DDNet 代码，也不要为小改动引入大抽象。
- QmClient 特有工作通常应落在 `src/game/client/components/qmclient/`、`src/game/client/QmUi/`、QmClient 配置头、翻译、文档、metadata 和 `qmclient_scripts/`。
- 超出范围的区域需要明确批准：上游引擎核心、服务端玩法、地图编辑器、第三方库、CI release 工作流、协议字段、物理、预测、snapshot、输入、碰撞、时序和回放语义。
- Windows 上默认用 `qmclient_scripts\cmake-windows.cmd` 作为构建入口；只有已确认当前 shell 已注入可用的 VS/MSVC 环境时，才直接使用裸 `cmake`。
- 完成一个完整功能或改进后，除非用户明确把任务限制为调查或纯文本输出，否则按 MMP 规则更新 QmClient 版本。
- 在实现完成并得到用户确认后，提交说明按 `FEAT`、`FIX`、`DEL` 三组归纳。

## 机械化入口

优先用脚本，不要依赖记忆：

```bash
python qmclient_scripts/gate/check_docs.py
python qmclient_scripts/gate/check_gate.py --mode quick --base-ref main
python qmclient_scripts/gate/check_gate.py --mode default --base-ref main
```

修改 `AGENTS.md`、`CLAUDE.md`、`.ai/`、workflow 脚本或 governance CI 后，运行：

```bash
python qmclient_scripts/gate/check_docs.py --sync-only --prefer agents
python qmclient_scripts/gate/check_docs.py
```

## 极简工作流

### 完成任务后

- 先按 `.ai/verification.md` 跑对应验证，至少覆盖当前改动的 build/test/gate。
- 影响核心逻辑时，必须派发一个新的只读子代理，按 `.ai/review.md` 做代码审查。
- 子代理指出的问题修完后，再看这次改动能否最小化提交：只保留和当前任务直接相关的文件与说明。

### 提交 commit / PR 前

- 先跑 `python qmclient_scripts/gate/check_docs.py`。
- commit 和 PR 文案按 `.ai/git-workflow.md` 编写。
- 如果准备提 PR，先确保这轮审查结论已经收口，不要带着已知 review finding 进入 PR。

### 修改文档后

- 先判断这次文档改动是否改变了入口或规范；如果改变了，先同步修改 `check_docs.py` 或其约束，再跑文档检查。
- 最后运行 `python qmclient_scripts/gate/check_docs.py`，确认没有断链或镜像漂移。
