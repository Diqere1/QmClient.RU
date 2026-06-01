# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

QmClient (a.k.a. Q1menG Client) is a customised DDNet/Teeworlds client built on top of DDNet + TaterClient. It is primarily C++20 with a small Rust surface (utilities only, via `cxx.rs` bridge). Build is driven by CMake.

Authoritative agent guide for this repo is `AGENTS.md`. Read it for layer rules, naming, and constraints. The notes below are deltas/specifics that aren't already there.

## Build & Test

Windows is the primary dev platform. The wrapper `qmclient_scripts/cmake-windows.cmd` locates VS 2022 / MSVC via `vswhere`, prepends Ninja, runs `cmake` with the given args, then strips MSVC `Note: including file:` / `注意: 包含文件:` lines from output. Always invoke CMake through this wrapper on Windows — calling `cmake` directly from a plain shell will not have the MSVC dev environment loaded.

```bat
REM Configure (Release, Ninja, export compile_commands.json for clangd)
qmclient_scripts\cmake-windows.cmd -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

REM Build the standard client
qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target game-client -j 10

REM Tests
qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target run_tests        REM C++ + Rust
qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target run_cxx_tests
qmclient_scripts\cmake-windows.cmd --build cmake-build-release --target run_rust_tests
```

Linux/macOS use plain `cmake` (the wrapper is Windows-only):

```sh
cmake -S . -B cmake-build-release
cmake --build cmake-build-release --target game-client -j 10
```

Common targets: `game-client`, `game-server`, `game-live-client`, `game-live-server`, `run_tests`, `run_cxx_tests`, `run_rust_tests`. The "live" variants are built when `CONF_QM_LIVE_CLIENT` is defined and rename the binary to `QmLiveClient` (see `src/game/version.h`). Live-client–only sources live under `src/game/client/live/`.

Running a single C++ test: build `run_cxx_tests` then run the resulting test binary directly (e.g. `cmake-build-release/<test-binary> --gtest_filter=Suite.Case`). Tests are GoogleTest-based; sources are in `src/test/*_test.cpp`.

Auto-fix style before committing:

```sh
python3 ./scripts/fix_style.py            # upstream DDNet rules
python3 ./qmclient_scripts/fix_style.py   # QmClient-specific overlay (if applicable)
```

clang-format 20 is required. The format config (`.clang-format`) uses tabs for indentation, `ColumnLimit: 0` (no hard wrap), `SpaceBeforeParens: Never`, and grouped includes — DDNet's `<base/...> <engine/...> <generated/...> <game/...>` ordering is enforced.

## Architecture

Three-layer design (see `AGENTS.md` for the table). Key additions specific to this fork:

- **QmClient overlay** lives in two places that future agents will trip on:
  - `src/game/client/components/qmclient/` — feature components (voice chat, scripting, lyrics, jelly-tee renderer, hitbox, weapon-trajectory, input overlay, modes, monitoring, translate). These follow the standard component lifecycle (`OnInit`, `OnConsoleInit`, `OnRender`, etc.) and are wired up in `gameclient.cpp` alongside upstream + tclient components.
  - `src/game/client/QmUi/` — a custom UI framework that sits beside the upstream `ui.*` / `ui_*.cpp` system. It provides its own animation engine (`QmAnim*`, `QmAnimCurves`), layout (`QmLayout`), runtime (`QmRt`), tree/render passes (`QmTree`, `QmRender`), navigation, overlays, dogfood, forms, and design tokens (`UiTokens.h`). When adding new UI, check whether QmUi already provides the primitive before reaching for the legacy `CUi`.
- **TClient overlay**: `src/game/client/components/tclient/` is the TaterClient feature set carried forward (background particles, bindchat, bindwheel, bg_draw, etc.). Treat it as upstream-ish — match its existing patterns when extending.
- **Live client**: a separate binary built when `CONF_QM_LIVE_CLIENT` is set. Sources are gated under `src/game/client/live/` and the `game-live-client` / `game-live-server` CMake targets. `CLIENT_NAME` switches to `"QmLiveClient"` automatically (see `src/game/version.h`).
- **Config variables** are split by origin to keep merges with upstream cleaner:
  - `src/engine/shared/config_variables.h` — upstream DDNet vars
  - `src/engine/shared/config_variables_tclient.h` — TClient vars
  - `src/engine/shared/config_variables_qmclient.h` — QmClient vars
  - `src/engine/shared/config_variables_qmclient_extra.h` — additional QmClient vars (use this for new QmClient-specific vars unless there's a strong reason to land them elsewhere)

  All four headers are included from the same config registration sites; pick the right one for the origin of the feature. `config_tags.cpp/.h` handle tag metadata.
- **Versioning**: two independent version strings live in `src/game/version.h`:
  - `GAME_RELEASE_VERSION_INTERNAL` — DDNet base (parsed by `CMakeLists.txt` for `project(...)` version of the DDNet portion).
  - `QMCLIENT_VERSION` — QmClient overlay (parsed separately into `project(QmClient ...)` version). Bump this for QmClient-only releases. Both end up in `data_version.txt` in the build dir.
- **Scripts dirs**: `scripts/` is the upstream DDNet toolset. `qmclient_scripts/` is the QmClient overlay (note `cmake-windows.cmd`, `fix_style.py`, `check_*` variants, `languages_qmclient/`). When two scripts have the same name, prefer the one matching the origin of the code you're touching, and run both `check_*` variants in CI-like checks.

## Conventions worth repeating

These bite people coming from other DDNet forks:

- Naming follows DDNet's quirky hybrid: `UpperCamelCase` everywhere except `src/base/` (snake_case). Hungarian-ish prefixes are mandatory: `m_` member, `g_` global, `s_` static, `p` pointer, `a` fixed-size array, `v` `std::vector`, `C`/`I`/`S` for class/interface/(legacy) struct. Don't use `c`/`b`/`i` for type. Filenames are `lower_snake_case.cpp`.
- No exceptions in C++ — use `dbg_assert()` and bool/error-code returns. `bool` returning `true` means success.
- Rust is for `src/base/` and `src/engine/` utilities only. Do not add Rust under `src/game/`. The FFI lives in `src/rust-bridge/` (cxx.rs) — extend the existing bridges instead of inventing a new mechanism.
- Includes use angle-bracket project paths: `<base/...>`, `<engine/...>`, `<generated/...>`, `<game/...>`. Quote-form is for sibling files only.
- Console commands register in each component's `OnConsoleInit()`. Config vars are declared via `MACRO_CONFIG_*` in the appropriate `config_variables*.h`.

## Constraints (hard rules)

From `CONTRIBUTING.md` and `AGENTS.md`, do not break:

- Network protocol or file formats (skins, demos, maps).
- Existing ranks (don't make completed runs impossible).
- Dummy with new gameplay-affecting features.

New features generally need maintainer discussion first.

## File location quick reference

| You want to… | Path |
|---|---|
| Add a QmClient client feature | `src/game/client/components/qmclient/` |
| Add a TClient-style feature | `src/game/client/components/tclient/` |
| Add a stock DDNet client component | `src/game/client/components/` |
| Add a QmUi widget/primitive | `src/game/client/QmUi/` |
| Add a server entity | `src/game/server/entities/` |
| Add a config variable (QmClient-origin) | `src/engine/shared/config_variables_qmclient_extra.h` |
| Add a config variable (TClient-origin) | `src/engine/shared/config_variables_tclient.h` |
| Add a console command | Component `OnConsoleInit()` |
| Add a cross-platform utility | `src/base/` (snake_case) |
| Add a C++ unit test | `src/test/<thing>_test.cpp` |
| Add a standalone tool | `src/tools/` |
| Add live-client–only code | `src/game/client/live/` (gate with `CONF_QM_LIVE_CLIENT`) |
