#include <game/client/components/qmclient/hud_notifications.h>

#include <gtest/gtest.h>

TEST(QmHudNotifications, MatchesKnownSoloPrompts)
{
	EXPECT_EQ(QmHudNotifications::MatchKnownSoloPrompt("You are now in a solo part"), QmHudNotifications::ESoloPrompt::Enter);
	EXPECT_EQ(QmHudNotifications::MatchKnownSoloPrompt("You are now out of the solo part"), QmHudNotifications::ESoloPrompt::Leave);
	EXPECT_EQ(QmHudNotifications::MatchKnownSoloPrompt("你现在处于单人区域"), QmHudNotifications::ESoloPrompt::Enter);
	EXPECT_EQ(QmHudNotifications::MatchKnownSoloPrompt("你现在已离开单人区域"), QmHudNotifications::ESoloPrompt::Leave);
	EXPECT_EQ(QmHudNotifications::MatchKnownSoloPrompt("regular server message"), QmHudNotifications::ESoloPrompt::None);
}

TEST(QmHudNotifications, SuppressesOnlyMatchedSoloChatMessages)
{
	EXPECT_TRUE(QmHudNotifications::ShouldSuppressSoloChatMessage("You are now in a solo part", QmHudNotifications::ESoloPrompt::None));
	EXPECT_TRUE(QmHudNotifications::ShouldSuppressSoloChatMessage("You are now in a solo part", QmHudNotifications::ESoloPrompt::Enter));
	EXPECT_FALSE(QmHudNotifications::ShouldSuppressSoloChatMessage("regular server message", QmHudNotifications::ESoloPrompt::Enter));
	EXPECT_FALSE(QmHudNotifications::ShouldSuppressSoloChatMessage("You are now out of the solo part", QmHudNotifications::ESoloPrompt::Enter));
}

TEST(QmHudNotifications, RoutesServerSystemMessagesWhenEnabled)
{
	EXPECT_EQ(QmHudNotifications::ServerMessageRoute("regular server message", QmHudNotifications::ESoloPrompt::None, false), QmHudNotifications::EServerMessageRoute::None);
	EXPECT_EQ(QmHudNotifications::ServerMessageRoute("regular server message", QmHudNotifications::ESoloPrompt::None, true), QmHudNotifications::EServerMessageRoute::System);
	EXPECT_EQ(QmHudNotifications::ServerMessageRoute("Team save already in progress", QmHudNotifications::ESoloPrompt::None, true), QmHudNotifications::EServerMessageRoute::System);
	EXPECT_EQ(QmHudNotifications::ServerMessageRoute("You are now in a solo part", QmHudNotifications::ESoloPrompt::Enter, true), QmHudNotifications::EServerMessageRoute::Solo);
	EXPECT_EQ(QmHudNotifications::ServerMessageRoute("You are now in a solo part", QmHudNotifications::ESoloPrompt::Enter, false), QmHudNotifications::EServerMessageRoute::None);
	EXPECT_EQ(QmHudNotifications::ServerMessageRoute("You are now in a solo part", QmHudNotifications::ESoloPrompt::None, true), QmHudNotifications::EServerMessageRoute::Solo);
	EXPECT_EQ(QmHudNotifications::ServerMessageRoute("DDraceNetwork 版本: 18.9", QmHudNotifications::ESoloPrompt::None, true), QmHudNotifications::EServerMessageRoute::None);
	EXPECT_EQ(QmHudNotifications::ServerMessageRoute("请访问 DDNet.org，或输入 /info，并确保阅读 /rules", QmHudNotifications::ESoloPrompt::None, true), QmHudNotifications::EServerMessageRoute::None);
	EXPECT_EQ(QmHudNotifications::ServerMessageRoute("'nameless tee' entered and joined the game", QmHudNotifications::ESoloPrompt::None, true), QmHudNotifications::EServerMessageRoute::None);
	EXPECT_EQ(QmHudNotifications::ServerMessageRoute("'nameless tee' joined the game", QmHudNotifications::ESoloPrompt::None, true), QmHudNotifications::EServerMessageRoute::None);
	EXPECT_EQ(QmHudNotifications::ServerMessageRoute("", QmHudNotifications::ESoloPrompt::None, true), QmHudNotifications::EServerMessageRoute::None);
	EXPECT_EQ(QmHudNotifications::ServerMessageRoute(nullptr, QmHudNotifications::ESoloPrompt::None, true), QmHudNotifications::EServerMessageRoute::None);
}

TEST(QmHudNotifications, ClassifiesServerSystemMessagesForFocusMode)
{
	EXPECT_EQ(QmHudNotifications::ServerMessageClass("DDraceNetwork 版本: 18.9", QmHudNotifications::ESoloPrompt::None), QmHudNotifications::EServerMessageClass::BasicInfo);
	EXPECT_EQ(QmHudNotifications::ServerMessageClass("请访问 DDNet.org，或输入 /info，并确保阅读 /rules", QmHudNotifications::ESoloPrompt::None), QmHudNotifications::EServerMessageClass::BasicInfo);
	EXPECT_EQ(QmHudNotifications::ServerMessageClass("'nameless tee' joined the game", QmHudNotifications::ESoloPrompt::None), QmHudNotifications::EServerMessageClass::BasicInfo);
	EXPECT_EQ(QmHudNotifications::ServerMessageClass("Players are not allowed to chat from VPNs at this time", QmHudNotifications::ESoloPrompt::None), QmHudNotifications::EServerMessageClass::BasicInfo);
	EXPECT_EQ(QmHudNotifications::ServerMessageClass("请友善交流。", QmHudNotifications::ESoloPrompt::None), QmHudNotifications::EServerMessageClass::BasicInfo);
	EXPECT_EQ(QmHudNotifications::ServerMessageClass("未设置服务器规则，请联系管理员。", QmHudNotifications::ESoloPrompt::None), QmHudNotifications::EServerMessageClass::BasicInfo);
	EXPECT_EQ(QmHudNotifications::ServerMessageClass("Team save already in progress", QmHudNotifications::ESoloPrompt::None), QmHudNotifications::EServerMessageClass::Prompt);
	EXPECT_EQ(QmHudNotifications::ServerMessageClass("You are now in a solo part", QmHudNotifications::ESoloPrompt::Enter), QmHudNotifications::EServerMessageClass::Prompt);
	EXPECT_EQ(QmHudNotifications::ServerMessageClass("", QmHudNotifications::ESoloPrompt::None), QmHudNotifications::EServerMessageClass::None);
	EXPECT_EQ(QmHudNotifications::ServerMessageClass(nullptr, QmHudNotifications::ESoloPrompt::None), QmHudNotifications::EServerMessageClass::None);
}

TEST(QmHudNotifications, ClampsVisibleCount)
{
	EXPECT_EQ(QmHudNotifications::ClampVisibleCount(-1), 1);
	EXPECT_EQ(QmHudNotifications::ClampVisibleCount(0), 1);
	EXPECT_EQ(QmHudNotifications::ClampVisibleCount(3), 3);
	EXPECT_EQ(QmHudNotifications::ClampVisibleCount(20), 8);
}

TEST(QmHudNotifications, ClampsTiming)
{
	EXPECT_EQ(QmHudNotifications::ClampHoldMs(200), 500);
	EXPECT_EQ(QmHudNotifications::ClampHoldMs(2500), 2500);
	EXPECT_EQ(QmHudNotifications::ClampHoldMs(30000), 10000);
	EXPECT_EQ(QmHudNotifications::ClampAnimationMs(0), 0);
	EXPECT_EQ(QmHudNotifications::ClampAnimationMs(9000), 2000);
}

TEST(QmHudNotifications, ClampsTextSize)
{
	EXPECT_EQ(QmHudNotifications::ClampTextSize(0), 1);
	EXPECT_EQ(QmHudNotifications::ClampTextSize(8), 8);
	EXPECT_EQ(QmHudNotifications::ClampTextSize(40), 24);
}

TEST(QmHudNotifications, ScalesSmallTextChrome)
{
	EXPECT_FLOAT_EQ(QmHudNotifications::SmallTextScale(1.0f), 0.33f);
	EXPECT_FLOAT_EQ(QmHudNotifications::PaddingX(1.0f), 1.98f);
	EXPECT_FLOAT_EQ(QmHudNotifications::PaddingY(1.0f), 1.32f);
	EXPECT_FLOAT_EQ(QmHudNotifications::MinBoxWidth(1.0f), 27.06f);
	EXPECT_FLOAT_EQ(QmHudNotifications::PaddingX(8.0f), 6.0f);
	EXPECT_FLOAT_EQ(QmHudNotifications::PaddingY(8.0f), 4.0f);
	EXPECT_FLOAT_EQ(QmHudNotifications::MinBoxWidth(8.0f), 82.0f);
}

TEST(QmHudNotifications, SelectsTextColorByNotificationKind)
{
	constexpr unsigned SystemColor = 0x111111;
	constexpr unsigned EchoOverrideColor = 0xFF222222;
	constexpr unsigned ChatEchoColor = 0x333333;

	const QmHudNotifications::STextColorConfig System = QmHudNotifications::TextColorConfig(QmHudNotifications::ETextSource::System, 1, SystemColor, EchoOverrideColor, ChatEchoColor);
	EXPECT_EQ(System.m_Color, SystemColor);
	EXPECT_TRUE(System.m_HasAlpha);

	const QmHudNotifications::STextColorConfig EchoInherited = QmHudNotifications::TextColorConfig(QmHudNotifications::ETextSource::Echo, 1, SystemColor, EchoOverrideColor, ChatEchoColor);
	EXPECT_EQ(EchoInherited.m_Color, ChatEchoColor);
	EXPECT_FALSE(EchoInherited.m_HasAlpha);

	const QmHudNotifications::STextColorConfig EchoOverride = QmHudNotifications::TextColorConfig(QmHudNotifications::ETextSource::Echo, 0, SystemColor, EchoOverrideColor, ChatEchoColor);
	EXPECT_EQ(EchoOverride.m_Color, EchoOverrideColor);
	EXPECT_TRUE(EchoOverride.m_HasAlpha);
}
