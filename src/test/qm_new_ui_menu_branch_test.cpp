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
	const size_t NextFunction = Source.find("\n}\n\nvoid CMenus::RenderMenubar", BodyStart);
	EXPECT_NE(NextFunction, std::string::npos) << Signature;
	return Source.substr(BodyStart, NextFunction - BodyStart);
}

} // namespace

TEST(QmNewUiMenuBranches, MenubarUsesExplicitQmNewUiColorBranch)
{
	const std::string Source = ReadTextFile("src/game/client/components/menus.cpp");
	const std::string DoMenuTabV2 = FunctionBody(Source, "int CMenus::DoMenuTabV2(");
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
	EXPECT_NE(Source.find("UseNewUi ? &IconButtonActive : nullptr"), std::string::npos);
	EXPECT_NE(Source.find("UseNewUi ? &IconButtonHover : nullptr"), std::string::npos);
	EXPECT_NE(Source.find("ColorRGBA InactiveColor = UseNewUi ? MenuTabDefaultColor() : ms_ColorTabbarInactive;"), std::string::npos);
	EXPECT_NE(Source.find("ColorRGBA ActiveColor = UseNewUi ? MenuTabActiveColor() : ms_ColorTabbarActive;"), std::string::npos);
	EXPECT_NE(Source.find("ColorRGBA HoverColor = UseNewUi ? MenuTabHoverColor() : ms_ColorTabbarHover;"), std::string::npos);
	EXPECT_NE(Source.find("const ColorRGBA IndicatorColor = g_Config.m_QmNewUi != 0 ? MenuUiColorAccent(1.0f) : ui_token::color::ACCENT_PRIMARY;"), std::string::npos);
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
	EXPECT_NE(Source.find("ServerListBase.Margin(std::clamp(ServerListBase.w * 0.006f, 1.0f, 4.0f), &ServerListBase);"), std::string::npos);
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
	EXPECT_EQ(Source.find("g_Config.m_ClMenuPanelElevatedOpacity, &g_Config.m_ClMenuPanelElevatedOpacity"), std::string::npos);
	EXPECT_NE(Source.find("Localize(\"设置栏透明度\")"), std::string::npos);
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
