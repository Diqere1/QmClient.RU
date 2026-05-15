---
doc_type: libdoc
status: current
created: 2026-05-07
last_reviewed: 2026-05-07
tags: [rmlui, render-helpers, api, reference]
related_feature: 2026-05-07-rmlui-runtime-shell
---

# RmlUiRenderHelpers Reference

Path: `src/game/client/RmlUi/RmlUiRenderHelpers.h/.cpp`

## Purpose

This module provides prototype helper functions for Monitoring HUD drawing and layout visualization.

## Public API

### `void DrawMonitoringGrid(IGraphics *pGraphics, const CUIRect &Rect, int Rows, bool DrawZeroAxis = false)`

Draws a monitoring grid in the given rectangle.

Use case:

- visual aid for prototype graphs
- layout debugging

### `void DrawMonitoringSeries(IGraphics *pGraphics, const CUIRect &Rect, const SQmMonitoringSeriesView &Series, ColorRGBA Color, float MinValue, float MaxValue)`

Draws a single monitoring series in a rectangle.

Use case:

- prototype chart rendering

### `void DrawMonitoringSeriesPair(IGraphics *pGraphics, const CUIRect &Upper, const SQmMonitoringSeriesView &A, ColorRGBA AColor, const CUIRect &Lower, const SQmMonitoringSeriesView &B, ColorRGBA BColor)`

Draws two monitoring series in stacked areas.

Use case:

- prototype comparison charts

## Boundary Notes

- These helpers are current prototype drawing helpers.
- They are not the render bridge.
- They should not absorb module registry, fallback, or resource policy.
