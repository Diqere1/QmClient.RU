---
doc_type: libdoc
status: current
created: 2026-05-07
last_reviewed: 2026-05-10
tags: [rmlui, gameclient, host, reference]
related_feature: 2026-05-07-rmlui-runtime-shell
---

# GameClient RmlUI Host Reference

Path: `src/game/client/gameclient.cpp`

This reference documents the current RmlUI host entry points.

## Host Responsibilities

Current host behavior combines:

- checking the Monitoring HUD RmlUI toggle
- ensuring core/backend availability
- calling the Monitoring HUD prototype renderer
- dispatching page/modal runtime slots for menu-facing surfaces
- falling back to legacy HUD behavior
- exporting diagnostics in the current prototype path

## Current Entry Points

### `CGameClient::RenderQmMonitoringHud(...)`

Current role:

- The concrete host owner for the Monitoring HUD surface.
- Switches between legacy HUD and the RmlUI HUD path.

Current behavior:

- Reads `g_Config.m_QmMonitoringUseRmlUi`.
- Allows the Monitoring HUD host path to keep rendering while menus are open so `Ctrl+Shift+G` can be used to inspect menu and settings performance.
- Checks backend/core readiness.
- Calls Monitoring HUD init/render.
- Fallbacks to legacy HUD if needed.

Important boundary:

- It is a host owner, not the runtime shell.
- It currently contains prototype-specific branching that should be reduced as runtime-shell takes over.
- Its RmlUI path now targets the HUD context domain and must not be merged back with menu/page/modal context ownership.

### `CGameClient::DispatchRmlUiMenuPageSlot()` / `DispatchRmlUiMenuModalSlot(...)`

Current role:

- Dispatches menu page and menu modal surfaces into the runtime host seam.
- Lets `CMenus` keep legacy ownership while page/modal migration proceeds incrementally.

Important boundary:

- These entry points are host seams, not proof that full menu migration is complete.
- Menu page/modal surfaces share menu-facing host ownership, but must still respect explicit context boundaries versus HUD/overlay surfaces.

### `CGameClient::EnsureRmlUiMonitoringReady(const char *pStage)`

Current role:

- Ensures the prototype RmlUI pieces are ready for Monitoring HUD rendering.

Important boundary:

- This is prototype-specific readiness logic.
- Runtime-shell should absorb the cross-feature readiness contract later.

### `CGameClient::ExportRmlUiDiagnostics(const char *pStage)`

Current role:

- Exports current runtime/module diagnostics for the active RmlUI surface.

Important boundary:

- Files are currently written to the current working directory's `debug-artifacts/`; when the client is launched from the build directory as intended, that resolves to the build-local diagnostics folder.
- The exporter is no longer Monitoring HUD-specific, but the host still owns when to trigger the export.

## Prototype State

Current host-specific behavior should be understood as prototype-only until runtime-shell acceptance.

Do not infer:

- render-command bridge completion
- input-bridge completion
- menu/settings/wheel/editor migration completion

Current note:

- The host still owns legacy fallback rendering and fallback notice presentation, but safe-mode state itself now lives in `CRmlUiRuntime`.
- The host now also owns the immediate hide/deactivate behavior when a module stops rendering; it must not assume “the next render of the same context will clean up later”.

## What To Look At Next

- Runtime-shell design and checklist for the first implementation slice.
- Resource diagnostics design for structured failure files.
- Render bridge draft for the next major backend question.
