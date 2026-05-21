#include "hud_notifications.h"

#include <base/color.h>
#include <base/system.h>

#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <game/client/gameclient.h>
#include <game/localization.h>

#include <algorithm>

namespace
{
	float EaseOutCubic(float t)
	{
		t = std::clamp(t, 0.0f, 1.0f);
		return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
	}

	ColorRGBA ApplyAlpha(ColorRGBA Color, float Alpha)
	{
		Color.a *= std::clamp(Alpha, 0.0f, 1.0f);
		return Color;
	}
} // namespace

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
	bool Solo = false;
	if(!LocalSoloState(Solo))
	{
		m_HasLastSolo = false;
		return;
	}

	if(!m_HasLastSolo)
	{
		m_HasLastSolo = true;
		m_LastSolo = Solo;
		return;
	}

	if(Solo == m_LastSolo)
		return;

	const auto Prompt = Solo ? QmHudNotifications::ESoloPrompt::Enter : QmHudNotifications::ESoloPrompt::Leave;
	if(g_Config.m_QmHudNotificationsCompatSolo)
	{
		m_PendingCompatPrompt = Prompt;
		m_PendingCompatUntil = time_get() + time_freq() / 2;
	}
	m_LastSolo = Solo;
}

void CQmHudNotifications::OnRender()
{
	const bool Preview = GameClient()->m_HudEditor.IsActive() && m_vNotifications.empty();
	if(!Preview && m_vNotifications.empty())
		return;
	if(!Preview && Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	const int HoldMs = QmHudNotifications::ClampHoldMs(g_Config.m_QmHudNotificationsHoldMs);
	const int AnimMs = QmHudNotifications::ClampAnimationMs(g_Config.m_QmHudNotificationsAnimMs);
	const int64_t Now = time_get();
	const int64_t Lifetime = (int64_t)(HoldMs + 2 * AnimMs) * time_freq() / 1000;
	while(!m_vNotifications.empty() && Now - m_vNotifications.front().m_StartTime > Lifetime)
		m_vNotifications.pop_front();
	if(!Preview && m_vNotifications.empty())
		return;

	const float Height = 300.0f;
	const float Width = Height * Graphics()->ScreenAspect();
	float SavedScreenX0 = 0.0f;
	float SavedScreenY0 = 0.0f;
	float SavedScreenX1 = 0.0f;
	float SavedScreenY1 = 0.0f;
	Graphics()->GetScreen(&SavedScreenX0, &SavedScreenY0, &SavedScreenX1, &SavedScreenY1);
	Graphics()->MapScreen(0.0f, 0.0f, Width, Height);

	CUIRect DefaultRect;
	DefaultRect.w = 172.0f;
	DefaultRect.h = 68.0f;
	DefaultRect.x = Width - DefaultRect.w - 8.0f;
	DefaultRect.y = 96.0f;

	const auto HudEditorScope = GameClient()->m_HudEditor.BeginTransform(EHudEditorElement::HudNotifications, DefaultRect);
	RenderNotifications(DefaultRect, Preview);
	GameClient()->m_HudEditor.EndTransform(HudEditorScope);
	Graphics()->MapScreen(SavedScreenX0, SavedScreenY0, SavedScreenX1, SavedScreenY1);
}

bool CQmHudNotifications::QueueEcho(const char *pMessage)
{
	if(!g_Config.m_QmHudNotificationsEcho)
		return false;
	if(pMessage == nullptr || pMessage[0] == '\0')
		return false;
	Queue(EKind::Echo, pMessage);
	return true;
}

bool CQmHudNotifications::ShouldSuppressServerChat(const char *pMessage)
{
	if(g_Config.m_QmHudNotificationsCompatSolo && m_PendingCompatPrompt != QmHudNotifications::ESoloPrompt::None && time_get() > m_PendingCompatUntil)
	{
		m_PendingCompatPrompt = QmHudNotifications::ESoloPrompt::None;
	}

	const QmHudNotifications::EServerMessageRoute Route = QmHudNotifications::ServerMessageRoute(pMessage, m_PendingCompatPrompt, g_Config.m_QmHudNotificationsSystem != 0);
	if(Route == QmHudNotifications::EServerMessageRoute::Solo)
	{
		m_PendingCompatPrompt = QmHudNotifications::ESoloPrompt::None;
		QueueSolo(QmHudNotifications::MatchKnownSoloPrompt(pMessage));
		return true;
	}
	if(Route == QmHudNotifications::EServerMessageRoute::System)
	{
		Queue(EKind::System, pMessage);
		return true;
	}
	return false;
}

bool CQmHudNotifications::ShouldConsumeHiddenServerChat(const char *pMessage, bool HideBasicInfo, bool HidePrompt)
{
	if(!HideBasicInfo && !HidePrompt)
		return false;
	if(g_Config.m_QmHudNotificationsCompatSolo && m_PendingCompatPrompt != QmHudNotifications::ESoloPrompt::None && time_get() > m_PendingCompatUntil)
	{
		m_PendingCompatPrompt = QmHudNotifications::ESoloPrompt::None;
	}

	const QmHudNotifications::EServerMessageClass MessageClass = QmHudNotifications::ServerMessageClass(pMessage, m_PendingCompatPrompt);
	if(MessageClass == QmHudNotifications::EServerMessageClass::BasicInfo)
		return HideBasicInfo;
	if(MessageClass == QmHudNotifications::EServerMessageClass::Prompt && HidePrompt)
	{
		if(QmHudNotifications::ShouldSuppressSoloChatMessage(pMessage, m_PendingCompatPrompt))
			m_PendingCompatPrompt = QmHudNotifications::ESoloPrompt::None;
		return true;
	}
	return false;
}

void CQmHudNotifications::Queue(EKind Kind, const char *pText)
{
	if(pText == nullptr || pText[0] == '\0')
		return;

	SNotification Notification;
	Notification.m_Kind = Kind;
	str_copy(Notification.m_aText, pText);
	Notification.m_StartTime = time_get();
	m_vNotifications.push_back(Notification);
	while((int)m_vNotifications.size() > QmHudNotifications::ClampVisibleCount(g_Config.m_QmHudNotificationsMaxVisible))
		m_vNotifications.pop_front();
}

void CQmHudNotifications::QueueSolo(QmHudNotifications::ESoloPrompt Prompt)
{
	if(Prompt == QmHudNotifications::ESoloPrompt::Enter)
		Queue(EKind::SoloEnter, Localize("你现在处于单人区域"));
	else if(Prompt == QmHudNotifications::ESoloPrompt::Leave)
		Queue(EKind::SoloLeave, Localize("你现在已离开单人区域"));
}

bool CQmHudNotifications::LocalSoloState(bool &Solo) const
{
	const int LocalId = GameClient()->m_Snap.m_LocalClientId;
	if(LocalId < 0 || LocalId >= MAX_CLIENTS)
		return false;
	if(!GameClient()->m_aClients[LocalId].m_Active)
		return false;
	Solo = GameClient()->m_aClients[LocalId].m_Solo;
	return true;
}

void CQmHudNotifications::RenderNotifications(const CUIRect &BaseRect, bool Preview)
{
	const int MaxVisible = QmHudNotifications::ClampVisibleCount(g_Config.m_QmHudNotificationsMaxVisible);
	const int HoldMs = QmHudNotifications::ClampHoldMs(g_Config.m_QmHudNotificationsHoldMs);
	const int AnimMs = QmHudNotifications::ClampAnimationMs(g_Config.m_QmHudNotificationsAnimMs);
	const int AnimType = std::clamp(g_Config.m_QmHudNotificationsAnimType, 0, 2);
	const float FontSize = (float)QmHudNotifications::ClampTextSize(g_Config.m_QmHudNotificationsTextSize);
	const float PaddingX = QmHudNotifications::PaddingX(FontSize);
	const float PaddingY = QmHudNotifications::PaddingY(FontSize);
	const float MinBoxWidth = QmHudNotifications::MinBoxWidth(FontSize);
	const float Gap = 4.0f * QmHudNotifications::SmallTextScale(FontSize);
	const float TextMaxWidth = maximum(1.0f, BaseRect.w - PaddingX * 2.0f);
	const int64_t Now = time_get();

	SNotification PreviewNotification;
	const SNotification *apVisible[8] = {};
	int NumVisible = 0;
	if(Preview)
	{
		PreviewNotification.m_Kind = EKind::SoloEnter;
		str_copy(PreviewNotification.m_aText, Localize("你现在处于单人区域"));
		PreviewNotification.m_StartTime = Now;
		apVisible[NumVisible++] = &PreviewNotification;
	}
	else
	{
		for(int SourceIndex = (int)m_vNotifications.size() - 1; SourceIndex >= 0 && NumVisible < MaxVisible; --SourceIndex)
			apVisible[NumVisible++] = &m_vNotifications[SourceIndex];
	}

	if(NumVisible == 0)
		return;

	ColorRGBA BgColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_QmHudNotificationsBgColor, true));
	float Y = BaseRect.y;
	float UsedHeight = 0.0f;

	for(int i = 0; i < NumVisible; ++i)
	{
		const SNotification &Notification = *apVisible[i];
		const int ElapsedMs = Preview ? AnimMs : (int)((Now - Notification.m_StartTime) * 1000 / time_freq());
		float Alpha = 1.0f;
		float OffsetX = 0.0f;
		if(AnimType != 2 && AnimMs > 0)
		{
			if(ElapsedMs < AnimMs)
				Alpha = EaseOutCubic(ElapsedMs / (float)AnimMs);
			else if(ElapsedMs > AnimMs + HoldMs)
				Alpha = 1.0f - EaseOutCubic((ElapsedMs - AnimMs - HoldMs) / (float)AnimMs);
			if(AnimType == 0)
				OffsetX = (1.0f - Alpha) * 32.0f;
		}

		const STextBoundingBox TextBox = TextRender()->TextBoundingBox(FontSize, Notification.m_aText, -1, TextMaxWidth);
		const float BoxW = minimum(BaseRect.w, maximum(MinBoxWidth, TextBox.m_W + PaddingX * 2.0f));
		const float BoxH = maximum(FontSize + PaddingY * 2.0f, TextBox.m_H + PaddingY * 2.0f);
		CUIRect Box = {BaseRect.x + BaseRect.w - BoxW + OffsetX, Y, BoxW, BoxH};
		const float CornerRadius = minimum(6.0f, BoxH / 2.0f);
		Box.Draw(ApplyAlpha(BgColor, Alpha), IGraphics::CORNER_ALL, CornerRadius);

		const QmHudNotifications::ETextSource TextSource = Notification.m_Kind == EKind::Echo ? QmHudNotifications::ETextSource::Echo : QmHudNotifications::ETextSource::System;
		const QmHudNotifications::STextColorConfig TextColorConfig = QmHudNotifications::TextColorConfig(TextSource, g_Config.m_QmHudNotificationsEchoInheritColor, g_Config.m_QmHudNotificationsTextColor, g_Config.m_QmHudNotificationsEchoTextColor, g_Config.m_ClMessageClientColor);
		const ColorRGBA TextColor = color_cast<ColorRGBA>(ColorHSLA(TextColorConfig.m_Color, TextColorConfig.m_HasAlpha));
		TextRender()->TextColor(ApplyAlpha(TextColor, Alpha));
		TextRender()->Text(Box.x + PaddingX, Box.y + PaddingY, FontSize, Notification.m_aText, TextMaxWidth);

		Y += BoxH + Gap;
		UsedHeight += BoxH + (i + 1 < NumVisible ? Gap : 0.0f);
	}

	TextRender()->TextColor(TextRender()->DefaultTextColor());
	GameClient()->m_HudEditor.UpdateVisibleRect(EHudEditorElement::HudNotifications, {BaseRect.x, BaseRect.y, BaseRect.w, maximum(BaseRect.h, UsedHeight)});
}
