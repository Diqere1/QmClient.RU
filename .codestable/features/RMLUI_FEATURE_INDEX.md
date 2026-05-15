# RmlUI Feature 索引

这份索引只列当前活跃的 RmlUI 功能文档，方便后续设计、实现和验收快速定位正确入口。settings 的长期 host 规则单独放在 [RmlUI Settings Host Contract](../reference/rmlui-settings-host-contract.md)。

## 已验收基线

| 功能 | 当前状态 | 下游使用方式 |
|---|---|---|
| [rmlui-runtime-shell](2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-design.md) | 已验收基线 | 作为当前 runtime / module / fallback / diagnostics 的默认基线，供后续设计评审和实现规划复用。 |
| [rmlui-resource-diagnostics](2026-05-07-rmlui-resource-diagnostics/rmlui-resource-diagnostics-design.md) | 已验收基线 | 作为 safe-mode、render bridge 和后续迁移功能的资源诊断 / 导出口径基线。 |
| [rmlui-render-command-bridge](2026-05-07-rmlui-render-command-bridge/rmlui-render-command-bridge-design.md) | 已验收最小 bridge 基线 | 作为当前避免 context 争抢的 graphics-thread callback 基线，但 full geometry / scissor / texture bridge 仍放在后续功能。 |
| [rmlui-scissor-texture-bridge](2026-05-08-rmlui-scissor-texture-bridge/rmlui-scissor-texture-bridge-design.md) | 已验收 texture / scissor bridge 基线 | 作为当前 texture registry 与 scissor translation contract 的 bridge 基线，但 full geometry / layer / filter / shader bridge 仍放在后续功能。 |
| [rmlui-layer-switchboard](2026-05-08-rmlui-layer-switchboard/rmlui-layer-switchboard-design.md) | 已验收宿主调度基线 | 作为后续具体 menu / debug / popup 界面迁移前的 host dispatch order、frame token / surface tag guard 与 legacy fallback ownership 基线。 |
| [rmlui-monitoring-hud-migration](2026-05-08-rmlui-monitoring-hud-migration/rmlui-monitoring-hud-migration-design.md) | 已验收首条 concrete migration 样板 | 作为后续 debug/menu/popup 具体界面迁移的 mixed render、surface contract、本地化归口与 host fallback owner 样板。 |
| [rmlui-input-bridge](2026-05-07-rmlui-input-bridge/rmlui-input-bridge-design.md) | 已验收交互输入协议基线 | 作为后续 popup / menu-pilot / click-gui 的输入消费、cancel、release-state 与 owner priority 基线。 |
| [rmlui-popup-migration](2026-05-08-rmlui-popup-migration/rmlui-popup-migration-design.md) | 已验收首条交互式 modal surface 样板 | 作为后续 menu-pilot 设计与实现的 popup/modal 宿主、action token 回接和 fallback 样板。 |
| [rmlui-safe-mode](2026-05-07-rmlui-safe-mode/rmlui-safe-mode-design.md) | 已验收基线 | 作为交互式界面迁移前的 safe-mode trip / demotion / reset 语义基线。 |
| [rmlui-menu-pilot](2026-05-09-rmlui-menu-pilot/rmlui-menu-pilot-design.md) | 已验收首个 `MENU_PAGE` concrete surface 基线 | 作为 settings 主线的 `MENU_PAGE` 宿主接缝、page/modal 菜单侧 context 语义和 fallback 基线；不再代表 settings 主线的最终宿主形态。 |

配套文档：

- [runtime-shell checklist](2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-checklist.yaml)
- [runtime-shell 实现说明](2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-implementation-guide.md)
- [runtime-shell 验收报告](2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-acceptance.md)
- [resource-diagnostics checklist](2026-05-07-rmlui-resource-diagnostics/rmlui-resource-diagnostics-checklist.yaml)
- [resource-diagnostics 验收报告](2026-05-07-rmlui-resource-diagnostics/rmlui-resource-diagnostics-acceptance.md)
- [render-command-bridge checklist](2026-05-07-rmlui-render-command-bridge/rmlui-render-command-bridge-checklist.yaml)
- [render-command-bridge 验收报告](2026-05-07-rmlui-render-command-bridge/rmlui-render-command-bridge-acceptance.md)
- [scissor-texture-bridge checklist](2026-05-08-rmlui-scissor-texture-bridge/rmlui-scissor-texture-bridge-checklist.yaml)
- [scissor-texture-bridge 验收报告](2026-05-08-rmlui-scissor-texture-bridge/rmlui-scissor-texture-bridge-acceptance.md)
- [layer-switchboard checklist](2026-05-08-rmlui-layer-switchboard/rmlui-layer-switchboard-checklist.yaml)
- [layer-switchboard 验收报告](2026-05-08-rmlui-layer-switchboard/rmlui-layer-switchboard-acceptance.md)
- [monitoring-hud-migration checklist](2026-05-08-rmlui-monitoring-hud-migration/rmlui-monitoring-hud-migration-checklist.yaml)
- [monitoring-hud-migration 验收报告](2026-05-08-rmlui-monitoring-hud-migration/rmlui-monitoring-hud-migration-acceptance.md)
- [input-bridge checklist](2026-05-07-rmlui-input-bridge/rmlui-input-bridge-checklist.yaml)
- [input-bridge 验收报告](2026-05-07-rmlui-input-bridge/rmlui-input-bridge-acceptance.md)
- [popup-migration checklist](2026-05-08-rmlui-popup-migration/rmlui-popup-migration-checklist.yaml)
- [popup-migration 验收报告](2026-05-08-rmlui-popup-migration/rmlui-popup-migration-acceptance.md)
- [safe-mode checklist](2026-05-07-rmlui-safe-mode/rmlui-safe-mode-checklist.yaml)
- [safe-mode 验收报告](2026-05-07-rmlui-safe-mode/rmlui-safe-mode-acceptance.md)
- [menu-pilot checklist](2026-05-09-rmlui-menu-pilot/rmlui-menu-pilot-checklist.yaml)
- [menu-pilot 验收报告](2026-05-09-rmlui-menu-pilot/rmlui-menu-pilot-acceptance.md)

## 已批准设计

| 功能 | 当前状态 | 下游使用方式 |
|---|---|---|
| [rmlui-settings-reorg](2026-05-10-rmlui-settings-reorg/rmlui-settings-reorg-design.md) | 已批准设计，准备实现 | settings 主线的宿主重整 feature；细节见 feature design 与 settings-host contract。 |

后续主线提示：

- `rmlui-settings-reorg` 完成后，settings 主线继续按 `rmlui-settings-native-controls -> rmlui-settings-search -> rmlui-settings-visual-refresh` 推进。
- settings 的长期 host 规则看 `../reference/rmlui-settings-host-contract.md`，feature 目标看 `rmlui-settings-reorg-design.md`。

## 草案设计

当前为空。

## 共享参考

- [RmlUI Runtime API 参考](../reference/rmlui-runtime-api-reference.md)
- [RmlUI Settings Host Contract](../reference/rmlui-settings-host-contract.md)
- [RmlUI Roadmap 就绪矩阵](../roadmap/rmlui-full-replacement/rmlui-full-replacement-readiness-matrix.md)
- [RmlUI 测试策略](../reference/rmlui-test-strategy.md)
- [RmlUI 开发指南](../reference/rmlui-developer-guide.md)

## 规则

- 不要从草案设计直接进入实现。
- 不要在 design 未批准且 checklist 未存在时，把 roadmap item 标成 `in-progress`。
- 不要在功能验收前，把未来目标模块写进 architecture 的 current state。
