---
doc_type: libdoc
status: current
created: 2026-05-07
last_reviewed: 2026-05-07
tags: [rmlui, custom-elements, decorators, api, reference]
related_roadmap: rmlui-full-replacement
external_docs:
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/custom_elements
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/decorators
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/interfaces/render
---

# RmlUI Custom Elements and Decorators Reference

Source basis: official RmlUi custom element, decorator, and render-interface documentation.

This reference covers upstream extension points that may become relevant for richer editor/widgets work, but are not part of the current runtime-shell core.

## Custom Element Instancer

### `Rml::ElementInstancer`

The instancer is responsible for creating and destroying custom elements.

Required methods:

- `Rml::ElementPtr InstanceElement(Rml::Element* parent, const Rml::String& tag, const Rml::XMLAttributes& attributes)`
- `void ReleaseElement(Rml::Element* element)`

Behavior notes:

- `parent` is only non-null when instanced from RML.
- Returning `nullptr` from `InstanceElement(...)` signals an instancing error.

QmClient implication:

- If future menu/editor surfaces need custom widgets, the instancer boundary should stay at element creation/destruction, not absorb host runtime or bridge responsibilities.

## Decorators

### `Rml::Decorator`

Decorators are per-element render extensions.

Required methods:

- `GenerateElementData(...)`
- `ReleaseElementData(...)`
- `RenderElement(...)`

Meaning:

- Decorators may allocate per-element data.
- That data must be released through the paired release call.

Review implication:

- Decorator proposals must include ownership and release semantics.
- They should not be treated as substitutes for the render bridge or module system.

## Shader Extension Hooks

Official render-interface extension points also include:

- `CompileShader(...)`
- `RenderShader(...)`
- `ReleaseShader(...)`

Meaning:

- Advanced appearance extensions may rely on shader compilation and rendering hooks.

Boundary:

- This is upstream render-interface capability.
- It does not imply QmClient should implement shader-backed RmlUI extensions early.

## Integration Notes

- Custom elements are a later-stage extension topic, not a prerequisite for runtime-shell.
- Decorators and shaders belong to surface/render extension review, not fallback/runtime ownership review.
- If these mechanisms are adopted later, the review baseline should demand:
  - explicit ownership/lifetime rules
  - backend compatibility statement
  - fallback behavior when the extension is unavailable
