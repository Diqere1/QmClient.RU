#include "test.h"

#include <game/client/QmUi/UiTokens.h>

#include <gtest/gtest.h>

// Compile-time invariants. Catching token regressions at compile time keeps
// downstream feat-003 widgets stable. NOTE: color4_base uses anonymous unions
// (x/r/h share storage); constexpr evaluation can only read the active union
// member, which is x/y/z/a (the constructor initializes those). Hence we
// assert via .x/.y/.z here and use .r/.g/.b at runtime.
static_assert(ui_token::color::ACCENT_PRIMARY.x > 0.39f && ui_token::color::ACCENT_PRIMARY.x < 0.41f,
	"ACCENT_PRIMARY R channel must match Steam #66c0f4 (~0.4)");
static_assert(ui_token::color::SURFACE_GLASS.a == 0.70f,
	"SURFACE_GLASS alpha must remain at 0.70 to preserve QmClient glass appearance");
static_assert(ui_token::radius::CARD == 12.0f,
	"radius::CARD must match LgCornerRadius in menus_qmclient.cpp");

// Spacing scale must be strictly monotonic so downstream code can pick a
// "next size up" without ambiguity.
static_assert(ui_token::spacing::XS < ui_token::spacing::SM, "spacing scale must be monotonic");
static_assert(ui_token::spacing::SM < ui_token::spacing::MD, "spacing scale must be monotonic");
static_assert(ui_token::spacing::MD < ui_token::spacing::LG, "spacing scale must be monotonic");
static_assert(ui_token::spacing::LG < ui_token::spacing::XL, "spacing scale must be monotonic");

static_assert(ui_token::radius::NONE < ui_token::radius::TIGHT, "radius scale must be monotonic");
static_assert(ui_token::radius::TIGHT < ui_token::radius::BASE, "radius scale must be monotonic");
static_assert(ui_token::radius::BASE < ui_token::radius::CARD, "radius scale must be monotonic");

static_assert(ui_token::font::TIP < ui_token::font::CAPTION, "font scale must be monotonic");
static_assert(ui_token::font::CAPTION < ui_token::font::BODY, "font scale must be monotonic");
static_assert(ui_token::font::BODY < ui_token::font::HEADLINE, "font scale must be monotonic");
static_assert(ui_token::font::HEADLINE < ui_token::font::HEADLINE_LG, "font scale must be monotonic");

TEST(QmUiTokens, AccentPrimaryMatchesSteamBlue)
{
	EXPECT_NEAR(ui_token::color::ACCENT_PRIMARY.r, 0.4f, 0.01f);
	EXPECT_NEAR(ui_token::color::ACCENT_PRIMARY.g, 0.753f, 0.01f);
	EXPECT_NEAR(ui_token::color::ACCENT_PRIMARY.b, 0.957f, 0.01f);
	EXPECT_EQ(ui_token::color::ACCENT_PRIMARY.a, 1.0f);
}

TEST(QmUiTokens, SurfaceGlassPreservesAlpha)
{
	EXPECT_NEAR(ui_token::color::SURFACE_GLASS.r, 0.08f, 0.001f);
	EXPECT_NEAR(ui_token::color::SURFACE_GLASS.g, 0.09f, 0.001f);
	EXPECT_NEAR(ui_token::color::SURFACE_GLASS.b, 0.12f, 0.001f);
	EXPECT_EQ(ui_token::color::SURFACE_GLASS.a, 0.70f);
}

TEST(QmUiTokens, MotionRefsBindToAnimCurves)
{
	EXPECT_EQ(ui_token::motion::BTN_HOVER.m_Easing, EEasing::EASE_OUT_QUART);
	EXPECT_EQ(ui_token::motion::MODAL_IN.m_Easing, EEasing::CUBIC_BEZIER);
	EXPECT_NEAR(ui_token::motion::TOGGLE.m_Stiffness, 280.0f, 1e-6f);
}
