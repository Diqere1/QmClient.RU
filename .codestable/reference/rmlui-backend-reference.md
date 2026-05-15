---
doc_type: libdoc
status: current
created: 2026-05-07
last_reviewed: 2026-05-10
tags: [rmlui, backend, api, reference]
related_feature: 2026-05-07-rmlui-runtime-shell
---

# CRmlUiBackend Reference

Path: `src/engine/client/rmlui_backend.h/.cpp`

This reference documents the current backend wrapper used by the RmlUI runtime.

## Purpose

`CRmlUiBackend` is the backend wrapper for RmlUI in QmClient.

It currently owns:

- system interface
- file interface
- render interface
- backend initialization/shutdown
- viewport/frame hooks
- initialization failure tracking

## Public API

### `bool Init(IStorage *pStorage)`

Initializes the backend interfaces.

Current behavior:

- Fails when storage is unavailable.
- Fails when there is no active OpenGL context.
- Initializes GL3-based RmlUI backend state on success.

Important:

- This is current backend behavior.
- It is not the long-term multi-backend render bridge.

### `void Shutdown()`

Shuts down the backend and releases its interface objects.

### `bool IsInitialized() const`

Returns whether initialization was performed and the backend has not been shut down.

### `bool IsAvailable() const`

Returns whether the backend currently has all required interface objects and is usable.

### `void SetViewport(int Width, int Height)`

Updates backend viewport state.

Current caveat:

- Only the current GL3 render interface path is supported.

### `void BeginFrame()`

Prepares the backend for a new frame.

### `void EndFrame()`

Completes frame submission.

### `Rml::SystemInterface *SystemInterface() const`

Returns the current RmlUI system interface.

### `Rml::FileInterface *FileInterface() const`

Returns the current RmlUI file interface.

### `Rml::RenderInterface *RenderInterface() const`

Returns the current RmlUI render interface.

### `EInitFailure LastInitFailure() const`

Returns the last backend init failure enum.

### `const char *LastInitFailureString() const`

Returns a stable string for the last init failure.

## Failure Enum

- `NONE`
- `STORAGE_UNAVAILABLE`
- `NO_GL_CONTEXT`
- `GL3_INIT_FAILED`
- `RENDER_INTERFACE_FAILED`

## Usage Notes

- `CRmlUiCore` owns higher-level runtime state.
- `CRmlUiBackend` should not be mistaken for the future render command bridge.
- Current GL3 backend dependence must remain visible in diagnostics.
- The backend owns the current `Rml::SystemInterface`, `Rml::FileInterface`, and `Rml::RenderInterface` prototype objects.
- Path resolution currently roots RmlUI assets under `qmclient/rmlui/` for relative lookups.
- Backend ownership is shared across multiple RmlUI contexts; `CRmlUiBackend` does not decide whether a surface belongs to HUD, menu, or modal context domains.

## Implementation Boundary

Do not extend this class into module registration or fallback policy.
Those belong to runtime-shell.
