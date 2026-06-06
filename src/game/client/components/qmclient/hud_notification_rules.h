#ifndef GAME_CLIENT_COMPONENTS_QMCLIENT_HUD_NOTIFICATION_RULES_H
#define GAME_CLIENT_COMPONENTS_QMCLIENT_HUD_NOTIFICATION_RULES_H

#include "hud_notification_static_rules.h"

#include <base/system.h>

#include <cstddef>

namespace QmHudNotifications
{
	enum class ESoloPrompt
	{
		None,
		Enter,
		Leave,
	};

	enum class EServerMessageRoute
	{
		None,
		Solo,
		System,
	};

	enum class EServerMessageClass
	{
		None,
		BasicInfo,
		Prompt,
	};

	enum class EServerMessageDomain
	{
		None,
		Solo,
		Team,
		SwapRescue,
		VoteModeration,
		Status,
		Unknown,
	};

	struct SServerMessageAnalysis
	{
		EServerMessageRoute m_Route = EServerMessageRoute::None;
		EServerMessageClass m_Class = EServerMessageClass::None;
		EServerMessageDomain m_Domain = EServerMessageDomain::None;
		ESoloPrompt m_SoloPrompt = ESoloPrompt::None;
		bool m_UseFallbackLocalization = false;
		char m_aLocalizedText[256] = {};
	};

	struct SServerMessageEntryDecision
	{
		bool m_ConsumeHiddenMessage = false;
		bool m_QueueNotification = false;
		bool m_ClearPendingCompatPrompt = false;
		bool m_UseFallbackNotification = false;
	};

	ESoloPrompt MatchKnownSoloPrompt(const char *pMessage);
	bool ShouldSuppressSoloChatMessage(const char *pMessage, ESoloPrompt PendingCompatPrompt);
	bool ShouldExcludeSystemNotification(const char *pMessage);
	SServerMessageAnalysis AnalyzeServerMessage(const char *pMessage, ESoloPrompt PendingCompatPrompt);
	SServerMessageEntryDecision DecideServerMessageEntry(const SServerMessageAnalysis &Analysis, bool RouteSystemMessages, bool HideBasicInfo, bool HidePrompt);
	EServerMessageRoute ServerMessageRoute(const char *pMessage, ESoloPrompt PendingCompatPrompt, bool RouteSystemMessages);
	EServerMessageClass ServerMessageClass(const char *pMessage, ESoloPrompt PendingCompatPrompt);
	bool TryFormatLocalizedNotificationMessage(const char *pMessage, char *pBuf, size_t BufSize);
} // namespace QmHudNotifications

#endif
