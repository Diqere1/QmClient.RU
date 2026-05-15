---
doc_type: implementation-guide
feature: 2026-05-07-rmlui-runtime-shell
status: current
created: 2026-05-07
tags: [rmlui, runtime, tdd, implementation]
related_design: rmlui-runtime-shell-design.md
related_checklist: rmlui-runtime-shell-checklist.yaml
---

# rmlui-runtime-shell implementation guide

This guide is the bridge from approved design to TDD implementation. The checklist remains the execution tracker; this guide explains how to interpret it.

This guide is the feature-specific execution of the wider RmlUI test strategy and landing notes slicing rules.

## 1. Working Assumptions

- RmlUI is optional and disabled by default.
- Old UI is a permanent supported path, not temporary scaffolding.
- Monitoring HUD is only the first prototype host.
- `Rml::Context` is a first-class runtime boundary: keep at least separate HUD-side and menu-side context domains, and do not let individual surfaces drive a shared context independently.
- Runtime-shell validates module registration, toggles, frame result semantics, fallback, and diagnostics.
- Runtime-shell does not solve render-command bridge, graph layout, CSS compatibility, font completeness, Vulkan, or Android rendering.

## 2. TDD Order

Implement in this order:

1. Write failing tests for disabled/global-off behavior.
2. Write failing tests for module-off behavior.
3. Write failing tests for runtime unavailable/fallback behavior.
4. Implement the smallest runtime shell that makes those tests pass.
5. Register `monitoring_hud` as `GAME_HUD` prototype host.
6. Connect `RenderQmMonitoringHud` to consume frame results and keep legacy fallback.
7. Add diagnostics export tests or field validation.
8. Clean up replaced prototype diagnostics only after tests pass.

The first test pass should not require real RmlUI rendering. It should prove runtime decisions and fallback semantics.

## 3. Test Slice Map

### Global Off

Input:

- `qm_rmlui_enable = 0`
- Monitoring HUD legacy path is otherwise available

Expected:

- Runtime returns `SKIPPED_DISABLED`.
- RmlUI core is not initialized.
- Host keeps legacy HUD fallback.

### Module Off

Input:

- `qm_rmlui_enable = 1`
- `qm_rmlui_monitoring_hud = 0`

Expected:

- Runtime returns `SKIPPED_DISABLED`.
- No Monitoring HUD RmlUI document load is attempted.
- Host keeps legacy HUD fallback.

### Runtime Unavailable

Input:

- `qm_rmlui_enable = 1`
- `qm_rmlui_monitoring_hud = 1`
- backend/core unavailable, for example no context or injected unavailable runtime state

Expected:

- Runtime returns `FALLBACK_REQUIRED`.
- Diagnostics include module, layer, fallback owner, and backend/core failure.
- Host keeps legacy HUD fallback.

### Prototype Surface Failure

Input:

- Runtime/core can proceed far enough to call the module.
- Monitoring HUD document/surface fails.

Expected:

- Runtime returns `FALLBACK_REQUIRED`.
- Failure reason distinguishes surface/document failure from core/backend failure.
- Host keeps legacy HUD fallback.

### Duplicate Registration

Input:

- Register `monitoring_hud` twice.

Expected:

- Only one effective module remains.
- Runtime reports duplicate registration through false return, diagnostics, or stable log behavior.

## 4. Implementation Boundaries

Do not change these during runtime-shell:

- Monitoring HUD graph algorithm.
- Monitoring HUD RCSS visual style.
- RmlUI font bundle.
- GL3 backend implementation strategy.
- Vulkan or Android backend behavior.
- Menu, popup, settings, wheel, HUD editor behavior.
- Context ownership must remain centralized; do not reintroduce per-surface `Context::Update()` / `Context::Render()` control over the same context domain.

Allowed changes:

- Add runtime shell types and implementation under the RmlUI client area.
- Add tests for runtime result semantics.
- Add or wire `qm_rmlui_*` config names.
- Keep compatibility with current Monitoring HUD experiment toggle while making the new names canonical.
- Move runtime diagnostics responsibility out of host-specific code when covered by tests.

## 5. Config Compatibility Rule

Canonical new names:

- `qm_rmlui_enable`
- `qm_rmlui_safe_mode`
- `qm_rmlui_monitoring_hud`

Existing experiment name:

- `qm_monitoring_use_rmlui`

Implementation must choose and document one compatibility behavior:

- Mirror old experiment toggle into the new module toggle during transition.
- Keep old toggle as an alias that only affects Monitoring HUD.
- Replace reads with new names and leave a migration note.

Do not introduce `cl_` names for QmClient-owned RmlUI config.

## 6. Diagnostics Contract

Minimum diagnostic fields:

- `schema`
- `module`
- `stage`
- `layer`
- `result`
- `core_initialized`
- `core_available`
- `context_available`
- `document_loaded`
- `core_failure`
- `backend_failure`
- `document_failure`
- `fallback_owner`
- `backend_assumption`
- `timestamp_local`

Target:

- `dumps/QmClient_Crash/`

Rules:

- Development diagnostics may be enabled for Release/RelWithDebInfo local runs.
- Release builds must not write per-frame diagnostics by default.
- Diagnostic writes should be throttled or event-based.

## 7. Acceptance Inputs To Preserve

When implementation finishes, collect:

- Test output proving global-off/module-off/runtime-unavailable cases.
- A sample diagnostics file for a forced failure.
- A log or screenshot proving legacy HUD fallback remains available.
- A note explaining how `qm_monitoring_use_rmlui` interacts with `qm_rmlui_monitoring_hud`.
- A short statement that no new GL context acquisition path was added.

These inputs feed `rmlui-runtime-shell-acceptance.md`.
