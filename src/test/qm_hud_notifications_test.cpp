#include <game/client/components/qmclient/hud_notifications.h>
#include <game/client/components/qmclient/hud_notification_rules.h>

#include <base/color.h>

#include <gtest/gtest.h>

void CComponentInterfaces::OnInterfacesInit(CGameClient *pClient)
{
}

void CQmHudNotifications::OnReset()
{
	m_vNotifications.clear();
	m_HasLastSolo = false;
	m_LastSolo = false;
	m_PendingCompatPrompt = QmHudNotifications::ESoloPrompt::None;
	m_PendingCompatUntil = 0;
}

void CQmHudNotifications::OnRelease()
{
	OnReset();
}

void CQmHudNotifications::OnNewSnapshot()
{
}

void CQmHudNotifications::OnRender()
{
}

namespace
{
	class CTestHudNotifications final : public CQmHudNotifications
	{
	public:
	};
} // namespace

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
	EXPECT_EQ(QmHudNotifications::ServerMessageClass("请友善交流。", QmHudNotifications::ESoloPrompt::None), QmHudNotifications::EServerMessageClass::BasicInfo);
	EXPECT_EQ(QmHudNotifications::ServerMessageClass("未设置服务器规则，请联系管理员。", QmHudNotifications::ESoloPrompt::None), QmHudNotifications::EServerMessageClass::BasicInfo);
	EXPECT_EQ(QmHudNotifications::ServerMessageClass("Team save already in progress", QmHudNotifications::ESoloPrompt::None), QmHudNotifications::EServerMessageClass::Prompt);
	EXPECT_EQ(QmHudNotifications::ServerMessageClass("You are now in a solo part", QmHudNotifications::ESoloPrompt::Enter), QmHudNotifications::EServerMessageClass::Prompt);
	EXPECT_EQ(QmHudNotifications::ServerMessageClass("", QmHudNotifications::ESoloPrompt::None), QmHudNotifications::EServerMessageClass::None);
	EXPECT_EQ(QmHudNotifications::ServerMessageClass(nullptr, QmHudNotifications::ESoloPrompt::None), QmHudNotifications::EServerMessageClass::None);
}

TEST(QmHudNotifications, KeepsShortSystemFeedbackOutOfBlacklist)
{
	EXPECT_FALSE(QmHudNotifications::ShouldExcludeSystemNotification("Players are not allowed to chat from VPNs at this time"));
	EXPECT_FALSE(QmHudNotifications::ShouldExcludeSystemNotification("You can see other players. To disable this use DDNet client and type /showothers"));
	EXPECT_FALSE(QmHudNotifications::ShouldExcludeSystemNotification("Unknown emote... Say /emote"));
	EXPECT_FALSE(QmHudNotifications::ShouldExcludeSystemNotification("Your timeout code has been set. 0.7 clients can not reclaim their tees on timeout; however, a 0.6 client can claim your tee "));

	EXPECT_EQ(QmHudNotifications::ServerMessageRoute("Players are not allowed to chat from VPNs at this time", QmHudNotifications::ESoloPrompt::None, true), QmHudNotifications::EServerMessageRoute::System);
	EXPECT_EQ(QmHudNotifications::ServerMessageClass("Players are not allowed to chat from VPNs at this time", QmHudNotifications::ESoloPrompt::None), QmHudNotifications::EServerMessageClass::Prompt);
}

TEST(QmHudNotifications, ExcludesHelpAndExampleMessagesFromNotifications)
{
	EXPECT_TRUE(QmHudNotifications::ShouldExcludeSystemNotification("Available practice commands: /rescue /lasttp /telecursor"));
	EXPECT_TRUE(QmHudNotifications::ShouldExcludeSystemNotification("Available rescue modes: auto, manual"));
	EXPECT_TRUE(QmHudNotifications::ShouldExcludeSystemNotification("Example: /map adr3 to call vote for Adrenaline 3. This means that the map name must start with 'a' and contain the characters 'd', 'r' and '3' in that order"));
	EXPECT_TRUE(QmHudNotifications::ShouldExcludeSystemNotification("See /practicecmdlist for a list of all available practice commands. Most commonly used ones are /telecursor, /lasttp and /rescue"));
}

TEST(QmHudNotifications, FormatsKnownSystemNotifications)
{
	char aBuf[256];

	EXPECT_TRUE(QmHudNotifications::TryFormatLocalizedNotificationMessage("Players are not allowed to chat from VPNs at this time", aBuf, sizeof(aBuf)));
	EXPECT_STREQ(aBuf, "当前使用 VPN 的玩家不允许发言");

	EXPECT_TRUE(QmHudNotifications::TryFormatLocalizedNotificationMessage("Unknown argument. Check '/rescuemode list'", aBuf, sizeof(aBuf)));
	EXPECT_STREQ(aBuf, "未知 rescue 模式参数");

	EXPECT_TRUE(QmHudNotifications::TryFormatLocalizedNotificationMessage("Team save already in progress", aBuf, sizeof(aBuf)));
	EXPECT_STREQ(aBuf, "队伍存档已在进行中");

	str_copy(aBuf, "sentinel", sizeof(aBuf));
	EXPECT_FALSE(QmHudNotifications::TryFormatLocalizedNotificationMessage("regular server message", aBuf, sizeof(aBuf)));
	EXPECT_STREQ(aBuf, "");

	str_copy(aBuf, "sentinel", sizeof(aBuf));
	EXPECT_FALSE(QmHudNotifications::TryFormatLocalizedNotificationMessage("", aBuf, sizeof(aBuf)));
	EXPECT_STREQ(aBuf, "");

	str_copy(aBuf, "sentinel", sizeof(aBuf));
	EXPECT_FALSE(QmHudNotifications::TryFormatLocalizedNotificationMessage(nullptr, aBuf, sizeof(aBuf)));
	EXPECT_STREQ(aBuf, "");
}

TEST(QmHudNotificationRules, AnalyzesSoloMessage)
{
	const auto Analysis = QmHudNotifications::AnalyzeServerMessage("You are now in a solo part", QmHudNotifications::ESoloPrompt::Enter);
	EXPECT_EQ(Analysis.m_Route, QmHudNotifications::EServerMessageRoute::Solo);
	EXPECT_EQ(Analysis.m_Class, QmHudNotifications::EServerMessageClass::Prompt);
	EXPECT_EQ(Analysis.m_Domain, QmHudNotifications::EServerMessageDomain::Solo);
	EXPECT_EQ(Analysis.m_SoloPrompt, QmHudNotifications::ESoloPrompt::Enter);
	EXPECT_STREQ(Analysis.m_aLocalizedText, "你现在处于单人区域");
	EXPECT_FALSE(Analysis.m_UseFallbackLocalization);
}

TEST(QmHudNotificationRules, AnalyzesBasicInfoMessage)
{
	const auto Analysis = QmHudNotifications::AnalyzeServerMessage("DDraceNetwork Version: 18.9", QmHudNotifications::ESoloPrompt::None);
	EXPECT_EQ(Analysis.m_Route, QmHudNotifications::EServerMessageRoute::None);
	EXPECT_EQ(Analysis.m_Class, QmHudNotifications::EServerMessageClass::BasicInfo);
	EXPECT_EQ(Analysis.m_Domain, QmHudNotifications::EServerMessageDomain::Status);
	EXPECT_STREQ(Analysis.m_aLocalizedText, "");
	EXPECT_FALSE(Analysis.m_UseFallbackLocalization);
}

TEST(QmHudNotificationRules, AnalyzesStaticTeamMessage)
{
	const auto Analysis = QmHudNotifications::AnalyzeServerMessage("Team save already in progress", QmHudNotifications::ESoloPrompt::None);
	EXPECT_EQ(Analysis.m_Route, QmHudNotifications::EServerMessageRoute::System);
	EXPECT_EQ(Analysis.m_Class, QmHudNotifications::EServerMessageClass::Prompt);
	EXPECT_EQ(Analysis.m_Domain, QmHudNotifications::EServerMessageDomain::Team);
	EXPECT_STREQ(Analysis.m_aLocalizedText, "队伍存档已在进行中");
	EXPECT_FALSE(Analysis.m_UseFallbackLocalization);
}

TEST(QmHudNotificationRules, AnalyzesTeamDynamicMessage)
{
	const auto Analysis = QmHudNotifications::AnalyzeServerMessage("'Alpha' joined team 5", QmHudNotifications::ESoloPrompt::None);
	EXPECT_EQ(Analysis.m_Route, QmHudNotifications::EServerMessageRoute::System);
	EXPECT_EQ(Analysis.m_Class, QmHudNotifications::EServerMessageClass::Prompt);
	EXPECT_EQ(Analysis.m_Domain, QmHudNotifications::EServerMessageDomain::Team);
	EXPECT_STREQ(Analysis.m_aLocalizedText, "'Alpha' 加入了 5 队");
	EXPECT_FALSE(Analysis.m_UseFallbackLocalization);
}

TEST(QmHudNotificationRules, AnalyzesSwapDynamicMessage)
{
	const auto Analysis = QmHudNotifications::AnalyzeServerMessage("You have requested to swap with Beta. Use /cancelswap to cancel the request.", QmHudNotifications::ESoloPrompt::None);
	EXPECT_EQ(Analysis.m_Route, QmHudNotifications::EServerMessageRoute::System);
	EXPECT_EQ(Analysis.m_Class, QmHudNotifications::EServerMessageClass::Prompt);
	EXPECT_EQ(Analysis.m_Domain, QmHudNotifications::EServerMessageDomain::SwapRescue);
	EXPECT_STREQ(Analysis.m_aLocalizedText, "你已向 Beta 发出交换请求。输入 /cancelswap 可取消");
	EXPECT_FALSE(Analysis.m_UseFallbackLocalization);
}

TEST(QmHudNotificationRules, AnalyzesStaticSwapRescueMessage)
{
	const auto Analysis = QmHudNotifications::AnalyzeServerMessage("Unknown argument. Check '/rescuemode list'", QmHudNotifications::ESoloPrompt::None);
	EXPECT_EQ(Analysis.m_Route, QmHudNotifications::EServerMessageRoute::System);
	EXPECT_EQ(Analysis.m_Class, QmHudNotifications::EServerMessageClass::Prompt);
	EXPECT_EQ(Analysis.m_Domain, QmHudNotifications::EServerMessageDomain::SwapRescue);
	EXPECT_STREQ(Analysis.m_aLocalizedText, "未知 rescue 模式参数");
	EXPECT_FALSE(Analysis.m_UseFallbackLocalization);
}

TEST(QmHudNotificationRules, AnalyzesVoteDynamicMessage)
{
	const auto Analysis = QmHudNotifications::AnalyzeServerMessage("'Alice' called vote to kick 'Bob' (afk)", QmHudNotifications::ESoloPrompt::None);
	EXPECT_EQ(Analysis.m_Route, QmHudNotifications::EServerMessageRoute::System);
	EXPECT_EQ(Analysis.m_Class, QmHudNotifications::EServerMessageClass::Prompt);
	EXPECT_EQ(Analysis.m_Domain, QmHudNotifications::EServerMessageDomain::VoteModeration);
	EXPECT_STREQ(Analysis.m_aLocalizedText, "'Alice' 发起了踢出 'Bob' 的投票（原因：afk）");
	EXPECT_FALSE(Analysis.m_UseFallbackLocalization);
}

TEST(QmHudNotificationRules, AnalyzesStaticStatusMessage)
{
	const auto Analysis = QmHudNotifications::AnalyzeServerMessage("Players are not allowed to chat from VPNs at this time", QmHudNotifications::ESoloPrompt::None);
	EXPECT_EQ(Analysis.m_Route, QmHudNotifications::EServerMessageRoute::System);
	EXPECT_EQ(Analysis.m_Class, QmHudNotifications::EServerMessageClass::Prompt);
	EXPECT_EQ(Analysis.m_Domain, QmHudNotifications::EServerMessageDomain::Status);
	EXPECT_STREQ(Analysis.m_aLocalizedText, "当前使用 VPN 的玩家不允许发言");
	EXPECT_FALSE(Analysis.m_UseFallbackLocalization);
}

TEST(QmHudNotificationRules, AnalyzesStaticVoteModerationMessage)
{
	const auto Analysis = QmHudNotifications::AnalyzeServerMessage("You are running a vote, please try again after the vote is done!", QmHudNotifications::ESoloPrompt::None);
	EXPECT_EQ(Analysis.m_Route, QmHudNotifications::EServerMessageRoute::System);
	EXPECT_EQ(Analysis.m_Class, QmHudNotifications::EServerMessageClass::Prompt);
	EXPECT_EQ(Analysis.m_Domain, QmHudNotifications::EServerMessageDomain::VoteModeration);
	EXPECT_STREQ(Analysis.m_aLocalizedText, "你正在发起投票，请等当前投票结束后再试");
	EXPECT_FALSE(Analysis.m_UseFallbackLocalization);
}

TEST(QmHudNotificationRules, FallsBackForUnknownMessage)
{
	const auto Analysis = QmHudNotifications::AnalyzeServerMessage("regular server message", QmHudNotifications::ESoloPrompt::None);
	EXPECT_EQ(Analysis.m_Route, QmHudNotifications::EServerMessageRoute::System);
	EXPECT_EQ(Analysis.m_Class, QmHudNotifications::EServerMessageClass::Prompt);
	EXPECT_EQ(Analysis.m_Domain, QmHudNotifications::EServerMessageDomain::Unknown);
	EXPECT_STREQ(Analysis.m_aLocalizedText, "");
	EXPECT_TRUE(Analysis.m_UseFallbackLocalization);
}

TEST(QmHudNotificationRules, ConsumesHiddenBasicInfoWhenConfigured)
{
	const auto Analysis = QmHudNotifications::AnalyzeServerMessage("DDraceNetwork Version: 18.9", QmHudNotifications::ESoloPrompt::None);
	const auto Decision = QmHudNotifications::DecideServerMessageEntry(Analysis, true, true, false);
	EXPECT_TRUE(Decision.m_ConsumeHiddenMessage);
	EXPECT_FALSE(Decision.m_QueueNotification);
	EXPECT_FALSE(Decision.m_ClearPendingCompatPrompt);
	EXPECT_FALSE(Decision.m_UseFallbackNotification);
}

TEST(QmHudNotificationRules, ConsumesHiddenPromptWhenConfigured)
{
	const auto Analysis = QmHudNotifications::AnalyzeServerMessage("Team save already in progress", QmHudNotifications::ESoloPrompt::None);
	const auto Decision = QmHudNotifications::DecideServerMessageEntry(Analysis, true, false, true);
	EXPECT_TRUE(Decision.m_ConsumeHiddenMessage);
	EXPECT_FALSE(Decision.m_QueueNotification);
	EXPECT_FALSE(Decision.m_ClearPendingCompatPrompt);
	EXPECT_FALSE(Decision.m_UseFallbackNotification);
}

TEST(QmHudNotificationRules, ClearsPendingCompatWhenSoloPromptIsHidden)
{
	const auto Analysis = QmHudNotifications::AnalyzeServerMessage("You are now in a solo part", QmHudNotifications::ESoloPrompt::Enter);
	const auto Decision = QmHudNotifications::DecideServerMessageEntry(Analysis, true, false, true);
	EXPECT_TRUE(Decision.m_ConsumeHiddenMessage);
	EXPECT_FALSE(Decision.m_QueueNotification);
	EXPECT_TRUE(Decision.m_ClearPendingCompatPrompt);
}

TEST(QmHudNotificationRules, DoesNotQueueWhenSystemRouteIsDisabled)
{
	const auto Analysis = QmHudNotifications::AnalyzeServerMessage("Team save already in progress", QmHudNotifications::ESoloPrompt::None);
	const auto Decision = QmHudNotifications::DecideServerMessageEntry(Analysis, false, false, false);
	EXPECT_FALSE(Decision.m_ConsumeHiddenMessage);
	EXPECT_FALSE(Decision.m_QueueNotification);
	EXPECT_FALSE(Decision.m_ClearPendingCompatPrompt);
	EXPECT_FALSE(Decision.m_UseFallbackNotification);
}

TEST(QmHudNotificationRules, KeepsUnknownFallbackNotificationWhenSystemRouteIsEnabled)
{
	const auto Analysis = QmHudNotifications::AnalyzeServerMessage("regular server message", QmHudNotifications::ESoloPrompt::None);
	const auto Decision = QmHudNotifications::DecideServerMessageEntry(Analysis, true, false, false);
	EXPECT_FALSE(Decision.m_ConsumeHiddenMessage);
	EXPECT_TRUE(Decision.m_QueueNotification);
	EXPECT_FALSE(Decision.m_ClearPendingCompatPrompt);
	EXPECT_TRUE(Decision.m_UseFallbackNotification);
}

TEST(QmHudNotifications, HandleServerChatUsesFallbackNotificationForUnknownMessage)
{
	CTestHudNotifications Notifications;
	QmHudNotifications::SServerMessageAnalysis Analysis;
	EXPECT_TRUE(Notifications.HandleServerChat("regular server message", true, false, false, &Analysis));
	EXPECT_TRUE(Analysis.m_UseFallbackLocalization);
	EXPECT_EQ(Notifications.NotificationCountForTests(), 1);
	EXPECT_STREQ(Notifications.LastNotificationTextForTests(), "regular server message");
}

TEST(QmHudNotifications, HandleServerChatRespectsDisabledSystemRoute)
{
	CTestHudNotifications Notifications;
	QmHudNotifications::SServerMessageAnalysis Analysis;
	EXPECT_FALSE(Notifications.HandleServerChat("Team save already in progress", false, false, false, &Analysis));
	EXPECT_EQ(Analysis.m_Class, QmHudNotifications::EServerMessageClass::Prompt);
	EXPECT_EQ(Notifications.NotificationCountForTests(), 0);
}

TEST(QmHudNotifications, HandleServerChatConsumesHiddenBasicInfo)
{
	CTestHudNotifications Notifications;
	QmHudNotifications::SServerMessageAnalysis Analysis;
	EXPECT_TRUE(Notifications.HandleServerChat("DDraceNetwork Version: 18.9", true, true, false, &Analysis));
	EXPECT_EQ(Analysis.m_Class, QmHudNotifications::EServerMessageClass::BasicInfo);
	EXPECT_EQ(Notifications.NotificationCountForTests(), 0);
}

TEST(QmHudNotifications, HandleServerChatClearsPendingCompatAfterHiddenSoloPrompt)
{
	CTestHudNotifications Notifications;
	Notifications.SetPendingCompatPromptForTests(QmHudNotifications::ESoloPrompt::Enter, time_get() + time_freq());

	QmHudNotifications::SServerMessageAnalysis HiddenAnalysis;
	EXPECT_TRUE(Notifications.HandleServerChat("You are now in a solo part", true, false, true, &HiddenAnalysis));
	EXPECT_EQ(HiddenAnalysis.m_Route, QmHudNotifications::EServerMessageRoute::Solo);
	EXPECT_EQ(Notifications.PendingCompatPromptForTests(), QmHudNotifications::ESoloPrompt::None);
	EXPECT_EQ(Notifications.NotificationCountForTests(), 0);

	QmHudNotifications::SServerMessageAnalysis FollowupAnalysis;
	EXPECT_TRUE(Notifications.HandleServerChat("You are now out of the solo part", true, false, false, &FollowupAnalysis));
	EXPECT_EQ(FollowupAnalysis.m_Route, QmHudNotifications::EServerMessageRoute::Solo);
	EXPECT_EQ(Notifications.NotificationCountForTests(), 1);
	EXPECT_STREQ(Notifications.LastNotificationTextForTests(), "你现在已离开单人区域");
}

TEST(QmHudNotifications, BuildsEchoPresentationFromQueuedMessage)
{
	constexpr unsigned FallbackEchoColor = 0x445566;
	const auto PlainEcho = QmHudNotifications::BuildEchoNotificationPayload("Regular echo", FallbackEchoColor);
	EXPECT_STREQ(PlainEcho.m_aText, "Regular echo");
	EXPECT_EQ(PlainEcho.m_Color, FallbackEchoColor);

	const auto ColoredEcho = QmHudNotifications::BuildEchoNotificationPayload("[[$FF7F7F]]禅模式: 开启", FallbackEchoColor);
	const unsigned ExpectedColor = color_cast<ColorHSLA>(ColorRGBA(1.0f, 127.0f / 255.0f, 127.0f / 255.0f, 1.0f)).Pack(false);
	EXPECT_STREQ(ColoredEcho.m_aText, "禅模式: 开启");
	EXPECT_EQ(ColoredEcho.m_Color, ExpectedColor);

	const auto EmptyEcho = QmHudNotifications::BuildEchoNotificationPayload(nullptr, FallbackEchoColor);
	EXPECT_STREQ(EmptyEcho.m_aText, "");
	EXPECT_EQ(EmptyEcho.m_Color, FallbackEchoColor);
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
