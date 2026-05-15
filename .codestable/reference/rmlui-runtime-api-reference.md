---
doc_type: libdoc
status: current
created: 2026-05-07
last_reviewed: 2026-05-10
tags: [rmlui, runtime, api, reference]
related_feature: 2026-05-07-rmlui-runtime-shell
related_roadmap: rmlui-full-replacement
external_docs:
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/main_loop
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/interfaces/render
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/interfaces/file
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/interfaces/font_engine
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/integrating
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/input
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/fonts
---

# RmlUI Runtime API Reference

This reference separates three layers:

- Upstream RmlUI API behavior from Context7 / official RmlUI documentation.
- Current QmClient prototype code.
- Runtime-shell contract that is ready for implementation.

It must not be read as evidence that the runtime shell already exists in code.

## Upstream RmlUI API Notes

Source basis: Context7 lookup for `/websites/mikke89_github_io_rmluidoc`, using the official RmlUI C++ manual pages for main loop, render interface, and fonts.

### Initialization Order

Official RmlUI integration flow:

1. Create application/window/graphics objects first.
2. Install render and system interfaces with `Rml::SetRenderInterface(...)` and `Rml::SetSystemInterface(...)`.
3. Call `Rml::Initialise()`.
4. Create one or more contexts with `Rml::CreateContext(name, Rml::Vector2i(width, height))`.
5. Load fonts before loading documents.
6. Load a document with `context->LoadDocument(...)`.
7. Show the document.
8. Submit input before `Context::Update()`.
9. Call `Context::Update()`.
10. Render the game/application.
11. Render RmlUI on top with `Context::Render()`.
12. Call `Rml::Shutdown()` before destroying interfaces.

QmClient implication:

- Runtime-shell must respect the order “interfaces/core/context before document”.
- Runtime-shell should not submit input or mutate document state between `Context::Update()` and `Context::Render()`.
- The current Monitoring HUD prototype can validate order/fallback, but render ordering still needs the later render-command bridge.
- If menu/page/modal surfaces and HUD/overlay surfaces need independent input state, focus, or render lifecycles, they should use separate contexts instead of sharing one context and taking turns to render it.

### Context API

Official context calls:

- `Rml::CreateContext("default", Rml::Vector2i(width, height))`
- `context->LoadDocument("document.rml")`
- `context->Update()`
- `context->Render()`

QmClient implication:

- Runtime-shell frame request must carry viewport dimensions.
- Document load failure must be distinguishable from backend/core failure.
- Update/render failure should be reflected in frame result and diagnostics.
- A single `Rml::Context` renders all visible documents inside that context; QmClient hosts must not model independent surfaces as if each module owned a private render pass while still sharing one context.
- Full-screen replacement surfaces should own a dedicated context sized to the real viewport; settings-specific host policy now lives in `rmlui-settings-host-contract.md`.

### Font Loading

Official font call:

- `Rml::LoadFontFace(file_path, face_index, fallback_face, weight)`

Official behavior:

- Fonts should be loaded before documents.
- The path is passed through the configured file interface.

QmClient implication:

- Font/resource failures belong in diagnostics.
- Missing fonts must not be treated as successful runtime initialization.
- Runtime-shell should not solve the full resource pipeline, but it must expose enough failure context for `rmlui-resource-diagnostics`.

### RenderInterface API

Official render interface concepts:

- `CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)`
- `RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture)`
- `ReleaseGeometry(Rml::CompiledGeometryHandle geometry)`
- `EnableScissorRegion(bool enable)`
- `SetScissorRegion(Rml::Rectanglei region)`
- `LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source)`
- `GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions)`
- `ReleaseTexture(Rml::TextureHandle texture)`

QmClient implication:

- Future render-command bridge should target compiled geometry handles, scissor rectangles, and texture lifecycle.
- Older direct `RenderGeometry(vertices, indices, ...)` style should not be used as the long-term design basis without checking the vendored RmlUI version.
- Runtime-shell must avoid baking old GL3 helper assumptions into public runtime contracts.

## Current Code Surface

Settings-specific host rules and IA live in [RmlUI Settings Host Contract](rmlui-settings-host-contract.md).

### CRmlUiBackend

Path: `src/engine/client/rmlui_backend.h/.cpp`

Current responsibility:

- Initializes the RmlUI backend interfaces.
- Creates the current GL3 render interface.
- Tracks backend initialization failure state.
- Provides viewport and frame hooks for the current GL3 path.

Important current constraints:

- `Init(IStorage *pStorage)` currently requires an active OpenGL context.
- The implementation constructs `RmlGL3` / `RenderInterface_GL3`.
- `SetViewport`, `BeginFrame`, and `EndFrame` are currently GL3 render-interface forwards.
- This is current state only. It is not the long-term multi-backend render bridge.

Known failure labels:

- `none`
- `storage_unavailable`
- `no_gl_context`
- `gl3_init_failed`
- `render_interface_failed`

### CRmlUiCore

Path: `src/game/client/RmlUi/RmlUiCore.h/.cpp`

Current responsibility:

- Owns RmlUI core lifecycle plus multiple named context domains over the shared backend.
- Initializes backend/core integration.
- Updates viewport and RmlUI context per domain.
- Exposes availability and last initialization failure information.

Boundary:

- `CRmlUiCore` is not the full runtime shell.
- It does not own module registration, layer routing, module toggles, or fallback policy.
- The runtime-shell feature may reuse it internally, but should not expand it into a module registry.
- Current context split is at least `HUD` and `MENU`; future feature work must explicitly justify any additional sharing or splitting instead of silently collapsing back to one shared context.

### CRmlUiMonitoringHud

Path: `src/game/client/RmlUi/RmlUiMonitoringHud.h/.cpp`

Current responsibility:

- Loads `data/qmclient/rmlui/monitoring_hud.rml`.
- Uses `data/qmclient/rmlui/monitoring_hud.rcss`.
- Updates Monitoring HUD document data.
- Attempts to render the Monitoring HUD prototype surface.

Boundary:

- This is a prototype host, not a stable reusable base.
- Current graph/layout/resource problems must not be inherited as runtime assumptions.
- Surface failure must remain distinguishable from runtime/backend failure.

### RmlUiRenderHelpers

Path: `src/game/client/RmlUi/RmlUiRenderHelpers.h/.cpp`

Current responsibility:

- Provides local helper behavior for the Monitoring HUD prototype.

Boundary:

- This is not a general render bridge.
- It must not become the place for module registry, fallback policy, or platform backend decisions.

## Companion References

Use these when you need the split-out API facts instead of the combined runtime summary:

- [RmlUI Render Interface Reference](rmlui-render-interface-reference.md)
- [RmlUI File Interface Reference](rmlui-file-interface-reference.md)
- [RmlUI System and Input Reference](rmlui-system-input-reference.md)
- [RmlUI Font Engine Reference](rmlui-font-engine-reference.md)
- [RmlUI Reference Index](rmlui-reference-index.md)

## Runtime-Shell Contract

The following contract comes from `rmlui-runtime-shell-design.md` and is implementation-ready, but not yet implemented. It intentionally stays above the upstream `Rml::RenderInterface` API so runtime-shell can be implemented before render-command bridge.

### ERmlUiLayer

Purpose: identifies where an RmlUI module participates in the UI order.

Required values for the first implementation:

- `GAME_HUD`
- `DEBUG_OVERLAY`
- `MENU_PAGE`
- `MENU_MODAL`
- `RADIAL_OVERLAY`
- `EDITOR_OVERLAY`

Runtime-shell scope:

- The first implementation only needs `GAME_HUD` for Monitoring HUD.
- The enum should still reserve the roadmap layer names so later features do not invent new naming.

### ERmlUiFrameResult

Purpose: tells the host whether runtime rendered, skipped, or requires legacy fallback.

Required values:

- `RENDERED`: runtime completed the requested layer path.
- `SKIPPED_DISABLED`: global or module toggle prevents RmlUI from running.
- `SKIPPED_UNAVAILABLE`: runtime exists but no usable module/runtime path is available.
- `FALLBACK_REQUIRED`: an enabled path failed and the host must run legacy UI.

Host rule:

- Runtime does not draw legacy fallback.
- The host owner consumes the result and invokes the legacy path.

### SRmlUiModuleDescriptor

Purpose: the stable declaration for one RmlUI surface.

Required fields:

- `m_pModuleName`: stable diagnostic/module name, for example `monitoring_hud`.
- `m_pConfigToggle`: QmClient config key, using `qm_` / `Qm` naming.
- `m_Layer`: one `ERmlUiLayer`.
- `m_pDocumentPath`: RML document path when the module has a document.
- `m_RequiresInput`: whether the module needs input routing.
- `m_HasLegacyFallback`: whether a legacy path exists.
- `m_pFallbackOwner`: host function/component that owns fallback execution.

Runtime-shell first module:

- `m_pModuleName = "monitoring_hud"`
- `m_pConfigToggle = "qm_rmlui_monitoring_hud"`
- `m_Layer = GAME_HUD`
- `m_RequiresInput = false`
- `m_HasLegacyFallback = true`
- `m_pFallbackOwner = "CGameClient::RenderQmMonitoringHud"`

Prototype warning:

- The Monitoring HUD module is only the first prototype host.
- It must not be used as proof that the current surface implementation is stable.

### SRmlUiFrameRequest

Purpose: per-frame request from a legacy host into runtime.

Required fields:

- `m_Layer`
- `m_ViewportWidth`
- `m_ViewportHeight`
- `m_FrameTimeSec`
- `m_DebugDiagnostics`

Rule:

- The request carries viewport data.
- Runtime-shell must not acquire a new GL context or introduce new backend-specific window assumptions.

### SRmlUiFrameResult

Purpose: per-frame response to the host.

Required fields:

- `m_Result`
- `m_pFailureReason`

Failure reason expectations:

- Disabled paths should produce stable disabled reasons.
- Runtime/backend failures should remain distinct from surface/document failures.
- Failure strings must be suitable for diagnostics and tests.

### SRmlUiDiagnostics

Purpose: structured runtime snapshot for logs/files.

Minimum fields:

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

File target:

- Development diagnostics should target the existing saved diagnostics area under `dumps/QmClient_Crash/`.
- The earlier Monitoring HUD `log/` output is a prototype state, not the target convention.

## Config Naming

QmClient-owned RmlUI config keys use the `qm_` / `Qm` prefix.

Runtime-shell keys:

- `qm_rmlui_enable`
- `qm_rmlui_safe_mode`
- `qm_rmlui_monitoring_hud`

Compatibility note:

- Existing `qm_monitoring_use_rmlui` may need a migration or compatibility bridge during implementation.
- The implementation plan must state how both names interact.

## Implementation Guardrails

- Start with TDD.
- Do not add new `SDL_GL_MakeCurrent` or `SDL_GL_GetCurrentContext` use points for runtime shell.
- Do not modify Monitoring HUD graph/layout/RCSS behavior in runtime-shell.
- Do not delete legacy HUD fallback.
- Do not describe GL3 as the long-term render bridge.
- Do not write target RenderBridge/LayerManager/InputBridge modules into architecture as current state until acceptance proves them.
