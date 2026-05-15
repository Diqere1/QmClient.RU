---
doc_type: libdoc
status: current
created: 2026-05-07
last_reviewed: 2026-05-07
tags: [rmlui, data-binding, api, reference]
related_roadmap: rmlui-full-replacement
external_docs:
  - https://mikke89.github.io/RmlUiDoc/pages/data_bindings
  - https://mikke89.github.io/RmlUiDoc/pages/data_bindings/examples
---

# RmlUI Data Binding Reference

Source basis: official RmlUi data binding documentation and examples.

This reference covers upstream data-model APIs and authoring rules relevant to future interactive menus, settings pages, and richer HUD/editor surfaces.

## Data Model Construction

### `Context::CreateDataModel(...)`

Creates a named data model on a context.

Typical flow:

1. Create the data model constructor from the target `Context`.
2. Register array and struct types before binding variables of those types.
3. Bind model properties and event callbacks.
4. Retrieve and retain the model handle.

## Type Registration

### Arrays

Use `RegisterArray<T>()` for container types that will be iterated or exposed as arrays.

### Structs

Use `RegisterStruct<T>()`, then register members explicitly.

Common member registration patterns:

- direct field member
- getter-based member

Rule:

- Underlying element or struct types must be registered before arrays of those types.

## Data Binding Operations

Relevant constructor operations:

- `Bind("name", &value)`
- `BindEventCallback("event_name", callback, user_data)`

Meaning:

- `Bind(...)` exposes C++ state to the model.
- `BindEventCallback(...)` maps RmlUi event hooks back into host-side behavior.

## Authoring Rules

The official data binding docs reserve:

- element attributes starting with `data-`
- `{{ ... }}` template expressions inside RML documents

Example binding authoring patterns from upstream docs:

- `data-value`
- `data-event-click`
- `data-for`
- `data-if`
- `data-class-*`
- `data-attr-*`
- `data-style-*`

QmClient implication:

- Reference or style review must not treat these prefixes as arbitrary custom attributes.
- Future settings/menu/HUD editor surfaces should decide explicitly whether they adopt data binding, rather than drifting into ad-hoc manual DOM mutation.

## Integration Notes

- Data binding is a likely fit for menu/settings style surfaces, not a requirement for runtime-shell itself.
- Runtime-shell and render bridge review should remain independent of whether a future surface uses manual DOM updates or data binding.
- If QmClient adopts data binding, review should verify:
  - model lifetime is tied to context/document lifetime correctly
  - bound callbacks do not bypass host fallback and input-layer rules
  - `data-` authoring conventions are documented for local surface authors
