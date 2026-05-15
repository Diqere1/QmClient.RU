---
doc_type: libdoc
status: current
created: 2026-05-07
last_reviewed: 2026-05-07
tags: [rmlui, render-interface, api, reference]
related_roadmap: rmlui-full-replacement
external_docs:
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/interfaces/render
---

# RmlUI Render Interface Reference

Source basis: official RmlUi render-interface documentation.

This reference covers the upstream `Rml::RenderInterface` contract used by future backend-agnostic bridge work.

## Geometry API

### `Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)`

Compiles vertex and index data into a reusable geometry handle.

Notes:

- `indices` are triangle indices and are expected to come in multiples of three.
- `0` is reserved for an invalid handle.

### `void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture)`

Renders a previously compiled geometry handle.

Notes:

- Translation is in pixel offsets from the current context origin.
- `texture` is `0` for untextured geometry.

### `void ReleaseGeometry(Rml::CompiledGeometryHandle geometry)`

Releases resources associated with a compiled geometry handle.

## Scissor API

### `void EnableScissorRegion(bool enable)`

Enables or disables clipping for subsequent render calls.

### `void SetScissorRegion(Rml::Rectanglei region)`

Sets the current clipping rectangle in pixel coordinates.

## Texture API

### `Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source)`

Loads a texture from a source path.

### `Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions)`

Creates a texture from raw pixel data.

### `void ReleaseTexture(Rml::TextureHandle texture)`

Releases a loaded or generated texture.

## Integration Notes

- The upstream render interface is handle-based, not direct GL draw-call based.
- QmClient render-bridge work should target these handle and scissor semantics.
- Do not treat the current GL3 helper path as the long-term contract.
