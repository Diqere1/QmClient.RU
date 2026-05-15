# QmClient Workflow Entry

## 目标

本文件只做一件事：把任务类型路由到正确的文档、脚本和产物目录。

不要把它继续写成长手册。每类工作流只保留：

- 适用场景
- 先读什么
- 主入口
- 产物目录

## 推荐读取顺序

### 任何非 trivial 仓库任务

1. `AGENTS.md`
2. `.codestable/attention.md`
3. 本文件

### 涉及 RmlUI / 崩溃 / 生命周期 / 渲染链

1. `AGENTS.md`
2. `.codestable/attention.md`
3. `.codestable/compound/2026-05-09-learning-windows-debug-symbol-workflow.md`
4. `.codestable/compound/2026-05-10-decision-render-lifecycle-safety-constraints.md`
5. 本文件里对应的调试、日志、迁移分节

## 核心入口分类

### 1. 标准调试流程

适用：

- 启动期崩溃
- 交互期崩溃
- 黑屏
- RmlUI 生命周期问题
- release only 问题

先读：

- `.codestable/compound/2026-05-09-learning-windows-debug-symbol-workflow.md`
- `.codestable/compound/2026-05-10-decision-render-lifecycle-safety-constraints.md`

主入口：

- `qmclient_scripts/strict-debug-check.ps1`

专项参考：

- `.codestable/reference/rmlui-runtime-api-reference.md`
- `.codestable/reference/rmlui-settings-host-contract.md`
- `.codestable/reference/rmlui-test-strategy.md`

### 2. 日志分类

适用：

- 想快速判断失败属于环境、仓库债务还是当前改动
- 想统一看 gate / debug / fallback 输出口径

先读：

- `.codestable/reference/log-classification.md`

主入口：

- `qmclient_scripts/check-gate.ps1`
- `qmclient_scripts/check-gate-workflow.md`
- `.github/workflows/governance.yml`

专项参考：

- `.codestable/reference/log-classification.md`
- `.codestable/reference/workflow-manifest.json`
- `qmclient_scripts/baseline_debt_allowlist.json`
- `qmclient_scripts/refresh_baseline_debt_allowlist.py`

补充约束：

- `baseline_debt_allowlist.json` 缺失时，总入口应按空 allowlist 继续，并允许后续 bootstrap
- `refresh_baseline_debt_allowlist.py` 默认只增量合并新增条目；全量重写必须显式传 `--rewrite`
- 需要让本地 gate 与 CI 使用同一套规则时，优先复用 `.github/workflows/governance.yml`，不要在 CI 里另起一套 ad hoc 命令

### 3. 对照清单进行 PR 审查

适用：

- 提交前 review
- PR 合并前 review
- 功能完成后的对照验收

先读：

- `.codestable/reference/pr-review-checklist.md`
- `.codestable/reference/pre-merge-verification.md`
- `.codestable/reference/shared-conventions.md`

主入口：

- `.codestable/reference/pr-review-checklist.md`

产物目录：

- `.codestable/audits/`
- `.codestable/features/*/*-acceptance.md`
- `.codestable/issues/*/*-fix-note.md`

### 4. 迁移规划

适用：

- UI 框架迁移
- 模块逐步替换
- 共存期策略设计
- 大功能拆分

先读：

- `.codestable/architecture/ARCHITECTURE.md`
- `.codestable/architecture/ui-rmlui-current.md`
- `.codestable/requirements/rmlui-full-replacement.md`
- `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-roadmap.md`

主入口：

- `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-roadmap.md`

产物目录：

- `.codestable/roadmap/`
- `.codestable/features/`
- `.codestable/issues/`

### 5. 发布说明撰写

适用：

- 准备 release / tag
- 汇总一批 feature / fix / breaking note

先读：

- `.codestable/reference/release-notes-template.md`
- `.codestable/reference/pre-merge-verification.md`

主入口：

- `.codestable/reference/release-notes-template.md`
- `qmclient_scripts/generate_release_notes.py`

素材目录：

- `.codestable/features/`
- `.codestable/issues/`
- `qmclient_scripts/check-gate.ps1` / `strict-debug-check.ps1` 的最终验证结果
- `tmp/check-gate-report.json` 一类由 `-ReportJsonPath` 产出的 JSON 报告

补充约束：

- `generate_release_notes.py` 默认只纳入正式状态条目
- 非正式状态条目应进入“待人工确认”分组，而不是混进主发布内容

### 6. 遥测或事件摘要

适用：

- 想沉淀 UI 切换、fallback、safe mode、backend 获取失败等事件
- 想把“出了问题”变成“哪个事件在哪个阶段失败”

先读：

- `.codestable/reference/event-summary-schema.md`
- `.codestable/reference/log-classification.md`

主入口：

- `.codestable/reference/event-summary-schema.md`

优先沉淀：

- UI 页面切换
- RmlUI backend/context acquire 结果
- safe mode 触发
- fallback / skip / defer reason
- settings host 激活/退出

当前统一发射点：

- `src/engine/client/rmlui_event_log.h`

## 目录使用原则

- `AGENTS.md`
  - 只放根规则、强约束、固定入口
- `.codestable/attention.md`
  - 只放启动必读短约束和高频坑
- `.codestable/reference/`
  - 放长期稳定的导航、参考、契约、策略
- `.codestable/features/`
  - 放单个 feature 的 design / checklist / acceptance
- `.codestable/issues/`
  - 放单个问题的 report / analysis / fix-note
- `.codestable/roadmap/`
  - 放跨 feature 的迁移或系统级规划
- `.codestable/audits/`
  - 放批量审查发现
- `.codestable/compound/`
  - 放需要跨多个阶段复用的经验、决策、复合分析

## 维护规则

- 新增长期工作流文档时，优先挂到本文件，而不是只在对话里口头提一次。
- 如果 `AGENTS.md` 里提到了某个必须读流程，最好同时在本文件增加导航。
- 如果某条文档已经废弃或被替代，先在本文件更新入口，再去清理旧引用。
- `AGENTS.md` / `attention.md` / `workflow-entry.md` 的分层维护原则，统一看 `.codestable/reference/agents-rules-maintenance.md`。
