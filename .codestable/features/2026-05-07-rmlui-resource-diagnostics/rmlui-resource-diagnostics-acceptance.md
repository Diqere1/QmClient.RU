---
doc_type: feature-acceptance
feature: 2026-05-07-rmlui-resource-diagnostics
status: pass
created: 2026-05-07
tags: [rmlui, diagnostics, acceptance, resources]
related_design: rmlui-resource-diagnostics-design.md
related_checklist: rmlui-resource-diagnostics-checklist.yaml
---

# rmlui-resource-diagnostics 验收报告

> 阶段：阶段 3（验收闭环）
> 验收日期：2026-05-07
> 关联方案 doc：`features/2026-05-07-rmlui-resource-diagnostics/rmlui-resource-diagnostics-design.md`

## 1. 接口契约核对

- [x] `SRmlUiDiagnostics` 已扩展 resource 字段，覆盖 `resource_type`、`resource_path`、`operation`、`status`、`error_code`、`error_text` 和 `timestamp_local`。
- [x] `CRmlUiRuntime::SetResourceDiagnostics(...)` 已落地，用于承接资源失败分类。
- [x] `CRmlUiRuntime::ShouldExportDiagnostics(...)` 已落地，用于控制开发环境导出门禁。
- [x] 导出目标已固定为 `dumps/QmClient_Crash/`，不再污染仓库根目录 `log/`。

## 2. 行为与决策核对

- [x] 缺失文档、缺失字体和 RCSS 警告可被归入结构化 resource diagnostics。
- [x] 资源 diagnostics 与 runtime/backend diagnostics 保持分离，不覆盖原有失败字段。
- [x] 重复失败具备去重导出约束，不会按帧持续落盘。
- [x] 开发诊断未开启时不导出文件。

## 3. 验收场景核对

- [x] `src/test/rmlui_runtime_test.cpp` 覆盖 `ResourceDiagnosticsFieldsAreStored`。
- [x] `src/test/rmlui_runtime_test.cpp` 覆盖 `ResourceDiagnosticsSurviveExportAndCanBeObservedByHost`。
- [x] 证据命令：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10`
- [x] 测试结果：718 个 C++ 测试通过。

## 4. 结论

`rmlui-resource-diagnostics` 已完成最小闭环：资源失败可结构化记录、可按开发诊断导出、可与 runtime 诊断并存，且不会覆盖旧 UI fallback 路径。
