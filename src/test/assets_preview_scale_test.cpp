#include <game/client/components/assets_preview_scale.h>

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

TEST(AssetsPreviewScale, LocalPreviewCapKeeps4096ImagesAtNativeResolution)
{
	const auto Scaled = ComputePreviewTargetSize(4096, 2048, LOCAL_ASSET_PREVIEW_MAX_TEXTURE_SIZE);
	EXPECT_EQ(Scaled.m_Width, 4096);
	EXPECT_EQ(Scaled.m_Height, 2048);
	EXPECT_FALSE(Scaled.m_Resized);
}

TEST(AssetsPreviewScale, WorkshopPreviewCapKeeps2048ImagesAtNativeResolution)
{
	const auto Scaled = ComputePreviewTargetSize(2048, 1024, WORKSHOP_ASSET_PREVIEW_MAX_TEXTURE_SIZE);
	EXPECT_EQ(Scaled.m_Width, 2048);
	EXPECT_EQ(Scaled.m_Height, 1024);
	EXPECT_FALSE(Scaled.m_Resized);
}

TEST(AssetsPreviewScale, LocalPreviewStillClampsTo4096)
{
	const auto Scaled = ComputePreviewTargetSize(8192, 4096, LOCAL_ASSET_PREVIEW_MAX_TEXTURE_SIZE);
	EXPECT_EQ(Scaled.m_Width, 4096);
	EXPECT_EQ(Scaled.m_Height, 2048);
	EXPECT_TRUE(Scaled.m_Resized);
}

TEST(AssetsPreviewScale, WorkshopPreviewStillClampsTo2048)
{
	const auto Scaled = ComputePreviewTargetSize(4096, 2048, WORKSHOP_ASSET_PREVIEW_MAX_TEXTURE_SIZE);
	EXPECT_EQ(Scaled.m_Width, 2048);
	EXPECT_EQ(Scaled.m_Height, 1024);
	EXPECT_TRUE(Scaled.m_Resized);
}

TEST(AssetsPreviewScale, DoesNotUpscaleSmallImages)
{
	const auto Scaled = ComputePreviewTargetSize(320, 160, LOCAL_ASSET_PREVIEW_MAX_TEXTURE_SIZE);
	EXPECT_EQ(Scaled.m_Width, 320);
	EXPECT_EQ(Scaled.m_Height, 160);
}

TEST(AssetsPreviewScale, WorkshopThumbDecodeResizesOffMainThread)
{
	std::ifstream File("src/game/client/components/menus_settings_assets.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();

	EXPECT_NE(Source.find("CFullAsyncImageLoadJob>(std::move(vPossiblePaths), pStorage, Asset.m_Name.c_str(), IStorage::TYPE_SAVE, WORKSHOP_ASSET_PREVIEW_MAX_TEXTURE_SIZE)"), std::string::npos);
	EXPECT_EQ(Source.find("const SPreviewTargetSize TargetSize = ComputePreviewTargetSize(Result.m_Image.m_Width, Result.m_Image.m_Height, WORKSHOP_ASSET_PREVIEW_MAX_TEXTURE_SIZE);"), std::string::npos);
}
