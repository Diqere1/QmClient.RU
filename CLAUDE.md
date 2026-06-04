# AGENTS.md / CLAUDE.md

QmClient 是一个定制化的 DDNet/TaterClient 分支。这个文件是仓库的智能体导航图，不是完整手册。请保持简短，并且只在任务需要时再读取下方对应的聚焦文档。

## 入口原则

- 仓库就是记录系统：决策、计划、功能状态、验证证据和交接说明都应该写进版本化文件。
- 这个文件是导航图，不是手册。不要在这里追加冗长的任务历史或一次性修复说明；把长期有效的信息放进精简的 `docs/ai-workflow/` 规则文档，把任务内容放进 `docs/superpowers/specs/` 或 `docs/superpowers/plans/`。
- 一次只处理一个功能。直接以当前用户请求和对应的 superpowers plan/spec 作为范围边界。
- 如果功能请求有歧义，先问清行为、范围和兼容性边界，再开始实现。
- 改行为之前先读真实代码。优先遵循本地模式和 DDNet 兼容性，而不是套用泛化的现代 C++ 偏好。
- 如果有 图谱代码类 MCP（如 codegraph），优先使用它获取代码上下文，而不是直接阅读真实代码。

## 路径约定

- 仓库文档中的文件和目录路径统一使用前斜杠 `/`（如 `src/game/client/components/qmclient/`）。
- 命令行示例中的路径同样使用前斜杠（如 `qmclient_scripts/cmake-windows.cmd`）。
- 即使是 Windows 本地路径，文档中也优先用 `/`，更易跨平台阅读和编辑。
- 此约定适用于所有 `docs/` 下的 `.md` 文件、`AGENTS.md`、`CLAUDE.md` 及其他仓库内文档。

## 文档状态约定

- `docs/superpowers/` 下的探索/计划/规格会随时间推移而老化；已标注 `文档已过时` 或 `部分内容已过时` banner 的文件仅供参考，不应作为实现依据。
- 无标注或标注为当前有效的文档才是本次开发的权威来源。
- 有效文档的判定以最新日期和 `status` 字段（如 `active`、`draft`）为准。

## 启动顺序

1. 先读这个文件。
2. 再读 `docs/superpowers/plans/` 和 `docs/superpowers/specs/` 里与当前任务匹配的计划或规格。
3. 再读与当前任务匹配的最小 `docs/ai-workflow/` 规则。
4. 如果这轮涉及文档/入口/gate，先看 `check_docs.py` 是否也要同步修改。
5. 读取与任务匹配的聚焦 `docs/ai-workflow/` 文档。
6. 修改前检查附近源码、调用点、配置变量、翻译和测试。

## 文档地图

| 路径 | 何时阅读 |
|------|--------------|
| `docs/ai-workflow/meta.md` | 读取 `docs/ai-workflow/` 自身的边界：什么该放这里，什么不该放这里。 |
| `docs/ai-workflow/ddnet-development.md` | 读取 DDNet/QmClient 的 C++ 规则、兼容性约束、风格、所有权、性能和风险边界。 |
| `docs/ai-workflow/verification.md` | 读取构建、测试、quick/default/full gate 命令、视觉检查和证据标准。 |
| `docs/ai-workflow/review.md` | 读取代码审查立场、严重级别格式、DDNet 特有风险点和输出格式。 |
| `docs/ai-workflow/git-workflow.md` | 读取 commit、PR 标题/描述和最终汇报格式规范。 |
| `qmclient_scripts/scripts_overview.md` | 读取脚本分层、推荐入口和 gate 工作流语义。 |

## 全局硬约束

- 保护 DDNet 兼容性：没有明确批准，不要改协议、demo/skin 格式、物理、预测、碰撞、地图行为、rank 可达性或既有玩法语义
- 补丁必须聚焦。不要重写无关的上游 DDNet 代码，也不要为小改动引入大抽象
- QmClient 特有工作通常应落在 `src/game/client/components/qmclient/`、`src/game/client/QmUi/`、QmClient 配置头、翻译、文档、metadata 和 `qmclient_scripts/`
- 超出范围的区域需要明确批准：上游引擎核心、服务端玩法、地图编辑器、第三方库、CI release 工作流、协议字段、物理、预测、snapshot、输入、碰撞、时序和回放语义
- 完成一个完整功能或改进后，除非用户明确把任务限制为调查或纯文本输出，否则按 MMP 规则更新 QmClient 版本
- 在实现完成并得到用户确认后，提交说明按 `FEAT`、`FIX`、`DEL` 三组归纳

## 机械化入口
- 优先用脚本，不要依赖记忆。具体命令见 `docs/ai-workflow/verification.md` 和 `qmclient_scripts/scripts_overview.md`。
- 修改 `AGENTS.md`、`CLAUDE.md`、`docs/ai-workflow/`、workflow 脚本或 governance CI 后，运行 `python qmclient_scripts/gate/check_docs.py --sync-only --prefer agents`，再运行 `python qmclient_scripts/gate/check_docs.py`

## 构建

- Windows 上默认用 `qmclient_scripts/cmake-windows.cmd` 作为构建入口；常规构建/测试目录是 `cmake-build-release`，完整构建命令：`qmclient_scripts/cmake-windows.cmd --build cmake-build-release --target game-client -j 10`。不要继续使用旧的 `build-ninja` 口径；只有已确认当前 shell 已注入可用的 VS/MSVC 环境时，才直接使用裸 `cmake`。

## 极简工作流

### 完成任务后

- 先按 `docs/ai-workflow/verification.md` 跑对应验证，至少覆盖当前改动的 build/test/gate。
- 影响核心逻辑时，必须派发一个新的只读子代理，按 `docs/ai-workflow/review.md` 做代码审查。
- 子代理指出的问题修完后，再看这次改动能否最小化提交：只保留和当前任务直接相关的文件与说明。

### 提交 commit / PR 前

- 先跑 `python qmclient_scripts/gate/check_docs.py`。
- commit 和 PR 文案按 `docs/ai-workflow/git-workflow.md` 编写。
- 如果准备提 PR，先确保这轮审查结论已经收口，不要带着已知 review finding 进入 PR。

### 修改文档后

- 先判断这次文档改动是否改变了入口或规范；如果改变了，先同步修改 `check_docs.py` 或其约束，再跑文档检查
- 最后运行 `python qmclient_scripts/gate/check_docs.py`，确认没有断链或镜像漂移。

## 十二原则：软件工程
除非另有明确说明，本项目中的所有任务都遵循以下规则。
基本倾向：遇到非简单任务时，宁可慢一点，也要更谨慎。简单任务则自行判断，不必过度流程化。
## 规则 1 — 写代码前先想清楚
明确说出你的假设。
不确定时先问，不要猜。
如果需求有歧义，列出可能的理解。
如果有更简单的做法，要主动指出。
如果卡住了，就停下来，说明哪里不清楚。
## 规则 2 — 简单优先
用能解决问题的最少代码。不要做预判式扩展。
不要实现需求之外的功能。不要为了只用一次的代码引入抽象。
自检标准：如果一位资深工程师会觉得这太复杂，那就简化。
## 规则 3 — 精准修改
只改必须改的地方。只清理你自己造成的问题。
不要顺手“优化”旁边的代码、注释或格式。
没坏的东西不要重构。保持和现有代码风格一致。
## 规则 4 — 以目标为导向
先定义成功标准，再不断验证，直到达成。
不要只是机械地执行步骤。要明确什么叫完成，并围绕它迭代。
清晰的成功标准能让你独立推进，而不是不断迷失在流程里。
## 规则 5 — 只把模型用于判断类工作
可以用我来做：分类、起草、总结、提取。
不要用我来做：路由、重试、确定性转换。
如果代码能给出答案，就让代码来回答。
## 规则 6 — Token 预算必须认真对待
单个任务：4,000 tokens。单个会话：30,000 tokens。
如果快接近预算，就先总结，再重新开始。
预算可能超限时要明确说出来，不要悄悄超支。
## 规则 7 — 暴露冲突，不要折中混合
如果两种模式互相冲突，选择其中一种，优先选更新的或验证更多的。
说明你为什么这样选，并把另一种标记为后续需要清理。
不要把冲突的做法混在一起。
## 规则 8 — 写之前先读
加代码之前，先看导出项、直接调用方和共享工具。
“看起来互不相关”并不安全。
如果不明白代码为什么这样组织，就先问。
## 规则 9 — 测试要验证意图，而不只是行为
测试应该体现这个行为为什么重要，而不只是检查它做了什么。
如果业务逻辑变了，测试却不会失败，那这个测试就是错的。
## 规则 10 — 重要步骤后要设检查点
每完成一个重要步骤，都总结：做了什么、验证了什么、还剩什么。
不要在一个自己都说不清的状态上继续往前走。
如果发现自己丢了上下文，就停下来，重新整理当前状态。
## 规则 11 — 遵循代码库约定，即使你不认同
在代码库内部，一致性优先于个人偏好。
如果你确实认为某个约定有问题，要明确指出。不要偷偷另起一套风格。
## 规则 12 — 失败要说清楚
如果有任何内容被跳过，就不能说“已完成”。
如果有任何测试被跳过，就不能说“测试通过”。
默认暴露不确定性，而不是把它藏起来。