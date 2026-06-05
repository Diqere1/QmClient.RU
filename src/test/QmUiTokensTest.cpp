#include "test.h"

#include <game/client/QmUi/QmMotion.h>
#include <game/client/QmUi/QmTheme.h>
#include <game/client/QmUi/UiTokens.h>
#include <game/client/lineinput.h>

#include <gtest/gtest.h>

// Compile-time invariants. Catching token regressions at compile time keeps
// downstream QmUi widgets stable. NOTE: color4_base uses anonymous unions
// (x/r/h share storage); constexpr evaluation can only read the active union
// member, which is x/y/z/a (the constructor initializes those). Hence we
// assert via .x/.y/.z here and use .r/.g/.b at runtime.
static_assert(ui_token::color::ACCENT_PRIMARY.x > 0.39f && ui_token::color::ACCENT_PRIMARY.x < 0.41f,
	"ACCENT_PRIMARY R channel must stay near the Qm accent blue (~0.4)");
static_assert(ui_token::color::SURFACE_GLASS.a == 0.70f,
	"SURFACE_GLASS alpha must remain at 0.70 to preserve QmClient glass appearance");
static_assert(ui_token::radius::CARD == 12.0f,
	"radius::CARD must match LG_CornerRadius in menus_qmclient.cpp");

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

TEST(QmUiTokens, AccentPrimaryMatchesQmBlue)
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
	EXPECT_FLOAT_EQ(ui_token::ime::SCALE, 0.75f);
	EXPECT_FLOAT_EQ(ui_token::ime::PANEL_BG_LIGHT.a, 0.45f);
	EXPECT_FLOAT_EQ(ui_token::ime::PANEL_BG_DARK.a, 0.45f);
	EXPECT_FLOAT_EQ(ui_token::ime::COMPOSITION_SELECTION.a, 0.18f);
	EXPECT_GT(ui_token::ime::TEXT_SAFE_PADDING_X, 0.0f);
	EXPECT_GT(ui_token::ime::TEXT_SAFE_PADDING_Y, 0.0f);
}

TEST(QmUiTokens, QmThemeMirrorsSharedTokens)
{
	EXPECT_EQ(qm_theme::ICON.m_Active.r, ui_token::color::ACCENT_PRIMARY.r);
	EXPECT_EQ(qm_theme::ICON.m_Active.g, ui_token::color::ACCENT_PRIMARY.g);
	EXPECT_EQ(qm_theme::ICON.m_Active.b, ui_token::color::ACCENT_PRIMARY.b);
	EXPECT_EQ(qm_theme::ICON.m_Disabled.a, 0.36f);

	EXPECT_EQ(qm_theme::IME.m_PanelBg.a, ui_token::ime::PANEL_BG.a);
	EXPECT_EQ(qm_theme::IME.m_CompositionSelection.a, ui_token::ime::COMPOSITION_SELECTION.a);
	EXPECT_EQ(qm_theme::IME.m_SelectedBg.b, ui_token::ime::SELECTED_BG.b);
	EXPECT_EQ(qm_theme::IME.m_Radius, ui_token::ime::RADIUS);
	EXPECT_EQ(qm_theme::IME.m_RowHeight, ui_token::ime::ROW_HEIGHT);
	EXPECT_EQ(qm_theme::IME.m_TextSafePaddingX, ui_token::ime::TEXT_SAFE_PADDING_X);
	EXPECT_EQ(qm_theme::IME.m_TextSafePaddingY, ui_token::ime::TEXT_SAFE_PADDING_Y);
}

TEST(QmImeOverlay, InvalidSelectionHighlightsFirstCandidate)
{
	EXPECT_EQ(qm_ime_overlay::NormalizeSelectedCandidateIndex(-1, 0), -1);
	EXPECT_EQ(qm_ime_overlay::NormalizeSelectedCandidateIndex(-1, 8), 0);
	EXPECT_EQ(qm_ime_overlay::NormalizeSelectedCandidateIndex(8, 8), 0);
	EXPECT_EQ(qm_ime_overlay::NormalizeSelectedCandidateIndex(3, 8), 3);
}

TEST(QmUiTokens, MotionRefsBindToAnimCurves)
{
	EXPECT_EQ(ui_token::motion::HOVER_FADE.m_Easing, EEasing::EASE_OUT_QUART);
	EXPECT_EQ(ui_token::motion::PRESS_SCALE.m_Easing, EEasing::EASE_IN);
	EXPECT_EQ(ui_token::motion::PAGE_SLIDE.m_Easing, EEasing::EASE_IN_OUT_CUBIC);
	EXPECT_EQ(ui_token::motion::TAB_SWITCH.m_Easing, EEasing::EASE_OUT_QUART);
	EXPECT_EQ(ui_token::motion::INPUT_FOCUS_RING.m_Easing, EEasing::EASE_OUT_QUART);
	EXPECT_EQ(ui_token::motion::TOAST_SLIDE.m_Easing, EEasing::CUBIC_BEZIER);
	EXPECT_EQ(ui_token::motion::TOOLTIP_FADE.m_Easing, EEasing::EASE_OUT_QUART);
	EXPECT_NEAR(ui_token::motion::TOGGLE_SPRING.m_Stiffness, 280.0f, 1e-6f);

	EXPECT_EQ(ui_token::motion::BTN_HOVER.m_Easing, EEasing::EASE_OUT_QUART);
	EXPECT_EQ(ui_token::motion::MODAL_IN.m_Easing, EEasing::CUBIC_BEZIER);
	EXPECT_NEAR(ui_token::motion::TOGGLE.m_Stiffness, 280.0f, 1e-6f);
}

TEST(QmUiTokens, QmMotionAppliesUserMotionLevel)
{
	SUiAnimTransition Transition = ui_token::motion::MODAL_FADE_SCALE;
	Transition.m_DelaySec = 0.20f;
	Transition.m_Driver = EUiAnimDriver::SPRING;
	Transition.m_Spring.m_Damping = 20.0f;
	Transition.m_Spring.m_RestEpsilon = 0.01f;
	Transition.m_Spring.m_RestVelocity = 0.05f;

	const SUiAnimTransition Disabled = qm_motion::ApplyMotionLevel(Transition, 0);
	EXPECT_EQ(Disabled.m_DurationSec, 0.0f);
	EXPECT_EQ(Disabled.m_DelaySec, 0.0f);
	EXPECT_EQ(Disabled.m_Driver, EUiAnimDriver::TWEEN);
	EXPECT_EQ(Disabled.m_Easing, EEasing::LINEAR);

	const SUiAnimTransition Reduced = qm_motion::ApplyMotionLevel(Transition, 1);
	EXPECT_NEAR(Reduced.m_DurationSec, Transition.m_DurationSec * 0.45f, 1e-6f);
	EXPECT_NEAR(Reduced.m_DelaySec, Transition.m_DelaySec * 0.25f, 1e-6f);
	EXPECT_NEAR(Reduced.m_Spring.m_Damping, Transition.m_Spring.m_Damping * 1.35f, 1e-6f);

	const SUiAnimTransition Full = qm_motion::ApplyMotionLevel(Transition, 2);
	EXPECT_EQ(Full.m_DurationSec, Transition.m_DurationSec);
	EXPECT_EQ(Full.m_DelaySec, Transition.m_DelaySec);
}
