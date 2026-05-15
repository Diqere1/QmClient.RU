---
doc_type: libdoc
status: current
created: 2026-05-07
last_reviewed: 2026-05-07
tags: [rmlui, text-input, ime, api, reference]
related_roadmap: rmlui-full-replacement
external_docs:
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/interfaces/text_input_handler
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/input
---

# RmlUI Text Input Reference

Source basis: official RmlUi text input handler and input documentation.

This reference focuses on IME and editable-text lifecycle behavior that should not be buried inside the broader system-input reference.

## `Rml::TextInputHandler`

The text input handler is the upstream extension point for IME and editable text integration.

Relevant callbacks:

- `OnActivate(TextInputContext* input_context)`
- `OnDeactivate(TextInputContext* input_context)`
- `OnDestroy(TextInputContext* input_context)`

Important upstream note:

- Implementing this handler is optional.
- It does not block basic application functionality if omitted.

## Installation Scope

The handler can be installed:

- globally with `Rml::SetTextInputHandler()`
- or during context construction for a context-specific handler

Important lifecycle rule:

- Replacing the global handler does not affect already existing contexts.

QmClient implication:

- Input-bridge design must decide whether text input handling is global runtime state or context-specific UI state.
- That decision must be documented explicitly because upstream allows both scopes.

## `Rml::TextInputContext`

The text input context is the proxy for a concrete editable text area.

Relevant operations:

- `GetBoundingBox(...)`
- `GetSelectionRange(...)`
- `SetSelectionRange(...)`
- `SetCursorPosition(...)`
- `SetText(...)`
- `SetCompositionRange(...)`
- `CommitComposition(...)`

Meaning:

- `SetText(...)` can replace a range directly.
- `CommitComposition(...)` is the IME-friendly path that respects internal restrictions.

## Lifetime Rule

The lifetime of a `TextInputContext` ends when `OnDestroy()` is called on the handler.

Strict host rule:

- After `OnDestroy()`, the handler must not interact with that same context instance again.

This is a high-value review point for:

- Android text input
- menu/search/settings text fields
- focus transitions
- document teardown during UI mode switches

## Integration Notes

- Raw key submission and text/IME composition are separate concerns.
- `ProcessKeyDown/Up(...)` do not replace host responsibility for text entry composition.
- Text input review should always check:
  - handler installation scope
  - context lifetime after deactivate/destroy
  - composition commit behavior during focus changes and document removal
