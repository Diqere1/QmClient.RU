---
doc_type: refactor-scan
refactor: 2026-05-11-rmlui-settings-doc-structure
status: draft
scope: .codestable/features/2026-05-10-rmlui-settings-reorg/*.md, .codestable/roadmap/rmlui-full-replacement/*.md, .codestable/features/RMLUI_FEATURE_INDEX.md, .codestable/reference/rmlui-runtime-api-reference.md
summary: 这批 settings / roadmap / reference 文档混合了承诺、现状、约束和索引，需要拆成职责更清晰的少数主文档，避免后续实现继续在多处重复改同一条规则。
---

# rmlui-settings-doc-structure scan

## 总览

扫描后我认为值得重构的点有 5 条，分布如下：

- 高风险 2 条：文档职责混写、跨文档约束重复
- 中风险 2 条：roadmap / readiness 过度描述、runtime reference 混入 feature-specific policy
- 低风险 1 条：feature index 叙述性过强

优先建议先做 1、2、3；4、5 可在同一轮顺手收口。

## 清单

### 1. 把 settings-reorg design 从“feature 方案 + 长期 contract + current-state 观察”拆开

- 扫描对象：`.codestable/features/2026-05-10-rmlui-settings-reorg/rmlui-settings-reorg-design.md`
- 现象：这份文档现在同时承担了 feature 目标、长期上下文约束、当前实现观察、验收场景和后续主线引导。
- 为什么值得重构：它已经不像单纯的 feature design，更像“设计稿 + 运行时契约 + 参考说明”的混合体；后续 settings-native-controls / search / visual-refresh 读它时会很难判断哪些是本 feature 的行为，哪些是以后都要遵守的长期规则。
- 建议方向：把长期稳定的 settings-host 约束单独抽成一份 reference / contract 文档，design 只保留本 feature 的目标、编排、验收和执行边界。

### 2. 把“dedicated contexts / no parallel render / no legacy island”收敛成一份 canonical 约束

- 扫描对象：`rmlui-settings-reorg-design.md`、`rmlui-settings-reorg-checklist.yaml`、`rmlui-full-replacement-roadmap.md`、`rmlui-full-replacement-readiness-matrix.md`、`RMLUI_FEATURE_INDEX.md`、`rmlui-runtime-api-reference.md`
- 现象：同一条策略在 6 份文档里反复出现，只是措辞略有不同。
- 为什么值得重构：这会造成明显的维护漂移。只要 settings 主线再调整一次，就得同步改多份文档；而且读者会误以为这是不同层级的不同规则。
- 建议方向：把它收敛到一个最权威的 settings-host contract 文档，其余位置都改成简短引用或链接。

### 3. 把 roadmap / readiness 的正文收回到“阶段、依赖、状态”，别继续重复设计稿

- 扫描对象：`.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-roadmap.md`、`.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-readiness-matrix.md`、`.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-items.yaml`
- 现象：roadmap 和 readiness 里有大量和 design 同义的段落，甚至包含 host swapover、context boundary、state adapter 这类接近实现方案级别的说明。
- 为什么值得重构：roadmap 应该更像“阶段图和依赖图”，不是第二份 design；现在这两层的叙述密度重叠太大，后续一改设计，roadmap 很容易跟着漏改。
- 建议方向：保留 phase / status / dependency / acceptance 的骨架，把细的实现论证尽量移回 design 或 reference。

### 4. 把 runtime reference 里的 settings-specific 说明拆出去

- 扫描对象：`.codestable/reference/rmlui-runtime-api-reference.md`
- 现象：runtime reference 里出现了明显偏 settings 主线的约束描述，比如 dedicated page / modal context、全屏 settings 页的 ownership 说明。
- 为什么值得重构：runtime reference 的职责应该是“RmlUI runtime API 和当前 runtime 边界”，而不是承载某个 feature 的特定实现路线；现在这个文件已经开始把 feature contract 和通用 API 混在一起。
- 建议方向：把 settings-specific 的内容移动到 settings-host contract / architecture note，runtime reference 保持通用。

### 5. 把 feature index 的说明压缩成导航型条目

- 扫描对象：`.codestable/features/RMLUI_FEATURE_INDEX.md`
- 现象：index 现在不仅列链接，还会解释 roadmap 顺序、主线意义和“不要怎样理解”。
- 为什么值得重构：它更像导览页 + 说明页混合体；当文档数量再增加时，这种写法会让索引越来越重，维护成本高。
- 建议方向：保留链接、状态、下游使用方式的最小信息，把较长的主线说明移回 roadmap 或专门的 overview 文档。

## 建议顺序

1. 先做 1 和 2，把“哪份是主约束、哪份是 feature 设计”划清。
2. 再做 3，把 roadmap / readiness 重新压回执行层。
3. 然后做 4 和 5，把 runtime reference 和 feature index 变轻。

## 暂不建议动的部分

- 不建议现在去拆 `menu-pilot` 的历史基线描述，它目前还是 `MENU_PAGE` 的已验收参考。
- 不建议现在动 `rmlui-settings-reorg` 的核心 IA 方向，这次 scan 关注的是文档结构，不是再改功能目标。

