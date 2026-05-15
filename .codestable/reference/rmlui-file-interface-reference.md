---
doc_type: libdoc
status: current
created: 2026-05-07
last_reviewed: 2026-05-07
tags: [rmlui, file-interface, api, reference]
related_roadmap: rmlui-full-replacement
external_docs:
  - https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/interfaces/file
---

# RmlUI File Interface Reference

Source basis: official RmlUi file-interface documentation.

This reference covers the upstream `Rml::FileInterface` contract used for documents, RCSS, fonts, and other resources.

## Public API

### `Rml::FileHandle Open(const Rml::String& path)`

Opens a file for reading.

Returns:

- a valid file handle on success
- `0` on failure

### `void Close(Rml::FileHandle file)`

Closes a previously opened file.

### `size_t Read(void* buffer, size_t size, Rml::FileHandle file)`

Reads data from an opened file into `buffer`.

### `bool Seek(Rml::FileHandle file, long offset, int origin)`

Moves the file cursor.

### `size_t Tell(Rml::FileHandle file)`

Returns the current file offset.

## Integration Notes

- The file interface controls how RmlUi finds RML, RCSS, fonts, and asset data.
- QmClient path resolution may root relative lookups under `qmclient/rmlui/`.
- File failures should be surfaced in diagnostics rather than treated as silent fallback success.
