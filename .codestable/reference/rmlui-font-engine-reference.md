---
doc_type: libdoc
status: current
created: 2026-05-07
last_reviewed: 2026-05-07
tags: [rmlui, font-engine, api, reference]
related_roadmap: rmlui-full-replacement
external_docs:
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/interfaces/font_engine
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/fonts
---

# RmlUI Font Engine Reference

Source basis: official RmlUi font-engine and font-loading documentation.

This reference covers the upstream font-loading entry points and font-engine responsibilities relevant to QmClient diagnostics.

## Font Loading API

### `bool LoadFontFace(const String& file_name, int face_index, bool fallback_face, Style::FontWeight weight)`

Loads a font face from a file.

Notes:

- Fonts should be loaded before documents.
- The host file interface determines how the file is opened.
- Missing fonts are a resource failure, not a successful initialization state.

## Font Engine Responsibilities

The upstream font engine is responsible for:

- resolving font face handles
- preparing font effects
- providing metrics
- measuring string width
- generating string geometry

Relevant upstream methods include:

- `GetFontFaceHandle(...)`
- `PrepareFontEffects(...)`
- `GetFontMetrics(...)`
- `GetStringWidth(...)`
- `GenerateString(...)`

## Integration Notes

- QmClient should treat font failures as structured diagnostics input.
- Font loading belongs before document loading in the runtime order.
- Do not let prototype success hide missing font coverage.
