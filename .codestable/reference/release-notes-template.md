# Release Notes Template

## 目标

本文件提供 QmClient 仓库统一的发布说明模板，避免每次 release / tag 都临时拼口径。

它同时覆盖两层内容：

- 面向用户的变更摘要
- 面向维护者的验证与风险摘要

## 适用场景

- 准备 tag
- 汇总一批 feature / fix
- 需要写内部发布说明或外部更新公告

## 素材来源

优先从以下产物抽取事实，不要凭记忆写：

- `.codestable/features/*/*-acceptance.md`
- `.codestable/issues/*/*-fix-note.md`
- `.codestable/roadmap/*/`
- `qmclient_scripts/check-gate.ps1` 最终汇总
- `qmclient_scripts/strict-debug-check.ps1` 关键验证结果
- `qmclient_scripts/generate_release_notes.py` 自动生成的初稿
- 必要的人工运行验证记录

## 自动初稿入口

默认先生成一版初稿，再人工改写：

```powershell
python qmclient_scripts\generate_release_notes.py --version vX.Y.Z --gate-report tmp\check-gate-report.json --output tmp\release-notes-vX.Y.Z.md
```

使用约束：

- 该脚本会优先解析 acceptance / fix-note 中的 `status`、`最终结论`、`修复内容`、`遗留` 等小节，自动生成摘要初稿。
- 该脚本默认只把正式状态条目纳入主列表；非正式状态会被列到“待人工确认的非正式产物”。
- 历史产物中的兼容别名状态会先做归一化：例如 `pass -> accepted`、`confirmed -> fixed`，避免因为旧口径漂移把已完成条目误丢到“待人工确认”。
- 主列表中的状态展示应优先输出归一化后的规范状态；如果某条被跳过，才额外保留原始状态供人工追溯。
- 如确需放宽状态白名单，显式传 `--include-status <status>`。
- 该脚本只负责汇总 feature / issue / gate 素材，不负责替你编造结论。
- 没有 `--gate-report` 时，维护者验证摘要会保留待补占位。
- 发布说明最终仍必须人工确认兼容性、已知限制和发布态验证口径。

## 用户向发布说明模板

```md
# QmClient <版本号>

## 本次重点

- [1 句话概述最重要的变化]
- [1 句话概述第二个重要变化]

## 新增

- [用户可感知的新能力]

## 修复

- [用户可感知的问题修复]

## 调整

- [行为、流程或界面调整]

## 兼容性说明

- [是否影响旧配置、旧地图、旧联机行为]

## 已知限制

- [当前仍未解决，但需要提前说明的限制]
```

## 维护者向发布说明模板

```md
# QmClient <版本号> Maintainer Notes

## 变更范围

- Feature:
  - [feature slug / 对应模块]
- Fix:
  - [issue slug / 对应模块]
- Refactor / Workflow:
  - [脚本、文档、流程收口]

## 风险检查

- 协议 / 文件格式兼容性：
  - [未触碰 / 触碰了什么，如何验证]
- 地图 / 物理 / 预测：
  - [未触碰 / 触碰了什么，如何验证]
- 生命周期 / 渲染：
  - [涉及哪些状态机或 fallback]

## 验证摘要

- Gate:
  - [check-gate 模式和结果]
- 构建 / 测试:
  - [构建目标、测试目标、是否人工运行]
- 发布态验证:
  - [是否做过 build-ninja / Release 真实运行]

## 升级或迁移提示

- [是否需要迁移配置、资源、脚本、文档]

## 未纳入本次发布

- [明确被推迟的事项]
```

## 书写规则

- 只写已完成、已验证的事实，不写“计划中”“以后会做”
- 优先写用户能感知到的结果，再写实现细节
- 不把底层日志原样贴进用户向说明
- 如果某项仍有限制，明确写在“已知限制”，不要藏在正文里
- breaking 变化必须显式标出影响范围

## Breaking / 风险提示模板

可直接复用以下句式：

- `本次未改变网络协议、demo、skin 或地图行为兼容性。`
- `本次仅调整客户端侧显示/流程，不改变服务端确定性。`
- `本次发布包含 <模块> 的内部重构；对外行为不应变化，但建议重点回归 <场景>。`
- `本次仍保留 <限制>，尚未纳入正式默认路径。`

## 常见误区

- 只抄 commit message，不回看 acceptance / fix-note
- 把“修复了日志”写成“修复了用户问题”
- 混淆“实验入口”“正式默认入口”
- 不写验证口径，导致维护者无法判断发布可信度

## 与其他文档的边界

- `.codestable/reference/workflow-entry.md`
  - 放导航
- 本文件
  - 放发布说明模板
- `.codestable/reference/pre-merge-verification.md`
  - 放提交前验证动作
- `.codestable/reference/event-summary-schema.md`
  - 放事件摘要结构
