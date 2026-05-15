---
doc_type: explore
type: module-overview
status: active
slug: rmlui-doc-completeness
created: 2026-05-07
updated: 2026-05-07
tags: [rmlui, documentation, completeness, audit]
supersedes: []
---
# RmlUI 文档完善度全面报告

## 速答

当前 RmlUI 文档体系顶层规划仍然很强，参考层维持 13 份、5 份 feature design 含 4 份 checklist、knowledge 层已有 explore / learn / trick / decide / review。与上版相比，runtime-shell 已完成第一轮工程落地；resource-diagnostics 与 safe-mode 也已进入 accepted baseline；render-command-bridge 则进入“minimal slice implemented, full bridge still pending”状态。`run_cxx_tests` 的当前文档证据已回写到 **725** 个 C++ 测试全绿。总体闭环完善度从最初约 **15-20%** 提升到约 **70%+**，但新的关键断点已经从“完全没有实现”转成 **已落地切片与 CodeStable 回写、ownership 收拢、以及 full-bridge 边界说明是否同步**。

核心结论：

1. **规划层（95%）** — requirements、roadmap、landing notes 三份文档 + 就绪度矩阵（readiness-matrix）已覆盖 19 条子 feature 的完整路线图、6 阶段门禁、接口契约、平台策略和证据要求。这是本体系的最强层。
2. **设计层（26%）** — 5/19 feature 有 design doc：runtime-shell（accepted baseline）、resource-diagnostics（accepted baseline）、safe-mode（accepted baseline）、render-cmd-bridge（approved + minimal slice implemented）、input-bridge（draft + checklist）。剩余 14 条无 design。
3. **架构层（35%）** — `ARCHITECTURE.md` + 新增 `ui-rmlui-current.md` 子文件，两份文档共同覆盖当前 RmlUI scope、system interfaces、render interaction、diagnostics 和 known issues。roadmap 目标模块尚未全部变成已落地架构。
4. **实现层（40%）** — runtime-shell 已不再只是 design：`CRmlUiRuntime` 已落地 module registration、frame request/result、surface render callback、diagnostics dedupe/export gate；safe-mode diagnostics / trip / demotion / reset 已落地；resource diagnostics schema 与 export gate 已落地；Monitoring HUD host 已改为通过 runtime 消费真实 surface result，并通过 graphics-thread callback 执行 RmlUI document render。仍只有 `GAME_HUD` 试点模块，完整 render-command bridge、input bridge 和多 surface 迁移仍未实现。
5. **测试层（40%）** — 已从“只有 1 个资产测试文件”提升到 “资产测试 + runtime/compat + safe-mode/resource diagnostics 单测”。`rmlui_runtime_test.cpp` 现在覆盖 global/module toggle、fallback、surface failure、diagnostics export dedupe、legacy/new toggle compatibility，以及 safe-mode 和 resource diagnostics 关键契约。TDD 已经真正进入执行，但 exported diagnostic file 与 full render bridge 的自动化证据仍未补齐。
6. **参考层（85%）** — 从 3 份跃升至 13 份 libdoc，形成完整的参考体系：
   - 顶层索引：`rmlui-reference-index.md`（指向所有 reference 文件）
   - Upstream API：`rmlui-runtime-api-reference.md`（323行，三层）、`rmlui-render-interface-reference.md`、`rmlui-file-interface-reference.md`、`rmlui-system-input-reference.md`、`rmlui-font-engine-reference.md`
   - QmClient Surface：`rmlui-backend-reference.md`、`rmlui-core-reference.md`、`rmlui-monitoring-hud-reference.md`、`rmlui-render-helpers-reference.md`、`rmlui-gameclient-host-reference.md`
   - Guides：`rmlui-developer-guide.md`、`rmlui-test-strategy.md`
   缺陷：developer-guide 未引用 compound/architecture 文档（审计 F5）。
7. **验收层（10%）** — acceptance-template 已定义 6 个 evidence section，未填充。
8. **知识沉淀层（85%）** — 2 份 explore + 1 份 review + 1 份 learn（GL context 原型集成踩坑）+ 1 份 trick（模块注册与 fallback 可复用模式）+ 1 份 decide（9 条长期架构约束归档）。基础沉淀体系完整，而且已经开始记录“设计到实现”的真实对照。

最关键的风险已从「design→checklist 断裂」转为 **accepted baseline、minimal bridge slice 与 active explore / readiness / ownership 说明是否同步**：runtime-shell、resource-diagnostics、safe-mode 已经完成 acceptance 回写，但 full-bridge 边界、exporter ownership 和后续 roadmap 入口仍然容易被误读。

```mermaid
flowchart TD
    subgraph 已有覆盖
        REQ["requirements ✅"]
        ROAD["roadmap ✅ (19项)"]
        LAND["landing notes ✅"]
        READY["readiness matrix ✅"]
        EXPL["explore (UI surfaces) ✅"]
        DES1["runtime-shell design ✅"]
        CHK["checklist.yaml ✅"]
        DES2["render-cmd-bridge design ✅"]
        DES3["input-bridge design ✅"]
        DES4["safe-mode design ✅"]
        DES5["resource-diag design ✅"]
        API["API reference (323行) ✅"]
        ACC_TMPL["acceptance template ✅"]
        IMPL_GUIDE["implementation guide ✅"]
        "README"["developer guide ✅"]
         TEST_STRAT["test strategy ✅"]
         "ARCH_FILE"["ui-rmlui-current ✅"]
        LEARN_DOC["learn (GL context) ✅"]
        TRICK_DOC["trick (fallback pattern) ✅"]
        DECIDE_DOC["decide (9 constraints) ✅"]
        REF_IDX["reference index ✅"]
        REF_BACKEND["backend ref ✅"]
        REF_CORE["core ref ✅"]
        REF_HUD["monitoring-hud ref ✅"]
        REF_HELPERS["render-helpers ref ✅"]
        REF_HOST["gameclient-host ref ✅"]
        REF_FONT["font-engine ref ✅"]
        REF_FILE["file-interface ref ✅"]
        REF_RENDER["render-interface ref ✅"]
        REF_INPUT["system-input ref ✅"]
        AUDIT_DOC["audit (8 findings) ✅"]
        RUNTIME_CODE["runtime-shell code ✅"]
        RUNTIME_TEST["runtime/compat tests ✅"]
        REVIEW_DOC["implementation review ✅"]
    end

    subgraph 缺失
        ARCH["ARCHITECTURE.md ⚠️"]
        DES_N["其余14 feature design ❌"]
        IMPL_BACKFILL["checklist/acceptance 回写 ❌"]
        TEST_BACKFILL["implementation evidence 回填 ❌"]
        ACCEPT["acceptance doc (未填充) ❌"]
    end

    REQ --> ROAD
    ROAD --> LAND
    ROAD --> READY
    ROAD --> DES1
    ROAD --> DES2
    ROAD --> DES3
    ROAD --> DES4
    ROAD --> DES5
    ROAD -.->|"断裂: 0/14"| DES_N
    ROAD --> ARCH
    ROAD --> ARCH_FILE
    DES1 --> CHK
    DES3 --> CHK
    DES4 --> CHK
    DES5 --> CHK
    CHK --> RUNTIME_CODE
    RUNTIME_CODE --> RUNTIME_TEST
    RUNTIME_CODE --> REVIEW_DOC
    RUNTIME_TEST -.->|"未回写"| IMPL_BACKFILL
    REVIEW_DOC -.->|"未回写"| TEST_BACKFILL
    CHK -.->|"未更新状态"| ACCEPT
    RUNTIME_CODE --> IMPL_GUIDE
    EXPL --> LAND
    AUDIT_DOC --> EXPL
```

## 关键证据

### 1. 规划层：4 份文档覆盖全部能力规划

- **证据**：[rmlui-full-replacement.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/requirements/rmlui-full-replacement.md) — 用户故事 7 条、成功标准 8 条、边界 7 条
- **证据**：[rmlui-full-replacement-roadmap.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-roadmap.md) — 9 模块、19 子 feature、6 阶段、接口契约 4.1-4.9 共 9 节、宿主接缝矩阵 13 行
- **证据**：[rmlui-full-replacement-items.yaml](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-items.yaml) — 19 条结构化条目，含 phase/module/depends/acceptance
- **证据**：[rmlui-full-replacement-landing-notes.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/roadmap/rmlui-full-replacement/drafts/rmlui-full-replacement-landing-notes.md) — 10 节实施指南，含切片模板、诊断格式、平台回归清单
- **证据**：[rmlui-full-replacement-readiness-matrix.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-readiness-matrix.md) — **新增**：就绪度矩阵，定义 `ready-for-impl` / `ready-for-design` 等状态与解禁条件，补充 items.yaml 的可读理由。
- 支撑结论：规划层已极度完善，可作为后续所有 feature 的唯一权威输入。

### 2. 设计层：19 条 feature 中已有 5 条 design doc（3 accepted baseline + 1 approved/in-progress + 1 draft）

- **证据**：[rmlui-runtime-shell-design.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-design.md) — status: **approved**，并已有 [rmlui-runtime-shell-checklist.yaml](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-checklist.yaml) 记录实现状态。
- **证据**：[rmlui-render-command-bridge-design.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-render-command-bridge/rmlui-render-command-bridge-design.md) — 当前为 approved design，并已有最小桥切片实现；[rmlui-input-bridge-design.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-input-bridge/rmlui-input-bridge-design.md) 仍为 draft；[rmlui-safe-mode-design.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-safe-mode/rmlui-safe-mode-design.md) 与 [rmlui-resource-diagnostics-design.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-resource-diagnostics/rmlui-resource-diagnostics-design.md) 已进入 accepted baseline。
- **证据**：[rmlui-input-bridge-checklist.yaml](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-input-bridge/rmlui-input-bridge-checklist.yaml)、[rmlui-safe-mode-checklist.yaml](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-safe-mode/rmlui-safe-mode-checklist.yaml)、[rmlui-render-command-bridge-checklist.yaml](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/features/2026-05-07-rmlui-render-command-bridge/rmlui-render-command-bridge-checklist.yaml) — input-bridge 仍停在 design-review；safe-mode 已完成并进入 acceptance baseline；render-command-bridge checklist 已有前三步 done、验证步骤待收口。
- **证据**：[items.yaml:L5-L33](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/roadmap/rmlui-full-replacement/rmlui-full-replacement-items.yaml#L5-L33) — runtime-shell、resource-diagnostics、safe-mode 已回写 feature/status；render-command-bridge 已回写 `status: in-progress` + feature 绑定。
- **证据**：其余 14 条 roadmap item 仍无 design。
- 支撑结论：设计层已经形成 5 条 design / checklist 骨架；当前缺口不是“没有 design”，而是 active baseline、minimal-slice 实现和 full-bridge 未完成边界需要保持一致表述。

### 3. 架构层：ARCHITECTURE.md 已补现状索引，但仍不是目标架构落地证明

- **证据**：[ARCHITECTURE.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/architecture/ARCHITECTURE.md) — 已记录 `CRmlUiBackend`、`CRmlUiCore`、`CRmlUiMonitoringHud`、`RmlUiRenderHelpers`、diagnostics 与 current assets。
- **证据**：[ARCHITECTURE.md:L21](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/architecture/ARCHITECTURE.md#L21) — 明确当前 RmlUI Host 只有 Monitoring HUD 试点宿主。
- **证据**：[ARCHITECTURE.md:L74](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/architecture/ARCHITECTURE.md#L74) — 明确当前 Monitoring HUD 接入仍是试点实现，不代表后续迁移模块具备同等质量。
- 支撑结论：架构层已从空骨架提升为“现状地图”，但还不能替代 feature-design。roadmap 中的 RenderBridge、LayerManager、InputBridge 等目标模块尚未实现，因此 architecture 只能说明现状，不能证明目标态已经落地。

### 4. 实现层：runtime-shell 已有首轮落地，但仍是单模块试点

- **证据**：[RmlUiRuntime.h:L25-L117](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/src/game/client/RmlUi/RmlUiRuntime.h#L25) — `SRmlUiModuleDescriptor`、`SRmlUiFrameRequest`、`SRmlUiFrameResponse`、`SRmlUiDiagnostics` 和 `CRmlUiRuntime` 已经成为真实代码，而不是只存在于 design/roadmap 中。
- **证据**：[RmlUiRuntime.cpp:L95-L188](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/src/game/client/RmlUi/RmlUiRuntime.cpp#L95) — `RenderRmlUiLayer()` 现在会真正调用 `FRenderModule` 获取 surface result，`ShouldExportDiagnostics()` 负责 debug diagnostics gate 和重复失败去重，修掉了早期“先报 rendered、后面再失败”的语义问题。
- **证据**：[gameclient.cpp:L1654-L1833](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/src/game/client/gameclient.cpp#L1654) — Monitoring HUD host 已改为通过 `EnsureRmlUiMonitoringRuntimeRegistered()` 注册 module、通过 `RenderRmlUiLayer()` 请求 `GAME_HUD` layer，并在 `RenderRmlUiMonitoringModule()` 中通过 `RunOnBackendThread(...)` 执行 backend-frame document render，返回真实的 `RENDERED` / `FALLBACK_REQUIRED` surface 结果。
- **证据**：[RmlUiConfigCompat.h](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/src/game/client/RmlUi/RmlUiConfigCompat.h) + [gameclient.cpp:L182-L193](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/src/game/client/gameclient.cpp#L182) + [gameclient.cpp:L432-L464](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/src/game/client/gameclient.cpp#L432) — legacy `qm_monitoring_use_rmlui` 与新 `qm_rmlui_monitoring_hud` / `qm_rmlui_enable` 的兼容同步已经被抽成可复用 helper，并接入 console chain。
- **证据**：[rmlui_backend.cpp:L160-L208](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/src/engine/client/rmlui_backend.cpp#L160) — backend 仍强依赖 `SDL_GL_GetCurrentContext()` 和 `RenderInterface_GL3`，证明 render-command-bridge / 多后端支持尚未进入实现层。
- 支撑结论：实现层已经从“纯试点散装路径”推进到“runtime-shell + 单模块宿主接线”的首轮工程闭环，但它仍然只覆盖 Monitoring HUD 和 GL3 原型后端，离 roadmap 目标态还有明显距离。

### 5. 测试层：TDD 已启动，runtime/compat 单测已经落地

- **证据**：[rmlui_monitoring_assets_test.cpp](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/src/test/rmlui_monitoring_assets_test.cpp) — 2 个资产 schema 测试仍然保留。
- **证据**：[rmlui_runtime_test.cpp](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/src/test/rmlui_runtime_test.cpp) — 现已覆盖 global/module toggle、runtime unavailable、surface failure、duplicate registration、diagnostics export gate/dedupe、legacy toggle compatibility，以及 safe-mode 与 resource diagnostics 的关键场景。
- **证据**：[rmlui-test-strategy.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/reference/rmlui-test-strategy.md) — 文档层测试矩阵与代码层测试切片现在开始对齐，不再只是宣告“TDD 默认开启”。
- 支撑结论：测试层已从“只有资产 schema”提升为“资产 + runtime/compat + safe-mode/resource diagnostics”，说明 TDD 已经真正进入执行；但 exported diagnostic file、render bridge 和 multi-backend evidence 仍未开始。

### 6. 参考层：从 3 份跃升至 13 份 libdoc，形成完整参考体系

- **证据**：[rmlui-reference-index.md](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/reference/rmlui-reference-index.md) — **新增**：顶层索引，将 13 份文档按 Upstream API 和 QmClient Surface 分组。
- **证据**：Upstream API 组（5 份）— 原有 `rmlui-runtime-api-reference.md`（323行，三层）+ 新补 `rmlui-render-interface-reference.md`、`rmlui-file-interface-reference.md`、`rmlui-system-input-reference.md`、`rmlui-font-engine-reference.md`。
- **证据**：QmClient Surface 组（5 份）— 全新增：`rmlui-backend-reference.md`（Init/Shutdown/IsAvailable/failure labels）、`rmlui-core-reference.md`（Public API/Init Failure Codes/LoadDocument/Availability）、`rmlui-monitoring-hud-reference.md`（Public API/Init/Shutdown/Render/State）、`rmlui-render-helpers-reference.md`、`rmlui-gameclient-host-reference.md`（Host entry points/failure responsibilities）。
- **证据**：Guides（2 份）— `rmlui-developer-guide.md` + `rmlui-test-strategy.md`。
- **证据**：developer-guide 未引用 compound/architecture 文档（审计 F5）。
- 支撑结论：参考层是本周文档质量提升最大的层，开发者可通过 reference-index 导航到 13 份结构化文档。唯一缺失是面向最终用户的 user guide。

### 7. 知识沉淀：learn/trick/decide 三类均已补齐

- **证据**：[learn](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/compound/2026-05-07-learn-rmlui-gl-context-prototype.md) — GL context 原型集成踩坑。三点根因（硬编码 `SDL_GL_GetCurrentContext`、`dynamic_cast<RenderInterface_GL3 *>` 单后端耦合、调用时机不确定），三条学到的规则（不在 surface module 中获取 context、backend init 与 rendering 分离、fallback 路径不依赖 RmlUI），三条后续避免方式。
- **证据**：[trick](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/compound/2026-05-07-trick-rmlui-module-fallback-pattern.md) — 模块注册与 fallback 可复用模式。四级协议（ModuleDescriptor → FrameRequest → FrameResult → HostFallback），与 DDNet 现有 `GRAPHICS_DISPLAY_BACKEND` 模式的精神类比。
- **证据**：[decide](file:///c:/Users/11054/.codex/worktrees/140c/QmClient/.codestable/compound/2026-05-07-decide-rmlui-architecture-constraints.md) — 9 条长期架构约束归档。分五类：平台约束（旧 UI 永久、RmlUI 默认关闭）、命名约束（qm_ 前缀、诊断路径）、渲染约束（禁止 SDL_GL_* 新增、GL3 是原型）、开发流程约束（TDD、Monitoring HUD 是 prototype host）、架构文档约束（acceptance 前不回写）。
- 支撑结论：知识沉淀从只有 explore 扩展到了 learn/trick/decide 三类全面覆盖，基础沉淀体系完整。

## 分层评估矩阵

| 层 | 覆盖度 | 已有产物 | 关键缺失 | 严重度 |
|---|---|---|---|---|
| 规划层 (requirements/roadmap) | 95% | req + roadmap + items.yaml + landing notes + readiness-matrix (84行，19项就绪度) | PRD 未细评 | 低 |
| 设计层 (feature-design) | 26% | 5/19 design（3 accepted baseline + 1 approved/in-progress + 1 draft），4 份 checklist | 14 feature design 空白、full-bridge 边界仍需持续对齐 | **严重** |
| 架构层 (architecture) | 35% | ARCHITECTURE.md + ui-rmlui-current.md（scope + interfaces + render + diagnostics + known issues） | 目标模块未通过 acceptance 回写 | 高 |
| 实现层 (source code) | 40% | runtime-shell + resource diagnostics + safe-mode + render-command-bridge minimal slice + 监控 HUD host 接线 + compat helper + impl-guide | 仍缺 full render bridge / input bridge / 多 surface | **严重** |
| 验收层 (acceptance) | 10% | acceptance-template (168行) | 全部未填充 | 高 |
| 测试层 (tests) | 40% | 1 资产测试文件 + runtime/compat/safe-mode/resource diagnostics 测试 + test-strategy | exported file / render bridge / multi-backend evidence | 高 |
| 参考层 (libdoc/guide) | 85% | 13 份：reference-index + 5 upstream API + 5 QmClient surface + 2 guides | 用户 guide、developer-guide F5 | 低 |
| 审计层 (audit) | 新增 | audit/rmlui-docs: 8 条发现（4 P1 + 4 P2） | P1 待修 | 中 |
| 知识层 (compound) | 80% | 2 explore + 1 learn + 1 trick + 1 decide (9 条约束) | 后续实践经验可继续补充 | 低 |

## 结论展开

### 规划层不是问题，承接链才是问题

roadmap 的细节密度已经到了「每个 feature 的验收标准、宿主接缝、fallback owner、后端假设和证据要求都可以直接执行」的程度。但 roadmap 向下走的关键断点现在主要体现在“accepted baseline 与 active bridge slice 是否被误写成 full migration”：

- roadmap → design：19 条 item 中 5 条有 design doc（其中 runtime-shell / resource-diagnostics / safe-mode 已 accepted，render-command-bridge 已 approved/in-progress，input-bridge 仍 draft），14 条无
- design → checklist：5 份 design 中 4 份已有 checklist；其中 runtime-shell / safe-mode 已完成，render-command-bridge 只差验证收口，input-bridge 仍未启动
- checklist → 实现：runtime-shell、resource diagnostics、safe-mode 和 render-command-bridge minimal slice 已有代码事实，但 full bridge / input bridge / migration 仍未启动
- 实现 → 审计回写：审计已产出 8 条发现（4 P1 + 4 P2）；当前更需要的是让 active 文档入口和实际代码状态持续一致

### 文档与实现开始重新接上，但流程产物落后于代码

本轮最大变化不再只是文档扩张，而是 runtime-shell、resource-diagnostics、safe-mode 以及 render-command-bridge minimal slice 都已经进了代码和测试/回写。问题也因此变了：现在不是“没实现”，而是“哪些已经是 accepted baseline，哪些只是 minimal slice，哪些还不能被夸大成 full bridge”。这比纯空白状态更好，但也更需要流程回写，否则文档会再次和代码脱节。

### 架构文档已补现状，但仍不能承接实现

ARCHITECTURE.md 已经能让读者看到当前 RmlUI 的 backend/core/Monitoring HUD 试点宿主和 fallback 边界，这是明显进展。但它仍然只记录现状，不能把 roadmap 里的 9 个目标模块写成已存在事实。真正的缺口已经从“架构入口空白”变成“feature-design / checklist / TDD 实现 / acceptance 回写还没有接上”。

### TDD 已落地到 runtime-shell，但 acceptance 还没接上

attention.md 声明了「RmlUI 相关 feature 实现默认走 TDD」，这条现在已经有代码证据：`rmlui_runtime_test.cpp` 覆盖了 runtime result、diagnostics gate、safe-mode 和 resource diagnostics。当前剩下的不是“有没有测试”，而是 exported file、render bridge 和 multi-backend 证据还没有顺着这次实现一起补齐。

### 知识沉淀基础已完成

learn/trick/decide 三类各有一份代表性文档。后续随着实现推进，新的踩坑（如 Vulkan/Android backend init 失败模式）、新的可复用技巧（如 diagnostic throttling 策略）、新的被拍板决定（如 click-gui 输入模型选择）应该继续补充，而不是一次性堆满。

### 参考文档体系基本成型

API reference + developer guide + test strategy 三份文档共同作用，开发者已经不需要对着源码盲猜 RmlUI 初始化顺序、四个核心类的职责边界、诊断合同字段和 TDD 测试切面。唯一缺口是面向最终用户的 user guide，但那是 roadmap 较后期的产物。

## 后续建议

优先级排序（从高到低）：

1. **优先把 render-command-bridge 的验证证据收口** — 最小桥切片已在代码里，但还缺构建/运行层面的最终 acceptance 收口。
2. **继续收拢 resource-diagnostics exporter ownership** — 当前真正剩下的不是 schema，而是从 Monitoring HUD host residue 往 runtime/resource diagnostics 层继续抽离。
3. **把 input-bridge 保持在 design-review 阶段** — 在 render bridge full contract、layer ownership 和 input safety 没拍板前，不要提前进入实现。
4. **暂停新的大规模文档扩张，优先收敛 active baseline 与实现闭环** — 这轮最大的收益已经不是“再多写 5 份文档”，而是让 accepted baseline、minimal slice 和 future work 边界保持一致。

> 当前最合适的下一步已经从“启动 runtime-shell 的 TDD”切换为“补 runtime-shell 的 acceptance / arch 回写”，然后再基于这条更稳的基线推进 safe-mode 或 resource-diagnostics。
