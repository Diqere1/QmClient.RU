---
doc_type: reference_index
status: current
created: 2026-05-07
last_reviewed: 2026-05-10
tags: [rmlui, reference, index]
related_roadmap: rmlui-full-replacement
---

# RmlUI Reference Index

This index points to the reference-layer docs that describe the current QmClient RmlUI surface and the upstream RmlUi APIs used by it.

## Upstream API References

- [RmlUI Runtime API Reference](rmlui-runtime-api-reference.md)
- [RmlUI Render Interface Reference](rmlui-render-interface-reference.md)
- [RmlUI File Interface Reference](rmlui-file-interface-reference.md)
- [RmlUI System and Input Reference](rmlui-system-input-reference.md)
- [RmlUI Events Reference](rmlui-events-reference.md)
- [RmlUI Text Input Reference](rmlui-text-input-reference.md)
- [RmlUI Font Engine Reference](rmlui-font-engine-reference.md)
- [RmlUI Data Binding Reference](rmlui-data-binding-reference.md)
- [RmlUI Custom Elements and Decorators Reference](rmlui-custom-elements-reference.md)
- [RmlUI Debugger Reference](rmlui-debugger-reference.md)

## QmClient Surface References

- [CRmlUiBackend Reference](rmlui-backend-reference.md)
- [CRmlUiCore Reference](rmlui-core-reference.md)
- [CRmlUiMonitoringHud Reference](rmlui-monitoring-hud-reference.md)
- [RmlUiRenderHelpers Reference](rmlui-render-helpers-reference.md)
- [GameClient RmlUI Host Reference](rmlui-gameclient-host-reference.md)

## Usage Notes

- Read upstream API references first when designing or reviewing integration behavior.
- For runtime or backend work, start with runtime/render/file/font.
- For interactive UI work, also read events/text-input/data-binding references before reviewing design or code.
- For richer widgets or editor-like surfaces, read custom-elements/decorators before proposing local abstractions.
- Read QmClient surface references when you need the current prototype code shape.
- Do not assume the Monitoring HUD prototype is a stable reusable base.
- When host work touches both menu/modal and HUD/overlay surfaces, explicitly verify whether the context boundary is documented; do not assume one shared context is acceptable just because the documents are all “RmlUI”.
