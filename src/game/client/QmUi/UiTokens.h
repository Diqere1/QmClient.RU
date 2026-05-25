/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_QMUI_UITOKENS_H
#define GAME_CLIENT_QMUI_UITOKENS_H

#include <base/color.h>

#include "QmAnimCurves.h"

// Centralized design tokens for feat-003. Inspired by Steam Library (deep blue
// glass surface with #66c0f4 accent). Values are unscaled — callers apply
// responsive UiScale on top, matching existing LG_* convention in
// menus_qmclient.cpp.

namespace ui_token::color
{
// surface
inline constexpr ColorRGBA SURFACE_GLASS{0.08f, 0.09f, 0.12f, 0.70f};
inline constexpr ColorRGBA SURFACE_ELEVATED{0.10f, 0.11f, 0.14f, 0.82f};
inline constexpr ColorRGBA SURFACE_OVERLAY{0.0f, 0.0f, 0.0f, 0.40f};
inline constexpr ColorRGBA SURFACE_HIGHLIGHT{1.0f, 1.0f, 1.0f, 0.05f};
inline constexpr ColorRGBA SURFACE_SHADOW{0.0f, 0.0f, 0.0f, 0.12f};
inline constexpr ColorRGBA BORDER_SUBTLE{1.0f, 1.0f, 1.0f, 0.08f};
inline constexpr ColorRGBA BORDER_FOCUS{0.4f, 0.753f, 0.957f, 0.85f};

// accent (Steam #66c0f4)
inline constexpr ColorRGBA ACCENT_PRIMARY{0.4f, 0.753f, 0.957f, 1.0f};
inline constexpr ColorRGBA ACCENT_PRIMARY_HOVER{0.55f, 0.82f, 0.98f, 1.0f};
inline constexpr ColorRGBA ACCENT_PRIMARY_PRESS{0.30f, 0.65f, 0.88f, 1.0f};
inline constexpr ColorRGBA ACCENT_PRIMARY_DIM{0.4f, 0.753f, 0.957f, 0.18f};

// text
inline constexpr ColorRGBA TEXT_PRIMARY{1.0f, 1.0f, 1.0f, 1.0f};
inline constexpr ColorRGBA TEXT_SECONDARY{1.0f, 1.0f, 1.0f, 0.72f};
inline constexpr ColorRGBA TEXT_TIP{1.0f, 1.0f, 1.0f, 0.65f};
inline constexpr ColorRGBA TEXT_DISABLED{1.0f, 1.0f, 1.0f, 0.38f};
inline constexpr ColorRGBA TEXT_ON_ACCENT{0.06f, 0.08f, 0.11f, 1.0f};

// semantic
inline constexpr ColorRGBA SUCCESS{0.42f, 0.85f, 0.52f, 1.0f};
inline constexpr ColorRGBA WARNING{0.98f, 0.78f, 0.30f, 1.0f};
inline constexpr ColorRGBA DANGER{0.95f, 0.41f, 0.38f, 1.0f};
} // namespace ui_token::color

namespace ui_token::spacing
{
inline constexpr float XS = 4.0f;
inline constexpr float SM = 8.0f;
inline constexpr float MD = 12.0f;
inline constexpr float LG = 16.0f;
inline constexpr float XL = 24.0f;
} // namespace ui_token::spacing

namespace ui_token::radius
{
inline constexpr float NONE = 0.0f;
inline constexpr float TIGHT = 3.0f;
inline constexpr float BASE = 5.0f;
inline constexpr float CARD = 12.0f;
inline constexpr float PILL = 999.0f;
} // namespace ui_token::radius

namespace ui_token::elevation
{
inline constexpr float SHADOW_X_LOW = 1.0f;
inline constexpr float SHADOW_Y_LOW = 1.0f;
inline constexpr float SHADOW_X_MED = 1.5f;
inline constexpr float SHADOW_Y_MED = 2.0f;
inline constexpr float SHADOW_X_HIGH = 3.0f;
inline constexpr float SHADOW_Y_HIGH = 6.0f;
} // namespace ui_token::elevation

namespace ui_token::font
{
inline constexpr float HEADLINE_LG = 16.0f;
inline constexpr float HEADLINE = 14.0f;
inline constexpr float BODY = 12.0f;
inline constexpr float CAPTION = 10.0f;
inline constexpr float TIP = 9.0f;
} // namespace ui_token::font

namespace ui_token::motion
{
inline constexpr const SUiAnimTransition &BTN_HOVER = ui_curve::DECELERATE;
inline constexpr const SUiAnimTransition &BTN_PRESS = ui_curve::ACCELERATE;
inline constexpr const SUiAnimTransition &MODAL_IN = ui_curve::EMPHASIZED;
inline constexpr const SUiSpringConfig &TOGGLE = ui_spring::SNAPPY;
} // namespace ui_token::motion

#endif
