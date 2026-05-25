# DDNet / QmClient Development Rules

Use this document for C++ implementation, refactoring, or debugging in QmClient.

## Compatibility first

DDNet compatibility beats generic modern C++ preferences.

Do not change these without explicit approval:

- Network protocol fields or layout.
- Demo, skin, map, config, or save file formats.
- Physics, collision, prediction, snapshots, inputs, timing, replay, or map behavior.
- Anything that makes existing ranks unreachable or existing maps easier.

If a task touches any of these areas, call out the risk before implementation and keep the patch minimal.

## Scope boundary

Normal QmClient scope:

- `src/game/client/components/qmclient/`
- `src/game/client/QmUi/`
- `src/engine/shared/config_variables_qmclient*.h`
- `src/game/version.h`
- `data/languages/simplified_chinese.txt`
- `docs/info.json`
- `qmclient_scripts/`
- `.ai/`, `AGENTS.md`, `CLAUDE.md`, and other harness files

Out of scope without explicit request:

- Upstream engine core outside QmClient config.
- Server gameplay, editor internals, protocol, physics, collision, prediction, snapshots, replay behavior.
- Third-party libraries in `ddnet-libs/` or `src/engine/external/`.
- Release CI workflow behavior.

## Style

Follow existing DDNet style over generic C++ style:

- Prefer UpperCamelCase for local variables, methods, and classes outside special areas like `src/base`.
- Use existing prefixes: `m_` members, `g_` globals, `s_` statics, `p` pointers, `a` fixed arrays, `v` vectors, `C` classes, `I` interfaces.
- Prefer semantic names. Short loop variables are acceptable only when the scope is tiny and obvious.
- Prefer early returns and small focused functions, but do not split code so much that DDNet-style readability suffers.

## Modern C++

Allowed when it fits the module:

- `constexpr`
- `enum class` with `E...` names and uppercase values
- `std::optional`
- `std::variant`
- move semantics
- `std::array`
- carefully scoped `std::string_view`

Avoid:

- raw `new` / `delete` unless the surrounding code owns objects that way
- unnecessary macros
- `goto`
- assignment inside `if` conditions
- treating integers as booleans
- hidden ownership transfer
- unnecessary heap allocation
- broad template or RAII rewrites that do not match the module

## Runtime and hot paths

DDNet is a real-time networked game. Check whether code runs per frame, tick, player, entity, snapshot, render item, or text layout.

Be suspicious of:

- heap allocations in render/tick paths
- repeated string construction
- repeated sorting or scanning
- repeated `TextWidth` or layout computation
- config writes on unchanged state
- serialization/deserialization inside frequent loops
- extra network bandwidth or protocol growth

Avoid premature optimization, but do not introduce obvious hot-path waste.

## Errors and data boundaries

- Do not silently ignore file, network, parse, config, console, resource, or external-data failures.
- Validate indices, sizes, pointers, and external input.
- Use debug assertions for developer invariants and runtime handling for user/external failures.
- Follow the module's existing error propagation style; do not convert large areas to exception-driven flow.

## Memory and lifetime

Check for:

- dangling pointers or references
- returning references to local data
- invalidated iterators
- out-of-bounds access
- use-after-free or double free
- uninitialized reads
- `string_view` or pointer lifetime mismatches

When using cached, static, or global state, consider initialization order and thread safety.

## Threads

Do not introduce threads, locks, or atomics speculatively. If code touches audio, graphics, HTTP, storage, database, logs, platform code, or background jobs, identify the thread boundary and shared mutable state.
