#ifndef GAME_CLIENT_COMPONENTS_QMCLIENT_HUD_NOTIFICATIONS_H
#define GAME_CLIENT_COMPONENTS_QMCLIENT_HUD_NOTIFICATIONS_H

#include <base/system.h>

#include <game/client/component.h>
#include <game/client/ui_rect.h>

#include <algorithm>
#include <deque>

namespace QmHudNotifications
{
	enum class ESoloPrompt
	{
		None,
		Enter,
		Leave,
	};

	enum class ETextSource
	{
		System,
		Echo,
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

	struct STextColorConfig
	{
		unsigned m_Color = 0;
		bool m_HasAlpha = false;
	};

	inline ESoloPrompt MatchKnownSoloPrompt(const char *pMessage)
	{
		if(pMessage == nullptr)
			return ESoloPrompt::None;
		if(str_comp(pMessage, "You are now in a solo part") == 0 || str_comp(pMessage, "你现在处于单人区域") == 0)
			return ESoloPrompt::Enter;
		if(str_comp(pMessage, "You are now out of the solo part") == 0 || str_comp(pMessage, "你现在已离开单人区域") == 0)
			return ESoloPrompt::Leave;
		return ESoloPrompt::None;
	}

	inline bool ShouldSuppressSoloChatMessage(const char *pMessage, ESoloPrompt PendingCompatPrompt)
	{
		const ESoloPrompt Known = MatchKnownSoloPrompt(pMessage);
		if(Known == ESoloPrompt::None)
			return false;
		return PendingCompatPrompt == ESoloPrompt::None || Known == PendingCompatPrompt;
	}

	inline bool ShouldExcludeSystemNotification(const char *pMessage)
	{
		if(pMessage == nullptr || pMessage[0] == '\0')
			return true;
		if(str_startswith(pMessage, "DDraceNetwork 版本:") ||
			str_startswith(pMessage, "DDraceNetwork Version:") ||
			str_startswith(pMessage, "请访问 DDNet.org") ||
			str_startswith(pMessage, "Please visit DDNet.org") ||
			str_comp(pMessage, "Players are not allowed to chat from VPNs at this time") == 0 ||
			str_comp(pMessage, "请友善交流。") == 0 ||
			str_comp(pMessage, "未设置服务器规则，请联系管理员。") == 0 ||
			str_endswith(pMessage, " entered and joined the game") ||
			str_endswith(pMessage, " joined the game") ||
			str_startswith(pMessage, "You can see other players. To disable this use DDNet client and type /showothers"))
			return true;
		return false;
	}

	inline EServerMessageRoute ServerMessageRoute(const char *pMessage, ESoloPrompt PendingCompatPrompt, bool RouteSystemMessages)
	{
		if(pMessage == nullptr || pMessage[0] == '\0')
			return EServerMessageRoute::None;
		if(!RouteSystemMessages)
			return EServerMessageRoute::None;
		if(ShouldSuppressSoloChatMessage(pMessage, PendingCompatPrompt))
			return EServerMessageRoute::Solo;
		if(ShouldExcludeSystemNotification(pMessage))
			return EServerMessageRoute::None;
		return EServerMessageRoute::System;
	}

	inline EServerMessageClass ServerMessageClass(const char *pMessage, ESoloPrompt PendingCompatPrompt)
	{
		if(pMessage == nullptr || pMessage[0] == '\0')
			return EServerMessageClass::None;
		if(ShouldSuppressSoloChatMessage(pMessage, PendingCompatPrompt))
			return EServerMessageClass::Prompt;
		if(ShouldExcludeSystemNotification(pMessage))
			return EServerMessageClass::BasicInfo;
		return EServerMessageClass::Prompt;
	}

	inline int ClampVisibleCount(int Value)
	{
		return std::clamp(Value, 1, 8);
	}

	inline int ClampHoldMs(int Value)
	{
		return std::clamp(Value, 500, 10000);
	}

	inline int ClampAnimationMs(int Value)
	{
		return std::clamp(Value, 0, 2000);
	}

	inline int ClampTextSize(int Value)
	{
		return std::clamp(Value, 1, 24);
	}

	inline float SmallTextScale(float FontSize)
	{
		return std::clamp(FontSize / 8.0f, 0.33f, 1.0f);
	}

	inline float PaddingX(float FontSize)
	{
		return 6.0f * SmallTextScale(FontSize);
	}

	inline float PaddingY(float FontSize)
	{
		return 4.0f * SmallTextScale(FontSize);
	}

	inline float MinBoxWidth(float FontSize)
	{
		return 82.0f * SmallTextScale(FontSize);
	}

	inline STextColorConfig TextColorConfig(ETextSource Source, int EchoInheritChatColor, unsigned SystemColor, unsigned EchoOverrideColor, unsigned ChatEchoColor)
	{
		if(Source == ETextSource::Echo && EchoInheritChatColor)
			return {ChatEchoColor, false};
		if(Source == ETextSource::Echo)
			return {EchoOverrideColor, true};
		return {SystemColor, true};
	}
} // namespace QmHudNotifications

class CQmHudNotifications : public CComponent
{
public:
	int Sizeof() const override { return sizeof(*this); }
	void OnReset() override;
	void OnRelease() override;
	void OnNewSnapshot() override;
	void OnRender() override;

	bool QueueEcho(const char *pMessage);
	bool ShouldSuppressServerChat(const char *pMessage);
	bool ShouldConsumeHiddenServerChat(const char *pMessage, bool HideBasicInfo, bool HidePrompt);

private:
	enum class EKind
	{
		SoloEnter,
		SoloLeave,
		System,
		Echo,
	};

	struct SNotification
	{
		EKind m_Kind = EKind::Echo;
		char m_aText[256] = {};
		int64_t m_StartTime = 0;
	};

	std::deque<SNotification> m_vNotifications;
	bool m_HasLastSolo = false;
	bool m_LastSolo = false;
	QmHudNotifications::ESoloPrompt m_PendingCompatPrompt = QmHudNotifications::ESoloPrompt::None;
	int64_t m_PendingCompatUntil = 0;

	void Queue(EKind Kind, const char *pText);
	void QueueSolo(QmHudNotifications::ESoloPrompt Prompt);
	bool LocalSoloState(bool &Solo) const;
	void RenderNotifications(const CUIRect &BaseRect, bool Preview);
};

#endif
