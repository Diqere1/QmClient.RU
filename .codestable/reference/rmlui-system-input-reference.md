---
doc_type: libdoc
status: current
created: 2026-05-07
last_reviewed: 2026-05-07
tags: [rmlui, system-interface, input, reference]
related_roadmap: rmlui-full-replacement
external_docs:
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/integrating
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/input
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/interfaces/text_input_handler
---

# RmlUI System and Input Reference

Source basis: official RmlUi integrating, input, and text input handler documentation.

This reference covers the upstream system interface responsibilities and the input submission APIs used by host code.

## System Interface Responsibilities

### `double GetElapsedTime()`

Provides elapsed time to RmlUi.

### `void JoinPath(Rml::String &TranslatedPath, const Rml::String &DocumentPath, const Rml::String &Path)`

Resolves relative resource paths.

### `bool LogMessage(Rml::Log::Type Type, const Rml::String &Message)`

Receives RmlUi log output.

### `void SetMouseCursor(const Rml::String &CursorName)`

Requests a cursor change.

## Context Input Submission

### Mouse

- `bool ProcessMouseMove(int x, int y, int key_modifier_state)`
- `bool ProcessMouseButtonDown(int button_index, int key_modifier_state)`
- `bool ProcessMouseButtonUp(int button_index, int key_modifier_state)`
- `bool ProcessMouseWheel(float wheel_delta, int key_modifier_state)`

### Keyboard

- `bool ProcessKeyDown(Rml::Input::KeyIdentifier key_identifier, int key_modifier_state)`
- `bool ProcessKeyUp(Rml::Input::KeyIdentifier key_identifier, int key_modifier_state)`

## Text Input Interfaces

### `Rml::TextInputHandler`

Used for IME and editable text lifecycle hooks.

Relevant callbacks:

- `OnActivate(TextInputContext* input_context)`
- `OnDeactivate(TextInputContext* input_context)`
- `OnDestroy(TextInputContext* input_context)`

### `Rml::TextInputContext`

Editable text proxy used by text fields and IME flows.

Relevant operations:

- `GetBoundingBox(...)`
- `GetSelectionRange(...)`
- `SetSelectionRange(...)`
- `SetCursorPosition(...)`
- `SetText(...)`
- `SetCompositionRange(...)`
- `CommitComposition(...)`

## Ordering Notes

- Input should be submitted before `Context::Update()`.
- After `Context::Update()`, element state should not be mutated until the frame render flow has completed.
- RmlUi does not translate physical keys into text; the host owns text entry composition.

## Integration Notes

- QmClient input bridge work should use these submission boundaries.
- Input consumption decisions belong to the host/layer bridge, not to the raw system interface itself.
