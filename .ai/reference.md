# QmClient 参考手册

本文件合并了工作流路由、提交验证、PR 审查、文档维护和发布说明等内容。

## 任务路由

### 推荐读取顺序

1. `AGENTS.md`（包含文档入口和启动约束）
2. `.ai/reference.md`（本文件）

### 核心入口分类

#### 启动约束

- `AGENTS.md` 的"文档入口"节
- 本文件的"文档维护原则"节

#### 仓库脚本

- `qmclient_scripts/gate/check-gate.sh`：仓库级门禁总入口
- `qmclient_scripts/gate/check_workflow_docs.py`：文档一致性检查
- `qmclient_scripts/gate/strict-debug-check.sh`：严格调试检查
- `qmclient_scripts/generate_release_notes.py`：发布说明生成

需要查脚本分层和使用建议时，看 `qmclient_scripts/脚本总览.md`。

### 目录使用原则

- `AGENTS.md`：根规则
- `.ai/reference.md`：详细参考（本文件）
- `qmclient_scripts/`：实际执行脚本

### 维护规则

- 新增长期约束时，先补最合适的文档层，再补脚本引用。
- 如果某条引用已经废弃，先更新入口文档，再清理旧引用。

## 提交前验证

### 适用

在合并、发 PR、或标记任务完成前使用。

### 最小要求

- 相关构建通过
- 相关测试通过
- 如果改动影响脚本或约束文档，先确认入口没有断链
- 如果改动涉及 `AGENTS.md` / `Claude.md` 镜像，先跑 `qmclient_scripts/gate/sync_agents_claude.py` 再跑 `qmclient_scripts/gate/check_workflow_docs.py`

### 与 check-gate.sh 的对应

| 提交验证项 | check-gate.sh 模式 |
|------------|-------------------|
| 构建通过 | `default`（含 strict-debug-check） |
| 测试通过 | `default`（含 run_cxx_tests） |
| 源码卫生 | `quick`（配置变量、头文件 guard、style） |
| 文档一致性 | `quick`（check_workflow_docs） |
| Rust 测试 | `full`（run_rust_tests） |

日常提交前跑 `--mode default --base-ref main` 即可覆盖大部分验证项。

## PR 审查清单

### 适用场景

- 提交前 review
- 合并前 review
- 变更收口验收

### 关注点

- 行为是否符合需求
- 验证是否覆盖关键路径
- 是否引入不必要的范围扩大
- 是否留下未解释的风险

PR 模板参见 `.github/pull_request_template.md`。

## 文档维护原则

### 分层

- `AGENTS.md` 放根规则
- 本文件（`reference.md`）放详细参考
- `Claude.md` 只做 `AGENTS.md` 的镜像，优先用脚本同步，不手工双改

### 维护原则

- 新约束先补最合适的层，不要堆在一个文件里
- 删除旧引用前，先确认有没有新入口接住
- 文档与脚本要一起检查，避免只修一边
- 只要 `AGENTS.md` 或 `Claude.md` 任意一边改动，先用 `qmclient_scripts/gate/sync_agents_claude.py` 追平，再跑 `qmclient_scripts/gate/check_workflow_docs.py`

## 发布说明模板

使用 `qmclient_scripts/generate_release_notes.py` 可从 CodeStable 产物自动生成初稿。

### 字段说明

| 字段 | 来源 | 说明 |
|------|------|------|
| 本次重点 | `*-acceptance.md` | feature 验收结论 |
| 新增 | `*-acceptance.md` | 新功能列表 |
| 修复 | `*-fix-note.md` | bug 修复列表 |
| 调整 | `*-acceptance.md` | 行为调整 |
| 兼容性 | 人工补充 | 协议/格式/行为兼容性影响 |
| Maintainer Notes | 人工补充 | 给维护者的额外说明 |

### 用法

```bash
python qmclient_scripts/generate_release_notes.py --gate-report tmp/check-gate-report.json
```
