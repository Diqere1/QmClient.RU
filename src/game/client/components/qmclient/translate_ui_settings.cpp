#include <base/color.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>
#include <game/client/components/menus.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/localization.h>

#include "translate_ui_settings.h"

void NTranslateUiSettings::RenderTranslateUiModule(CMenus *pMenus, CUIRect &CardContent, float LineHeight, float BodySize, float LineSpacing)
{
	static CButtonContainer s_TranslateBtnDisabledId, s_TranslateBtnEnabledId;
	static CButtonContainer s_TranslateMenuBgId, s_TranslateMenuSelectedId, s_TranslateMenuNormalId;

	CUIRect Row;
	CardContent.HSplitTop(LineHeight, &Row, &CardContent);
	pMenus->DoLine_ColorPicker(&s_TranslateBtnDisabledId, LineHeight, BodySize, 0, &Row,
		Localize("按钮 - 禁用"), &g_Config.m_QmTranslateBtnColorDisabled,
		color_cast<ColorRGBA>(ColorHSLA(CConfig::ms_QmTranslateBtnColorDisabled, true)), true, nullptr, true);
	CardContent.HSplitTop(LineSpacing, nullptr, &CardContent);

	CardContent.HSplitTop(LineHeight, &Row, &CardContent);
	pMenus->DoLine_ColorPicker(&s_TranslateBtnEnabledId, LineHeight, BodySize, 0, &Row,
		Localize("按钮 - 启用"), &g_Config.m_QmTranslateBtnColorEnabled,
		color_cast<ColorRGBA>(ColorHSLA(CConfig::ms_QmTranslateBtnColorEnabled, true)), true, nullptr, true);
	CardContent.HSplitTop(LineSpacing, nullptr, &CardContent);

	CardContent.HSplitTop(LineHeight, &Row, &CardContent);
	pMenus->DoLine_ColorPicker(&s_TranslateMenuBgId, LineHeight, BodySize, 0, &Row,
		Localize("菜单背景"), &g_Config.m_QmTranslateMenuBgColor,
		color_cast<ColorRGBA>(ColorHSLA(CConfig::ms_QmTranslateMenuBgColor, true)), true, nullptr, true);
	CardContent.HSplitTop(LineSpacing, nullptr, &CardContent);

	CardContent.HSplitTop(LineHeight, &Row, &CardContent);
	pMenus->DoLine_ColorPicker(&s_TranslateMenuSelectedId, LineHeight, BodySize, 0, &Row,
		Localize("菜单选项 - 选中"), &g_Config.m_QmTranslateMenuOptionSelected,
		color_cast<ColorRGBA>(ColorHSLA(CConfig::ms_QmTranslateMenuOptionSelected, true)), true, nullptr, true);
	CardContent.HSplitTop(LineSpacing, nullptr, &CardContent);

	CardContent.HSplitTop(LineHeight, &Row, &CardContent);
	pMenus->DoLine_ColorPicker(&s_TranslateMenuNormalId, LineHeight, BodySize, 0, &Row,
		Localize("菜单选项 - 普通"), &g_Config.m_QmTranslateMenuOptionNormal,
		color_cast<ColorRGBA>(ColorHSLA(CConfig::ms_QmTranslateMenuOptionNormal, true)), true, nullptr, true);
	CardContent.HSplitTop(LineSpacing, nullptr, &CardContent);
}
