---
doc_type: developer-guide
status: current
created: 2026-05-07
last_reviewed: 2026-05-10
tags: [rmlui, developer-guide, workflow]
related_roadmap: rmlui-full-replacement
---

# RmlUI Developer Guide

This guide tells implementers which RmlUI documents to read and what boundaries to respect before writing code.

## 1. Start Here

Read in this order:

1. `rmlui-full-replacement-roadmap.md`
2. `rmlui-full-replacement-readiness-matrix.md`
3. `ARCHITECTURE.md`
4. `ui-rmlui-current.md`
5. `rmlui-reference-index.md`
6. `rmlui-runtime-api-reference.md`
7. The active feature design/checklist
8. `rmlui-test-strategy.md`

Minimum upstream reference set by topic:

- Runtime/backend work: `rmlui-runtime-api-reference.md`, `rmlui-render-interface-reference.md`, `rmlui-file-interface-reference.md`, `rmlui-font-engine-reference.md`
- Interactive/input work: `rmlui-system-input-reference.md`, `rmlui-events-reference.md`, `rmlui-text-input-reference.md`
- State-driven/menu work: `rmlui-data-binding-reference.md`
- Rich widget/editor extension work: `rmlui-custom-elements-reference.md`
- Dev tooling and inspection: `rmlui-debugger-reference.md`
- Architecture and knowledge: `ARCHITECTURE.md`, `ui-rmlui-current.md`, `rmlui-developer-guide.md`, `rmlui-test-strategy.md`, and the `compound/` learn/trick/decide docs from the reference index.

Accepted baseline:

- `2026-05-07-rmlui-runtime-shell`
- `2026-05-07-rmlui-resource-diagnostics`
- `2026-05-07-rmlui-safe-mode`
- `2026-05-07-rmlui-input-bridge`
- `2026-05-08-rmlui-popup-migration`
- Evidence anchor: `features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-acceptance.md`

Current next candidates:

- `2026-05-07-rmlui-render-command-bridge` is in the "minimal slice implemented, full bridge still pending" state and should be reviewed against the current bridge/readiness explores before any broader bridge claims.
- `2026-05-08-rmlui-menu-pilot` is the next page-level interactive migration candidate and should reuse the accepted popup/input/safe-mode baseline instead of redefining host input and fallback policy.

## 2. Current Truths

- RmlUI is optional and disabled by default.
- Old UI remains a permanent supported path.
- Monitoring HUD is only a prototype host.
- Current backend path is GL3 prototype state.
- Vulkan and Android must not be made second-class by baking GL assumptions into runtime APIs.
- QmClient config keys use `qm_` / `Qm` prefixes.
- Engineering work defaults to TDD.
- Menu/page/modal surfaces and HUD/overlay surfaces must not silently share one `Rml::Context`; if they need independent focus, input, or render lifecycles, split the contexts first and keep that boundary explicit in design and code.

## 3. What Not To Do

- Do not implement from a draft design.
- Do not mark a roadmap item in-progress unless design is approved and checklist exists.
- Do not remove legacy fallback.
- Do not use `cl_` names for QmClient-owned config.
- Do not write placeholder modules/docs/stubs as deliverables.
- Do not treat current Monitoring HUD rendering success as proof of a stable RmlUI pipeline.
- Do not add new GL context acquisition in runtime-shell.
- Do not reintroduce the old shared-context pattern where multiple independent modules each call `Context::Update()/Render()` on the same context.

## 4. Runtime-Shell Baseline

Use these docs:

- `features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-design.md`
- `features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-checklist.yaml`
- `features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-implementation-guide.md`
- `features/2026-05-07-rmlui-runtime-shell/rmlui-runtime-shell-acceptance.md`

Accepted outcome:

- Establish runtime/module/fallback/diagnostics behavior.
- Keep rendering prototype behavior contained.
- Prove disabled and fallback paths before adding more UI surfaces.

## 5. Resource Diagnostics Path

Use these docs:

- `features/2026-05-07-rmlui-resource-diagnostics/rmlui-resource-diagnostics-design.md`
- `features/2026-05-07-rmlui-resource-diagnostics/rmlui-resource-diagnostics-checklist.yaml`

Current status:

- Design approved and acceptance passed.
- Checklist and acceptance exist.
- Runtime/resource diagnostics baseline is already implemented and can be treated as accepted downstream baseline, while exporter ownership still has Monitoring HUD host-specific residue.

Implementation goal:

- Convert font/RML/RCSS/file failures into structured diagnostics.
- Export diagnostics to `dumps/QmClient_Crash/`.
- Avoid per-frame file spam.

This should follow or closely pair with runtime-shell diagnostics fields.

## 6. Render Bridge Path

Use this accepted design plus the current readiness explore:

- `features/2026-05-07-rmlui-render-command-bridge/rmlui-render-command-bridge-design.md`
- `compound/2026-05-07-explore-rmlui-render-bridge-readiness.md`

Current review focus:

- Confirm vendored RmlUI `RenderInterface` signatures.
- Confirm QmClient graphics submission primitives.
- Decide compiled geometry and texture handle ownership.

Do not describe the current state as a full backend-neutral render bridge.

## 6.5 Input and Event Path

When reviewing or extending the accepted `rmlui-input-bridge` baseline:

- Read `rmlui-system-input-reference.md`
- Read `rmlui-events-reference.md`
- Read `rmlui-text-input-reference.md`
- Read `compound/2026-05-07-explore-rmlui-input-bridge-readiness.md`

Check explicitly:

- host input submission boundary
- event propagation and default-action consequences
- text-input handler installation scope and context lifetime
- legacy fallback owner and release-state sequencing for the concrete interactive surface being added

Do not review interactive surface behavior from raw `ProcessKey*` / `ProcessMouse*` API names alone.

## 6.6 Data and Widget Path

Before reviewing settings/menu/editor-style surfaces:

- Read `rmlui-data-binding-reference.md`
- Read `rmlui-custom-elements-reference.md`

Check explicitly:

- whether the surface uses manual DOM mutation or data binding
- whether any proposed custom widget actually needs custom elements/decorators
- whether extension ownership and backend assumptions are documented

## 7. Evidence Discipline

Every implemented RmlUI feature should return evidence:

- Tests or reason tests are not possible yet.
- Logs or diagnostics file.
- Legacy fallback proof.
- Backend/platform coverage statement.
- Architecture backfill note if implementation changed current system shape.

## 8. Platform Notes

OpenGL:

- May be first backend to prove bridge shape.
- Must not leak GL context assumptions into runtime APIs.

Vulkan:

- Must share runtime/module/layer/diagnostics protocols.
- Unsupported status should be explicit, not silent.

Android:

- Back/cancel, focus loss, drawable size, and asset path differences matter.
- Do not assume desktop working directory behavior.
- Text input and IME lifecycle must be reviewed explicitly for Android-facing surfaces.

## 9. When To Update Architecture

Update architecture only after implementation acceptance.

Safe to backfill after runtime-shell acceptance:

- runtime shell responsibility
- module descriptor fields
- frame result semantics
- diagnostics ownership

Do not backfill before implementation:

- render command bridge
- layer switchboard
- input bridge
- Vulkan/Android backend support
