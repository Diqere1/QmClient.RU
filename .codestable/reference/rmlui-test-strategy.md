---
doc_type: test-strategy
status: current
created: 2026-05-07
last_reviewed: 2026-05-07
tags: [rmlui, testing, tdd, diagnostics]
related_roadmap: rmlui-full-replacement
---

# RmlUI Test Strategy

This document defines how RmlUI work should be tested before implementation claims success. It is a documentation strategy, not proof that tests already exist.

## 1. Current Coverage

Current known test coverage:

- `src/test/rmlui_monitoring_assets_test.cpp`
- Asset schema checks for Monitoring HUD RML/RCSS.
- `src/test/rmlui_runtime_test.cpp`
- Runtime-shell and config-compat tests for global/module toggle, frame result handling, runtime-unavailable fallback, surface fallback, duplicate registration, diagnostics export gate/dedupe, and legacy/module toggle normalization.
- Safe-mode tests for trip threshold, post-trip demotion, ensure-core fallback counting, reset on success, module toggle cycle reset, and `qm_rmlui_safe_mode` toggle cycle reset.

Current gaps:

- No real exported diagnostic file test against the `dumps/QmClient_Crash/` storage path.
- No render bridge tests.
- No automated multi-backend regression evidence for OpenGL, Vulkan, or Android platform differences.

## 2. TDD Rule

All QmClient engineering work defaults to TDD.

Per-feature implementation guides are the tactical execution layer for this strategy. Use them together with the active feature checklist when implementing a specific slice.

For RmlUI feature implementation:

1. Write the failing test first.
2. Confirm the failure maps to the intended behavior.
3. Implement the smallest change to pass.
4. Keep old UI fallback behavior observable.
5. Add acceptance evidence after tests pass.

## 3. Runtime-Shell Test Matrix

| case | setup | expected |
|---|---|---|
| global disabled | `qm_rmlui_enable=0` | result `SKIPPED_DISABLED`, no core init, old HUD fallback |
| module disabled | global enabled, `qm_rmlui_monitoring_hud=0` | result `SKIPPED_DISABLED`, old HUD fallback |
| runtime unavailable | global/module enabled, backend/core unavailable | result `FALLBACK_REQUIRED`, diagnostics includes backend/core failure |
| prototype surface failure | runtime path reaches module, document/surface fails | result `FALLBACK_REQUIRED`, diagnostics distinguishes surface/document failure |
| duplicate registration | register same module twice | only one effective module or stable duplicate rejection |
| diagnostics export | forced failure with debug export enabled | diagnostic file under `dumps/QmClient_Crash/` with required fields |

## 4. Safe-Mode Test Matrix

| case | setup | expected |
|---|---|---|
| streak counts fallback | enabled module returns `FALLBACK_REQUIRED` 3 times | third failure trips safe mode |
| disabled does not count | `qm_rmlui_enable=0` or module toggle off | no streak increment, no trip |
| trip frame | third consecutive failure | result `FALLBACK_REQUIRED`, reason `safe_mode_trip`, legacy fallback still runs |
| post-trip frame | module already demoted in-session | result `SKIPPED_UNAVAILABLE`, reason `safe_mode_session_disabled` |
| reset on success | failure streak then one `RENDERED` frame | streak clears |
| reset on toggle cycle | toggle module or `qm_rmlui_safe_mode` off then on | demotion state clears |

## 5. Resource Diagnostics Test Matrix

| case | setup | expected |
|---|---|---|
| missing RML | target module document absent or injected file failure | `resource_type=rml`, `error_code=document_missing` |
| missing required font | target required font absent or injected file failure | `resource_type=font`, `error_code=font_missing`, can participate in `FALLBACK_REQUIRED` |
| missing optional icon font | optional icon font absent | structured `font_missing` event without automatic fallback |
| RCSS parse issue | unsupported property or parse warning | `resource_type=rcss`, stable error classification |
| path privacy | exported resource path | storage-relative path, not absolute OS path |
| repeated failure | same failure every frame | throttled export, no per-frame file spam |
| export gate disabled | diagnostics export not explicitly enabled | no automatic per-frame diagnostic file writes |

## 6. Render Bridge Test Matrix

These tests are for later expansion beyond the current minimal bridge slice.

| case | setup | expected |
|---|---|---|
| compiled geometry | simple triangles from RmlUI | bridge stores/handles compiled geometry |
| translated render | render geometry with translation | submitted command includes translation |
| scissor region | enable and set rectangle | submitted command applies clipping semantics |
| generated texture | RmlUI generates texture | texture handle lifecycle is tracked |
| release geometry/texture | RmlUI releases resources | bridge releases backend/CPU resources |
| no GL context grab | runtime/host path searched | no new `SDL_GL_MakeCurrent` dependency |

## 7. Manual Evidence

Some RmlUI behavior still needs manual or screenshot evidence until render bridge automation exists.

Minimum evidence per migrated surface:

- RmlUI module enabled screenshot/log.
- Legacy fallback screenshot/log.
- Diagnostics file for one forced failure.
- Statement of backend used: OpenGL, Vulkan, Android, or not covered.

## 8. Commands

Windows C++ tests:

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests
```

Windows all tests:

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_tests
```

Notes:

- Use the configured Windows build wrapper.
- Do not run multiple build commands against the same build directory in parallel.
- If adding C++ tests, place them in `src/test/` and update the root test list as required by the project build.

## 9. Acceptance Link

Feature acceptance should cite:

- Test command.
- Passing/failing status.
- Test files added or updated.
- Manual evidence if required.
- Remaining platform gaps.

## 10. Current Reality Notes

- `src/test/rmlui_runtime_test.cpp` already covers diagnostics export gate/dedupe through `DiagnosticsExportHonorsDebugGateAndDeduplicates`.
- `src/test/rmlui_runtime_test.cpp` already covers resource diagnostics storage/host observation through `ResourceDiagnosticsFieldsAreStored` and `ResourceDiagnosticsSurviveExportAndCanBeObservedByHost`.
- Render-bridge coverage is still limited to the current minimal slice; there are still no geometry/scissor/texture lifecycle bridge tests.
