---
doc_type: refactor-apply-notes
refactor: 2026-05-11-rmlui-settings-doc-structure
---

# rmlui-settings-doc-structure apply notes

## 步骤 1: 抽出 canonical settings-host contract
- 完成时间: 2026-05-11
- 改动文件: `.codestable/reference/rmlui-settings-host-contract.md`
- 验证结果: YAML/frontmatter 校验通过
- 偏离: 无

## 步骤 2: 压缩 settings feature / checklist / roadmap / readiness / runtime / index
- 完成时间: 2026-05-11
- 改动文件: `.codestable/features/2026-05-10-rmlui-settings-reorg/rmlui-settings-reorg-design.md`, `.codestable/features/2026-05-10-rmlui-settings-reorg/rmlui-settings-reorg-checklist.yaml`, `.codestable/features/RMLUI_FEATURE_INDEX.md`, `.codestable/reference/rmlui-runtime-api-reference.md`, `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-roadmap.md`, `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-readiness-matrix.md`, `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-items.yaml`
- 验证结果: 相关 frontmatter / YAML 全部通过验证；`settings-host contract` 已成为单一长约束来源
- 偏离: 无

## 步骤 3: 校验引用与格式
- 完成时间: 2026-05-11
- 改动文件: `.codestable/refactors/2026-05-11-rmlui-settings-doc-structure/rmlui-settings-doc-structure-refactor-design.md`, `.codestable/refactors/2026-05-11-rmlui-settings-doc-structure/rmlui-settings-doc-structure-checklist.yaml`
- 验证结果: 相关 YAML 通过 validate-yaml.py
- 偏离: 无
