---
doc_type: libdoc
status: current
created: 2026-05-07
last_reviewed: 2026-05-10
tags: [rmlui, monitoring-hud, api, reference]
related_feature: 2026-05-07-rmlui-runtime-shell
---

# CRmlUiMonitoringHud Reference

Path: `src/game/client/RmlUi/RmlUiMonitoringHud.h/.cpp`

This reference documents the Monitoring HUD prototype surface.

## Purpose

`CRmlUiMonitoringHud` is the first concrete HUD-facing RmlUI surface in the current runtime path.

It loads the Monitoring HUD document, updates values, validates document structure, and renders the HUD surface within the HUD context domain.

## Public API

### `bool Init(CRmlUiCore *pCore)`

Initializes the Monitoring HUD surface with the runtime core.

### `void Shutdown()`

Releases the current document and prototype surface state.

### `bool IsInitialized() const`

Returns whether the HUD prototype was initialized.

### `bool IsAvailable() const`

Returns whether the HUD prototype is currently usable.

### `bool HasDocument() const`

Returns whether a document has been loaded.

### `bool Render(IGraphics *pGraphics, const CUIRect &View, const SQmMonitoringViewModel &ViewModel)`

Attempts to render the Monitoring HUD prototype surface.

Important:

- This is a prototype host call, not the future general surface API.
- Rendering success does not imply the implementation is stable enough for reuse.

### `ERenderFailure LastFailure() const`

Returns the last Monitoring HUD render failure enum.

## Failure Enum

- `NONE`
- `NOT_AVAILABLE`
- `NO_GRAPHICS`
- `DOCUMENT_LOAD_FAILED`
- `DOCUMENT_STRUCTURE_INVALID`
- `MAIN_GRAPH_RECT_INVALID`
- `FPS_GRAPH_RECT_INVALID`

## Helper Methods

Private helper methods are prototype implementation details:

- `EnsureDocument()`
- `ValidateDocumentStructure()`
- `UpdateDocument(...)`
- `UpdateText(...)`
- `UpdateMetric(...)`
- `UpdateRate(...)`
- `ResolveRect(...)`

## Boundary Notes

- The current surface should not be used as a template for render bridge behavior.
- Graph/layout/resource issues belong to the prototype and should be exposed through diagnostics, not hidden.
- Fallback remains the responsibility of the host.
- This surface belongs to the HUD context domain; menu/page/modal surfaces must not share its context ownership just because they also use RmlUI.
