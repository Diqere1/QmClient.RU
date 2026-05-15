---
doc_type: reference
slug: rmlui-settings-host-contract
status: current
created: 2026-05-11
tags: [rmlui, settings, contract, navigation]
related_feature: 2026-05-10-rmlui-settings-reorg
related_roadmap: rmlui-full-replacement
---

# RmlUI Settings Host Contract

This file is the canonical home for long-lived settings host rules.

## 1. Official basis

- RmlUi contexts are independent document collections.
- Each context owns its own size, hover/focus/input state, and update/render lifecycle.
- If a surface needs independent input or lifecycle, give it a separate context.
- Official references:
  - [main loop](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/main_loop)
  - [contexts](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/contexts)

## 2. Host Rules

- `CMenus::RenderSettings(...)` is the host seam, not the active settings content owner.
- Active RmlUI settings path must not parallel render the legacy settings UI.
- Settings page uses a dedicated page context sized to the viewport.
- Settings modal uses a dedicated modal context and owns modal focus/input.
- Page and modal contexts do not share hover/focus/document state.
- If the active RmlUI path fails, the host may exit that path, but it must not mix legacy and RmlUI settings in the same frame.
- Legacy settings renderer stays as reference and fallback path, not as an active sibling renderer.

## 3. Canonical IA

- `Tee`
- `游戏界面（HUD）`
- `图像`
- `声音`
- `资源`
- `配置`
- `功能`
- `搜索`

Rules:

- `搜索` is reserved as a first-class slot even before search behavior lands.
- `功能` is the home for `TClient`, `DDNet`, and `栖梦` capability groups.
- `配置` is the home for controls, binds, and client configuration state.

## 4. What This Is Not

- Not the feature design.
- Not the roadmap.
- Not the implementation checklist.
- Not a place to restate every destination or state adapter detail.
