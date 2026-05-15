---
doc_type: libdoc
status: current
created: 2026-05-07
last_reviewed: 2026-05-07
tags: [rmlui, debugger, diagnostics, api, reference]
related_roadmap: rmlui-full-replacement
external_docs:
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/debugger
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/main_loop
---

# RmlUI Debugger Reference

Source basis: official RmlUi debugger and main-loop documentation.

This reference covers the upstream debugger plugin and its boundary relative to QmClient diagnostics and logging.

## Upstream Debugger

RmlUi ships with a visual debugger plugin for development use.

From the official integration example:

- include `RmlUi/Core/Debugger.h`
- initialize debugger after creating the context
- toggle visibility from host input, commonly with a debug key such as `F8`

## Debugger Lifecycle

Typical usage pattern:

1. Create the RmlUi context.
2. Call `Rml::Debugger::Initialise(context)`.
3. Toggle visibility from host-side debug controls.

Boundary:

- The debugger is a development aid.
- It is not a replacement for structured runtime diagnostics or crash/dump exports.

## Relationship to QmClient Diagnostics

QmClient-specific diagnostics currently include:

- structured logs
- runtime failure reasons
- exported diagnostic files under `dumps/QmClient_Crash/`

Review rule:

- Use the debugger for interactive UI inspection.
- Use diagnostics/log export for host/runtime/backend/resource failure analysis.

These concerns should stay separate.

## Integration Notes

- Do not design runtime-shell around the assumption that debugger availability is required.
- Do not treat debugger visibility as evidence that the backend/resource pipeline is healthy.
- If QmClient later exposes debugger support, the design should specify:
  - dev-only vs release visibility
  - context selection
  - input binding ownership
  - interaction with existing diagnostics toggles
