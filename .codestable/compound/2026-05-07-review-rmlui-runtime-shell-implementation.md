---
doc_type: review
status: current
created: 2026-05-07
tags: [rmlui, runtime-shell, review, diagnostics]
related_feature: 2026-05-07-rmlui-runtime-shell
---

# RmlUI Runtime-Shell Implementation Review

Review basis:

- `.codestable/reference/` reference layer, including Context7-backed RmlUi API notes
- `rmlui-runtime-shell` / `rmlui-safe-mode` / `rmlui-resource-diagnostics` feature designs
- current prototype implementation in `src/game/client/RmlUi/RmlUiRuntime.*` and `src/game/client/gameclient.cpp`

## Findings

### Severity: Major

File + line:

- `src/game/client/gameclient.cpp:128-131`

Problem description:

- The global runtime toggle is not authoritative. `IsRmlUiToggleEnabled()` treats legacy `qm_monitoring_use_rmlui` as an OR-condition for both `qm_rmlui_enable` and `qm_rmlui_monitoring_hud`, so the runtime can remain effectively enabled even when the new global toggle is turned off.

Why it matters:

- This breaks the runtime-shell contract and the TDD matrix case that `qm_rmlui_enable=0` must skip RmlUI without core init. It also makes future safe-mode reset and support/debug instructions ambiguous because one legacy knob can silently override the new runtime policy.

Suggested fix:

- Keep compatibility aliasing one-way at migration boundaries, but make `qm_rmlui_enable` the final authority for runtime activation. If legacy config must be honored, fold it into migration/init sync instead of OR-ing it in the live global enable check.

### Severity: Major

File + line:

- `src/game/client/RmlUi/RmlUiRuntime.cpp:120-123`
- `src/game/client/gameclient.cpp:1648-1692`

Problem description:

- `CRmlUiRuntime::RenderRmlUiLayer()` returns `RENDERED` as soon as core availability is confirmed, before backend frame-context acquisition, surface readiness, document availability, and surface render completion are known. The host then performs additional failure-prone work after a nominal runtime success.

Why it matters:

- This makes `RENDERED` a false-positive status. Any future surface that consumes runtime results as documented could incorrectly assume the layer has rendered successfully, while fallback is still needed later. It also weakens safe-mode and diagnostics because later failures are no longer part of the primary frame result contract.

Suggested fix:

- Either keep runtime results scoped to “runtime-ready” and rename the success state accordingly, or move the surface execution callback into runtime-shell so `RENDERED` is only emitted after the module path actually completes.

### Severity: Major

File + line:

- `src/game/client/gameclient.cpp:1631-1637`
- `src/game/client/gameclient.cpp:1641-1643`
- `src/game/client/gameclient.cpp:1664-1665`
- `src/game/client/gameclient.cpp:1689-1690`
- `src/game/client/gameclient.cpp:1768-1822`

Problem description:

- The prototype host hardcodes `m_DebugDiagnostics=true` for every Monitoring HUD runtime request in debug builds and exports a diagnostics file on fallback paths without any deduplication or throttle. Repeated identical failures can therefore create one file per frame.

Why it matters:

- This directly conflicts with the resource-diagnostics design goal of development-only, event-based export and will flood `dumps/QmClient_Crash/` during persistent failures. It also makes later diagnostics review harder because repeated noise obscures state transitions.

Suggested fix:

- Separate “development diagnostics intent” from the default debug build path, and add session-level deduplication keyed by module/stage/error tuple before calling the exporter.

## Overall verdict

- Needs Fix

Short explanation:

- The current runtime-shell prototype is a useful skeleton, but three contract mismatches remain: global toggle authority, false-positive `RENDERED` semantics, and unthrottled diagnostics export. These should be resolved before treating the runtime-shell path as a stable base for safe-mode or resource diagnostics implementation.
