/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "menus_start.h"

#include <base/str.h>

#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <engine/storage.h>
#include <engine/textrender.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include <generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/QmUi/QmAnimCurves.h>
#include <game/client/QmUi/QmAnimResolve.h>
#include <game/client/QmUi/QmLayout.h>
#include <game/client/QmUi/QmLegacy.h>
#include <game/client/QmUi/UiButtons.h>
#include <game/client/QmUi/UiContext.h>
#include <game/client/QmUi/UiTokens.h>
#include <game/localization.h>
#include <game/version.h>

#if defined(CONF_PLATFORM_ANDROID)
#include <android/android_main.h>
#endif

using namespace FontIcons;

namespace
{
void ComputeExternalButtons(const CUIRect &MainView, bool UseV2Layout, CUIRect &DiscordButton, CUIRect &LearnButton, CUIRect &TutorialButton, CUIRect &WebsiteButton, CUIRect &StatisticsButton, CUIRect &NewsButton)
{
	CUIRect ExtMenu;
	MainView.VSplitLeft(30.0f, nullptr, &ExtMenu);
	ExtMenu.VSplitLeft(100.0f, &ExtMenu, nullptr);

	if(!UseV2Layout)
	{
		ExtMenu.HSplitBottom(20.0f, &ExtMenu, &DiscordButton);
		ExtMenu.HSplitBottom(5.0f, &ExtMenu, nullptr);
		ExtMenu.HSplitBottom(20.0f, &ExtMenu, &LearnButton);
		ExtMenu.HSplitBottom(5.0f, &ExtMenu, nullptr);
		ExtMenu.HSplitBottom(20.0f, &ExtMenu, &TutorialButton);
		ExtMenu.HSplitBottom(5.0f, &ExtMenu, nullptr);
		ExtMenu.HSplitBottom(20.0f, &ExtMenu, &WebsiteButton);
		ExtMenu.HSplitBottom(5.0f, &ExtMenu, nullptr);
		ExtMenu.HSplitBottom(20.0f, &ExtMenu, &NewsButton);
		ExtMenu.HSplitBottom(5.0f, &ExtMenu, nullptr);
		ExtMenu.HSplitBottom(20.0f, &ExtMenu, &StatisticsButton);
		return;
	}

	CUiV2LayoutEngine LayoutEngine;
	SUiStyle ContainerStyle;
	ContainerStyle.m_Axis = EUiAxis::COLUMN;
	ContainerStyle.m_Gap = 5.0f;
	ContainerStyle.m_AlignItems = EUiAlign::STRETCH;
	ContainerStyle.m_JustifyContent = EUiAlign::END;

	std::vector<SUiLayoutChild> vChildren(6);
	for(SUiLayoutChild &Child : vChildren)
	{
		Child.m_Style.m_Height = SUiLength::Px(20.0f);
	}

	LayoutEngine.ComputeChildren(ContainerStyle, CUiV2LegacyAdapter::FromCUIRect(ExtMenu), vChildren);
	StatisticsButton = CUiV2LegacyAdapter::ToCUIRect(vChildren[0].m_Box);
	NewsButton = CUiV2LegacyAdapter::ToCUIRect(vChildren[1].m_Box);
	WebsiteButton = CUiV2LegacyAdapter::ToCUIRect(vChildren[2].m_Box);
	TutorialButton = CUiV2LegacyAdapter::ToCUIRect(vChildren[3].m_Box);
	LearnButton = CUiV2LegacyAdapter::ToCUIRect(vChildren[4].m_Box);
	DiscordButton = CUiV2LegacyAdapter::ToCUIRect(vChildren[5].m_Box);
}

void ComputeMainButtons(const CUIRect &MenuArea, bool UseV2Layout, CUIRect aMenuButtons[6])
{
	if(!UseV2Layout)
	{
		CUIRect Cursor = MenuArea;
		Cursor.HSplitBottom(40.0f, &Cursor, &aMenuButtons[0]);
		Cursor.HSplitBottom(100.0f, &Cursor, nullptr);
		Cursor.HSplitBottom(40.0f, &Cursor, &aMenuButtons[1]);
		Cursor.HSplitBottom(5.0f, &Cursor, nullptr);
		Cursor.HSplitBottom(40.0f, &Cursor, &aMenuButtons[2]);
		Cursor.HSplitBottom(5.0f, &Cursor, nullptr);
		Cursor.HSplitBottom(40.0f, &Cursor, &aMenuButtons[3]);
		Cursor.HSplitBottom(5.0f, &Cursor, nullptr);
		Cursor.HSplitBottom(40.0f, &Cursor, &aMenuButtons[4]);
		Cursor.HSplitBottom(5.0f, &Cursor, nullptr);
		Cursor.HSplitBottom(40.0f, &Cursor, &aMenuButtons[5]);
		return;
	}

	CUIRect TopArea = MenuArea;
	TopArea.HSplitBottom(40.0f, &TopArea, &aMenuButtons[0]);
	TopArea.HSplitBottom(100.0f, &TopArea, nullptr);

	CUiV2LayoutEngine LayoutEngine;
	SUiStyle ContainerStyle;
	ContainerStyle.m_Axis = EUiAxis::COLUMN;
	ContainerStyle.m_Gap = 5.0f;
	ContainerStyle.m_AlignItems = EUiAlign::STRETCH;
	ContainerStyle.m_JustifyContent = EUiAlign::END;

	std::vector<SUiLayoutChild> vChildren(5);
	for(SUiLayoutChild &Child : vChildren)
	{
		Child.m_Style.m_Height = SUiLength::Px(40.0f);
	}

	LayoutEngine.ComputeChildren(ContainerStyle, CUiV2LegacyAdapter::FromCUIRect(TopArea), vChildren);
	aMenuButtons[5] = CUiV2LegacyAdapter::ToCUIRect(vChildren[0].m_Box);
	aMenuButtons[4] = CUiV2LegacyAdapter::ToCUIRect(vChildren[1].m_Box);
	aMenuButtons[3] = CUiV2LegacyAdapter::ToCUIRect(vChildren[2].m_Box);
	aMenuButtons[2] = CUiV2LegacyAdapter::ToCUIRect(vChildren[3].m_Box);
	aMenuButtons[1] = CUiV2LegacyAdapter::ToCUIRect(vChildren[4].m_Box);
}

}

void CMenusStart::RenderStartMenu(CUIRect MainView)
{
	RenderStartMenuImpl(MainView, false);
}

void CMenusStart::RenderStartMenuV2(CUIRect MainView)
{
	RenderStartMenuImpl(MainView, true);
}

void CMenusStart::RenderStartMenuImpl(CUIRect MainView, bool UseV2Layout)
{
	GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_START);

	// render logo
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_BANNER].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1, 1, 1, 1);
	IGraphics::CQuadItem QuadItem(MainView.w / 2 - 170, 60, 360, 103);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	const float Rounding = 10.0f;
	const float VMargin = MainView.w / 2 - 190.0f;

	int NewPage = -1;
	CUIRect DiscordButtonRect, LearnButtonRect, TutorialButtonRect, WebsiteButtonRect, StatisticsButtonRect, NewsButtonRect;
	ComputeExternalButtons(MainView, UseV2Layout, DiscordButtonRect, LearnButtonRect, TutorialButtonRect, WebsiteButtonRect, StatisticsButtonRect, NewsButtonRect);
	static CButtonContainer s_DiscordButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_DiscordButton, Localize("Discord"), 0, &DiscordButtonRect, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
	{
		Client()->ViewLink(Localize("https://ddnet.org/discord"));
	}

	static CButtonContainer s_LearnButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_LearnButton, Localize("Learn"), 0, &LearnButtonRect, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
	{
		Client()->ViewLink(Localize("https://wiki.ddnet.org/"));
	}

	static CButtonContainer s_TutorialButton;
	static float s_JoinTutorialTime = 0.0f;
	if(GameClient()->m_Menus.DoButton_Menu(&s_TutorialButton, Localize("Tutorial"), 0, &TutorialButtonRect, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)) ||
		(s_JoinTutorialTime != 0.0f && Client()->LocalTime() >= s_JoinTutorialTime))
	{
		// Activate internet tab before joining tutorial to make sure the server info
		// for the tutorial servers is available.
		GameClient()->m_Menus.SetMenuPage(CMenus::PAGE_INTERNET);
		GameClient()->m_Menus.RefreshBrowserTab(true);
		const char *pAddr = ServerBrowser()->GetTutorialServer();
		if(pAddr)
		{
			Client()->Connect(pAddr);
			s_JoinTutorialTime = 0.0f;
		}
		else if(s_JoinTutorialTime == 0.0f)
		{
			dbg_msg("menus", "couldn't find tutorial server, retrying in 5 seconds");
			s_JoinTutorialTime = Client()->LocalTime() + 5.0f;
		}
		else
		{
			Client()->AddWarning(SWarning(Localize("Can't find a Tutorial server")));
			s_JoinTutorialTime = 0.0f;
		}
	}

	static CButtonContainer s_WebsiteButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_WebsiteButton, Localize("Website"), 0, &WebsiteButtonRect, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
	{
		Client()->ViewLink("https://ddnet.org/");
	}

	static CButtonContainer s_StatisticsButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_StatisticsButton, "统计", 0, &StatisticsButtonRect, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
		NewPage = CMenus::PAGE_STATS;

	static CButtonContainer s_NewsButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_NewsButton, Localize("News"), 0, &NewsButtonRect, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, g_Config.m_UiUnreadNews ? ColorRGBA(0.0f, 1.0f, 0.0f, 0.25f) : ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)) || CheckHotKey(KEY_N))
		NewPage = CMenus::PAGE_NEWS;

	CUIRect Menu;
	MainView.VMargin(VMargin, &Menu);
	CUIRect QuitNote;
	Menu.HSplitBottom(22.0f, &Menu, &QuitNote);
	CUIRect Line1, Line2;
	QuitNote.HSplitTop(11.0f, &Line1, &QuitNote);
	QuitNote.HSplitTop(11.0f, &Line2, nullptr);
	// feat-004: render the slogan with TEXT_TIP color so it reads as a
	// secondary inscription rather than competing with the primary controls.
	TextRender()->TextColor(ui_token::color::TEXT_TIP);
	Ui()->DoLabel(&Line1, "在我死去之前", 6.0f, TEXTALIGN_MC);
	Ui()->DoLabel(&Line2, "    谨以此端,回忆我", 3.0f, TEXTALIGN_MC);
	TextRender()->TextColor(ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f));

	const bool LocalServerRunning = GameClient()->m_LocalServer.IsServerRunning();
	const bool EditorDirty = GameClient()->Editor()->HasUnsavedData();
	{
		constexpr int MenuButtonCount = 6;
		CUIRect aMenuButtons[MenuButtonCount];
		ComputeMainButtons(Menu, UseV2Layout, aMenuButtons);

		// feat-004: replace the hover-scale animation block with feat-003
		// widgets. Play gets PrimaryButton (Steam-blue fill) for hero status;
		// the other five use SecondaryButton (border-only, hover-tint).
		IUiContext Ctx;
		Ctx.m_pUi = Ui();
		Ctx.m_pMenus = &GameClient()->m_Menus;
		Ctx.m_pAnim = &GameClient()->UiRuntimeV2()->AnimRuntime();
		Ctx.m_pTooltips = &GameClient()->m_Tooltips;
		Ctx.m_pTextRender = TextRender();
		Ctx.m_ScopeHash = MakeUiScopeHash("start_menu");

		static CButtonContainer s_QuitButton;
		bool UsedEscape = false;
		if(ui_widget::SecondaryButton(Ctx, &s_QuitButton, Localize("Quit"), aMenuButtons[0]) || (UsedEscape = Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE)) || CheckHotKey(KEY_Q))
		{
			if(UsedEscape || EditorDirty || (GameClient()->CurrentRaceTime() / 60 >= g_Config.m_ClConfirmQuitTime && g_Config.m_ClConfirmQuitTime >= 0))
			{
				GameClient()->m_Menus.ShowQuitPopup();
			}
			else
			{
				Client()->Quit();
			}
		}

		static CButtonContainer s_SettingsButton;
		if(ui_widget::SecondaryButton(Ctx, &s_SettingsButton, Localize("Settings"), aMenuButtons[1]) || CheckHotKey(KEY_S))
			NewPage = CMenus::PAGE_SETTINGS;

		static CButtonContainer s_LocalServerButton;
		if(ui_widget::SecondaryButton(Ctx, &s_LocalServerButton, LocalServerRunning ? Localize("Stop server") : Localize("Run server"), aMenuButtons[2]) || (CheckHotKey(KEY_R) && Input()->KeyPress(KEY_R)))
		{
			if(LocalServerRunning)
			{
				GameClient()->m_LocalServer.KillServer();
			}
			else
			{
				GameClient()->m_LocalServer.RunServer({});
			}
		}

		static CButtonContainer s_MapEditorButton;
		if(ui_widget::SecondaryButton(Ctx, &s_MapEditorButton, Localize("Editor"), aMenuButtons[3]) || CheckHotKey(KEY_E))
		{
			g_Config.m_ClEditor = 1;
			Input()->MouseModeRelative();
		}

		static CButtonContainer s_DemoButton;
		if(ui_widget::SecondaryButton(Ctx, &s_DemoButton, Localize("Demos"), aMenuButtons[4]) || CheckHotKey(KEY_D))
		{
			NewPage = CMenus::PAGE_DEMOS;
		}

		static CButtonContainer s_PlayButton;
		if(ui_widget::PrimaryButton(Ctx, &s_PlayButton, Localize("Play", "Start menu"), aMenuButtons[5]) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER) || CheckHotKey(KEY_P))
		{
			NewPage = ((g_Config.m_UiPage >= CMenus::PAGE_INTERNET && g_Config.m_UiPage <= CMenus::PAGE_FAVORITE_COMMUNITY_5) || g_Config.m_UiPage == CMenus::PAGE_FAVORITE_MAPS) ? g_Config.m_UiPage : CMenus::PAGE_INTERNET;
		}
	}

	// render version
	CUIRect CurVersion, ConsoleButton;
	MainView.HSplitBottom(45.0f, nullptr, &CurVersion);
	CurVersion.VSplitRight(40.0f, &CurVersion, nullptr);
	CurVersion.HSplitTop(20.0f, &ConsoleButton, &CurVersion);
	CurVersion.HSplitTop(5.0f, nullptr, &CurVersion);
	ConsoleButton.VSplitRight(40.0f, nullptr, &ConsoleButton);
	Ui()->DoLabel(&CurVersion, GAME_RELEASE_VERSION, 14.0f, TEXTALIGN_MR);

	CUIRect TClientVersion;
	MainView.HSplitTop(15.0f, &TClientVersion, &MainView);
	TClientVersion.VSplitRight(40.0f, &TClientVersion, nullptr);
	char aTBuf[64];
	str_format(aTBuf, sizeof(aTBuf), CLIENT_NAME " %s", CLIENT_RELEASE_VERSION);
	Ui()->DoLabel(&TClientVersion, aTBuf, 14.0f, TEXTALIGN_MR);
#if defined(CONF_AUTOUPDATE)
	CUIRect UpdateToDateText;
	MainView.HSplitTop(15.0f, &UpdateToDateText, nullptr);
	UpdateToDateText.VSplitRight(40.0f, &UpdateToDateText, nullptr);
	if(!GameClient()->m_TClient.m_FetchedTClientInfo)
	{
		Ui()->DoLabel(&UpdateToDateText, Localize("(Fetching Update Info)"), 14.0f, TEXTALIGN_MR);
	}
	else if(GameClient()->m_TClient.NeedUpdate())
	{
		Ui()->DoLabel(&UpdateToDateText, Localize("(需要更新)"), 14.0f, TEXTALIGN_MR);
	}
	else
	{
		Ui()->DoLabel(&UpdateToDateText, Localize("(On Latest)"), 14.0f, TEXTALIGN_MR);
	}
#endif
	static CButtonContainer s_ConsoleButton;
	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGNMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
	if(GameClient()->m_Menus.DoButton_Menu(&s_ConsoleButton, FONT_ICON_TERMINAL, 0, &ConsoleButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.1f)))
	{
		GameClient()->m_GameConsole.Toggle(CGameConsole::CONSOLETYPE_LOCAL);
	}
	TextRender()->SetRenderFlags(0);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);

	if(NewPage != -1)
	{
		GameClient()->m_Menus.SetShowStart(false);
		GameClient()->m_Menus.SetMenuPage(NewPage);
	}
}

bool CMenusStart::CheckHotKey(int Key) const
{
	return !Input()->ShiftIsPressed() && !Input()->ModifierIsPressed() && !Input()->AltIsPressed() && // no modifier
	       Input()->KeyPress(Key) &&
	       !GameClient()->m_GameConsole.IsActive();
}
