---
doc_type: acceptance-template
feature: 2026-05-07-rmlui-runtime-shell
status: template
created: 2026-05-07
tags: [rmlui, runtime, acceptance, evidence]
related_design: rmlui-runtime-shell-design.md
related_checklist: rmlui-runtime-shell-checklist.yaml
---

# rmlui-runtime-shell acceptance template

This is not an acceptance result. It is the evidence template to fill after implementation.

## 1. Summary

Implementation status:

- [ ] Complete
- [ ] Partial
- [ ] Blocked

Acceptance verdict:

- [ ] Pass
- [ ] Needs fix
- [ ] Unsafe

## 2. Checklist Step Results

| step | status | evidence |
|---|---|---|
| TDD baseline | pending | |
| Runtime shell and frame result types | pending | |
| Monitoring HUD prototype module registration | pending | |
| RenderQmMonitoringHud host integration | pending | |
| Structured diagnostics export | pending | |
| Config naming and cleanup | pending | |

## 3. Required Evidence

### 3.1 TDD Evidence

Required:

- Test proving global-off path.
- Test proving module-off path.
- Test proving runtime-unavailable fallback path.
- Test proving duplicate module registration behavior.

Evidence location:

- Test command:
- Test output summary:
- Relevant test file:

### 3.2 Runtime Result Evidence

Required frame results:

- `SKIPPED_DISABLED`
- `FALLBACK_REQUIRED`
- `RENDERED` or a documented reason why the runtime-shell slice only validates render attempt/fallback

Evidence location:

- Log:
- Test:
- Manual run:

### 3.3 Fallback Evidence

Required:

- Legacy HUD still renders when global RmlUI is disabled.
- Legacy HUD still renders when Monitoring HUD RmlUI module is disabled.
- Legacy HUD still renders when runtime/backend/core fails.
- Legacy HUD still renders when Monitoring HUD prototype surface fails.

Evidence location:

- Screenshot/log:
- Manual run notes:

### 3.4 Diagnostics Evidence

Required fields:

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

Evidence location:

- Diagnostic file path:
- Sample result:

### 3.5 Config Evidence

Required:

- `qm_rmlui_enable` behavior documented.
- `qm_rmlui_safe_mode` state or placeholder behavior documented without claiming safe-mode automation is implemented.
- `qm_rmlui_monitoring_hud` behavior documented.
- Existing `qm_monitoring_use_rmlui` compatibility/migration behavior documented.

Evidence location:

- Code/config reference:
- Manual run notes:

### 3.6 Backend Boundary Evidence

Required:

- No new `SDL_GL_MakeCurrent` use.
- No new runtime-shell `SDL_GL_GetCurrentContext` dependency.
- Existing GL3 backend limitation remains classified as backend/prototype state, not runtime-shell target architecture.

Evidence location:

- Search command:
- Search result summary:

## 4. Architecture Backfill Notes

Only fill after implementation passes.

Architecture entries to update:

- RmlUI runtime shell responsibility.
- RmlUI module descriptor stable fields.
- Runtime frame request/result semantics.
- Diagnostics target and failure taxonomy.

Do not backfill as current state:

- Render command bridge.
- Layer manager beyond implemented runtime-shell behavior.
- Input bridge.
- Vulkan/Android backend implementation.

## 5. Open Issues

Use this section for real unresolved issues found during implementation.

- None yet.

## 6. Final Acceptance Statement

Acceptance should state:

- What was implemented.
- What evidence proves it.
- What remains out of scope.
- Whether roadmap item `rmlui-runtime-shell` can move to `done`.
