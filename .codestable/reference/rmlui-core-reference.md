---
doc_type: libdoc
status: current
created: 2026-05-07
last_reviewed: 2026-05-10
tags: [rmlui, core, api, reference]
related_feature: 2026-05-07-rmlui-runtime-shell
---

# CRmlUiCore Reference

Path: `src/game/client/RmlUi/RmlUiCore.h/.cpp`

This document describes the current runtime core wrapper.

## Purpose

`CRmlUiCore` owns the current RmlUI core/context lifecycle and the shared backend.

It currently bridges:

- backend initialization
- font loading
- context creation for separate UI domains
- per-context viewport updates
- per-context document loading
- per-context update/render calls
- core-level availability and failure state

## Public API

### `bool Init(IStorage *pStorage)`

Initializes the backend and RmlUI core state.

Current order expectations:

1. Initialize backend.
2. Initialize RmlUI core.
3. Create required contexts.
4. Load fonts.

### `void Shutdown()`

Releases all managed contexts and backend state.

### `void SetViewport(ERmlUiContextSlot Slot, int Width, int Height)`

Updates the selected context viewport.

### `Rml::ElementDocument *LoadDocument(ERmlUiContextSlot Slot, const char *pPath)`

Loads an RML document into the selected context.

Important:

- Document load failure is separate from backend/core failure.
- The current prototype should not treat missing documents as a successful runtime state.

### `void Update(ERmlUiContextSlot Slot)`

Advances the selected RmlUI context.

### `void Render(ERmlUiContextSlot Slot)`

Renders the selected RmlUI context.

### State Queries

- `bool IsInitialized() const`
- `bool IsAvailable() const`
- `bool HasContext() const`
- `bool HasContext(ERmlUiContextSlot Slot) const`
- `const CRmlUiBackend &Backend() const`
- `Rml::Context *Context(ERmlUiContextSlot Slot) const`
- `EInitFailure LastInitFailure() const`
- `const char *LastInitFailureString() const`

## Failure Enum

- `NONE`
- `BACKEND_INIT_FAILED`
- `CORE_INITIALISE_FAILED`
- `CREATE_CONTEXT_FAILED`

## Behavior Notes

- Fonts are loaded as part of initialization.
- The current prototype path expects a backend before context creation.
- `CRmlUiCore` is not the future module registry.
- It can be reused internally by runtime-shell, but should not absorb layer routing or fallback policy.
- `LoadDocument()` is a surface-level document load, not a runtime registry call.
- `SetViewport()` must stay tied to the selected context dimensions and not become a render bridge surrogate.
- Menu/page/modal documents and HUD/overlay documents must not silently collapse back into one shared `Rml::Context`; they need separate context ownership whenever independent input state, focus, or render lifecycles are required.

## Implementation Boundary

Keep this class focused on core/context lifecycle.
Do not move Monitoring HUD routing or diagnostics file ownership into it.
