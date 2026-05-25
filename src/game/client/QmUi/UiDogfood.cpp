/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "UiDogfood.h"

#include <engine/graphics.h>
#include <engine/textrender.h>

#include <game/client/lineinput.h>
#include <game/client/ui.h>
#include <game/client/ui_rect.h>

#include "UiButtons.h"
#include "UiContainers.h"
#include "UiForms.h"
#include "UiNavigation.h"
#include "UiOverlays.h"
#include "UiTokens.h"

namespace
{
const char *const kTabLabels[] = {"Overview", "Forms", "Data"};

CButtonContainer s_PrimaryBtn;
CButtonContainer s_SecondaryBtn;
CButtonContainer s_DisabledBtn;
CButtonContainer s_IconBtn;
CLineInputBuffered<128> s_TextField;
bool s_ToggleOn = true;
bool s_ToggleOff = false;
float s_SliderValue = 0.4f;
int s_TabActive = 0;
int s_ListSelected = 1;
bool s_ModalOpen = false;
CButtonContainer s_ModalOpenBtn;
CButtonContainer s_TooltipBtn;
} // namespace

void RenderQmUiDogfood(const IUiContext &Ctx, const CUIRect &Rect)
{
	if(Ctx.m_pUi == nullptr)
		return;

	// Header strip
	CUIRect Header, Body;
	Rect.HSplitTop(36.0f, &Header, &Body);
	Ctx.m_pUi->DoLabel(&Header, "feat-003 dogfood: tokens + 11 widgets", ui_token::font::HEADLINE_LG, TEXTALIGN_ML);

	// Two columns: 1x and 0.78x to verify UiScale-style downscale
	CUIRect ColLeft, ColRight;
	Body.VSplitMid(&ColLeft, &ColRight);
	ColLeft.VSplitRight(ui_token::spacing::SM, &ColLeft, nullptr);
	ColRight.VSplitLeft(ui_token::spacing::SM, nullptr, &ColRight);

	auto RenderColumn = [&](const CUIRect &Col, float Scale) {
		// Card with all the controls inside
		ui_widget::SCardProps Card;
		Card.m_pTitle = Scale > 0.9f ? "Native scale (1.0)" : "Downscaled (0.78)";
		Card.m_TitleFontSize = ui_token::font::HEADLINE * Scale;
		Card.m_Padding = ui_token::spacing::MD * Scale;

		ui_widget::DrawCard(Ctx, Col, Card, [&](CUIRect &Content) {
			const float RowH = 28.0f * Scale;
			const float Gap = ui_token::spacing::SM * Scale;
			CUIRect Row;

			// Row 1: Primary + Secondary buttons
			Content.HSplitTop(RowH, &Row, &Content);
			{
				CUIRect L, R;
				Row.VSplitMid(&L, &R);
				L.VSplitRight(Gap * 0.5f, &L, nullptr);
				R.VSplitLeft(Gap * 0.5f, nullptr, &R);
				if(Scale > 0.9f)
				{
					ui_widget::PrimaryButton(Ctx, &s_PrimaryBtn, "Primary", L);
					ui_widget::SecondaryButton(Ctx, &s_SecondaryBtn, "Secondary", R);
				}
				else
				{
					// Reuse same containers — already shown above; just paint
					// the scaled column with the same statics. Hover state
					// shares which is fine for dogfood.
					ui_widget::PrimaryButton(Ctx, &s_PrimaryBtn, "Primary", L);
					ui_widget::SecondaryButton(Ctx, &s_SecondaryBtn, "Secondary", R);
				}
			}
			Content.HSplitTop(Gap, nullptr, &Content);

			// Row 2: Disabled + Icon button
			Content.HSplitTop(RowH, &Row, &Content);
			{
				CUIRect L, R;
				Row.VSplitMid(&L, &R);
				L.VSplitRight(Gap * 0.5f, &L, nullptr);
				R.VSplitLeft(Gap * 0.5f, nullptr, &R);
				ui_widget::PrimaryButton(Ctx, &s_DisabledBtn, "Disabled", L, true);
				ui_widget::IconButton(Ctx, &s_IconBtn, "\xEF\x80\x85", R); // FONT_ICON_STAR
			}
			Content.HSplitTop(Gap, nullptr, &Content);

			// Row 3: TextField
			Content.HSplitTop(RowH, &Row, &Content);
			ui_widget::TextField(Ctx, &s_TextField, Row, "Type something...", ui_token::font::BODY * Scale);
			Content.HSplitTop(Gap, nullptr, &Content);

			// Row 4: Two toggles
			Content.HSplitTop(RowH, &Row, &Content);
			{
				CUIRect L, R;
				Row.VSplitMid(&L, &R);
				L.VSplitRight(Gap * 0.5f, &L, nullptr);
				R.VSplitLeft(Gap * 0.5f, nullptr, &R);
				ui_widget::Toggle(Ctx, &s_ToggleOn, &s_ToggleOn, L);
				ui_widget::Toggle(Ctx, &s_ToggleOff, &s_ToggleOff, R);
			}
			Content.HSplitTop(Gap, nullptr, &Content);

			// Row 5: Slider
			Content.HSplitTop(RowH, &Row, &Content);
			ui_widget::Slider(Ctx, &s_SliderValue, &s_SliderValue, 0.0f, 1.0f, Row, "");
			Content.HSplitTop(Gap, nullptr, &Content);

			// Row 6: TabBar
			Content.HSplitTop(RowH, &Row, &Content);
			ui_widget::TabBar(Ctx, kTabLabels, 3, &s_TabActive, Row);
			Content.HSplitTop(Gap, nullptr, &Content);

			// Rows 7-9: List items
			for(int i = 0; i < 3; ++i)
			{
				Content.HSplitTop(RowH, &Row, &Content);
				char aLabel[32];
				str_format(aLabel, sizeof(aLabel), "Item %d", i + 1);
				ui_widget::SListItemProps ItemProps;
				ItemProps.m_Selected = (s_ListSelected == i);
				ItemProps.m_pTrailingText = (i == 1) ? "new" : nullptr;
				if(ui_widget::ListItem(Ctx, (const void *)(uintptr_t)(0x100 + i), aLabel, Row, ItemProps))
					s_ListSelected = i;
				Content.HSplitTop(2.0f, nullptr, &Content);
			}
			Content.HSplitTop(Gap, nullptr, &Content);

			// Row 10: Modal trigger + Tooltip target
			Content.HSplitTop(RowH, &Row, &Content);
			{
				CUIRect L, R;
				Row.VSplitMid(&L, &R);
				L.VSplitRight(Gap * 0.5f, &L, nullptr);
				R.VSplitLeft(Gap * 0.5f, nullptr, &R);
				if(ui_widget::PrimaryButton(Ctx, &s_ModalOpenBtn, "Open modal", L))
					s_ModalOpen = true;
				ui_widget::SecondaryButton(Ctx, &s_TooltipBtn, "Hover for tooltip", R);
				ui_widget::Tooltip(Ctx, &s_TooltipBtn, R, "This tooltip is provided by ui_widget::Tooltip — a thin shim over CTooltips.");
			}
		});
	};

	RenderColumn(ColLeft, 1.0f);
	RenderColumn(ColRight, 0.78f);

	// Modal — rendered on top of everything
	ui_widget::SModalProps ModalProps;
	ModalProps.m_pTitle = "Hello from ui_widget::Modal";
	ModalProps.m_Width = 360.0f;
	ModalProps.m_Height = 180.0f;
	ui_widget::Modal(Ctx, &s_ModalOpen, &s_ModalOpen, Rect, ModalProps, [&](CUIRect &Content) {
		Ctx.m_pUi->DoLabel(&Content, "Press ESC to close. Watch the scale-in spring on open.", ui_token::font::BODY, TEXTALIGN_TL);
	});
}
