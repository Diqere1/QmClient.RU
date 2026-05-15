---
doc_type: audit-finding
title: check-gate 仍把 allowlist 文件缺失当成硬前置，破坏 baseline 体系 bootstrap
severity: P1
category: bug
confidence: high
suggested_action: cs-issue
created_at: 2026-05-12
---

# Finding 01

## Summary

`check-gate.ps1` 已经实现了“allowlist 缺失时按空表处理”的加载逻辑，但环境前置检查仍把 `baseline_debt_allowlist.json` 当成必需文件硬拦截。结果是：一旦文件丢失、重命名，或者新分支还没 bootstrap allowlist，总入口会先在 preflight 直接失败，后面的 refresh 流程根本无法自举。

## Evidence

- `qmclient_scripts/check-gate.ps1:102-118`
  - `Load-BaselineDebtAllowlist` 已明确支持文件不存在时回退为空 allowlist。
- `qmclient_scripts/check-gate.ps1:907-915`
  - `Assert-WorkingTreePreflight` 又对 `qmclient_scripts/baseline_debt_allowlist.json` 执行 `Test-RequiredPath`，把它重新升级成硬依赖。

## Why It Matters

- 这会让“首次建立 baseline debt 记录”这件事本身无法通过总入口完成。
- 生产级 gate 不应要求操作者先手工补一个哑文件，才能进入自动刷新流程。
- 这类 bootstrap 自相矛盾会让 AI 和人工都误以为“脚本坏了”或“环境不完整”，而不是稳定地引导到初始化步骤。

## Suggested Fix

- 移除 `baseline_debt_allowlist.json` 的 preflight 硬依赖，统一以 `Load-BaselineDebtAllowlist` 的空表回退逻辑为准。
- 如果文件不存在，显式记录一条 `WARN` 或 `INFO`，提示可用 `refresh_baseline_debt_allowlist.py` 初始化。
- 最好再补一个最小测试或至少一次文档化验证，覆盖“allowlist 缺失但 quick 仍可生成 report 并完成 bootstrap”。
