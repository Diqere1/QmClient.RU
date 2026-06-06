#include <test/test.h>

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>

namespace
{

std::string ReadTextFile(const char *pPath)
{
	std::ifstream File(pPath);
	EXPECT_TRUE(File.good()) << pPath;
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	return Buffer.str();
}

std::string FunctionBody(const std::string &Source, const std::string &Signature)
{
	const size_t FunctionStart = Source.find(Signature);
	EXPECT_NE(FunctionStart, std::string::npos) << Signature;
	const size_t BodyStart = Source.find("{", FunctionStart);
	EXPECT_NE(BodyStart, std::string::npos) << Signature;
	int Depth = 0;
	for(size_t Index = BodyStart; Index < Source.size(); ++Index)
	{
		if(Source[Index] == '{')
			++Depth;
		else if(Source[Index] == '}')
		{
			--Depth;
			if(Depth == 0)
				return Source.substr(BodyStart, Index - BodyStart);
		}
	}
	ADD_FAILURE() << Signature;
	return {};
}

std::string BlockBodyAfter(const std::string &Source, const std::string &Anchor)
{
	const size_t AnchorPos = Source.find(Anchor);
	EXPECT_NE(AnchorPos, std::string::npos) << Anchor;
	const size_t BodyStart = Source.find("{", AnchorPos);
	EXPECT_NE(BodyStart, std::string::npos) << Anchor;
	int Depth = 0;
	for(size_t Index = BodyStart; Index < Source.size(); ++Index)
	{
		if(Source[Index] == '{')
			++Depth;
		else if(Source[Index] == '}')
		{
			--Depth;
			if(Depth == 0)
				return Source.substr(BodyStart, Index - BodyStart);
		}
	}
	ADD_FAILURE() << Anchor;
	return {};
}

size_t MatchingBrace(const std::string &Source, size_t BodyStart)
{
	int Depth = 0;
	for(size_t Index = BodyStart; Index < Source.size(); ++Index)
	{
		if(Source[Index] == '{')
			++Depth;
		else if(Source[Index] == '}')
		{
			--Depth;
			if(Depth == 0)
				return Index;
		}
	}
	return std::string::npos;
}

} // namespace

TEST(QmNewUiMenuBranches, MenubarUsesExplicitQmNewUiColorBranch)
{
	const std::string Source = ReadTextFile("src/game/client/components/menus.cpp");
	const std::string DoMenuTabV2 = FunctionBody(Source, "int CMenus::DoMenuTabV2(");
	const std::string RenderMenubar = FunctionBody(Source, "void CMenus::RenderMenubar(");
	const size_t UseNewUiIfPos = RenderMenubar.find("if(UseNewUi)");
	ASSERT_NE(UseNewUiIfPos, std::string::npos);
	const size_t UseNewUiBodyStart = RenderMenubar.find("{", UseNewUiIfPos);
	ASSERT_NE(UseNewUiBodyStart, std::string::npos);
	const size_t UseNewUiBodyEnd = MatchingBrace(RenderMenubar, UseNewUiBodyStart);
	ASSERT_NE(UseNewUiBodyEnd, std::string::npos);
	const std::string UseNewUiBlock = RenderMenubar.substr(UseNewUiBodyStart, UseNewUiBodyEnd - UseNewUiBodyStart);
	const size_t OldUiElsePos = RenderMenubar.find("else", UseNewUiBodyEnd);
	ASSERT_NE(OldUiElsePos, std::string::npos);
	const size_t OldUiBodyStart = RenderMenubar.find("{", OldUiElsePos);
	ASSERT_NE(OldUiBodyStart, std::string::npos);
	const size_t OldUiBodyEnd = MatchingBrace(RenderMenubar, OldUiBodyStart);
	ASSERT_NE(OldUiBodyEnd, std::string::npos);
	const std::string OldUiBlock = RenderMenubar.substr(OldUiBodyStart, OldUiBodyEnd - OldUiBodyStart);
	const size_t HoverBranch = DoMenuTabV2.find("if(Hover)");
	const size_t ActiveBranch = DoMenuTabV2.find("else if(Active)");

	EXPECT_NE(Source.find("const bool UseNewUi = g_Config.m_QmNewUi != 0;"), std::string::npos);
	EXPECT_NE(Source.find("MenuTabDefaultColor("), std::string::npos);
	EXPECT_NE(Source.find("MenuTabActiveColor("), std::string::npos);
	EXPECT_NE(Source.find("MenuTabHoverColor("), std::string::npos);
	EXPECT_NE(Source.find("MenuIconButtonDefaultColor("), std::string::npos);
	ASSERT_NE(HoverBranch, std::string::npos);
	ASSERT_NE(ActiveBranch, std::string::npos);
	EXPECT_LT(HoverBranch, ActiveBranch);
	EXPECT_NE(DoMenuTabV2.find("Target = pCustomHover != nullptr ? *pCustomHover : HoverColor;"), std::string::npos);
	EXPECT_NE(DoMenuTabV2.find("Target = pCustomActive != nullptr ? *pCustomActive : ActiveColor;"), std::string::npos);
	EXPECT_NE(Source.find("return UseNewUi ? MenuTabDefaultColor() : ColorRGBA(0.0f, 0.0f, 0.0f, 0.0f);"), std::string::npos);
	EXPECT_NE(Source.find("const ColorRGBA DefaultColor = UseNewUi ? MenuTabDefaultColor() : ms_ColorTabbarInactive;"), std::string::npos);
	EXPECT_NE(Source.find("const ColorRGBA ActiveColor = UseNewUi ? MenuTabActiveColor() : ms_ColorTabbarActive;"), std::string::npos);
	EXPECT_NE(Source.find("const ColorRGBA HoverColor = UseNewUi ? MenuTabHoverColor() : ms_ColorTabbarHover;"), std::string::npos);
	EXPECT_NE(Source.find("const bool UseNewUi = g_Config.m_QmNewUi != 0;"), std::string::npos);
	EXPECT_NE(Source.find("ColorRGBA InactiveColor = MenuTabDefaultColor();"), std::string::npos);
	EXPECT_NE(Source.find("ColorRGBA ActiveColor = MenuTabActiveColor();"), std::string::npos);
	EXPECT_NE(Source.find("ColorRGBA HoverColor = MenuMenubarHoverColor();"), std::string::npos);
	EXPECT_NE(Source.find("ColorRGBA InactiveColor = ms_ColorTabbarInactive;"), std::string::npos);
	EXPECT_NE(Source.find("ColorRGBA ActiveColor = ms_ColorTabbarActive;"), std::string::npos);
	EXPECT_NE(Source.find("ColorRGBA HoverColor = ms_ColorTabbarHover;"), std::string::npos);
	EXPECT_NE(Source.find("const ColorRGBA IndicatorColor = g_Config.m_QmNewUi != 0 ? MenuUiColorAccent(1.0f) : ui_token::color::ACCENT_PRIMARY;"), std::string::npos);
	EXPECT_NE(RenderMenubar.find("if(UseNewUi)"), std::string::npos);
	EXPECT_NE(UseNewUiBlock.find("if(DoMenuTabV2(&s_SettingsButton"), std::string::npos);
	EXPECT_NE(UseNewUiBlock.find("if(DoMenuTabV2(&s_InternetButton"), std::string::npos);
	EXPECT_EQ(UseNewUiBlock.find("DoButton_MenuTab(&s_SettingsButton"), std::string::npos);
	EXPECT_EQ(UseNewUiBlock.find("DoButton_MenuTab(&s_InternetButton"), std::string::npos);
	EXPECT_NE(OldUiBlock.find("if(DoButton_MenuTab(&s_SettingsButton"), std::string::npos);
	EXPECT_NE(OldUiBlock.find("if(DoButton_MenuTab(&s_InternetButton"), std::string::npos);
	EXPECT_EQ(OldUiBlock.find("DoMenuTabV2(&s_SettingsButton"), std::string::npos);
	EXPECT_EQ(OldUiBlock.find("DoMenuTabV2(&s_InternetButton"), std::string::npos);
}

TEST(QmNewUiMenuBranches, BrowserUsesExplicitQmNewUiShellBranch)
{
	const std::string Source = ReadTextFile("src/game/client/components/menus_browser.cpp");

	EXPECT_NE(Source.find("const bool UseNewUi = g_Config.m_QmNewUi != 0;"), std::string::npos);
	EXPECT_NE(Source.find("if(UseNewUi)"), std::string::npos);
	EXPECT_NE(Source.find("ServerListBase.Draw(MenuPanelColor()"), std::string::npos);
	EXPECT_NE(Source.find("(void)DrawBackground;"), std::string::npos);
	EXPECT_EQ(Source.find("View.Draw(ms_ColorTabbarActive"), std::string::npos);
	EXPECT_NE(Source.find("View.Margin(std::clamp(View.w * 0.008f, 4.0f, 8.0f), &View);"), std::string::npos);
	EXPECT_NE(Source.find("const float ToolBoxWidth = UseNewUi ? 205.0f : 188.0f;"), std::string::npos);
	EXPECT_NE(Source.find("const float ColumnGap = UseNewUi ? 10.0f : 6.0f;"), std::string::npos);
	EXPECT_NE(Source.find("const float StatusHeight = UseNewUi ? 84.0f : 76.0f;"), std::string::npos);
	EXPECT_NE(Source.find("CUIRect ServerListStackBase = ServerListBase;"), std::string::npos);
	EXPECT_NE(Source.find("ServerListStackBase.HSplitBottom(StatusHeight, &ServerListBase, &StatusBox);"), std::string::npos);
	EXPECT_NE(Source.find("StatusBox.y = ServerListStackBase.y + ServerListStackBase.h - StatusHeight;"), std::string::npos);
	EXPECT_NE(Source.find("ServerListBase.h = maximum(StatusBox.y - ColumnGap - ServerListBase.y, 0.0f);"), std::string::npos);
	EXPECT_EQ(Source.find("ServerListBase.HSplitBottom(ColumnGap, &ServerListBase, nullptr);"), std::string::npos);
	EXPECT_NE(Source.find("ServerListBase.Margin(std::clamp(ServerListBase.w * 0.006f, 1.0f, 4.0f), &ServerListBase);"), std::string::npos);
}

TEST(QmNewUiMenuBranches, QmLocalizationEnglishOverlayUsesExplicitEnglishFile)
{
	const std::string Source = ReadTextFile("src/game/client/gameclient.cpp");

	EXPECT_EQ(Source.find("str_format(aBuf, sizeof(aBuf), \"qmclient/%s\", g_Config.m_ClLanguagefile);"), std::string::npos);
	EXPECT_NE(Source.find("static void LoadQmClientLanguageOverlay("), std::string::npos);
	EXPECT_EQ(Source.find("const char *pQmLanguageFile = g_Config.m_ClLanguagefile[0] != '\\0' ? g_Config.m_ClLanguagefile : \"english.txt\";"), std::string::npos);
	EXPECT_EQ(Source.find("const char *pQmLanguageFile = pLanguageFile[0] != '\\0' ? pLanguageFile : \"english.txt\";"), std::string::npos);
	EXPECT_NE(Source.find("if(str_comp(pLanguageFile, \"languages/simplified_chinese.txt\") == 0)"), std::string::npos);
	EXPECT_NE(Source.find("const char *pQmLanguageFile = pLanguageFile[0] != '\\0' ? pLanguageFile : \"languages/english.txt\";"), std::string::npos);
	EXPECT_NE(Source.find("str_format(aBuf, sizeof(aBuf), \"qmclient/%s\", pQmLanguageFile);"), std::string::npos);
	EXPECT_NE(Source.find("LoadQmClientLanguageOverlay(g_Localization, g_Config.m_ClLanguagefile, Storage(), Console());"), std::string::npos);
}

TEST(QmNewUiMenuBranches, QmClientUpdateFlowUsesQmClientNamingAndComparisonHelper)
{
	const std::string TClientSource = ReadTextFile("src/game/client/components/tclient/tclient.cpp");
	const std::string TClientHeader = ReadTextFile("src/game/client/components/tclient/tclient.h");
	const std::string MenusStartSource = ReadTextFile("src/game/client/components/menus_start.cpp");

	EXPECT_NE(TClientSource.find("#include <game/client/components/qmclient/update_version.h>"), std::string::npos);
	EXPECT_NE(TClientSource.find("static constexpr const char *QMCLIENT_INFO_URL"), std::string::npos);
	EXPECT_NE(TClientSource.find("static constexpr const char *QMCLIENT_UPDATE_EXE_URL"), std::string::npos);
	EXPECT_NE(TClientSource.find("FetchQmClientUpdateInfo();"), std::string::npos);
	EXPECT_NE(TClientSource.find("FinishQmClientUpdateInfo();"), std::string::npos);
	EXPECT_NE(TClientSource.find("ResetQmClientUpdateInfoTask();"), std::string::npos);
	EXPECT_NE(TClientSource.find("NeedQmClientUpdate()"), std::string::npos);
	EXPECT_NE(TClientSource.find("RequestQmClientUpdateCheckAndUpdate()"), std::string::npos);
	EXPECT_NE(TClientSource.find("IsQmClientRemoteVersionNewer(pLatestVersion, QMCLIENT_VERSION)"), std::string::npos);
	EXPECT_EQ(TClientSource.find("NeedUpdate()"), std::string::npos);
	EXPECT_EQ(TClientSource.find("FetchTClientInfo()"), std::string::npos);
	EXPECT_EQ(TClientSource.find("FinishTClientInfo()"), std::string::npos);
	EXPECT_EQ(TClientSource.find("ResetTClientInfoTask()"), std::string::npos);
	EXPECT_EQ(TClientSource.find("TCLIENT_INFO_URL"), std::string::npos);
	EXPECT_EQ(TClientSource.find("TCLIENT_UPDATE_EXE_URL"), std::string::npos);

	EXPECT_NE(TClientHeader.find("m_pQmClientUpdateInfoTask"), std::string::npos);
	EXPECT_NE(TClientHeader.find("m_FetchedQmClientUpdateInfo"), std::string::npos);
	EXPECT_NE(TClientHeader.find("m_QmClientAutoUpdateAfterCheck"), std::string::npos);
	EXPECT_NE(TClientHeader.find("m_aQmClientLatestVersionStr"), std::string::npos);
	EXPECT_EQ(TClientHeader.find("m_pTClientInfoTask"), std::string::npos);
	EXPECT_EQ(TClientHeader.find("m_FetchedTClientInfo"), std::string::npos);
	EXPECT_EQ(TClientHeader.find("m_AutoUpdateAfterCheck"), std::string::npos);
	EXPECT_EQ(TClientHeader.find("m_aVersionStr"), std::string::npos);

	EXPECT_NE(MenusStartSource.find("m_FetchedQmClientUpdateInfo"), std::string::npos);
	EXPECT_NE(MenusStartSource.find("NeedQmClientUpdate()"), std::string::npos);
	EXPECT_EQ(MenusStartSource.find("m_FetchedTClientInfo"), std::string::npos);
	EXPECT_EQ(MenusStartSource.find("NeedUpdate()"), std::string::npos);
}

TEST(QmNewUiMenuBranches, AssetsPreviewUsesInnerFrameRectForPreviewImage)
{
	const std::string Source = ReadTextFile("src/game/client/components/menus_settings_assets.cpp");

	EXPECT_NE(Source.find("auto DrawPreviewFrame = [&](const CUIRect &TextureRect) -> CUIRect {"), std::string::npos);
	EXPECT_NE(Source.find("PreviewFrame.Margin(3.0f, &PreviewFrame);"), std::string::npos);
	EXPECT_NE(Source.find("return PreviewFrame;"), std::string::npos);
	EXPECT_NE(Source.find("auto ComputeAssetPreviewContentSize = [&](bool WorkshopCard)"), std::string::npos);
	EXPECT_NE(Source.find("const CUIRect PreviewFrameRect = DrawPreviewFrame(HeaderLayout.m_TextureRect);"), std::string::npos);
	EXPECT_NE(Source.find("const auto [PreviewContentWidth, PreviewContentHeight] = ComputeAssetPreviewContentSize(false);"), std::string::npos);
	EXPECT_NE(Source.find("const auto [PreviewContentWidth, PreviewContentHeight] = ComputeAssetPreviewContentSize(true);"), std::string::npos);
	EXPECT_NE(Source.find("const CUIRect PreviewRect = ComputePreviewDrawRect(PreviewFrameRect, PreviewContentWidth, PreviewContentHeight);"), std::string::npos);
	EXPECT_EQ(Source.find("const CUIRect PreviewRect = ComputePreviewDrawRect(HeaderLayout.m_TextureRect, TextureWidth, TextureHeight);"), std::string::npos);
	EXPECT_EQ(Source.find("const CUIRect PreviewRect = ComputePreviewDrawRect(HeaderLayout.m_TextureRect, TextureWidth, TextureWidth);"), std::string::npos);
}

TEST(QmNewUiMenuBranches, SettingsColorLabelsUseChineseText)
{
	const std::string Source = ReadTextFile("src/game/client/components/menus_settings.cpp");

	EXPECT_EQ(Source.find("Localize(\"UI Color\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Menu panel color\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Menu panel opacity\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Menu panel elevated opacity\")"), std::string::npos);
	EXPECT_NE(Source.find("DoLine_ColorPicker(&s_UiColorResetId"), std::string::npos);
	EXPECT_NE(Source.find("DoLine_ColorPicker(&s_MenuPanelColorResetId"), std::string::npos);
	EXPECT_NE(Source.find("g_Config.m_ClMenuPanelOpacity"), std::string::npos);
	EXPECT_NE(Source.find("g_Config.m_ClSettingsTabbarOpacity"), std::string::npos);
	EXPECT_NE(Source.find("g_Config.m_ClMenuPanelElevatedOpacity, &g_Config.m_ClMenuPanelElevatedOpacity"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"菜单强调面板透明度\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"设置栏透明度\")"), std::string::npos);
}

TEST(QmNewUiMenuBranches, SettingsGraphicsOpacitySlidersExposeElevatedAndTabbarControls)
{
	const std::string Source = ReadTextFile("src/game/client/components/menus_settings.cpp");

	EXPECT_NE(Source.find("DoScrollbarOption(&g_Config.m_ClMenuPanelOpacity, &g_Config.m_ClMenuPanelOpacity, &Button, Localize(\"菜单面板透明度\")"), std::string::npos);
	EXPECT_NE(Source.find("DoScrollbarOption(&g_Config.m_ClMenuPanelElevatedOpacity, &g_Config.m_ClMenuPanelElevatedOpacity, &Button, Localize(\"菜单强调面板透明度\")"), std::string::npos);
	EXPECT_NE(Source.find("DoScrollbarOption(&g_Config.m_ClSettingsTabbarOpacity, &g_Config.m_ClSettingsTabbarOpacity, &Button, Localize(\"设置栏透明度\")"), std::string::npos);
}

TEST(QmNewUiMenuBranches, IngameMenuPrimaryActionLabelsUseChineseText)
{
	const std::string Source = ReadTextFile("src/game/client/components/menus_ingame.cpp");

	EXPECT_EQ(Source.find("Localize(\"Disconnect\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Edit HUD\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Stop record\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Record demo\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Save last %d min\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Mark demo\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Join red\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Join blue\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Join game\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Kill\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Pause\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Stop practice\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Fast practice\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Please wait…\")"), std::string::npos);

	EXPECT_NE(Source.find("Localize(\"断开连接\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"编辑 HUD\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"停止录制\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"录制 Demo\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"保存最近 %d 分钟\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"标记 Demo\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"加入红队\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"加入蓝队\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"加入游戏\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"自杀\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"暂停\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"结束练习\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"快速练习\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"请稍候…\")"), std::string::npos);
}

TEST(QmNewUiMenuBranches, TClientAxiomStatusPromptsUseChineseText)
{
	const std::string Source = ReadTextFile("src/game/client/components/tclient/tclient.cpp");

	EXPECT_EQ(Source.find("Localize(\"Attempting Axiom auto login\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Axiom auto login succeeded\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Axiom auto login failed, retrying\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Axiom auto login failed\")"), std::string::npos);

	EXPECT_NE(Source.find("Localize(\"正在尝试 Axiom 自动登录\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"Axiom 自动登录成功\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"Axiom 自动登录失败，正在重试\")"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"Axiom 自动登录失败\")"), std::string::npos);
	EXPECT_EQ(Source.find("Localize(\"Axiom 分身自动登录成功\")"), std::string::npos);
}

TEST(QmNewUiMenuBranches, QmClientLanguageReadmeDescribesChineseSourceKeys)
{
	const std::string Readme = ReadTextFile("data/qmclient/languages/README.txt");

	EXPECT_EQ(Readme.find("English keys preserved"), std::string::npos);
	EXPECT_NE(Readme.find("中文 key"), std::string::npos);
	EXPECT_NE(Readme.find("简体中文环境"), std::string::npos);
	EXPECT_NE(Readme.find("直接使用源码中的中文 key"), std::string::npos);
}

TEST(QmNewUiMenuBranches, KcpLogUsesBoundedFormatting)
{
	const std::string Source = ReadTextFile("src/engine/external/kcp/ikcp.c");

	EXPECT_EQ(Source.find("vsprintf(buffer, fmt, argptr);"), std::string::npos);
	EXPECT_NE(Source.find("vsnprintf(buffer, sizeof(buffer), fmt, argptr);"), std::string::npos);
}

TEST(QmNewUiMenuBranches, DisplayChangedDoesNotUseDisplayUnionData)
{
	const std::string Source = ReadTextFile("src/engine/client/input.cpp");
	const size_t CaseStart = Source.find("case SDL_WINDOWEVENT_DISPLAY_CHANGED:");
	ASSERT_NE(CaseStart, std::string::npos);
	const size_t Break = Source.find("break;", CaseStart);
	ASSERT_NE(Break, std::string::npos);
	const std::string Body = Source.substr(CaseStart, Break - CaseStart);

	EXPECT_EQ(Body.find("Event.display.data1"), std::string::npos);
	EXPECT_NE(Body.find("Event.window.data1"), std::string::npos);
	EXPECT_NE(Body.find("Graphics()->SwitchWindowScreen(DisplayIndex, false);"), std::string::npos);
}

TEST(QmNewUiMenuBranches, ImplausibleRefreshRatesAreNotPersisted)
{
	const std::string Backend = ReadTextFile("src/engine/client/backend_sdl.cpp");
	const std::string Graphics = ReadTextFile("src/engine/client/graphics_threaded.cpp");

	EXPECT_NE(Backend.find("static bool IsPlausibleRefreshRate(int RefreshRate)"), std::string::npos);
	EXPECT_NE(Backend.find("static bool IsPlausibleWindowSize(int Width, int Height)"), std::string::npos);
	EXPECT_NE(Backend.find("Ignoring implausible configured window size"), std::string::npos);
	EXPECT_NE(Backend.find("*pWidth = DisplayMode.w;"), std::string::npos);
	EXPECT_NE(Backend.find("*pHeight = DisplayMode.h;"), std::string::npos);
	EXPECT_NE(Backend.find("Ignoring implausible configured refresh rate"), std::string::npos);
	EXPECT_NE(Backend.find("*pRefreshRate = 0;"), std::string::npos);
	EXPECT_NE(Graphics.find("static bool IsPlausibleWindowRefreshRate(int RefreshRate)"), std::string::npos);
	EXPECT_NE(Graphics.find("Ignoring implausible refresh rate during resize"), std::string::npos);
	EXPECT_NE(Graphics.find("RefreshRate = m_ScreenRefreshRate;"), std::string::npos);
	EXPECT_NE(Graphics.find("static bool IsPlausibleWindowSize(int Width, int Height)"), std::string::npos);
	EXPECT_NE(Graphics.find("static int LogicalWindowSizeFromViewport(int ViewportSize, float HiDPIScale)"), std::string::npos);
	EXPECT_NE(Graphics.find("Ignoring implausible resize dimensions"), std::string::npos);
	EXPECT_NE(Graphics.find("if(IsPlausibleWindowSize(g_Config.m_GfxScreenWidth, g_Config.m_GfxScreenHeight))"), std::string::npos);
	EXPECT_EQ(Graphics.find("w = g_Config.m_GfxScreenWidth > 0 ? g_Config.m_GfxScreenWidth : m_ScreenWidth;"), std::string::npos);
	EXPECT_EQ(Graphics.find("h = g_Config.m_GfxScreenHeight > 0 ? g_Config.m_GfxScreenHeight : m_ScreenHeight;"), std::string::npos);
	EXPECT_NE(Graphics.find("w = LogicalWindowSizeFromViewport(m_ScreenWidth, m_ScreenHiDPIScale);"), std::string::npos);
	EXPECT_NE(Graphics.find("h = LogicalWindowSizeFromViewport(m_ScreenHeight, m_ScreenHiDPIScale);"), std::string::npos);
}
