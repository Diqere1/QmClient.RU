# RmlUI Monitoring HUD 迁移验收报告

> 阶段：阶段 3（验收闭环）
> 验收日期：2026-05-08
> 关联方案 doc：`.codestable/features/2026-05-08-rmlui-monitoring-hud-migration/rmlui-monitoring-hud-migration-design.md`

## 1. 接口契约核对

**接口示例逐项核对**

- [x] `src/game/client/RmlUi/RmlUiMonitoringHud.h` 的 `SSurfaceContract`：文档成功、主图矩形成功、副图矩形成功、图表成功和失败阶段/原因都已落到结构体字段；`src/test/rmlui_monitoring_hud_test.cpp` 覆盖了默认值、阶段映射和原因映射。

**名词层“现状 → 变化”逐项核对**

- [x] “图表矩形契约” 已从内部瞬时结果提升为显式 surface contract：`CRmlUiMonitoringHud::RenderDocument(...)` 会更新 `m_DocumentReady`、`m_MainGraphRectReady`、`m_FpsGraphRectReady` 与 `m_FailureStage`。
- [x] “静态界面文案统一归口” 已落地：`data/qmclient/rmlui/monitoring_hud.rml` 只保留中性占位和 `RmlUI` marker，显示文本统一由 `UpdateDocument(...)` 在运行时填充。
- [x] “图表契约失败与文档失败分层诊断” 已落地：`src/game/client/gameclient.cpp` 现在复用 `CRmlUiMonitoringHud::FailureReason(...)` / `FailureStageName(...)`，不再把所有失败压成同一个 surface reason。

**流程图核对**

- [x] `CGameClient::RenderQmMonitoringHud` → `CRmlUiLayerSwitchboard::Dispatch(GAME_HUD, monitoring_hud)` → `CRmlUiRuntime::RenderRmlUiLayer(...)` → `RenderRmlUiMonitoringModule(...)` → backend-thread `RenderDocument(...)` → 宿主 `DrawGraphs(...)` 的节点关系都能在 `src/game/client/gameclient.cpp` 与 `src/game/client/RmlUi/RmlUiMonitoringHud.cpp` 对上。

## 2. 行为与决策核对

**需求摘要逐项验证**

- [x] Monitoring HUD 继续经过 `GAME_HUD` → switchboard → runtime → module render 进入 RmlUI 路径，没有回退到宿主直连 runtime。
- [x] RmlUI 版本的摘要区、图例、卡片、主图和 FPS/游戏时间区都已有独立文档锚点和运行时本地化填充，不再依赖试点期硬编码文本。
- [x] 图表矩形契约失效时会稳定失败并回到旧 HUD，不再把“壳层出来了但图表无效”误记成成功。
- [x] UI 上保留了显式 `RmlUI` 标识：`summary-chip` 固定显示 `RmlUI`，`summary-kicker` 运行时显示 `RmlUI Monitoring HUD` 的本地化文本。

**明确不做逐项核对**

- [x] 未实现输入桥：本功能没有新增 input dispatch、focus、cancel 或 text route 代码。
- [x] 未迁移调试 HUD、菜单页或弹窗：debug/menu/popup 仍只停留在 switchboard host slot，未新增 concrete RmlUI surface。
- [x] 未把图表折线改成纯 RmlUI 几何/着色器：图表依旧由宿主 `DrawGraphs(...)` 走旧 `IGraphics` 路径叠加。
- [x] 未改变 `SQmMonitoringViewModel` 的指标来源、采样策略和计算语义：本轮只改 surface contract、本地化和 fallback 语义。
- [x] 未移除旧 Monitoring HUD：`CGameClient::RenderQmMonitoringHud` 在 RmlUI 关闭或失败时仍直接调用 `m_QmMonitoring.RenderHud(...)`。

**关键决策落地**

- [x] 决策“继续采用混合渲染迁移”已落地：RmlUI 负责文档壳层与布局，旧 `IGraphics` 负责折线和网格。
- [x] 决策“迁移目标是内容完整性 + 回退稳定性，不重做宿主调度”已落地：本轮没有重改 switchboard，只在 Monitoring HUD surface 内补契约和文案。
- [x] 决策“显式 RmlUI 标识属于界面契约”已落地：资源测试要求保留 `RmlUI` marker，运行时标题明确标明这是 RmlUI HUD。

**编排层“现状 → 变化”逐项核对**

- [x] 文档更新、矩形解析和图表绘制的三层阶段，现在可通过 `SurfaceContract()` 和 `LastFailure()` 分别观察。
- [x] 失败诊断从“单一失败 bool”收紧为“文档 / 矩形 / 图表”三层失败语义。
- [x] 文档锚点和资源测试现在双向对齐，不再出现 `ValidateDocumentStructure()` 已要求但资源测试未覆盖的 id 漂移。

**流程级约束核对**

- [x] `GAME_HUD` 宿主仍通过 switchboard dispatch 进入 runtime，没有旁路。
- [x] fallback owner 仍在原宿主：`RenderQmMonitoringHud` 在 RmlUI 未渲染成功时统一执行旧 HUD 和 fallback notice。
- [x] Monitoring HUD 走 RmlUI 版本时没有引入 gameplay 输入消费逻辑，也不依赖 input bridge。

**挂载点反向核对**

- [x] `src/game/client/RmlUi/RmlUiMonitoringHud.*`、`src/game/client/gameclient.*`、`data/qmclient/rmlui/monitoring_hud.rml`、`src/test/rmlui_monitoring_*` 仍是本 feature 的核心挂载点。
- [x] 反向 grep 后，本轮新增命中的关键代码仍收敛在上述挂载点与语言表文件，没有出现方案外的宿主级扩散。
- [x] 拔除沙盘推演：如果移除 `CRmlUiMonitoringHud` / `monitoring_hud.rml` / `RenderQmMonitoringHudRmlUi(...)` 这条链，Monitoring HUD 会回到旧 HUD；不会残留第二条并行 runtime render owner。

## 3. 验收场景核对

- [x] **S1：开启 `qm_rmlui_enable=1`、`qm_rmlui_monitoring_hud=1` 且 runtime 正常**
  - 证据来源：人工验收记录 + 手工运行日志 + 当前构建
  - 结果：通过。用户在 2026-05-08 的手工运行日志中已拿到 `backend init success`、`core init success`、`hud init`、`document load success`；同一轮对话中用户明确确认“游戏画面和 RmlUI 的画面可以并行”。本轮代码只继续收口文案、本地化和契约，没有改回渲染主路径。

- [x] **S2：关闭 Monitoring HUD RmlUI 模块开关或 runtime 返回非 `RENDERED`**
  - 证据来源：代码核对 + targeted tests
  - 结果：通过。`RenderQmMonitoringHud(...)` 在 `RenderQmMonitoringHudRmlUi(...)` 返回 `false` 时，会先渲染旧 HUD，再叠加 fallback notice；旧 HUD 没有被挪出宿主。

- [x] **S3：文档缺失、结构错误或图表矩形无效**
  - 证据来源：代码核对 + `RmlUiMonitoringHud.*` / `RmlUiMonitoringAssets.*` tests
  - 结果：通过。`DOCUMENT_LOAD_FAILED`、`DOCUMENT_STRUCTURE_INVALID`、`MAIN_GRAPH_RECT_INVALID`、`FPS_GRAPH_RECT_INVALID` 都会落成清晰失败阶段/原因，并回到宿主 fallback。资源测试、surface contract 测试与宿主 fallback 逻辑已经对齐。

- [x] **S4：Monitoring HUD 走 RmlUI 版本时不消费 gameplay 输入**
  - 证据来源：代码核对
  - 结果：通过。本轮没有新增任何 input bridge、focus 或事件消费逻辑；`monitoring_hud` module descriptor 仍声明 `RequiresInput=false`。

- [x] **S5：本功能验收带回构建、测试和人工验收口径**
  - 证据来源：本轮构建 / 测试记录 + 项目经验回写
  - 结果：通过。本轮已补齐 targeted tests、构建证据，并把“UI/游戏内表现最终验收统一由人工检查，不以截图为默认产物”回写到 `.codestable/attention.md`。

## 4. 术语一致性

- `Monitoring HUD 迁移闭环`：代码和文档都围绕 Monitoring HUD 这条 surface 收口，没有混入 `layer-switchboard` 的宿主调度术语。
- `图表矩形契约`：代码统一使用 `main-graph` / `fps-graph` 对应 surface contract，不再出现新的平行名词。
- `混合渲染迁移`：实现层明确保持 “document/core render + host DrawGraphs” 的双段式渲染，没有引入“纯 RmlUI 图表”新概念。
- `显式 RmlUI 标识`：资源与运行时都统一以 `RmlUI` marker 表达，没有再次散落旧试点标题。

## 5. 架构归并

- [x] `.codestable/architecture/ui-rmlui-current.md`：把 Monitoring HUD 从“试点路径”回写成“当前唯一已验收的真实迁移样板”，并补了 surface contract、本地化归口与 fallback owner 的 current state。
- [x] `.codestable/architecture/ARCHITECTURE.md`：把总入口里的 Monitoring HUD 描述从“试点宿主”更新为“当前唯一已验收的 concrete RmlUI module”，同时保留其它 surface 仍未迁移的边界。

## 6. requirement 回写

- [x] `requirement=rmlui-full-replacement` 已实际更新：`implemented_by` 新增 `2026-05-08-rmlui-monitoring-hud-migration`，变更日志补了 Monitoring HUD 已成为第一条已验收迁移样板；整体 requirement 继续保持 `draft`，因为 menu/debug/popup migration 与 input bridge 仍未 current。

## 7. roadmap 回写

- [x] `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-items.yaml`：`rmlui-monitoring-hud-migration` 已从 `in-progress` 改为 `done`。
- [x] `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-roadmap.md`：子 feature 清单里的 Monitoring HUD 迁移条目已同步为 `done`，文案改成“当前第一条已验收混合迁移样板”。
- [x] `.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-readiness-matrix.md`：Monitoring HUD migration 已从 `ready-for-design` 更新为 `done`，后续 debug HUD migration 直接复用这条样板。

## 8. attention.md 候选盘点

- [x] 本 feature 已暴露需要长期复用的工作流约束，并已实际写入 `.codestable/attention.md`：
  - 涉及 UI 或游戏内表现的最终验收统一由人工检查完成，不把截图当作默认验收产物。

## 9. 遗留

- 后续优化点：当前 shell 环境里直接拉起 `build-ninja/DDNet.exe` 会在 storage 阶段后立即退出，无法在本会话里追加自动化窗口级运行取证；后续若要补充开发机自动验收，需要另开专门的本地运行探针/桌面宿主方案。
- 已知限制：Monitoring HUD 仍是当前唯一 concrete RmlUI surface；debug/menu/popup 与 input bridge 不在本次范围内。
- 实现阶段顺手发现：语言表完整性此前没有覆盖新增 RmlUI key，现已通过 `MonitoringLocalizationKeysHaveChineseTranslations` 补上最小护栏。
