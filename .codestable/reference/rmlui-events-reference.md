---
doc_type: libdoc
status: current
created: 2026-05-07
last_reviewed: 2026-05-07
tags: [rmlui, events, api, reference]
related_roadmap: rmlui-full-replacement
external_docs:
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/events
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/input
---

# RmlUI Events Reference

Source basis: official RmlUi events and input documentation.

This reference covers the upstream event object, propagation phases, and the behavioral consequences of host input submission.

## Event Object

### `Rml::Event`

The event object represents one dispatched event in RmlUi.

Relevant public API:

- `Rml::EventPhase GetPhase() const`
- `Rml::Element* GetCurrentElement() const`
- `Rml::Element* GetTargetElement() const`
- `const Rml::String& GetType() const`
- `EventId GetId() const`
- `void StopPropagation()`
- `void StopImmediatePropagation()`
- `template <typename T> T GetParameter(const Rml::String& key, const T& default_value)`

## Propagation Phases

### `EventPhase::Capture`

The event is propagating down toward the target.

### `EventPhase::Target`

The event is being delivered to the target element itself.

### `EventPhase::Bubble`

The event is propagating upward through target ancestors.

## Target vs Current Element

- `GetTargetElement()` returns the original event target.
- `GetCurrentElement()` returns the element currently handling the event during propagation.

Integration implication:

- Host code must not assume the listener element and the original event target are the same.
- Input and UI-layer bridge review should reason in terms of propagation, not just focused element state.

## Interruption Semantics

### `StopPropagation()`

Stops further propagation when the event type is interruptible, but lets remaining listeners on the current element finish.

### `StopImmediatePropagation()`

Stops further propagation and also stops any remaining listeners on the current element.

Important behavior:

- If propagation is interrupted, default actions are not processed.

QmClient implication:

- Future input-bridge and interactive surface review must distinguish:
  - host decides whether to submit input into RmlUi
  - RmlUi event listeners decide whether propagation/default action continues
- “event consumed” is not the same as “host must suppress all legacy handling” unless the bridge contract says so.

## Event Parameters

Use `GetParameter()` to retrieve event-specific payload values.

Examples from host-review perspective:

- pointer positions
- key identifiers
- wheel deltas
- focus or form-related values

Rule:

- Event parameter usage belongs in surface/controller code.
- Runtime-shell should not become an event-parameter interpreter.

## Input Submission Relationship

Input APIs such as:

- `ProcessMouseMove(...)`
- `ProcessMouseButtonDown(...)`
- `ProcessMouseButtonUp(...)`
- `ProcessMouseWheel(...)`
- `ProcessKeyDown(...)`
- `ProcessKeyUp(...)`

cause RmlUi to generate and dispatch events internally.

Important boundary:

- Input submission is a host responsibility.
- Event propagation is an upstream RmlUi responsibility after submission.

## Integration Notes

- Review interactive RmlUI surfaces against propagation rules, not only raw input APIs.
- Do not collapse event propagation, focus behavior, and host fallback policy into one layer.
- Input-bridge design should explicitly define how host-level “consumed” maps to RmlUi event/default-action outcomes.
