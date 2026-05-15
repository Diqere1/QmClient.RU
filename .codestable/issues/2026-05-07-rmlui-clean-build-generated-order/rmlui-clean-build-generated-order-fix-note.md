---
doc_type: issue-fix
issue: 2026-05-07-rmlui-clean-build-generated-order
path: fast-track
fix_date: 2026-05-07
tags: [build, windows, rmlui, cmake]
---

# RmlUi clean build generated header order fix note

## 1. Problem

After cleaning `build-ninja`, rebuilding `game-client` failed before the client executable could be restored:

```text
src/game/client/components/menus.h(19): fatal error C1083: cannot open include file: "generated/client_data.h"
```

The runtime symptom before the clean rebuild was `DDNet.exe` printing only early storage initialization logs and then not opening a window reliably.

## 2. Root Cause

`game-client-no-ipo` was introduced as an `OBJECT` library for two ChaiScript-heavy source files. Those sources include client headers that depend on generated files such as `generated/client_data.h`.

Because `game-client-no-ipo` was also appended to `TARGETS_OWN`, CMake added it as an order-only dependency of generated output rules. That made the build graph require `game-client-no-ipo` before generating `client_data.h`, while `game-client-no-ipo` itself needed `client_data.h` to compile.

## 3. Fix

Removed the separate `game-client-no-ipo` object library and put the two source files back into the main `game-client` target. On MSVC, `game-client` is now added to `TARGETS_NO_IPO` so the client target stays out of IPO/LTCG without creating a generated-file dependency cycle or per-source `/GL-` override warnings.

## 4. Changed Files

- `CMakeLists.txt`

## 5. Verification

- `qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 80` passed.
- Recompile check passed without MSVC `D9025` `/GL` override warnings.
- Launch smoke test from `build-ninja` kept `DDNet.exe` alive after 10 seconds:

```text
RUNTIME_ALIVE pid=59288
```

## 6. Remaining Notes

The build still emits the pre-existing developer warning that `GAME_CLIENT` does not list `RmlUiConfigCompat.h`. That warning is unrelated to this startup/build-order fix.
