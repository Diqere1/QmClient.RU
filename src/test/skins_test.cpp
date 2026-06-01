#include <engine/gfx/image_loader.h>

#include <generated/client_data.h>

#include <game/client/animstate.h>
#include <game/client/components/skins.h>
#include <game/client/render.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <list>
#include <sstream>

extern CDataContainer *g_pData;

static vec2 ComputeRenderedTeeMid(const CTeeRenderInfo &Info)
{
	const CAnimState *pIdle = CAnimState::GetIdle();
	float AnimScale, BaseSize;
	CRenderTools::GetRenderTeeAnimScaleAndBaseSize(&Info, AnimScale, BaseSize);
	const vec2 BodyPos = vec2(pIdle->GetBody()->m_X, pIdle->GetBody()->m_Y) * AnimScale;
	const float AssumedScale = BaseSize / 64.0f;
	vec2 BodyOffset;
	float BodyWidth, BodyHeight;
	CRenderTools::GetRenderTeeBodySize(pIdle, &Info, BodyOffset, BodyWidth, BodyHeight);
	vec2 FeetOffset;
	float FeetWidth, FeetHeight;
	CRenderTools::GetRenderTeeFeetSize(pIdle, &Info, FeetOffset, FeetWidth, FeetHeight);
	const vec2 FeetPos[2] = {
		vec2(pIdle->GetFrontFoot()->m_X, pIdle->GetFrontFoot()->m_Y) * AnimScale,
		vec2(pIdle->GetBackFoot()->m_X, pIdle->GetBackFoot()->m_Y) * AnimScale,
	};
	float MinX = -32.0f * AssumedScale + BodyPos.x + BodyOffset.x;
	float MaxX = MinX + BodyWidth;
	for(const vec2 &FootPos : FeetPos)
	{
		const float FootMinX = -32.0f * AssumedScale + FootPos.x + FeetOffset.x;
		MinX = minimum(MinX, FootMinX);
		MaxX = maximum(MaxX, FootMinX + FeetWidth);
	}
	float MinY = -32.0f * AssumedScale + BodyPos.y + BodyOffset.y;
	float MaxY = MinY + BodyHeight;
	for(const vec2 &FootPos : FeetPos)
	{
		MaxY = maximum(MaxY, -16.0f * AssumedScale + FootPos.y + FeetOffset.y + FeetHeight);
	}
	return vec2(MinX + (MaxX - MinX) / 2.0f, MinY + (MaxY - MinY) / 2.0f);
}

static void SetBeastLikeMetrics(CTeeRenderInfo &Info)
{
	Info.m_Size = 64.0f;
	Info.m_SkinMetrics.m_Body.m_Width = 80;
	Info.m_SkinMetrics.m_Body.m_Height = 82;
	Info.m_SkinMetrics.m_Body.m_OffsetX = 16;
	Info.m_SkinMetrics.m_Body.m_OffsetY = 14;
	Info.m_SkinMetrics.m_Body.m_MaxWidth = 96;
	Info.m_SkinMetrics.m_Body.m_MaxHeight = 96;
	Info.m_SkinMetrics.m_Feet.m_Width = 44;
	Info.m_SkinMetrics.m_Feet.m_Height = 23;
	Info.m_SkinMetrics.m_Feet.m_OffsetX = 20;
	Info.m_SkinMetrics.m_Feet.m_OffsetY = 9;
	Info.m_SkinMetrics.m_Feet.m_MaxWidth = 64;
	Info.m_SkinMetrics.m_Feet.m_MaxHeight = 32;
}

static void SetTestPixel(CImageInfo &Image, size_t x, size_t y, uint8_t Red, uint8_t Green, uint8_t Blue, uint8_t Alpha)
{
	const size_t Offset = (y * Image.m_Width + x) * Image.PixelSize();
	Image.m_pData[Offset] = Red;
	Image.m_pData[Offset + 1] = Green;
	Image.m_pData[Offset + 2] = Blue;
	Image.m_pData[Offset + 3] = Alpha;
}

static CImageInfo MakeTestSkinImage(size_t Width, size_t Height, CImageInfo::EImageFormat Format = CImageInfo::FORMAT_RGBA)
{
	CImageInfo Image;
	Image.m_Width = Width;
	Image.m_Height = Height;
	Image.m_Format = Format;
	Image.m_pData = static_cast<uint8_t *>(calloc(Image.DataSize(), 1));
	return Image;
}

TEST(Skins, UsageTrackingSkipsAlwaysLoadedStates)
{
	using EState = CSkins::CSkinContainer::EState;

	EXPECT_FALSE(CSkins::CSkinContainer::TracksUsage(EState::PENDING, true));
	EXPECT_FALSE(CSkins::CSkinContainer::TracksUsage(EState::LOADING, true));
	EXPECT_FALSE(CSkins::CSkinContainer::TracksUsage(EState::LOADED, true));

	EXPECT_TRUE(CSkins::CSkinContainer::TracksUsage(EState::PENDING, false));
	EXPECT_TRUE(CSkins::CSkinContainer::TracksUsage(EState::LOADING, false));
	EXPECT_TRUE(CSkins::CSkinContainer::TracksUsage(EState::LOADED, false));
	EXPECT_FALSE(CSkins::CSkinContainer::TracksUsage(EState::UNLOADED, false));
}

TEST(Skins, AlwaysLoadedStateTransitionsNeverTouchUsageList)
{
	using EState = CSkins::CSkinContainer::EState;

	for(const EState State : {EState::PENDING, EState::LOADING, EState::LOADED})
	{
		const auto Clean = CSkins::CSkinContainer::UsageTrackingUpdate(State, true, false);
		EXPECT_FALSE(Clean.m_ShouldTouch);
		EXPECT_FALSE(Clean.m_ShouldErase);

		const auto Polluted = CSkins::CSkinContainer::UsageTrackingUpdate(State, true, true);
		EXPECT_FALSE(Polluted.m_ShouldTouch);
		EXPECT_TRUE(Polluted.m_ShouldErase);
	}
}

TEST(Skins, UsageListEntriesThatCannotBeUnloadedAreDiscarded)
{
	using EState = CSkins::CSkinContainer::EState;

	EXPECT_TRUE(CSkins::CSkinContainer::ShouldDiscardUsageEntryBeforeUnload(false, EState::LOADED, false));
	EXPECT_TRUE(CSkins::CSkinContainer::ShouldDiscardUsageEntryBeforeUnload(true, EState::LOADED, true));
	EXPECT_TRUE(CSkins::CSkinContainer::ShouldDiscardUsageEntryBeforeUnload(true, EState::NOT_FOUND, false));
	EXPECT_FALSE(CSkins::CSkinContainer::ShouldDiscardUsageEntryBeforeUnload(true, EState::PENDING, false));
	EXPECT_FALSE(CSkins::CSkinContainer::ShouldDiscardUsageEntryBeforeUnload(true, EState::LOADING, false));
	EXPECT_FALSE(CSkins::CSkinContainer::ShouldDiscardUsageEntryBeforeUnload(true, EState::LOADED, false));
}

TEST(Skins, OnlyNotFoundTransitionsRequireSkinListRefresh)
{
	using EState = CSkins::CSkinContainer::EState;

	EXPECT_FALSE(CSkins::CSkinContainer::StateChangeRequiresListRefresh(EState::UNLOADED, EState::PENDING));
	EXPECT_FALSE(CSkins::CSkinContainer::StateChangeRequiresListRefresh(EState::PENDING, EState::LOADING));
	EXPECT_FALSE(CSkins::CSkinContainer::StateChangeRequiresListRefresh(EState::LOADING, EState::LOADED));
	EXPECT_FALSE(CSkins::CSkinContainer::StateChangeRequiresListRefresh(EState::ERROR, EState::UNLOADED));

	EXPECT_TRUE(CSkins::CSkinContainer::StateChangeRequiresListRefresh(EState::LOADING, EState::NOT_FOUND));
	EXPECT_TRUE(CSkins::CSkinContainer::StateChangeRequiresListRefresh(EState::NOT_FOUND, EState::PENDING));
}

TEST(Skins, RegularStateTransitionsEnterAndLeaveUsageList)
{
	using EState = CSkins::CSkinContainer::EState;

	for(const EState State : {EState::PENDING, EState::LOADING, EState::LOADED})
	{
		const auto MissingEntry = CSkins::CSkinContainer::UsageTrackingUpdate(State, false, false);
		EXPECT_TRUE(MissingEntry.m_ShouldTouch);
		EXPECT_FALSE(MissingEntry.m_ShouldErase);

		const auto ExistingEntry = CSkins::CSkinContainer::UsageTrackingUpdate(State, false, true);
		EXPECT_FALSE(ExistingEntry.m_ShouldTouch);
		EXPECT_FALSE(ExistingEntry.m_ShouldErase);
	}

	for(const EState State : {EState::UNLOADED, EState::ERROR, EState::NOT_FOUND})
	{
		const auto ExistingEntry = CSkins::CSkinContainer::UsageTrackingUpdate(State, false, true);
		EXPECT_FALSE(ExistingEntry.m_ShouldTouch);
		EXPECT_TRUE(ExistingEntry.m_ShouldErase);
	}
}

TEST(Skins, ImmediateRequestLoadShouldTouchUsageTrackingData)
{
	using EState = CSkins::CSkinContainer::EState;
	const auto MissingEntry = CSkins::CSkinContainer::UsageTrackingUpdate(EState::PENDING, false, false);
	EXPECT_TRUE(MissingEntry.m_ShouldTouch);
	EXPECT_FALSE(MissingEntry.m_ShouldErase);
}

TEST(Skins, BackgroundRequestDoesNotTouchPriorityUsageTrackingData)
{
	using EState = CSkins::CSkinContainer::EState;
	const auto MissingEntry = CSkins::CSkinContainer::UsageTrackingUpdate(EState::PENDING, false, false, ESettingsResourcePriority::BACKGROUND);
	EXPECT_FALSE(MissingEntry.m_ShouldTouch);
	EXPECT_FALSE(MissingEntry.m_ShouldErase);

	const auto ExistingEntry = CSkins::CSkinContainer::UsageTrackingUpdate(EState::LOADED, false, true, ESettingsResourcePriority::BACKGROUND);
	EXPECT_FALSE(ExistingEntry.m_ShouldTouch);
	EXPECT_TRUE(ExistingEntry.m_ShouldErase);
}

TEST(Skins, PriorityRequestsCanReclaimBackgroundLoadingSlots)
{
	std::ifstream File("src/game/client/components/skins.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();

	const size_t ReclaimBackground = Source.find("bool CSkins::ReclaimBackgroundSkinForPriorityRequest");
	ASSERT_NE(ReclaimBackground, std::string::npos);
	const size_t ReclaimBackgroundEnd = Source.find("\n}", ReclaimBackground);
	ASSERT_NE(ReclaimBackgroundEnd, std::string::npos);
	const std::string ReclaimBody = Source.substr(ReclaimBackground, ReclaimBackgroundEnd - ReclaimBackground);
	EXPECT_NE(ReclaimBody.find("CSkinContainer::EState::LOADING"), std::string::npos);
	EXPECT_NE(ReclaimBody.find("m_pLoadJob->Abort()"), std::string::npos);

	const size_t StartLoadJob = Source.find("auto StartLoadJob = [&]");
	ASSERT_NE(StartLoadJob, std::string::npos);
	const size_t StartLoadJobEnd = Source.find("\n\t};", StartLoadJob);
	ASSERT_NE(StartLoadJobEnd, std::string::npos);
	const std::string StartLoadBody = Source.substr(StartLoadJob, StartLoadJobEnd - StartLoadJob);
	EXPECT_NE(StartLoadBody.find("ReclaimBackgroundSkinForPriorityRequest"), std::string::npos);
	EXPECT_NE(StartLoadBody.find("Stats = LoadingStats();"), std::string::npos);
	EXPECT_EQ(StartLoadBody.find("Priority != ESettingsResourcePriority::BACKGROUND"), std::string::npos);
}

TEST(Skins, PreviewCachePinnedSkinsAreExcludedFromUnloadPaths)
{
	std::ifstream File("src/game/client/components/skins.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();

	const size_t UpdateUnload = Source.find("void CSkins::UpdateUnloadSkins");
	ASSERT_NE(UpdateUnload, std::string::npos);
	EXPECT_NE(Source.find("IsSettingsPreviewCachePinned()", UpdateUnload), std::string::npos);

	const size_t ReclaimBackground = Source.find("bool CSkins::ReclaimBackgroundSkinForPriorityRequest");
	ASSERT_NE(ReclaimBackground, std::string::npos);
	EXPECT_NE(Source.find("IsSettingsPreviewCachePinned()", ReclaimBackground), std::string::npos);
}

TEST(Skins, SkinDataPreparationBuildsMergedMetricsWithoutGraphics)
{
	CImageInfo Image = MakeTestSkinImage(64, 32);
	SetTestPixel(Image, 1, 2, 12, 24, 48, 255);
	SetTestPixel(Image, 26, 4, 255, 0, 0, 255);
	SetTestPixel(Image, 28, 6, 255, 0, 0, 255);
	SetTestPixel(Image, 49, 10, 0, 255, 0, 255);
	SetTestPixel(Image, 52, 19, 0, 255, 0, 255);

	const CSkins::SSkinSpriteSpec Body{8, 4, 0, 0, 3, 3};
	const CSkins::SSkinSpriteSpec BodyOutline{8, 4, 3, 0, 3, 3};
	const CSkins::SSkinSpriteSpec Feet{8, 4, 6, 1, 2, 1};
	const CSkins::SSkinSpriteSpec FeetOutline{8, 4, 6, 2, 2, 1};
	CSkins::SSkinDataPlan Plan;

	EXPECT_TRUE(CSkins::BuildSkinDataPlan(Image, Body, BodyOutline, Feet, FeetOutline, Plan));

	EXPECT_EQ(Plan.m_Body.m_Width, 4);
	EXPECT_EQ(Plan.m_Body.m_Height, 5);
	EXPECT_EQ(Plan.m_Body.m_OffsetX, 1);
	EXPECT_EQ(Plan.m_Body.m_OffsetY, 2);
	EXPECT_EQ(Plan.m_Body.m_MaxWidth, 24);
	EXPECT_EQ(Plan.m_Body.m_MaxHeight, 24);
	EXPECT_EQ(Plan.m_Feet.m_Width, 4);
	EXPECT_EQ(Plan.m_Feet.m_Height, 2);
	EXPECT_EQ(Plan.m_Feet.m_OffsetX, 1);
	EXPECT_EQ(Plan.m_Feet.m_OffsetY, 2);
	EXPECT_EQ(Plan.m_Feet.m_MaxWidth, 16);
	EXPECT_EQ(Plan.m_Feet.m_MaxHeight, 8);

	Image.Free();
}

TEST(Skins, SkinDataPreparationUsesOutlineMetricsWhenFillIsEmpty)
{
	CImageInfo Image = MakeTestSkinImage(64, 32);
	SetTestPixel(Image, 27, 5, 255, 0, 0, 255);
	SetTestPixel(Image, 29, 7, 255, 0, 0, 255);
	SetTestPixel(Image, 50, 18, 0, 255, 0, 255);
	SetTestPixel(Image, 53, 21, 0, 255, 0, 255);

	const CSkins::SSkinSpriteSpec Body{8, 4, 0, 0, 3, 3};
	const CSkins::SSkinSpriteSpec BodyOutline{8, 4, 3, 0, 3, 3};
	const CSkins::SSkinSpriteSpec Feet{8, 4, 6, 1, 2, 1};
	const CSkins::SSkinSpriteSpec FeetOutline{8, 4, 6, 2, 2, 1};
	CSkins::SSkinDataPlan Plan;

	EXPECT_TRUE(CSkins::BuildSkinDataPlan(Image, Body, BodyOutline, Feet, FeetOutline, Plan));

	EXPECT_EQ(Plan.m_Body.m_Width, 3);
	EXPECT_EQ(Plan.m_Body.m_Height, 3);
	EXPECT_EQ(Plan.m_Body.m_OffsetX, 3);
	EXPECT_EQ(Plan.m_Body.m_OffsetY, 5);
	EXPECT_EQ(Plan.m_Feet.m_Width, 4);
	EXPECT_EQ(Plan.m_Feet.m_Height, 4);
	EXPECT_EQ(Plan.m_Feet.m_OffsetX, 2);
	EXPECT_EQ(Plan.m_Feet.m_OffsetY, 2);

	Image.Free();
}

TEST(Skins, RenderedTeeOffsetKeepsGlobalHorizontalOffsetStable)
{
	CTeeRenderInfo Info;
	SetBeastLikeMetrics(Info);

	vec2 OffsetToMid;
	CRenderTools::GetRenderTeeOffsetToRenderedTee(CAnimState::GetIdle(), &Info, OffsetToMid);
	const vec2 Mid = ComputeRenderedTeeMid(Info);

	EXPECT_FLOAT_EQ(OffsetToMid.x, 0.0f);
	EXPECT_NEAR(OffsetToMid.y + Mid.y, 0.0f, 0.0001f);
}

TEST(Skins, SkinQueueEntrySixupDataParticipatesInEquality)
{
	CSkins::CSkinQueueEntry Base;
	Base.m_SkinName = "cammostripes";
	Base.m_UseCustomColor = true;
	Base.m_ColorBody = 123;
	Base.m_ColorFeet = 456;
	Base.m_HasSixup = true;
	for(int Part = 0; Part < protocol7::NUM_SKINPARTS; ++Part)
	{
		str_copy(Base.m_aaSixupSkinPartNames[Part], "standard", sizeof(Base.m_aaSixupSkinPartNames[Part]));
		Base.m_aSixupUseCustomColors[Part] = 0;
		Base.m_aSixupSkinPartColors[Part] = Part;
	}

	CSkins::CSkinQueueEntry Same = Base;
	EXPECT_TRUE(Base == Same);

	CSkins::CSkinQueueEntry DifferentPartName = Base;
	str_copy(DifferentPartName.m_aaSixupSkinPartNames[0], "kitty", sizeof(DifferentPartName.m_aaSixupSkinPartNames[0]));
	EXPECT_FALSE(Base == DifferentPartName);

	CSkins::CSkinQueueEntry DifferentUseCustomColor = Base;
	DifferentUseCustomColor.m_aSixupUseCustomColors[1] = 1;
	EXPECT_FALSE(Base == DifferentUseCustomColor);

	CSkins::CSkinQueueEntry DifferentPartColor = Base;
	DifferentPartColor.m_aSixupSkinPartColors[2] = 999;
	EXPECT_FALSE(Base == DifferentPartColor);
}

TEST(Skins, WebPSaveRoundTripPreservesImageShape)
{
	CImageInfo Image = MakeTestSkinImage(4, 4);
	SetTestPixel(Image, 0, 0, 255, 0, 0, 255);
	SetTestPixel(Image, 3, 0, 0, 255, 0, 255);
	SetTestPixel(Image, 0, 3, 0, 0, 255, 255);
	SetTestPixel(Image, 3, 3, 255, 255, 255, 255);

	CByteBufferWriter Writer;
	EXPECT_TRUE(CImageLoader::SaveWebP(Writer, Image));

	CImageInfo Reloaded;
	EXPECT_TRUE(CImageLoader::LoadWebP(Writer.Data(), Writer.Size(), "skins-test-webp", Reloaded));
	EXPECT_EQ(Reloaded.m_Width, 4u);
	EXPECT_EQ(Reloaded.m_Height, 4u);
	EXPECT_EQ(Reloaded.m_Format, CImageInfo::FORMAT_RGBA);

	Reloaded.Free();
	Image.Free();
}

TEST(Skins, TeePreviewLayerFlagsAreDistinctBits)
{
	EXPECT_NE(TEE_PREVIEW_LAYER_BODY_OUTLINE, TEE_PREVIEW_LAYER_BODY);
	EXPECT_NE(TEE_PREVIEW_LAYER_BACK_FEET_OUTLINE, TEE_PREVIEW_LAYER_BACK_FEET);
	EXPECT_NE(TEE_PREVIEW_LAYER_FRONT_FEET_OUTLINE, TEE_PREVIEW_LAYER_FRONT_FEET);
	EXPECT_NE(TEE_PREVIEW_LAYER_OUTLINE, TEE_PREVIEW_LAYER_BODY);
	EXPECT_NE(TEE_PREVIEW_LAYER_BODY, TEE_PREVIEW_LAYER_FEET);
	EXPECT_NE(TEE_PREVIEW_LAYER_FEET, TEE_PREVIEW_LAYER_EYES);
}

TEST(Skins, TeePreviewLayerFlagsDefaultToAllLayers)
{
	EXPECT_EQ(ResolveTeePreviewLayers(0), TEE_PREVIEW_LAYER_ALL);
	EXPECT_TRUE(HasTeePreviewLayer(0, TEE_PREVIEW_LAYER_OUTLINE));
	EXPECT_TRUE(HasTeePreviewLayer(0, TEE_PREVIEW_LAYER_BODY_OUTLINE));
	EXPECT_TRUE(HasTeePreviewLayer(0, TEE_PREVIEW_LAYER_BACK_FEET_OUTLINE));
	EXPECT_TRUE(HasTeePreviewLayer(0, TEE_PREVIEW_LAYER_FRONT_FEET_OUTLINE));
	EXPECT_TRUE(HasTeePreviewLayer(0, TEE_PREVIEW_LAYER_BODY));
	EXPECT_TRUE(HasTeePreviewLayer(0, TEE_PREVIEW_LAYER_FEET));
	EXPECT_TRUE(HasTeePreviewLayer(0, TEE_PREVIEW_LAYER_EYES));
}

TEST(Skins, TeePreviewLayerFlagsRespectExplicitMask)
{
	const int Mask = TEE_PREVIEW_LAYER_BODY | TEE_PREVIEW_LAYER_FEET;
	EXPECT_TRUE(HasTeePreviewLayer(Mask, TEE_PREVIEW_LAYER_BODY));
	EXPECT_TRUE(HasTeePreviewLayer(Mask, TEE_PREVIEW_LAYER_FEET));
	EXPECT_FALSE(HasTeePreviewLayer(Mask, TEE_PREVIEW_LAYER_OUTLINE));
	EXPECT_FALSE(HasTeePreviewLayer(Mask, TEE_PREVIEW_LAYER_EYES));
}

TEST(Skins, TeePreviewLayerOutlineMasksDoNotSelectFillLayers)
{
	const int Mask = TEE_PREVIEW_LAYER_BODY_OUTLINE | TEE_PREVIEW_LAYER_BACK_FEET_OUTLINE;
	EXPECT_TRUE(HasTeePreviewLayer(Mask, TEE_PREVIEW_LAYER_BODY_OUTLINE));
	EXPECT_TRUE(HasTeePreviewLayer(Mask, TEE_PREVIEW_LAYER_BACK_FEET_OUTLINE));
	EXPECT_FALSE(HasTeePreviewLayer(Mask, TEE_PREVIEW_LAYER_BODY));
	EXPECT_FALSE(HasTeePreviewLayer(Mask, TEE_PREVIEW_LAYER_BACK_FEET));
	EXPECT_FALSE(HasTeePreviewLayer(Mask, TEE_PREVIEW_LAYER_FRONT_FEET_OUTLINE));
}
